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

/** 拥有治疗动作的生命周期；生命修改与次数消费交给资源组件。 */
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Heal : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UETGA_Heal();
    // 配置校验与动画通知入口
    bool HasValidConfiguration() const;
    bool HasValidTagPolicy() const;
    // 仅根据本能力基础配置与传入加成计算，不要求当前可施放、不读世界或消费次数。
    bool TryCalculateHealAmount(float Bonus, float& OutAmount) const;
    void HandleHealCommit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
                          int32 MontageInstanceID, UAnimNotify* NotifyToken);

    // GAS生命周期：预检、播放和所有退出路径的统一清理。
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
    friend class FETHealingAbilityTest;
    // 类默认值配置；激活后使用本次快照隔离运行中的配置变化。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing")
    TObjectPtr<UAnimMontage> HealMontage;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing",
              meta = (ClampMin = "1.0", ClampMax = "1000.0"))
    float HealAmount = 40.0f;
    // 当前任务 / Montage由本能力拥有，退出时仅解绑自己的委托。
    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    // 资产通知可共享：Token、实例ID和代次共同隔离重复 / 迟到事件。
    TWeakObjectPtr<UAnimNotify> ActiveCommitToken;
    uint32 ActivationSerial = 0;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    // 本次接受施放时的基础值 + 聚合加成；中途GE变化只影响下一次施放。
    float ActiveHealAmount = 0.0f;
    float ActiveCommitTime = 0.0f;
    // 在可重入回调前先封锁本次提交；取消后不重试。
    bool bCommitAttempted = false;
    // 启动时拒绝通知，结束时拒绝递归结束。
    bool bStarting = false;
    bool bEnding = false;

    // 预检与启动共用：读取当前聚合加成，失败清空输出；生效通知使用施放快照。
    bool TryGetEffectiveHealAmount(UAbilitySystemComponent* ASC, float& OutAmount) const;

    // 生效点重新核对身体与控制权，不依赖激活时的裸指针。
    bool ReadContext(const FGameplayAbilityActorInfo* ActorInfo, AEmberTrialCharacter*& OutBody,
                     UAbilitySystemComponent*& OutASC) const;
    void ReleaseMontage();
    UFUNCTION()
    void HandleCompleted();
    UFUNCTION()
    void HandleAborted();
};