# M2-05C2-02：唯一动画通知与真实范围破势

2026-10-23，固定UE5.5.4。前置[C2-01已验收](Reports/M2_05C2_01_Verification.md)，无待补项；本章为**待手动实施**，三新 / 五旧共8份Source，完整代码见下文。助手只准备忽略目录候选和指南，未直接改实际Source、资产，未代Build / 自动化 / PIE或操作Git。

本批把已有F施放接到第12帧 / .4秒的一次范围Poise GE，复用B2的归零反应 / 恢复。F绑定、6秒真实冷却、三类旧资源HUD保持原逻辑；冷却反馈UI留D。预计手动70–110分钟（代码40–65、资产配置15–30、短查8–12），构建 / 索引等待另列。

## 1. 做完后的规则与边界

| 情况 | 行为 |
| --- | --- |
| 站定新按F并被接受 | 立刻提交一次6秒冷却并开始前推动作；按键瞬间不扣目标Poise |
| AM第12帧 / .400000秒 | 只接受当前Mesh / Montage / 实例ID / Notify Token；本次范围结算尝试一次 |
| 范围 | **360度球体**，球心为施放者当时Actor / 胶囊中心、半径300cm；不是手掌起点或前方扇形 |
| 最终距离 | 目标Actor / 胶囊中心距球心≤300cm；胶囊边缘擦入球但中心在外仍拒绝，包含Z差 |
| 合法目标 | 现有Enemy或Neutral身体，Ready、存活、ASC / 属性归属正确、Poise>0；自身 / Player同阵营 / 不支持的Pawn跳过 |
| 世界遮挡 | 两身体中心间Visibility检测；忽略来源和该目标自身，有阻挡就跳过该目标 |
| 真实结算 | 每名目标独立创建一个原生Instant PoiseDelta GE Spec，SetByCaller=-60；不改Health / Stamina / 次数 |
| 100→40→0 | 第一次保持40；下一次F归零触发已有1.2秒Stagger，结束恢复当前MaxPoise |
| 无敌 / 已Stunned / Poise已0等待反应 | 跳过，不在反应中持续压低 / 重启破势 |
| 第12帧前取消 | 无范围结算，冷却不退；第12帧后取消保留已生效GE |
| 目标进入 / 离开范围 | 按**生效时**位置与状态，按键时不缓存候选 |
| 空放 / 全部被拒绝 | 仍只尝试一次，不退款冷却，不反复查询直到命中 |
| Pause / Stop / 换身体 | Pause冻结通知与冷却；旧实例 / 旧回调不能写新施放，EndPlay撤销自身活动状态 |

多个目标按中心距离由近到远处理。某个GE的同步回调若取消施放，则停止后续目标，已经落地的GE保留；不承诺整组目标原子回滚。范围中心、Radius / Damage都是本次提交的局部快照，Actor列表为弱引用，每次应用前重查当前激活，应用后不复用旧裸指针。

查询不逐帧扫描Actor：仅通知时一次Overlap，TSet按身体去重，再做当前状态 / 精确距离 / 遮挡检查。Tag、输入、恢复Timer和冷却GE都复用现有机制；不新增范围组件、第二个资源集、临时Controller函数或常驻Tick。没有新性能采样，不声称FPS提升。

## 2. 准备与准确文件清单

1. Stop PIE → Save All → 关闭UE，CLion保存已有文件。本批新增反射类 / 属性，需要正常UBT / UHT，不用Live Coding。
2. 所有下面的文件路径均相对`E:\UE_Project\Demo\EmberTrial\Source`；`Animation`用单数，不建在Content里。新文件只创建下表三份，旧文件按对应章节改。

| 新旧 | Source相对路径 | 操作 |
| --- | --- | --- |
| 新 | EmberTrial/Public/Animation/ETAN_BreakPulseCommit.h | 第3.1节全文 |
| 新 | EmberTrial/Private/Animation/ETAN_BreakPulseCommit.cpp | 第3.2节全文 |
| 新 | EmberTrial/Tests/ETBreakPulseRangeTests.cpp | 第6节全文 |
| 旧 | EmberTrial/Public/Abilities/ETGA_BreakPulse.h | 第4.1节全文替换 |
| 旧 | EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp | 第4.2节全文替换，含原冷却 / 清理逻辑 |
| 旧 | EmberTrial/Public/Characters/EmberTrialCharacter.h | 第5.1节增加一行friend |
| 旧 | EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp | 第5.2节替换一行诊断文字 |
| 旧 | EmberTrial/Tests/ETBreakPulseAbilityTests.cpp | 第5.3节替换一条断言说明，不放宽条件 |

现有ETPoiseDeltaEffect与SetByCaller.PoiseDelta直接复用，不新建GE蓝图；不改Build.cs、Target、uproject、Tag或F输入映射。先完成第3–6节全部Source，再第7节Build；不要只创建空类就提前构建。

## 3. 新增无状态原生Notify

### 3.1 `EmberTrial/Public/Animation/ETAN_BreakPulseCommit.h`

新文件完整内容：

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ETAN_BreakPulseCommit.generated.h"

// 动画资产上的无状态路由；一次提交、范围目标结算与取消保护由当前GA负责。
UCLASS(meta = (DisplayName = "ET Break Pulse Commit"))
class EMBERTRIAL_API UETAN_BreakPulseCommit : public UAnimNotify
{
    GENERATED_BODY()
public:
    UETAN_BreakPulseCommit();
    // 传递Mesh / 动画 / Montage实例 / Notify身份，不在共享Notify中缓存身体。
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};
```

### 3.2 `EmberTrial/Private/Animation/ETAN_BreakPulseCommit.cpp`

新文件完整内容：

```cpp
#include "Animation/ETAN_BreakPulseCommit.h"
#include "Abilities/ETGA_BreakPulse.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETAN_BreakPulseCommit::UETAN_BreakPulseCommit()
{
    // BranchingPoint保持同步通知；编辑器预览不执行正式破势范围结算业务。
    bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;
    NotifyColor = FColor::Cyan;
#endif
}
// 当前AnimatingAbility负责核对激活与准确实例，Notify不直接操作Poise。
void UETAN_BreakPulseCommit::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body =
        IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC = IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_BreakPulse* Pulse =
        IsValid(ASC) ? Cast<UETGA_BreakPulse>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Pulse))
    {
        Pulse->HandleBreakPulseCommit(Mesh, Payload.SequenceAsset, Payload.MontageInstanceID, this);
    }
}
```

Notify只转发准确事件；共享通知对象不缓存角色、目标或本次是否命中的布尔值。bIsNativeBranchingPoint保证同步路由，预览不执行玩法；真正的一次提交由当前GA实例保存。

## 4. 更新破势GA完整文件

本节两份是**完整最终版**，分别全选文件内容后替换，避免只追加ResolvePulse却漏掉新配置 / 生效标记 / EndPlay清理。原冷却实现保留在最终版里，Notify里**不再调用CommitAbility或添加第二份冷却GE**。

### 4.1 `EmberTrial/Public/Abilities/ETGA_BreakPulse.h`

完整替换：

```cpp
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
```

### 4.2 `EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp`

完整替换：

```cpp
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Effects/ETBreakPulseCooldownEffect.h"
#include "Abilities/Effects/ETPoiseDeltaEffect.h"
#include "Animation/ETAN_BreakPulseCommit.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogETBreakPulse, Log, All);

