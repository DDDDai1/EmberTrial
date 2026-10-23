#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_BreakPulse.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UAnimMontage;
class UAnimNotify;
class UAnimSequenceBase;
class USkeletalMeshComponent;
class UAbilityTask_PlayMontageAndWait;

// 施放与冷却由GA拥有；无状态Notify只路由准确实例的一次范围提交。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_BreakPulse : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UETGA_BreakPulse();
    bool HasValidConfiguration() const;
    void HandleBreakPulseCommit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
                                int32 MontageInstanceID, UAnimNotify* NotifyToken);
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
    friend class FETBreakPulseRangeTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse")
    TObjectPtr<UAnimMontage> CastMontage;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "0.1", ClampMax = "60.0"))
    float CooldownSeconds = 6.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "0.0", ClampMax = "600.0"))
    float CastMaxHorizontalSpeed = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "1.0", ClampMax = "3000.0", Units = "cm"))
    float PulseRadius = 300.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "1.0", ClampMax = "1000.0"))
    float PoiseDamage = 60.0f;

    // 一次激活的快照；运行中改CDO不改变已接受施放的范围 / 数值。
    TWeakObjectPtr<UAnimNotify> ActiveCommitToken;
    float ActiveCommitTime = 0.0f;
    float ActiveRadius = 0.0f;
    float ActivePoiseDamage = 0.0f;
    bool bStarting = false;
    bool bCommitAttempted = false;
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
    bool IsCurrentCast(uint32 Serial) const;
    void ResolvePulse(uint32 Serial, const FVector& Origin, float Radius, float Damage);
    void ReleaseMontage();
    UFUNCTION()
    void HandleCompleted();
    UFUNCTION()
    void HandleAborted();
};