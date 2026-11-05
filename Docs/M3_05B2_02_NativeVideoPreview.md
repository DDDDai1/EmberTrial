# M3-05B2-02：原生视频预览、实时时钟与回退

2026-11-05，UE **5.5.4**。**本章已验收，无待补。**[验收报告](Reports/M3_05B2_02_Verification.md)汇总双Build / 两Success与独立窗口实测；当前进入[B3-01显示设置草稿](M3_05B3_01_VideoSettingsDraft.md)。以下实现步骤保留供复查，不要求重做。

## 1. 本章交付与约束

把01事务接到当前GameInstance的真实窗口。预览通过RequestResolutionChange申请窗口变化，同时保护配置：**UE的OnViewportResized会自行SetScreenResolution并虚调用ConfirmVideoMode**，因此派生设置类在预览期间恢复原请求 / 确认字段，保存时同样保护；确认才解除保护并提交新值。实际Viewport尺寸与原生窗口模式匹配后进入确认，取消或15秒到期申请实际基线，再观察是否恢复。

CoreTicker仅在应用 / 确认 / 恢复期间运行，时间来自FPlatformTime::Seconds，不随World暂停。当前世界清理、Owner销毁 / 换世界会取消；恢复失败保留基线，停止自动Ticker并允许显式重试。进程关闭只能尽力申请恢复，不能声称已再次观察成功；未确认候选始终不进配置。

**只允许独立游戏进程真实切屏**（!GIsEditor、Game World、Standalone）；PIE / 编辑器内嵌窗口明确拒绝。主显示器是本批验证范围，跨显示器 / HDR / 刷新率不在本批。新确认UI留B3，这批使用开发控制台。后续页面必须在关闭时主动CancelPreview；Core无法把“Owner对象仍存活”当作“Widget仍在屏幕上”。

实施前预算代码90–140 / 阅读10–15 / 实机10–15分钟；实际代码70 / 阅读10 / 检查15，共95分钟，等待5。现209 Source / 118cpp（Core83 / UI32 / Editor3）、36测试、300 Content / 来源81。

## 2. 文件与准备

UE Save All、停止PIE并关闭，CLion保留。本批**两新 / 六旧Core文件**，无新Widget / Config：

```text
EmberTrial/Source/EmberTrial/Public/Settings/ETVideoSettingsSubsystem.h       新
EmberTrial/Source/EmberTrial/Private/Settings/ETVideoSettingsSubsystem.cpp    新
EmberTrial/Source/EmberTrial/Public/Settings/ETVideoModeRules.h              旧
EmberTrial/Source/EmberTrial/Private/Settings/ETVideoModeRules.cpp           旧
EmberTrial/Source/EmberTrial/Tests/ETVideoModeRulesTests.cpp                 旧
EmberTrial/Source/EmberTrial/EmberTrial.Build.cs                            旧
EmberTrial/Source/EmberTrial/Public/Settings/ETGameUserSettings.h            旧
EmberTrial/Source/EmberTrial/Private/Settings/ETGameUserSettings.cpp          旧
```

路径均相对`E:\UE_Project\Demo`。Subsystem随GameInstance自动实例化，不挂Controller组件，也不创建对应Blueprint。新Subsystem是UCLASS，头的generated.h必须为最后一条include，不手写生成文件；01内核不改。新增ApplicationCore公开依赖用于模式枚举、SlateCore私有依赖用于窗口；显示目录调用Engine现成接口，Core仍不依赖项目UI。

## 3. 六旧文件精确修改

### 3.1 Public/Settings/ETVideoModeRules.h

为公开模式转换函数加入ApplicationCore中的原生窗口枚举头。

查找原文：

```cpp
#include "Containers/ArrayView.h"
```

替换为：

```cpp
#include "Containers/ArrayView.h"
#include "GenericPlatform/GenericWindow.h"
```

### 3.2 Public/Settings/ETVideoModeRules.h

将模式映射集中到B1规则，避免原生适配重复维护两套枚举映射。

查找原文：

```cpp
class EMBERTRIAL_API FETVideoModeRules
{
public:
```

替换为：

