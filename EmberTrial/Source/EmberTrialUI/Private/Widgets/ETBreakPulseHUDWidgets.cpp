#include "Widgets/ETBreakPulseHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Internationalization/Text.h"

#define LOCTEXT_NAMESPACE "ETBreakPulseHUD"

void UETBreakPulseHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (IsDesignTime())
    {
        return;
    }
    OnVisibilityChanged.AddUniqueDynamic(this, &ThisClass::HandleVisibilityChanged);
    const UETBreakPulsePresenter* Presenter = BoundPresenter.Get();
    ApplyViewData(Presenter ? Presenter->GetSnapshot() : FETBreakPulseViewData{});
}

void UETBreakPulseHUDWidget::NativeDestruct()
{
    StopObserving();
    OnVisibilityChanged.RemoveDynamic(this, &ThisClass::HandleVisibilityChanged);
    Super::NativeDestruct();
}

void UETBreakPulseHUDWidget::StartObserving(UETBreakPulsePresenter* Presenter)
{
    StopObserving();
    if (IsDesignTime())
    {
        return;
    }
    if (!IsValid(Presenter))
    {
        ApplyViewData(FETBreakPulseViewData{});
        return;
    }
    bObserving = true;
    BoundPresenter = Presenter;
    Presenter->OnViewDataChanged.AddUniqueDynamic(this, &ThisClass::HandleViewDataChanged);
    // 晚加入 / 重建时先重读真实剩余时间，再显示；不从缓存数字自行倒计时。
    Presenter->SetPresentationEnabled(IsVisible());
    if (bObserving && BoundPresenter.Get() == Presenter)
    {
        ApplyViewData(Presenter->GetSnapshot());
    }
}

void UETBreakPulseHUDWidget::StopObserving()
{
    bObserving = false;
    UETBreakPulsePresenter* Presenter = BoundPresenter.Get();
    BoundPresenter.Reset();
    if (IsValid(Presenter))
    {
        Presenter->OnViewDataChanged.RemoveDynamic(this, &ThisClass::HandleViewDataChanged);
        Presenter->SetPresentationEnabled(false);
    }
}

void UETBreakPulseHUDWidget::HandleVisibilityChanged(ESlateVisibility InVisibility)
{
    (void)InVisibility;
    if (bObserving && BoundPresenter.IsValid())
    {
        BoundPresenter->SetPresentationEnabled(IsVisible());
    }
}

void UETBreakPulseHUDWidget::HandleViewDataChanged(const FETBreakPulseViewData& Data)
{
    if (bObserving && BoundPresenter.IsValid())
    {
        ApplyViewData(Data);
    }
}

void UETBreakPulseHUDWidget::ApplyViewData(const FETBreakPulseViewData& Data)
{
    if (!BreakPulseStatusText || !BreakPulseValuesText || !BreakPulseBar || !BreakPulseContent)
    {
        return;
    }
    if (Data.State == EETBreakPulseViewState::Ready)
    {
        BreakPulseContent->SetVisibility(ESlateVisibility::HitTestInvisible);
        BreakPulseBar->SetPercent(Data.ReadyFraction);
        BreakPulseStatusText->SetText(Data.bCasting ? LOCTEXT("Casting", "破势（F）· 施放中")
                                      : Data.bCoolingDown
                                          ? LOCTEXT("Cooling", "破势（F）· 冷却中")
                                          : LOCTEXT("Ready", "破势（F）· 冷却就绪"));
        if (Data.bCoolingDown)
        {
            // Tag撤销前至少显示0.1，避免读成“0.0秒但仍不能用”。
            const float Seconds =
                FMath::Max(.1f, FMath::CeilToFloat(Data.Remaining * 10.0f) / 10.0f);
            FNumberFormattingOptions Options;
            Options.SetMinimumFractionalDigits(1);
            Options.SetMaximumFractionalDigits(1);
            BreakPulseValuesText->SetText(FText::Format(LOCTEXT("Remaining", "剩余 {0} 秒"),
                                                        FText::AsNumber(Seconds, &Options)));
        }
        else
        {
            BreakPulseValuesText->SetText(LOCTEXT("StationaryHint", "地面站定后按 F"));
        }
        return;
    }
    BreakPulseContent->SetVisibility(ESlateVisibility::Collapsed);
    BreakPulseBar->SetPercent(0.0f);
    BreakPulseValuesText->SetText(FText::GetEmpty());
    switch (Data.State)
    {
    case EETBreakPulseViewState::Dead:
        BreakPulseStatusText->SetText(LOCTEXT("Dead", "破势（F）· 角色已死亡"));
        break;
    case EETBreakPulseViewState::NoPawn:
        BreakPulseStatusText->SetText(LOCTEXT("NoPawn", "破势（F）· 暂无受控角色"));
        break;
    case EETBreakPulseViewState::UnsupportedPawn:
        BreakPulseStatusText->SetText(LOCTEXT("Unsupported", "当前角色不提供破势技能"));
        break;
    case EETBreakPulseViewState::WaitingForGameplayReady:
        BreakPulseStatusText->SetText(LOCTEXT("Waiting", "破势（F）· 角色准备中"));
        break;
    default:
        BreakPulseStatusText->SetText(LOCTEXT("Unavailable", "破势（F）· 暂不可用"));
        break;
    }
}

#undef LOCTEXT_NAMESPACE