// ---- 配置与互斥；普通受击 / 破势 / 死亡可以中断施放 ----
UETGA_BreakPulse::UETGA_BreakPulse()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_BreakPulse);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_CastingBreakPulse);
    const FGameplayTag BlockedStates[] = {ETGameplayTags::State_CastingBreakPulse,
                                          ETGameplayTags::State_Attacking,
                                          ETGameplayTags::State_Dodging,
                                          ETGameplayTags::State_Healing,
                                          ETGameplayTags::State_HitReact,
                                          ETGameplayTags::State_Stunned,
                                          ETGameplayTags::State_Dead};
    for (const FGameplayTag& Tag : BlockedStates)
    {
        ActivationBlockedTags.AddTag(Tag);
    }
    const FGameplayTag BlockedAbilities[] = {ETGameplayTags::Ability_Attack_Light,
                                             ETGameplayTags::Ability_Dodge,
                                             ETGameplayTags::Ability_Heal};
    for (const FGameplayTag& Tag : BlockedAbilities)
    {
        BlockAbilitiesWithTag.AddTag(Tag);
    }
    NativeCooldownTags.AddTag(ETGameplayTags::Cooldown_BreakPulse);
}

const FGameplayTagContainer* UETGA_BreakPulse::GetCooldownTags() const
{
    return &NativeCooldownTags;
}

bool UETGA_BreakPulse::HasValidConfiguration() const
{
    const FGameplayTag States[] = {ETGameplayTags::State_CastingBreakPulse,
                                   ETGameplayTags::State_Attacking,
                                   ETGameplayTags::State_Dodging,
                                   ETGameplayTags::State_Healing,
                                   ETGameplayTags::State_HitReact,
                                   ETGameplayTags::State_Stunned,
                                   ETGameplayTags::State_Dead};
    const FGameplayTag Actions[] = {ETGameplayTags::Ability_Attack_Light,
                                    ETGameplayTags::Ability_Dodge, ETGameplayTags::Ability_Heal};
    if (InstancingPolicy != EGameplayAbilityInstancingPolicy::InstancedPerActor ||
        NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::ServerOnly ||
        ReplicationPolicy != EGameplayAbilityReplicationPolicy::ReplicateNo ||
        bRetriggerInstancedAbility || GetCostGameplayEffect() || GetCooldownGameplayEffect() ||
        GetAssetTags().Num() != 1 ||
        !GetAssetTags().HasTagExact(ETGameplayTags::Ability_BreakPulse) ||
        ActivationOwnedTags.Num() != 1 ||
        !ActivationOwnedTags.HasTagExact(ETGameplayTags::State_CastingBreakPulse) ||
        ActivationBlockedTags.Num() != UE_ARRAY_COUNT(States) ||
        BlockAbilitiesWithTag.Num() != UE_ARRAY_COUNT(Actions) ||
        CancelAbilitiesWithTag.Num() != 0 || !FMath::IsFinite(CooldownSeconds) ||
        CooldownSeconds < .1f || CooldownSeconds > 60.0f ||
        !FMath::IsFinite(CastMaxHorizontalSpeed) || CastMaxHorizontalSpeed < 0.0f ||
        CastMaxHorizontalSpeed > 600.0f || !IsValid(CastMontage) ||
        !IsValid(CastMontage->GetSkeleton()) || CastMontage->HasRootMotion() ||
        !FMath::IsNearlyEqual(CastMontage->RateScale, 1.0f) || !CastMontage->bEnableAutoBlendOut ||
        CastMontage->SlotAnimTracks.Num() != 1 ||
        CastMontage->SlotAnimTracks[0].SlotName != FName(TEXT("DefaultSlot")) ||
        CastMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num() != 1 ||
        CastMontage->CompositeSections.Num() != 1 ||
        !CastMontage->CompositeSections[0].NextSectionName.IsNone() ||
        CastMontage->Notifies.Num() != 1 || !FMath::IsFinite(PulseRadius) || PulseRadius < 1.0f ||
        PulseRadius > 3000.0f || !FMath::IsFinite(PoiseDamage) || PoiseDamage < 1.0f ||
        PoiseDamage > 1000.0f)
    {
        return false;
    }
    for (const FGameplayTag& Tag : States)
    {
        if (!ActivationBlockedTags.HasTagExact(Tag))
        {
            return false;
        }
    }
    for (const FGameplayTag& Tag : Actions)
    {
        if (!BlockAbilitiesWithTag.HasTagExact(Tag))
        {
            return false;
        }
    }
    const FAnimSegment& Segment = CastMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
    const UAnimSequenceBase* Clip = Segment.GetAnimReference();
    const FAnimNotifyEvent& Event = CastMontage->Notifies[0];
    const float CommitTime = Event.GetTime();
    const float FadeIn = CastMontage->BlendIn.GetBlendTime();
    const float FadeOut = CastMontage->BlendOut.GetBlendTime();
    return IsValid(Clip) && Clip->GetSkeleton() == CastMontage->GetSkeleton() &&
           Clip->Notifies.Num() == 0 && !Clip->HasRootMotion() &&
           FMath::IsNearlyEqual(Clip->RateScale, 1.0f) && Segment.LoopingCount == 1 &&
           FMath::IsNearlyZero(Segment.StartPos) && FMath::IsNearlyZero(Segment.AnimStartTime) &&
           FMath::IsNearlyEqual(Segment.AnimPlayRate, 1.0f) &&
           FMath::IsNearlyEqual(Segment.AnimEndTime, Clip->GetPlayLength(), .001f) &&
           FMath::IsFinite(CastMontage->GetPlayLength()) && CastMontage->GetPlayLength() > .2f &&
           IsValid(Event.Notify) && Event.Notify->IsA<UETAN_BreakPulseCommit>() &&
           !Event.NotifyStateClass && Event.IsBranchingPoint() &&
           FMath::IsNearlyEqual(Event.NotifyTriggerChance, 1.0f) &&
           Event.NotifyFilterType == ENotifyFilterType::NoFiltering && FMath::IsFinite(FadeIn) &&
           FadeIn >= 0.0f && FMath::IsFinite(FadeOut) && FadeOut >= 0.0f &&
           FMath::IsFinite(CommitTime) && CommitTime > FMath::Max(.05f, FadeIn) &&
           CommitTime < CastMontage->GetPlayLength() - FadeOut;
}

bool UETGA_BreakPulse::ReadContext(const FGameplayAbilityActorInfo* ActorInfo,
                                   AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC) const
{
    Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    APlayerController* PC =
        IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    const UCharacterMovementComponent* Move =
        IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice =
        IsValid(Body) ? Body->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    return IsValid(Body) && !Body->IsActorBeingDestroyed() && Body->HasAuthority() &&
           Body->IsGameplayReady() && !Body->IsCombatDead() &&
           Body->GetCombatTeam() == EETCombatTeam::Player && IsValid(ASC) &&
           ActorInfo->OwnerActor.Get() == Body && Body->GetAbilitySystemComponent() == ASC &&
           Body->GetWorld() && Body->GetWorld()->GetNetMode() == NM_Standalone &&
           !Body->GetWorld()->IsPaused() && IsValid(PC) && PC->IsLocalController() &&
           PC->GetPawn() == Body && !PC->IsMoveInputIgnored() && IsValid(Move) &&
           Move->IsMovingOnGround() && !Move->HasRootMotionSources() &&
           !Body->GetVelocity().ContainsNaN() &&
           Body->GetVelocity().SizeSquared2D() <= FMath::Square(CastMaxHorizontalSpeed) &&
           (!Practice || Practice->IsPracticeActionIdle()) && IsValid(ActorInfo->GetAnimInstance());
}