```cpp
class EMBERTRIAL_API FETVideoModeRules
{
public:
    static EETVideoWindowMode FromNative(EWindowMode::Type Mode);
    static bool ToNative(EETVideoWindowMode Mode, EWindowMode::Type& Out);
```

### 3.3 Private/Settings/ETVideoModeRules.cpp

把原私有FromNative改成类的公开静态实现，下面switch保留。

查找原文：

```cpp
namespace ETVideoModePrivate
{
EETVideoWindowMode FromNative(const EWindowMode::Type Mode)
```

替换为：

```cpp
EETVideoWindowMode FETVideoModeRules::FromNative(const EWindowMode::Type Mode)
```

### 3.4 Private/Settings/ETVideoModeRules.cpp

原FromNative函数右花括号保留；仅把其后的namespace结束行替换为ToNative整个函数。

查找原文：

```cpp
} // namespace ETVideoModePrivate
```

替换为：

```cpp
bool FETVideoModeRules::ToNative(const EETVideoWindowMode Mode, EWindowMode::Type& Out)
{
    Out = EWindowMode::Windowed;
    switch (Mode)
    {
    case EETVideoWindowMode::Windowed: Out = EWindowMode::Windowed; return true;
    case EETVideoWindowMode::Borderless: Out = EWindowMode::WindowedFullscreen; return true;
    case EETVideoWindowMode::Fullscreen: Out = EWindowMode::Fullscreen; return true;
    default: return false;
    }
}
```

### 3.5 Private/Settings/ETVideoModeRules.cpp

Capture读取请求模式时使用同类静态函数。

查找原文：

```cpp
ETVideoModePrivate::FromNative(Settings->GetFullscreenMode())
```

替换为：

```cpp
FromNative(Settings->GetFullscreenMode())
```

### 3.6 Private/Settings/ETVideoModeRules.cpp

Capture读取确认模式时同样改用同类函数。

查找原文：

```cpp
ETVideoModePrivate::FromNative(Settings->GetLastConfirmedFullscreenMode())
```

替换为：

```cpp
FromNative(Settings->GetLastConfirmedFullscreenMode())
```

### 3.7 Tests/ETVideoModeRulesTests.cpp

在旧专项开头补模式映射断言，原测试与注册保留，不新建重复测试。

查找原文：

```cpp
    (void)Parameters;
    const FIntPoint Desktop
```

替换为：

```cpp
    (void)Parameters;
    for (const EWindowMode::Type Native : {EWindowMode::Windowed,
        EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen})
    {
        EWindowMode::Type Back = EWindowMode::Windowed;
        TestTrue(TEXT("Known mode round trip"),
            FETVideoModeRules::ToNative(FETVideoModeRules::FromNative(Native), Back) && Back == Native);
    }
    EWindowMode::Type Rejected = EWindowMode::Fullscreen;
    TestFalse(TEXT("Invalid project mode rejected"),
        FETVideoModeRules::ToNative(EETVideoWindowMode::Invalid, Rejected));
    TestTrue(TEXT("Rejected native output is reset"), Rejected == EWindowMode::Windowed);
    const FIntPoint Desktop
```

### 3.8 EmberTrial.Build.cs

公开模式转换使用ApplicationCore的EWindowMode，因此显式加入公开依赖。

查找原文：

```cpp
"Core", "CoreUObject", "Engine", "InputCore"
```

替换为：

```cpp
"Core", "CoreUObject", "Engine", "ApplicationCore", "InputCore"
```

### 3.9 EmberTrial.Build.cs

加SlateCore私有依赖用于原生窗口；分辨率目录复用Engine现成接口，不额外直接依赖RHI，Core仍不依赖EmberTrialUI。

查找原文：

```cpp
            "GameplayTasks", "PhysicsCore"
```

替换为：

```cpp
            "GameplayTasks", "PhysicsCore", "SlateCore"
```

### 3.10 Public/Settings/ETGameUserSettings.h

拦截引擎自动确认和保存，防止预览分辨率经其他入口带入配置。

查找原文：

```cpp
    virtual void SetToDefaults() override;
```

替换为：

```cpp
    virtual void SetToDefaults() override;
    virtual void ConfirmVideoMode() override;
    virtual void SaveSettings() override;
```

### 3.11 Public/Settings/ETGameUserSettings.h

