# M3-05A2：暂停页输入设置与草稿确认

2026-11-05收尾，UE **5.5.4**。**本章已验收通过，无待补。**[验收报告](Reports/M3_05A2_Verification.md)区分用户反馈时间与实际日志日期，记录双Build / 两Success、两Valid和键鼠闭环。当前进入[A3主菜单复用](M3_05A3_MainMenuInputSettings.md)，以下保留实施代码与Designer步骤供复查，不要求重做。

## 1. 本章体验与边界

P或祭坛E打开既有页面，左列原菜单下出现“输入设置”卡：灵敏度滑条、反转鼠标Y轴、应用 / 取消修改 / 默认值与状态提示。右列遗物列表 / 确认与原继续 / 返回主菜单保持，使用同一个Flow与世界暂停。输入卡始终显示，不新增设置页面路由或第二份暂停状态。

| 操作 | 本章行为 |
| --- | --- |
| 拖动滑条 / 勾反转 | 只改变草稿；运行输入和磁盘不变 |
| 应用 | 确认当前基线未被其他入口修改，应用合法值并请求本地保存；页面保持打开 |
| 取消修改 / 输入卡内Esc | 重新读取当前已应用设置；不关闭整个暂停页、不写盘 |
| 默认值 | 仅把输入默认1 / 不反转填入草稿，仍需应用；不重置画质 / 窗口 |
| P / 继续 / 离开页面 | 未应用草稿丢弃；已应用偏好保持 |
| 外部入口改过设置 | 旧草稿确认拒绝并刷新，避免覆盖外部新值 |

`FETInputSettingsDraft`只保存值；View负责控件与确认、复用A1接口。原Pause向输入卡透传Flow已有busy，Flow源码不改，菜单 / 身体 / 暂停仍由它拥有。确认与焦点恢复还检查当前卡或其UserWidget祖先仍在Viewport，避免页面移除后的迟到输入；不把嵌套卡自身IsInViewport通常为False误判为未显示。无Tick、属性Binding、ASC观察、新Action或临时验收蓝图。主菜单单独打开设置的入口、显示 / 画质 / 分辨率确认回退、音量实际路由留后续批次；这不是完整M3设置菜单。

实施前预算代码60–90 / Designer20–35 / 阅读配置5–10 / 检查5–10。实际代码60 / Designer30 / 阅读5 / 检查5，共100分钟、等待5；当前201 Source / 113cpp（Core78 / UI32 / Editor3）、34自动化、300 Content / 来源81。M3累计1105 / 等待73，项目4516 / 明确等待397。

## 2. 准备与准确路径

Save All、停止PIE并关闭UE，再在CLion创建普通源码文件。以下五份全部属于**EmberTrialUI**；Draft是普通C++值类，**不是UObject**，不使用UE向导生成UCLASS，也没有generated.h。Widget头才有UCLASS与generated.h；不要让两个同名类留在不同目录。

仓库根目录为`E:\UE_Project\Demo`，完整层级：

```text
EmberTrial/Source/EmberTrialUI/Public/Settings/ETInputSettingsDraft.h          新
EmberTrial/Source/EmberTrialUI/Private/Settings/ETInputSettingsDraft.cpp       新
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETInputSettingsWidget.h         新
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETInputSettingsWidget.cpp       新
EmberTrial/Source/EmberTrialUI/Tests/ETInputSettingsDraftTests.cpp             新
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h                 旧
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp               旧
```

UI模块已有Core / UMG公开依赖，不改Build.cs、Core Source、Config或FailureFlow。尚未补第五绑定前，旧Pause可能显示required binding消息；先做完第6节再Play，不改成Optional避开装配。

## 3. 五份新文件：各自完整粘贴

### 3.1 Public/Settings/ETInputSettingsDraft.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Settings/ETGameUserSettings.h"

/** 页面草稿；只保存值副本，不持有引擎设置、Pawn或Widget，也不负责写盘。 */
class EMBERTRIALUI_API FETInputSettingsDraft
{
public:
    bool Open(const FETInputPreferences& Applied);
    void Close();

    bool SetSensitivity(float NewValue);
    bool SetInvertY(bool bValue);
    void UseDefaults();

    bool IsOpen() const { return bOpen; }
    bool IsDirty() const;
    bool IsDefault() const;
    const FETInputPreferences& GetValue() const { return Value; }

    // 确认时重验当前已应用值；其他入口改过设置就拒绝旧草稿，调用者重新读取。
    bool TryMakeApply(const FETInputPreferences& Current, FETInputPreferences& Out) const;

private:
    static bool Same(const FETInputPreferences& A, const FETInputPreferences& B);

