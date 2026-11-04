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