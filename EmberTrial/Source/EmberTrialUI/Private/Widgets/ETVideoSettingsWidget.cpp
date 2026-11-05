#include "Widgets/ETVideoSettingsWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Settings/ETGameUserSettings.h"
#include "Settings/ETVideoSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogETVideoSettingsView, Log, All);

namespace ETVideoViewPrivate
{
    constexpr float InitialCaptureRetryInterval = 0.1f;
    constexpr double InitialCaptureTimeoutSeconds = 3.0;

FString ModeLabel(const EETVideoWindowMode Mode)
{
    switch (Mode)
    {
    case EETVideoWindowMode::Windowed: return TEXT("窗口");
    case EETVideoWindowMode::Borderless: return TEXT("无边框");
    case EETVideoWindowMode::Fullscreen: return TEXT("独占全屏");
    default: return TEXT("不可用");
    }
}

FText ResultText(const EETVideoPreviewResult Result)
{
    switch (Result)
    {
    case EETVideoPreviewResult::Confirmed: return FText::FromString(TEXT("已确认并请求保存显示设置。"));
    case EETVideoPreviewResult::TimedOut: return FText::FromString(TEXT("确认已超时，已恢复原显示设置。"));
    case EETVideoPreviewResult::Cancelled: return FText::FromString(TEXT("已取消预览并恢复原显示设置。"));
    case EETVideoPreviewResult::ApplyFailed: return FText::FromString(TEXT("候选未能生效，已恢复原显示设置。"));
    case EETVideoPreviewResult::ExternalChange: return FText::FromString(TEXT("显示环境发生变化，已恢复原显示设置。"));
    default: return FText::FromString(TEXT("选择模式和分辨率后预览；确认后才保存。"));
    }
}
} // namespace ETVideoViewPrivate

// ---- 初始化绑定控件；Construct观察服务，Destruct先解绑再取消 ----

bool UETVideoSettingsWidget::HasRequiredLayout() const
{
    return ModeCombo && ResolutionCombo && PreviewButton && RevertButton &&
        ConfirmButton && CancelPreviewButton && CurrentModeText && VideoStatusText;
}

void UETVideoSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout()) return;
    ModeCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleModeChanged);
    ResolutionCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleResolutionChanged);
    PreviewButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePreview);
    RevertButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRevert);
    ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
    CancelPreviewButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCancel);
}

void UETVideoSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsDesignTime() || !HasRequiredLayout()) return;
    bViewActive = true;
    if (UGameInstance* Instance = GetGameInstance())
    {
        Service = Instance->GetSubsystem<UETVideoSettingsSubsystem>();
    }
    if (UETVideoSettingsSubsystem* Current = Service.Get())
    {
        Current->OnChanged.Remove(PreviewChangedHandle);
        PreviewChangedHandle = Current->OnChanged.AddUObject(this, &ThisClass::HandleSnapshot);
    }
    ReadCurrent(ETVideoViewPrivate::ResultText(EETVideoPreviewResult::None));
    StartInitialCaptureRetry();
}

void UETVideoSettingsWidget::NativeDestruct()
{
    CloseView();
    Super::NativeDestruct();
}

void UETVideoSettingsWidget::CloseView()
{
    bViewActive = false;
    StopInitialCaptureRetry();
    if (UETVideoSettingsSubsystem* Current = Service.Get())
    {
        Current->OnChanged.Remove(PreviewChangedHandle);
    }
    PreviewChangedHandle.Reset();
    CancelOwnedPreview();
    Service.Reset();
    Draft.Close();
    Modes.Reset();
    Resolutions.Reset();
}

void UETVideoSettingsWidget::CancelOwnedPreview()
{
    const uint64 OldSerial = PreviewSerial;
    PreviewSerial = 0; // 先封口，取消的同步广播不能让旧View重新认领事务。
    if (UETVideoSettingsSubsystem* Current = Service.Get(); Current && OldSerial != 0)
    {
        Current->CancelPreview(this, OldSerial);
        UE_LOG(LogETVideoSettingsView, Display,
            TEXT("[M3-05B3-02] View cancel Widget=%s Serial=%llu"), *GetName(), OldSerial);
    }
}

void UETVideoSettingsWidget::SetHostBusy(const bool bBusy)
{
    bHostBusy = bBusy;
    if (bBusy) CancelOwnedPreview();
    Render();
}

// ---- 快照与候选；不从配置请求值猜测当前窗口，不拥有世界暂停 ----