    FETInputPreferences Baseline;
    FETInputPreferences Value;
    bool bOpen = false;
};
```

### 3.2 Private/Settings/ETInputSettingsDraft.cpp

```cpp
#include "Settings/ETInputSettingsDraft.h"

// ---- 打开 / 关闭拥有草稿；不会应用设置或保存文件 ----

bool FETInputSettingsDraft::Open(const FETInputPreferences& Applied)
{
    Close();
    if (!Applied.IsValid())
    {
        return false;
    }
    Baseline = Value = Applied;
    bOpen = true;
    return true;
}

void FETInputSettingsDraft::Close()
{
    bOpen = false;
    Baseline = Value = FETInputPreferences{};
}

// ---- 本地编辑；拒绝非法值时保留原草稿 ----

bool FETInputSettingsDraft::SetSensitivity(const float NewValue)
{
    FETInputPreferences Next = Value;
    Next.MouseSensitivity = NewValue;
    if (!bOpen || !Next.IsValid())
    {
        return false;
    }
    Value = Next;
    return true;
}

bool FETInputSettingsDraft::SetInvertY(const bool bValue)
{
    if (!bOpen)
    {
        return false;
    }
    Value.bInvertMouseY = bValue;
    return true;
}

void FETInputSettingsDraft::UseDefaults()
{
    if (bOpen)
    {
        Value = FETInputPreferences{};
    }
}

// ---- 状态与最终候选；浮点微差不产生虚假的未应用提示 ----

bool FETInputSettingsDraft::Same(const FETInputPreferences& A, const FETInputPreferences& B)
{
    return A.IsValid() && B.IsValid() &&
        FMath::IsNearlyEqual(A.MouseSensitivity, B.MouseSensitivity, KINDA_SMALL_NUMBER) &&
        A.bInvertMouseY == B.bInvertMouseY;
}

bool FETInputSettingsDraft::IsDirty() const
{
    return bOpen && !Same(Value, Baseline);
}

bool FETInputSettingsDraft::IsDefault() const
{
    return bOpen && Same(Value, FETInputPreferences{});
}

bool FETInputSettingsDraft::TryMakeApply(
    const FETInputPreferences& Current, FETInputPreferences& Out) const
{
    Out = FETInputPreferences{};
    if (!bOpen || !Value.IsValid() || !Same(Current, Baseline))
    {
        return false;
    }
    Out = Value;
    return true;
}
```

### 3.3 Public/Widgets/ETInputSettingsWidget.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/ETInputSettingsDraft.h"
#include "ETInputSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class USlider;
class UTextBlock;

/** 现有Pause中的输入设置卡；不拥有世界暂停 / 输入模式，不另建Flow。 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETInputSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void SetHostBusy(bool bBusy);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(
        const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    bool CanEdit() const;
    bool HasLiveHost() const;
    void ReadCurrent(const FText& Message);
    void Render();
    void FocusSlider();

    UFUNCTION()
    void HandleSensitivityChanged(float Value);

    UFUNCTION()
    void HandleInvertChanged(bool bChecked);

    UFUNCTION()
    void HandleApply();

    UFUNCTION()
    void HandleRevert();

    UFUNCTION()
    void HandleDefaults();

    // ---- 以下七个名称 / 类型是Designer装配契约 ----
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USlider> SensitivitySlider;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SensitivityValueText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UCheckBox> InvertYCheckBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ApplyButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RevertButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> DefaultsButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SettingsStatusText;

    TWeakObjectPtr<UETGameUserSettings> Settings;
    FETInputSettingsDraft Draft;
    FText Feedback;
    bool bViewActive = false;
    bool bSyncingControls = false;
    bool bHostBusy = false;
};
```

### 3.4 Private/Widgets/ETInputSettingsWidget.cpp

