# M3-05C3：主菜单与暂停页共用音量设置卡

2026-11-06 20:55用户完成。**[本章验收通过](Reports/M3_05C3_Verification.md)，无待补，进入[M3-06A新版独立包](M3_06A_IntegratedPackage.md)。** 以下保留完整手动代码 / 装配步骤，不要求重跑本章矩阵。

## 1. 本批交付与体验

主菜单与Pause右列加入同一张音量卡：总音量 / 音乐 / 音效三条滑条、当前已应用值、应用 / 取消修改 / 恢复默认。编辑只改草稿，**点击应用才改变真实音量并请求一次保存**；默认只填三项100%，不重置其他设置。离开页面丢弃未应用草稿，重开读取已应用值。

首屏通过声音路由就绪事件自动启用，无须先进试炼。显示预览期间禁用音量卡，保留其草稿；预览结束恢复。PIE只读，真实音量在原生-game测试。卡片没有Tick / Timer，也不拥有另一个SoundMix；未来正式声音沿C2同一类树播放。

| 代码边界 | 本批职责 |
| --- | --- |
| UI Draft | 基线 / 三项副本、非法编辑与外部过期快照拒绝、关闭清理 |
| Audio Widget | 控件与百分比、事件订阅、焦点、向Core提交意图 |
| 原Core AudioSubsystem | 新增原生就绪事件；原路由 / Mix所有权 / 清理不变 |
| 原两个宿主 | 第八项AudioSettingsPanel绑定，透传Busy，先关闭音量卡再清理视频页 |

本批五新UI / 六旧Core+UI、一新 / 两旧Widget均已实施。实际233 Source / 133cpp（Core89 / UI41 / Editor3）、42自动化、309 Content / 来源92。代码70 / Designer35 / 阅读10 / 检查10，共125分钟、等待5；一专项 / 三Valid与两轮真实界面及重启读取通过。

## 2. 文件与准备

Save All并关闭UE。五新文件**全部属于EmberTrialUI**，Tests也放UI自己的Tests，避免再次出现Core找不到UI头的循环依赖。不要手建generated.h。

```text
EmberTrial/Source/EmberTrialUI/Public/Settings/ETAudioSettingsDraft.h       新
EmberTrial/Source/EmberTrialUI/Private/Settings/ETAudioSettingsDraft.cpp    新
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETAudioSettingsWidget.h      新
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETAudioSettingsWidget.cpp    新
EmberTrial/Source/EmberTrialUI/Tests/ETAudioSettingsDraftTests.cpp          新
EmberTrial/Source/EmberTrial/Public/Settings/ETAudioSubsystem.h            旧Core
EmberTrial/Source/EmberTrial/Private/Settings/ETAudioSubsystem.cpp          旧Core
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h           旧UI
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp         旧UI
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h              旧UI
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp            旧UI
```

GameUserSettings / Build.cs / Config / 声音资源本批不修改。第3节三组新类型使用完整代码；第4节旧文件仅做精确增量，保留原实现。新增事件为原生委托，不需要蓝图接线。

## 3. 五份新文件完整代码

### 3.1 Public/Settings/ETAudioSettingsDraft.h

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Settings/ETAudioSettingsDraft.h`。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Settings/ETAudioSettingsTypes.h"

/** 页面纯值草稿；只有显式应用生成请求，不操作混音、World或配置文件。 */
class EMBERTRIALUI_API FETAudioSettingsDraft
{
public:
    bool Open(const FETAudioPreferences& Applied);
    void Close();
    bool SetValues(const FETAudioPreferences& Request);
    void UseDefaults();
    bool TryMakeApply(const FETAudioPreferences& Current, FETAudioPreferences& Out) const;

    bool IsOpen() const { return bOpen; }
    bool IsDirty() const { return bOpen && !Same(Value, Baseline); }
    bool IsDefault() const { return bOpen && Same(Value, FETAudioPreferences{}); }
    const FETAudioPreferences& GetValue() const { return Value; }

private:
    static bool Same(const FETAudioPreferences& A, const FETAudioPreferences& B);
    FETAudioPreferences Baseline;
    FETAudioPreferences Value;
    bool bOpen = false;
};
```

### 3.2 Private/Settings/ETAudioSettingsDraft.cpp

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Settings/ETAudioSettingsDraft.cpp`。

```cpp
#include "Settings/ETAudioSettingsDraft.h"

// ---- 页面会话：复制基线，取消 / 重开取新快照，默认只影响当前卡 ----

bool FETAudioSettingsDraft::Open(const FETAudioPreferences& Applied)
{
    const FETAudioPreferences Copy = Applied;
    Close();
    if (!Copy.IsValid())
    {
        return false;
    }
    Baseline = Value = Copy;
    bOpen = true;
    return true;
}

void FETAudioSettingsDraft::Close()
{
    bOpen = false;
    Baseline = Value = FETAudioPreferences{};
}

bool FETAudioSettingsDraft::SetValues(const FETAudioPreferences& Request)
{
    if (!bOpen || !Request.IsValid())
    {
        return false;
    }
    Value = Request;
    return true;
}

void FETAudioSettingsDraft::UseDefaults()
{
    if (bOpen)
    {
        Value = FETAudioPreferences{};
    }
}

// ---- 最终提交：旧页面不能覆盖其他入口刚应用的值 ----

