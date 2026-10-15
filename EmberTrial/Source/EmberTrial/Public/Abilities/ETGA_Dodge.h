#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Dodge : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Dodge();
    static bool ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost);

protected:
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    friend class FETDodgeCostContractTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge")
    TObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    uint32 ActivationSerial = 0;
    // UE declares ApplyCost const. This flag belongs only to the current attempt.
    mutable bool bCostSubmitted = false;

    UFUNCTION()
    void HandleMontageCompleted();
    UFUNCTION()
    void HandleMontageAborted();
};