```cpp
#include "Widgets/ETInputSettingsWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogETInputSettingsView, Log, All);

// ---- 初始化只绑定一次；重新Construct只取得新快照，不累计按钮回调 ----

bool UETInputSettingsWidget::HasRequiredLayout() const
{
    return IsValid(SensitivitySlider.Get()) && IsValid(SensitivityValueText.Get()) &&
        IsValid(InvertYCheckBox.Get()) && IsValid(ApplyButton.Get()) &&
        IsValid(RevertButton.Get()) && IsValid(DefaultsButton.Get()) &&
        IsValid(SettingsStatusText.Get());
}

void UETInputSettingsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    SensitivitySlider->OnValueChanged.AddUniqueDynamic(
        this, &ThisClass::HandleSensitivityChanged);
    InvertYCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &ThisClass::HandleInvertChanged);
    ApplyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleApply);
    RevertButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRevert);
    DefaultsButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDefaults);
}

void UETInputSettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsDesignTime() || !HasRequiredLayout())
    {
        return;
    }
    bViewActive = true;
    Settings = UETGameUserSettings::GetETGameUserSettings();
    SensitivitySlider->SetMinValue(FETInputPreferences::MinSensitivity);
    SensitivitySlider->SetMaxValue(FETInputPreferences::MaxSensitivity);
    SensitivitySlider->SetStepSize(0.05f);
    ReadCurrent(FText::FromString(TEXT("修改后选择应用；P继续会丢弃未应用的修改。")));
}

void UETInputSettingsWidget::NativeDestruct()
{
    // 旧View即使仍被持有，也不能再确认；没有未应用草稿的自动保存。
    bViewActive = false;
    Draft.Close();
    Settings.Reset();
    Feedback = FText::GetEmpty();
    Super::NativeDestruct();
}

// ---- 草稿呈现：没有Binding / Tick，程序同步控件不能回写一遍 ----

bool UETInputSettingsWidget::CanEdit() const
{
    return bViewActive && !bHostBusy && Settings.IsValid() && Draft.IsOpen();
}

bool UETInputSettingsWidget::HasLiveHost() const
{
    // 嵌套卡自身通常不直接加入Viewport，要沿UserWidget外层找实际屏幕所有者。
    for (const UUserWidget* View = this; View; View = View->GetTypedOuter<UUserWidget>())
    {
        if (View->IsInViewport())
        {
            return true;
        }
    }
    return false;
}

void UETInputSettingsWidget::SetHostBusy(const bool bBusy)
{
    bHostBusy = bBusy;
    Render();
}

void UETInputSettingsWidget::ReadCurrent(const FText& Message)
{
    const UETGameUserSettings* Current = Settings.Get();
    const bool bReady = Current && Draft.Open(Current->GetInputPreferences());
    if (!bReady)
    {
        Draft.Close();
    }
    Feedback = bReady ? Message : FText::FromString(TEXT("输入设置暂不可用。"));
    Render();
}

void UETInputSettingsWidget::Render()
{
    if (!bViewActive || !HasRequiredLayout())
    {
        return;
    }
    const TGuardValue<bool> SyncGuard(bSyncingControls, true);
    const FETInputPreferences Value = Draft.GetValue();
    SensitivitySlider->SetValue(Value.MouseSensitivity);
    InvertYCheckBox->SetIsChecked(Value.bInvertMouseY);

    FNumberFormattingOptions Number;
    Number.SetMinimumFractionalDigits(2);
    Number.SetMaximumFractionalDigits(2);
    SensitivityValueText->SetText(FText::Format(
        NSLOCTEXT("ETInputSettings", "Sensitivity", "镜头灵敏度：{0}倍"),
        FText::AsNumber(Value.MouseSensitivity, &Number)));
    SettingsStatusText->SetText(Feedback);

    const bool bEditable = CanEdit();
    SensitivitySlider->SetIsEnabled(bEditable);
    InvertYCheckBox->SetIsEnabled(bEditable);
    ApplyButton->SetIsEnabled(bEditable && Draft.IsDirty());
    RevertButton->SetIsEnabled(bEditable && Draft.IsDirty());
    DefaultsButton->SetIsEnabled(bEditable && !Draft.IsDefault());
}

void UETInputSettingsWidget::FocusSlider()
{
    if (CanEdit() && HasLiveHost() && IsValid(SensitivitySlider.Get()) && IsValid(GetOwningPlayer()))
    {
        // 刚确认 / 取消的按钮可能变成Disabled，显式保留一个有效键盘焦点。
        SensitivitySlider->SetUserFocus(GetOwningPlayer());
    }
}

void UETInputSettingsWidget::HandleSensitivityChanged(const float Value)
{
    if (!bSyncingControls && CanEdit() && Draft.SetSensitivity(Value))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

void UETInputSettingsWidget::HandleInvertChanged(const bool bChecked)
{
    if (!bSyncingControls && CanEdit() && Draft.SetInvertY(bChecked))
    {
        Feedback = FText::FromString(TEXT("有未应用的修改。"));
        Render();
    }
}

// ---- 确认才应用 / 保存；取消与默认值只影响本卡草稿 ----

void UETInputSettingsWidget::HandleApply()
{
    if (!CanEdit() || !HasLiveHost() || !Draft.IsDirty())
    {
        return;
    }
    UETGameUserSettings* Current = Settings.Get();
    FETInputPreferences Candidate;
    if (!Draft.TryMakeApply(Current->GetInputPreferences(), Candidate))
    {
        ReadCurrent(FText::FromString(TEXT("设置已由其他入口更改，已刷新，请重新编辑。")));
        FocusSlider();
        return;
    }
    if (!Current->ApplyInputPreferences(Candidate))
    {
        Feedback = FText::FromString(TEXT("未能应用，请取消修改后重试。"));
        Render();
        return;
    }

    const TStrongObjectPtr<UETInputSettingsWidget> KeepAlive(this);
    Current->SaveSettings();
    UE_LOG(LogETInputSettingsView, Display,
        TEXT("[M3-05A2] Applied Sensitivity=%.2f InvertY=%d SaveRequested=1"),
        Candidate.MouseSensitivity, Candidate.bInvertMouseY ? 1 : 0);
    // 保存接口未来可能触发同步外部回调；已移除的View不再更新 / 抢焦点。
    if (bViewActive && HasLiveHost() && Settings.Get() == Current)
    {
        ReadCurrent(FText::FromString(TEXT("已应用；继续游戏后使用新设置。")));
        FocusSlider();
    }
}

void UETInputSettingsWidget::HandleRevert()
{
    if (CanEdit())
    {
        ReadCurrent(FText::FromString(TEXT("已取消修改，保留当前已应用设置。")));
        FocusSlider();
    }
}

void UETInputSettingsWidget::HandleDefaults()
{
    if (CanEdit())
    {
        Draft.UseDefaults();
        Feedback = FText::FromString(TEXT("默认值已填入草稿，选择应用后才生效。"));
        Render();
        FocusSlider();
    }
}

FReply UETInputSettingsWidget::NativeOnPreviewKeyDown(
    const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        HandleRevert();
        return FReply::Handled();
    }
    // P继续冒泡给原Pause；Enter / Space由当前焦点控件处理，不在这里执行继续。
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
```

