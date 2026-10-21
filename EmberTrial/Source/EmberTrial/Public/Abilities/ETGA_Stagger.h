#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "ETGA_Stagger.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UWorld;

// 本批仅拥有破势状态 / 时长；Poise结算与动作表现后续接入。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Stagger : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Stagger();
    bool HasValidConfiguration() const;

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

    FTimerHandle DurationTimer;
    TWeakObjectPtr<UWorld> TimerWorld;
    uint64 ActivationSerial = 0;

    bool ReadContext(const FGameplayAbilityActorInfo* ActorInfo, AEmberTrialCharacter*& Body,
                     UAbilitySystemComponent*& ASC) const;

    void HandleDurationFinished(uint64 Serial);
};