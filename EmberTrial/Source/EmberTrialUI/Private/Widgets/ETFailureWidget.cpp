#include "Widgets/ETFailureWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

bool UETFailureWidget::HasRequiredLayout() const
{
    return IsValid(RetryButton.Get()) && IsValid(MainMenuButton.Get()) &&
        IsValid(StatusText.Get());
}

void UETFailureWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    RetryButton->OnClicked.AddUniqueDynamic(
        this, &ThisClass::HandleRetryClicked);
    MainMenuButton->OnClicked.AddUniqueDynamic(
        this, &ThisClass::HandleMainMenuClicked);
}

void UETFailureWidget::ShowReady()
{
    ApplyState(false, FText::FromString(TEXT("重新试炼将从入口开始。")));
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
    bBusy = bPending;
    if (HasRequiredLayout())
    {
        RetryButton->SetIsEnabled(!bBusy);
        MainMenuButton->SetIsEnabled(!bBusy);
        StatusText->SetText(Message);
    }
}

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