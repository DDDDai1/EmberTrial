#include "Widgets/ETMainMenuWidget.h"
#include "Widgets/ETInputSettingsWidget.h"
#include "Widgets/ETVideoSettingsWidget.h"
#include "Widgets/ETGraphicsSettingsWidget.h"
#include "Widgets/ETAudioSettingsWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMainMenuView, Log, All);

// ---- 必需控件与一次初始化；控件由Widget树拥有 ----

bool UETMainMenuWidget::HasRequiredLayout() const
{
    return IsValid(StartButton.Get()) && IsValid(ContinueButton.Get()) &&
        IsValid(QuitButton.Get()) && IsValid(StatusText.Get()) && IsValid(InputSettingsPanel.Get()) &&
        IsValid(VideoSettingsPanel.Get()) && IsValid(GraphicsSettingsPanel.Get()) &&
        IsValid(AudioSettingsPanel.Get());
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

void UETMainMenuWidget::NativeDestruct()
{
    if (AudioSettingsPanel)
    {
        AudioSettingsPanel->CloseView();
    }
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->CloseView();
    }
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->CloseView();
    }
    Super::NativeDestruct();
}

void UETMainMenuWidget::ShowReady(bool bCanContinue)
{
    SetNewRunConfirmation(false);
    bContinueAvailable = bCanContinue;
    ApplyState(false,
               FText::FromString(bCanContinue ? TEXT("可继续本次试炼。")
                                              : TEXT("没有可继续的试炼。")));
}

void UETMainMenuWidget::ShowPending(const FText& Message)
{
    SetNewRunConfirmation(false);
    ApplyState(true, Message);
}

void UETMainMenuWidget::ShowError(const FText& Message, bool bCanContinue)
{
    SetNewRunConfirmation(false);
    bContinueAvailable = bCanContinue;
    ApplyState(false, Message);
}

void UETMainMenuWidget::ShowNewRunConfirmation()
{
    SetNewRunConfirmation(true);
    ApplyState(false, NSLOCTEXT("ETSave", "ConfirmNewTrial",
        "开始新试炼将覆盖当前进度。选择「确认新试炼」或「取消」。"));
    if (APlayerController* Controller = GetOwningPlayer())
    {
        ContinueButton->SetUserFocus(Controller);
    }
}

void UETMainMenuWidget::SetNewRunConfirmation(bool bConfirm)
{
    if (bConfirmingNewRun == bConfirm)
    {
        return;
    }
    bConfirmingNewRun = bConfirm;
    if (UTextBlock* Label = Cast<UTextBlock>(StartButton->GetContent()))
    {
        if (bConfirm)
        {
            StartLabel = Label->GetText();
        }
        Label->SetText(bConfirm ? NSLOCTEXT("ETSave", "ConfirmNewTrialButton", "确认新试炼") : StartLabel);
    }
    if (UTextBlock* Label = Cast<UTextBlock>(ContinueButton->GetContent()))
    {
        if (bConfirm)
        {
            ContinueLabel = Label->GetText();
        }
        Label->SetText(bConfirm ? NSLOCTEXT("ETSave", "CancelNewTrialButton", "取消") : ContinueLabel);
    }
}

void UETMainMenuWidget::ApplyState(bool bPending, const FText& Message)
{
    // 先保存状态，即使布局不完整，后续意图也不能绕过pending。
    bBusy = bPending;
    if (InputSettingsPanel)
    {
        InputSettingsPanel->SetHostBusy(bBusy);
    }
    if (AudioSettingsPanel)
    {
        AudioSettingsPanel->SetHostBusy(bBusy);
    }
    if (GraphicsSettingsPanel)
    {
        GraphicsSettingsPanel->SetHostBusy(bBusy);
    }
    if (VideoSettingsPanel)
    {
        VideoSettingsPanel->SetHostBusy(bBusy);
    }
    if (!HasRequiredLayout())
    {
        return;
    }
    StartButton->SetIsEnabled(!bBusy);
    ContinueButton->SetIsEnabled(!bBusy && (bContinueAvailable || bConfirmingNewRun));
    QuitButton->SetIsEnabled(!bBusy && !bConfirmingNewRun);
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
    if (!bBusy && (bContinueAvailable || bConfirmingNewRun))
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
