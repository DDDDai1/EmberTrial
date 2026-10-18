#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETLifecycleStatusWidget.generated.h"

class APawn;
class UTextBlock;
class UETUILocalPlayerSubsystem;

// 只观察LocalPlayer的Pawn身份；有Pawn不代表玩法属性已Ready。
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

    // 布局只提供控件，不添加Text Binding或输入业务。
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PawnText;

    // 只移除本View的委托；不清空共享Subsystem的其他订阅。
    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    bool bObserving = false;
};
