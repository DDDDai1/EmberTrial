# M3-05B4-02：主菜单与暂停页共用画质设置卡

2026-11-05，UE **5.5.4**。[B4-01已验收](Reports/M3_05B4_01_Verification.md)，Graphics03证实重启读取0 / 30 / 0，无待补。以下为本批手动实施指南；新源码 / 新WBP尚未写入正式工程，没有本批Build或实机成功结论。

## 1. 体验、职责与范围

主菜单右侧、Pause右列加入同一张“画质与帧率”卡：当前配置、画质档、帧率上限、垂直同步、应用 / 取消修改 / 恢复默认。选择只改草稿；应用才调用已验收的GameUserSettings。默认只填本卡的中画质 / 60 FPS / 同步关，不重置窗口、输入、音量或试炼。

离开页面丢弃未应用草稿；重新打开读当前已应用值。显示预览进行中，本卡编辑 / 应用禁用，但草稿保留；确认 / 取消显示预览后恢复。PIE只读、独立-game可改。Core仍负责保存与保护，View不建立另一份设置单例、窗口事务、Tick或Timer。

本批五新 / 四旧UI，一新WBP / 两旧宿主。当前217 Source / 123cpp（Core85 / UI35 / Editor3）、38测试、301 Content / 来源82；实施后预计222 Source / 126cpp（Core85 / UI38 / Editor3）、39测试，新WBP保存后Content302 / 来源83。预算代码60–90 / Designer30–45 / 阅读5–10 / 检查10–15分钟，构建等待另记，不提前计实耗。

## 2. 文件与准备

先Save All、退出所有UE窗口。在CLion创建下列文件。**五个新文件都属于EmberTrialUI，不放进Core的EmberTrial。** tests要放UI模块自己的Tests；不要手建generated.h。

```text
EmberTrial/Source/EmberTrialUI/Public/Settings/ETGraphicsSettingsDraft.h       新
EmberTrial/Source/EmberTrialUI/Private/Settings/ETGraphicsSettingsDraft.cpp    新
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETGraphicsSettingsWidget.h      新
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETGraphicsSettingsWidget.cpp   新
EmberTrial/Source/EmberTrialUI/Tests/ETGraphicsSettingsDraftTests.cpp         新
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h              旧
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp            旧
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h                 旧
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp               旧
```

Core与Editor源码 / Build.cs不改；已有通用BindWidget校验器会识别新类及第七绑定。只有Widget头有generated.h，普通Draft头没有。旧Widget资源父类不变。

## 3. 五份新文件完整代码

### 3.1 Public/Settings/ETGraphicsSettingsDraft.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Settings/ETGraphicsSettingsTypes.h"

/** 页面值副本；关闭丢弃，应用前重验可见偏好快照，不持有引擎对象。 */
class EMBERTRIALUI_API FETGraphicsSettingsDraft
{
public:
    bool Open(const FETGraphicsPreferences& Applied);
    void Close();
    bool SetQuality(int32 Level);
    bool SetFrameRateLimit(float Limit);
    bool SetVSync(bool bEnabled);
    void UseDefaults();

    void GetQualityLevels(TArray<int32>& Out) const;
    void GetFrameRateLimits(TArray<float>& Out) const;
    bool TryMakeApply(const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out) const;

    bool IsOpen() const { return bOpen; }
    bool IsDirty() const { return bOpen && !Same(Value, Baseline); }
    bool IsDefault() const { return bOpen && Same(Value, Defaults()); }
    const FETGraphicsPreferences& GetValue() const { return Value; }
    static FETGraphicsPreferences Defaults();

private:
    static bool Same(const FETGraphicsPreferences& A, const FETGraphicsPreferences& B);
    FETGraphicsPreferences Baseline;
    FETGraphicsPreferences Value;
    bool bOpen = false;
};
```

### 3.2 Private/Settings/ETGraphicsSettingsDraft.cpp

```cpp
#include "Settings/ETGraphicsSettingsDraft.h"

// ---- 页面会话与本卡默认；不调用全局SetToDefaults ----

bool FETGraphicsSettingsDraft::Open(const FETGraphicsPreferences& Applied)
{
    const FETGraphicsPreferences Copy = Applied;
    Close();
    if (!Copy.IsValid())
    {
        return false;
    }
    Baseline = Value = Copy;
    bOpen = true;
    return true;
}

void FETGraphicsSettingsDraft::Close()
{
    bOpen = false;
    Baseline = Value = FETGraphicsPreferences{};
}

FETGraphicsPreferences FETGraphicsSettingsDraft::Defaults()
{
    FETGraphicsPreferences Result;
    Result.QualityLevel = 1;
    Result.FrameRateLimit = 60.0f;
    Result.bUseVSync = false;
    return Result;
}

void FETGraphicsSettingsDraft::UseDefaults()
{
    if (bOpen)
    {
        Value = Defaults();
    }
}

// ---- 选择目录；保留合法的自定义 / 非常用帧率，不伪造混合画质 ----

void FETGraphicsSettingsDraft::GetQualityLevels(TArray<int32>& Out) const
{
    Out.Reset();
    if (!bOpen)
    {
        return;
    }
    if (Baseline.QualityLevel == FETGraphicsPreferences::CustomQuality)
    {
        Out.Add(FETGraphicsPreferences::CustomQuality);
    }
    for (int32 Level = 0; Level <= FETGraphicsPreferences::MaxQuality; ++Level)
    {
        Out.Add(Level);
    }
}

void FETGraphicsSettingsDraft::GetFrameRateLimits(TArray<float>& Out) const
{
    Out.Reset();
    if (!bOpen)
    {
        return;
    }
    Out = {0.0f, 30.0f, 60.0f, 120.0f};
    if (!Out.ContainsByPredicate([this](float Limit)
        { return FMath::IsNearlyEqual(Limit, Baseline.FrameRateLimit); }))
    {
        Out.Add(Baseline.FrameRateLimit);
        Out.Sort();
    }
}