bool UETGA_BreakPulse::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayTagContainer* SourceTags,
                                          const FGameplayTagContainer* TargetTags,
                                          FGameplayTagContainer* OptionalRelevantTags) const
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo, Body, ASC) ||
        !Body->IsFormalActionIdle() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    const USkeletalMeshComponent* Mesh = Anim->GetSkelMeshComponent();
    return !Anim->GetCurrentActiveMontage() && IsValid(Mesh) &&
           IsValid(Mesh->GetSkeletalMeshAsset()) &&
           Mesh->GetSkeletalMeshAsset()->GetSkeleton() == CastMontage->GetSkeleton();
}

// ---- 资格检查不消费；真实Duration GE成功应用是本次唯一冷却提交点 ----
void UETGA_BreakPulse::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    if (!IsActive())
    {
        return;
    }
    const uint32 Serial = ++ActivationSerial;
    bEnding = false;
    bStarting = true;
    bCommitAttempted = false;
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo, Body, ASC) ||
        ActorInfo->GetAnimInstance()->GetCurrentActiveMontage() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    const float Duration = CooldownSeconds;
    const FString BodyName = Body->GetName();
    ActiveMontage = CastMontage;
    ActiveCommitToken = ActiveMontage->Notifies[0].Notify;
    ActiveCommitTime = ActiveMontage->Notifies[0].GetTime();
    ActiveRadius = PulseRadius;
    ActivePoiseDamage = PoiseDamage;
    UAbilityTask_PlayMontageAndWait* Task =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, ActiveMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!Task)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask = Task;
    Task->OnCompleted.AddDynamic(this, &ThisClass::HandleCompleted);
    Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleAborted);
    Task->OnCancelled.AddDynamic(this, &ThisClass::HandleAborted);
    // 内置Cost / Cooldown Class保持None，避免CommitAbility额外应用第二份GE。
    const bool bFrameworkCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
    if (!IsActive() || Serial != ActivationSerial || bEnding)
    {
        return;
    }
    if (!bFrameworkCommitted || !ReadContext(ActorInfo, Body, ASC) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
        UETBreakPulseCooldownEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
    if (!Spec.IsValid())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Spec.Data->SetDuration(Duration, true);
    const FActiveGameplayEffectHandle Cooldown =
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    const bool bApplied = Cooldown.IsValid() && Cooldown.WasSuccessfullyApplied();
    UE_LOG(LogETBreakPulse, Display,
           TEXT("[M2-05C2-01] Cooldown Body=%s Serial=%u Applied=%d Duration=%.2f"), *BodyName,
           Serial, bApplied ? 1 : 0, Duration);
    if (!IsActive() || Serial != ActivationSerial || bEnding)
    {
        return;
    }
    if (!bApplied || !ReadContext(ActorInfo, Body, ASC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    Body->GetCharacterMovement()->StopMovementImmediately();
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC = ASC;
    const TWeakObjectPtr<UAnimInstance> WeakAnim = ActorInfo->GetAnimInstance();
    UAnimMontage* StartingMontage = ActiveMontage;
    Task->ReadyForActivation();
    ASC = WeakASC.Get();
    UAnimInstance* Anim = WeakAnim.Get();
    if (!IsActive() || Serial != ActivationSerial)
    {
        if (!IsActive() && IsValid(ASC) && ASC->GetAnimatingAbility() == this &&
            ASC->GetCurrentMontage() == StartingMontage)
        {
            ASC->CurrentMontageStop(0.0f);
            if (!IsActive() && ASC->GetAnimatingAbility() == this &&
                ASC->GetCurrentMontage() == StartingMontage)
            {
                ASC->ClearAnimatingAbility(this);
            }
        }
        return;
    }
    FAnimMontageInstance* Instance =
        IsValid(Anim) ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    if (!IsValid(ASC) || !Instance || !Anim->Montage_IsPlaying(ActiveMontage) ||
        ASC->GetAnimatingAbility() != this || MontageTask != Task)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    ActiveMontageInstanceID = Instance->GetInstanceID();
    bStarting = false;
    UE_LOG(LogETBreakPulse, Display, TEXT("[M2-05C2-01] Begin Body=%s Serial=%u MontageID=%d"),
           *BodyName, Serial, ActiveMontageInstanceID);
}

// ---- 生效点只接受当前Mesh / Montage / 实例 / Token，先封口再产生外部回调 ----
bool UETGA_BreakPulse::IsCurrentCast(uint32 Serial) const
{
    if (!IsActive() || bStarting || bEnding || Serial != ActivationSerial || !CurrentActorInfo)
    {
        return false;
    }
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadContext(CurrentActorInfo, Body, ASC) || ASC->GetAnimatingAbility() != this ||
        ASC->GetCurrentMontage() != ActiveMontage)
    {
        return false;
    }
    UAnimInstance* Anim = CurrentActorInfo->GetAnimInstance();
    const FAnimMontageInstance* Instance = Anim->GetMontageInstanceForID(ActiveMontageInstanceID);
    return Instance && Instance->IsActive() && Instance->Montage == ActiveMontage;
}

void UETGA_BreakPulse::HandleBreakPulseCommit(USkeletalMeshComponent* Mesh,
                                              UAnimSequenceBase* Animation, int32 MontageInstanceID,
                                              UAnimNotify* NotifyToken)
{
    const uint32 Serial = ActivationSerial;
    if (bCommitAttempted || !IsCurrentCast(Serial) || Animation != ActiveMontage ||
        MontageInstanceID != ActiveMontageInstanceID || !IsValid(NotifyToken) ||
        ActiveCommitToken.Get() != NotifyToken)
    {
        return;
    }
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    const FAnimMontageInstance* Instance =
        CurrentActorInfo->GetAnimInstance()->GetMontageInstanceForID(MontageInstanceID);
    if (Mesh != Body->GetMesh() || !Instance ||
        Instance->GetPosition() + KINDA_SMALL_NUMBER < ActiveCommitTime)
    {
        return;
    }
    // 当前施放即使空放也只尝试一次。Notify不再调用CommitAbility，避免第二次冷却。
    bCommitAttempted = true;
    const FVector Origin = Body->GetActorLocation();
    const float Radius = ActiveRadius;
    const float Damage = ActivePoiseDamage;
    ResolvePulse(Serial, Origin, Radius, Damage);
}

void UETGA_BreakPulse::ResolvePulse(uint32 Serial, const FVector& Origin, float Radius,
                                    float Damage)
{
    if (!IsCurrentCast(Serial) || Origin.ContainsNaN() || !FMath::IsFinite(Radius) ||
        Radius <= 0.0f || !FMath::IsFinite(Damage) || Damage <= 0.0f)
    {
        return;
    }
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    UWorld* World = Body->GetWorld();
    const FString BodyName = Body->GetName();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETBreakPulse), false, Body);
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    // 球只在生效点查询一次；Overlap会包含边缘胶囊，最终另以身体中心距离判定。
    World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, Objects,
                                    FCollisionShape::MakeSphere(Radius), Query);
    TSet<TWeakObjectPtr<AEmberTrialCharacter>> Seen;
    TArray<TWeakObjectPtr<AEmberTrialCharacter>> Candidates;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AEmberTrialCharacter* Target = Cast<AEmberTrialCharacter>(Overlap.GetActor());
        if (IsValid(Target) && Target != Body)
        {
            const TWeakObjectPtr<AEmberTrialCharacter> Key(Target);
            if (!Seen.Contains(Key))
            {
                Seen.Add(Key);
                Candidates.Add(Key);
            }
        }
    }
    // 距离近者先结算；发生同步取消时只停止余下目标，不回滚已生效的GE。
    Candidates.Sort(
        [Origin](const TWeakObjectPtr<AEmberTrialCharacter>& Left,
                 const TWeakObjectPtr<AEmberTrialCharacter>& Right)
        {
            const AEmberTrialCharacter* A = Left.Get();
            const AEmberTrialCharacter* B = Right.Get();
            const double DA =
                IsValid(A) ? FVector::DistSquared(Origin, A->GetActorLocation()) : MAX_dbl;
            const double DB =
                IsValid(B) ? FVector::DistSquared(Origin, B->GetActorLocation()) : MAX_dbl;
            return DA < DB;
        });
    UE_LOG(LogETBreakPulse, Display,
           TEXT("[M2-05C2-02] Pulse Body=%s Serial=%u Radius=%.1f Damage=%.1f Candidates=%d"),
           *BodyName, Serial, Radius, Damage, Candidates.Num());
    for (const TWeakObjectPtr<AEmberTrialCharacter>& Candidate : Candidates)
    {
        // GE可同步取消施放 / 销毁身体；每个目标前重查，不持有跨回调的裸来源指针。
        if (!IsCurrentCast(Serial))
        {
            return;
        }
        Body = Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
        UAbilitySystemComponent* SourceASC = CurrentActorInfo->AbilitySystemComponent.Get();
        AEmberTrialCharacter* Target = Candidate.Get();
        if (!IsValid(Target) || !Body->CanDealCombatDamageTo(Target) || Target->IsCombatDead() ||
            !Target->IsGameplayReady() || Target->GetWorld() != Body->GetWorld())
        {
            continue;
        }
        const FVector TargetPoint = Target->GetActorLocation();
        if (TargetPoint.ContainsNaN() ||
            FVector::DistSquared(Origin, TargetPoint) > FMath::Square(Radius))
        {
            continue;
        }
        UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
        float PoiseBefore = 0.0f;
        float MaxPoise = 0.0f;
        float Health = 0.0f;
        float MaxHealth = 0.0f;
        if (!IsValid(TargetASC) || !TargetASC->AbilityActorInfo.IsValid() ||
            TargetASC->AbilityActorInfo->OwnerActor.Get() != Target ||
            TargetASC->AbilityActorInfo->AvatarActor.Get() != Target ||
            !TargetASC->HasAttributeSetForAttribute(UETPoiseAttributeSet::GetPoiseAttribute()) ||
            TargetASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable) ||
            TargetASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned) ||
            !Target->GetGameplayHealthSnapshot(Health, MaxHealth) || !FMath::IsFinite(Health) ||
            !FMath::IsFinite(MaxHealth) || MaxHealth < 1.0f || Health <= 0.0f ||
            Health > MaxHealth || !Target->GetGameplayPoiseSnapshot(PoiseBefore, MaxPoise) ||
            !FMath::IsFinite(PoiseBefore) || !FMath::IsFinite(MaxPoise) || MaxPoise < 1.0f ||
            PoiseBefore <= 0.0f || PoiseBefore > MaxPoise)
        {
            continue;
        }
        FCollisionQueryParams Occlusion(SCENE_QUERY_STAT(ETBreakPulseOcclusion), false, Body);
        Occlusion.AddIgnoredActor(Target);
        FHitResult Block;
        if (Body->GetWorld()->LineTraceSingleByChannel(Block, Origin, TargetPoint, ECC_Visibility,
                                                       Occlusion))
        {
            UE_LOG(LogETBreakPulse, Display,
                   TEXT("[M2-05C2-02] Skip Serial=%u Target=%s Reason=Occluded"), Serial,
                   *Target->GetName());
            continue;
        }
        FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
        if (!Context.IsValid())
        {
            continue;
        }
        Context.AddSourceObject(this);
        FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
            UETPoiseDeltaEffect::StaticClass(), GetAbilityLevel(), Context);
        if (!Spec.IsValid())
        {
            continue;
        }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_PoiseDelta, -Damage);
        const FString TargetName = Target->GetName();
        const TWeakObjectPtr<UAbilitySystemComponent> WeakTargetASC(TargetASC);
        const FActiveGameplayEffectHandle Applied =
            SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
        // Instant GE的IsValid可为false；成功以WasSuccessfullyApplied为准。
        Target = Candidate.Get();
        TargetASC = WeakTargetASC.Get();
        float After = 0.0f;
        float AfterMax = 0.0f;
        const bool bSnapshot = IsValid(Target) && IsValid(TargetASC) &&
                               Target->GetAbilitySystemComponent() == TargetASC &&
                               Target->GetGameplayPoiseSnapshot(After, AfterMax);
        UE_LOG(LogETBreakPulse, Display,
               TEXT("[M2-05C2-02] Target Body=%s Serial=%u Target=%s Applied=%d Poise=%.1f->%.1f "
                    "Snapshot=%d"),
               *BodyName, Serial, *TargetName, Applied.WasSuccessfullyApplied() ? 1 : 0,
               PoiseBefore, bSnapshot ? After : -1.0f, bSnapshot ? 1 : 0);
    }
}

