#include "Settings/ETVideoSettingsSubsystem.h"
#include "Settings/ETGameUserSettings.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/DefaultValueHelper.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogETVideoPreview, Log, All);

// ---- GI生命周期；空闲无Ticker，世界退出只取消当前预览 ----

void UETVideoSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnding = false;
    WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(
        this, &ThisClass::HandleWorldCleanup);
}

void UETVideoSettingsSubsystem::Deinitialize()
{
    bEnding = true;
    FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
    WorldCleanupHandle.Reset();
    // 结束时只申请恢复，不保存未确认候选；关闭进程时无法承诺再观察一次结果。
    const FETVideoPreviewSnapshot Ending = Transaction.GetSnapshot();
    if (Ending.Phase != EETVideoPreviewPhase::Idle)
    {
        Execute({EETVideoPreviewCommandType::RestoreBaseline, Ending.Baseline, Ending.Serial});
    }
    if (TickHandle.IsValid() && !bInsideTicker)
    {
        FTSTicker::RemoveTicker(TickHandle);
    }
    TickHandle.Reset();
    OnChanged.Clear();
    ReleaseContext();
    Super::Deinitialize();
}

bool UETVideoSettingsSubsystem::IsStandaloneContext() const
{
    const UWorld* World = GetWorld();
    return !bEnding && !GIsEditor && World && !World->bIsTearingDown &&
        World->WorldType == EWorldType::Game &&
        World->GetNetMode() == NM_Standalone;
}

bool UETVideoSettingsSubsystem::IsLocalOwner(const UObject* Owner) const
{
    return IsStandaloneContext() && IsValid(Owner) && Owner->GetWorld() == GetWorld();
}

// ---- 真实窗口 / Viewport像素；不使用Requested配置冒充实际基线 ----

bool UETVideoSettingsSubsystem::ReadViewport(
    UGameViewportClient* Viewport, FETVideoMode& Out) const
{
    Out = FETVideoMode{};
    if (!IsValid(Viewport) || !Viewport->Viewport)
    {
        return false;
    }
    const TSharedPtr<SWindow> Window = Viewport->GetWindow();
    if (!Window.IsValid() || !Window->GetNativeWindow().IsValid())
    {
        return false;
    }
    Out.Resolution = Viewport->Viewport->GetSizeXY();
    Out.Mode = FETVideoModeRules::FromNative(Window->GetWindowMode());
    return Out.IsWellFormed();
}

bool UETVideoSettingsSubsystem::GetActualMode(FETVideoMode& Out) const
{
    Out = FETVideoMode{};
    return IsStandaloneContext() && GetGameInstance() &&
        ReadViewport(GetGameInstance()->GetGameViewportClient(), Out);
}

bool UETVideoSettingsSubsystem::ReadBoundActual(FETVideoMode& Out) const
{
    Out = FETVideoMode{};
    UGameViewportClient* Viewport = BoundViewport.Get();
    return GetGameInstance() && Viewport &&
        GetGameInstance()->GetGameViewportClient() == Viewport && BoundWindow.IsValid() &&
        Viewport->GetWindow() == BoundWindow.Pin() && ReadViewport(Viewport, Out);
}

bool UETVideoSettingsSubsystem::GetAvailableResolutions(
    const EETVideoWindowMode Mode, TArray<FIntPoint>& Out) const
{
    Out.Reset();
    FETVideoMode Actual;
    const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    EWindowMode::Type Native;
    if (!IsInGameThread() || !GetActualMode(Actual) || !IsValid(Settings) ||
        !FETVideoModeRules::ToNative(Mode, Native))
    {
        return false;
    }
    const FIntPoint Desktop = Settings->GetDesktopResolution();
    TArray<FIntPoint> PlatformSizes;
    if (Mode == EETVideoWindowMode::Fullscreen)
    {
        UKismetSystemLibrary::GetSupportedFullscreenResolutions(PlatformSizes);
    }
    else if (Mode == EETVideoWindowMode::Windowed)
    {
        UKismetSystemLibrary::GetConvenientWindowedResolutions(PlatformSizes);
    }
    for (const FIntPoint Size : PlatformSizes)
    {
        if (Size.X > 0 && Size.Y > 0 &&
            (Mode != EETVideoWindowMode::Windowed || Desktop.X <= 0 || Desktop.Y <= 0 ||
             (Size.X < Desktop.X && Size.Y < Desktop.Y)))
        {
            Out.AddUnique(Size);
        }
    }
    if (Mode == EETVideoWindowMode::Windowed)
    {
        if (Actual.Mode == EETVideoWindowMode::Windowed)
        {
            Out.AddUnique(Actual.Resolution);
        }
    }
    if (Mode == EETVideoWindowMode::Borderless && Desktop.X > 0 && Desktop.Y > 0)
    {
        Out.AddUnique(Desktop);
    }
    Out.Sort([](const FIntPoint& A, const FIntPoint& B)
    {
        return A.X == B.X ? A.Y < B.Y : A.X < B.X;
    });
    return !Out.IsEmpty();
}