bool FETGraphicsSettingsDraft::SetQuality(const int32 Level)
{
    TArray<int32> Choices;
    GetQualityLevels(Choices);
    if (!bOpen || !Choices.Contains(Level))
    {
        return false;
    }
    Value.QualityLevel = Level;
    return true;
}

bool FETGraphicsSettingsDraft::SetFrameRateLimit(const float Limit)
{
    TArray<float> Choices;
    GetFrameRateLimits(Choices);
    const int32 Index = Choices.IndexOfByPredicate([Limit](float Choice)
        { return FMath::IsNearlyEqual(Choice, Limit); });
    if (!bOpen || !FMath::IsFinite(Limit) || Index == INDEX_NONE)
    {
        return false;
    }
    Value.FrameRateLimit = Choices[Index];
    return true;
}

bool FETGraphicsSettingsDraft::SetVSync(const bool bEnabled)
{
    if (!bOpen)
    {
        return false;
    }
    Value.bUseVSync = bEnabled;
    return true;
}

// ---- 最终候选；过期草稿拒绝，不覆盖其他入口刚应用的偏好 ----

bool FETGraphicsSettingsDraft::Same(
    const FETGraphicsPreferences& A, const FETGraphicsPreferences& B)
{
    return A.IsValid() && B.IsValid() && A.QualityLevel == B.QualityLevel &&
        FMath::IsNearlyEqual(A.FrameRateLimit, B.FrameRateLimit) &&
        A.bUseVSync == B.bUseVSync;
}

bool FETGraphicsSettingsDraft::TryMakeApply(
    const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out) const
{
    const FETGraphicsPreferences Copy = Current;
    Out = FETGraphicsPreferences{};
    return bOpen && IsDirty() && Same(Copy, Baseline) &&
        FETGraphicsSettingsRules::TryResolve(Value, Copy, Out);
}
```

### 3.3 Public/Widgets/ETGraphicsSettingsWidget.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Settings/ETGraphicsSettingsDraft.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETGraphicsSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UTextBlock;
class UETGameUserSettings;
class UETVideoSettingsSubsystem;

/** 共用画质卡；View拥有草稿，原生GameUserSettings负责应用 / 保存。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETGraphicsSettingsWidget : public UUserWidget
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
    void Render(bool bRebuildChoices = false);
    void FocusQuality();
    void HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot);

    UFUNCTION()
    void HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleFrameRateChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleVSyncChanged(bool bChecked);
    UFUNCTION()
    void HandleApply();
    UFUNCTION()
    void HandleRevert();
    UFUNCTION()
    void HandleDefaults();

    // ---- 八项必需绑定；控件程序同步不回写，不使用属性Binding ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> QualityCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> FrameRateCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> VSyncCheckBox;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplyButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> DefaultsButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> GraphicsCurrentText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> GraphicsStatusText;

    TWeakObjectPtr<UETGameUserSettings> Settings;
    TWeakObjectPtr<UETVideoSettingsSubsystem> PreviewService;
    FDelegateHandle PreviewChangedHandle;
    FETGraphicsSettingsDraft Draft;
    TArray<int32> QualityLevels;
    TArray<float> FrameRateLimits;
    FText Feedback;
    bool bViewActive = false;
    bool bHostBusy = false;
    bool bSyncingControls = false;
};
```

### 3.4 Private/Widgets/ETGraphicsSettingsWidget.cpp