// ---- 只结束自己的任务 / Montage，不移除已提交冷却GE ----
void UETGA_BreakPulse::ReleaseMontage()
{
    const uint32 Serial = ActivationSerial;
    UAbilityTask_PlayMontageAndWait* OldTask = MontageTask;
    UAnimMontage* OldMontage = ActiveMontage;
    const int32 OldID = ActiveMontageInstanceID;
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC =
        CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent
                         : TWeakObjectPtr<UAbilitySystemComponent>();
    const TWeakObjectPtr<UAnimInstance> WeakAnim =
        CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
    UAnimInstance* Anim = WeakAnim.Get();
    FAnimMontageInstance* Instance =
        IsValid(Anim) && OldID != INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    MontageTask = nullptr;
    ActiveMontage = nullptr;
    ActiveMontageInstanceID = INDEX_NONE;
    ActiveCommitToken.Reset();
    ActiveCommitTime = 0.0f;
    ActiveRadius = 0.0f;
    ActivePoiseDamage = 0.0f;
    if (IsValid(OldTask))
    {
        OldTask->OnCompleted.RemoveAll(this);
        OldTask->OnInterrupted.RemoveAll(this);
        OldTask->OnCancelled.RemoveAll(this);
        if (Instance)
        {
            if (Instance->OnMontageBlendedInEnded.IsBoundToObject(OldTask))
            {
                Instance->OnMontageBlendedInEnded.Unbind();
            }
            if (Instance->OnMontageBlendingOutStarted.IsBoundToObject(OldTask))
            {
                Instance->OnMontageBlendingOutStarted.Unbind();
            }
            if (Instance->OnMontageEnded.IsBoundToObject(OldTask))
            {
                Instance->OnMontageEnded.Unbind();
            }
        }
        OldTask->EndTask();
        OldTask->Ability = nullptr;
    }
    UAbilitySystemComponent* ASC = WeakASC.Get();
    Anim = WeakAnim.Get();
    Instance =
        IsValid(Anim) && OldID != INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    if (Serial == ActivationSerial && IsValid(ASC) && IsValid(OldMontage) &&
        ASC->GetAnimatingAbility() == this && ASC->GetCurrentMontage() == OldMontage &&
        (Instance || OldID == INDEX_NONE))
    {
        ASC->CurrentMontageStop(.10f);
        if (Serial == ActivationSerial && ASC->GetAnimatingAbility() == this &&
            ASC->GetCurrentMontage() == OldMontage)
        {
            ASC->ClearAnimatingAbility(this);
        }
    }
}