### 3.5 Tests/ETInputSettingsDraftTests.cpp

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETInputSettingsDraft.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETInputSettingsDraftTest,
    "EmberTrial.UI.Settings.InputDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETInputSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETInputSettingsDraft Draft;
    FETInputPreferences Candidate;
    const FETInputPreferences Defaults;
    TestFalse(TEXT("Closed draft cannot edit"), Draft.SetSensitivity(0.5f));
    TestFalse(TEXT("Closed draft cannot submit"), Draft.TryMakeApply(Defaults, Candidate));

    TestTrue(TEXT("Open from applied snapshot"), Draft.Open(Defaults));
    TestFalse(TEXT("New snapshot has no pending change"), Draft.IsDirty());
    TestTrue(TEXT("Initial defaults recognised"), Draft.IsDefault());
    TestTrue(TEXT("Edit sensitivity"), Draft.SetSensitivity(0.5f));
    TestTrue(TEXT("Edit inversion"), Draft.SetInvertY(true));
    TestTrue(TEXT("Editing creates pending changes"), Draft.IsDirty());
    TestEqual(TEXT("Source value was not changed by editing"), Defaults.MouseSensitivity, 1.0f);
    TestTrue(TEXT("Same applied baseline yields candidate"), Draft.TryMakeApply(Defaults, Candidate));
    TestEqual(TEXT("Candidate keeps chosen sensitivity"), Candidate.MouseSensitivity, 0.5f);
    TestTrue(TEXT("Candidate keeps chosen inversion"), Candidate.bInvertMouseY);

    // 候选只是一份值；测试不调用全局Apply / Save，不改本机个人偏好。
    const FETInputPreferences Committed = Candidate;
    TestTrue(TEXT("Successful application can become the new baseline"), Draft.Open(Committed));
    TestFalse(TEXT("New applied snapshot is clean"), Draft.IsDirty());
    Draft.UseDefaults();
    TestTrue(TEXT("Defaults only changes the draft"), Draft.IsDirty() && Draft.IsDefault());
    TestEqual(TEXT("Previous applied value stays unchanged"), Committed.MouseSensitivity, 0.5f);
    TestTrue(TEXT("Cancel rereads current applied state"), Draft.Open(Committed));
    TestTrue(TEXT("Cancel preserves inversion"), Draft.GetValue().bInvertMouseY);
    TestFalse(TEXT("Cancel removes dirty state"), Draft.IsDirty());

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    for (const float Bad : {0.0f, 3.01f, NaN})
    {
        TestFalse(TEXT("Invalid draft edit rejected"), Draft.SetSensitivity(Bad));
        TestEqual(TEXT("Rejected edit retains old draft"), Draft.GetValue().MouseSensitivity, 0.5f);
    }
    TestTrue(TEXT("Valid edit before external change"), Draft.SetSensitivity(0.75f));
    FETInputPreferences External = Committed;
    External.MouseSensitivity = 1.5f;
    Candidate = Committed;
    TestFalse(TEXT("External update rejects a stale confirmation"), Draft.TryMakeApply(External, Candidate));
    TestEqual(TEXT("Rejected candidate is cleared"), Candidate.MouseSensitivity, 1.0f);
    TestFalse(TEXT("Rejected candidate clears inversion"), Candidate.bInvertMouseY);
    TestTrue(TEXT("Reload external state"), Draft.Open(External));
    TestFalse(TEXT("Reload has no pending old values"), Draft.IsDirty());

    Draft.Close();
    TestFalse(TEXT("Close invalidates even a held draft"), Draft.IsOpen());
    TestFalse(TEXT("Close rejects inversion edits"), Draft.SetInvertY(false));
    Draft.UseDefaults();
    TestFalse(TEXT("Defaults cannot reopen closed draft"), Draft.TryMakeApply(External, Candidate));
    FETInputPreferences Corrupt;
    Corrupt.MouseSensitivity = NaN;
    TestFalse(TEXT("Invalid baseline is unavailable"), Draft.Open(Corrupt));
    TestFalse(TEXT("Invalid open leaves draft closed"), Draft.IsOpen());
    TestTrue(TEXT("Next page can open cleanly"), Draft.Open(Defaults));
    TestFalse(TEXT("Next page has no old pending change"), Draft.IsDirty());
    return true;
}
#endif
```


## 4. 两份旧Pause文件：五处准确修改

每处只用原文定位并替换一次；没有列出的函数 / 字段保持，尤其原RelicPanel命令桥与P释放逻辑。

### 4.1 Public/Widgets/ETPauseWidget.h

在前置声明组增加输入卡类型。

查找原文：

```cpp
class UETRelicPanelWidget;
```

替换为：

```cpp
class UETRelicPanelWidget;
class UETInputSettingsWidget;
```

### 4.2 Public/Widgets/ETPauseWidget.h

在原RelicPanel字段之后增加第五个必需绑定；原四绑定保留。

查找原文：

```cpp
    TObjectPtr<UETRelicPanelWidget> RelicPanel;