```cpp
#include "Widgets/ETGraphicsSettingsWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Settings/ETGameUserSettings.h"
#include "Settings/ETVideoSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGraphicsSettingsView, Log, All);

namespace ETGraphicsViewPrivate
{
FString QualityLabel(const int32 Level)
{
    switch (Level)
    {
    case -1: return TEXT("自定义（保留当前组）");
    case 0: return TEXT("低");
    case 1: return TEXT("中");
    case 2: return TEXT("高");
    case 3: return TEXT("极高");
    case 4: return TEXT("影视级");
    default: return TEXT("不可用");
    }
}

FString FrameRateLabel(const float Limit)
{
    if (Limit == 0.0f)
    {
        return TEXT("不限制（0）");
    }
    FNumberFormattingOptions Number;
    Number.SetMinimumFractionalDigits(0);
    Number.SetMaximumFractionalDigits(2);
    return FString::Printf(TEXT("%s FPS"), *FText::AsNumber(Limit, &Number).ToString());
}
} // namespace ETGraphicsViewPrivate

// ---- 初始化只绑一次；每次Construct观察新服务，宿主关闭先撤销观察 ----

bool UETGraphicsSettingsWidget::HasRequiredLayout() const
{
    return IsValid(QualityCombo.Get()) && IsValid(FrameRateCombo.Get()) &&
        IsValid(VSyncCheckBox.Get()) && IsValid(ApplyButton.Get()) &&
        IsValid(RevertButton.Get()) && IsValid(DefaultsButton.Get()) &&
        IsValid(GraphicsCurrentText.Get()) && IsValid(GraphicsStatusText.Get());
}

void UETGraphicsSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    QualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleQualityChanged);
    FrameRateCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleFrameRateChanged);
    VSyncCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &ThisClass::HandleVSyncChanged);
    ApplyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleApply);
    RevertButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRevert);
    DefaultsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDefaults);
}

void UETGraphicsSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsDesignTime() || !HasRequiredLayout())
    {
        return;
    }
    bViewActive = true;
    Settings = UETGameUserSettings::GetETGameUserSettings();
    if (UGameInstance* Instance = GetGameInstance())
    {
        PreviewService = Instance->GetSubsystem<UETVideoSettingsSubsystem>();
    }
    if (UETVideoSettingsSubsystem* Service = PreviewService.Get())
    {
        Service->OnChanged.Remove(PreviewChangedHandle);
        PreviewChangedHandle = Service->OnChanged.AddUObject(
            this, &ThisClass::HandlePreviewSnapshot);
    }
    ReadCurrent(FText::FromString(TEXT("修改后选择应用；离开页面丢弃未应用草稿。")));
}

void UETGraphicsSettingsWidget::NativeDestruct()
{
    CloseView();
    Super::NativeDestruct();
}

void UETGraphicsSettingsWidget::CloseView()
{
    bViewActive = false;
    if (UETVideoSettingsSubsystem* Service = PreviewService.Get())
    {
        Service->OnChanged.Remove(PreviewChangedHandle);
    }
    PreviewChangedHandle.Reset();
    PreviewService.Reset();
    Settings.Reset();
    Draft.Close();
    QualityLevels.Reset();
    FrameRateLimits.Reset();
}

// ---- 资格与呈现；视频预览事件只刷新可用性，不清理本卡未应用草稿 ----

bool UETGraphicsSettingsWidget::HasLiveHost() const
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

bool UETGraphicsSettingsWidget::IsPreviewBusy() const
{
    return PreviewService.IsValid() &&
        PreviewService->GetSnapshot().Phase != EETVideoPreviewPhase::Idle;
}

bool UETGraphicsSettingsWidget::CanEdit() const
{
    const UWorld* World = GetWorld();
    return bViewActive && !bHostBusy && Draft.IsOpen() && Settings.IsValid() &&
        PreviewService.IsValid() && !IsPreviewBusy() && !GIsEditor && World &&
        !World->bIsTearingDown && World->WorldType == EWorldType::Game &&
        World->GetNetMode() == NM_Standalone;
}

void UETGraphicsSettingsWidget::SetHostBusy(const bool bBusy)
{
    bHostBusy = bBusy;
    Render();
}

void UETGraphicsSettingsWidget::ReadCurrent(const FText& Message)
{
    FETGraphicsPreferences Current;
    const bool bReady = Settings.IsValid() &&
        Settings->ReadGraphicsPreferences(Current) && Draft.Open(Current);
    if (!bReady)
    {
        Draft.Close();
    }
    Feedback = bReady ? Message : FText::FromString(TEXT("图形设置暂不可用。"));
    Render(true);
}

void UETGraphicsSettingsWidget::HandlePreviewSnapshot(const FETVideoPreviewSnapshot& Snapshot)
{
    (void)Snapshot;
    if (bViewActive)
    {
        Render(); // 不重建下拉目录、不重读覆盖草稿、不抢视频确认按钮的焦点。
    }
}

void UETGraphicsSettingsWidget::Render(const bool bRebuildChoices)
{
    if (!bViewActive || !HasRequiredLayout())
    {
        return;
    }
    const TGuardValue<bool> SyncGuard(bSyncingControls, true);
    if (bRebuildChoices)
    {
        Draft.GetQualityLevels(QualityLevels);
        Draft.GetFrameRateLimits(FrameRateLimits);
        QualityCombo->ClearOptions();
        FrameRateCombo->ClearOptions();
        for (const int32 Level : QualityLevels)
        {
            QualityCombo->AddOption(ETGraphicsViewPrivate::QualityLabel(Level));
        }
        for (const float Limit : FrameRateLimits)
        {
            FrameRateCombo->AddOption(ETGraphicsViewPrivate::FrameRateLabel(Limit));
        }
    }
    const FETGraphicsPreferences& Value = Draft.GetValue();
    QualityCombo->SetSelectedIndex(QualityLevels.IndexOfByKey(Value.QualityLevel));
    FrameRateCombo->SetSelectedIndex(FrameRateLimits.IndexOfByPredicate([&Value](float Limit)
        { return FMath::IsNearlyEqual(Limit, Value.FrameRateLimit); }));
    VSyncCheckBox->SetIsChecked(Value.bUseVSync);

    FETGraphicsPreferences Applied;
    const bool bReady = Settings.IsValid() && Settings->ReadGraphicsPreferences(Applied);
    GraphicsCurrentText->SetText(bReady ? FText::FromString(FString::Printf(
        TEXT("当前配置：%s / %s / 垂直同步%s"),
        *ETGraphicsViewPrivate::QualityLabel(Applied.QualityLevel),
        *ETGraphicsViewPrivate::FrameRateLabel(Applied.FrameRateLimit),
        Applied.bUseVSync ? TEXT("开") : TEXT("关"))) :
        FText::FromString(TEXT("当前配置不可用")));

    FText Message = Feedback;
    if (GIsEditor)
    {
        Message = FText::FromString(TEXT("请在独立游戏窗口中调整图形设置。"));
    }
    else if (IsPreviewBusy())
    {
        Message = FText::FromString(TEXT("请先确认或取消显示预览；本卡草稿保留。"));
    }
    else if (bHostBusy)
    {
        Message = FText::FromString(TEXT("正在切换页面，请稍候。"));
    }
    GraphicsStatusText->SetText(Message);

    const bool bEdit = CanEdit();
    QualityCombo->SetIsEnabled(bEdit);
    FrameRateCombo->SetIsEnabled(bEdit);
    VSyncCheckBox->SetIsEnabled(bEdit);
    ApplyButton->SetIsEnabled(bEdit && Draft.IsDirty());
    RevertButton->SetIsEnabled(bEdit && Draft.IsDirty());
    DefaultsButton->SetIsEnabled(bEdit && !Draft.IsDefault());
}

void UETGraphicsSettingsWidget::FocusQuality()
{
    if (CanEdit() && HasLiveHost() && IsValid(GetOwningPlayer()))
    {
        QualityCombo->SetUserFocus(GetOwningPlayer());
    }
}

// ---- 编辑只改值副本；只在独立应用按钮中交给Core提交 ----

void UETGraphicsSettingsWidget::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;
    const int32 Index = QualityCombo->GetSelectedIndex();
    if (!bSyncingControls && CanEdit() && QualityLevels.IsValidIndex(Index) &&
        Draft.SetQuality(QualityLevels[Index]))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

void UETGraphicsSettingsWidget::HandleFrameRateChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    (void)SelectedItem;
    (void)SelectionType;
    const int32 Index = FrameRateCombo->GetSelectedIndex();
    if (!bSyncingControls && CanEdit() && FrameRateLimits.IsValidIndex(Index) &&
        Draft.SetFrameRateLimit(FrameRateLimits[Index]))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

void UETGraphicsSettingsWidget::HandleVSyncChanged(const bool bChecked)
{
    if (!bSyncingControls && CanEdit() && Draft.SetVSync(bChecked))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

void UETGraphicsSettingsWidget::HandleApply()
{
    if (!CanEdit() || !HasLiveHost() || !Draft.IsDirty())
    {
        return;
    }
    UETGameUserSettings* Current = Settings.Get();
    FETGraphicsPreferences Applied;
    FETGraphicsPreferences Request;
    if (!Current->ReadGraphicsPreferences(Applied) || !Draft.TryMakeApply(Applied, Request))
    {
        ReadCurrent(FText::FromString(TEXT("配置已由其他入口更改，已刷新，请重新选择。")));
        FocusQuality();
        return;
    }

    const TStrongObjectPtr<UETGraphicsSettingsWidget> KeepAlive(this);
    if (!Current->ApplyGraphicsPreferences(Request, GetWorld()))
    {
        Feedback = FText::FromString(TEXT("暂不能应用，请确认显示预览已结束。"));
        Render();
        return;
    }
    UE_LOG(LogETGraphicsSettingsView, Display,
        TEXT("[M3-05B4-02] Applied Widget=%s Quality=%d Cap=%.2f VSync=%d"),
        *GetName(), Request.QualityLevel, Request.FrameRateLimit, Request.bUseVSync ? 1 : 0);
    if (bViewActive && HasLiveHost() && Settings.Get() == Current)
    {
        ReadCurrent(FText::FromString(TEXT("已应用并请求保存；实际帧率仍受性能和同步限制。")));
        FocusQuality();
    }
}

void UETGraphicsSettingsWidget::HandleRevert()
{
    if (CanEdit())
    {
        ReadCurrent(FText::FromString(TEXT("已取消修改，保留当前配置。")));
        FocusQuality();
    }
}

void UETGraphicsSettingsWidget::HandleDefaults()
{
    if (CanEdit())
    {
        Draft.UseDefaults();
        Feedback = FText::FromString(TEXT("草稿已填入中画质 / 60 FPS / 同步关，应用后生效。"));
        Render();
        FocusQuality();
    }
}

FReply UETGraphicsSettingsWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (HasRequiredLayout() && InKeyEvent.GetKey() == EKeys::Escape &&
        !QualityCombo->IsOpen() && !FrameRateCombo->IsOpen())
    {
        HandleRevert();
        return FReply::Handled();
    }
    // 展开列表先自行处理Esc；P继续冒泡给Pause，Enter / Space由焦点控件处理。
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
```

