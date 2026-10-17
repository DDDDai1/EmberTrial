#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_Heal.generated.h"

class AEmberTrialCharacter;
class UAnimMontage;
class UAnimNotify;
class UAnimSequenceBase;
class USkeletalMeshComponent;
class UAbilityTask_PlayMontageAndWait;
class UAbilitySystemComponent;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Heal : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UETGA_Heal();
    bool HasValidConfiguration() const;
    bool HasValidTagPolicy() const;
    void HandleHealCommit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
        int32 MontageInstanceID, UAnimNotify* NotifyToken);

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
    friend class FETHealingAbilityTest;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Healing")
    TObjectPtr<UAnimMontage> HealMontage;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Healing", meta=(ClampMin="1.0", ClampMax="1000.0"))
    float HealAmount = 40.0f;
    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    TWeakObjectPtr<UAnimNotify> ActiveCommitToken;
    uint32 ActivationSerial = 0;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    float ActiveHealAmount = 0.0f;
    float ActiveCommitTime = 0.0f;
    bool bCommitAttempted = false;
    bool bStarting = false;
    bool bEnding = false;

    bool ReadContext(const FGameplayAbilityActorInfo* ActorInfo,
        AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const;
    void ReleaseMontage();
    UFUNCTION()
    void HandleCompleted();
    UFUNCTION()
    void HandleAborted();
};