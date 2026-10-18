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
struct FAnimMontageInstance;

// 后续段只配置Montage和倍率；第一段沿用AttackMontage，避免两处配置首段。
USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETLightAttackStage
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    TObjectPtr<UAnimMontage> Montage = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "0.01"))
    float DamageMultiplier = 1.0f;
};

// 同一Ability拥有整条攻击链，段间不释放Attacking；敌人复用单段路径。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_LightAttack : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_LightAttack();
    bool IsComboEnabled() const
    {
        return bEnableCombo;
    }
    bool HasValidComboConfiguration() const;
    // 输入只更新一个有世界时间期限的槽位，不为连点建立队列。
    bool TryBufferComboInput();
    bool CanCancelIntoDodge() const;
    void HandleComboWindow(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
                           int32 MontageInstanceID, UAnimNotifyState* WindowToken, bool bOpen);

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
    friend class FETLightComboFlowTest;
    friend class FETLightComboCancelTest;

    // 静态配置：动作 / 伤害契约与玩家三段开关。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack")
    TObjectPtr<UAnimMontage> AttackMontage;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack",
              meta = (ClampMin = "0.0", Units = "cm/s"))
    float AttackMaxHorizontalSpeed = 3.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack")
    TSubclassOf<UGameplayEffect> DamageEffectClass;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Attack", meta = (ClampMin = "0.0"))
    float AttackDamage = 25.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Combo")
    bool bEnableCombo = false;
    // Exactly two follow-ups when enabled. AttackMontage remains stage one.
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Combo",
              meta = (EditCondition = "bEnableCombo"))
    TArray<FETLightAttackStage> ComboFollowUps;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Combo",
              meta = (EditCondition = "bEnableCombo", ClampMin = "0.05", ClampMax = "0.50",
                      Units = "s"))
    float ComboBufferSeconds = 0.25f;

    // 激活快照和当前段资源；结束不回写蓝图默认值。
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
    // 三层身份分别拒绝旧命中、旧激活与旧段回调，不可合为一个序号。
    uint32 ActiveTraceSerial = 0;
    uint32 ActivationSerial = 0;
    uint32 StageSerial = 0;
    int32 ActiveStageIndex = INDEX_NONE;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    float ActiveStageDamage = 0.0f;
    // 输入 / 窗口状态与推进重入保护，只在当前激活内有效。
    double BufferedInputExpiresAt = 0.0;
    bool bHasBufferedInput = false;
    bool bComboWindowOpen = false;
    bool bComboWindowPassed = false;
    bool bTransitionPending = false;
    bool bStartingStage = false;
    bool bEndingAttack = false;

    // 查询与推进不对外暴露内部状态；两个玩家请求共用同一身份门控。
    const FAnimMontageInstance* FindPlayerStageMontageInstance() const;
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