### 3.5 Tests/ETGraphicsSettingsDraftTests.cpp

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETGraphicsSettingsDraft.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETGraphicsSettingsDraftTest,
    "EmberTrial.UI.Settings.GraphicsDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETGraphicsSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETGraphicsSettingsDraft Draft;
    FETGraphicsPreferences Current = FETGraphicsSettingsDraft::Defaults();
    FETGraphicsPreferences Out;
    TestFalse(TEXT("Closed draft rejects edits"), Draft.SetQuality(0));
    TestFalse(TEXT("Unavailable snapshot rejected"), Draft.Open(FETGraphicsPreferences{}));
    TestTrue(TEXT("Open valid defaults"), Draft.Open(Current));
    TestFalse(TEXT("New draft is clean"), Draft.IsDirty());
    TestTrue(TEXT("Default presentation"), Draft.IsDefault());
    TestFalse(TEXT("Unchanged draft cannot save"), Draft.TryMakeApply(Current, Out));
    TestTrue(TEXT("Select low quality"), Draft.SetQuality(0));
    TestTrue(TEXT("Select 30 cap"), Draft.SetFrameRateLimit(30.0f));
    TestTrue(TEXT("Toggle VSync in draft"), Draft.SetVSync(true));
    TestEqual(TEXT("Browsing has not changed current"), Current.QualityLevel, 1);
    TestTrue(TEXT("Dirty draft produces request"), Draft.TryMakeApply(Current, Out));
    TestEqual(TEXT("Candidate low"), Out.QualityLevel, 0);
    TestEqual(TEXT("Candidate cap"), Out.FrameRateLimit, 30.0f);
    TestTrue(TEXT("Candidate VSync"), Out.bUseVSync);
    FETGraphicsPreferences Changed = Current;
    Changed.FrameRateLimit = 120.0f;
    TestFalse(TEXT("External change rejects old page"), Draft.TryMakeApply(Changed, Out));
    TestFalse(TEXT("Rejected output invalid"), Out.IsValid());
    TestFalse(TEXT("Unsupported quality rejected"), Draft.SetQuality(5));
    TestFalse(TEXT("Arbitrary cap not added by browsing"), Draft.SetFrameRateLimit(45.0f));
    TestTrue(TEXT("Rejected edits preserve existing draft"), Draft.IsDirty());
    Draft.UseDefaults();
    TestFalse(TEXT("Defaults match original baseline"), Draft.IsDirty());

    Current.QualityLevel = -1;
    Current.FrameRateLimit = 59.94f;
    TestTrue(TEXT("Mixed quality and custom cap readable"), Draft.Open(Current));
    TArray<float> Limits;
    Draft.GetFrameRateLimits(Limits);
    TestTrue(TEXT("Preserve existing custom cap"), Limits.Contains(59.94f));
    TestTrue(TEXT("Can propose preset"), Draft.SetQuality(3));
    TestTrue(TEXT("Can return to original mixed groups"), Draft.SetQuality(-1));
    TestTrue(TEXT("Can cancel cap"), Draft.SetFrameRateLimit(0.0f));
    TestTrue(TEXT("Mixed groups survive cap-only request"), Draft.TryMakeApply(Current, Out));
    TestEqual(TEXT("Do not overwrite mixed groups"), Out.QualityLevel, -1);
    TestTrue(TEXT("Open permits alias of draft value"), Draft.Open(Draft.GetValue()));
    TestFalse(TEXT("Alias reopen is clean"), Draft.IsDirty());
    Draft.Close();
    TestFalse(TEXT("Closed view cannot commit"), Draft.TryMakeApply(Current, Out));
    return true;
}
#endif
```

## 4. 两个宿主各六处增量

以下每项都是**查找旧块 → 替换为新块**，不要用新片段覆盖整个public / private区或整份文件。所有旧按钮、六个旧绑定、方法声明和实现均保留。MainMenu与Pause各执行六项，共十二项；本节按文件分组。

### 4.1 ETMainMenuWidget：逐项完成以下六处

1. **头文件：新增完整类型的前置声明**，文件：`Public/Widgets/ETMainMenuWidget.h`。

查找：

```cpp
class UETVideoSettingsWidget;
```

替换为：

```cpp
class UETVideoSettingsWidget;
class UETGraphicsSettingsWidget;
```

2. **头文件：保留VideoSettingsPanel，在其后增加第七绑定**，文件：`Public/Widgets/ETMainMenuWidget.h`。

查找：

```cpp
    TObjectPtr<UETVideoSettingsWidget> VideoSettingsPanel;