bool FETAudioSettingsDraft::Same(const FETAudioPreferences& A, const FETAudioPreferences& B)
{
    constexpr float Tolerance = 0.0001f; // 小于百分比显示精度，不把滑条浮点误差视为修改。
    return A.IsValid() && B.IsValid() &&
        FMath::IsNearlyEqual(A.MasterVolume, B.MasterVolume, Tolerance) &&
        FMath::IsNearlyEqual(A.MusicVolume, B.MusicVolume, Tolerance) &&
        FMath::IsNearlyEqual(A.EffectsVolume, B.EffectsVolume, Tolerance);
}

bool FETAudioSettingsDraft::TryMakeApply(
    const FETAudioPreferences& Current, FETAudioPreferences& Out) const
{
    const FETAudioPreferences Copy = Current;
    Out = FETAudioPreferences{};
    if (!bOpen || !IsDirty() || !Same(Copy, Baseline) || !Value.IsValid())
    {
        return false;
    }
    Out = Value;
    return true;
}
```

### 3.3 Public/Widgets/ETAudioSettingsWidget.h

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Widgets/ETAudioSettingsWidget.h`。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/ETAudioSettingsDraft.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETAudioSettingsWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;
class UETGameUserSettings;
class UETAudioSubsystem;
class UETVideoSettingsSubsystem;

