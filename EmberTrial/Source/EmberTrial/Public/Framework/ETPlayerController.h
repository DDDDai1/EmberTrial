#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ETPlayerController.generated.h"

class APawn;
class UInputAction;
class UETTargetLockComponent;

DECLARE_MULTICAST_DELEGATE(FETPauseRequested);

// Core只负责输入意图 / 控制生命周期；菜单与Widget装配由UI模块决定。
UCLASS()
class EMBERTRIAL_API AETPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AETPlayerController();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Target Lock")
    UETTargetLockComponent* GetTargetLockComponent() const;

    // Core emits an intent; the UI module decides whether to present a menu.
    FETPauseRequested OnPauseRequested;

    // 蓝图事件桥接现有交互提示，不在Core中引用具体UI类型。
    UFUNCTION(BlueprintImplementableEvent, Category = "EmberTrial|Flow")
    void OnGameMenuStateChanged();

    // ---- 可复用PIE诊断；0只读，非Editor实现为空，不作为正式资源操作入口 ----
    UFUNCTION(Exec, meta = (DevelopmentOnly))
    void ET_Stamina(float Delta);

    UFUNCTION(Exec, meta = (DevelopmentOnly))
    void ET_Healing(float Amount);

protected:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void HandlePauseReleased();

    // Controller跨Possess存在；目标身份由组件在换身体时主动清空。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Target Lock",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETTargetLockComponent> TargetLockComponent;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock")
    TObjectPtr<UInputAction> TargetLockAction;
    void HandleTargetLockStarted();
};