```

替换为：

```cpp
    TObjectPtr<UETVideoSettingsWidget> VideoSettingsPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETGraphicsSettingsWidget> GraphicsSettingsPanel;
```

3. **实现：引入Graphics Widget头**，文件：`Private/Widgets/ETMainMenuWidget.cpp`。

查找：

```cpp
#include "Widgets/ETVideoSettingsWidget.h"
```

替换为：

```cpp
#include "Widgets/ETVideoSettingsWidget.h"
#include "Widgets/ETGraphicsSettingsWidget.h"
```

4. **实现：布局检查包含第七项**，文件：`Private/Widgets/ETMainMenuWidget.cpp`。

查找：

```cpp
        IsValid(VideoSettingsPanel.Get());
```

替换为：

```cpp
        IsValid(VideoSettingsPanel.Get()) && IsValid(GraphicsSettingsPanel.Get());
```

5. **实现：宿主析构先关闭画质观察，再执行原视频清理**，文件：`Private/Widgets/ETMainMenuWidget.cpp`。

查找：

```cpp
void UETMainMenuWidget::NativeDestruct()
{
```

替换为：

```cpp
void UETMainMenuWidget::NativeDestruct()
{
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->CloseView();
    }
```

6. **实现：Pending先传画质卡，再传原视频卡**，文件：`Private/Widgets/ETMainMenuWidget.cpp`。

查找：

```cpp
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->SetHostBusy(bBusy);
    }
```

替换为：

```cpp
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->SetHostBusy(bBusy);
    }
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->SetHostBusy(bBusy);
    }
```

### 4.2 ETPauseWidget：逐项完成以下六处

1. **头文件：新增完整类型的前置声明**，文件：`Public/Widgets/ETPauseWidget.h`。

查找：

```cpp
class UETVideoSettingsWidget;
```

替换为：

```cpp
class UETVideoSettingsWidget;
class UETGraphicsSettingsWidget;
```

2. **头文件：保留VideoSettingsPanel，在其后增加第七绑定**，文件：`Public/Widgets/ETPauseWidget.h`。

查找：

```cpp
    TObjectPtr<UETVideoSettingsWidget> VideoSettingsPanel;
```

替换为：

```cpp
    TObjectPtr<UETVideoSettingsWidget> VideoSettingsPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETGraphicsSettingsWidget> GraphicsSettingsPanel;
```

3. **实现：引入Graphics Widget头**，文件：`Private/Widgets/ETPauseWidget.cpp`。

查找：

```cpp
#include "Widgets/ETVideoSettingsWidget.h"
```

替换为：

```cpp
#include "Widgets/ETVideoSettingsWidget.h"
#include "Widgets/ETGraphicsSettingsWidget.h"
```

4. **实现：布局检查包含第七项**，文件：`Private/Widgets/ETPauseWidget.cpp`。

查找：

```cpp
        IsValid(VideoSettingsPanel.Get());
```

替换为：

```cpp
        IsValid(VideoSettingsPanel.Get()) && IsValid(GraphicsSettingsPanel.Get());
```

5. **实现：宿主析构先关闭画质观察，再执行原视频清理**，文件：`Private/Widgets/ETPauseWidget.cpp`。

查找：

```cpp
void UETPauseWidget::NativeDestruct()
{
```

替换为：

```cpp
void UETPauseWidget::NativeDestruct()
{
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->CloseView();
    }
```

6. **实现：Pending先传画质卡，再传原视频卡**，文件：`Private/Widgets/ETPauseWidget.cpp`。

查找：

```cpp
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->SetHostBusy(bBusy);
    }
```

替换为：

```cpp
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->SetHostBusy(bBusy);
    }
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->SetHostBusy(bBusy);
    }
