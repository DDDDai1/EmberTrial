# M2-05C2-01：F施放、真实冷却与动作优先级

2026-10-23，UE5.5.4。前置[C1动画素材已验收](Reports/M2_05C1_Verification.md)，无待补项。**[本章已验收](Reports/M2_05C2_01_Verification.md)**，用户14:48反馈全项完成，手动70 / 等待5分钟；以下保留完整手动实施指南，五新 / 八旧共13份Source的代码在下文；助手没有写入实际Source或创建UE资产，也未代Build / 自动化 / PIE / Git操作。

先完成F的站定施放、6游戏秒真实冷却和中断清理；C2-02再接第12帧的唯一原生Notify、范围 / 阵营 / 遮挡和Poise结算。本批F播放完成却不扣敌人韧性是预期；不添加临时按键播放图。预计手动80–130分钟，含代码40–70、资源配置20–40、检查8–12；构建 / 索引等待另记。

## 1. 本批规则与最终效果

| 条件 / 操作 | 预期 |
| --- | --- |
| 玩家Ready、存活、地面、水平速度≤3cm/s、没有其他动作 / Montage或冷却 | 新按F接受一次，播放C1前推动作 |
| 持续按F、忙碌 / 冷却中重复按、空中 / 移动 / 暂停中按 | 拒绝，不缓存、不补播，不刷新冷却 |
| 接受施放 | 原生Duration GE授予Cooldown.BreakPulse；时长来自GA的6秒配置 |
| 动画约1秒结束 | 行动恢复；余下冷却继续。6秒从接受时算起，不从动画结束重算 |
| 普通受击、Stagger或死亡 | 立即取消施放并释放自身Tag / Task / Montage；已提交冷却不退款 |
| 施放中左键 / 2 / Q / E / 移动 / 跳跃 | 阻断；镜头及P保留。P恢复后不补执行菜单内F |
| 空放 / 无目标 | 本批均为空放，正常进入冷却；生命 / 体力 / 治疗次数 / Poise不变化 |
| Pause / Retry / Continue | Pause冻结当前世界冷却；新身体具有独立ASC，不继承旧冷却 |

新增`Ability.BreakPulse`、`State.Casting.BreakPulse`、`Cooldown.BreakPulse`三Tag。Casting是动作状态，Cooldown是ASC活动GE的状态，不能用一个“忙碌”布尔值代替二者。HitReact取消Casting，但不能把Casting加入HitReact的阻断名单，否则真实伤害不能中断它。

默认6秒、速度阈值3为GA可调参数；原生GE的1秒CDO值只是合法结构占位，每次Spec以SetDuration覆盖。**内置Cost GameplayEffect Class与Cooldown GameplayEffect Class均保持None**：CommitAbility只做框架资格检查，再明确检查一次真实GE应用结果，避免内置冷却与自定义冷却重复应用。GetCooldownTags重写后，UE5.5的CheckCooldown和剩余时间查询仍能按原生冷却Tag工作。GE成功后才启动动画；运行时动画意外失败也不退已提交冷却。

不新增常驻Tick、冷却轮询Timer、UI或Niagara；冷却到期由GAS / 世界时间管理。C1的`Montage`目录用**单数**，沿用真实有效路径，不为目录名称移动已提交资产。

## 2. 操作前准备与目录

1. Stop PIE → Save All。保存并**关闭UE编辑器**，保持CLion打开；先不Live Coding。本批有UCLASS / 反射字段，必须正常UBT / UHT构建。
2. CLion打开仓库根工程。确认现有`Private/Abilities/Effects`已存在，新的头文件放`Public/Abilities/Effects`；不要再次把Effects建到Public / Private直接根目录。现有恢复组件目录是`Resources`复数。
3. 下文每个路径均相对`E:\UE_Project\Demo\EmberTrial\Source`。新建普通.h / .cpp文件并粘贴完整代码；若通过UE创建类，须关闭编辑器后用下文完整内容替换模板，避免遗留重复GENERATED_BODY / .generated.h。
4. 不改Build.cs / Target / uproject；既有GAS、EnhancedInput和Editor依赖足够。不要把新Tests文件移动到Content，或给测试文件添加到Runtime蓝图资源。
5. 本次不需要先做Commit；所有检查通过、正式值恢复并保存后，按最后清单一次Stage最新内容。

## 3. 新增施放与冷却四个文件

### 3.1 文件列表

按下列顺序创建。每个头文件的generated.h必须是**最后一条include**；不要把generated.h放到第1行或漏掉。

### 3.2 `EmberTrial/Public/Abilities/Effects/ETBreakPulseCooldownEffect.h`

全新文件，完整内容如下。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETBreakPulseCooldownEffect.generated.h"

// 固定结构只授予Cooldown.BreakPulse；每次Spec的时长来自施放GA配置。
UCLASS(NotBlueprintable)
class EMBERTRIAL_API UETBreakPulseCooldownEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UETBreakPulseCooldownEffect(const FObjectInitializer& ObjectInitializer);
};
```

### 3.3 `EmberTrial/Private/Abilities/Effects/ETBreakPulseCooldownEffect.cpp`

全新文件，完整内容如下。

```cpp
#include "Abilities/Effects/ETBreakPulseCooldownEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UETBreakPulseCooldownEffect::UETBreakPulseCooldownEffect(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    // 正值只是CDO结构占位；运行Spec必须SetDuration，不在这里保存6秒平衡数值。
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f));
    Period = FScalableFloat(0.0f);
    StackingType = EGameplayEffectStackingType::None;
    UTargetTagsGameplayEffectComponent* Tags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("BreakPulseCooldownTags"));
    GEComponents.Add(Tags);
    FInheritedTagContainer Granted;
    Granted.AddTag(ETGameplayTags::Cooldown_BreakPulse);
    Tags->SetAndApplyTargetTagChanges(Granted);
}
```

### 3.4 `EmberTrial/Public/Abilities/ETGA_BreakPulse.h`

全新文件，完整内容如下。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_BreakPulse.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

// C2-01只负责施放 / 真实冷却事务；C2-02再接唯一通知与范围结算。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_BreakPulse : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UETGA_BreakPulse();
    bool HasValidConfiguration() const;
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

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse")
    TObjectPtr<UAnimMontage> CastMontage;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "0.1", ClampMax = "60.0"))
    float CooldownSeconds = 6.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse",
              meta = (ClampMin = "0.0", ClampMax = "600.0"))
    float CastMaxHorizontalSpeed = 3.0f;

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
    void ReleaseMontage();
    UFUNCTION()
    void HandleCompleted();
    UFUNCTION()
    void HandleAborted();
};
```

