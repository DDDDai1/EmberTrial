#include "Widgets/ETMainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMainMenuView, Log, All);

bool UETMainMenuWidget::HasRequiredLayout() const
{
    return IsValid(StartButton.Get()) && IsValid(ContinueButton.Get()) &&
        IsValid(QuitButton.Get()) && IsValid(StatusText.Get());
}

void UETMainMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        UE_LOG(LogETMainMenuView, Warning,
            TEXT("[M1-06A] Missing menu BindWidget Widget=%s"),
            *GetNameSafe(this));
        return;
    }
    StartButton->OnClicked.AddUniqueDynamic(
        this, &UETMainMenuWidget::HandleStartClicked);
    QuitButton->OnClicked.AddUniqueDynamic(
        this, &UETMainMenuWidget::HandleQuitClicked);
}

void UETMainMenuWidget::ShowReady()
{
    ApplyState(false, FText::FromString(
        TEXT("尚无存档，继续功能暂不可用。")));
}

void UETMainMenuWidget::ShowPending(const FText& Message)
{
    ApplyState(true, Message);
}

void UETMainMenuWidget::ShowError(const FText& Message)
{
    ApplyState(false, Message);
}

void UETMainMenuWidget::ApplyState(bool bPending, const FText& Message)
{
    bBusy = bPending;
    if (!HasRequiredLayout())
    {
        return;
    }
    StartButton->SetIsEnabled(!bBusy);
    ContinueButton->SetIsEnabled(false);
    QuitButton->SetIsEnabled(!bBusy);
    StatusText->SetText(Message);
}

void UETMainMenuWidget::FocusStartButton()
{
    APlayerController* Controller = GetOwningPlayer();
    if (!bBusy && IsValid(StartButton.Get()) && IsValid(Controller))
    {
        StartButton->SetUserFocus(Controller);
    }
}

void UETMainMenuWidget::HandleStartClicked()
{
    if (!bBusy)
    {
        OnStartRequested.Broadcast();
    }
}

void UETMainMenuWidget::HandleQuitClicked()
{
    if (!bBusy)
    {
        OnQuitRequested.Broadcast();
    }
}