```

替换为：

```cpp
    TObjectPtr<UETRelicPanelWidget> RelicPanel;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETInputSettingsWidget> InputSettingsPanel;
```

### 4.3 Private/Widgets/ETPauseWidget.cpp

在include组增加新卡头文件。

查找原文：

```cpp
#include "Widgets/ETRelicPanelWidget.h"
```

替换为：

```cpp
#include "Widgets/ETRelicPanelWidget.h"
#include "Widgets/ETInputSettingsWidget.h"
```

### 4.4 Private/Widgets/ETPauseWidget.cpp

只替换HasRequiredLayout内的return，加入第五绑定并合理换行。

查找原文：

```cpp
    return IsValid(BTN_Resume.Get()) && IsValid(MainMenuButton.Get()) && IsValid(StatusText.Get()) && IsValid(RelicPanel.Get());
```

替换为：

```cpp
    return IsValid(BTN_Resume.Get()) && IsValid(MainMenuButton.Get()) &&
        IsValid(StatusText.Get()) && IsValid(RelicPanel.Get()) && IsValid(InputSettingsPanel.Get());
```

### 4.5 Private/Widgets/ETPauseWidget.cpp

在ApplyState里把同一Pending状态传给输入卡；不改原RelicPanel与Flow。

查找原文：

```cpp
    if (RelicPanel)
    {
        RelicPanel->SetCommandBusy(bBusy);
    }
    if (HasRequiredLayout())
```

替换为：

```cpp
    if (RelicPanel)
    {
        RelicPanel->SetCommandBusy(bBusy);
    }
    if (InputSettingsPanel)
    {
        InputSettingsPanel->SetHostBusy(bBusy);
    }
    if (HasRequiredLayout())
```


## 5. 正常Build与三模块索引

UE保持关闭、源码保存。PowerShell先执行Editor构建 / 索引：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后再做Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion按提示Reload CMake，预计Core78 / UI32 / Editor3；确认新文件属于**EmberTrialUICodeModel**，`Components/Slider.h`与`Settings/ETGameUserSettings.h`能导航。不要用CMake索引目标代替UBT构建。遇错贴第一条error及前后10–20行；exit code 6是汇总。

重开UE，先完成以下Designer，再Play。旧Pause此时缺InputSettingsPanel属尚未装配，别删除原资产或Reparent旧Pause。

## 6. 新输入设置卡：先造完整控件树，再Reparent

### 6.1 创建资产和根容器

1. Content Browser进入`/Game/EmberTrial/ui`，创建新文件夹**Settings**；在里面新建User Interface → Widget Blueprint，选择UserWidget，命名**WBP_ETInputSettings**。这是新资产，不复制Pause或RelicPanel。
2. 打开Designer。若有默认空Canvas Panel，删除该空根；从Palette拖**Size Box**成为根，命名**InputSettingsSize**。Width / Height Override都**不勾**，不写固定宽高。预览区域暂时保持默认；嵌入原Pause后再按左列实际320宽度观察换行。
3. SizeBox唯一子项放**Border**，命名**InputSettingsBorder**。`Content → Padding`四方向均**12**，Brush Color填R=.05 / G=.05 / B=.05 / A=1；Content Color and Opacity白色。`Slot (Size Box Slot)`对齐Fill / Fill、Padding全0，别把12同时填两处。
4. Border唯一子项放**Vertical Box**，命名**InputSettingsContent**。下面所有主控件按下列顺序成为它的直接子级：

```text
InputSettingsSize (SizeBox，无固定宽高)
  InputSettingsBorder (Border，Content Padding12)
    InputSettingsContent (VerticalBox)
      InputSettingsTitle (Text)
      SensitivityValueText (Text)             必需
      SensitivitySlider (Slider)              必需
      InvertYCheckBox (CheckBox)               必需
        InvertYLabel (Text)
      InputSettingsButtons (HorizontalBox)
        ApplyButton (Button)                  必需
          ApplyLabel (Text)
        RevertButton (Button)                 必需
          RevertLabel (Text)
        DefaultsButton (Button)               必需
          DefaultsLabel (Text)
      SettingsStatusText (Text)               必需