### 3.5 `EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp`

全新文件，完整内容如下。

```cpp
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Effects/ETBreakPulseCooldownEffect.h"
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
        CastMontage->Notifies.Num() != 0)
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
    return IsValid(Clip) && Clip->GetSkeleton() == CastMontage->GetSkeleton() &&
           Clip->Notifies.Num() == 0 && !Clip->HasRootMotion() &&
           FMath::IsNearlyEqual(Clip->RateScale, 1.0f) && Segment.LoopingCount == 1 &&
           FMath::IsNearlyZero(Segment.StartPos) && FMath::IsNearlyZero(Segment.AnimStartTime) &&
           FMath::IsNearlyEqual(Segment.AnimPlayRate, 1.0f) &&
           FMath::IsNearlyEqual(Segment.AnimEndTime, Clip->GetPlayLength(), .001f) &&
           FMath::IsFinite(CastMontage->GetPlayLength()) && CastMontage->GetPlayLength() > .2f;
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
    UE_LOG(LogETBreakPulse, Display, TEXT("[M2-05C2-01] Begin Body=%s Serial=%u MontageID=%d"),
           *BodyName, Serial, ActiveMontageInstanceID);
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
    const FString BodyName = GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    bEnding = true;
    ++ActivationSerial;
    ReleaseMontage();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    // 父类结束可能重入新激活；之后只使用局部值记录，不重置实例成员。
    UE_LOG(LogETBreakPulse, Display, TEXT("[M2-05C2-01] End Body=%s Serial=%u Cancelled=%d"),
           *BodyName, EndedSerial, bWasCancelled ? 1 : 0);
}
```

## 4. 修改八个既有文件

以下按文件分组。每个小项先找到“原位置”，只按其说明替换 / 追加；保留该文件其他逻辑。`原位置`为短锚点时不用整段函数重写，尤其不要覆盖Character已有移动、三段攻击、资源与EndPlay实现。

`ETGA_Stagger.cpp`有两个完全相同的Actions数组末尾，要在**构造函数与HasValidConfiguration各改一次**；其他原位置在当前基线各出现一次。不要把原生F绑定再复制到角色EventGraph。

### 4.1 `EmberTrial/Public/Abilities/ETGameplayTags.h`

**第1处：三个原生Tag声明**。

原位置：

```cpp
    // ---- 治疗动作 ----
```

替换为：

```cpp
    // ---- 破势施放与独立冷却 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_BreakPulse);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_CastingBreakPulse);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_BreakPulse);
    // ---- 治疗动作 ----
```

### 4.2 `EmberTrial/Private/Abilities/ETGameplayTags.cpp`

**第1处：三个原生Tag定义**。

原位置：

```cpp
    // ---- 治疗动作 ----
```

替换为：

```cpp
    // ---- 破势施放与独立冷却 ----
    UE_DEFINE_GAMEPLAY_TAG(Ability_BreakPulse, "Ability.BreakPulse");
    UE_DEFINE_GAMEPLAY_TAG(State_CastingBreakPulse, "State.Casting.BreakPulse");
    UE_DEFINE_GAMEPLAY_TAG(Cooldown_BreakPulse, "Cooldown.BreakPulse");
    // ---- 治疗动作 ----
```

### 4.3 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

**第1处：Character前向声明**。

原位置：

```cpp
class UETGA_Heal;
```

替换为：

```cpp
class UETGA_Heal;
class UETGA_BreakPulse;
```

**第2处：自动化最小friend**。

原位置：

```cpp
    friend class FETPoiseReactionTest;
```

替换为：

```cpp
    friend class FETPoiseReactionTest;
    friend class FETBreakPulseAbilityTest;
```

**第3处：玩家能力Class / Action / Handle**。

原位置：

```cpp
    void HandleHealStarted();
```

替换为：

```cpp
    void HandleHealStarted();

    // ---- 破势施放：玩家专属，只由原生Started入口请求 ----
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse")
    TSubclassOf<UETGA_BreakPulse> BreakPulseAbilityClass;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|BreakPulse")
    TObjectPtr<UInputAction> BreakPulseAction;
    FGameplayAbilitySpecHandle BreakPulseAbilityHandle;
    void GrantBreakPulseAbility();
    void HandleBreakPulseStarted();
```

**第4处：正式请求接口**。

原位置：

```cpp
    bool RequestHeal();
```

替换为：

```cpp
    bool RequestHeal();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestBreakPulse();
```

### 4.4 `EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

**第1处：Character include**。

原位置：

```cpp
#include "Abilities/ETGA_Heal.h"
```

替换为：

```cpp
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_BreakPulse.h"
```

**第2处：BeginPlay授予**。

原位置：

```cpp
    GrantHealAbility();
```

替换为：

```cpp
    GrantHealAbility();
    GrantBreakPulseAbility();
```

**第3处：攻击缓冲拒绝施放中输入**。

原位置：

```cpp
        AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned))
```

替换为：

```cpp
        (AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned) ||
         AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse)))
```

**第4处：公共移动 / 跳跃 / E门控**。

原位置：

