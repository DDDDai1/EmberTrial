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

// 正式闪避的事务所有者；动画、位移与短无敌分别结束，不能互相代替。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Dodge : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Dodge();
    // 配置查询同时供运行时预检、Editor校验器与自动化使用。
    static bool ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost);
    bool HasValidMovementConfiguration() const;
    bool HasValidInvulnerabilityConfiguration() const;
    bool HasValidRecoveryCancelPolicy() const;

protected:
    // GAS入口：预检无副作用，启动成功后提交成本，所有退出汇入EndAbility。
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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
    friend class FETLightComboCancelTest;

    // 动画 / 位移配置与任务。Montage拥有整段动作，位移完成只释放自己的Task。
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

    // 短无敌使用独立GE时长；只保存本次Handle，不清除其他来源的保护。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge",
              meta = (ClampMin = "0.01", ClampMax = "0.50", Units = "s"))
    float InvulnerabilityDuration = 0.15f;

    UPROPERTY(Transient)
    TWeakObjectPtr<UAbilitySystemComponent> InvulnerabilityASC;

    FActiveGameplayEffectHandle InvulnerabilityHandle;
    bool StartInvulnerabilityWindow(UAbilitySystemComponent* ASC, uint32 Serial);

    // Activate / End推进序号；同步回调后用它识别已结束或已被替换的激活。
    uint32 ActivationSerial = 0;
    // ApplyCost在UE中为const；该标记只保证本次激活不重复提交，并非第二份体力。
    mutable bool bCostSubmitted = false;

    // 任务回调：位移结束可继续播放动画；任一任务中断则取消整次闪避。
    UFUNCTION()
    void HandleMontageCompleted();
    UFUNCTION()
    void HandleMontageAborted();

    UFUNCTION()
    void HandleMoveCompleted();
    UFUNCTION()
    void HandleMoveAborted();
};