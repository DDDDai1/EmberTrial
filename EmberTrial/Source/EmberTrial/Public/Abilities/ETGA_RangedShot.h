#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Combat/ETRangedShot.h"
#include "ETGA_RangedShot.generated.h"

class AEmberTrialCharacter;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UAnimSequenceBase;
class USkeletalMeshComponent;
class UETAN_RangedShotRelease;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_RangedShot : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_RangedShot();
    bool HasValidConfiguration() const;
    const FETRangedShotConfig& GetShotConfig() const { return ShotConfig; }
    void HandleRelease(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
                       int32 InstanceID, const UETAN_RangedShotRelease* Notify);

protected:
    virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo,
                                              const FGameplayEventData* Payload) const override;
    virtual bool CanActivateAbility(
        FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                 FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                            FGameplayAbilityActivationInfo ActivationInfo,
                            bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    friend class FETRangedShotAbilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Ranged Shot")
    TObjectPtr<UAnimMontage> CastMontage;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Ranged Shot")
    FETRangedShotConfig ShotConfig;

    UPROPERTY(Transient)
    FETRangedShotConfig ActiveConfig;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    TWeakObjectPtr<AEmberTrialCharacter> ActiveTarget;
    TWeakObjectPtr<UETAN_RangedShotRelease> ActiveNotify;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
    FDelegateHandle HitHandle;
    FDelegateHandle StunHandle;
    int32 ActiveInstanceID = INDEX_NONE;
    float ActiveReleaseTime = 0.0f;
    uint32 ActivationSerial = 0;
    bool bReleaseAttempted = false;
    bool bEnding = false;

    static AEmberTrialCharacter* ReadSource(const FGameplayAbilityActorInfo* ActorInfo);
    static AEmberTrialCharacter* ReadTarget(AEmberTrialCharacter* Source, const FGameplayEventData* Payload);
    void HandleInterruptTag(FGameplayTag Tag, int32 Count);
    UFUNCTION()
    void HandleCompleted();
    UFUNCTION()
    void HandleAborted();
};