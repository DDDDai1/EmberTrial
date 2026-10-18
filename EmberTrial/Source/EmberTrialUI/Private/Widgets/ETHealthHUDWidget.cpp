#include "Widgets/ETHealthHUDWidget.h"
#include "Widgets/ETResourceHUDText.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Internationalization/Text.h"
#include "Logging/LogMacros.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHealthHUD, Log, All);
#define LOCTEXT_NAMESPACE "ETHealthHUD"

// ---- 视图生命周期与订阅 ----

void UETHealthHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (IsDesignTime())
    {
        return;
    }

    // Construct may run more than once; subscription belongs to StartObserving.
    const UETHealthPresenter* Presenter = BoundPresenter.Get();
    ApplyViewData(Presenter ? Presenter->GetSnapshot() : FETHealthViewData{},
                  TEXT("ConstructSnapshot"));
}

void UETHealthHUDWidget::NativeDestruct()
{
    StopObserving();
    Super::NativeDestruct();
}

void UETHealthHUDWidget::StartObserving(UETHealthPresenter* Presenter)
{
    StopObserving();
    if (IsDesignTime())
    {
        return;
    }

    if (!IsValid(Presenter))
    {
        ApplyViewData(FETHealthViewData{}, TEXT("MissingPresenter"));
        return;
    }

    BoundPresenter = Presenter;
    bObserving = true;
    Presenter->OnViewDataChanged.AddUniqueDynamic(this, &UETHealthHUDWidget::HandleViewDataChanged);

    UE_LOG(LogETHealthHUD, Display, TEXT("[M1-03B2] View subscribed Widget=%s Presenter=%s"),
           *GetNameSafe(this), *GetNameSafe(Presenter));

    // Bind first, then get current data: a late-joining View needs no new effect.
    ApplyViewData(Presenter->GetSnapshot(), TEXT("SubscribeSnapshot"));
}

void UETHealthHUDWidget::StopObserving()
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    UETHealthPresenter* Presenter = BoundPresenter.Get();
    if (Presenter)
    {
        Presenter->OnViewDataChanged.RemoveDynamic(this,
                                                   &UETHealthHUDWidget::HandleViewDataChanged);
    }
    BoundPresenter.Reset();

    if (bWasObserving)
    {
        UE_LOG(LogETHealthHUD, Display, TEXT("[M1-03B2] View unsubscribed Widget=%s"),
               *GetNameSafe(this));
    }
}

void UETHealthHUDWidget::HandleViewDataChanged(const FETHealthViewData& Data)
{
    if (!bObserving || !BoundPresenter.IsValid())
    {
        return;
    }
    ApplyViewData(Data, TEXT("ViewDataChanged"));
}

// ---- 事件快照渲染；只共享数字文本策略，状态标题与比例仍由各面板负责 ----

void UETHealthHUDWidget::ApplyViewData(const FETHealthViewData& Data, const TCHAR* Reason)
{
    if (!HealthStatusText || !HealthValuesText || !HealthBar || !HealthContent)
    {
        UE_LOG(LogETHealthHUD, Warning, TEXT("[M1-03B2] Missing required BindWidget Widget=%s"),
               *GetNameSafe(this));
        return;
    }

    if (Data.State == EETHealthViewState::Ready)
    {
        HealthStatusText->SetText(LOCTEXT("HealthTitle", "生命值"));
        HealthContent->SetVisibility(ESlateVisibility::HitTestInvisible);
        HealthValuesText->SetText(ETResourceHUDText::FormatValues(
            LOCTEXT("HealthValues", "{0} / {1}"), Data.Health, Data.MaxHealth));
        HealthBar->SetPercent(Data.HealthFraction);
    }
    else
    {
        HealthContent->SetVisibility(ESlateVisibility::Collapsed);
        HealthValuesText->SetText(FText::GetEmpty());
        HealthBar->SetPercent(0.0f);
        switch (Data.State)
        {
        case EETHealthViewState::NoPawn:
            HealthStatusText->SetText(LOCTEXT("NoPawn", "暂无受控角色"));
            break;
        case EETHealthViewState::UnsupportedPawn:
            HealthStatusText->SetText(LOCTEXT("UnsupportedPawn", "当前角色不提供生命信息"));
            break;
        case EETHealthViewState::WaitingForGameplayReady:
            HealthStatusText->SetText(LOCTEXT("Waiting", "角色准备中"));
            break;
        default:
            HealthStatusText->SetText(LOCTEXT("Unavailable", "生命信息暂不可用"));
            break;
        }
    }

    const FString StateName =
        StaticEnum<EETHealthViewState>()->GetNameStringByValue(static_cast<int64>(Data.State));
    UE_LOG(
        LogETHealthHUD, Display,
        TEXT(
            "[M1-03B2] View Widget=%s State=%s Health=%.1f MaxHealth=%.1f Fraction=%.3f Reason=%s"),
        *GetNameSafe(this), *StateName, Data.Health, Data.MaxHealth, Data.HealthFraction, Reason);
}

#undef LOCTEXT_NAMESPACE