在private开头加内部保护合同，InputPreferences原字段保留；不新增公开蓝图控制入口。

查找原文：

```cpp
private:
    UPROPERTY(Config)
```

替换为：

```cpp
private:
    friend class UETVideoSettingsSubsystem;
    friend class FETVideoModeRulesTest;

    bool BeginVideoPreviewGuard();
    void RestoreGuardedVideo();
    void EndVideoPreviewGuard();
    void CommitVideoPreview(FIntPoint Resolution, EWindowMode::Type Mode);

    struct FVideoConfigState
    {
        FIntPoint Requested = FIntPoint::ZeroValue;
        FIntPoint Confirmed = FIntPoint::ZeroValue;
        int32 Mode = 0;
        int32 ConfirmedMode = 0;
        int32 PreferredMode = 0;
        float ResolutionQuality = 0.0f;
    };
    FVideoConfigState GuardedVideo;
    bool bVideoPreviewGuard = false; // 非Config，不跨进程保存。

    UPROPERTY(Config)
```

### 3.12 Private/Settings/ETGameUserSettings.cpp

在开发控制台条件编译块之前加入完整保护实现；这些是运行功能，不能放进非Shipping条件中。

查找原文：

```cpp
#if !UE_BUILD_SHIPPING
```

替换为：

```cpp
// ---- 预览配置保护：UE窗口变化会自动SetScreenResolution + ConfirmVideoMode ----

bool UETGameUserSettings::BeginVideoPreviewGuard()
{
    if (bVideoPreviewGuard)
    {
        return false;
    }
    GuardedVideo.Requested = GetScreenResolution();
    GuardedVideo.Confirmed = GetLastConfirmedScreenResolution();
    GuardedVideo.Mode = FullscreenMode;
    GuardedVideo.ConfirmedMode = LastConfirmedFullscreenMode;
    GuardedVideo.PreferredMode = PreferredFullscreenMode;
    GuardedVideo.ResolutionQuality = ScalabilityQuality.ResolutionQuality;
    bVideoPreviewGuard = true;
    return true;
}

void UETGameUserSettings::RestoreGuardedVideo()
{
    if (bVideoPreviewGuard)
    {
        ResolutionSizeX = GuardedVideo.Requested.X;
        ResolutionSizeY = GuardedVideo.Requested.Y;
        LastUserConfirmedResolutionSizeX = GuardedVideo.Confirmed.X;
        LastUserConfirmedResolutionSizeY = GuardedVideo.Confirmed.Y;
        FullscreenMode = GuardedVideo.Mode;
        LastConfirmedFullscreenMode = GuardedVideo.ConfirmedMode;
        PreferredFullscreenMode = GuardedVideo.PreferredMode;
        ScalabilityQuality.ResolutionQuality = GuardedVideo.ResolutionQuality;
    }
}

void UETGameUserSettings::EndVideoPreviewGuard()
{
    RestoreGuardedVideo();
    bVideoPreviewGuard = false;
}

void UETGameUserSettings::ConfirmVideoMode()
{
    if (bVideoPreviewGuard)
    {
        RestoreGuardedVideo();
        return;
    }
    Super::ConfirmVideoMode();
}

void UETGameUserSettings::SaveSettings()
{
    RestoreGuardedVideo();
    Super::SaveSettings(); // 输入等非视频偏好照常保存，候选视频值不入盘。
}

void UETGameUserSettings::CommitVideoPreview(
    const FIntPoint Resolution, const EWindowMode::Type Mode)
{
    EndVideoPreviewGuard();
    SetScreenResolution(Resolution);
    SetFullscreenMode(Mode);
    if (Mode != EWindowMode::Windowed)
    {
        // UE5.5的SetPreferredFullscreenMode是private；在派生类同步受保护字段与同名CVar。
        PreferredFullscreenMode = Mode == EWindowMode::Fullscreen ? 0 : 1;
        if (IConsoleVariable* Fullscreen = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FullScreenMode")))
        {
            Fullscreen->Set(PreferredFullscreenMode, ECVF_SetByGameSetting);
        }
    }
    Super::ConfirmVideoMode();
    Super::SaveSettings();
}

#if !UE_BUILD_SHIPPING
```

### 3.13 Tests/ETVideoModeRulesTests.cpp

