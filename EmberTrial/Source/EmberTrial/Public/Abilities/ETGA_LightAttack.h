#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "ETGA_LightAttack.generated.h"

class UAnimMontage;
class UAnimNotifyState;
class UAnimSequenceBase;
class USkeletalMeshComponent;
class UETCombatTraceComponent;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
struct FHitResult;

USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETLightAttackStage
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category="Combo")
    TObjectPtr<UAnimMontage> Montage = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="Combo", meta=(ClampMin="0.01"))
    float DamageMultiplier = 1.0f;
};

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_LightAttack : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_LightAttack();
    bool IsComboEnabled() const { return bEnableCombo; }
    bool HasValidComboConfiguration() const;
    bool TryBufferComboInput();
    bool CanCancelIntoDodge() const;
    void HandleComboWindow(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
        int32 MontageInstanceID, UAnimNotifyState* WindowToken, bool bOpen);

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
    friend class FETLightComboFlowTest;
    friend class FETLightComboCancelTest;

    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack")
    TObjectPtr<UAnimMontage> AttackMontage;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack", meta=(ClampMin="0.0",Units="cm/s"))
    float AttackMaxHorizontalSpeed = 3.0f;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack")
    TSubclassOf<UGameplayEffect> DamageEffectClass;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack", meta=(ClampMin="0.0"))
    float AttackDamage = 25.0f;

    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Combo")
    bool bEnableCombo = false;
    // Exactly two follow-ups when enabled. AttackMontage remains stage one.
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Combo", meta=(EditCondition="bEnableCombo"))
    TArray<FETLightAttackStage> ComboFollowUps;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Combo",
        meta=(EditCondition="bEnableCombo",ClampMin="0.05",ClampMax="0.50",Units="s"))
    float ComboBufferSeconds = 0.25f;

    UPROPERTY(Transient)
    TArray<FETLightAttackStage> RuntimeStages;
    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    TWeakObjectPtr<UETCombatTraceComponent> ActiveTrace;
    TWeakObjectPtr<UAnimNotifyState> ActiveComboWindow;
    FDelegateHandle TraceHitHandle;
    FTimerHandle TransitionTimer;
    uint32 ActiveTraceSerial = 0;
    uint32 ActivationSerial = 0;
    uint32 StageSerial = 0;
    int32 ActiveStageIndex = INDEX_NONE;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    float ActiveStageDamage = 0.0f;
    double BufferedInputExpiresAt = 0.0;
    bool bHasBufferedInput = false;
    bool bComboWindowOpen = false;
    bool bComboWindowPassed = false;
    bool bTransitionPending = false;
    bool bStartingStage = false;
    bool bEndingAttack = false;

    bool HasValidDamageConfiguration() const;
    bool HasNextStage() const;
    bool CanAcceptComboInput() const;
    bool StartStage(int32 Index);
    void ClearBufferedInput();
    void ClearStageResources();
    void TryConsumeBufferedInput();
    void HandleQueuedTransition(uint32 ExpectedActivation, uint32 ExpectedStage);
    void HandleTraceHit(uint32 Serial, const FHitResult& Hit);
    UFUNCTION()
    void HandleMontageCompleted();
    UFUNCTION()
    void HandleMontageAborted();
};