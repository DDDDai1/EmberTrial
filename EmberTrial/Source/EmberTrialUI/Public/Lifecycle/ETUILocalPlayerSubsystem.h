#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ETUILocalPlayerSubsystem.generated.h"

class AActor;
class APawn;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FETUIObservedPawnChanged, APawn*, PreviousPawn, APawn*,
                                             CurrentPawn);

// 每个LocalPlayer的一份身份观察入口，不拥有Pawn，也不判断GameplayReady。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETUILocalPlayerSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void PlayerControllerChanged(APlayerController* NewController) override;

    // Getter只读当前弱引用，不触发Possess或资源初始化。
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    APlayerController* GetObservedController() const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    APawn* GetObservedPawn() const;

    // 先更新内部当前身份，再广播；消费者可重新查询当前快照。
    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETUIObservedPawnChanged OnObservedPawnChanged;

private:
    // ---- Controller / Pawn两层订阅分别清理，禁止清空其他观察者的共享事件 ----
    void RebindController(APlayerController* NewController);
    void ClearController(const TCHAR* Reason);
    void RefreshObservedPawn(const TCHAR* Reason);
    void SetObservedPawn(APawn* NewPawn, const TCHAR* Reason);

    // ---- 引擎事件适配；旧对象EndPlay必须先核对身份 ----
    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    UFUNCTION()
    void HandleControllerEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

    UFUNCTION()
    void HandlePawnEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

    // 不延长世界对象寿命；IsStale区分“从未绑定”和“旧对象已失效”。
    TWeakObjectPtr<APlayerController> ObservedController;
    TWeakObjectPtr<APawn> ObservedPawn;
};