```

第5项只替换函数签名与开花括号这几行，**不要删除它后面的原VideoSettingsPanel清理 / Super调用**。完成后NativeDestruct中顺序应为Graphics CloseView → 原Video CloseView → Super。第6项不新增广播或按钮事件。

## 5. 正常Build与索引

源码已保存、UE保持关闭。在PowerShell完整运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后执行Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload，预计Core85 / UI38 / Editor3。新五文件属于UI CodeModel，Draft / Widget / Core设置头可导航。不清缓存或改生成CMake。

此时旧主菜单 / Pause还缺GraphicsSettingsPanel，临时编译警告属于装配未完成；先完成6–7节再Play，不能把BindWidget改Optional或删除HasRequiredLayout检查。

## 6. 新Widget：八绑定与确定样式

### 6.1 创建、父类与根布局

1. 在`/Game/EmberTrial/ui/Settings`新建Widget Blueprint：**WBP_ETGraphicsSettings**，父类选**ETGraphicsSettingsWidget**。若已创建同名资产，先核对父类，别覆盖旧视频 / 输入卡。找不到原生类时确认双Build成功并重开UE。
2. Class Defaults：**Is Focusable=False，Tick Frequency=Never**。新原生类带DisableNativeTick；无Event Tick、Text Binding或Blueprint点击业务。
3. Designer若有默认的空Canvas Panel，先选中并删除这个空根，再从Palette拖**Size Box**成为唯一根，命名GraphicsSettingsSize；不要把Size Box留在未设锚点的Canvas里。Width / Height Override均不勾。唯一子项**Border**名GraphicsSettingsBorder，Brush Color R/G/B=.08、A=.85，Content Padding四向12；Size Box Slot Fill / Fill、Padding0。
4. Border唯一子项**Vertical Box**名GraphicsSettingsContent。按下列树添加，普通Vertical Box Slot都Auto / Fill，不用Fill1撑高卡片：

```text
GraphicsSettingsSize (SizeBox，无固定宽高)
  GraphicsSettingsBorder (Border，Content Padding12)
    GraphicsSettingsContent (VerticalBox)
      GraphicsTitle (Text：“画质与帧率”，20)
      GraphicsCurrentText (Text，必需，14)
      QualityLabel (Text：“画质档”，14)
      QualityCombo (ComboBoxString，必需)
      FrameRateLabel (Text：“帧率上限”，14，Slot Top8)
      FrameRateCombo (ComboBoxString，必需)
      VSyncCheckBox (CheckBox，必需，Slot Top8)
        VSyncLabel (Text：“垂直同步”，14)
      GraphicsButtons (HorizontalBox，Slot Top8)
        ApplyButton (Button，必需) → 子Text：“应用”
        RevertButton (Button，必需) → 子Text：“取消修改”
        DefaultsButton (Button，必需) → 子Text：“恢复默认”
      GraphicsStatusText (Text，必需，14，Slot Top8)
```

### 6.2 每种控件的Details逐项核对

| 外层控件名称 | 类型 / Is Variable | 关键属性 |
| --- | --- | --- |
| QualityCombo、FrameRateCombo | ComboBoxString / True | Default Options空；Is Focusable=True；Enable Gamepad Navigation Mode=False；Font16、Content Padding6、Max List Height200 |
| VSyncCheckBox | CheckBox / True | Is Focusable=True；子Text不勾Variable、不命名成VSyncCheckBox |
| ApplyButton、RevertButton、DefaultsButton | Button / True | Focusable=True；Click / Press Method均Down and Up；子Text14 / 居中 |
| GraphicsCurrentText、GraphicsStatusText | Text Block / True | Auto Wrap Text=True、Wrap Text At280；Min Desired Width0，无Text Bind；Visibility Visible |

1. 两Combo各展开**Style → Item Style**，Text Color与Selected Text Color都用**指定白色RGBA=1**，Inherit不勾。保留折叠框的浅底深色Foreground，不把外层Foreground改白；否则又会出现白底白字。两个都设置。
2. 三按钮的Horizontal Box Slot均Fill1；第一按钮Right4、第二Left4 / Right4、第三Left4，其余Padding0。外层GraphicsButtons的Vertical Box Slot为Auto / Fill。
3. Current占位“当前配置”，Status占位“修改后选择应用；离开页面丢弃未应用草稿。”。其余普通文字不要求Variable，沿用现有中文字体链；不设固定文字高度 / Render Transform平移。
4. 整个根Visibility=Visible、Is Enabled=True。运行时原生逻辑禁用PIE / 预览中的控件；不要用Designer将它们永久Disabled或Hit Test Invisible。
5. 完成全部八项后Compile → Save。此时不用到Graph连接任何函数 / OnClicked / OnSelectionChanged。

## 7. 两宿主装配与滚动布局

卡片增加后用独立右列滚动，原菜单按钮 / 遗物布局保持可见。**只有包含ScrollBox的外层列才设Height Override=680**；各卡根和内部文字仍Auto高。

### 7.1 主菜单：复用三张设置卡

1. 打开`/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`，父类保持ETMainMenuWidget。Hierarchy找到`MainMenuColumns → MainMenuSettingsSize → MainMenuSettingsStack`；已有InputSettingsPanel与VideoSettingsPanel。
2. 只选**MainMenuSettingsStack这个Vertical Box**，右键**Wrap With… → Scroll Box**。UE5.5菜单按名称排序，Scroll Box在S附近；别选它里面的两个子Widget一起Wrap。将新外层命名**MainMenuSettingsScroll**，原Stack和两个实例名保留。
3. MainMenuSettingsSize：Width Override=320、**Height Override=680**；Horizontal Box Slot Auto / Vertical Center、Left Padding32，其余0。新ScrollBox在Size Box Slot为Fill / Fill、Padding0；原Stack在Scroll Box Slot为Horizontal Fill / Vertical Top、Padding0。
4. 选MainMenuSettingsScroll：Orientation=Vertical、Scroll Bar Visibility=Visible、Always Show Scrollbar=False、Consume Mouse Wheel=When Scrolling Possible、**Scroll When Focus Changes=Instant Scroll**、Navigation Destination=Into View。不要关闭整个ScrollBox的输入。
5. 从Palette → User Created拖**WBP_ETGraphicsSettings**到原MainMenuSettingsStack，放在VideoSettingsPanel后；外层实例名设**GraphicsSettingsPanel**、Is Variable=True，Vertical Box Slot Auto / Fill、Top Padding12，其余0。原VideoSettingsPanel Top12，InputSettingsPanel Top0，三个外层实例都保持Visible / Enabled。
6. 原MainMenuActionsSize宽480 / 无固定高、Text_Title字号32 / Auto Wrap / Wrap480、MainMenuScale Scale To Fit / Down Only、背景Padding24保留。不要为迁就右列增高而重新挪标题或把开始按钮放进ScrollBox。

```text
MainMenuColumns
  MainMenuActionsSize (480，原标题 / 三按钮 / Status)
  MainMenuSettingsSize (320 × 680，Left32)
    MainMenuSettingsScroll (ScrollBox)
      MainMenuSettingsStack (原VerticalBox)
        InputSettingsPanel (原WBP)
        VideoSettingsPanel (原WBP，Top12)
        GraphicsSettingsPanel (新WBP，Top12)
