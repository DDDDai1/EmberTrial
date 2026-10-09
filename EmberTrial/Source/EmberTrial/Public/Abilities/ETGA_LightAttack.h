#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_LightAttack.generated.h"

class UAnimMontage;
class UETCombatTraceComponent;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
struct FHitResult;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_LightAttack : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_LightAttack();

protected:
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
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
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack")
    TObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack",
        meta = (ClampMin = "0.0", Units = "cm/s"))
    float AttackMaxHorizontalSpeed = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack",
        meta = (ClampMin = "0.0"))
    float AttackDamage = 25.0f;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    TWeakObjectPtr<UETCombatTraceComponent> ActiveTrace;
    FDelegateHandle TraceHitHandle;
    uint32 ActiveTraceSerial = 0;

    bool HasValidDamageConfiguration() const;

    void HandleTraceHit(uint32 Serial, const FHitResult& Hit);

    UFUNCTION()
    void HandleMontageCompleted();

    UFUNCTION()
    void HandleMontageAborted();
};