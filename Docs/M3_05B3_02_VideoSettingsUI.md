# M3-05B3-02：共享显示设置卡与确认回退

2026-11-05，UE **5.5.4**。[B3-01已验收](Reports/M3_05B3_01_Verification.md)，无待补。以下为手动实施步骤；正式源码 / 资产尚未修改，没有本章Build或实机成功结论。

## 1. 本章体验与所有权

主菜单右列、原Pause第三列出现同一张显示卡：当前实际状态、模式 / 分辨率选择、预览 / 取消修改，以及预览中的保留设置 / 取消预览和实时时限。倒计时提示与确认按钮就在卡内，不额外增加全屏弹窗或第二层世界暂停。

| 操作 | 预期行为 |
| --- | --- |
| 选择模式 / 分辨率 | 只改B3-01草稿；当前窗口 / 配置不变 |
| 取消修改 / 未预览时卡内Esc | 重新读取实际快照，丢弃草稿 |
| 预览 | 用本Widget作为Owner，保存返回的Serial；实际应用后开启Core的15秒确认 |
| 保留设置 | 只提交自己的Serial，Core重验期限 / 实际模式，再请求保存 |
| 取消预览 / 不确认 | 申请并观察实际基线恢复，重新读取草稿 |
| P继续 / 切图 / 关闭页面 | 宿主立即CloseView：先解绑、再取消自己的Serial，不把未确认候选自动保存 |
| PIE | 显示独立游戏提示，调整控件禁用；编辑器窗口不切屏 |

Core继续拥有事务、平台时钟与窗口副作用。新View订阅服务的状态 / 整秒事件，无Tick、属性Binding、World Timer或输入Action。Begin同步广播发生在Serial返回前，因此View暂时忽略这一段回调，返回后认领Serial并读取当前快照；旧页不得拿全局最新Serial确认别人的预览。

实际212 Source / 120cpp（Core83 / UI34 / Editor3）、37测试、300 Content / 来源81。两新 / 四旧UI后预计214 Source / 121cpp（Core83 / UI35 / Editor3），测试37；新WBP实物保存后Content预计301、来源预计82，未创建前不先登记。

预算代码90–130 / Designer30–45 / 阅读5–10 / 检查10–15分钟；可分代码与资产两次完成。最终美术排版仍在后期，本批保证中文、操作层次与小窗口可用。

## 2. 准备与文件范围

Save All、Stop并关闭UE，在CLion创建两份新文件。新Widget为UCLASS，generated.h是最后一条include；不要手建生成头。仓库根为`E:\UE_Project\Demo`：

```text
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETVideoSettingsWidget.h      新
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETVideoSettingsWidget.cpp   新
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h           旧
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp        旧
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h              旧
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp           旧
```

UI Build.cs已有UMG / Slate / Core依赖，本批不加Core→UI反向依赖。旧Controller / FailureFlow / UI Bootstrap不增加组件或Class引用，保留原主菜单和Pause所有者。

## 3. 两份新文件完整代码

### 3.1 Public/Widgets/ETVideoSettingsWidget.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Settings/ETVideoSettingsDraft.h"
#include "Settings/ETVideoPreviewTransaction.h"
#include "ETVideoSettingsWidget.generated.h"

class UButton;
class UTextBlock;
class UETVideoSettingsSubsystem;