```cpp
               !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned);
```

替换为：

```cpp
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse);
```

**第5处：EndPlay清理自身Spec**。

原位置：

```cpp
            AbilitySystemComponent->ClearAbility(HealAbilityHandle);
        }
```

替换为：

```cpp
            AbilitySystemComponent->ClearAbility(HealAbilityHandle);
        }
        if (BreakPulseAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(BreakPulseAbilityHandle);
        }
```

**第6处：结束Handle失效**。

原位置：

```cpp
    HealAbilityHandle = FGameplayAbilitySpecHandle();
```

替换为：

```cpp
    HealAbilityHandle = FGameplayAbilitySpecHandle();
    BreakPulseAbilityHandle = FGameplayAbilitySpecHandle();
```

**第7处：唯一原生Started绑定**。

原位置：

```cpp
        if (IsValid(HealAction))
```

替换为：

```cpp
        if (IsValid(BreakPulseAction))
        {
            EnhancedInputComponent->BindAction(BreakPulseAction, ETriggerEvent::Started, this,
                &AEmberTrialCharacter::HandleBreakPulseStarted);
        }
        if (IsValid(HealAction))
```

**第8处：授予 / 请求 / 日志与按键回调实现**。

原位置：

```cpp
void AEmberTrialCharacter::HandleHealStarted()
```

在这个函数**签名之前**插入下面三个完整函数；原HandleHealStarted及其函数体保留，不再复制它的签名：

```cpp
// ---- 破势请求与冷却快照日志；不在输入层播放动画或修改Poise ----
void AEmberTrialCharacter::GrantBreakPulseAbility()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !HasAuthority() ||
        CombatTeam != EETCombatTeam::Player || !BreakPulseAbilityClass ||
        BreakPulseAbilityHandle.IsValid())
    {
        return;
    }
    if (BreakPulseAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        return;
    }
    BreakPulseAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(BreakPulseAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-05C2-01] Break granted Body=%s Valid=%d"),
           *GetNameSafe(this), BreakPulseAbilityHandle.IsValid() ? 1 : 0);
}

bool AEmberTrialCharacter::RequestBreakPulse()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !IsValid(AbilitySystemComponent) || !BreakPulseAbilityHandle.IsValid())
    {
        return false;
    }
    const FGameplayAbilitySpec* Spec =
        AbilitySystemComponent->FindAbilitySpecFromHandle(BreakPulseAbilityHandle);
    if (!Spec || !Spec->Ability || Spec->Ability->GetClass() != BreakPulseAbilityClass.Get())
    {
        return false;
    }
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC = AbilitySystemComponent;
    const FString BodyName = GetName();
    const FGameplayAbilitySpecHandle Handle = BreakPulseAbilityHandle;
    const bool bActivated = AbilitySystemComponent->TryActivateAbility(Handle, false);
    float Remaining = 0.0f;
    float Duration = 0.0f;
    UAbilitySystemComponent* ASC = WeakASC.Get();
    Spec = IsValid(ASC) ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
    const bool bAccepted = bActivated && Spec && Spec->IsActive();
    const UETGA_BreakPulse* Ability = Spec ? Cast<UETGA_BreakPulse>(Spec->Ability) : nullptr;
    if (Ability && ASC->AbilityActorInfo.IsValid())
    {
        Ability->GetCooldownTimeRemainingAndDuration(Handle, ASC->AbilityActorInfo.Get(), Remaining,
                                                     Duration);
    }
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M2-05C2-01] Break request Body=%s Accepted=%d Remaining=%.2f Duration=%.2f"),
           *BodyName, bAccepted ? 1 : 0, Remaining, Duration);
    return bAccepted;
}

void AEmberTrialCharacter::HandleBreakPulseStarted()
{
    RequestBreakPulse();
}
```

### 4.5 `EmberTrial/Private/Abilities/ETGA_HitReact.cpp`

**第1处：普通受击可以取消施放**。

原位置：

```cpp
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Heal);
```

替换为：

```cpp
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Heal);
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_BreakPulse);
```

**第2处：受击策略校验保留旧API**。

原位置：

```cpp
           !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Healing);
```

替换为：

```cpp
           !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Healing) &&
           CancelAbilitiesWithTag.HasTagExact(ETGameplayTags::Ability_BreakPulse) &&
           !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_CastingBreakPulse);
```

### 4.6 `EmberTrial/Private/Abilities/ETGA_Stagger.cpp`

**第1处：Stagger五种动作数量契约**。

原位置：

```cpp
        CancelAbilitiesWithTag.Num() != 4 || BlockAbilitiesWithTag.Num() != 4)
```

替换为：

```cpp
        CancelAbilitiesWithTag.Num() != 5 || BlockAbilitiesWithTag.Num() != 5)
```

**第2处：构造函数和HasValidConfiguration两个Actions数组末尾都追加**。

原位置：

```cpp
ETGameplayTags::Ability_HitReact};
```

替换为（两个Actions数组末尾项在本文件替换两次）：

```cpp
ETGameplayTags::Ability_HitReact, ETGameplayTags::Ability_BreakPulse};
```

### 4.7 `EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp`

**第1处：体力恢复共用阻断名单**。

原位置：

```cpp
ETGameplayTags::State_Dodging, ETGameplayTags::State_Healing,ETGameplayTags::State_Stunned};
```

替换为：

```cpp
ETGameplayTags::State_Dodging, ETGameplayTags::State_Healing, ETGameplayTags::State_Stunned,
        ETGameplayTags::State_CastingBreakPulse};
```

### 4.8 `EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`

**第1处：Validator include**。

原位置：

```cpp
#include "Abilities/ETGA_Stagger.h"
```

替换为：

```cpp
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGA_BreakPulse.h"
```

**第2处：输入稳定对象路径**。

原位置：

```cpp
constexpr TCHAR HealActionObject[]
```

替换为：