void UETGA_BreakPulse::HandleCompleted()
{
    if (IsActive() && !bEnding)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
    }
}
void UETGA_BreakPulse::HandleAborted()
{
    if (IsActive() && !bEnding)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
    }
}
void UETGA_BreakPulse::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo) || bEnding)
    {
        return;
    }
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }
    const uint32 EndedSerial = ActivationSerial;
    const bool bAttempted = bCommitAttempted;
    const FString BodyName = GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    bEnding = true;
    bStarting = false;
    ++ActivationSerial;
    ReleaseMontage();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    // 父类结束可能重入新激活；之后只使用局部值记录，不重置实例成员。
    UE_LOG(LogETBreakPulse, Display,
           TEXT("[M2-05C2-01] End Body=%s Serial=%u Cancelled=%d PulseAttempted=%d"), *BodyName,
           EndedSerial, bWasCancelled ? 1 : 0, bAttempted ? 1 : 0);
}
```

### 4.3 需要理解的四处分工

1. HasValidConfiguration从“AM通知空”升级为“唯一ET Break Pulse Commit、在淡入后 / 淡出前、Branching Point”。AS仍无通知；本批正式资源固定12帧 / .4秒并由专项核对，运行规则从Notify资产读取时间，不把.4硬编码到生产校验。将来替换动画可同步调整素材 / 专项契约。先Build再配AM，期间不要把旧AM无通知的校验失败误认为类创建失败。
2. 激活时快照Radius / Damage / Token / 时点；启动完成登记Montage实例ID后才允许提交。结束先退役这些字段，再释放自身Task / Montage，不移除已提交冷却。
3. HandleBreakPulseCommit先验证真实身份与时间，然后把bCommitAttempted置True再进行范围查询。重复 / 旧通知不会二次应用；空放也消耗本次尝试。
4. ResolvePulse只给合法目标逐个创建Instant GE，成功用WasSuccessfullyApplied判定，**不能要求Instant GE的Handle.IsValid也为True**。归零到反应 / 回填由原CombatPoise组件接管，GA不再复制Stagger Timer或直接填Poise。

## 5. 三个既有文件的小修改

### 5.1 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

专项最小friend；找到原位置：

```cpp
    friend class FETBreakPulseAbilityTest;
```

替换为：

```cpp
    friend class FETBreakPulseAbilityTest;
    friend class FETBreakPulseRangeTest;
