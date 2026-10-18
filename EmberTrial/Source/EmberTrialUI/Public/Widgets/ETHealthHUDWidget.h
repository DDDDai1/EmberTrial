#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETHealthPresenter.h"
#include "ETHealthHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UVerticalBox;

// View只消费Presenter快照；无Tick / Text Binding，不直接读写ASC。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETHealthHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 订阅归StartObserving所有；NativeConstruct只重放当前快照。
    void StartObserving(UETHealthPresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETHealthViewData& Data, const TCHAR* Reason);

    UFUNCTION()
    void HandleViewDataChanged(const FETHealthViewData& Data);

    // 绑定名与布局资产保持一致，纯格式清理不改蓝图契约。
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HealthStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> HealthValuesText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> HealthContent;

    // 重建 / Destruct先解绑；旧View即使仍存活，也不再接收Presenter事件。
    TWeakObjectPtr<UETHealthPresenter> BoundPresenter;
    bool bObserving = false;
};