// ---- 对外意图；实际副作用由单一Execute入口执行 ----

uint64 UETVideoSettingsSubsystem::BeginPreview(UObject* Owner, const FETVideoMode& Request)
{
    const TStrongObjectPtr<UETVideoSettingsSubsystem> KeepAlive(this);
    if (!IsInGameThread() || bOperating || bPublishing || !IsLocalOwner(Owner) ||
        Transaction.GetSnapshot().Phase != EETVideoPreviewPhase::Idle)
    {
        return 0;
    }
    FETVideoMode Actual;
    FETVideoMode Candidate;
    TArray<FIntPoint> Allowed;
    UETGameUserSettings* Settings = UETGameUserSettings::GetETGameUserSettings();
    if (!IsValid(Settings) || !GetActualMode(Actual) ||
        !GetAvailableResolutions(Request.Mode, Allowed) ||
        !FETVideoModeRules::TryResolve(Request, Settings->GetDesktopResolution(), Allowed, Candidate))
    {
        return 0;
    }
    {
        const TGuardValue<bool> Guard(bOperating, true);
        if (!Settings->BeginVideoPreviewGuard())
        {
            return 0;
        }
        const FETVideoPreviewCommand Command = Transaction.Begin(Actual, Candidate, FPlatformTime::Seconds());
        if (Command.Type == EETVideoPreviewCommandType::None)
        {
            Settings->EndVideoPreviewGuard();
            return 0;
        }
        PreviewOwner = Owner;
        PreviewWorld = GetWorld();
        BoundViewport = GetGameInstance()->GetGameViewportClient();
        BoundWindow = BoundViewport->GetWindow();
        BoundSettings = Settings;
        Execute(Command);
    }
    EnsureTicker();
    Publish();
    return Transaction.GetSnapshot().Serial;
}

bool UETVideoSettingsSubsystem::ConfirmPreview(UObject* Owner, const uint64 ExpectedSerial)
{
    const TStrongObjectPtr<UETVideoSettingsSubsystem> KeepAlive(this);
    if (!IsInGameThread() || bOperating || bPublishing || !IsLocalOwner(Owner) ||
        PreviewOwner.Get() != Owner || ExpectedSerial != Transaction.GetSnapshot().Serial)
    {
        return false;
    }
    FETVideoMode Actual;
    if (!ReadBoundActual(Actual) || !BoundSettings.IsValid() || !GEngine ||
        GEngine->GetGameUserSettings() != BoundSettings.Get())
    {
        CancelInternal();
        return false;
    }
    bool bConfirmed = false;
    {
        const TGuardValue<bool> Guard(bOperating, true);
        const FETVideoPreviewCommand Command = Transaction.Confirm(
            ExpectedSerial, FPlatformTime::Seconds(), Actual);
        bConfirmed = Command.Type == EETVideoPreviewCommandType::CommitCandidate;
        Execute(Command); // 过期确认返回RestoreBaseline也必须执行。
    }
    EnsureTicker();
    Publish();
    return bConfirmed;
}