/** 主菜单 / Pause共用显示卡；Core拥有真实预览，本View拥有草稿与本次Serial。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETVideoSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void SetHostBusy(bool bBusy);
    void CloseView(); // 宿主移除时立即封口；子控件稍后Destruct可重复调用。

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent) override;

private:
    bool HasLiveHost() const;
    bool CanEdit() const;
    bool Capture(FETVideoMode& Actual, FETVideoResolutionCatalogue& Available) const;
    void ReadCurrent(const FText& Message);
    void Render(bool bRebuildChoices = false);
    void FocusMode();
    void CancelOwnedPreview();
    void HandleSnapshot(const FETVideoPreviewSnapshot& Snapshot);

    UFUNCTION()
    void HandleModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandlePreview();
    UFUNCTION()
    void HandleRevert();
    UFUNCTION()
    void HandleConfirm();
    UFUNCTION()
    void HandleCancel();

    // ---- 八项Designer必需绑定；选项由C++生成，无属性Binding ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ModeCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> ResolutionCombo;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> PreviewButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ConfirmButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> CancelPreviewButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CurrentModeText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> VideoStatusText;

    TWeakObjectPtr<UETVideoSettingsSubsystem> Service;
    FDelegateHandle PreviewChangedHandle;
    FETVideoSettingsDraft Draft;
    TArray<EETVideoWindowMode> Modes;
    TArray<FIntPoint> Resolutions;
    FText Feedback;
    uint64 PreviewSerial = 0;
    bool bViewActive = false;
    bool bHostBusy = false;
    bool bSyncing = false;
    bool bStartingPreview = false;
};
```

### 3.2 Private/Widgets/ETVideoSettingsWidget.cpp

```cpp
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

DEFINE_LOG_CATEGORY_STATIC(LogETVideoSettingsView, Log, All);

namespace ETVideoViewPrivate
{
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
}

void UETVideoSettingsWidget::NativeDestruct()
{
    CloseView();
    Super::NativeDestruct();
}

void UETVideoSettingsWidget::CloseView()
{
    bViewActive = false;
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
```


## 4. 四份既有宿主文件精确修改

按顺序改。同一文件可能出现多小节，不另建重复类。每处都提供查找原文与替换内容；ShowReady段完整替换后，前面增加NativeDestruct，原ShowReady也会保留，不重复粘贴第二份。

### 4.1 Public/Widgets/ETMainMenuWidget.h

增加新卡前置声明。

查找：

```cpp
class UETInputSettingsWidget;
```

替换为：

```cpp
class UETInputSettingsWidget;
class UETVideoSettingsWidget;
```

### 4.2 Public/Widgets/ETMainMenuWidget.h

宿主退出主动关闭嵌套卡，不依赖嵌套Slate控件何时释放。

查找：

```cpp
    virtual void NativeOnInitialized() override;
```

替换为：

```cpp
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
```

### 4.3 Public/Widgets/ETMainMenuWidget.h

在原busy注释前新增第六个必需绑定，保留原输入卡。

查找：

```cpp
    // busy仅表示请求处理中，不等于世界暂停；Continue来自Session快照。
```

替换为：

```cpp
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETVideoSettingsWidget> VideoSettingsPanel;

    // busy仅表示请求处理中，不等于世界暂停；Continue来自Session快照。
```

### 4.4 Private/Widgets/ETMainMenuWidget.cpp

引入完整新卡类型。

查找：

```cpp
#include "Widgets/ETInputSettingsWidget.h"
```

替换为：

```cpp
#include "Widgets/ETInputSettingsWidget.h"
#include "Widgets/ETVideoSettingsWidget.h"
```

### 4.5 Private/Widgets/ETMainMenuWidget.cpp

原布局检查末尾增加VideoSettingsPanel。

查找：

```cpp
IsValid(InputSettingsPanel.Get());
```

替换为：

```cpp
IsValid(InputSettingsPanel.Get()) &&
        IsValid(VideoSettingsPanel.Get());
```

### 4.6 Private/Widgets/ETMainMenuWidget.cpp

ApplyState透传宿主Pending；返回菜单 / 开始切图时取消未确认预览。

查找：

```cpp
    if (InputSettingsPanel)
    {
        InputSettingsPanel->SetHostBusy(bBusy);
    }
```

替换为：

```cpp
    if (InputSettingsPanel)
    {
        InputSettingsPanel->SetHostBusy(bBusy);
    }
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->SetHostBusy(bBusy);
    }
```

### 4.7 Private/Widgets/ETMainMenuWidget.cpp

完整替换此段：增加NativeDestruct，原ShowReady签名 / 内容一并保留在下面。

查找：

```cpp
void UETMainMenuWidget::ShowReady(bool bCanContinue)
{
    bContinueAvailable = bCanContinue;
    ApplyState(false,
               FText::FromString(bCanContinue ? TEXT("可继续本次试炼；关闭游戏后会话检查点清除。")
                                              : TEXT("尚无本次试炼；开始后可在当前进程内继续。")));
}

```

替换为：

```cpp
void UETMainMenuWidget::NativeDestruct()
{
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->CloseView();
    }
    Super::NativeDestruct();
}

void UETMainMenuWidget::ShowReady(bool bCanContinue)
{
    bContinueAvailable = bCanContinue;
    ApplyState(false,
               FText::FromString(bCanContinue ? TEXT("可继续本次试炼；关闭游戏后会话检查点清除。")
                                              : TEXT("尚无本次试炼；开始后可在当前进程内继续。")));
}

```

### 4.8 Public/Widgets/ETPauseWidget.h

增加新卡前置声明。

查找：

```cpp
class UETInputSettingsWidget;
```

替换为：

```cpp
class UETInputSettingsWidget;
class UETVideoSettingsWidget;
```

### 4.9 Public/Widgets/ETPauseWidget.h

宿主退出主动关闭嵌套卡，不依赖嵌套Slate控件何时释放。

查找：

```cpp
    virtual void NativeOnInitialized() override;
```

替换为：

```cpp
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;
```

### 4.10 Public/Widgets/ETPauseWidget.h

在原busy注释前新增第六个必需绑定，保留原输入卡。

查找：

```cpp
    // busy只保护本View请求中的输入，不代表世界暂停或角色死亡。
```

替换为：

```cpp
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETVideoSettingsWidget> VideoSettingsPanel;

    // busy只保护本View请求中的输入，不代表世界暂停或角色死亡。
```

### 4.11 Private/Widgets/ETPauseWidget.cpp

引入完整新卡类型。

查找：

```cpp
#include "Widgets/ETInputSettingsWidget.h"
```

替换为：

```cpp
#include "Widgets/ETInputSettingsWidget.h"
#include "Widgets/ETVideoSettingsWidget.h"
```

### 4.12 Private/Widgets/ETPauseWidget.cpp

原布局检查末尾增加VideoSettingsPanel。

查找：

```cpp
IsValid(InputSettingsPanel.Get());
```

替换为：

```cpp
IsValid(InputSettingsPanel.Get()) &&
        IsValid(VideoSettingsPanel.Get());
```

### 4.13 Private/Widgets/ETPauseWidget.cpp

ApplyState透传宿主Pending；返回菜单 / 开始切图时取消未确认预览。

查找：

```cpp
    if (InputSettingsPanel)
    {
        InputSettingsPanel->SetHostBusy(bBusy);
    }
```

替换为：

```cpp
    if (InputSettingsPanel)
    {
        InputSettingsPanel->SetHostBusy(bBusy);
    }
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->SetHostBusy(bBusy);
    }
```

### 4.14 Private/Widgets/ETPauseWidget.cpp

完整替换此段：增加NativeDestruct，原ShowReady签名 / 内容一并保留在下面。

查找：

```cpp
void UETPauseWidget::ShowReady()
{
    ApplyState(false, FText::FromString(TEXT("松开 P 或选择继续游戏返回。")));
}

```

替换为：

```cpp
void UETPauseWidget::NativeDestruct()
{
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->CloseView();
    }
    Super::NativeDestruct();
}

void UETPauseWidget::ShowReady()
{
    ApplyState(false, FText::FromString(TEXT("松开 P 或选择继续游戏返回。")));
}

```


## 5. 正常Build与索引

保存六文件，UE保持关闭。PowerShell逐段完整复制，路径中的反斜杠不能省略：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload，预计Core83 / UI35 / Editor3；ComboBoxString与新Widget头可导航。此时打开UE后旧MainMenu / Pause尚缺第六绑定属未完成装配，先完成第6–7节再Play，不改Optional绕过检查。

## 6. 新WBP：八项绑定与确定布局

### 6.1 新建与根容器

1. 在`/Game/EmberTrial/ui/Settings`新建Widget Blueprint，命名**WBP_ETVideoSettings**；创建时父类选**ETVideoSettingsWidget**。若列表找不到，先确认第5节Build成功并重开UE，再用All Classes搜索。已有同名文件时先检查，不覆盖旧业务资产。
2. Class Defaults：**Is Focusable=False，Tick Frequency=Never**。新原生类带DisableNativeTick，不做Tick Graph / Text Binding / OnClicked蓝图业务。
3. Designer若有空Canvas根，替换为空**Size Box**根，命名VideoSettingsSize；Width / Height Override均不勾。根下唯一子项放**Border**，命名VideoSettingsBorder，Brush Color可用R/G/B=0.08、A=0.85，Content Padding四向12；Border Slot Padding0、Fill / Fill。
4. Border唯一子项放**Vertical Box**，命名VideoSettingsContent。以下内容均按树顺序添加；普通文字的容器Slot=Auto / Fill，Padding0，额外间距单独注明。

```text
VideoSettingsSize
  VideoSettingsBorder (Border，Content Padding12)
    VideoSettingsContent (VerticalBox)
      VideoTitle (Text：“显示设置”，字号20)
      CurrentModeText (Text，必需)
      ModeLabel (Text：“窗口模式”，字号14)
      ModeCombo (ComboBoxString，必需)
      ResolutionLabel (Text：“分辨率”，字号14)
      ResolutionCombo (ComboBoxString，必需)
      EditButtons (HorizontalBox，Top8)
        PreviewButton (Button，必需) → 子Text：“预览”
        RevertButton (Button，必需) → 子Text：“取消修改”
      ConfirmButtons (HorizontalBox，Top8)
        ConfirmButton (Button，必需) → 子Text：“保留设置”
        CancelPreviewButton (Button，必需) → 子Text：“取消预览”
      VideoStatusText (Text，必需，Top8)
```

### 6.2 名称 / 类型 / 文字属性逐项核对

1. **八个必需名称**是ModeCombo、ResolutionCombo、PreviewButton、RevertButton、ConfirmButton、CancelPreviewButton、CurrentModeText、VideoStatusText。在Hierarchy选各自外层，勾**Is Variable**；Button的子Text不占绑定，不能把子Text命名成Button字段。
2. 两ComboBoxString在Details的Content → Default Options清空，不手填Windowed / 1920等选项；运行时C++生成。Is Focusable=True；Enable Gamepad Navigation Mode=False以便键盘方向键直接导航，当前不开发手柄功能。Max List Height=200，Content Padding=6，Font Size=16。列表行的Item Style → Text Color / Selected Text Color设为指定白色，完整位置与操作见第8.7.2节。若选项藏在Advanced，展开对应区，不到Graph接事件。
3. 四个Button：Is Focusable=True、Click Method=**Down and Up**、Press Method=**Down and Up**、Touch Method保留默认；子Text字号14、居中。每个Horizontal Box Slot设**Fill1**，左按钮Padding Right4，其余0；右按钮Left4，其余0。两排的VerticalBox Slot为Auto / Fill。
4. CurrentModeText占位“当前显示状态”，VideoStatusText占位“选择模式和分辨率后预览；确认后才保存。”。两者字号14、Auto Wrap Text=True、Wrap Text At=**296**；其余标签无Binding。原项目中文字体能正常显示即沿用。
5. 默认Visibility都保留Visible，运行Render会在空闲时折叠确认按钮。整个根Visibility=Visible / Is Enabled=True；不要把父容器设Hit Test Invisible导致内部不能点。
6. Compile → Save。无Required Widget Binding / Native Tick警告后，Content Browser右键新资产 → Asset Actions → Validate Assets，看到本项目Valid；无新Editor校验代码。若只残留上一条Message Log，先看时间，重新Compile / Save / Validate，不在运行时忽略真实缺失。

## 7. 两宿主嵌入同一资产，均为第六绑定

### 7.1 主菜单：右列上下两张卡

1. 打开`/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`。父类保持ETMainMenuWidget，原MainMenuScale / MainMenuColumns、左MainMenuActionsSize与三个按钮 / StatusText保持。
2. 找**MainMenuSettingsSize**（Width Override320、无Height Override），其唯一子项原为InputSettingsPanel。选这个已有InputSettingsPanel外层实例，右键**Wrap With → Vertical Box**，新容器命名MainMenuSettingsStack。保持原实例名称 / Is Variable，不能Duplicate原输入卡。
3. 将新**WBP_ETVideoSettings**从Palette → User Created拖进MainMenuSettingsStack，放在InputSettingsPanel下面。选新UserWidget实例外层，改名**VideoSettingsPanel**，勾Is Variable。它必须是ETVideoSettingsWidget类型，不能拿普通VerticalBox代替。
4. 两卡的Vertical Box Slot为Auto / Fill；原输入卡Padding0，新显示卡Top12、其余0。MainMenuSettingsStack在Size Box Slot为Fill / Fill、Padding0。外层Width320、左间距32、Vertical Alignment Center与原Scale To Fit / Down Only保留，无固定Height。
5. 最终右列：`MainMenuSettingsSize → MainMenuSettingsStack → InputSettingsPanel + VideoSettingsPanel`。原主菜单Is Focusable=True / Tick Never；标题Text_Title的左列换行设置见第8.7.1节，避免长标题越界覆盖右列。Compile → Save / Validate，应有本项目Valid。

### 7.2 Pause：新增第三列，原两列不挪业务

1. 打开`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`，父类保持ETPauseWidget。找到PauseScale下的**PauseColumns HorizontalBox**；现有左PauseActionsSize Width320（含原菜单与输入卡）、右RelicPanel均保留，不把显示卡塞进遗物列表。
2. 在PauseColumns中，RelicPanel右侧加**Size Box**，命名PauseVideoSettingsSize；Width Override=**320**，Height Override不勾；HorizontalBox Slot Size=Auto、Vertical Alignment=Center、Padding Left32，其余0。
3. SizeBox唯一子项拖入**同一份WBP_ETVideoSettings**，外层实例命名**VideoSettingsPanel**、Is Variable=True。Size Box Slot Fill / Fill、Padding0，Visibility Visible、Is Enabled True。
4. 最终层级：

```text
原Canvas / 全屏Border
  PauseScale (Scale To Fit / Down Only)
    PauseColumns
      PauseActionsSize (Width320，原菜单 / 原InputSettingsPanel)
      RelicPanel (旧遗物业务)
      PauseVideoSettingsSize (Width320，Left32)
        VideoSettingsPanel (新WBP实例)
```

5. 原Pause的六绑定为BTN_Resume / MainMenuButton / StatusText / RelicPanel / InputSettingsPanel / VideoSettingsPanel。Is Focusable=True / Tick Never，Compile → Save / Validate。原ScaleBox可在小窗口整体缩小；若裁切先查固定Height、Fill1列宽或根Padding，不缩掉按钮或删遗物内容。
6. 三资产Save All；Controller、地图、UIBootstrap与Flow不增加新引用。没有新增Blueprint按键 / 事件连线。

## 8. 分层验证（约10–15分钟）

### 8.1 两个相关专项各一次

停止PIE，在Output Log依次执行，前一个完成后再执行下一个：

```text
Automation RunTests EmberTrial.UI.Settings.VideoDraft
```

```text
Automation RunTests EmberTrial.M3.Settings.VideoPreviewTransaction
```

各一次Success。复用已有规则测试，不新增镜像Widget测试或跑全37项。三个资产Valid负责结构，真实控件 / 关闭取消由下面实机补齐。

### 8.2 编辑器内的边界短查

常用地图Play → P：显示卡提示“请在独立游戏窗口中调整显示设置”，Combo / 预览禁用；原输入卡仍可用且1 / 不反转，遗物仍可浏览。P继续后移动转向正常 → Stop / Save All / 关闭编辑器。PIE不切真实显示属本章预期。

### 8.3 独立游戏启动

用当前新版源码的-game进程，不使用旧M2包。完整复制PowerShell（每条路径保留反斜杠）；日志和测试个人设置留本地：

```powershell
Set-Location 'E:\UE_Project\Demo'
$videoUiRuntime = 'E:\UE_Project\Demo\Artifacts\M3_05B3_02\Runtime'
New-Item -ItemType Directory -Path $videoUiRuntime -Force | Out-Null
$videoUiProfile = Join-Path $videoUiRuntime 'VideoUIUser.ini'
$videoUiLog = Join-Path $videoUiRuntime 'VideoUI01.log'
$videoUiArgs = @(
    '"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"'
    '/Game/EmberTrial/Maps/L_ET_MainMenu'
    '-game'
    '-windowed'
    '-ResX=1280'
    '-ResY=720'
    ('-GameUserSettingsINI="{0}"' -f $videoUiProfile)
    ('-abslog="{0}"' -f $videoUiLog)
)
Start-Process -FilePath 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $videoUiArgs -WindowStyle Normal
```

### 8.4 主菜单草稿、预览与确认

1. 首屏只一份显示卡，当前尺寸来自实际窗口。选择目录中另一尺寸：窗口不应立刻变化，预览启用；取消修改回当前实际值。不要固定要求1600×900，按实际目录选。
2. 鼠标选择另一尺寸 → 预览：窗口变化，编辑禁用，卡内出现保留 / 取消和约15秒倒计时。点取消预览：窗口实际恢复，草稿刷新。再预览并保持不确认，自动恢复且提示超时；不必掐秒。
3. 再预览，实际成功后点“保留设置”：保持新尺寸、提示请求保存，预览 / 取消修改禁用，确认排折叠，不能重复保存。配置文件与长日志由助手读取，用户不用抄Serial。
4. Tab进模式框，方向键 / Enter选择；到尺寸框操作后Tab到预览，Enter或Space触发。预览后初始焦点在取消预览，Shift+Tab可到保留设置；不确认时Esc取消。在下拉框展开时Esc只关闭下拉框，再按Esc才丢弃草稿 / 取消预览。Enter / Space不能意外开始游戏。

### 8.5 Pause、离开页面与新View

1. 开始试炼 → P：新卡读到菜单确认后的实际值，旧生命 / 体力 / 次数 / 冷却HUD与输入卡 / 遗物正常。选择另一尺寸预览，不确认且保持P暂停约20秒；窗口应按真实时间回退，World仍暂停，卡提示超时。
2. 再预览，直接按P继续：页面关闭且窗口恢复；移动转向正常，无卡键。重新P打开，新卡是实际快照，没有旧确认按钮 / 倒计时。用鼠标重复一次“预览→继续游戏”；不要搭临时函数持有旧View。
3. P页预览中返回菜单：未确认候选恢复，菜单新卡读实际值；菜单再预览并开始新试炼，同样取消恢复。长日志检查View cancel与对应Core恢复，旧View不再更新 / 保存。
4. 无边框选择后，分辨率框应只显示桌面并禁用；预览 → 取消恢复窗口。独占全屏若在实际目录中可选，额外做一次预览 → 取消并记录实际结果；若目录没有该模式就如实记不可用。跨显示器 / HDR / 刷新率不在本批。
5. 最终保持一个实际可选窗口尺寸，记录具体值，不强求1280×720。正常退出独立游戏；重开编辑器回常用地图、Save All。测试个人ini保持Artifacts，不拷入共享Config；输入偏好保持1 / 不反转。

### 8.6 2026-11-05 助手实测记录

用户授权助手操作8.4 / 8.5；独立进程使用`Artifacts/M3_05B3_02/Runtime/VideoUI01.log`及同目录`VideoUIUser.ini`。游戏最初处于试炼中，先通过暂停页返回主菜单后测试。以下按实际观察记载，未覆盖的内容不计为通过。

| 检查 | 实际结果 |
| --- | --- |
| 主菜单草稿 | 1280×720窗口选择1536×864只改变草稿；取消修改后回1280×720，窗口未变化 |
| 鼠标取消 / 超时 | Serial 1手动取消恢复1280×720，Result=3；Serial 2不确认，超时恢复1280×720，Result=4，界面显示超时提示 |
| 确认 / 配置 | Serial 3确认1536×864，Result=2，日志一条SaveRequested；确认按钮折叠，专用ini中请求和确认尺寸均为1536×864。没有单独再次调用确认按钮验证重复提交 |
| Pause超时 | Serial 4预览1365×767，世界保持Paused=1，约15秒后恢复1536×864 / Result=4，暂停页显示超时提示 |
| 关闭暂停页 | Serial 5按P、Serial 6鼠标点继续，均取消并恢复1536×864 / Result=3；重新打开的显示卡显示实际值，没有旧确认排 |
| 双向切图 | Serial 7暂停页预览中返回菜单、Serial 10菜单预览中开始试炼，均记录View cancel并恢复1536×864 / Result=3；未单独持有旧View验证后续事件 |
| 显示模式 | Serial 8无边框实际2048×1152 / Mode1，取消恢复1536×864；Serial 9独占全屏实际1024×768 / Mode2，取消后日志恢复1536×864窗口，均Result=3 |
| 键盘局部 | 展开的分辨率列表按Esc可关闭；展开后方向键选择、Enter确认选项可用；Serial 11预览中Esc取消并恢复，游戏仍暂停。Tab完整遍历、Shift+Tab焦点顺序、Space与不误触开始尚未完整验证 |
| 最终状态 | 输入卡显示1.00 / 不反转；正常退出日志为19:23:24。专用ini最终为1536×864窗口。移动键曾短按，但未取得可判断位移 / 鼠标转向的充分证据；编辑器未在本次测试后重开 |

**发现两处待处理的界面问题：**在本次主菜单窗口中，标题右侧与输入设置卡的文字区域发生重叠；模式 / 分辨率下拉列表的未选项为深色文字叠深色背景，可读性不足。已测试的窗口回退链路正常，但这两处布局 / 样式以及上表未覆盖项目尚不能标为整章通过。

### 8.7 两处界面问题的手动修复（待用户执行）

以下保留修复方案。助手检查发现标题和两个下拉框的目标属性已设置；视频设置控件原先未保存，现已编译并保存，主菜单也已编译通过。视觉窄复验见8.7.3；首次启动显示卡不可用是另行发现的待排查问题，不能因此宣称本章已全部通过。保留第8.6节原始测试记录。

#### 8.7.1 主菜单标题越过左列，覆盖输入卡

目标是让标题完整显示在原宽480的左列内，必要时自动换行；右列仍宽320。不要通过把右侧输入卡拖走或裁掉标题来掩盖重叠。

1. 打开`/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`，进入**Designer**。清空Details右上角搜索框，展开Hierarchy：`MainMenuColumns → MainMenuActionsSize → VerticalBox_Menu`。标题通常名为**Text_Title**；若你手动改过这个非绑定控件的名字，按其Content → Text为“EmberTrial / 灰烬试炼”识别，选中的是这个**Text Block**，不是整个VerticalBox或右侧设置卡。
2. 选Text_Title，在Details中设置下表。需要搜索属性时一次只搜一个名称，改完清空搜索再找下一项。

| 位置 / 属性 | 最终值 |
| --- | --- |
| Slot (Vertical Box Slot) → Size | Auto |
| 同一Slot → Horizontal Alignment | Fill |
| 同一Slot → Padding | Left / Top / Right / Bottom均0 |
| Appearance → Font → Size | **32**（原40调整为32；字体资源沿用现有中文字体） |
| Justification | Center |
| Wrapping → Auto Wrap Text | **True** |
| Wrapping → Wrap Text At | **480** |
| Min Desired Width（若有显示） | 0 |

3. Content → Text保持原完整标题，不要求插入手工换行；不要给标题固定Height。Wrap Text At以Widget的布局单位计算，不随游戏窗口像素手动改为1280或1536。若标题在本机字体下仍需两行，让Auto高度自然增加，下面副标题和按钮会随VerticalBox向下排列。
4. 核对上级容器，防止文字已经换行但左列仍被错误挤压：MainMenuActionsSize的Width Override勾选=**480**、Height Override不勾；其Horizontal Box Slot为**Auto**、Vertical Alignment=Center。VerticalBox_Menu在Size Box Slot为Fill / Fill、Padding0。MainMenuSettingsSize的Width Override=**320**、Height Override不勾，其Horizontal Box Slot为Auto、Padding Left=**32**、其余0。
5. MainMenuScale保持Scale To Fit / Down Only，原全屏Border中的Slot为Fill / Fill、Padding四向24。不要改标题Render Transform来挪位置；若你曾为重叠临时加过标题平移或缩放，将该标题的Translation归0、Scale归1后再看正常布局。原三个按钮、StatusText与六项绑定名称均保持。
6. Compile → Save。Designer可先用1280×720预览：标题应完整处于左列，即使换成两行，也不能压到右侧输入卡的“输入设置”文字上。最终以独立游戏画面为准。

#### 8.7.2 下拉列表未选文字太暗

修复应改**共享资产内部的两个ComboBoxString**，主菜单和Pause随后都会使用同一份样式。折叠框当前是浅底黑字，可以保留；展开列表的深底需要亮色行文字。两种状态由不同的前景样式控制。

1. 打开`/Game/EmberTrial/ui/Settings/WBP_ETVideoSettings`的Designer，先在Hierarchy选**ModeCombo**外层。清空Details搜索，展开**Style → Item Style**；也可在Details搜索`Text Color`，找到路径属于**Item Style**的两个颜色项。
2. 将下表两项都改为**指定颜色**。这是Slate Color，若颜色项使用了“Use Foreground / 继承前景”等规则，先在该项的颜色规则菜单切换到**Specify Color / 指定颜色**，再点颜色块输入RGBA；不能只改继承状态下未启用的颜色数值。

| Item Style内的属性 | 指定RGBA |
| --- | --- |
| Text Color（未选行文字） | **R=1，G=1，B=1，A=1** |
| Selected Text Color（选中行文字） | **R=1，G=1，B=1，A=1** |

3. 选**ResolutionCombo**，重复上一步两项设置。两个Combo都必须修改；只改ModeCombo不会修复分辨率列表。若属性面板搜不到，清空搜索并手动展开Style / Item Style及其Advanced，不去修改Widget Blueprint的Class Defaults。
4. 保留现有深色弹出列表背景和蓝色选中行。**不要把两个Combo外层的Foreground Color一并改成白色**，否则折叠框浅色背景上的当前值可能变成白底白字；本次需要修改的是Item Style的行文字。也不要改根Widget的Color and Opacity、Render Opacity，或通过强行启用Disabled控件修颜色。
5. Font Size=16、原八项绑定、Focusable和选项生成方式保持。无需绑定On Generate Widget、手写条目Widget或把选项复制进Default Options；C++仍负责模式 / 尺寸数据。
6. Compile → Save。按本机UE5.5源码核对，默认Combo条目由TextBlock生成，其行前景来自`FTableRowStyle.TextColor / SelectedTextColor`；折叠内容则可继承Combo自身前景。因此这里改行样式即可针对性修复展开列表。Designer没有运行选项不代表失败，展开效果在独立游戏检查。

#### 8.7.3 保存、窄复验与反馈（约5分钟）

**本次助手检查：**Text_Title已为32字号、Auto / Fill、Padding 0、Auto Wrap开启且Wrap 480；两个Combo的Item Style普通行及选中行均为指定白色RGBA 1。两个Widget编译显示成功，视频设置Widget已保存；编辑器独立预览中标题未覆盖右侧设置卡，但该预览的显示设置被禁用，不能检查展开行。通过命令启动带专用ini的`-game`窗口未出现，尚未完成两个下拉框、第二窗口尺寸及Pause共享样式的实机验证；三资产Validate、Pause编译也尚未执行。已回常用地图并恢复Selected Viewport。第8.6节未覆盖的键盘与移动复验仍待进行。

**后续独立进程复验：**用户启动带专用ini的`-game`，实际日志是`Artifacts/M3_05B3_02/Runtime/VideoUI01.log`，本轮开始覆盖前已有的同名日志（旧日志自动备份）。主菜单1280×720及预览1707×960时标题完整且未压到右侧卡；取消后日志确认恢复1280×720。主菜单及Pause中两个下拉框的普通项和蓝色选中项均可读，折叠值为浅底深字；未单独检查悬停态。Pause原输入卡和遗物列表可见，最终从主菜单正常退出。未执行三资产Validate / Pause Compile，未追加8.6节未完成的键盘与移动检查。

**新增待排查：**本次独立进程首次进入主菜单时，显示卡显示“当前显示状态不可用 / 请在独立游戏窗口中调整显示设置”，选项禁用；进入试炼打开Pause后恢复，返回主菜单亦恢复，未等待验证首次页面是否自行恢复。日志命令行确有`-game`，因此不能简单归因于PIE。初步需要检查首次构造时窗口 / Viewport实际尺寸及`Capture`失败原因；当前尚无足够证据断定根因，也未修改源码。此问题修复及重启复验完成前，本章保持未验收。

1. 对修改过的**WBP_ETMainMenu、WBP_ETVideoSettings**分别Compile → Save → Validate Assets，应有本项目Valid。再对**WBP_PauseMenu**Compile / Validate，确认嵌套卡仍正常；只有它实际变为未保存时才Save，不为本次修复重新制作或改名Pause实例。
2. 用第8.3节的独立-game脚本启动，**仅将日志文件名VideoUI01.log改为VideoUI02.log**，保留同一个Artifacts专用个人ini。两处均是Designer修改，无需再跑C++ Build、两个专项、全回归或Cook。
3. 主菜单先看实际初始窗口：标题完整、不覆盖输入设置卡。展开ModeCombo，观察普通行、鼠标悬停行和选中行都能辨认；按Esc收起。展开ResolutionCombo同样检查。折叠后的当前值仍应清楚，不能变成白底白字。
4. 从实际目录选择另一个窗口尺寸，预览后观察同一主菜单布局；**点取消预览**恢复。只为查看另一尺寸下的标题 / 样式，不重复超时、保存、全屏或切图矩阵，也不强求某个固定尺寸必在目录中。
5. 开始试炼 → P，展开Pause显示卡的两个下拉框，确认共享样式也生效；原输入卡和遗物列表仍在。正常继续 / 返回菜单并退出。预览期间禁用控件的灰色文字属于既有状态表现，本次对比度验收针对**启用的可选行**，不要求把禁用态改成启用色。
6. 本节只关闭两处视觉问题；第8.6节尚未完整覆盖的Tab / Shift+Tab / Space与恢复移动 / 转向仍按原记录待复验，不因视觉修复成功而自动记通过。

执行后可反馈：

```text
M3-05B3-02 视觉补修
Text_Title字号32 / Fill / Auto Wrap / Wrap480；两窗口尺寸下无重叠：
两个Combo的Item Style白色行文字；展开 / 选中 / 折叠均可读：
Pause共享样式正常；Compile-Save / 本项目Valid：
本次实际修改的资产名称 / 失败或特别说明（无填无）：
完成时分 / 额外手动用时：
```

## 9. 定位与生命周期说明

- 新WBP八绑定 / 两宿主第六绑定都必须是正确外层类型、名称和Is Variable；不要用Optional掩盖缺失。
- 卡在PIE禁用是服务保护。独立-game仍禁用时保留日志和当前实际文字，先查父类、六份新旧源码与GetActualMode，而非移除保护。
- 计时来自Core事件，Widget Tick保持Never。确认重验本View的Serial和期限；恢复中不能新预览，恢复失败只允许显式取消重试，不假报恢复成功。
- NativeConstruct成对订阅 / NativeDestruct成对解绑；宿主NativeDestruct额外CloseView及时撤销，重复关闭无副作用。Host Pending也取消本次预览，失败恢复页面时可以重新编辑。
- 预览Begin的同步事件被暂时抑制，返回后统一读取；程序填充Combo时抑制回写。仅目录变化重建选项，倒计时不每秒重建列表，不扫描Actor / 读写ini。
- 本章静态API检查采用本机UE5.5 ComboBoxString / Widget头及B2实际接口，仍以用户Build与实机为验收证据。

## 10. 完成后反馈

```text
M3-05B3-02 完成
两新 / 四旧UI；Editor Build / Core83-UI35-Editor3索引 / Game Build：
新WBP八绑定 / 两宿主第六绑定、三Valid / Save：
VideoDraft / VideoPreviewTransaction各一次；PIE禁用与旧P短查：
菜单草稿无副作用 / 取消-超时实际恢复 / 确认一次与真实配置：
Tab-方向键-Enter-Space / 展开Esc / 预览Esc / 原开始焦点：
P暂停实时时限 / P与鼠标关闭取消 / 重开新View：
双向切图取消 / 无边框恢复 / 独占全屏实际结果：
最终实际窗口尺寸 / Runtime目录 / 正常退出：
原输入1不反转、Source与三资产保存；失败或布局问题（无填无）：
完成时分 / 手动代码 / Designer / 阅读 / 检查 / 构建启动等待：
```

长日志由助手读取，真实错误发送第一条error和附近10–20行；布局 / 焦点问题附窗口截图、完整Hierarchy与操作步骤。助手接手实机按当次授权执行。

## 11. Commit / Push（固定放在最后）

### 11.1 当前收尾文档

B3-01源码已在本地`3682aa6`。当前Stage 01验收报告 / 指南收尾、本指南与实际变化的Progress / Backlog / M3及M2 Roadmap / Architecture / AcceptanceWorkflow_Options / VersionControl / Introduction / README。02源码和资产尚未实施，不Stage忽略候选或空报告。

英文：`docs: verify display settings draft and prepare shared video settings UI`。

专业中文：`docs: 完成显示设置草稿验收并准备共享显示设置界面`。

### 11.2 本章实施后的源码与资产

第2节两新 / 四旧UI文件，加以下三个实际保存的资产：

```text
EmberTrial/Content/EmberTrial/ui/Settings/WBP_ETVideoSettings.uasset  新
EmberTrial/Content/EmberTrial/ui/Menu/WBP_ETMainMenu.uasset          旧
EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset           旧
```

英文：`feat: add shared display settings UI with preview confirmation and rollback`。

专业中文：`feat: 新增共享显示设置界面与预览确认回退`。

### 11.3 文档独立提交

本指南、实际生成的验收报告与同步状态 / 来源清单；来源SHA待真实资产保存后更新，不预填空行。英文：`docs: record shared video settings UI implementation and verification`；专业中文：`docs: 记录共享显示设置界面实现与验证结果`。源码 / 资产 / 文档合并提交可使用11.2的feat建议。

### 11.4 两处视觉补修的提交建议

**目前只补了方案**：本次Stage本指南。英文：`docs: add fixes for menu title overlap and combo list contrast`；专业中文：`docs: 补充主菜单标题重叠与下拉列表对比度修复步骤`。

**用户执行并复验后**：Stage实际修改的WBP_ETMainMenu / WBP_ETVideoSettings，以及确有保存变化的WBP_PauseMenu；再附实际更新的本指南 / 来源清单。英文：`fix: prevent menu title overlap and improve combo list contrast`；专业中文：`fix: 修复主菜单标题重叠并改善下拉列表对比度`。不把尚未执行的方案提交描述成已修复。

Runtime / VideoUIUser.ini、Artifacts候选、Saved / Intermediate / Binaries、.idea与生成索引留本地。Stage后再修改需重新Stage → Review → Commit / Push由用户操作。