为扩原专项的引擎自动确认保护用例加入已有派生设置头。

查找原文：

```cpp
#include "Settings/ETVideoModeRules.h"
```

替换为：

```cpp
#include "Settings/ETVideoModeRules.h"
#include "Settings/ETGameUserSettings.h"
```

### 3.14 Tests/ETVideoModeRulesTests.cpp

在旧RunTest末尾return true之前扩配置保护用例，测试只操作独立对象内存。

查找原文：

```cpp
    return true;
}
#endif
```

替换为：

```cpp
    // 真实引擎窗口调整顺序：先改请求尺寸，再虚调用ConfirmVideoMode。
    UETGameUserSettings* Guarded = NewObject<UETGameUserSettings>();
    if (!TestNotNull(TEXT("Guarded settings object"), Guarded))
    {
        return false;
    }
    Guarded->SetScreenResolution(FIntPoint(1280, 720));
    Guarded->SetFullscreenMode(EWindowMode::Windowed);
    Guarded->ConfirmVideoMode();
    TestTrue(TEXT("Preview guard acquired"), Guarded->BeginVideoPreviewGuard());
    TestFalse(TEXT("Second guard cannot replace first baseline"), Guarded->BeginVideoPreviewGuard());
    Guarded->SetScreenResolution(FIntPoint(1600, 900));
    Guarded->ConfirmVideoMode();
    TestTrue(TEXT("Engine automatic confirmation restores requested and confirmed baseline"),
        Guarded->GetScreenResolution() == FIntPoint(1280, 720) &&
        Guarded->GetLastConfirmedScreenResolution() == FIntPoint(1280, 720));
    Guarded->EndVideoPreviewGuard();
    Guarded->SetScreenResolution(FIntPoint(1600, 900));
    Guarded->ConfirmVideoMode();
    TestTrue(TEXT("Ordinary confirmation works after releasing guard"),
        Guarded->GetLastConfirmedScreenResolution() == FIntPoint(1600, 900));
    // 这里不调用SaveSettings / CommitVideoPreview，不写本机个人文件。
    return true;
}
#endif
```


## 4. 两份新源码完整内容

### 4.1 Public/Settings/ETVideoSettingsSubsystem.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETVideoSettingsSubsystem.generated.h"

class UGameViewportClient;
class UETGameUserSettings;
class UWorld;
class SWindow;

DECLARE_MULTICAST_DELEGATE_OneParam(FETVideoPreviewChanged, const FETVideoPreviewSnapshot&);

/** 单GI视频预览适配；不拥有UI / 暂停，不把候选写入持久配置。 */
UCLASS()
class EMBERTRIAL_API UETVideoSettingsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool GetActualMode(FETVideoMode& Out) const;
    bool GetAvailableResolutions(EETVideoWindowMode Mode, TArray<FIntPoint>& Out) const;
    uint64 BeginPreview(UObject* Owner, const FETVideoMode& Request);
    bool ConfirmPreview(UObject* Owner, uint64 ExpectedSerial);
    void CancelPreview(UObject* Owner, uint64 ExpectedSerial);

    FETVideoPreviewSnapshot GetSnapshot() const { return Transaction.GetSnapshot(); }
    double GetRemainingSeconds() const;
    FETVideoPreviewChanged OnChanged;