bool UETVideoSettingsWidget::HasLiveHost() const
{
    for (const UUserWidget* View = this; View; View = View->GetTypedOuter<UUserWidget>())
    {
        if (View->IsInViewport()) return true;
    }
    return false;
}

bool UETVideoSettingsWidget::CanEdit() const
{
    return bViewActive && !bHostBusy && Draft.IsOpen() && Service.IsValid() &&
        Service->GetSnapshot().Phase == EETVideoPreviewPhase::Idle;
}

bool UETVideoSettingsWidget::Capture(FETVideoMode& Actual, FETVideoResolutionCatalogue& Available) const
{
    Actual = FETVideoMode{};
    Available = FETVideoResolutionCatalogue{};
    const UETVideoSettingsSubsystem* Current = Service.Get();
    FETVideoModeSnapshot Config;
    if (!Current || !Current->GetActualMode(Actual) ||
        !FETVideoModeRules::Capture(UETGameUserSettings::GetETGameUserSettings(), Config))
    {
        return false;
    }
    Available.Desktop = Config.DesktopResolution;
    Current->GetAvailableResolutions(EETVideoWindowMode::Windowed, Available.Windowed);
    Current->GetAvailableResolutions(EETVideoWindowMode::Fullscreen, Available.Fullscreen);
    return Available.Desktop.X > 0 && Available.Desktop.Y > 0;
}

void UETVideoSettingsWidget::ReadCurrent(const FText& Message)
{
    FETVideoMode Actual;
    FETVideoResolutionCatalogue Available;
    const bool bReady = Capture(Actual, Available) && Draft.Open(Actual, Available);
    if (!bReady) Draft.Close();
    Feedback = bReady ? Message : FText::FromString(TEXT("请在独立游戏窗口中调整显示设置。"));
    Render(true);
}

void UETVideoSettingsWidget::StartInitialCaptureRetry()
{
    StopInitialCaptureRetry();

    const UWorld* World = GetWorld();
    if (Draft.IsOpen() || !bViewActive || !Service.IsValid() || GIsEditor ||
        !World || World->WorldType != EWorldType::Game ||
        World->GetNetMode() != NM_Standalone)
    {
        return;
    }

    InitialCaptureDeadlineSeconds = FPlatformTime::Seconds() +
        ETVideoViewPrivate::InitialCaptureTimeoutSeconds;
    InitialCaptureTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &ThisClass::TickInitialCapture),
        ETVideoViewPrivate::InitialCaptureRetryInterval);

    Feedback = FText::FromString(TEXT("显示状态暂不可用，正在重试。"));
    Render();
    UE_LOG(LogETVideoSettingsView, Display,
        TEXT("[M3-05B3-02] Initial capture deferred Widget=%s"), *GetName());
}

void UETVideoSettingsWidget::StopInitialCaptureRetry()
{
    if (InitialCaptureTickerHandle.IsValid() && !bInsideInitialCaptureTick)
    {
        FTSTicker::RemoveTicker(InitialCaptureTickerHandle);
    }
    InitialCaptureTickerHandle.Reset();
    InitialCaptureDeadlineSeconds = 0.0;
}

bool UETVideoSettingsWidget::TickInitialCapture(float)
{
    const TGuardValue<bool> InCallback(bInsideInitialCaptureTick, true);

    // 当前回调返回false自行退出；关闭页面时不会留下旧View观察。
    if (!bViewActive || !Service.IsValid() || !HasLiveHost())
    {
        StopInitialCaptureRetry();
        return false;
    }
    if (Draft.IsOpen())
    {
        StopInitialCaptureRetry();
        return false; // 其他状态事件已完成捕获，不覆盖用户草稿。
    }

    ReadCurrent(ETVideoViewPrivate::ResultText(EETVideoPreviewResult::None));
    if (!bViewActive)
    {
        StopInitialCaptureRetry();
        return false;
    }
    if (Draft.IsOpen())
    {
        StopInitialCaptureRetry();
        UE_LOG(LogETVideoSettingsView, Display,
            TEXT("[M3-05B3-02] Initial capture ready Widget=%s"), *GetName());
        return false;
    }
    if (FPlatformTime::Seconds() >= InitialCaptureDeadlineSeconds)
    {
        StopInitialCaptureRetry();
        Feedback = FText::FromString(TEXT("显示状态读取失败，请重开游戏后重试。"));
        Render();
        UE_LOG(LogETVideoSettingsView, Warning,
            TEXT("[M3-05B3-02] Initial capture timed out Widget=%s"), *GetName());
        return false;
    }
    return true;
}