/** 共用音量卡；草稿归页面，实际路由与保存沿用Core唯一入口。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETAudioSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void SetHostBusy(bool bBusy);
    void CloseView();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(
        const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    bool HasLiveHost() const;
    bool IsPreviewBusy() const;
    bool CanEdit() const;
    void ReadCurrent(const FText& Message);
    void Render();
    void FocusMaster();
    void UpdateDraft(const FETAudioPreferences& Request);
    void HandleRoutingChanged();
    void HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot);

    UFUNCTION()
    void HandleMasterChanged(float Value);
    UFUNCTION()
    void HandleMusicChanged(float Value);
    UFUNCTION()
    void HandleEffectsChanged(float Value);
    UFUNCTION()
    void HandleApply();
    UFUNCTION()
    void HandleRevert();
    UFUNCTION()
    void HandleDefaults();

    // ---- 十一项绑定；没有属性Binding或蓝图点击链 ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> MasterSlider;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> MusicSlider;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> EffectsSlider;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MasterValueText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> MusicValueText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> EffectsValueText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplyButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> DefaultsButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> AudioCurrentText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> AudioStatusText;

    TWeakObjectPtr<UETGameUserSettings> Settings;
    TWeakObjectPtr<UETAudioSubsystem> AudioService;
    TWeakObjectPtr<UETVideoSettingsSubsystem> PreviewService;
    FDelegateHandle RoutingChangedHandle;
    FDelegateHandle PreviewChangedHandle;
    FETAudioSettingsDraft Draft;
    FText Feedback;
    bool bViewActive = false;
    bool bHostBusy = false;
    bool bSyncingControls = false;
};
```

### 3.4 Private/Widgets/ETAudioSettingsWidget.cpp

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETAudioSettingsWidget.cpp`。

```cpp
#include "Widgets/ETAudioSettingsWidget.h"

#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Settings/ETAudioSubsystem.h"
#include "Settings/ETGameUserSettings.h"
#include "Settings/ETVideoSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogETAudioSettingsView, Log, All);

// ---- 控件初始化；Construct观察当前服务，Close先封口再移除两个句柄 ----

bool UETAudioSettingsWidget::HasRequiredLayout() const
{
    return IsValid(MasterSlider.Get()) && IsValid(MusicSlider.Get()) &&
        IsValid(EffectsSlider.Get()) && IsValid(MasterValueText.Get()) &&
        IsValid(MusicValueText.Get()) && IsValid(EffectsValueText.Get()) &&
        IsValid(ApplyButton.Get()) && IsValid(RevertButton.Get()) &&
        IsValid(DefaultsButton.Get()) && IsValid(AudioCurrentText.Get()) &&
        IsValid(AudioStatusText.Get());
}

void UETAudioSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    MasterSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMasterChanged);
    MusicSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleMusicChanged);
    EffectsSlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleEffectsChanged);
    ApplyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleApply);
    RevertButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRevert);
    DefaultsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDefaults);
    for (USlider* Slider : { MasterSlider.Get(), MusicSlider.Get(), EffectsSlider.Get() })
    {
        Slider->SetMinValue(0.0f);
        Slider->SetMaxValue(1.0f);
        Slider->SetStepSize(0.05f);
    }
}

void UETAudioSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsDesignTime() || !HasRequiredLayout())
    {
        return;
    }
    CloseView();
    bViewActive = true;
    Settings = UETGameUserSettings::GetETGameUserSettings();
    if (UGameInstance* Instance = GetGameInstance())
    {
        AudioService = Instance->GetSubsystem<UETAudioSubsystem>();
        PreviewService = Instance->GetSubsystem<UETVideoSettingsSubsystem>();
    }
    if (UETAudioSubsystem* Audio = AudioService.Get())
    {
        RoutingChangedHandle = Audio->OnRoutingChanged.AddUObject(
            this, &ThisClass::HandleRoutingChanged);
    }
    if (UETVideoSettingsSubsystem* Preview = PreviewService.Get())
    {
        PreviewChangedHandle = Preview->OnChanged.AddUObject(
            this, &ThisClass::HandlePreviewSnapshot);
    }
    ReadCurrent(FText::FromString(TEXT("修改后选择应用；离开页面丢弃未应用草稿。")));
}

void UETAudioSettingsWidget::NativeDestruct()
{
    CloseView();
    Super::NativeDestruct();
}

void UETAudioSettingsWidget::CloseView()
{
    bViewActive = false;
    if (UETAudioSubsystem* Audio = AudioService.Get())
    {
        Audio->OnRoutingChanged.Remove(RoutingChangedHandle);
    }
    if (UETVideoSettingsSubsystem* Preview = PreviewService.Get())
    {
        Preview->OnChanged.Remove(PreviewChangedHandle);
    }
    RoutingChangedHandle.Reset();
    PreviewChangedHandle.Reset();
    AudioService.Reset();
    PreviewService.Reset();
    Settings.Reset();
    Draft.Close();
    Feedback = FText::GetEmpty();
}

// ---- 就绪 / 视频事件只刷新可用性，不重开草稿，不增加Ticker / Timer ----

bool UETAudioSettingsWidget::HasLiveHost() const
{
    for (const UUserWidget* View = this; View; View = View->GetTypedOuter<UUserWidget>())
    {
        if (View->IsInViewport())
        {
            return true;
        }
    }
    return false;
}

bool UETAudioSettingsWidget::IsPreviewBusy() const
{
    return PreviewService.IsValid() &&
        PreviewService->GetSnapshot().Phase != EETVideoPreviewPhase::Idle;
}

bool UETAudioSettingsWidget::CanEdit() const
{
    const UWorld* World = GetWorld();
    return bViewActive && !bHostBusy && Draft.IsOpen() && Settings.IsValid() &&
        AudioService.IsValid() && AudioService->IsRoutingApplied(World) &&
        PreviewService.IsValid() && !IsPreviewBusy() && !GIsEditor &&
        World && !World->bIsTearingDown && World->WorldType == EWorldType::Game &&
        World->GetNetMode() == NM_Standalone;
}

void UETAudioSettingsWidget::SetHostBusy(const bool bBusy)
{
    bHostBusy = bBusy;
    Render();
}

void UETAudioSettingsWidget::ReadCurrent(const FText& Message)
{
    const bool bReady = Settings.IsValid() && Draft.Open(Settings->GetAudioPreferences());
    if (!bReady)
    {
        Draft.Close();
    }
    Feedback = bReady ? Message : FText::FromString(TEXT("音量设置暂不可用。"));
    Render();
}

void UETAudioSettingsWidget::HandleRoutingChanged()
{
    Render(); // 首屏PostLoadMap后启用；不覆盖用户已编辑的草稿。
}

void UETAudioSettingsWidget::HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot)
{
    (void)Snapshot;
    Render();
}

void UETAudioSettingsWidget::Render()
{
    if (!bViewActive || !HasRequiredLayout())
    {
        return;
    }
    const TGuardValue<bool> SyncGuard(bSyncingControls, true);
    const FETAudioPreferences& Value = Draft.GetValue();
    MasterSlider->SetValue(Value.MasterVolume);
    MusicSlider->SetValue(Value.MusicVolume);
    EffectsSlider->SetValue(Value.EffectsVolume);
    FNumberFormattingOptions Number;
    Number.SetMaximumFractionalDigits(1);
    MasterValueText->SetText(FText::Format(NSLOCTEXT("ETAudioUI", "Master", "总音量：{0}"),
        FText::AsPercent(Value.MasterVolume, &Number)));
    MusicValueText->SetText(FText::Format(NSLOCTEXT("ETAudioUI", "Music", "音乐：{0}"),
        FText::AsPercent(Value.MusicVolume, &Number)));
    EffectsValueText->SetText(FText::Format(NSLOCTEXT("ETAudioUI", "Effects", "音效：{0}"),
        FText::AsPercent(Value.EffectsVolume, &Number)));
    if (Settings.IsValid())
    {
        const FETAudioPreferences Applied = Settings->GetAudioPreferences();
        AudioCurrentText->SetText(FText::Format(
            NSLOCTEXT("ETAudioUI", "Current", "当前：总 {0} / 音乐 {1} / 音效 {2}"),
            FText::AsPercent(Applied.MasterVolume, &Number),
            FText::AsPercent(Applied.MusicVolume, &Number),
            FText::AsPercent(Applied.EffectsVolume, &Number)));
    }
    FText Message = Feedback;
    if (GIsEditor)
    {
        Message = FText::FromString(TEXT("请在独立游戏窗口中调整音量。"));
    }
    else if (IsPreviewBusy())
    {
        Message = FText::FromString(TEXT("请先确认或取消显示预览；音量草稿保留。"));
    }
    else if (bHostBusy)
    {
        Message = FText::FromString(TEXT("正在切换页面，请稍候。"));
    }
    else if (!AudioService.IsValid() || !AudioService->IsRoutingApplied(GetWorld()))
    {
        Message = FText::FromString(TEXT("声音路由尚未就绪，请稍候。"));
    }
    AudioStatusText->SetText(Message);
    const bool bEdit = CanEdit();
    MasterSlider->SetIsEnabled(bEdit);
    MusicSlider->SetIsEnabled(bEdit);
    EffectsSlider->SetIsEnabled(bEdit);
    ApplyButton->SetIsEnabled(bEdit && Draft.IsDirty());
    RevertButton->SetIsEnabled(bEdit && Draft.IsDirty());
    DefaultsButton->SetIsEnabled(bEdit && !Draft.IsDefault());
}

// ---- 编辑只更新纯值；实际应用沿Core唯一提交入口 ----

void UETAudioSettingsWidget::UpdateDraft(const FETAudioPreferences& Request)
{
    if (!bSyncingControls && CanEdit() && Draft.SetValues(Request))
    {
        Feedback = FText::FromString(Draft.IsDirty() ?
            TEXT("有未应用的修改。") : TEXT("与当前设置一致。"));
        Render();
    }
}

void UETAudioSettingsWidget::HandleMasterChanged(const float Value)
{
    FETAudioPreferences Next = Draft.GetValue();
    Next.MasterVolume = Value;
    UpdateDraft(Next);
}

void UETAudioSettingsWidget::HandleMusicChanged(const float Value)
{
    FETAudioPreferences Next = Draft.GetValue();
    Next.MusicVolume = Value;
    UpdateDraft(Next);
}

void UETAudioSettingsWidget::HandleEffectsChanged(const float Value)
{
    FETAudioPreferences Next = Draft.GetValue();
    Next.EffectsVolume = Value;
    UpdateDraft(Next);
}

void UETAudioSettingsWidget::FocusMaster()
{
    if (CanEdit() && HasLiveHost() && IsValid(GetOwningPlayer()))
    {
        MasterSlider->SetUserFocus(GetOwningPlayer());
    }
}

void UETAudioSettingsWidget::HandleApply()
{
    if (!CanEdit() || !HasLiveHost() || !Draft.IsDirty())
    {
        return;
    }
    UETGameUserSettings* Current = Settings.Get();
    FETAudioPreferences Request;
    if (!Draft.TryMakeApply(Current->GetAudioPreferences(), Request))
    {
        ReadCurrent(FText::FromString(TEXT("音量已由其他入口更改，已刷新，请重新编辑。")));
        FocusMaster();
        return;
    }
    const TStrongObjectPtr<UETAudioSettingsWidget> KeepAlive(this);
    if (!Current->ApplyAudioPreferences(Request, GetWorld()))
    {
        Feedback = FText::FromString(TEXT("暂不能应用，请确认声音与显示预览状态。"));
        Render();
        return;
    }
    UE_LOG(LogETAudioSettingsView, Display,
        TEXT("[M3-05C3] Applied Widget=%s Master=%.3f Music=%.3f Effects=%.3f"),
        *GetName(), Request.MasterVolume, Request.MusicVolume, Request.EffectsVolume);
    if (bViewActive && HasLiveHost() && Settings.Get() == Current)
    {
        ReadCurrent(FText::FromString(TEXT("已应用并请求保存。")));
        FocusMaster();
    }
}

void UETAudioSettingsWidget::HandleRevert()
{
    if (CanEdit())
    {
        ReadCurrent(FText::FromString(TEXT("已取消修改，保留当前音量。")));
        FocusMaster();
    }
}

void UETAudioSettingsWidget::HandleDefaults()
{
    if (CanEdit())
    {
        Draft.UseDefaults();
        Feedback = FText::FromString(TEXT("草稿已恢复三项100%，选择应用后生效。"));
        Render();
        FocusMaster();
    }
}

FReply UETAudioSettingsWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        HandleRevert();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
```

### 3.5 Tests/ETAudioSettingsDraftTests.cpp

完整路径：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Tests/ETAudioSettingsDraftTests.cpp`。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETAudioSettingsDraft.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAudioSettingsDraftTest,
    "EmberTrial.UI.Settings.AudioDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAudioSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETAudioSettingsDraft Draft;
    FETAudioPreferences Applied;
    Applied.MasterVolume = 0.6f;
    Applied.MusicVolume = 0.3f;
    Applied.EffectsVolume = 0.8f;
    FETAudioPreferences Request = Applied;
    Request.MasterVolume = 0.0f;
    FETAudioPreferences Out;
    TestFalse(TEXT("Closed view rejects edits"), Draft.SetValues(Request));
    TestTrue(TEXT("Open saved values"), Draft.Open(Applied));
    TestFalse(TEXT("Snapshot is clean"), Draft.IsDirty());
    TestFalse(TEXT("Unchanged view cannot apply"), Draft.TryMakeApply(Applied, Out));
    TestTrue(TEXT("Mute only changes draft"), Draft.SetValues(Request));
    TestEqual(TEXT("Browsing keeps applied master"), Applied.MasterVolume, 0.6f);
    TestTrue(TEXT("Edited view proposes valid request"), Draft.TryMakeApply(Applied, Out));
    TestEqual(TEXT("Candidate mute"), Out.MasterVolume, 0.0f);
    TestEqual(TEXT("Candidate retains child volume"), Out.MusicVolume, 0.3f);
    FETAudioPreferences Changed = Applied;
    Changed.EffectsVolume = 0.5f;
    TestFalse(TEXT("External change rejects stale draft"), Draft.TryMakeApply(Changed, Out));
    TestTrue(TEXT("Rejected draft stays edited"), Draft.IsDirty());
    Request.MusicVolume = 1.1f;
    TestFalse(TEXT("Invalid edit rejected"), Draft.SetValues(Request));
    TestEqual(TEXT("Invalid edit preserves prior draft"), Draft.GetValue().MusicVolume, 0.3f);
    Draft.UseDefaults();
    TestTrue(TEXT("Default affects draft only"), Draft.IsDefault());
    TestEqual(TEXT("Defaults do not overwrite current"), Applied.MasterVolume, 0.6f);
    TestTrue(TEXT("Defaults require explicit apply"), Draft.IsDirty());
    TestTrue(TEXT("Reopen supports own value alias"), Draft.Open(Draft.GetValue()));
    TestFalse(TEXT("Reopened snapshot is clean"), Draft.IsDirty());
    FETAudioPreferences Invalid;
    Invalid.EffectsVolume = -1.0f;
    TestFalse(TEXT("Invalid open closes old session"), Draft.Open(Invalid));
    TestFalse(TEXT("Failed open cannot commit"), Draft.TryMakeApply(Applied, Out));
    Draft.Close();
    TestFalse(TEXT("Closed old view cannot commit"), Draft.TryMakeApply(Applied, Out));
    return true;
}
#endif
```

## 4. 六个旧文件增量

下面每项先定位“原文”，只用“替换为”覆盖那一段。不要把两段同时留在源码，也不要用完整候选覆盖自己的文件。Core四处先完成，再改两个宿主；未列出的原代码全部保留。

### 4.1 Core头文件

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Settings/ETAudioSubsystem.h`。

Core头文件：在类型前置声明后声明原生事件，generated.h仍为最后include。

原文：

```cpp
struct FSoundClassAdjuster;
```

替换为：

```cpp
struct FSoundClassAdjuster;

DECLARE_MULTICAST_DELEGATE(FETAudioRoutingChanged);
```

### 4.2 Core头文件

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Settings/ETAudioSubsystem.h`。

Core头文件：public区增加事件实例；原StopProbe与后面的非Shipping声明保留。

原文：

```cpp
    void StopProbe();
```

替换为：

```cpp
    void StopProbe();

    // 路由可用性事件；监听者只刷新资格，不在回调里再次ApplyRuntime。
    FETAudioRoutingChanged OnRoutingChanged;
```

### 4.3 Core实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Settings/ETAudioSubsystem.cpp`。

Core实现：ApplyRuntime成功、状态稳定后广播。回调只Render；不能在监听回调里再次调用ApplyRuntime。

原文：

```cpp
        *GetNameSafe(World), Value.MasterVolume, Value.MusicVolume, Value.EffectsVolume);
    return true;
```

替换为：

```cpp
        *GetNameSafe(World), Value.MasterVolume, Value.MusicVolume, Value.EffectsVolume);
    OnRoutingChanged.Broadcast();
    return true;
```

### 4.4 Core实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Settings/ETAudioSubsystem.cpp`。

Core实现：ReleaseWorld先清状态再广播不可用。只替换末尾两行，前面的Stop / Clear / Pop不能删。

原文：

```cpp
    bMixPushed = false;
    BoundWorld.Reset();
```

替换为：

```cpp
    bMixPushed = false;
    BoundWorld.Reset();
    OnRoutingChanged.Broadcast();
```

### 4.5 头文件

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h`。

头文件：增加Audio Widget前置声明，不增加include到generated.h之后。

原文：

```cpp
class UETGraphicsSettingsWidget;
```

替换为：

```cpp
class UETGraphicsSettingsWidget;
class UETAudioSettingsWidget;
```

### 4.6 头文件

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h`。

头文件：保留Graphics绑定，增加第八项AudioSettingsPanel；每个字段各有自己的UPROPERTY。

原文：

```cpp
    TObjectPtr<UETGraphicsSettingsWidget> GraphicsSettingsPanel;
```

替换为：

```cpp
    TObjectPtr<UETGraphicsSettingsWidget> GraphicsSettingsPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETAudioSettingsWidget> AudioSettingsPanel;
```

### 4.7 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp`。

实现：顶部补Audio Widget头。

原文：

```cpp
#include "Widgets/ETGraphicsSettingsWidget.h"
```

替换为：

```cpp
#include "Widgets/ETGraphicsSettingsWidget.h"
#include "Widgets/ETAudioSettingsWidget.h"
```

### 4.8 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp`。

实现：HasRequiredLayout包含音量卡，保留原七项。

原文：

```cpp
IsValid(VideoSettingsPanel.Get()) && IsValid(GraphicsSettingsPanel.Get());
```

替换为：

```cpp
IsValid(VideoSettingsPanel.Get()) && IsValid(GraphicsSettingsPanel.Get()) &&
        IsValid(AudioSettingsPanel.Get());
```

### 4.9 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp`。

实现：NativeDestruct开始处先关闭音量观察，再继续原Graphics / Video / Super；这里只替换函数签名与开花括号，后面函数体保留。

原文：

```cpp
void UETMainMenuWidget::NativeDestruct()
{
```

替换为：

```cpp
void UETMainMenuWidget::NativeDestruct()
{
    if (AudioSettingsPanel)
    {
        AudioSettingsPanel->CloseView();
    }
```

### 4.10 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp`。

实现：Busy先通知音量再通知画质。原视频通知仍在它们之后，不增加按钮广播。

原文：

```cpp
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->SetHostBusy(bBusy);
    }
```

替换为：

```cpp
    if (AudioSettingsPanel)
    {
        AudioSettingsPanel->SetHostBusy(bBusy);
    }
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->SetHostBusy(bBusy);
    }
```

### 4.11 头文件

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h`。

头文件：增加Audio Widget前置声明，不增加include到generated.h之后。

原文：

```cpp
class UETGraphicsSettingsWidget;
```

替换为：

```cpp
class UETGraphicsSettingsWidget;
class UETAudioSettingsWidget;
```

### 4.12 头文件

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h`。

头文件：保留Graphics绑定，增加第八项AudioSettingsPanel；每个字段各有自己的UPROPERTY。

原文：

```cpp
    TObjectPtr<UETGraphicsSettingsWidget> GraphicsSettingsPanel;
```

替换为：

```cpp
    TObjectPtr<UETGraphicsSettingsWidget> GraphicsSettingsPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETAudioSettingsWidget> AudioSettingsPanel;
```

### 4.13 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp`。

实现：顶部补Audio Widget头。

原文：

```cpp
#include "Widgets/ETGraphicsSettingsWidget.h"
```

替换为：

```cpp
#include "Widgets/ETGraphicsSettingsWidget.h"
#include "Widgets/ETAudioSettingsWidget.h"
```

### 4.14 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp`。

实现：HasRequiredLayout包含音量卡，保留原七项。

原文：

```cpp
IsValid(VideoSettingsPanel.Get()) && IsValid(GraphicsSettingsPanel.Get());
```

替换为：

```cpp
IsValid(VideoSettingsPanel.Get()) && IsValid(GraphicsSettingsPanel.Get()) &&
        IsValid(AudioSettingsPanel.Get());
```

### 4.15 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp`。

实现：NativeDestruct开始处先关闭音量观察，再继续原Graphics / Video / Super；这里只替换函数签名与开花括号，后面函数体保留。

原文：

```cpp
void UETPauseWidget::NativeDestruct()
{
```

替换为：

```cpp
void UETPauseWidget::NativeDestruct()
{
    if (AudioSettingsPanel)
    {
        AudioSettingsPanel->CloseView();
    }
```

### 4.16 实现

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp`。

实现：Busy先通知音量再通知画质。原视频通知仍在它们之后，不增加按钮广播。

原文：

```cpp
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->SetHostBusy(bBusy);
    }
```

替换为：

```cpp
    if (AudioSettingsPanel)
    {
        AudioSettingsPanel->SetHostBusy(bBusy);
    }
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->SetHostBusy(bBusy);
    }
```

## 5. 正常Build、索引与一次专项

保持UE关闭，PowerShell逐段执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor Build成功再执行Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload现有工程，预期Core89 / UI41 / Editor3个cpp，五新文件均属于UI CodeModel；新Widget、Draft、AudioSubsystem头可导航。不要改生成CMake或清缓存。

打开UE，非Play的Output Log执行一次：

```text
Automation RunTests EmberTrial.UI.Settings.AudioDraft
```

应Success。它覆盖浏览无副作用、默认仅草稿、外部快照过期、非法编辑保留、关闭 / 重开；原Core路由没改计算规则，C2专项沿用，不跑全42项 / Cook。两个旧宿主此时尚缺第八绑定，先完成6–7节再Play；不要改成BindWidgetOptional绕过装配。

## 6. 新Widget：十一绑定与具体样式

### 6.1 创建、父类与控件树

1. 在`/Game/EmberTrial/ui/Settings`创建**WBP_ETAudioSettings**，父类**ETAudioSettingsWidget**。找不到父类先确认双Build并重开UE；不要继承Graphics Widget，也不要覆盖原输入卡。
2. Class Defaults设**Is Focusable=False、Tick Frequency=Never**。原生父类已DisableNativeTick；Graph不新增点击 / Value Changed / Tick逻辑。
3. Designer用唯一**Size Box**根，命名AudioSettingsSize，Width / Height Override均不勾。若已有空Canvas，把这个空根换为Size Box，不留一个没有锚点设置的Canvas。
4. Size Box唯一子项Border，名AudioSettingsBorder，Brush Color RGBA=.08/.08/.08/.85，Content Padding四向12；Slot Fill / Fill、Padding0。
5. Border唯一子项Vertical Box，名AudioSettingsContent。按下树依次添加，所有Vertical Box Slot均**Auto / Horizontal Fill**，不用Fill1把卡撑满父ScrollBox高度：

```text
AudioSettingsSize (SizeBox，无固定宽高)
  AudioSettingsBorder (Border，Padding12)
    AudioSettingsContent (VerticalBox)
      AudioTitle (Text：“音量”，20)
      AudioCurrentText (Text，必需，14，Top8 / Bottom10)
      MasterValueText (Text，必需，16)
      MasterSlider (Slider，必需，Top4 / Bottom10)
      MusicValueText (Text，必需，16)
      MusicSlider (Slider，必需，Top4 / Bottom10)
      EffectsValueText (Text，必需，16)
      EffectsSlider (Slider，必需，Top4 / Bottom10)
      AudioButtons (HorizontalBox，Top4)
        ApplyButton (Button，必需) → Text：“应用”
        RevertButton (Button，必需) → Text：“取消修改”
        DefaultsButton (Button，必需) → Text：“恢复默认”
      AudioStatusText (Text，必需，14，Top8)
```

**只有十一项“必需”控件勾选Is Variable**，名称逐字一致；绑定的是Button / Slider / Text Block本体，不是内部的按钮文字。AudioTitle与装饰容器不需要Variable。

### 6.2 Slider逐项设置

三个Slider分别选中，在Details清空旧搜索后核对：

| 属性 | 三条统一值 |
| --- | --- |
| Min Value / Max Value / Value | 0 / 1 / 1 |
| Step Size | .05，即键盘一次5个百分点 |
| Is Focusable | True |
| Requires Controller Lock | False，键盘聚焦后方向键直接调节 |
| Mouse Uses Step | True，鼠标按5%步进，方便取得明确测试值 |
| Orientation | Horizontal |
| Locked | False |
| Visibility / Is Enabled | Visible / True |

这些项有的在Advanced展开内，搜索完整属性名即可。Min / Max / Step运行时原生也会设置，但Focus / Lock / Mouse Uses Step仍按表配置。保持默认可读的Slider样式，不把滑轨设透明；不设置Value Bind。

### 6.3 Text、按钮与编译

1. 五个动态Text均无Text Bind、无BP属性绑定；字体沿用已有中文字体链、颜色指定白色RGBA1。`AudioCurrentText / AudioStatusText`启用Auto Wrap Text、Wrap Text At280、Min Desired Width0；不要给文字固定高度。三项Value Text直接一行显示“总音量：100%”“音乐：100%”“音效：100%”作Designer占位，运行时C++更新。
2. 三Button：Is Focusable=True；Click Method / Press Method均Down and Up；子Text14、居中。Horizontal Box Slot三者Fill1；第一Right4、第二Left4/Right4、第三Left4，其余Padding0。
3. Current占位“当前：总100% / 音乐100% / 音效100%”，Status占位“修改后选择应用；离开页面丢弃未应用草稿。”。根Visible / Enabled，不在Designer永久禁用整张卡。
4. Compile → Save。若提示required binding缺失，检查Hierarchy的类型 / 名称 / Is Variable；不靠删除原生字段通过编译。此卡只有一张新资产，不创建新Controller、Input Action或Mix。

## 7. 两宿主第八绑定与原ScrollBox

### 7.1 主菜单

1. 打开`/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`，父类保持ETMainMenuWidget。Hierarchy定位已有的**MainMenuSettingsSize → MainMenuSettingsScroll → MainMenuSettingsStack**，其中已是Input / Video / Graphics三卡。
2. Palette → User Created拖入**WBP_ETAudioSettings**到原Stack，放GraphicsSettingsPanel之后。外层实例名严格为**AudioSettingsPanel**，Is Variable=True，Vertical Box Slot Auto / Horizontal Fill、Top Padding12、其他Padding0。
3. 原Size保持320×680，原ScrollBox保持Vertical、Scroll When Focus Changes=Instant Scroll、Navigation Destination=Into View、Consume Mouse Wheel=When Scrolling Possible。不再Wrap第二个ScrollBox，不给新卡固定Height，不把开始按钮放进右侧滚动。
4. 编译后必需布局现在八项：StartButton / ContinueButton / QuitButton / StatusText / InputSettingsPanel / VideoSettingsPanel / GraphicsSettingsPanel / AudioSettingsPanel。原标题 / ScaleBox / 背景保持。Compile → Save。

### 7.2 Pause / 祭坛共用页

1. 打开`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`，父类保持ETPauseWidget。右列已有**PauseVideoSettingsSize → PauseSettingsScroll → PauseSettingsStack**；如实际命名不同，按包含Video / Graphics的原Vertical Box定位，不另建右列。
2. 在这个原Stack的GraphicsSettingsPanel之后拖入同一个WBP_ETAudioSettings。外层名**AudioSettingsPanel**，Is Variable=True，Auto / Fill、Top12，其余0。
3. 原右列320×680与ScrollBox焦点滚动保持。左侧遗物 / 输入卡、原继续 / 返回菜单不挪；祭坛沿同一个Pause资产自动复用音量卡，不创建新祭坛页面。
4. 八项绑定：BTN_Resume / MainMenuButton / StatusText / RelicPanel / InputSettingsPanel / VideoSettingsPanel / GraphicsSettingsPanel / AudioSettingsPanel。Compile → Save。

### 7.3 保存与本项目Validate

依次对新WBP、MainMenu、Pause执行Compile → Save，再在Content Browser右键Validate Assets，三份均应有效。当前Message Log可能保留装配前旧错误；以最新编译 / 校验为准。不能只保存两宿主而漏掉新WBP。以下测试前Save All。

## 8. 简化验收：一项专项、三Valid与两轮原生窗口

### 8.1 PIE短查

正式地图PIE → P：右列能滚动到音量卡，应显示原偏好但控件禁用并提示独立窗口。原输入 / 显示 / 画质卡仍存在，继续后移动转向正常，Stop。无需重复战斗 / 装卸、增加临时蓝图或做旧全矩阵。

### 8.2 第一轮启动脚本

Save All关闭UE，在PowerShell整段执行。代码就在本文，不需要寻找另一个.ps1。专用ini保留在本批Artifacts，与C2证据隔离。

```powershell
Set-Location 'E:\UE_Project\Demo'
$audioUiRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05C3\Runtime'
New-Item -ItemType Directory -Path $audioUiRuntime -Force | Out-Null
$audioUiIni = Join-Path $audioUiRuntime 'AudioUIUser.ini'
$audioUiLog = Join-Path $audioUiRuntime 'AudioUI02.log'
$audioUiArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    '-NoSplash'
    ('-GameUserSettingsINI="{0}"' -f $audioUiIni)
    ('-abslog="{0}"' -f $audioUiLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $audioUiArgs -WindowStyle Normal
```

### 8.3 首屏、草稿和真实应用

1. 全新进程主菜单**不要先进试炼**，右列滚到音量卡。首次新ini三项100%、卡片应自行可用；没有永久“路由未就绪”。服务事件只启用，不抢菜单焦点或每帧覆盖草稿。
2. 先把音乐滑到0%，**不应用**。用游戏控制台`et.AudioSettings`只读应仍音乐1；`et.AudioProbe music`仍能听见。回卡点击取消，音乐滑条回100%。这只试听一次，确认浏览无声音副作用。
3. 再将音乐改0% → 点击**音量卡的应用**。当前文字应音乐0%，应用按钮禁用；游戏控制台`et.AudioProbe music`静音，`et.AudioProbe effects`仍可听见。日志由助手读取；Core / View两条日志是同一次提交，不表示保存两次。
4. 点击本卡恢复默认：三滑条100%，**当前文字仍音乐0%**；音乐仍静音。然后点击应用回三项100%。不要调用全局SetToDefaults或改SoundWave Volume。
5. 用滑条明确设**总50% / 音乐50% / 音效100%**再应用，作为最终保存值。Mouse Uses Step按5%便于定位；也可以Tab聚焦后用方向键调至目标，数值看卡上文字。总0的数学 / 真实路由C2已验收，本批不重复所有组合。
6. Tab / Shift+Tab沿三滑条 / 三按钮移动，左右键调节，Enter / Space操作焦点按钮；焦点进到屏幕外时原ScrollBox应滚入视野，不能误开始试炼。新卡内Esc只取消本卡未应用草稿。默认 / 应用 / 取消后焦点回总音量Slider，避免按钮禁用后丢焦点。

### 8.4 Pause丢弃、视频预览与页面切换

1. 开始试炼 → P → 音量卡应读总50 / 音乐50 / 音效100。改音乐为别值但不应用，P继续；再P打开仍读已应用组，证明关闭丢弃。用鼠标或键盘继续一种即可，移动转向无卡键。
2. 再做一份未应用音量草稿，滚到同右列的显示卡，选择另一个可用窗口尺寸并点预览。音量卡禁用 / 提示，草稿保留；取消显示预览后音量卡恢复、仍保留该草稿。点击音量卡取消修改回最终组，无须抢15秒控制台输入。
3. P返回菜单，新音量卡仍读最终组、无旧草稿 / 重影。**返回菜单不会把已保存音量重置**；输入 / 画质与原菜单焦点作短观察。用户需要实际试听时只用C2 probe，控件本身不增加开发测试按钮。
4. 正常退出第一轮。不要求第三次打开地图、装卸全组或打包；取消视频预览后保持原窗口。

### 8.5 新进程只读和试听

重复8.2整段脚本，仅把`AudioUI01.log`改为`AudioUI02.log`，其余尤其AudioUIUser.ini相同。

新进程主菜单卡自行可用，三项应50 / 50 / 100。**不先应用**，游戏控制台仅查询`et.AudioSettings`：应Master0.5 / Music0.5 / Effects1、RoutingApplied1。执行一次`et.AudioProbe music`与`et.AudioProbe effects`确认声音存在，正常退出。保留两轮日志 / ini供我读取，不上传个人配置。

## 9. 实现边界、排错与完成反馈

- UI不逐次拖动写盘或实时改Mix；默认 / 取消是本卡纯草稿。未来要支持实时试听回退，需要另建明确事务，本批不偷偷增加。
- 原AudioSubsystem在PostLoadMap成功后广播就绪，移除时广播不可用。View构造时先读现值、再由事件刷新可编辑性；视频 / 声音事件均不重开Draft。首次就绪时用户草稿不会被覆盖。
- 宿主关闭顺序为Audio CloseView → Graphics CloseView → Video CloseView → Super；两个事件句柄都精确移除，旧页即使被持有也不能应用。没有新轮询、Ticker或Timer。
- 编译错误发第一条真实error与附近代码；BindWidget错误发新Hierarchy截图；焦点 / 滚动问题说明PIE还是-game并附可见界面。首屏永久禁用时保留本轮日志，不靠先进试炼绕过。

```text
M3-05C3 完成
五新UI / 六旧Core+UI；Editor Build / Core89-UI41-Editor3索引 / Game Build：
AudioDraft一次；新WBP十一绑定 / 两宿主八绑定 / 三Valid-Save：
原ScrollBox与滑条焦点 / 5%步进；PIE禁用与原P移动短查：
全新-game首次主菜单自行可用；浏览无副作用 / 取消 / 默认仅草稿：
控件真实应用音乐0 / 音效仍有；最终总50-音乐50-音效100：
Tab-ShiftTab-方向键-Enter-Space / Esc本卡取消 / 无误开始：
Pause离开丢弃 / 显示预览禁用保留草稿-取消恢复 / 菜单新卡：
新进程同ini只读0.5-0.5-1 / 两路试听 / 两轮正常退出：
Source / 三资产Save All，专用ini保持；失败或布局问题（无填无）：
完成时分 / 手动代码 / Designer / 阅读 / 检查 / 构建启动等待：
```

## 10. Commit / Push（固定放在最后）

本章十一源码 / 三Widget已在本地`f363841`。历史实现建议：英文`feat: add shared audio settings UI with explicit draft application`；专业中文`feat: 新增支持草稿确认与实际音量应用的共享设置界面`。已提交内容不重复Stage，远端由你确认。

当前Stage本指南收尾、[验收报告](Reports/M3_05C3_Verification.md)、AssetManifest新卡 / 两宿主、[M3-06A指南](M3_06A_IntegratedPackage.md)、Run-StageBuild.ps1及实际修改的共享状态文档；准确清单见新指南第9节。

- 英文：`docs: verify audio settings UI and prepare M3 package integration`
- 专业中文：`docs: 完成音量设置界面验收并准备M3开发包集成`

C3实现不变；后续06A已补修三份旧测试准备，42项阶段回归全Success，新包尚未构建。该增量与建议fix Commit见06A指南第2.3 / 9节，不需重存C3资产。Artifacts证据 / 个人ini与缓存留本地；Stage后继续修改需重新Stage → Review → Commit / Push由用户操作。