void UETVideoSettingsSubsystem::CancelPreview(UObject* Owner, const uint64 ExpectedSerial)
{
    const bool bRecovery = Transaction.GetSnapshot().Phase == EETVideoPreviewPhase::RestoreFailed;
    if (!IsInGameThread() || !IsLocalOwner(Owner) ||
        ExpectedSerial != Transaction.GetSnapshot().Serial ||
        (PreviewOwner.Get() != Owner && !(bRecovery && !PreviewOwner.IsValid())))
    {
        return;
    }
    CancelInternal();
}

void UETVideoSettingsSubsystem::CancelInternal()
{
    const TStrongObjectPtr<UETVideoSettingsSubsystem> KeepAlive(this);
    if (bEnding)
    {
        return;
    }
    if (bOperating || bPublishing)
    {
        bCancelDeferred = true;
        return;
    }
    {
        const TGuardValue<bool> Guard(bOperating, true);
        Execute(Transaction.Cancel(Transaction.GetSnapshot().Serial, FPlatformTime::Seconds()));
    }
    EnsureTicker();
    Publish();
}

void UETVideoSettingsSubsystem::Execute(const FETVideoPreviewCommand& Command)
{
    if (Command.Type == EETVideoPreviewCommandType::None)
    {
        return;
    }
    EWindowMode::Type Native;
    if (!FETVideoModeRules::ToNative(Command.Mode.Mode, Native))
    {
        return;
    }
    if (Command.Type == EETVideoPreviewCommandType::CommitCandidate)
    {
        // Confirm前已重验设置身份。先持有局部强引用，Save的同步回调可移除宿主。
        const TStrongObjectPtr<UETGameUserSettings> Settings(BoundSettings.Get());
        if (!Settings.Get() || bEnding)
        {
            return;
        }
        Settings->CommitVideoPreview(Command.Mode.Resolution, Native);
        UE_LOG(LogETVideoPreview, Display, TEXT("[M3-05B2-02] SaveRequested Serial=%llu %dx%d Mode=%d"),
            Command.Serial, Command.Mode.Resolution.X, Command.Mode.Resolution.Y,
            static_cast<int32>(Command.Mode.Mode));
        return;
    }
    FETVideoMode Actual;
    if (!ReadBoundActual(Actual))
    {
        UE_LOG(LogETVideoPreview, Warning, TEXT("[M3-05B2-02] Window unavailable Serial=%llu Command=%d"),
            Command.Serial, static_cast<int32>(Command.Type));
        return;
    }
    // 请求本身不写设置；派生设置的保护会撤销引擎窗口回调的自动确认。
    UGameUserSettings::RequestResolutionChange(
        Command.Mode.Resolution.X, Command.Mode.Resolution.Y, Native, false);
    // 与引擎ApplyResolutionSettings相同，及时派发r.SetRes；切图加载不能占掉尚未派发的恢复期限。
    IConsoleManager::Get().CallAllConsoleVariableSinks();
    UE_LOG(LogETVideoPreview, Display, TEXT("[M3-05B2-02] WindowRequest Serial=%llu Command=%d %dx%d Mode=%d"),
        Command.Serial, static_cast<int32>(Command.Type), Command.Mode.Resolution.X,
        Command.Mode.Resolution.Y, static_cast<int32>(Command.Mode.Mode));
}

// ---- CoreTicker只在预览 / 恢复期间运行；绝对平台时间不随World暂停 ----

bool UETVideoSettingsSubsystem::NeedsTick() const
{
    const EETVideoPreviewPhase Phase = Transaction.GetSnapshot().Phase;
    return !bEnding && (Phase == EETVideoPreviewPhase::Applying ||
        Phase == EETVideoPreviewPhase::AwaitingConfirmation || Phase == EETVideoPreviewPhase::Restoring);
}

void UETVideoSettingsSubsystem::EnsureTicker()
{
    if (NeedsTick() && !TickHandle.IsValid())
    {
        TickHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &ThisClass::TickPreview));
    }
}

