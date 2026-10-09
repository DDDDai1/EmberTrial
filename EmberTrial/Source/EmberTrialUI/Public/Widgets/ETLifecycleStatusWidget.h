#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETLifecycleStatusWidget.generated.h"

class APawn;
class UTextBlock;
class UETUILocalPlayerSubsystem;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETLifecycleStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 主动解绑，不等待 Slate 销毁或 GC。
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void RefreshStatus(const TCHAR* Reason);

    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PawnText;

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    bool bObserving = false;
};