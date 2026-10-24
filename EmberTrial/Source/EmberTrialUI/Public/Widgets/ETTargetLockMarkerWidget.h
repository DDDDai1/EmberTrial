#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETTargetLockMarkerWidget.generated.h"

class UBorder;

// 只接收UMG局部坐标与显示资格，不拥有目标或查询玩法状态。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETTargetLockMarkerWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    bool HasValidMarkerLayout() const;
    void ApplyMarkerFrame(bool bShow, const FVector2D& Position);

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> LockMarker;
};