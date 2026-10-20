#include "Widgets/ETHealingHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Internationalization/Text.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHealingHUD, Log, All);
#define LOCTEXT_NAMESPACE "ETHealingHUD"

// ---- Construct只重放快照，订阅归StartObserving所有 ----
void UETHealingHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (IsDesignTime())
    {
        return;
    }
    const UETHealingPresenter* Presenter = BoundPresenter.Get();
    ApplyViewData(Presenter ? Presenter->GetSnapshot() : FETHealingViewData{},
                  TEXT("ConstructSnapshot"));
}

void UETHealingHUDWidget::NativeDestruct()
{
    StopObserving();
    Super::NativeDestruct();
}

void UETHealingHUDWidget::StartObserving(UETHealingPresenter* Presenter)
{
    StopObserving();
    if (IsDesignTime())
    {
        return;
    }
    if (!IsValid(Presenter))
    {
        ApplyViewData(FETHealingViewData{}, TEXT("MissingPresenter"));
        return;
    }
    BoundPresenter = Presenter;
    bObserving = true;
    Presenter->OnViewDataChanged.AddUniqueDynamic(this,
                                                  &UETHealingHUDWidget::HandleViewDataChanged);
    UE_LOG(LogETHealingHUD, Display, TEXT("[M2-04C] View subscribed Widget=%s Presenter=%s"),
           *GetNameSafe(this), *GetNameSafe(Presenter));
    ApplyViewData(Presenter->GetSnapshot(), TEXT("SubscribeSnapshot"));
}

void UETHealingHUDWidget::StopObserving()
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    if (UETHealingPresenter* Presenter = BoundPresenter.Get())
    {
        Presenter->OnViewDataChanged.RemoveDynamic(this,
                                                   &UETHealingHUDWidget::HandleViewDataChanged);
    }
    BoundPresenter.Reset();
    if (bWasObserving)
    {
        UE_LOG(LogETHealingHUD, Display, TEXT("[M2-04C] View unsubscribed Widget=%s"),
               *GetNameSafe(this));
    }
}

void UETHealingHUDWidget::HandleViewDataChanged(const FETHealingViewData& Data)
{
    if (bObserving && BoundPresenter.IsValid())
    {
        ApplyViewData(Data, TEXT("ViewDataChanged"));
    }
}

// ---- 只读展示；次数耗尽与资源不可用有不同文案 ----
void UETHealingHUDWidget::ApplyViewData(const FETHealingViewData& Data, const TCHAR* Reason)
{
    if (!HealingStatusText || !HealingValuesText || !HealingContent)
    {
        UE_LOG(LogETHealingHUD, Warning, TEXT("[M2-04C] Missing required BindWidget Widget=%s"),
               *GetNameSafe(this));
        return;
    }
    if (Data.State == EETHealingViewState::Ready)
    {
        HealingStatusText->SetText(Data.Charges == 0 ? LOCTEXT("EmptyTitle", "治疗次数已用尽（Q）")
                                                     : LOCTEXT("HealingTitle", "治疗次数（Q）"));
        HealingContent->SetVisibility(ESlateVisibility::HitTestInvisible);
        HealingValuesText->SetText(FText::Format(LOCTEXT("HealingValues", "{0} / {1}"),
                                                 FText::AsNumber(Data.Charges),
                                                 FText::AsNumber(Data.MaxCharges)));
    }
    else
    {
        HealingContent->SetVisibility(ESlateVisibility::Collapsed);
        HealingValuesText->SetText(FText::GetEmpty());
        switch (Data.State)
        {
        case EETHealingViewState::NoPawn:
            HealingStatusText->SetText(LOCTEXT("NoPawn", "暂无受控角色"));
            break;
        case EETHealingViewState::UnsupportedPawn:
            HealingStatusText->SetText(LOCTEXT("UnsupportedPawn", "当前角色不提供治疗次数"));
            break;
        case EETHealingViewState::WaitingForGameplayReady:
            HealingStatusText->SetText(LOCTEXT("Waiting", "角色准备中"));
            break;
        default:
            HealingStatusText->SetText(LOCTEXT("Unavailable", "治疗次数暂不可用"));
            break;
        }
    }
    UE_LOG(LogETHealingHUD, Verbose, TEXT("[M2-04C] View Widget=%s Charges=%d/%d Reason=%s"),
           *GetNameSafe(this), Data.Charges, Data.MaxCharges, Reason);
}

#undef LOCTEXT_NAMESPACE