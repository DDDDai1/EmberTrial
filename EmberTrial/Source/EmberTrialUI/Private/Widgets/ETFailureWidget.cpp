#include "Widgets/ETFailureWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

// ---- 必需控件与一次初始化；Widget树拥有按钮 ----

bool UETFailureWidget::HasRequiredLayout() const
{
    return IsValid(RetryButton.Get()) && IsValid(MainMenuButton.Get()) && IsValid(StatusText.Get());
}

void UETFailureWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    RetryButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRetryClicked);
    MainMenuButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleMainMenuClicked);
}

// ---- 状态呈现；无Tick / 属性Binding，也不修改世界 ----

void UETFailureWidget::ShowReady()
{
    ApplyState(false,
               FText::FromString(TEXT("重新试炼将从最近检查点开始；尚未激活检查点时从入口开始。")));
}

void UETFailureWidget::ShowPending(const FText& Message)
{
    ApplyState(true, Message);
}

void UETFailureWidget::ShowError(const FText& Message)
{
    ApplyState(false, Message);
}

void UETFailureWidget::ApplyState(bool bPending, const FText& Message)
{
    // 先写本地状态，布局无效时也不能绕过pending继续发请求。
    bBusy = bPending;
    if (HasRequiredLayout())
    {
        RetryButton->SetIsEnabled(!bBusy);
        MainMenuButton->SetIsEnabled(!bBusy);
        StatusText->SetText(Message);
    }
}

// ---- 焦点与按钮意图；聚焦不等于执行按钮 ----

void UETFailureWidget::FocusRetryButton()
{
    APlayerController* Controller = GetOwningPlayer();
    if (!bBusy && IsValid(RetryButton.Get()) && IsValid(Controller))
    {
        RetryButton->SetUserFocus(Controller);
    }
}

void UETFailureWidget::HandleRetryClicked()
{
    if (!bBusy)
    {
        OnRetryRequested.Broadcast();
    }
}

void UETFailureWidget::HandleMainMenuClicked()
{
    if (!bBusy)
    {
        OnMainMenuRequested.Broadcast();
    }
}
