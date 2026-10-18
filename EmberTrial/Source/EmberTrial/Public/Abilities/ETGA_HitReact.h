#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_HitReact.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

// 正式受击只管理自己的动画；伤害数值与死亡封口由CombatLife链路处理。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_HitReact : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_HitReact();

    // 供激活预检、资产校验和测试共用；受击必须能取消治疗。
    bool HasValidHealingCancelPolicy() const;

protected:
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                            const FGameplayAbilityActorInfo* ActorInfo,
                            const FGameplayAbilityActivationInfo ActivationInfo,
                            bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    // 配置与当前Task分开保存，动画自然结束或中断统一交回GAS收尾。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Hit React")
    TObjectPtr<UAnimMontage> HitReactMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    UFUNCTION()
    void HandleMontageCompleted();

    UFUNCTION()
    void HandleMontageAborted();
};