```

CheckBox / Button / Border各只接一个直接子项，三个按钮是HorizontalBox的兄弟，不把下一个按钮放进上一个按钮。**七个“必需”外层控件**名称和类型必须完全一致，并勾Is Variable；其他布局 / 内层Label不参与绑定，Is Variable可不勾。

### 6.2 标题、数值和滑条

1. 选InputSettingsTitle：Text填“输入设置”，沿用当前支持中文的字体 / 字体回退，Font Size=**20**。Vertical Box Slot：Size=Auto、Horizontal Alignment=Fill、Padding四向0。
2. 选SensitivityValueText：Text填“镜头灵敏度：1.00倍”，Font=**16**，Auto Wrap Text=True；Vertical Box Slot Size=Auto / Horizontal Fill、Padding Top=**8**、其余0。Text旁不创建Bind，运行由C++更新。
3. 选**Slider外层SensitivitySlider**，不是Text：Appearance中Min Value=**0.25**、Max Value=**3.0**、Value=**1.0**、Step Size=**0.05**。Value旁不要绑定。
4. Slider的Interaction → **Is Focusable=True**。Appearance展开Advanced（也可Details搜索完整英文属性）：**Mouse Uses Step=True、Requires Controller Lock=False、Locked=False**。这使Tab聚焦后直接左右方向键调整，不要求先Enter进入控制器捕获。UE5.5这些选项是Designer属性，不要编写不存在的SetRequiresControllerLock接口。
5. Slider的Vertical Box Slot：Size=Auto、Horizontal Fill、Padding Top=**4**、Bottom=**4**、Left / Right=0。保持原生滑条高度，不设整个页面的Fill1高度。

### 6.3 反转勾选和三个按钮

1. 选**InvertYCheckBox外层**：Checked State=Unchecked，不创建Binding；Is Focusable=True。Interaction Advanced中的Click Method / Press Method保持**Down and Up**。Vertical Box Slot Size=Auto / Horizontal Fill，Padding Top=**4**、其余0。
2. 在CheckBox内部放**唯一一个Text Block**，命名InvertYLabel，Text“反转鼠标 Y 轴”，Font=**16**，保留原生文字间距。此处是普通Panel Slot，没有可填写Padding的专用Check Box Slot。名字在Hierarchy，显示文字在Content → Text。
3. 拖Horizontal Box作为CheckBox下方兄弟，命名InputSettingsButtons。Vertical Box Slot Size=Auto / Horizontal Fill，Padding Top=**10**、其余0。放三个Button作为其兄弟子级，名称依次**ApplyButton / RevertButton / DefaultsButton**。
4. 三个外层Button：Horizontal Box Slot均**Size=Fill，系数1**，Vertical Fill；Apply右Padding=**4**，Revert左右Padding各**2**，Defaults左Padding=**4**，其余0。Is Focusable=True；Click Method / Press Method=Down and Up。不点OnClicked旁的“+”，原生已绑定。
5. 每个Button只放一个Text Block，名ApplyLabel / RevertLabel / DefaultsLabel，文字依次**应用 / 取消修改 / 默认值**，Font=**14**、Justification=Center，沿用中文字库；Button Slot Padding Left / Right=**4**、Top / Bottom=**8**，对齐Center / Center。这是按钮内容内边距，不是外层Horizontal Slot间距。
6. 最下方独立Text Block命名**SettingsStatusText**，勾Is Variable，Text填“修改后选择应用；P继续会丢弃未应用的修改。”、Font=**14**，Auto Wrap Text=True；Vertical Box Slot Size=Auto / Horizontal Fill，Padding Top=**8**、其余0。不要固定单行高度，不创建Text Binding。
7. 三按钮在Designer可保持Enabled；运行时无草稿的应用 / 取消会自动变灰。图里变灰不等于Binding失效；不要把C++设置的状态再用蓝图Tick覆盖。

### 6.4 Reparent与本项目校验

1. 先逐个核对七绑定：Slider、Text、CheckBox、三个Button、Text；内层Label不代替外层Button / CheckBox。
2. Class Settings → Reparent Blueprint → **ETInputSettingsWidget**，原生路径应为`/Script/EmberTrialUI.ETInputSettingsWidget`。
3. Class Defaults搜索Tick，Tick Frequency=**Never**；新UserWidget自身Is Focusable=**False**（内部Slider / CheckBox / Button仍True）。Graph不接任何Event Construct / 按键 / OnClicked，Designer属性无Bind。
4. Compile → Save。无required widget binding / Native Tick警告后，在Content Browser右键资产 → Asset Actions → Validate Assets；应有本项目Valid。通用校验器已经覆盖此目录 / 必需绑定，不新增Editor源码。

## 7. 原Pause嵌入：第五绑定与既有布局

1. 打开`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`的Designer；**保留原ETPauseWidget父类**，不Reparent、复制或删除Graph。
2. Hierarchy找**PauseActionsSize**（SizeBox，Width Override=320），展开其唯一子项——原VerticalBox。这个VerticalBox已有原标题 / BTN_Resume / MainMenuButton / StatusText。选中这整个VerticalBox，在其**最后、StatusText下方**添加新WBP_ETInputSettings实例，不能放进StatusText或Button内部。
3. Palette搜索**WBP_ETInputSettings**，从User Created拖入上述VerticalBox。将**外层实例名**改为**InputSettingsPanel**，勾Is Variable。不要改新资产名称，也不要把里面InputSettingsContent重命名为InputSettingsPanel。
4. 选该实例：Vertical Box Slot **Size=Auto、Horizontal Alignment=Fill**、Padding Top=**12**，Left / Right / Bottom=0；Visibility=Visible / Is Enabled=True。不要设Fill1、额外固定高度或把它拖到右侧RelicPanel内部。
5. 原PauseActionsSize Width=320、Height Override不勾；原PauseScale保持Scale To Fit / Down Only。右列**RelicPanel**与原三个按钮 / 文本、全屏背景和24外边距保持。新卡根据宽度自动换行；若小视口缩放，由原ScaleBox整体处理。
6. 最终结构应为：

```text
原Canvas / 全屏Border
  PauseScale (ScaleBox)
    PauseColumns (HorizontalBox)
      PauseActionsSize (SizeBox Width320)
        原VerticalBox
          原标题 / BTN_Resume / MainMenuButton / StatusText
          InputSettingsPanel (WBP_ETInputSettings，新增)
      RelicPanel (WBP_ETRelicPanel，旧右列)
