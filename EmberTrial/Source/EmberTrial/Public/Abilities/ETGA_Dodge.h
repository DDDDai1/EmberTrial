#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActiveGameplayEffectHandle.h"
#include "ETGA_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
class UETAbilityTask_DodgeMove;
class UAbilitySystemComponent;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Dodge : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Dodge();
    static bool ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost);
    bool HasValidMovementConfiguration() const;
    bool HasValidInvulnerabilityConfiguration() const;

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
    friend class FETDodgeMovementTest;
    friend class FETDodgeInvulnerabilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge")
    TObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge",
        meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "cm"))
    float DodgeDistance = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge",
        meta = (ClampMin = "0.05", ClampMax = "0.50", Units = "s"))
    float DodgeDuration = 0.30f;

    UPROPERTY(Transient)
    TObjectPtr<UETAbilityTask_DodgeMove> MoveTask;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge",
        meta = (ClampMin = "0.01", ClampMax = "0.50", Units = "s"))
    float InvulnerabilityDuration = 0.15f;

    UPROPERTY(Transient)
    TWeakObjectPtr<UAbilitySystemComponent> InvulnerabilityASC;

    FActiveGameplayEffectHandle InvulnerabilityHandle;
    bool StartInvulnerabilityWindow(UAbilitySystemComponent* ASC, uint32 Serial);

    uint32 ActivationSerial = 0;
    // UE declares ApplyCost const. This flag belongs only to the current attempt.
    mutable bool bCostSubmitted = false;

    UFUNCTION()
    void HandleMontageCompleted();
    UFUNCTION()
    void HandleMontageAborted();

    UFUNCTION()
    void HandleMoveCompleted();
    UFUNCTION()
    void HandleMoveAborted();
};