```cpp
constexpr TCHAR BreakActionObject[] = TEXT("/Game/EmberTrial/Input/Actions/IA_ET_BreakPulse.IA_ET_BreakPulse");
constexpr TCHAR HealActionObject[]
```

**第3处：正式GA校验白名单**。

原位置：

```cpp
        {TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger"), UETGA_Stagger::StaticClass()},
```

替换为：

```cpp
        {TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger"), UETGA_Stagger::StaticClass()},
        {TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse"), UETGA_BreakPulse::StaticClass()},
```

**第4处：能力配置分派**。

原位置：

```cpp
    else if (const UETGA_Stagger* Stagger
```

替换为：

```cpp
    else if (const UETGA_BreakPulse* Break = Cast<UETGA_BreakPulse>(Defaults))
    {
        if (!Break->HasValidConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-BREAK-001"),
                TEXT("BreakPulse requires valid native policy, in-place empty-notify montage, "
                     "Cooldown .1..60, Speed 0..600, and built-in Cost/Cooldown classes None."), Issues);
        }
    }
    else if (const UETGA_Stagger* Stagger
```

**第5处：正式玩家Class与输入配置**。

原位置：

```cpp
        RequireObject(Blueprint, Defaults, TEXT("HealAbilityClass"), TEXT("ET-HEAL-002"), Issues);
```

替换为：

```cpp
        RequireObject(Blueprint, Defaults, TEXT("BreakPulseAbilityClass"), TEXT("ET-BREAK-001"), Issues);
        const UInputAction* BreakInput = Cast<UInputAction>(
            RequireObject(Blueprint, Defaults, TEXT("BreakPulseAction"), TEXT("ET-BREAK-002"), Issues));
        if (IsValid(BreakInput) &&
            (BreakInput->GetPathName() != ETGameplayValidationPaths::BreakActionObject ||
             BreakInput->ValueType != EInputActionValueType::Boolean || BreakInput->bTriggerWhenPaused ||
             BreakInput->Triggers.Num() != 0 || BreakInput->Modifiers.Num() != 0))
        {
            AddIssue(Blueprint, TEXT("ET-BREAK-002"),
                TEXT("BreakPulseAction must be IA_ET_BreakPulse, Boolean, no Triggers/Modifiers, not while paused."), Issues);
        }
        RequireObject(Blueprint, Defaults, TEXT("HealAbilityClass"), TEXT("ET-HEAL-002"), Issues);
```

**第6处：受击错误说明**。

原位置：

```cpp
TEXT("HitReact must cancel Ability.Heal and must not block State.Healing."),
```

替换为：

```cpp
TEXT("HitReact must cancel Heal and BreakPulse, without blocking Healing/Casting states."),
```

**第7处：IMC唯一F与无触发器 / 修改器**。

原位置：

```cpp
    if (HealKeyCount != 1 || HealMappingCount != 1)
```

替换为：

```cpp
    int32 BreakKeyCount = 0;
    int32 BreakActionCount = 0;
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        const bool bBreak = IsValid(Mapping.Action) &&
            Mapping.Action->GetPathName() == ETGameplayValidationPaths::BreakActionObject;
        BreakKeyCount += Mapping.Key == EKeys::F ? 1 : 0;
        BreakActionCount += bBreak ? 1 : 0;
        if ((Mapping.Key == EKeys::F && !bBreak) || (bBreak && Mapping.Key != EKeys::F) ||
            (bBreak && (Mapping.Triggers.Num() != 0 || Mapping.Modifiers.Num() != 0)))
        {
            AddIssue(Context, TEXT("ET-BREAK-002"),
                TEXT("Only one F mapping to IA_ET_BreakPulse, with no mapping Triggers/Modifiers."), Issues);
        }
    }
    if (BreakKeyCount != 1 || BreakActionCount != 1)
    {
        AddIssue(Context, TEXT("ET-BREAK-002"), TEXT("Exactly one F / BreakPulse mapping required."), Issues);
    }
    if (HealKeyCount != 1 || HealMappingCount != 1)
```

### 4.9 完成后自查两处契约

1. Stagger的Cancel / Block数量均由4变5，两处Actions数组均有Ability_BreakPulse；不改变其1.2秒 / 旧HitResult资源及Poise恢复逻辑。
2. 恢复组件只在共用阻断名单加入Casting；注册与解绑都消费同一名单，不增加第二套Timer。Character公共Idle与轻攻击缓冲均拒绝Casting，防止F期间左键进入下一段缓冲。

请求日志的Accepted表示这次TryActivate后该Spec仍活动；真实提交另有Cooldown Applied=1日志。异步取消后保留GE，不依赖输入层自行维护剩余时间。HasValidHealingCancelPolicy保留既有接口名，同时检查受击对治疗与破势的取消策略，避免破坏旧校验调用。

## 5. 新增专项自动化

新建`EmberTrial/Tests/ETBreakPulseAbilityTests.cpp`，完整复制下文。现有16项测试保留；不要把测试类声明写成另一个名字，它与两处friend对应。