```

7. 原Pause保持自身Is Focusable=True / Tick Never。新卡自身不额外占Tab停点；内部Slider / CheckBox / 三按钮Focusable，默认Navigation规则保持Escape / 自动，不写蓝图键盘业务。Tab应能走旧按钮 → 输入卡的可用控件 → 右侧遗物；Disabled按钮会被跳过，初始焦点仍是原“继续游戏”。
8. Compile → Save，确认**五个绑定**（BTN_Resume / MainMenuButton / StatusText / RelicPanel / InputSettingsPanel），无缺失信息；对Pause也执行Validate Assets看到本项目Valid，再Save All。无需更改Controller或UIBootstrap配置。

## 8. 分层检查：两专项与一组真实UI（约5–10分钟）

### 8.1 两项相关自动化

停止PIE，从Output Log的Cmd分别执行，前一项完成后再执行后一项：

```text
Automation RunTests EmberTrial.UI.Settings.InputDraft
```

```text
Automation RunTests EmberTrial.M3.Settings.InputPreferences
```

各一次Success。草稿测试不创建UMG / 实际写盘，不把纯值夹具当键鼠或磁盘验证；A1的实际重启读取已验收，这批**不再重做编辑器重启 / 全34项 / 新Cook**。Camera构图未改，不重复CameraRules。

### 8.2 草稿、取消与默认的实际语义

1. 正式地图Play，P打开。输入卡应仅一份，初始1.00 / 未勾，应用与取消变灰；旧右列、继续和返回菜单仍在，没有裁掉卡片 / 中文文字。
2. 拖到约0.50（无需掐确切像素），勾反转。提示有未应用修改，按钮可用。**不点应用，P继续**：自由镜头手感仍原1 / 不反转。再P打开，应回1.00 / 未勾，说明关闭丢弃草稿。
3. 再改约0.50 / 反转，点**应用**；页面保持，提示已应用、应用 / 取消变灰。P继续后鼠标变慢且上下反转。再P打开仍显示已应用值；这一步证明真实应用，保存沿用A1接口。
4. 点**默认值**，草稿显示1.00 / 未勾，**先点取消修改**，应回约0.50 / 勾选；不应真的恢复默认。再次默认值 → **应用**，最后回到**1.00 / 不反转**。这是本批要求的最终配置，个人ini不Stage。

### 8.3 键盘焦点与最短集成

1. P打开，按Tab进入SensitivitySlider，左右键改值；Tab到CheckBox，Space切换；Tab到可用应用，用Enter确认一次。确认后焦点回滑条，继续Tab仍有效；再试取消修改用Space，避免因为按钮Disabled而丢失焦点。
2. 在输入卡内部改草稿后Esc，应只取消本卡修改，整个暂停页保持；P才继续。鼠标继续一次，确认无需重新点击视口即可移动 / 转向，没有补执行攻击 / 跳跃。回卡用默认值 → 应用恢复1 / 不反转。
3. 祭坛旁E打开原页，输入卡也出现；右列选择 / 比较、原确认与卸装仍按C2工作，实际配装沿当前状态即可，不重复完整装卸矩阵。普通P仍只读遗物，输入偏好允许修改是另一项职责。
4. 最后新卡保留一项未应用草稿，Stop再Play / P，应只有一份卡、显示已应用1 / 不反转，没有旧草稿。左键 / 2 / E与移动短查，Save All。无需搭临时蓝图或测精确时间点。

## 9. 常见定位、性能与后续

- 必需控件缺失：核对新卡七个外层名字 / 类型 / Is Variable，原Pause的InputSettingsPanel必须是新WBP实例；不要设BindWidgetOptional、错把普通VerticalBox命名为该字段。
- 滑条鼠标可拖、方向键不动：选Slider外层查Is Focusable=True / Requires Controller Lock=False；不要改整个Pause的Input Mode。
- 点击应用没有效果：先看是否有未应用修改，Disabled是正常；若提示其他入口修改，说明旧草稿被拒绝，刷新后再编辑。设置值来自同一个引擎实例，不创建第二个NewObject设置来“修复”。
- 布局裁切：查PauseActionsSize Height Override是否误勾、输入卡实例是否误设Fill1 / 固定高度、StatusText自动换行与ScaleBox原配置；先贴完整Hierarchy和实际窗口截图，不删除原RelicPanel。
- 每次Construct读一次偏好；滑条事件只改值和控件，明确应用才SaveSettings。无每帧查配置、扫描身体、重复Timer或UI→ASC写入。

API按本机5.5的`UMG/Public/Components/Slider.h`、`CheckBox.h`与`Slate/Private/Widgets/Input/SSlider.cpp`核对；[官方5.5 CheckBox API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/Components/UCheckBox?application_version=5.5)列出状态变更委托、焦点与按压配置。具体键鼠 / 字体和布局以本章实机证据为准，不把静态代码审查写成已运行成功。之后再扩主菜单入口与显示 / 音量，沿同一偏好与草稿边界。

## 10. 完成后反馈

```text
M3-05A2 完成
五新UI / 两旧Pause；Editor Build / Core78-UI32-Editor3索引 / Game Build：
新WBP七绑定 / 无Bind / Never；Pause第五绑定与两Valid / Save：
InputDraft / InputPreferences各一次：
未应用P丢弃 / 应用实际改变 / 取消恢复已应用 / 默认为草稿：
Tab左右键 / Space勾选 / Enter-Space按钮 / Esc仅取消 / P继续焦点：
祭坛原确认与普通P只读 / Stop无旧草稿 / 最短旧功能检查：
最终1.00 / 不反转，Source / 两资产Save All；布局或失败问题（无填无）：
完成时分 / 手动代码 / Designer / 阅读配置 / 检查 / 构建等待：
```

无需抄长日志，我读取记录；C++错误发文字与第一条error，控件 / 焦点问题配完整截图和复现步骤，复杂情况再按当次授权直接排查。

## 11. Commit / Push（固定放在最后）

A2五新UI / 两旧Pause与两资产已在本地`df3a1cd`，审查起点工作区干净。当前Stage A2验收报告 / 指南收尾、来源清单、A3新指南与同步状态文档；A3正式Source / 资产尚未实施，不Stage忽略Artifacts内的候选。未变化源码 / 资产不重复Stage；未联网核验远端。

- 英文建议：`docs: verify pause input settings and prepare main menu reuse`
- 中文译文：`docs: 完成暂停输入设置验收并准备主菜单复用`

Stage / Commit / Push由用户操作；个人GameUserSettings.ini、Raw / 候选 / 日志 / 缓存 / 包留本地。
