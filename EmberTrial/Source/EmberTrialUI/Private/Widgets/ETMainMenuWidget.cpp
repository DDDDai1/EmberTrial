#include "Widgets/ETMainMenuWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMainMenuView, Log, All);

// ---- 必需控件与一次初始化；控件由Widget树拥有 ----

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
        UE_LOG(LogETMainMenuView, Warning, TEXT("[M1-06C] Missing menu BindWidget Widget=%s"),
               *GetNameSafe(this));
        return;
    }
    StartButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStartClicked);
    ContinueButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleContinueClicked);
    QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
}

// ---- Ready / Pending / Error的呈现；没有Tick或属性Binding ----

void UETMainMenuWidget::ShowReady(bool bCanContinue)
{
    bContinueAvailable = bCanContinue;
    ApplyState(false,
               FText::FromString(bCanContinue ? TEXT("可继续本次试炼；关闭游戏后会话检查点清除。")
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
    // 先保存状态，即使布局不完整，后续意图也不能绕过pending。
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

// ---- 焦点与按钮意图；焦点设置本身不执行开始 / 继续 ----

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