```

### 5.2 `EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`

校验错误同步新通知契约；找到原位置：

```cpp
TEXT("BreakPulse requires valid native policy, in-place empty-notify montage, "
```

替换为：

```cpp
TEXT("BreakPulse requires valid native policy, one ET Break Pulse Commit outside blending, "
```

### 5.3 `EmberTrial/Tests/ETBreakPulseAbilityTests.cpp`

旧专项断言明确只保护施放者自身；找到原位置：

```cpp
TEXT("C2-01 has no Poise resolution")
```

替换为：

```cpp
TEXT("Cast does not reduce caster Poise")
```

Character的friend只给新的专项读取测试前置配置，不增加生产接口。旧BreakPulseAbility的这个断言始终检查施放者自己Poise100，改文字是为了与本批“范围会扣目标Poise”区分；它仍验证原资源隔离。

## 6. 新增真实通知 / 范围专项

新建`EmberTrial/Tests/ETBreakPulseRangeTests.cpp`，全文如下。它使用已保存GA / C1 Montage和正式训练身体蓝图；原生骨架、真实动画通知、Overlap / 遮挡、GAS与Poise反应都参与运行，不用假的回调替代主结算路径。

14组场景：两目标 / 多形状去重与下一轮归零恢复、边缘中心距离、墙、同阵营、死亡、Waiting、无敌、Stunned、临时进 / 出范围、通知前取消、Pause、GE回调同步取消、EndPlay。重复 / 旧实例 / 过早通知在相关场景附带检查。测试不Destroy后推进再读旧ASC；EndPlay后只检查Fixture仍持有的对象，最终销毁交给Fixture。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGameplayTags.h"
#include "Animation/ETAN_BreakPulseCommit.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETBreakPulseRangeTest, "EmberTrial.M2.Combat.BreakPulseRange",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETBreakPulseRangeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* PulseClass = LoadClass<UETGA_BreakPulse>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse.GA_ET_BreakPulse_C"));
    UClass* TargetClass = LoadClass<AEmberTrialCharacter>(
        nullptr, TEXT("/Game/EmberTrial/Tests/Blueprints/"
                      "BP_ETCombatTrainingTarget.BP_ETCombatTrainingTarget_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved pulse"), PulseClass) ||
        !TestNotNull(TEXT("Saved native training body Blueprint"), TargetClass) ||
        !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_BreakPulse* Defaults = PulseClass->GetDefaultObject<UETGA_BreakPulse>();
    if (!TestTrue(TEXT("Saved one-notify config"), Defaults->HasValidConfiguration()) ||
        !TestEqual(TEXT("Saved radius"), Defaults->PulseRadius, 300.0f) ||
        !TestEqual(TEXT("Saved Poise damage"), Defaults->PoiseDamage, 60.0f))
    {
        return false;
    }
    TestTrue(TEXT("Formal asset commit point remains .4 seconds"),
             FMath::IsNearlyEqual(Defaults->CastMontage->Notifies[0].GetTime(), .4f, .001f));
    enum class EScenario : uint8
    {
        Cycle,
        Edge,
        Wall,
        Team,
        Dead,
        Waiting,
        Invulnerable,
        Stunned,
        EnterRange,
        LeaveRange,
        CancelBefore,
        Pause,
        ReentrantCancel,
        EndPlay
    };
    for (const EScenario Scenario :
         {EScenario::Cycle, EScenario::Edge, EScenario::Wall, EScenario::Team, EScenario::Dead,
          EScenario::Waiting, EScenario::Invulnerable, EScenario::Stunned, EScenario::EnterRange,
          EScenario::LeaveRange, EScenario::CancelBefore, EScenario::Pause,
          EScenario::ReentrantCancel, EScenario::EndPlay})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = Fixture.GetTestWorld();
        AActor* Floor = World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Floor"), Floor))
        {
            return false;
        }
        UBoxComponent* FloorBox = NewObject<UBoxComponent>(Floor);
        Floor->SetRootComponent(FloorBox);
        FloorBox->SetBoxExtent(FVector(2000, 2000, 20));
        FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FloorBox->SetCollisionObjectType(ECC_WorldStatic);
        FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
        FloorBox->RegisterComponent();
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Source = World->SpawnActor<AEmberTrialCharacter>(
            FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        AEmberTrialCharacter* A = World->SpawnActor<AEmberTrialCharacter>(
            TargetClass, FVector(180, 0, 200), FRotator::ZeroRotator, Spawn);
        AEmberTrialCharacter* B = World->SpawnActor<AEmberTrialCharacter>(
            TargetClass, FVector(0, 220, 200), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Source"), Source) || !TestNotNull(TEXT("PC"), PC) ||
            !TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B))
        {
            return false;
        }
        Source->CombatTeam = EETCombatTeam::Player;
        Source->BreakPulseAbilityClass = PulseClass;
        Source->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Source->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Source->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Source->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Source->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        for (AEmberTrialCharacter* Body : {Source, A, B})
        {
            Body->AutoPossessAI = EAutoPossessAI::Disabled;
            Body->GetCharacterMovement()->bRunPhysicsWithNoController = true;
            Body->GetMesh()->VisibilityBasedAnimTickOption =
                EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        }
        PC->Possess(Source);
        if (!Fixture.BeginPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        auto Advance = [&Fixture](float Seconds)
        {
            for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
            {
                if (!Fixture.TickTestWorld(.01f))
                {
                    return false;
                }
            }
            return true;
        };
        for (AEmberTrialCharacter* Body : {Source, A, B})
        {
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        }
        if (!Advance(1.0f) ||
            !TestTrue(TEXT("Source grounded"), Source->GetCharacterMovement()->IsMovingOnGround()))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Source->GetAbilitySystemComponent();
        UAbilitySystemComponent* ASC_A = A->GetAbilitySystemComponent();
        UAbilitySystemComponent* ASC_B = B->GetAbilitySystemComponent();
        if (!TestTrue(TEXT("Formal target A ready"), A->IsGameplayReady()) ||
            !TestTrue(TEXT("Formal target B ready"), B->IsGameplayReady()))
        {
            return false;
        }
        const FGameplayAbilitySpecHandle Handle = Source->BreakPulseAbilityHandle;
        FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
        UETGA_BreakPulse* Ability =
            Spec ? Cast<UETGA_BreakPulse>(Spec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Pulse instance"), Ability))
        {
            return false;
        }
        auto Poise = [](UAbilitySystemComponent* Comp)
        { return Comp->GetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute()); };
        auto Place = [Source](AEmberTrialCharacter* Target, const FVector& Offset)
        {
            Target->GetCharacterMovement()->StopMovementImmediately();
            Target->SetActorLocation(Source->GetActorLocation() + Offset, false);
        };
        // 除Cycle / 重入两组外，B移出范围，避免用例互相掩盖。
        if (Scenario != EScenario::Cycle && Scenario != EScenario::ReentrantCancel)
        {
            Place(B, FVector(700, 500, 0));
        }
        if (Scenario == EScenario::Edge)
        {
            Place(A, FVector(315, 0, 0));
        }
        if (Scenario == EScenario::EnterRange)
        {
            Place(A, FVector(400, 0, 0));
        }
        if (Scenario == EScenario::Team)
        {
            A->CombatTeam = EETCombatTeam::Player;
        }
        if (Scenario == EScenario::Dead)
        {
            ASC_A->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
        }
        if (Scenario == EScenario::Waiting)
        {
            A->SetGameplayReady(false);
        }
        if (Scenario == EScenario::Invulnerable)
        {
            ASC_A->AddLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (Scenario == EScenario::Stunned)
        {
            ASC_A->AddLooseGameplayTag(ETGameplayTags::State_Stunned);
        }
        if (Scenario == EScenario::Wall)
        {
            AActor* Wall = World->SpawnActor<AActor>();
            if (!TestNotNull(TEXT("Wall"), Wall))
            {
                return false;
            }
            UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
            Wall->SetRootComponent(WallBox);
            WallBox->SetBoxExtent(FVector(8, 120, 180));
            WallBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            WallBox->SetCollisionObjectType(ECC_WorldStatic);
            WallBox->SetCollisionResponseToAllChannels(ECR_Block);
            WallBox->RegisterComponent();
            Wall->SetActorLocation(Source->GetActorLocation() + FVector(90, 0, 0));
        }
        if (Scenario == EScenario::Cycle)
        {
            // 第二份Pawn查询形状必须去重，不能让一名目标一次扣120。
            UBoxComponent* Duplicate = NewObject<UBoxComponent>(A);
            Duplicate->SetupAttachment(A->GetRootComponent());
            Duplicate->SetBoxExtent(FVector(25));
            Duplicate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Duplicate->SetCollisionObjectType(ECC_Pawn);
            Duplicate->SetCollisionResponseToAllChannels(ECR_Overlap);
            Duplicate->RegisterComponent();
        }
        FDelegateHandle Reentrant;
        if (Scenario == EScenario::ReentrantCancel)
        {
            const TWeakObjectPtr<UAbilitySystemComponent> WeakSource(ASC);
            Reentrant = ASC_A
                            ->GetGameplayAttributeValueChangeDelegate(
                                UETPoiseAttributeSet::GetPoiseAttribute())
                            .AddLambda(
                                [WeakSource, Handle](const FOnAttributeChangeData& Data)
                                {
                                    if (Data.NewValue < Data.OldValue && WeakSource.IsValid())
                                    {
                                        WeakSource->CancelAbilityHandle(Handle);
                                    }
                                });
        }
        if (!TestTrue(TEXT("Formal F request accepted"), Source->RequestBreakPulse()))
        {
            return false;
        }
        UAnimMontage* OldMontage = Ability->ActiveMontage;
        UAnimNotify* OldToken = Ability->ActiveCommitToken.Get();
        const int32 OldID = Ability->ActiveMontageInstanceID;
        if (!Advance(.1f))
        {
            return false;
        }
        TestEqual(TEXT("No pre-notify damage"), Poise(ASC_A), 100.0f);
        Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
        TestFalse(TEXT("Early same-identity callback does not seal"), Ability->bCommitAttempted);
        if (Scenario == EScenario::EndPlay)
        {
            if (!Fixture.EndPlayInTestWorld())
            {
                return false;
            }
            TestFalse(TEXT("EndPlay ends cast"), Ability->IsActive());
            TestEqual(TEXT("EndPlay before notify leaves target untouched"), Poise(ASC_A), 100.0f);
            continue;
        }
        if (Scenario == EScenario::CancelBefore)
        {
            ASC->CancelAbilityHandle(Handle);
        }
        if (Scenario == EScenario::Pause)
        {
            APlayerState* Pauser = PC->PlayerState;
            if (!IsValid(Pauser))
            {
                Pauser = World->SpawnActor<APlayerState>();
            }
            if (!TestNotNull(TEXT("Pauser"), Pauser))
            {
                return false;
            }
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            if (!Advance(1.0f))
            {
                return false;
            }
            TestEqual(TEXT("Pause does not deliver commit"), Poise(ASC_A), 100.0f);
            TestFalse(TEXT("Pause does not seal commit"), Ability->bCommitAttempted);
            World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        }
        if (Scenario == EScenario::LeaveRange)
        {
            Place(A, FVector(400, 0, 0));
        }
        if (Scenario == EScenario::EnterRange)
        {
            Place(A, FVector(180, 0, 0));
        }
        if (!Advance(.45f))
        {
            return false;
        }
        const bool bRejectedTarget =
            Scenario == EScenario::Edge || Scenario == EScenario::Wall ||
            Scenario == EScenario::Team || Scenario == EScenario::Dead ||
            Scenario == EScenario::Waiting || Scenario == EScenario::Invulnerable ||
            Scenario == EScenario::Stunned || Scenario == EScenario::LeaveRange ||
            Scenario == EScenario::CancelBefore;
        TestEqual(TEXT("Real notify applies or skips by current target policy"), Poise(ASC_A),
                  bRejectedTarget ? 100.0f : 40.0f);
        Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
        TestEqual(TEXT("Duplicate or cancelled callback cannot add damage"), Poise(ASC_A),
                  bRejectedTarget ? 100.0f : 40.0f);
        TestEqual(TEXT("Poise does not damage source health"),
                  ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                  100.0f);
        if (Scenario != EScenario::Dead)
        {
            TestEqual(TEXT("Poise does not damage target health"),
                      ASC_A->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                      100.0f);
        }
        TestEqual(TEXT("One accepted-cast cooldown only"),
                  ASC->GetTagCount(ETGameplayTags::Cooldown_BreakPulse), 1);
        if (Scenario == EScenario::ReentrantCancel)
        {
            TestFalse(TEXT("GE callback really cancels source"), Ability->IsActive());
            TestEqual(TEXT("Cancelled source stops remaining candidates"), Poise(ASC_B), 100.0f);
            ASC_A
                ->GetGameplayAttributeValueChangeDelegate(UETPoiseAttributeSet::GetPoiseAttribute())
                .Remove(Reentrant);
        }
        if (Scenario == EScenario::Cycle || Scenario == EScenario::CancelBefore)
        {
            if (Scenario == EScenario::Cycle)
            {
                TestEqual(TEXT("Both targets once"), Poise(ASC_B), 40.0f);
            }
            if (!Advance(6.0f) || !TestTrue(TEXT("Next cast accepted after real cooldown"),
                                            Source->RequestBreakPulse()))
            {
                return false;
            }
            Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
            TestFalse(TEXT("Old montage ID cannot seal a new activation"),
                      Ability->bCommitAttempted);
            if (!Advance(.55f))
            {
                return false;
            }
            TestEqual(TEXT("New activation resolves independently"), Poise(ASC_A),
                      Scenario == EScenario::Cycle ? 0.0f : 40.0f);
            if (Scenario == EScenario::Cycle)
            {
                TestTrue(TEXT("Zero Poise triggers existing formal Stagger"),
                         ASC_A->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
                TestEqual(TEXT("Second target also reaches zero"), Poise(ASC_B), 0.0f);
                if (!Advance(1.6f))
                {
                    return false;
                }
                TestFalse(TEXT("Reaction exits"),
                          ASC_A->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
                TestEqual(TEXT("Existing reaction restores current maximum"), Poise(ASC_A), 100.0f);
            }
        }
        if (Scenario == EScenario::Invulnerable)
        {
            ASC_A->RemoveLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (Scenario == EScenario::Stunned)
        {
            ASC_A->RemoveLooseGameplayTag(ETGameplayTags::State_Stunned);
        }
        // 不Destroy后推进再读旧ASC；EndPlay返回时对象仍由Fixture世界持有。
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("No stale cast after EndPlay"), Ability->IsActive());
        TestFalse(TEXT("No casting tag after EndPlay"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse));
    }
    return !HasAnyErrors();
}
#endif
```

## 7. 正常Editor / 索引 / Game Build

UE关闭、八份Source全部保存后，PowerShell逐项执行，前项失败就停止：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

CLion Reload CMake Project；预期**139 Source / 75cpp，Core55 / UI17 / Editor3**。原17项自动化保留，新Range使总数18。新增头generated.h仍必须最后include；文件名 / 类名 / friend / 测试宏名按全文，不给函数参数取AController已有成员名。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标成功后打开工程，先做第8节资产，再校验 / 自动化；这时AM还没有新Notify，不要提前跑专项。代码已按本机UE5.5.4接口静态核对，助手未实际Build或运行，不能把文本准备写成Success。

## 8. UE资产配置（只改施放Montage与GA配置）

### 8.1 AM：第12帧唯一点通知

1. 关闭Sequencer，打开**`/Game/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01`**，目录是Montage单数；核对标题不是旧HitResult或Heal。唯一AS仍AS_ET_BreakPulse_01，DefaultSlot / Loop1 / Rate1 / Blend .10-.10 / Auto Blend Out=True。
2. 时间轴找到**Notifies组下面的一条子轨道**（常叫1）。如无子轨道，用Notifies旁+ / Add Track → Add Notify Track，只添加轨道，不添加Notify State。
3. 在该子轨道空白处右键 → **Add Notify → ET Break Pulse Commit**（原生类ETAN_BreakPulseCommit）。不是Add Notify State，不用New Notify，不在Skeleton上建名字相似的事件。
4. 新增后应为一个通知点 / 标签，没有持续状态条。**右键通知点本体**，在菜单的**Notify Begin Time**输入`0.400000`，按Enter提交；再右键核对Notify Frame=**12**（30fps）。时间数值.4秒是最终依据，不靠拖拽估计，也不去普通Details找Start / Duration。
5. 左键选中通知点，Details搜索Montage Tick Type；可见时设**Branching Point**。原生类已强制分支点，该字段不显示时核对类名，不造替代通知。Trigger Chance=1 / 100%，Notify Filter Type=No Filtering，其他触发项保留默认。
6. AM Notifies仅这一条；原AS / LS不加通知。Save，关闭重开AM确认通知仍在12帧 / .4秒。预览没有Pulse业务日志正常，因为本类禁止编辑器预览结算。
7. 本步骤沿用已解决的Heal通知菜单操作，并核对了本机SAnimNotifyPanel源码。若选中后看不到右键时间字段，截图同时含标题、通知点和右键菜单；不要搜索空白轨道的右键菜单。

### 8.2 GA_ET_BreakPulse：两个新可调值

1. 打开`/Game/EmberTrial/Abilities/GA_ET_BreakPulse` → Class Defaults，清空Details搜索再搜BreakPulse / Break Pulse。
2. **Pulse Radius=300.0cm，Poise Damage=60.0**。原Cast Montage仍第8.1节的AM，Cooldown Seconds=6.0，Cast Max Horizontal Speed=3.0。
3. 内置Cost GameplayEffect Class / Cooldown GameplayEffect Class继续**None**；Tags与原生F绑定保持C2-01配置，EventGraph空。不要新增Apply GE或Play Montage节点。
4. Compile → Save。已有玩家Break Pulse Class / Action保持，不需要新增组件或给敌人 / 训练目标装备F技能。原GA_ET_Stagger继续Duration1.2 / 旧HitResult受害反应；**不能把新的前推AM赋给Stagger**。
5. 训练 / 敌人胶囊原查询与Pawn类型、Mesh NoCollision保持；原手掌Trace窗口与F范围独立。不要扩大近战TraceRadius来调整300cm的F范围，不改Life伤害25 / 10或Poise出生100。

## 9. 配置校验与三项自动化

1. Content Browser选GA_ET_BreakPulse → 右键Asset Actions → Validate Assets，应有本项目`[M1-07C2] Valid / Issues=0`。
2. ET-BREAK-001先查AM唯一通知在淡入后 / 淡出前、Branching Point / AS无通知、新范围 / 数值合法、原策略 / Cost-Cooldown None；本批具体.4秒由新专项的正式资产断言核对。不要把通知删掉来恢复旧的“空通知”规则；本批已同时升级原生校验。
3. Stop PIE，Output Log逐条运行以下三项，上一项Success再下一项：

```text
Automation RunTests EmberTrial.M2.Combat.BreakPulseRange
Automation RunTests EmberTrial.M2.Combat.BreakPulseAbility
Automation RunTests EmberTrial.M2.Combat.PoiseReaction
```

新Range覆盖通知 / 真实查询 / 目标策略 / 去重 / 取消；旧Ability覆盖冷却与施放事务；旧Reaction覆盖归零 / 回填和原生命周期。不改闪避 / 治疗 / HUD实现，不机械跑全部18项。失败先发首条断言和本轮时间，不继续勾后面的实机通过；闪退保留Saved/Crashes，不反复撞同一错误。

## 10. 实机只查四组（约8–12分钟）

**准备：**L_ET_VerticalSlice、正式默认身体 / 单训练目标TrainingTarget_A / 单敌人，slomo1，Debug False。Output Log搜索M2-05C2-02能看Pulse / Target；若要看归零反应，搜索Stagger / Poise，或清空过滤。键鼠操作不用手掐12帧，精确时间与各类拒绝已由自动化覆盖。

### 10.1 训练目标两次F与资源隔离（约3分钟）

1. TrainingTarget_A是**BP_ETCombatTrainingTarget**战斗身体，**不是按E激活的M1_Target_A / BP_ETPracticeActor**。在其旁边约1–2米站定，不先左键打死它。
2. 打开控制台输入`ke BP_ETCombatTrainingTarget_C ET_Poise 0`可查询训练目标；关闭控制台恢复游戏焦点。新PIE默认应100/100，查询不会修改。
3. 按F，前推到通知时应一条`Pulse ... Radius=300 Damage=60`，指向该目标的一条`Target ... Applied=1 Poise=100.0->40.0`；玩家生命 / 体力 / 次数HUD不因F变化。
4. 等冷却约7秒，再按F。目标40→0并播放旧受害占位反应，约1.2游戏秒结束后回100。查日志Depleted / Stagger / Restore，不要求复制全部日志；一次F不能出现同目标两条真实扣除。
5. 若敌人干扰施放，F被受击取消是预期。需要隔离时Stop，**只临时把场景MeleeEnemy_A的Auto Possess AI设Disabled**，不改敌人Blueprint Class Defaults；记录原Placed in World or Spawned，先不要保存地图，训练短查后Stop立即恢复原值，再做后续交战与最终Save All。无需搭任何Controller图。

### 10.2 空放、取消与真实交战观察（约2–3分钟）

1. 离开训练目标中心300cm外、避免其他身体进入范围，冷却结束新按F。应有一次Pulse但无合法Target Applied，仍进入6秒冷却；不持续扫描追着目标补结算。
2. F后立即按3：出现取消，冷却保留；通知前取消本次不应有Pulse。无需反复卡秒，若手动刚好过了通知点，只据日志如实记录，前后界限由新专项覆盖。
3. 敌人AI恢复正常后可在一次挥空后的空档F，观察真实敌人Poise下降 / 归零反应与随后继续追击；若被敌人打断，也应只取消本次、保留冷却。**不要求为了验收精确撞某一帧或强行无伤连放两次**。稳定Poise流程由10.1与自动化作为必查；实际交战结果按发生情况反馈。

### 10.3 暂停与本轮结束（约2分钟）

1. 等冷却结束，F后立刻P，暂停现实约2秒再以一种方式继续。通知 / 冷却不在Pause中推进，菜单内F不补播，恢复后一次结算且不漂移 / 卡键。
2. F中Stop一次，再Play，F重新可用、训练目标与玩家Poise100；旧Pulse / Target不能继续写新世界。这里不重做Retry / Continue / 多Pawn / Widget十次重建矩阵，这些所有者本批未改。

### 10.4 恢复与最后短基线（约1–2分钟）

1. 若临时改过MeleeEnemy_A实例Auto Possess AI，Stop后恢复**Placed in World or Spawned**；重开确认敌人能正常感知 / 追击。没有改过可填“未改”。保留单训练 / 单敌人，不加第二目标或墙体专门重跑自动化矩阵。
2. 左键仍真实25、2正常位移与扣25、4受伤后Q一次40 / 减1、E与P一种继续方式、移动 / 镜头 / 跳跃简查。
3. Stop → Save All：GA6 / 3 / 300 / 60、AM唯一.4秒通知、Stagger1.2与旧AM、双100 / 3次 / Poise100、slomo1 / Debug False；默认地图 / 原数值不乱改。地图只有恢复后且确有差异时才提交，不保存临时禁用AI。

## 11. 完成反馈与问题提交

```text
M2-05C2-02 完成
三新 / 五旧Source；关闭UE后Editor Build / Core55-UI17-Editor3索引 / Game Build：
AM唯一ET Break Pulse Commit在12帧/.4秒、AS无Notify；GA300/60/6/3与原配置；Compile-Save-Valid：
BreakPulseRange / BreakPulseAbility / PoiseReaction各一次：
训练目标两次F100→40→0 / 一次反应后100；玩家资源隔离：
空放冷却 / 取消不退款 / P冻结与无补播；真实交战看到的结果：
F中Stop再开 / 原能力-E-P与移动跳跃短基线：
敌人实例AI已恢复或未改；Source资产Save All、slomo1 / Debug False：
失败或最明显占位问题（无填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

编译 / 自动化用文字贴首条Error或断言、文件行号与前后约20行；通知配置用截图含AM标题 / 通知点 / 右键菜单或Details；时序难描述给5–10秒视频和按键先后。给完成时分即可，我读本地日志；不用抄每行成功日志，也不让你精准掐测试时点。正常开发由你输入代码；直接接手仍需你明确要求。

通过后保存报告，同轮进入D的最小冷却反馈与模块收口；本批没有新Cook / 性能采样。当前139Source / 75cpp / 18测试仅是**实施后的预期**，实际工程仍C2-01的136 / 73 / 17。

## 12. 本批 Commit / Push（固定放在最后）

**实施且验收通过后**Stage以下三新 / 五旧Source（前缀EmberTrial/Source/）：

```text
EmberTrial/Public/Animation/ETAN_BreakPulseCommit.h
EmberTrial/Private/Animation/ETAN_BreakPulseCommit.cpp
EmberTrial/Tests/ETBreakPulseRangeTests.cpp
EmberTrial/Public/Abilities/ETGA_BreakPulse.h
EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp
EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp
EmberTrial/Tests/ETBreakPulseAbilityTests.cpp
```

**UE资产**：

```text
EmberTrial/Content/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01.uasset
EmberTrial/Content/EmberTrial/Abilities/GA_ET_BreakPulse.uasset
```

AM应有新通知差异；GA完成Compile / Save，只有实际出现差异时Stage，原生继承的新默认值不要求人为制造资产变化。C1 LS / AS、Skeleton / Rig、旧Stagger / Hit / Body / 输入均无计划改动；临时禁用AI不能入库，地图恢复后如无差异就不Stage。

**文档**：本指南、实施后生成的`Docs/Reports/M2_05C2_02_Verification.md`、来源清单中AM / GA实际哈希与用途、Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / 两份路线 / README / Introduction最新收尾与下一批指南。若当前C2-01收尾尚未提交，一起包含；来源仍70项，不为原生Notify的源码虚增UE资产行。

建议Commit：

```text
feat: resolve break pulse poise damage from montage notify
```

CLion刷新Changes → Stage最新变化（先Stage后又修改的文件要**再Stage**）→ 检查完整Source、AM真实Notify配置 / LFS与正式值 → Commit → Push。现在尚不存在的新Source / 后续报告不要提前创建空文件提交；Raw / 日志 / 崩溃转储 / Binaries / Intermediate / Saved / IDE生成物与包体留本地。用户操作Git，助手未改索引、Commit或联网核验远端。
