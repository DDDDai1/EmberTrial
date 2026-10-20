#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETHealingPresenter.h"
#include "ETHealingHUDWidget.generated.h"

class UTextBlock;
class UVerticalBox;

// 离散次数仅显示整数，不造进度条或在View缓存一套业务资源。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETHealingHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void StartObserving(UETHealingPresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETHealingViewData& Data, const TCHAR* Reason);

    UFUNCTION()
    void HandleViewDataChanged(const FETHealingViewData& Data);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HealingStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HealingValuesText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> HealingContent;

    TWeakObjectPtr<UETHealingPresenter> BoundPresenter;
    bool bObserving = false;
};