bool UETVideoSettingsSubsystem::TickPreview(float DeltaSeconds)
{
    (void)DeltaSeconds;
    const TStrongObjectPtr<UETVideoSettingsSubsystem> KeepAlive(this);
    const TGuardValue<bool> Inside(bInsideTicker, true);
    if (!bEnding && !bOperating && !bPublishing)
    {
        const EETVideoPreviewPhase Phase = Transaction.GetSnapshot().Phase;
        if ((Phase == EETVideoPreviewPhase::Applying || Phase == EETVideoPreviewPhase::AwaitingConfirmation) &&
            (bCancelDeferred || !PreviewOwner.IsValid() || !PreviewWorld.IsValid() ||
             PreviewWorld.Get() != GetWorld() || PreviewOwner->GetWorld() != PreviewWorld.Get()))
        {
            bCancelDeferred = false;
            CancelInternal();
        }
        {
            const TGuardValue<bool> Guard(bOperating, true);
            FETVideoMode Actual;
            ReadBoundActual(Actual);
            Execute(Transaction.Advance(FPlatformTime::Seconds(), Actual));
        }
        Publish();
    }
    const bool bKeepTicking = NeedsTick();
    if (!bKeepTicking)
    {
        TickHandle.Reset(); // 回调返回false，由CoreTicker移除自身，不在自身执行中等待RemoveTicker。
    }
    return bKeepTicking;
}

double UETVideoSettingsSubsystem::GetRemainingSeconds() const
{
    return Transaction.GetRemainingSeconds(FPlatformTime::Seconds());
}

void UETVideoSettingsSubsystem::Publish()
{
    if (bEnding || bPublishing)
    {
        return;
    }
    const FETVideoPreviewSnapshot Copy = Transaction.GetSnapshot();
    const int32 Remaining = FMath::CeilToInt(GetRemainingSeconds());
    const bool bChanged = !bPublished || Copy.Phase != LastPublished.Phase ||
        Copy.Result != LastPublished.Result || Copy.Serial != LastPublished.Serial || Remaining != LastRemaining;
    if (Copy.Phase == EETVideoPreviewPhase::Idle)
    {
        ReleaseContext();
    }
    if (!bChanged)
    {
        return;
    }
    LastPublished = Copy;
    LastRemaining = Remaining;
    bPublished = true;
    FETVideoMode Actual;
    GetActualMode(Actual);
    FETVideoModeSnapshot Config;
    FETVideoModeRules::Capture(GEngine ? GEngine->GetGameUserSettings() : nullptr, Config);
    UE_LOG(LogETVideoPreview, Display,
        TEXT("[M3-05B2-02] State Serial=%llu Phase=%d Result=%d Remaining=%d Actual=%dx%d Mode=%d Paused=%d")
        TEXT(" ConfigRequested=%dx%d ConfigConfirmed=%dx%d"),
        Copy.Serial, static_cast<int32>(Copy.Phase), static_cast<int32>(Copy.Result), Remaining,
        Actual.Resolution.X, Actual.Resolution.Y, static_cast<int32>(Actual.Mode),
        GetWorld() && UGameplayStatics::IsGamePaused(this) ? 1 : 0,
        Config.Requested.Resolution.X, Config.Requested.Resolution.Y,
        Config.LastConfirmed.Resolution.X, Config.LastConfirmed.Resolution.Y);
    {
        const TGuardValue<bool> Guard(bPublishing, true);
        OnChanged.Broadcast(Copy);
    }
    if (bCancelDeferred && !bEnding && !bOperating)
    {
        bCancelDeferred = false;
        CancelInternal();
    }
}

void UETVideoSettingsSubsystem::ReleaseContext()
{
    if (UETGameUserSettings* Settings = BoundSettings.Get())
    {
        Settings->EndVideoPreviewGuard();
    }
    PreviewOwner.Reset();
    PreviewWorld.Reset();
    BoundViewport.Reset();
    BoundSettings.Reset();
    BoundWindow.Reset();
    bCancelDeferred = false;
}

void UETVideoSettingsSubsystem::HandleWorldCleanup(
    UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    (void)bSessionEnded;
    (void)bCleanupResources;
    if (!bEnding && PreviewWorld.Get() == World)
    {
        CancelInternal();
    }
}

