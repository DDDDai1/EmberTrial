#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETHealthPresenter.h"
#include "ETHealthHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UVerticalBox;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETHealthHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void StartObserving(UETHealthPresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETHealthViewData& Data, const TCHAR* Reason);

    UFUNCTION()
    void HandleViewDataChanged(const FETHealthViewData& Data);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HealthStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HealthValuesText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> HealthContent;

    TWeakObjectPtr<UETHealthPresenter> BoundPresenter;
    bool bObserving = false;
};