本测试使用真实保存的GA / Montage、真实GAS冷却与世界Tick，覆盖正常 / 非法配置 / 移动空中忙碌 / 暂停 / 普通受击 / Stagger / 死亡 / EndPlay八组。到期断言留时间余量，不在恰好6.000秒争论GE与Tick先后。EndPlay后只查询仍保留在测试世界中的对象，没有Destroy后推进再读旧ASC的路径；最终销毁由Fixture管理。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETBreakPulseAbilityTest, "EmberTrial.M2.Combat.BreakPulseAbility",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETBreakPulseAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    // 使用正式已保存配置；不能用测试自制GA掩盖实际资产的Tag或Montage漏配。
    UClass* BreakClass = LoadClass<UETGA_BreakPulse>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse.GA_ET_BreakPulse_C"));
    UClass* AttackClass = LoadClass<UETGA_LightAttack>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass = LoadClass<UETGA_Dodge>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    UClass* HealClass =
        LoadClass<UETGA_Heal>(nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal.GA_ET_Heal_C"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    UClass* StaggerClass = LoadClass<UETGA_Stagger>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger.GA_ET_Stagger_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved BreakPulse"), BreakClass) ||
        !TestNotNull(TEXT("Saved Attack"), AttackClass) ||
        !TestNotNull(TEXT("Saved Dodge"), DodgeClass) ||
        !TestNotNull(TEXT("Saved Heal"), HealClass) || !TestNotNull(TEXT("Saved Hit"), HitClass) ||
        !TestNotNull(TEXT("Saved Stagger"), StaggerClass) || !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_BreakPulse* Defaults = BreakClass->GetDefaultObject<UETGA_BreakPulse>();
    if (!TestTrue(TEXT("Saved cast configuration"), Defaults->HasValidConfiguration()) ||
        !TestEqual(TEXT("Default cooldown six seconds"), Defaults->CooldownSeconds, 6.0f))
    {
        return false;
    }
    enum class EScenario : uint8
    {
        Normal,
        Invalid,
        Gates,
        Pause,
        Hit,
        Stagger,
        Death,
        EndPlay
    };
    for (const EScenario Scenario :
         {EScenario::Normal, EScenario::Invalid, EScenario::Gates, EScenario::Pause, EScenario::Hit,
          EScenario::Stagger, EScenario::Death, EScenario::EndPlay})
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
        UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
        Floor->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(2000, 2000, 20));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Controller"), PC))
        {
            return false;
        }
        Body->CombatTeam = EETCombatTeam::Player;
        Body->InitialHealth = 40.0f;
        Body->LightAttackAbilityClass = AttackClass;
        Body->DodgeAbilityClass = DodgeClass;
        Body->HealAbilityClass = HealClass;
        Body->BreakPulseAbilityClass = BreakClass;
        Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Body->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->GetCharacterMovement()->bRunPhysicsWithNoController = true;
        PC->Possess(Body);
        TestFalse(TEXT("Not ready rejects"), Body->RequestBreakPulse());
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
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) ||
            !TestTrue(TEXT("Grounded"), Body->GetCharacterMovement()->IsMovingOnGround()))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Body->BreakPulseAbilityHandle);
        UETGA_BreakPulse* Ability =
            Spec ? Cast<UETGA_BreakPulse>(Spec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Instanced cast"), Ability))
        {
            return false;
        }
        auto Remaining = [Ability, ASC, Body]()
        {
            float Left = 0.0f;
            float Duration = 0.0f;
            Ability->GetCooldownTimeRemainingAndDuration(
                Body->BreakPulseAbilityHandle, ASC->AbilityActorInfo.Get(), Left, Duration);
            return Left;
        };
        auto HasCooldown = [ASC]()
        { return ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse); };
        if (Scenario == EScenario::Invalid)
        {
            UAnimMontage* Saved = Ability->CastMontage;
            Ability->CastMontage = nullptr;
            TestFalse(TEXT("None montage refuses"), Body->RequestBreakPulse());
            TestFalse(TEXT("None montage does not spend cooldown"), HasCooldown());
            Ability->CastMontage = Saved;
            Ability->CooldownSeconds = 0.0f;
            TestFalse(TEXT("Zero cooldown refuses"), Body->RequestBreakPulse());
            TestFalse(TEXT("Invalid config does not commit"), HasCooldown());
            Ability->CooldownSeconds = 6.0f;
        }
        else if (Scenario == EScenario::Gates)
        {
            Body->GetCharacterMovement()->Velocity = FVector(100, 0, 0);
            TestFalse(TEXT("Moving rejects"), Body->RequestBreakPulse());
            Body->GetCharacterMovement()->StopMovementImmediately();
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
            TestFalse(TEXT("Air rejects"), Body->RequestBreakPulse());
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            TestTrue(TEXT("Attack begins"), Body->RequestLightAttack());
            TestFalse(TEXT("Busy rejects cast"), Body->RequestBreakPulse());
            TestFalse(TEXT("All preflight refusals leave no cooldown"), HasCooldown());
        }
        else
        {
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
                TestFalse(TEXT("Paused input refuses"), Body->RequestBreakPulse());
                TestFalse(TEXT("Paused refusal leaves no cooldown"), HasCooldown());
                World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            }
            if (!TestTrue(TEXT("Cast begins"), Body->RequestBreakPulse()))
            {
                return false;
            }
            TestTrue(TEXT("Cast state active"), Ability->IsActive());
            TestEqual(TEXT("One casting tag"),
                      ASC->GetTagCount(ETGameplayTags::State_CastingBreakPulse), 1);
            TestEqual(TEXT("One cooldown tag"),
                      ASC->GetTagCount(ETGameplayTags::Cooldown_BreakPulse), 1);
            TestTrue(TEXT("Real duration GE starts near six"),
                     FMath::IsNearlyEqual(Remaining(), 6.0f, .05f));
            const uint32 Serial = Ability->ActivationSerial;
            TestFalse(TEXT("Busy repeated request refuses"), Body->RequestBreakPulse());
            TestTrue(TEXT("Repeat does not restart"), Ability->ActivationSerial == Serial);
            TestFalse(TEXT("Cast blocks attack"), Body->RequestLightAttack());
            TestFalse(TEXT("Cast blocks dodge"), Body->RequestDodge());
            TestFalse(TEXT("Cast blocks healing"), Body->RequestHeal());
            TestFalse(TEXT("Cast blocks ordinary actions"), Body->IsFormalActionIdle());
            if (!Advance(.15f))
            {
                return false;
            }
            if (Scenario == EScenario::Pause)
            {
                APlayerState* Pauser = PC->PlayerState;
                if (!IsValid(Pauser))
                {
                    Pauser = World->SpawnActor<APlayerState>();
                }
                if (!TestNotNull(TEXT("Pauser for active cast"), Pauser))
                {
                    return false;
                }
                const float BeforePause = Remaining();
                World->GetWorldSettings()->SetPauserPlayerState(Pauser);
                if (!Advance(1.5f))
                {
                    return false;
                }
                TestTrue(TEXT("Pause freezes cast montage"), Ability->IsActive());
                TestTrue(TEXT("Pause freezes real cooldown"),
                         FMath::IsNearlyEqual(Remaining(), BeforePause, .001f));
                World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            }
            if (Scenario == EScenario::Hit || Scenario == EScenario::Stagger)
            {
                const FGameplayAbilitySpecHandle Interrupt = ASC->GiveAbility(
                    FGameplayAbilitySpec(Scenario == EScenario::Hit ? HitClass : StaggerClass, 1));
                TestTrue(TEXT("Higher priority action accepted"),
                         ASC->TryActivateAbility(Interrupt));
                TestFalse(TEXT("Higher priority action cancels cast"), Ability->IsActive());
                TestFalse(TEXT("Casting tag removed on interrupt"),
                          ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse));
                TestTrue(TEXT("Interrupt does not refund cooldown"), HasCooldown());
            }
            else if (Scenario == EScenario::Death)
            {
                ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
                TestTrue(TEXT("Body really dead"), Body->IsCombatDead());
                TestFalse(TEXT("Death cancels cast"), Ability->IsActive());
                TestFalse(TEXT("Dead input refuses"), Body->RequestBreakPulse());
                TestTrue(TEXT("Death does not refund old body cooldown"), HasCooldown());
                AEmberTrialCharacter* NewBody = World->SpawnActor<AEmberTrialCharacter>(
                    FVector(400, 0, 200), FRotator::ZeroRotator, Spawn);
                if (!TestNotNull(TEXT("New body"), NewBody))
                {
                    return false;
                }
                TestFalse(TEXT("Cooldown belongs only to old ASC"),
                          NewBody->GetAbilitySystemComponent()->HasMatchingGameplayTag(
                              ETGameplayTags::Cooldown_BreakPulse));
            }
            if (Scenario != EScenario::Death && Scenario != EScenario::EndPlay)
            {
                if (!Advance(1.6f))
                {
                    return false;
                }
                TestFalse(TEXT("Cast finishes and clears state"), Ability->IsActive());
                TestTrue(TEXT("Actions unlock before cooldown expires"),
                         Body->IsFormalActionIdle());
                TestTrue(TEXT("Cooldown survives animation end or cancel"), HasCooldown());
                TestFalse(TEXT("Cooling rejects new request"), Body->RequestBreakPulse());
                TestEqual(TEXT("Only one cooldown after repeated requests"),
                          ASC->GetTagCount(ETGameplayTags::Cooldown_BreakPulse), 1);
                // 给GE到期Tick留余量，不在严格6秒边界断言Timer / GE先后顺序。
                if (!Advance(5.0f))
                {
                    return false;
                }
                TestFalse(TEXT("Cooldown naturally expires in game time"), HasCooldown());
                TestTrue(TEXT("After expiry a fresh press is accepted"), Body->RequestBreakPulse());
            }
            if (Scenario != EScenario::Death)
            {
                TestEqual(TEXT("Cast does not damage health"),
                          ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                          40.0f);
                TestEqual(
                    TEXT("Cast does not spend stamina"),
                    ASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
                    100.0f);
                TestEqual(TEXT("C2-01 has no Poise resolution"),
                          ASC->GetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute()),
                          100.0f);
                int32 Charges = 0;
                int32 Maximum = 0;
                UETHealingResourceComponent* Resource =
                    Body->FindComponentByClass<UETHealingResourceComponent>();
                TestTrue(TEXT("Healing resource remains available"),
                         Resource && Resource->GetHealingSnapshot(Charges, Maximum));
                TestEqual(TEXT("No healing charge cost"), Charges, 3);
            }
        }
        // 保留World EndPlay后的对象作只读断言；没有Destroy后推进再访问旧ASC。
        const FGameplayAbilitySpecHandle OwnedHandle = Body->BreakPulseAbilityHandle;
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("EndPlay removes casting tag"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse));
        TestNull(TEXT("EndPlay removes owned spec"), ASC->FindAbilitySpecFromHandle(OwnedHandle));
        TestFalse(TEXT("Ended body cannot start again"), Body->RequestBreakPulse());
    }
    return !HasAnyErrors();
}
#endif
```

## 6. 正常Build与CLion索引

确认UE已关闭、13份Source已保存。在PowerShell逐项执行；前一项失败就停止，不用清缓存继续掩盖首条错误。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

这条命令先正常Build EmberTrialEditor，再刷新三个模块CodeModel。之后在CLion Reload CMake Project；预期Core **53cpp**、UI17、Editor3，总73cpp / 136份Source。新测试算Core，不是Editor模块第4个cpp。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

Game Build成功后打开工程。若失败，发**首条编译Error / 文件行号 / 上下约20行**；仅Exit code6不是根因。UHT先查generated.h顺序 / 反射声明；链接错误先核对新函数定义是否完整，不删除旧定义来消除“重复”。本指南代码已做本地文本 / API静态核对，**未代替实际Build**。

## 7. UE资产与输入配置（不接蓝图执行线）

### 7.1 创建GA_ET_BreakPulse

1. Content Browser打开`/Game/EmberTrial/Abilities`，空白处右键 → Blueprint Class → All Classes。
2. 搜索`ETGA_BreakPulse`，核对原生类名，选择后创建`GA_ET_BreakPulse`。不要复制GA_ET_Stagger / Heal，以免继承旧数组或事件。
3. 打开新GA → Class Defaults。清空Details搜索，再按表逐项填写；EventGraph保持空。

| 属性 | 配置 |
| --- | --- |
| Cast Montage | `/Game/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01` |
| Cooldown Seconds | `6.0`，真正平衡参数只填这里 |
| Cast Max Horizontal Speed | `3.0` cm/s |
| Cost Gameplay Effect Class / Cooldown Gameplay Effect Class | **均None**，不要创建 / 赋GE蓝图 |
| Instancing Policy / Net Execution Policy / Replication Policy | Instanced Per Actor / Server Only / Replicate No |
| Retrigger Instanced Ability | False |

Tags来自原生构造，**逐项核对，不先清空再凭记忆重建**：

| 容器 | 精确内容 |
| --- | --- |
| Asset / Ability Tags | Ability.BreakPulse，1项 |
| Activation Owned Tags | State.Casting.BreakPulse，1项 |
| Activation Blocked Tags | State.Casting.BreakPulse、State.Attacking、State.Dodging、State.Healing、State.HitReact、State.Stunned、State.Dead，7项 |
| Block Abilities With Tag | Ability.Attack.Light、Ability.Dodge、Ability.Heal，3项 |
| Cancel Abilities With Tag | 空，0项；F不抢占正在播放的动作 |

Cooldown.BreakPulse由活动GE授予，不手填Owned Tags，也不额外填到7项Blocked里；框架CheckCooldown负责冷却拒绝。Compile → Save。

### 7.2 创建IA_ET_BreakPulse

1. `/Game/EmberTrial/Input/Actions`空白处右键 → Input → Input Action，命名`IA_ET_BreakPulse`。
2. 双击打开：Value Type=**Digital (bool) / Boolean**；Triggers和Modifiers数组均0项；Trigger When Paused=False。其他值保持默认，Save。
3. 打开**玩家当前引用的IMC_Default**（沿BP_ThirdPersonCharacter的Default Mapping Context右侧放大镜定位，不新建第二份）。本机为`/Game/ThirdPerson/Input/IMC_Default`。
4. Mappings点+增加一条，Action选IA_ET_BreakPulse，展开它的Key行，选键盘**F**。这一行Mappings的Triggers / Modifiers也均空。
5. 搜索 / 检查整个Mappings：F恰好一行、IA_ET_BreakPulse恰好一行；旧左键 / 数字1 / 2 / 3 / 4 / Q / E / P等原有项不改。不要添加Hold / Pressed Trigger；单次由C++的Started保证。
6. Save。若F已有其他映射且用途与本指南不同，先发实际行截图，不叠加第二个F。

### 7.3 玩家装配

1. 打开`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` → Class Defaults，搜索`BreakPulse`（找不到再搜`Break Pulse`）。
2. Break Pulse Ability Class选`GA_ET_BreakPulse`；Break Pulse Action选`IA_ET_BreakPulse`。
3. 只改Class Defaults，不在Level实例覆盖。EventGraph**不添加F / IA事件、不调用Montage Play**；唯一绑定已在原生SetupPlayerInputComponent。
4. 玩家原LightAttack / Dodge / Heal Class、输入、单ASC / CombatLife / CombatPoise等配置全部保留。Compile → Save。
5. 训练目标和敌人两个BreakPulse字段保持None；Grant函数只为Player阵营授予，不给AI增加F技能或输入。

### 7.4 修正已有受击与Stagger的保存数组

原生构造改变后，已保存蓝图可能仍保存旧数组。**不要假设重编C++会自动改写所有旧蓝图值。**

1. 打开`/Game/EmberTrial/Abilities/GA_ET_HitReact` → Class Defaults → Details搜索`Cancel`。
2. 在**Cancel Abilities With Tag**中保留现有Attack.Light / Dodge / Heal，追加`Ability.BreakPulse`；只追加一次。不要改成Activation Blocked。
3. 搜索`Blocked`，确认Activation Blocked中没有State.Casting.BreakPulse，也没有State.Healing；已有其他规则保留。Compile → Save。
4. 打开`/Game/EmberTrial/Abilities/GA_ET_Stagger` → Class Defaults。**Cancel Abilities With Tag**和**Block Abilities With Tag**两组都在原四项Attack.Light / Dodge / Heal / HitReact基础上追加Ability.BreakPulse，各恰好5项。
5. Stagger的Activation Owned仍只有State.Stunned；Activation Blocked仍State.Stunned / State.Dead两项；Duration1.2、旧HitResult Montage保持。Compile → Save。
6. 本批不改C1的AS / AM，**Notifies继续空**；不要提前在12帧创建名字相似的蓝图Notify。C2-02会同时更新通知类与严格配置校验。

## 8. 静态配置校验与自动化（按影响选择）

1. 六项资产依次Content Browser右键 → Asset Actions → **Validate Assets**：新GA、新IA、IMC_Default、玩家BP、GA_ET_HitReact、GA_ET_Stagger。原生规则覆盖GA / 玩家 / IMC / Hit / Stagger；IA本身可能显示Not Validated，**其字段由玩家与IMC规则间接检查，不要求IA单独打印本项目Valid**。
2. 其余五项应出现`[M1-07C2] Valid / Issues0`。ET-BREAK-001检查GA或玩家Class；ET-BREAK-002检查IA / F映射。出现问题按错误资产改对应字段，不改规则绕过。
3. 打开Output Log，先Stop PIE。逐条输入下列命令，上一项显示Success后再下一项；不是一次输入四行。

```text
Automation RunTests EmberTrial.M2.Combat.BreakPulseAbility
Automation RunTests EmberTrial.M2.Combat.StaggerAbility
Automation RunTests EmberTrial.M2.Combo.RecoveryDodge
Automation RunTests EmberTrial.M2.Healing.Ability
```

只跑这四项各一次；不重跑全部17项，也不重做旧多Pawn / HUD重建矩阵。新专项覆盖本批真实世界时间 / GE与清理；后三项覆盖取消名单、连段恢复闪避、治疗受击取消的共享影响。自动化失败先停止后续实机验收，发失败断言原文和本轮日志时间；若闪退，保留Saved/Crashes，不反复重跑。

## 9. 分组实机短查（约8–12分钟）

在`L_ET_VerticalSlice`正常PIE；保留正式敌人，不搭测试Controller图。Output Log搜索`M2-05C2-01`；仅看这一前缀会隐藏受击 / 死亡日志，需要核对时清空过滤。每组1次即可；无需逐行复制日志，告诉我完成时分，我读取本机记录。

### 9.1 施放、拒绝与自然到期（约2分钟）

1. 先避开敌人，站定后按F一次。应出现`Cooldown ... Applied=1 Duration=6.00`、`Begin ... MontageID=...`与`Break request ... Accepted=1`，播放前推并自动回待机。生命 / 体力 / 次数HUD不变。
2. 冷却未满再按F：Accepted=0，Remaining大于0；没有第二条Cooldown Applied=1 / Begin。动画结束后先试正常走动，行动已解锁；**冷却不等于整整6秒都锁人**。
3. 从第一次接受开始等待约7秒，再新按F应接受第二次。持有F超过6秒只播放一次，松开后重新按才可再放；忙碌 / 冷却中按过F不会在到期时自动施放。
4. 等冷却完：跑动中按F拒绝，站定重新按接受；下次等完后跳起按F拒绝，落地不自动播，再新按才接受。敌人不扣血 / 不进入新破势，属于本批边界。

### 9.2 互斥、打断与暂停（约3–4分钟）

1. 施放中尝试左键 / 2 / Q / E / Space，动作不抢占F，不扣闪避体力 / 治疗次数；镜头能转，F结束后重新按键功能恢复。攻击中按F应拒绝、不打断当前攻击。
2. 冷却结束后F立刻按3，正式HitReact取消F；随后F请求仍因剩余冷却拒绝。可顺便在敌人附近观察一次真实伤害取消，**不要求卡帧专门撞敌人**，真实优先级由专项覆盖。
3. 冷却结束后按F立刻P打开暂停，等现实约2秒再恢复。动画从冻结处继续，菜单内F不补播；暂停的现实2秒不会算入冷却。不要用现实手机计时精确断言，专项已检查世界时间。
4. 用P一种继续方式即可，确认恢复无需点击视口也能移动 / 转镜头，无卡键。本批不重复旧鼠标 / Enter / Space全矩阵。

### 9.3 生命周期与最后短基线（约3分钟）

1. F中按4若触发真实伤害则会被受击取消；按现有方式致死，确认Failure而非Pause、无卡在Casting。Retry后双100 / 3次，新身体F立即可用，不带旧冷却。
2. F中Stop PIE一次，重开后站定F可用，单生命 / 体力 / 次数HUD；无Accessed None或旧Body继续Begin。
3. 最后短查左键真实25、2消耗25 / 位移、受伤后Q一次40与减数、E提示、P一种继续方式、移动 / 转向 / 跳跃。旧用例只做这条短基线，不再搭临时函数。
4. Save All；正式玩家双100、Poise100，敌人100 / 伤害10、玩家攻击25；GA冷却6 / 速度3，原Stagger1.2与旧Montage、slomo1 / Debug False保持。没有修改这些值就填“原配置未改”。

## 10. 完成反馈与异常信息

```text
M2-05C2-01 完成
五新 / 八旧Source；关闭UE后Editor Build / Core53-UI17-Editor3索引 / Game Build：
GA父类 / C1实际Montage / 冷却6 / 速度3 / 内置Cost-Cooldown None / Tags：
IA Boolean / 单F / 单Started；玩家两引用 / Hit取消与Stagger五项；Compile-Save-Valid：
BreakPulseAbility / StaggerAbility / RecoveryDodge / Healing.Ability各一次：
F单次 / 长按 / 冷却到期 / 移动空中拒绝无补播、资源隔离：
动作互斥 / 3取消不退款 / P冻结恢复与无卡键：
死亡Retry / F中Stop重开 / 原能力与HUD短基线：
原配置 / slomo1 / Debug False、Source资产Save All：
失败或最明显表现问题（无填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