```

7. 七绑定为StartButton / ContinueButton / QuitButton / StatusText / InputSettingsPanel / VideoSettingsPanel / GraphicsSettingsPanel。选实例时确认是新Widget外层，不改内部GraphicsSettingsContent的名字去冒充绑定。Compile → Save。

### 7.2 Pause：右列两张卡，左侧输入卡位置不变

1. 打开`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`，父类保持ETPauseWidget。Hierarchy找PauseColumns最后的**PauseVideoSettingsSize**；它目前唯一子项是VideoSettingsPanel。
2. 选这个**VideoSettingsPanel外层实例**，右键Wrap With… → **Vertical Box**，新包装名**PauseSettingsStack**；VideoSettingsPanel本名及原绑定保留。
3. 选PauseSettingsStack，右键Wrap With… → **Scroll Box**，命名**PauseSettingsScroll**。在原PauseSettingsStack中、VideoSettingsPanel后加入同一WBP_ETGraphicsSettings，外层名**GraphicsSettingsPanel**、Is Variable=True；Auto / Fill、Top12，其余0。
4. PauseVideoSettingsSize名字保留，Width Override=320、**Height Override=680**；Horizontal Box Slot Auto / Vertical Center、Left32，其余0。ScrollBox的Size Box Slot Fill / Fill、Padding0；Stack的Scroll Box Slot Horizontal Fill / Vertical Top、Padding0。
5. PauseSettingsScroll属性与7.1第4点相同，尤其Instant Scroll / Into View。原左PauseActionsSize（含InputSettingsPanel）、中RelicPanel保留；原PauseScale Scale To Fit / Down Only，外边距24。

```text
PauseColumns
  PauseActionsSize (原菜单 / 原InputSettingsPanel)
  RelicPanel (原遗物页)
  PauseVideoSettingsSize (320 × 680，Left32)
    PauseSettingsScroll
      PauseSettingsStack
        VideoSettingsPanel (原WBP)
        GraphicsSettingsPanel (新WBP，Top12)