void UETVideoSettingsWidget::HandleSnapshot(const FETVideoPreviewSnapshot& Snapshot)
{
    if (!bViewActive || bStartingPreview) return;
    if (Snapshot.Phase == EETVideoPreviewPhase::Idle)
    {
        const bool bWasOurs = PreviewSerial != 0 && PreviewSerial == Snapshot.Serial;
        PreviewSerial = 0;
        ReadCurrent(ETVideoViewPrivate::ResultText(Snapshot.Result));
        if (bWasOurs) FocusMode();
    }
    else
    {
        Render(); // 倒计时由Core的状态事件驱动；这里不创建Tick / Timer。
    }
}

// ---- 呈现与焦点；只在目录变化时重建选项，不按倒计时重建下拉框 ----

void UETVideoSettingsWidget::Render(const bool bRebuildChoices)
{
    if (!bViewActive || !HasRequiredLayout()) return;
    const TGuardValue<bool> Sync(bSyncing, true);
    if (bRebuildChoices)
    {
        Draft.GetModes(Modes);
        Draft.GetResolutions(Draft.GetValue().Mode, Resolutions);
        ModeCombo->ClearOptions();
        for (const EETVideoWindowMode Mode : Modes) ModeCombo->AddOption(ETVideoViewPrivate::ModeLabel(Mode));
        ResolutionCombo->ClearOptions();
        for (const FIntPoint Size : Resolutions)
        {
            ResolutionCombo->AddOption(FString::Printf(TEXT("%d × %d"), Size.X, Size.Y));
        }
    }
    ModeCombo->SetSelectedIndex(Modes.IndexOfByKey(Draft.GetValue().Mode));
    ResolutionCombo->SetSelectedIndex(Resolutions.IndexOfByKey(Draft.GetValue().Resolution));

    const UETVideoSettingsSubsystem* Current = Service.Get();
    const FETVideoPreviewSnapshot Snapshot = Current ? Current->GetSnapshot() : FETVideoPreviewSnapshot{};
    const bool bBusy = Snapshot.Phase != EETVideoPreviewPhase::Idle;
    const bool bOurs = PreviewSerial != 0 && PreviewSerial == Snapshot.Serial;
    const bool bEdit = CanEdit();
    ModeCombo->SetIsEnabled(bEdit);
    ResolutionCombo->SetIsEnabled(bEdit && Draft.GetValue().Mode != EETVideoWindowMode::Borderless);
    PreviewButton->SetIsEnabled(bEdit && Draft.IsDirty());
    RevertButton->SetIsEnabled(bEdit && Draft.IsDirty());
    ConfirmButton->SetVisibility(bBusy ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    CancelPreviewButton->SetVisibility(bBusy ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ConfirmButton->SetIsEnabled(!bHostBusy && bOurs && Snapshot.Phase == EETVideoPreviewPhase::AwaitingConfirmation);
    const bool bCanCancel = bOurs && (Snapshot.Phase == EETVideoPreviewPhase::Applying ||
        Snapshot.Phase == EETVideoPreviewPhase::AwaitingConfirmation);
    CancelPreviewButton->SetIsEnabled(!bHostBusy &&
        (bCanCancel || Snapshot.Phase == EETVideoPreviewPhase::RestoreFailed));

    FETVideoMode Actual;
    const bool bActual = Current && Current->GetActualMode(Actual);
    CurrentModeText->SetText(bActual ? FText::FromString(FString::Printf(TEXT("当前：%d × %d · %s"),
        Actual.Resolution.X, Actual.Resolution.Y, *ETVideoViewPrivate::ModeLabel(Actual.Mode))) :
        FText::FromString(TEXT("当前显示状态不可用")));
    FText Message = Feedback;
    if (Snapshot.Phase == EETVideoPreviewPhase::RestoreFailed)
    {
        Message = FText::FromString(TEXT("显示恢复未完成，选择取消预览重试；仍失败请重开游戏。"));
    }
    else if (bBusy && !bOurs)
    {
        Message = FText::FromString(TEXT("正在处理显示变化，请稍候。"));
    }
    else if (bOurs && Snapshot.Phase == EETVideoPreviewPhase::AwaitingConfirmation)
    {
        Message = FText::Format(NSLOCTEXT("ETVideoSettings", "ConfirmCountdown", "是否保留？{0}秒后自动恢复。"),
            FText::AsNumber(FMath::CeilToInt(Current->GetRemainingSeconds())));
    }
    else if (bOurs && bBusy)
    {
        Message = FText::FromString(Snapshot.Phase == EETVideoPreviewPhase::Restoring ?
            TEXT("正在恢复原显示设置…") : TEXT("正在应用显示预览…"));
    }
    VideoStatusText->SetText(Message);
}

void UETVideoSettingsWidget::FocusMode()
{
    if (CanEdit() && HasLiveHost() && GetOwningPlayer()) ModeCombo->SetUserFocus(GetOwningPlayer());
}

// ---- 控件只发选择 / 预览 / 确认意图；不直接Apply或Save ----

void UETVideoSettingsWidget::HandleModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;
    const int32 Index = ModeCombo->GetSelectedIndex();
    if (!bSyncing && CanEdit() && Modes.IsValidIndex(Index) && Draft.SelectMode(Modes[Index]))
    {
        Feedback = FText::FromString(TEXT("有未预览的修改。"));
        Render(true);
    }
}

void UETVideoSettingsWidget::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;
    const int32 Index = ResolutionCombo->GetSelectedIndex();
    if (!bSyncing && CanEdit() && Resolutions.IsValidIndex(Index) && Draft.SelectResolution(Resolutions[Index]))
    {
        Feedback = FText::FromString(TEXT("有未预览的修改。"));
        Render();
    }
}