#if !UE_BUILD_SHIPPING
namespace ETVideoPreviewConsole
{
EETVideoWindowMode ParseMode(const FString& Text)
{
    if (Text.Equals(TEXT("windowed"), ESearchCase::IgnoreCase)) return EETVideoWindowMode::Windowed;
    if (Text.Equals(TEXT("borderless"), ESearchCase::IgnoreCase)) return EETVideoWindowMode::Borderless;
    if (Text.Equals(TEXT("fullscreen"), ESearchCase::IgnoreCase)) return EETVideoWindowMode::Fullscreen;
    return EETVideoWindowMode::Invalid;
}

void Run(const TArray<FString>& Args, UWorld* World)
{
    UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
    UETVideoSettingsSubsystem* Service = Instance ? Instance->GetSubsystem<UETVideoSettingsSubsystem>() : nullptr;
    UObject* Owner = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
    if (!Service || !Owner)
    {
        return;
    }
    if (Args.Num() == 2 && Args[0].Equals(TEXT("list"), ESearchCase::IgnoreCase))
    {
        TArray<FIntPoint> Sizes;
        const bool bReady = Service->GetAvailableResolutions(ParseMode(Args[1]), Sizes);
        UE_LOG(LogETVideoPreview, Display, TEXT("[M3-05B2-02] Catalogue Ready=%d Count=%d"), bReady ? 1 : 0, Sizes.Num());
        for (const FIntPoint Size : Sizes)
        {
            UE_LOG(LogETVideoPreview, Display, TEXT("[M3-05B2-02] Size=%dx%d"), Size.X, Size.Y);
        }
        return;
    }
    if (Args.Num() == 4 && Args[0].Equals(TEXT("begin"), ESearchCase::IgnoreCase))
    {
        FETVideoMode Request;
        Request.Mode = ParseMode(Args[1]);
        if (FDefaultValueHelper::ParseInt(Args[2], Request.Resolution.X) &&
            FDefaultValueHelper::ParseInt(Args[3], Request.Resolution.Y))
        {
            const uint64 Serial = Service->BeginPreview(Owner, Request);
            UE_LOG(LogETVideoPreview, Display, TEXT("[M3-05B2-02] Begin Serial=%llu (0 means refused or unchanged)"), Serial);
        }
        return;
    }
    if (Args.Num() == 1 && Args[0].Equals(TEXT("confirm"), ESearchCase::IgnoreCase))
    {
        const bool bAccepted = Service->ConfirmPreview(Owner, Service->GetSnapshot().Serial);
        UE_LOG(LogETVideoPreview, Display, TEXT("[M3-05B2-02] ConfirmAccepted=%d"), bAccepted ? 1 : 0);
    }
    else if (Args.Num() == 1 && Args[0].Equals(TEXT("cancel"), ESearchCase::IgnoreCase))
    {
        Service->CancelPreview(Owner, Service->GetSnapshot().Serial);
    }
    else if (!Args.IsEmpty() && !(Args.Num() == 1 && Args[0].Equals(TEXT("status"), ESearchCase::IgnoreCase)))
    {
        UE_LOG(LogETVideoPreview, Display, TEXT("[M3-05B2-02] Usage: et.VideoPreview [status | list <mode> | begin <mode> <width> <height> | confirm | cancel]"));
        return;
    }
    FETVideoMode Actual;
    const bool bActual = Service->GetActualMode(Actual);
    const FETVideoPreviewSnapshot Snapshot = Service->GetSnapshot();
    UE_LOG(LogETVideoPreview, Display,
        TEXT("[M3-05B2-02] Status Native=%d Serial=%llu Phase=%d Result=%d Remaining=%.2f Actual=%dx%d Mode=%d"),
        bActual ? 1 : 0, Snapshot.Serial, static_cast<int32>(Snapshot.Phase),
        static_cast<int32>(Snapshot.Result), Service->GetRemainingSeconds(),
        Actual.Resolution.X, Actual.Resolution.Y, static_cast<int32>(Actual.Mode));
}

FAutoConsoleCommandWithWorldAndArgs Command(TEXT("et.VideoPreview"),
    TEXT("Standalone-only video preview / confirmation / cancellation."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Run), ECVF_Cheat);
} // namespace ETVideoPreviewConsole
#endif