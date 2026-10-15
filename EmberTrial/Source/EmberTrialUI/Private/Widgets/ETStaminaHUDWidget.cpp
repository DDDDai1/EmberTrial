#include "Widgets/ETStaminaHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Internationalization/Text.h"
#include "Logging/LogMacros.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStaminaHUD, Log, All);
#define LOCTEXT_NAMESPACE "ETStaminaHUD"

void UETStaminaHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (IsDesignTime())
    {
        return;
    }

    // Construct may run more than once; subscription belongs to StartObserving.
    const UETStaminaPresenter* Presenter = BoundPresenter.Get();
    ApplyViewData(Presenter ? Presenter->GetSnapshot() : FETStaminaViewData{},
        TEXT("ConstructSnapshot"));
}

void UETStaminaHUDWidget::NativeDestruct()
{
    StopObserving();
    Super::NativeDestruct();
}

void UETStaminaHUDWidget::StartObserving(UETStaminaPresenter* Presenter)
{
    StopObserving();
    if (IsDesignTime())
    {
        return;
    }

    if (!IsValid(Presenter))
    {
        ApplyViewData(FETStaminaViewData{}, TEXT("MissingPresenter"));
        return;
    }

    BoundPresenter = Presenter;
    bObserving = true;
    Presenter->OnViewDataChanged.AddUniqueDynamic(
        this, &UETStaminaHUDWidget::HandleViewDataChanged);

    UE_LOG(LogETStaminaHUD, Display,
        TEXT("[M2-01B2] View subscribed Widget=%s Presenter=%s"),
        *GetNameSafe(this), *GetNameSafe(Presenter));

    // Bind first, then get current data: a late-joining View needs no new effect.
    ApplyViewData(Presenter->GetSnapshot(), TEXT("SubscribeSnapshot"));
}

void UETStaminaHUDWidget::StopObserving()
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    UETStaminaPresenter* Presenter = BoundPresenter.Get();
    if (Presenter)
    {
        Presenter->OnViewDataChanged.RemoveDynamic(
            this, &UETStaminaHUDWidget::HandleViewDataChanged);
    }
    BoundPresenter.Reset();

    if (bWasObserving)
    {
        UE_LOG(LogETStaminaHUD, Display,
            TEXT("[M2-01B2] View unsubscribed Widget=%s"), *GetNameSafe(this));
    }
}

void UETStaminaHUDWidget::HandleViewDataChanged(const FETStaminaViewData& Data)
{
    if (!bObserving || !BoundPresenter.IsValid())
    {
        return;
    }
    ApplyViewData(Data, TEXT("ViewDataChanged"));
}

void UETStaminaHUDWidget::ApplyViewData(
    const FETStaminaViewData& Data, const TCHAR* Reason)
{
    if (!StaminaStatusText || !StaminaValuesText || !StaminaBar || !StaminaContent)
    {
        UE_LOG(LogETStaminaHUD, Warning,
            TEXT("[M2-01B2] Missing required BindWidget Widget=%s"),
            *GetNameSafe(this));
        return;
    }

    if (Data.State == EETStaminaViewState::Ready)
    {
        StaminaStatusText->SetText(LOCTEXT("StaminaTitle", "体力值"));
        StaminaContent->SetVisibility(ESlateVisibility::HitTestInvisible);
        FNumberFormattingOptions NumberOptions;
        NumberOptions.SetMinimumFractionalDigits(0);
        NumberOptions.SetMaximumFractionalDigits(0);
        StaminaValuesText->SetText(FText::Format(
            LOCTEXT("StaminaValues", "{0} / {1}"),
            FText::AsNumber(Data.Stamina, &NumberOptions),
            FText::AsNumber(Data.MaxStamina, &NumberOptions)));
        StaminaBar->SetPercent(Data.StaminaFraction);
    }
    else
    {
        StaminaContent->SetVisibility(ESlateVisibility::Collapsed);
        StaminaValuesText->SetText(FText::GetEmpty());
        StaminaBar->SetPercent(0.0f);
        switch (Data.State)
        {
        case EETStaminaViewState::NoPawn:
            StaminaStatusText->SetText(LOCTEXT("NoPawn", "暂无受控角色"));
            break;
        case EETStaminaViewState::UnsupportedPawn:
            StaminaStatusText->SetText(LOCTEXT("UnsupportedPawn", "当前角色不提供体力信息"));
            break;
        case EETStaminaViewState::WaitingForGameplayReady:
            StaminaStatusText->SetText(LOCTEXT("Waiting", "角色准备中"));
            break;
        default:
            StaminaStatusText->SetText(LOCTEXT("Unavailable", "体力信息暂不可用"));
            break;
        }
    }

    const FString StateName = StaticEnum<EETStaminaViewState>()->GetNameStringByValue(
        static_cast<int64>(Data.State));
    UE_LOG(LogETStaminaHUD, Verbose,
        TEXT("[M2-01B2] View Widget=%s State=%s Stamina=%.1f MaxStamina=%.1f Fraction=%.3f Reason=%s"),
        *GetNameSafe(this), *StateName, Data.Stamina, Data.MaxStamina,
        Data.StaminaFraction, Reason);
}

#undef LOCTEXT_NAMESPACE