void UETVideoSettingsWidget::HandlePreview()
{
    if (!CanEdit() || !HasLiveHost()) return;
    const TStrongObjectPtr<UETVideoSettingsWidget> KeepAlive(this);
    FETVideoMode Actual, Candidate;
    FETVideoResolutionCatalogue Available;
    if (!Capture(Actual, Available) || !Draft.TryMakePreview(Actual, Available, Candidate))
    {
        ReadCurrent(FText::FromString(TEXT("显示状态或可选项已变化，请重新选择。")));
        FocusMode();
        return;
    }
    UETVideoSettingsSubsystem* Current = Service.Get();
    uint64 Started = 0;
    {
        const TGuardValue<bool> Starting(bStartingPreview, true);
        Started = Current->BeginPreview(this, Candidate); // Begin会同步广播，返回后才认领Serial。
    }
    if (!bViewActive || !HasLiveHost())
    {
        if (Started != 0) Current->CancelPreview(this, Started);
        return;
    }
    PreviewSerial = Started;
    UE_LOG(LogETVideoSettingsView, Display,
        TEXT("[M3-05B3-02] View begin Widget=%s Serial=%llu"), *GetName(), Started);
    if (Started == 0)
    {
        ReadCurrent(FText::FromString(TEXT("未能开始预览，请重新选择。")));
        FocusMode();
        return;
    }
    HandleSnapshot(Current->GetSnapshot());
    if (Started != 0 && CancelPreviewButton->GetIsEnabled() && GetOwningPlayer())
    {
        CancelPreviewButton->SetUserFocus(GetOwningPlayer());
    }
}

void UETVideoSettingsWidget::HandleRevert()
{
    if (!CanEdit()) return;
    ReadCurrent(FText::FromString(TEXT("已取消修改，保留当前显示设置。")));
    FocusMode();
}

void UETVideoSettingsWidget::HandleConfirm()
{
    if (!bViewActive || bHostBusy || !HasLiveHost() || PreviewSerial == 0 || !Service.IsValid()) return;
    const TStrongObjectPtr<UETVideoSettingsWidget> KeepAlive(this);
    Service->ConfirmPreview(this, PreviewSerial); // 旧页不能取当前全局Serial冒充自己的请求。
}

void UETVideoSettingsWidget::HandleCancel()
{
    if (!bViewActive || bHostBusy || !HasLiveHost() || !Service.IsValid()) return;
    const FETVideoPreviewSnapshot Snapshot = Service->GetSnapshot();
    // 恢复失败时允许显式请求重试；Core仍检查原Owner是否有效以及Serial。
    const uint64 Serial = PreviewSerial != 0 ? PreviewSerial :
        (Snapshot.Phase == EETVideoPreviewPhase::RestoreFailed ? Snapshot.Serial : 0);
    if (Serial != 0) Service->CancelPreview(this, Serial);
}

FReply UETVideoSettingsWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (HasRequiredLayout() && InKeyEvent.GetKey() == EKeys::Escape &&
        !ModeCombo->IsOpen() && !ResolutionCombo->IsOpen())
    {
        if (PreviewSerial != 0) HandleCancel();
        else HandleRevert();
        return FReply::Handled();
    }
    // 展开的下拉框先自行处理Esc；P继续冒泡给Pause，离开页面会取消本View的预览。
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}