编译 / 自动化用文字贴首条Error或断言、路径行号和上下文；编辑器字段找不到用截图含当前资产名称 / 选中对象 / Details搜索框；动作时序给5–10秒视频与按键顺序。不要只发“退出码6”，也无需手抄所有成功日志。正常开发仍由你输入代码；需要直接接手时明确说明，我再修改实际工程。

本批通过后保存报告、登记真实新增GA / IA来源，再生成C2-02通知与范围结算。**2026-10-23收尾：实际已落地136Source / 73cpp / 17测试及两新GA / IA，来源70；范围结算仍待[C2-02](M2_05C2_02_BreakPulseRange.md)。**

## 11. 本批 Commit / Push（固定放在最后）

2026-10-23收尾：完整五新 / 八旧Source、两新 / 两变UE资产已由用户纳入本地`0cce598`，核对起点工作区 / 暂存区干净；HitReact / Stagger字节无差异，原生继承规则已有Valid / 实测支持，不强制重存。

当前Stage最新内容：本指南、`Docs/Reports/M2_05C2_01_Verification.md`、`Docs/AssetManifest.csv`、Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / 两份M2路线 / README / Introduction，以及`Docs/M2_05C2_02_BreakPulseRange.md`。源码 / 资产无差异不重复Stage。

```text
docs: verify break pulse cast and prepare range resolution
```

C2-02源码 / 通知尚未实施，按其最后清单完成后另交，不提前创建空文件。CLion刷新Changes → Stage最新文档 → 检查Diff → Commit → Push；Raw / 日志 / 生成文件 / 包体本地，用户操作Git，助手未改索引或联网核验远端。
