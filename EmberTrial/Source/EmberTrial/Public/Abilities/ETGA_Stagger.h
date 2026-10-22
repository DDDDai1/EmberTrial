#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "ETGA_Stagger.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UWorld;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

// Timer决定状态时长；Montage只负责表现，不提前解除Stunned。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Stagger : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Stagger();
    bool HasValidConfiguration() const;

    // 本轮是否成功接管过动作；供结束观察者区分启动拒绝，不代表当前Active状态。
    bool WasReactionStarted() const
    {
        return bReactionStarted;
    }

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
    friend class FETStaggerAbilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stagger",
              meta = (ClampMin = "0.05", ClampMax = "10.0"))
    float StaggerDuration = 1.2f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stagger")
    TObjectPtr<UAnimMontage> StaggerMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    bool bReactionStarted = false;

    FTimerHandle DurationTimer;
    TWeakObjectPtr<UWorld> TimerWorld;
    uint64 ActivationSerial = 0;

    bool ReadContext(const FGameplayAbilityActorInfo* ActorInfo, AEmberTrialCharacter*& Body,
                     UAbilitySystemComponent*& ASC) const;

    void HandleDurationFinished(uint64 Serial);
};