private:
    bool IsStandaloneContext() const;
    bool IsLocalOwner(const UObject* Owner) const;
    bool ReadViewport(UGameViewportClient* Viewport, FETVideoMode& Out) const;
    bool ReadBoundActual(FETVideoMode& Out) const;
    bool NeedsTick() const;
    void EnsureTicker();
    bool TickPreview(float DeltaSeconds);
    void Execute(const FETVideoPreviewCommand& Command);
    void CancelInternal();
    void Publish();
    void ReleaseContext();
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

    FETVideoPreviewTransaction Transaction;
    TWeakObjectPtr<UObject> PreviewOwner;
    TWeakObjectPtr<UWorld> PreviewWorld;
    TWeakObjectPtr<UGameViewportClient> BoundViewport;
    TWeakObjectPtr<UETGameUserSettings> BoundSettings;
    TWeakPtr<SWindow> BoundWindow;
    FTSTicker::FDelegateHandle TickHandle;
    FDelegateHandle WorldCleanupHandle;

    FETVideoPreviewSnapshot LastPublished;
    int32 LastRemaining = INDEX_NONE;
    bool bPublished = false;
    bool bOperating = false;
    bool bPublishing = false;
    bool bInsideTicker = false;
    bool bCancelDeferred = false;
    bool bEnding = false;
};
```

### 4.2 Private/Settings/ETVideoSettingsSubsystem.cpp

```cpp
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
```

## 5. 正常Build与两个相关专项

保存源码、UE保持关闭。PowerShell先正常Editor构建 / 索引：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload，预计Core83 / UI32 / Editor3，新类在EmberTrialCodeModel，SWindow / KismetSystemLibrary头可导航。打开UE，停止PIE，分别运行，前一项完成后再执行后一项：

```text
Automation RunTests EmberTrial.M3.Settings.VideoModeRules
```

```text
Automation RunTests EmberTrial.M3.Settings.VideoPreviewTransaction
```

各一次Success。原规则专项扩模式转换与真实“SetScreenResolution→自动Confirm”保护调用顺序，事务专项回归01；不在自动化里切真实窗口或保存个人配置，不新建镜像测试。第一条真实error文字与附近10–20行最有用，exit code 6只是汇总。

## 6. 启动独立游戏与测试专用配置

### 6.1 先确认PIE明确拒绝

常用地图Play，视口`~`控制台先输入`con.MinLogVerbosity 4`并回车，让Display级别的开发日志可见；再输入：

```text
et.VideoPreview begin windowed 1600 900
```

应输出Begin Serial=0，不改变编辑器 / PIE窗口。这是本章边界。Stop、Save All并**关闭UE编辑器**，然后执行下一节独立游戏；不能仅换成New Editor Window (PIE)。

### 6.2 从新进程运行当前源码版本

在PowerShell完整执行下面一段。使用UnrealEditor.exe的`-game`运行未Cook的新源码 / 资产，不使用旧M2包；`-GameUserSettingsINI`指向Artifacts专用个人文件，避免测试覆盖日常编辑器显示配置。此参数已按本机ConfigCacheIni::GetDestIniFilename核对。

```powershell
Set-Location 'E:\UE_Project\Demo'
$videoRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05B2_02\Runtime'
New-Item -ItemType Directory -Path $videoRuntime -Force | Out-Null
$videoProfile = Join-Path $videoRuntime 'VideoPreviewUser.ini'
$videoLog = Join-Path $videoRuntime 'VideoPreview01.log'
$videoArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '/Game/EmberTrial/Maps/L_ET_MainMenu'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    ('-GameUserSettingsINI="{0}"' -f $videoProfile)
    ('-abslog="{0}"' -f $videoLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $videoArgs -WindowStyle Normal
```

这是需要看见 / 操作的测试游戏窗口。等主菜单正常显示，先按`~`，输入`con.MinLogVerbosity 4`并回车，然后查询：

```text
et.VideoPreview status
```

应Native=1、Phase=0、Actual约1280×720 / Mode0（窗口）。Actual来自Viewport像素与原生窗口模式，以实际日志为准，不要求它等于B1的Requested配置。若Native=0，保留日志，不继续切屏；先核对确实是新进程`-game`而非PIE。

所有命令都在游戏的`~`控制台输入，一次一行并回车。需要读历史输出可再次按`~`展开控制台，完成后再按`~`关闭并回到界面。若屏幕输出被其他日志挤走，不必重做，完整日志在`Artifacts/M3_05B2_02/Runtime/VideoPreview01.log`。

## 7. 实际窗口分组检查（10–15分钟，不用掐秒点）

模式：windowed / borderless / fullscreen。Phase数字0空闲、1应用中、2等待确认、3恢复中、4恢复失败；Result0无结果、1未变化、2已发确认提交、3取消、4超时、5应用未生效、6外部变化、7恢复失败。长日志由我读取。

### 7.1 窗口预览、取消与超时

1. 执行`et.VideoPreview list windowed`，看本次可选窗口目录。窗口候选由引擎GetConvenientWindowedResolutions按主显示器生成，并加入当前合法窗口尺寸；它不同于GetSupportedFullscreenResolutions底层RHI枚举的独占显示目录。以下用1600×900；若目录没有它，选择其中一项**不同于当前、且小于桌面**的尺寸替换示例，不把示例当硬编码支持列表。候选仍须经过OS实际观察，不能把目录资格当切换必然成功。
2. 输入下面命令，窗口应发生实际变化；再status看到Phase2后输入cancel。恢复到本次Begin捕获的实际基线后Phase0 / Result3，未确认不应出现本服务的SaveRequested；State日志的ConfigRequested / ConfigConfirmed应保持预览前值，不能被引擎自动确认成候选。

```text
et.VideoPreview begin windowed 1600 900
```

```text
et.VideoPreview cancel
```

3. 再begin同一尺寸，**不确认**，等待约20秒再status。应自动回基线，Phase0 / Result4；无需精确在15秒按键。应用未生效会回退 / Result5，按实际反馈，不假报通过。
4. 正常应用后、15秒内可用confirm确认；如果手动输入慢导致已回退，重新begin即可，不修改时限绕过保护。

```text
et.VideoPreview begin windowed 1600 900
```

```text
et.VideoPreview confirm
```

5. 确认后Phase0 / Result2，Actual保持候选，有一条SaveRequested。重复confirm应ConfirmAccepted=0，不重复保存。当前控制台确认操作的是此刻Serial；B3页面将缓存自己的Serial，旧页隔离已由01专项验证。

### 7.2 暂停期间的真实时间与世界重建

1. 从菜单进入游戏，P打开原暂停页，保持World暂停。先重新查询`list windowed`，控制台begin其中另一个尺寸，**不确认，仍保持暂停约20秒**。应恢复到这一轮实际基线，日志State有Paused=1且Result4；这才是暂停下CoreTicker实机证据。1280×720可能只因是当前窗口而暂时出现在目录，确认其他尺寸后不一定仍可选择。
2. P正常继续，确认不卡键、鼠标可转；无需重复全套攻击 / 遗物验收。
3. 回主菜单，begin一个与当前不同的windowed尺寸，随后点击开始新试炼，**不确认**。世界清理应取消，进入新地图后status观察恢复原基线 / Result3，未保存候选。不要为此搭临时蓝图或精确卡应用帧。

### 7.3 无边框与最终测试状态

1. 在游戏或主菜单执行`et.VideoPreview begin borderless 0 0`。0在无边框请求中会被B1正规化为主桌面尺寸，实际应无边框 / Mode1，绝不把0写进应用请求。
2. 用cancel恢复本次真实基线；如果逾时已自动恢复，同样记录实际Result，不把它当手动取消证据。独占fullscreen先保留接口，完整平台表现留B3；本批不要求冒险重复多种显示模式。
3. 最后保持窗口模式和一个目录中可选的尺寸。1280×720并非所有情况下都在目录内；若请求返回0，不要把它当成已恢复。记录实际尺寸与专用测试配置即可。
4. 正常菜单退出或关闭独立窗口。保留Runtime日志与专用ini；日常Saved中的输入偏好应仍1 / 不反转。重新打开UE回常用地图，不修改资产 / Config、不新Cook。

## 8. 故障与生命周期边界

- 窗口短暂黑屏 / 无法操作时先等待回退期限，不立刻确认；若日志Phase4，`cancel`是显式恢复重试。窗口已销毁或OS持续拒绝时只能报告失败，不能宣称已恢复；重启独立测试进程重新取得正常窗口后再查日志。
- 新请求返回0可能是PIE拒绝、非法 / 未列出尺寸、繁忙或模式未变化，先看status。原内核基线保留到实际恢复，恢复失败不允许新预览覆盖它。
- RequestResolutionChange申请窗口后，UE会自动确认窗口尺寸；派生GameUserSettings的保护会恢复原视频字段，并在SaveSettings时再次保护。因此输入卡保存个人偏好不会顺带写入候选。窗口移动的原生SaveConfig也只能看到已恢复的原视频值；只有本服务明确确认才解除保护 / 提交。SaveRequested仍不是I/O成功回执。
- 视图关闭由B3显式Cancel；本批自动处理Owner失效、换世界与WorldCleanup。窗口请求后与引擎ApplyResolutionSettings一样立即调用ConsoleVariableSinks，避免切图时请求排到漫长加载之后才派发；仍须观察实际结果才算恢复。GI结束不保存候选、撤自己的委托与Ticker；在Ticker回调内部用返回false退出，避免RemoveTicker等待自身。
- 实际预览期间每帧只读当前窗口 / Viewport与时钟；空闲无持续Ticker，倒计时事件只按整秒 / 状态变化发布。没有扫描Actor、世界Timer或Core→项目UI依赖。事件回调期间禁止重入提交，取消请求可延后到下一次更新执行。

### 8.1 2026-11-05 实际测试记录

助手按用户授权操作独立`-game`窗口；完整日志为本地`Artifacts/M3_05B2_02/Runtime/VideoPreview01.log`，配置为同目录`VideoPreviewUser.ini`。编辑器日志显示`VideoModeRules`与`VideoPreviewTransaction`各一次Success，PIE请求返回Serial=0；本次电脑操作未独立复核构建和索引；用户17:25已确认双Build / 三索引和完整短查通过，整章结论见验收报告。

- 初始Native=1，实际1280×720窗口；候选目录包含1536×864和1365×767。
- Serial 1：1280×720 → 1536×864，手动取消后实际恢复1280×720，Result=3。预览时配置请求值与确认值均保持1280×720。
- Serial 2：相同候选未确认，超时后实际恢复1280×720，Result=4。
- Serial 3：确认1536×864，ConfirmAccepted=1、SaveRequested一次；重复确认返回0。专用ini中请求和确认尺寸均为1536×864。
- 暂停中1280×720请求返回Serial=0；结合目录实现，当前尺寸不再为1280时它可能不在候选目录，具体拒绝原因日志未细分。改用目录中的1365×767：Serial 4在Paused=1期间倒计时结束并恢复1536×864，Result=4；P可关闭暂停页；助手未单独核验移动和鼠标转向，用户17:25反馈已补齐通过。
- Serial 5：游戏暂停时预览1365×767，返回主菜单触发世界清理，实际恢复1536×864，Result=3。此处覆盖游戏到菜单的切图，未重复菜单到游戏方向。
- Serial 6：无边框实际2048×1152 / Mode=1，手动取消恢复1536×864窗口，Result=3。正常退出日志完整；专用配置最终1536×864，未恢复1280×720。独占全屏未测。

本轮未修改源码或共享Config；以上为所列实测，不代表其他平台或所有窗口尺寸已验证。

## 9. 完成后反馈

```text
M3-05B2-02 完成
两新 / 六旧Core；Editor Build / Core83-UI32-Editor3索引 / Game Build：
VideoModeRules / VideoPreviewTransaction各一次：
PIE拒绝；新进程-game与专用ini / Native1 / 实际初始尺寸：
窗口预览-取消 / 超时实际恢复 / 配置保护 / 确认一次保存-重复拒绝：
P暂停期间实际倒计时回退 / 继续无卡键：
预览中切图取消 / 新世界实际基线恢复：
无边框桌面与取消恢复 / 最终测试窗口1280×720或实际值：
Runtime目录（沿用默认可填沿用）/ 专用配置保留 / 正常退出：
源码已保存、资产与共享Config未改；失败或特别说明（无填无）：
完成时分 / 手动代码 / 阅读 / 检查 / 构建与启动等待：
```

不必抄长日志；首条真实编译error用文字与附近内容，窗口大小 / 模式不符附完整窗口截图与操作步骤。

## 10. Commit / Push（固定放在最后）

B2-02源码已在本地`c2df3c1`，本轮起点工作区干净。当前Stage B2-02验收报告 / 指南收尾、B3-01新指南与实际更新的共享状态文档；B3-01三新UI源码尚未写入正式工程，Artifacts候选不Stage。源码 / 资产 / Config未变化时不重复Stage。

- 英文文档建议：`docs: verify native video preview and plan display settings draft`
- 专业中文：`docs: 完成原生视频预览验收并规划显示设置草稿`

未来B3-01实施后的源码与文档建议分别见新指南末尾。Stage / Commit / Push由用户操作，个人设置 / 日志 / 候选 / 缓存 / 包留本地。
