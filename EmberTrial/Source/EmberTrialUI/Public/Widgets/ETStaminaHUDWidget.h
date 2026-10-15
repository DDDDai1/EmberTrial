#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETStaminaPresenter.h"
#include "ETStaminaHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UVerticalBox;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETStaminaHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void StartObserving(UETStaminaPresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETStaminaViewData& Data, const TCHAR* Reason);

    UFUNCTION()
    void HandleViewDataChanged(const FETStaminaViewData& Data);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StaminaStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StaminaValuesText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> StaminaBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> StaminaContent;

    TWeakObjectPtr<UETStaminaPresenter> BoundPresenter;
    bool bObserving = false;
};