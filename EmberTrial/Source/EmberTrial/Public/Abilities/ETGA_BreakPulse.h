#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_BreakPulse.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

// C2-01只负责施放 / 真实冷却事务；C2-02再接唯一通知与范围结算。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_BreakPulse : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UETGA_BreakPulse();
    bool HasValidConfiguration() const;
    virtual const FGameplayTagContainer* GetCooldownTags() const override;

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
    friend class FETBreakPulseAbilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse")
    TObjectPtr<UAnimMontage> CastMontage;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "0.1", ClampMax = "60.0"))
    float CooldownSeconds = 6.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "0.0", ClampMax = "600.0"))
    float CastMaxHorizontalSpeed = 3.0f;

    FGameplayTagContainer NativeCooldownTags;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;
    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
    uint32 ActivationSerial = 0;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    bool bEnding = false;

    bool ReadContext(const FGameplayAbilityActorInfo* ActorInfo, AEmberTrialCharacter*& Body,
                     UAbilitySystemComponent*& ASC) const;
    void ReleaseMontage();
    UFUNCTION()
    void HandleCompleted();
    UFUNCTION()
    void HandleAborted();
};