#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETStaminaPresenter.h"
#include "ETStaminaHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UVerticalBox;

// View只消费Presenter快照；无Tick / Text Binding，不直接读写ASC。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETStaminaHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 订阅归StartObserving所有；NativeConstruct只重放当前快照。
    void StartObserving(UETStaminaPresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETStaminaViewData& Data, const TCHAR* Reason);

    UFUNCTION()
    void HandleViewDataChanged(const FETStaminaViewData& Data);

    // 绑定名与布局资产保持一致，纯格式清理不改蓝图契约。
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StaminaStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StaminaValuesText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> StaminaBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> StaminaContent;

    // 重建 / Destruct先解绑；旧View即使仍存活，也不再接收Presenter事件。
    TWeakObjectPtr<UETStaminaPresenter> BoundPresenter;
    bool bObserving = false;
};