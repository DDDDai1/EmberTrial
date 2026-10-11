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
            TEXT("[M1-06C] Missing menu BindWidget Widget=%s"),
            *GetNameSafe(this));
        return;
    }
    StartButton->OnClicked.AddUniqueDynamic(
        this, &ThisClass::HandleStartClicked);
    ContinueButton->OnClicked.AddUniqueDynamic(
        this, &ThisClass::HandleContinueClicked);
    QuitButton->OnClicked.AddUniqueDynamic(
        this, &ThisClass::HandleQuitClicked);
}

void UETMainMenuWidget::ShowReady(bool bCanContinue)
{
    bContinueAvailable = bCanContinue;
    ApplyState(false, FText::FromString(bCanContinue
        ? TEXT("可继续本次试炼；关闭游戏后会话检查点清除。")
        : TEXT("尚无本次试炼；开始后可在当前进程内继续。")));
}

void UETMainMenuWidget::ShowPending(const FText& Message)
{
    ApplyState(true, Message);
}

void UETMainMenuWidget::ShowError(const FText& Message, bool bCanContinue)
{
    bContinueAvailable = bCanContinue;
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
    ContinueButton->SetIsEnabled(!bBusy && bContinueAvailable);
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

void UETMainMenuWidget::HandleContinueClicked()
{
    if (!bBusy && bContinueAvailable)
    {
        OnContinueRequested.Broadcast();
    }
}

void UETMainMenuWidget::HandleQuitClicked()
{
    if (!bBusy)
    {
        OnQuitRequested.Broadcast();
    }
}