#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETBreakPulsePresenter.h"
#include "ETBreakPulseHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UVerticalBox;

// 一份Presenter对应一份正式View；无Tick / Binding，不改变GAS冷却。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETBreakPulseHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void StartObserving(UETBreakPulsePresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETBreakPulseViewData& Data);
    UFUNCTION()
    void HandleViewDataChanged(const FETBreakPulseViewData& Data);
    UFUNCTION()
    void HandleVisibilityChanged(ESlateVisibility InVisibility);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> BreakPulseStatusText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> BreakPulseValuesText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> BreakPulseBar;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> BreakPulseContent;

    TWeakObjectPtr<UETBreakPulsePresenter> BoundPresenter;
    bool bObserving = false;
};