```

6. 七绑定为BTN_Resume / MainMenuButton / StatusText / RelicPanel / InputSettingsPanel / VideoSettingsPanel / GraphicsSettingsPanel。原Is Focusable=True / Tick Never保留，Compile → Save。
7. Content Browser选新WBP_ETGraphicsSettings与两宿主 → Validate Assets，应三份本项目Valid。若Message Log仍显示旧ET-UI-001，先确认已Compile / Save再重新Validate，看最新结果；不要用Optional隐藏缺失绑定。没有新增Controller组件、地图配置或Blueprint事件链。

## 8. 分层验证（约10–15分钟）

### 8.1 一次草稿专项与PIE短查

停止PIE，在编辑器Output Log执行一次：

```text
Automation RunTests EmberTrial.UI.Settings.GraphicsDraft
```

应Success。覆盖浏览无副作用、默认仅草稿、关闭失效、外部可见快照变化后拒绝、失败清输出、混合组保留、自定义帧率与别名重开；不创建窗口、不Apply / Save。Core专项沿用B4-01，不跑全部39项或新Cook。

常用地图Play → P：右列可滚动找到新卡，PIE提示独立游戏、编辑禁用；原输入 / 显示 / 遗物仍在。P继续后移动转向正常，Stop。先修所有Compile / BindWidget错误再进独立游戏。

### 8.2 独立窗口脚本（完整代码就在本节）

保存关闭UE。整段复制到PowerShell执行，新专用GraphicsUIUser.ini与B4-01的GraphicsUser.ini分开，不改共享Config：

```powershell
Set-Location 'E:\UE_Project\Demo'
$graphicsUiRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05B4_02\Runtime'
New-Item -ItemType Directory -Path $graphicsUiRuntime -Force | Out-Null
$graphicsUiProfile = Join-Path $graphicsUiRuntime 'GraphicsUIUser.ini'
$graphicsUiLog = Join-Path $graphicsUiRuntime 'GraphicsUI02.log'
$graphicsUiArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '/Game/EmberTrial/Maps/L_ET_MainMenu'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    ('-GameUserSettingsINI="{0}"' -f $graphicsUiProfile)
    ('-abslog="{0}"' -f $graphicsUiLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $graphicsUiArgs -WindowStyle Normal
```

### 8.3 主菜单：浏览 / 确认 / 默认与键盘

1. 右侧滚动到新卡；初值来自实际配置，不强求固定档位。两个下拉列表普通 / 选中行及折叠值都可读，所有控件通过滚动可见，标题和开始按钮未被覆盖。
2. 选择与当前不同的画质 / 帧率 / VSync，观察**当前配置文字保持原值**、提示未应用；点击取消修改，三个控件回当前值。选择不会切窗口或保存。
3. 使用控件明确应用**低画质 / 30 FPS / VSync开**；若刚好已是该组，可以先选择别组应用再回本组。应用后当前配置更新、应用 / 取消禁用，原生与View各一条应用日志是同一操作的分层记录，不代表保存两次。
4. 点恢复默认：控件草稿为**中 / 60 / VSync关**，当前配置仍为上一步；先取消确认回原值，再恢复默认 → 应用，最终明确保持**1 / 60 / 0**。应用按钮禁用后不能重复提交；默认不是全局SetToDefaults，窗口及输入1 / 不反转保留。
5. 在新卡内Tab / Shift+Tab移动、方向键选项、Space勾选、Enter或Space按按钮；焦点进入屏幕外控件时ScrollBox应自动滚入视野，不能意外开始试炼。可编辑一项后，展开另一下拉框按Esc只收起；再按Esc取消本卡草稿。没有全页Esc退出逻辑。

### 8.4 Pause草稿、显示预览隔离与页面关闭

1. 开始试炼 → P → 右列滚动到新卡，应读到中 / 60 / 同步关。编辑低 / 30但不应用，P继续；重新P打开应仍中 / 60 / 关，证明离开丢弃草稿。
2. 再编辑低 / 30但不应用；向上滚到**原显示卡**选另一个实际可用窗口尺寸并预览。新画质卡应禁用并提示先确认或取消，原草稿保留；不要求抢15秒输入控制台命令。
3. 取消显示预览后回新卡，控件恢复可用、仍是低 / 30草稿，当前配置仍中 / 60 / 关。点新卡取消修改回当前值。事件只刷新可用性，没有每秒清草稿或重建目录。
4. P鼠标或键盘继续一次，移动转向正常；再P返回菜单，新实例仍显示已应用的中 / 60 / 关，无旧草稿 / 重影。输入 / 遗物与HUD作最短观察，不重跑战斗 / 装备矩阵。当前已有纯规则覆盖外部快照变化，本批不额外搭控制台改值的临时验收。

### 8.5 重启读取与正常退出

1. 第一轮最终明确保持中 / 60 / VSync关；若仍有未应用草稿，先取消。正常退出。
2. 重新运行**8.2整段脚本**，只把`$graphicsUiLog = ... 'GraphicsUI01.log'`改为`$graphicsUiLog = Join-Path $graphicsUiRuntime 'GraphicsUI02.log'`；其他内容不变。这是本文代码块，不是另一个.ps1文件。
3. 新进程主菜单滚动到画质卡，应显示中 / 60 / 同步关。按`~`打开**游戏控制台**，只执行一次`et.GraphicsSettings`，应Valid1 / Quality1 / Cap60 / VSync0；不先apply，日志由助手读取。这一步不能只用ini或“已启动”代替。
4. 收起控制台，从主菜单正常退出即可。不要求第三轮、重新进战斗或创建新包。资产Save All，两个专用ini / 日志保持Artifacts本地。

## 9. 说明与问题定位

- “当前配置”来自Core读取，不是GPU实际FPS。VSync开启时即使选择不限制，仍可能受同步 / 性能限制；实际性能验收沿用专门流程。
- 自定义只在引擎初始十组混合时出现，表示保留当前组；已有59.94等合法自定义帧率会出现在目录。这里不提供逐组画质编辑，原混合组的内部变化不做逐字段实时监听；最终应用会重验Core公开快照。
- 新卡只订阅原视频服务的状态事件，关页立即解绑；旧页即使被持有也不能应用。没有业务Tick / 世界Timer，ScrollBox自身的Slate布局与滚动属于正常UI工作。
- 高度680只用于有ScrollBox的外层列，卡片本体Auto高。看不到底部时先核对ScrollBox是否确实包住整个Stack、Height Override和焦点自动滚动；不要把卡片强缩放、删按钮或固定文字高。
- 源码报错发送第一条真实error与附近10–20行；布局 / 焦点问题附Hierarchy和可见界面。旧Message Log红字不等于最新编译失败，看当前Compile与Validate结果。

## 10. 完成后反馈

```text
M3-05B4-02 完成
五新 / 四旧UI；关闭UE后Editor Build / Core85-UI38-Editor3索引 / Game Build：
新WBP八绑定 / 两宿主七绑定 / 两ScrollBox焦点滚动；三Valid / Save：
GraphicsDraft一次 / PIE禁用与原P短查：
菜单浏览无副作用 / 取消 / 真实应用 / 默认仅草稿 / 最终中60同步关：
Tab-ShiftTab-方向键-Space-Enter / 展开Esc与草稿Esc / 不误开始：
Pause离开丢弃 / 显示预览禁用并保留草稿 / 取消后恢复：
返回菜单新卡 / 独立重启1-60-0查询 / 两轮日志 / 正常退出：
Source / 三资产Save All，配置留专用ini；失败或布局问题（无填无）：
完成时分 / 手动代码 / Designer / 阅读 / 检查 / 构建启动等待：
```

## 11. Commit / Push（固定放在最后）

### 11.1 当前B4-01收尾与新指南

B4-01五源码已在本地d20de4e，当前Stage其验收报告 / 指南收尾、本指南与实际更新的共享状态文档。英文：`docs: verify graphics preferences and prepare shared graphics settings UI`；专业中文：`docs: 完成画质偏好验收并准备共享画质设置界面`。新源码 / WBP尚未实施，不提前Stage候选 / 空报告。

### 11.2 本章手动实施后

Stage第2节五新 / 四旧UI，及以下真实保存的三个资产：

```text
EmberTrial/Content/EmberTrial/ui/Settings/WBP_ETGraphicsSettings.uasset  新
EmberTrial/Content/EmberTrial/ui/Menu/WBP_ETMainMenu.uasset              旧
EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset               旧
```

英文：`feat: add shared graphics settings UI with draft application`；专业中文：`feat: 新增支持草稿确认的共享画质设置界面`。文档单独提交可用`docs: record shared graphics settings UI verification`；专业中文：`docs: 记录共享画质设置界面验证结果`。

来源清单只在真实资产保存后更新SHA，不预填来源。Artifacts候选 / Runtime / 个人ini、Saved / Intermediate / Binaries、.idea与生成索引留本地。Stage后继续修改需重新Stage → Review → Commit / Push由用户操作。
