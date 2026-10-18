#include "Widgets/ETPauseWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

// ---- 必需控件与一次初始化；Widget树拥有按钮 ----

bool UETPauseWidget::HasRequiredLayout() const
{
    return IsValid(BTN_Resume.Get()) && IsValid(MainMenuButton.Get()) && IsValid(StatusText.Get());
}

void UETPauseWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    BTN_Resume->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleResumeClicked);
    MainMenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMainMenuClicked);
}

// ---- 状态呈现；无Tick / 属性Binding，也不修改世界 ----

void UETPauseWidget::ShowReady()
{
    ApplyState(false, FText::FromString(TEXT("松开 P 或选择继续游戏返回。")));
}

void UETPauseWidget::ShowPending(const FText& Message)
{
    ApplyState(true, Message);
}

void UETPauseWidget::ShowError(const FText& Message)
{
    ApplyState(false, Message);
}

void UETPauseWidget::ApplyState(bool bPending, const FText& Message)
{
    // 先写本地状态，布局无效时也不能绕过pending继续发请求。
    bBusy = bPending;
    if (HasRequiredLayout())
    {
        BTN_Resume->SetIsEnabled(!bBusy);
        MainMenuButton->SetIsEnabled(!bBusy);
        StatusText->SetText(Message);
    }
}

// ---- 焦点与按钮意图；聚焦不等于执行按钮 ----

void UETPauseWidget::FocusResume()
{
    APlayerController* Controller = GetOwningPlayer();
    if (!bBusy && IsValid(BTN_Resume.Get()) && IsValid(Controller))
    {
        BTN_Resume->SetUserFocus(Controller);
    }
}

void UETPauseWidget::HandleResumeClicked()
{
    if (!bBusy)
    {
        OnResumeIntent.Broadcast();
    }
}

void UETPauseWidget::HandleMainMenuClicked()
{
    if (!bBusy)
    {
        OnMainMenuIntent.Broadcast();
    }
}

// ---- UIOnly输入适配：P释放被消费；busy时也不向游戏层泄漏 ----

FReply UETPauseWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::P)
    {
        HandleResumeClicked();
        return FReply::Handled();
    }
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

FReply UETPauseWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                               const FPointerEvent& InMouseEvent)
{
    if (!bBusy && IsValid(BTN_Resume.Get()) &&
        InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        // 背景左键只将焦点还给继续按钮，不自动继续；按钮自身处理自己的点击。
        return FReply::Handled().SetUserFocus(BTN_Resume->TakeWidget());
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
