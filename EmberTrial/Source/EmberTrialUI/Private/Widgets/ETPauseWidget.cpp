#include "Widgets/ETPauseWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"

bool UETPauseWidget::HasRequiredLayout() const
{
    return IsValid(BTN_Resume.Get()) && IsValid(MainMenuButton.Get()) &&
        IsValid(StatusText.Get());
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
    bBusy = bPending;
    if (HasRequiredLayout())
    {
        BTN_Resume->SetIsEnabled(!bBusy);
        MainMenuButton->SetIsEnabled(!bBusy);
        StatusText->SetText(Message);
    }
}

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

FReply UETPauseWidget::NativeOnKeyUp(const FGeometry& InGeometry,
    const FKeyEvent& InKeyEvent)
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
        // Only background clicks bubble here; buttons handle their own clicks.
        return FReply::Handled().SetUserFocus(BTN_Resume->TakeWidget());
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}