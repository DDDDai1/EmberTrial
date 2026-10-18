#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETHealingResourceComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;

// 事件表示快照可能变化；观察者收到后应重新读取当前状态。
DECLARE_MULTICAST_DELEGATE(FETHealingResourceChanged);

/** 身体持有的离散治疗次数；不管理输入、Montage或HUD。 */
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETHealingResourceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETHealingResourceComponent();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

    // 配置 / 快照查询与一次治疗事务
    bool HasValidConfiguration() const;
    bool GetHealingSnapshot(int32& OutCharges, int32& OutMaxCharges) const;
    bool CanApplyHealing(float Amount) const;
    bool TryApplyHealing(float Amount);
    // 订阅者负责解绑；EndPlay撤销可用性后清空委托。
    FETHealingResourceChanged OnSnapshotChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETHealingResourceTest;

    // 出生配置属于当前身体；失活 / 再激活不会补满次数。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing",
              meta = (ClampMin = "1", ClampMax = "9"))
    int32 MaxCharges = 3;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing",
              meta = (ClampMin = "0", ClampMax = "9"))
    int32 InitialCharges = 3;

    bool ReadBody(AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const;
    void RefreshAvailability();

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Body, bool bReady);

    // 可用性与初始化分离，Ready变化不等于重新补满。
    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    int32 CurrentCharges = 0;
    bool bInitialized = false;
    bool bAvailable = false;
    // 阻止GE / 快照回调递归消费；结束后不再恢复可用性。
    bool bApplying = false;
    bool bEndingPlay = false;
};