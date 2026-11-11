# M4-01C2 远程守卫的GAS施放、发射通知与冷却

2026-11-10已验收。七新三旧Core文件已提交，双Build、索引、四项专项与地图短查完成。实际手动60分钟，等待5分钟。以下保留实现步骤。[C2验收](Reports/M4_01C2_Verification.md)

## 1. 本批行为

| 环节 | 预期 |
| --- | --- |
| 请求 | `Event.Combat.RangedShot`携带发射者和目标。仅权威、就绪、站地的敌方身体能开始，其他动作状态和已有冷却拒绝请求 |
| 起手 | 提交真实2.5游戏秒冷却GE。远程能力持有现成的`State.Attacking`，受击或破势可取消，死亡沿用角色的取消所有能力 |
| 发射 | `ET Ranged Shot Release`是无状态Branching Point。GA核对当前Mesh、Montage、实例ID与通知对象，再消耗本次唯一发射尝试。射程、视线与目标在这一刻由C1入口复核 |
| 取消或空放 | 起手被打断且未到发射点时不会出弹。提交后的冷却照常计时；暂停按游戏时间冻结Montage和冷却 |
| 本批资产 | 自动化使用现有破势Montage的临时副本，换成远程发射通知后测试。正式远程动画、GA资产与守卫配置在后续接入，现有地图继续用近战守卫 |

## 2. 现有文件增量

先保存并关闭UE编辑器。下面路径均从`EmberTrial/Source/EmberTrial`起算。

### 2.1 `Public/Abilities/ETGameplayTags.h`

在现有的`Ability_Attack_Light`声明下插入三行。

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Ranged);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_RangedShot);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_RangedShot);
```

### 2.2 `Private/Abilities/ETGameplayTags.cpp`

在`Ability_Attack_Light`的完整定义后插入三行，注意原定义跨两行。

```cpp
    UE_DEFINE_GAMEPLAY_TAG(Ability_Attack_Ranged, "Ability.Attack.Ranged");
    UE_DEFINE_GAMEPLAY_TAG(Event_RangedShot, "Event.Combat.RangedShot");
    UE_DEFINE_GAMEPLAY_TAG(Cooldown_RangedShot, "Cooldown.RangedShot");
```

### 2.3 `Public/Characters/EmberTrialCharacter.h`

在`friend class FETRangedShotTest;`后增加一行，供专项配置阵营。

```cpp
    friend class FETRangedShotAbilityTest;
```

## 3. 新建运行时文件

按下列路径创建六份文件。`.generated.h`由正常构建生成。

### 3.1 `Public/Abilities/Effects/ETRangedShotCooldownEffect.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETRangedShotCooldownEffect.generated.h"

UCLASS(NotBlueprintable)
class EMBERTRIAL_API UETRangedShotCooldownEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETRangedShotCooldownEffect(const FObjectInitializer& ObjectInitializer);
};
```

### 3.2 `Private/Abilities/Effects/ETRangedShotCooldownEffect.cpp`

```cpp
#include "Abilities/Effects/ETRangedShotCooldownEffect.h"

#include "Abilities/ETGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UETRangedShotCooldownEffect::UETRangedShotCooldownEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.5f));

    UTargetTagsGameplayEffectComponent* Tags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("RangedShotCooldownTags"));
    GEComponents.Add(Tags);
    FInheritedTagContainer Granted;
    Granted.AddTag(ETGameplayTags::Cooldown_RangedShot);
    Tags->SetAndApplyTargetTagChanges(Granted);
}
```

### 3.3 `Public/Animation/ETAN_RangedShotRelease.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ETAN_RangedShotRelease.generated.h"

UCLASS(meta = (DisplayName = "ET Ranged Shot Release"))
class EMBERTRIAL_API UETAN_RangedShotRelease : public UAnimNotify
{
    GENERATED_BODY()

public:
    UETAN_RangedShotRelease();
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};
```

### 3.4 `Private/Animation/ETAN_RangedShotRelease.cpp`

```cpp
#include "Animation/ETAN_RangedShotRelease.h"

#include "Abilities/ETGA_RangedShot.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETAN_RangedShotRelease::UETAN_RangedShotRelease()
{
    bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;
#endif
}

void UETAN_RangedShotRelease::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body = IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC = IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_RangedShot* Ability = IsValid(ASC) ? Cast<UETGA_RangedShot>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Ability))
    {
        Ability->HandleRelease(Mesh, Payload.SequenceAsset, Payload.MontageInstanceID, this);
    }
}
```

### 3.5 `Public/Abilities/ETGA_RangedShot.h`

```cpp
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
```

### 3.6 `Private/Abilities/ETGA_RangedShot.cpp`

```cpp
#include "Abilities/ETGA_RangedShot.h"

#include "Abilities/Effects/ETRangedShotCooldownEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/ETAN_RangedShotRelease.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatDamage.h"
#include "Combat/ETCombatProjectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRangedShotAbility, Log, All);

UETGA_RangedShot::UETGA_RangedShot()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    CooldownGameplayEffectClass = UETRangedShotCooldownEffect::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(ETGameplayTags::Ability_Attack_Ranged);
    SetAssetTags(Tags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Attacking);
    for (const FGameplayTag& Tag : {FGameplayTag(ETGameplayTags::State_Attacking),
                                   FGameplayTag(ETGameplayTags::State_Dodging),
                                   FGameplayTag(ETGameplayTags::State_Healing),
                                   FGameplayTag(ETGameplayTags::State_CastingBreakPulse),
                                   FGameplayTag(ETGameplayTags::State_HitReact),
                                   FGameplayTag(ETGameplayTags::State_Stunned),
                                   FGameplayTag(ETGameplayTags::State_Dead)})
    {
        ActivationBlockedTags.AddTag(Tag);
    }

    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = ETGameplayTags::Event_RangedShot;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);
}

bool UETGA_RangedShot::HasValidConfiguration() const
{
    if (!IsValid(CastMontage) || !IsValid(CastMontage->GetSkeleton()) ||
        CastMontage->HasRootMotion() || !CastMontage->bEnableAutoBlendOut ||
        !FMath::IsNearlyEqual(CastMontage->RateScale, 1.0f) ||
        CastMontage->CompositeSections.Num() != 1 ||
        !CastMontage->CompositeSections[0].NextSectionName.IsNone() ||
        CastMontage->Notifies.Num() != 1 || GetCostGameplayEffect() ||
        !GetCooldownGameplayEffect() ||
        GetCooldownGameplayEffect()->GetClass() != UETRangedShotCooldownEffect::StaticClass() ||
        !IsValid(ShotConfig.ProjectileClass.Get()) ||
        ShotConfig.ProjectileClass->HasAnyClassFlags(CLASS_Abstract) ||
        !ETCombatDamage::IsValidConfiguration(ShotConfig.DamageEffectClass, ShotConfig.Damage) ||
        !FMath::IsFinite(ShotConfig.MinRange) || !FMath::IsFinite(ShotConfig.MaxRange) ||
        ShotConfig.MinRange < 1.0f || ShotConfig.MaxRange < ShotConfig.MinRange)
    {
        return false;
    }

    const FAnimNotifyEvent& Release = CastMontage->Notifies[0];
    const float Time = Release.GetTime();
    return IsValid(Release.Notify) && Release.Notify->IsA<UETAN_RangedShotRelease>() &&
           Release.IsBranchingPoint() && FMath::IsNearlyEqual(Release.NotifyTriggerChance, 1.0f) &&
           Release.NotifyFilterType == ENotifyFilterType::NoFiltering && FMath::IsFinite(Time) &&
           Time > FMath::Max(.05f, CastMontage->BlendIn.GetBlendTime()) &&
           Time < CastMontage->GetPlayLength() - CastMontage->BlendOut.GetBlendTime();
}

AEmberTrialCharacter* UETGA_RangedShot::ReadSource(const FGameplayAbilityActorInfo* ActorInfo)
{
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    const UWorld* World = IsValid(Body) ? Body->GetWorld() : nullptr;
    const UCharacterMovementComponent* Move = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    return IsValid(Body) && !Body->IsActorBeingDestroyed() && Body->HasAuthority() &&
           Body->GetCombatTeam() == EETCombatTeam::Enemy && Body->IsGameplayReady() && !Body->IsCombatDead() &&
           IsValid(ASC) && ActorInfo->OwnerActor.Get() == Body && Body->GetAbilitySystemComponent() == ASC &&
           World && !World->IsPaused() && IsValid(Move) && Move->IsMovingOnGround() &&
           IsValid(ActorInfo->GetAnimInstance()) ? Body : nullptr;
}

AEmberTrialCharacter* UETGA_RangedShot::ReadTarget(AEmberTrialCharacter* Source, const FGameplayEventData* Payload)
{
    AEmberTrialCharacter* Target = Payload
        ? const_cast<AEmberTrialCharacter*>(Cast<AEmberTrialCharacter>(Payload->Target.Get())) : nullptr;
    return IsValid(Source) && Payload && Payload->EventTag == ETGameplayTags::Event_RangedShot &&
           Payload->Instigator.Get() == Source && IsValid(Target) &&
           Target->IsGameplayReady() && !Target->IsCombatDead() &&
           Target->GetWorld() == Source->GetWorld() && Source->CanDealCombatDamageTo(Target) ? Target : nullptr;
}

bool UETGA_RangedShot::ShouldAbilityRespondToEvent(
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
    return ReadTarget(ReadSource(ActorInfo), Payload) != nullptr &&
           Super::ShouldAbilityRespondToEvent(ActorInfo, Payload);
}

bool UETGA_RangedShot::CanActivateAbility(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    const AEmberTrialCharacter* Body = ReadSource(ActorInfo);
    if (!Body || !Body->IsFormalActionIdle() || !HasValidConfiguration() ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    const UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    const USkeletalMeshComponent* Mesh = Body->GetMesh();
    return IsValid(Mesh) && IsValid(Mesh->GetSkeletalMeshAsset()) &&
           Mesh->GetSkeletalMeshAsset()->GetSkeleton() == CastMontage->GetSkeleton() &&
           Anim->GetSkelMeshComponent() == Mesh && !Anim->GetCurrentActiveMontage();
}

void UETGA_RangedShot::ActivateAbility(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    const uint32 Serial = ++ActivationSerial;
    bEnding = false;
    bReleaseAttempted = false;
    AEmberTrialCharacter* Body = ReadSource(ActorInfo);
    AEmberTrialCharacter* Target = ReadTarget(Body, TriggerEventData);
    if (!IsValid(Target) || !HasValidConfiguration())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }

    ActiveTarget = Target;
    ActiveConfig = ShotConfig;
    ActiveMontage = CastMontage;
    ActiveNotify = Cast<UETAN_RangedShotRelease>(ActiveMontage->Notifies[0].Notify.Get());
    ActiveReleaseTime = ActiveMontage->Notifies[0].GetTime();

    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    ObservedASC = ASC;
    // 既有受击与破势资产可以保留原取消列表。
    HitHandle = ASC->RegisterGameplayTagEvent(ETGameplayTags::State_HitReact)
        .AddUObject(this, &ThisClass::HandleInterruptTag);
    StunHandle = ASC->RegisterGameplayTagEvent(ETGameplayTags::State_Stunned)
        .AddUObject(this, &ThisClass::HandleInterruptTag);

    const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
    if (!IsActive() || Serial != ActivationSerial || bEnding)
    {
        return;
    }
    Body = ReadSource(ActorInfo);
    ASC = ObservedASC.Get();
    if (!bCommitted || !Body || !IsValid(ASC) || Body->GetAbilitySystemComponent() != ASC ||
        !ActiveTarget.IsValid() ||
        !ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_RangedShot) ||
        ActorInfo->GetAnimInstance()->GetCurrentActiveMontage())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }

    Body->GetCharacterMovement()->StopMovementImmediately();
    UAbilityTask_PlayMontageAndWait* Task =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, ActiveMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!IsValid(Task))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask = Task;
    Task->OnCompleted.AddDynamic(this, &ThisClass::HandleCompleted);
    Task->OnInterrupted.AddDynamic(this, &ThisClass::HandleAborted);
    Task->OnCancelled.AddDynamic(this, &ThisClass::HandleAborted);

    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC(ASC);
    const TWeakObjectPtr<UAnimInstance> WeakAnim(ActorInfo->GetAnimInstance());
    const TWeakObjectPtr<UAnimMontage> StartedMontage(ActiveMontage.Get());
    Task->ReadyForActivation();

    ASC = WeakASC.Get();
    UAnimInstance* Anim = WeakAnim.Get();
    if (!IsActive() || Serial != ActivationSerial)
    {
        if (!IsActive() && IsValid(ASC) && StartedMontage.IsValid() &&
            ASC->GetAnimatingAbility() == this && ASC->GetCurrentMontage() == StartedMontage.Get())
        {
            ASC->CurrentMontageStop(0.0f);
            if (!IsActive() && ASC->GetAnimatingAbility() == this)
            {
                ASC->ClearAnimatingAbility(this);
            }
        }
        return;
    }

    const FAnimMontageInstance* Instance =
        IsValid(Anim) ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    if (!Instance || !IsValid(ASC) || ASC->GetAnimatingAbility() != this ||
        ASC->GetCurrentMontage() != ActiveMontage || MontageTask != Task)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    ActiveInstanceID = Instance->GetInstanceID();
}

void UETGA_RangedShot::HandleRelease(
    USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
    int32 InstanceID, const UETAN_RangedShotRelease* Notify)
{
    if (!IsActive() || bEnding || bReleaseAttempted || InstanceID == INDEX_NONE ||
        InstanceID != ActiveInstanceID || Animation != ActiveMontage ||
        !IsValid(Notify) || Notify != ActiveNotify.Get())
    {
        return;
    }
    AEmberTrialCharacter* Body = ReadSource(CurrentActorInfo);
    UAbilitySystemComponent* ASC = ObservedASC.Get();
    if (!Body || Mesh != Body->GetMesh() || !IsValid(ASC) ||
        ASC != Body->GetAbilitySystemComponent() || ASC->GetAnimatingAbility() != this ||
        ASC->GetCurrentMontage() != ActiveMontage)
    {
        return;
    }
    const FAnimMontageInstance* Instance =
        CurrentActorInfo->GetAnimInstance()->GetMontageInstanceForID(InstanceID);
    if (!Instance || !Instance->IsActive() || Instance->Montage != ActiveMontage ||
        Instance->GetPosition() + .001f < ActiveReleaseTime)
    {
        return;
    }

    // 即使临时失去视线，当前动画只尝试发射一次。
    bReleaseAttempted = true;
    const FString Name = GetNameSafe(Body);
    const uint32 Serial = ActivationSerial;
    const bool bFired = ETRangedShot::TryFire(Body, ActiveTarget.Get(), ActiveConfig, GetAbilityLevel()) != nullptr;
    UE_LOG(LogETRangedShotAbility, Display, TEXT("[M4-01C2] Release Body=%s Serial=%u Fired=%d"),
           *Name, Serial, bFired ? 1 : 0);
}

void UETGA_RangedShot::HandleInterruptTag(FGameplayTag Tag, int32 Count)
{
    (void)Tag;
    if (Count > 0 && IsActive() && !bEnding)
    {
        CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false);
    }
}

void UETGA_RangedShot::HandleCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_RangedShot::HandleAborted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UETGA_RangedShot::EndAbility(
    FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
    FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
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

    bEnding = true;
    ++ActivationSerial;
    if (UAbilitySystemComponent* ASC = ObservedASC.Get())
    {
        ASC->UnregisterGameplayTagEvent(HitHandle, ETGameplayTags::State_HitReact);
        ASC->UnregisterGameplayTagEvent(StunHandle, ETGameplayTags::State_Stunned);
    }
    HitHandle.Reset();
    StunHandle.Reset();
    ObservedASC.Reset();

    if (IsValid(MontageTask))
    {
        MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
        MontageTask->OnCancelled.RemoveAll(this);
    }
    MontageTask = nullptr;
    ActiveMontage = nullptr;
    ActiveNotify.Reset();
    ActiveTarget.Reset();
    ActiveConfig = FETRangedShotConfig{};
    ActiveInstanceID = INDEX_NONE;
    ActiveReleaseTime = 0.0f;
    // GAS在结束时清理自己的任务与Montage。已经提交的冷却由ASC计时。
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
```

## 4. 新建能力专项

在`Tests/ETRangedShotAbilityTests.cpp`粘贴完整代码。测试在隔离世界授予原生能力，并只给本次实例设置临时蒙太奇和发射配置。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_RangedShot.h"

#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/ETAN_RangedShotRelease.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRangedShotAbilityTest, "EmberTrial.M4.Combat.RangedShotAbility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRangedShotAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UAnimMontage* Template = LoadObject<UAnimMontage>(nullptr,
        TEXT("/Game/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01.AM_ET_BreakPulse_01"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    FETRangedShotConfig Config;
    Config.ProjectileClass = AETCombatProjectile::StaticClass();
    Config.DamageEffectClass = LoadClass<UGameplayEffect>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    if (!TestNotNull(TEXT("Saved montage template"), Template) || Template->Notifies.Num() != 1 ||
        !TestNotNull(TEXT("Quinn"), Quinn) || !TestNotNull(TEXT("Hit reaction"), HitClass) ||
        !TestNotNull(TEXT("Health damage GE"), Config.DamageEffectClass.Get()))
    {
        return false;
    }

    // 复用现有动作，临时副本仅换通知。不会保存或修改正式动画。
    TStrongObjectPtr<UAnimMontage> Montage(DuplicateObject<UAnimMontage>(Template, GetTransientPackage()));
    Montage->Notifies[0].Notify = NewObject<UETAN_RangedShotRelease>(Montage.Get());
    Montage->RefreshCacheData();

    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>();
    AEmberTrialCharacter* Target = World->SpawnActor<AEmberTrialCharacter>(
        FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
    AAIController* Controller = World->SpawnActor<AAIController>();
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Shooter"), Body) || !TestNotNull(TEXT("Target"), Target) ||
        !TestNotNull(TEXT("AI controller"), Controller) || !TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    Body->CombatTeam = EETCombatTeam::Enemy;
    Target->CombatTeam = EETCombatTeam::Player;
    Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
    Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Body->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Controller->Possess(Body);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    for (AEmberTrialCharacter* Character : {Body, Target})
    {
        Character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Character->GetCharacterMovement()->SetComponentTickEnabled(false);
    }

    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const FGameplayAbilitySpecHandle Ranged = ASC->GiveAbility(FGameplayAbilitySpec(UETGA_RangedShot::StaticClass()));
    const FGameplayAbilitySpecHandle Hit = ASC->GiveAbility(FGameplayAbilitySpec(HitClass));
    const FGameplayAbilitySpec* RangedSpec = ASC->FindAbilitySpecFromHandle(Ranged);
    UETGA_RangedShot* RangedAbility =
        RangedSpec ? Cast<UETGA_RangedShot>(RangedSpec->GetPrimaryInstance()) : nullptr;
    if (!TestNotNull(TEXT("Granted ranged ability instance"), RangedAbility))
    {
        return false;
    }
    // 配置本世界的实例，不改CDO或磁盘上的能力资产。
    RangedAbility->CastMontage = Montage.Get();
    RangedAbility->ShotConfig = Config;
    if (!TestTrue(TEXT("Can activate with temporary montage"), RangedAbility->CanActivateAbility(
            Ranged, ASC->AbilityActorInfo.Get())))
    {
        return false;
    }

    const auto Request = [ASC, Body](AActor* Aim)
    {
        FGameplayEventData Event;
        Event.EventTag = ETGameplayTags::Event_RangedShot;
        Event.Instigator = Body;
        Event.Target = Aim;
        return ASC->HandleGameplayEvent(Event.EventTag, &Event);
    };
    const auto Advance = [&Fixture](float Seconds)
    {
        for (int32 Index = 0; Index < FMath::CeilToInt(Seconds / .01f); ++Index)
        {
            if (!Fixture.TickTestWorld(.01f))
            {
                return false;
            }
        }
        return true;
    };
    const auto IsCasting = [ASC, Ranged]()
    {
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Ranged);
        return Spec && Spec->IsActive();
    };
    const auto IsCooling = [ASC]()
    {
        return ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_RangedShot);
    };

    TestEqual(TEXT("Missing target event rejects"), Request(nullptr), 0);
    TestFalse(TEXT("Rejected request has no cooldown"), IsCooling());
    if (!TestEqual(TEXT("Accept target event"), Request(Target), 1) ||
        !TestTrue(TEXT("Attacking tag at windup"),
                  IsCasting() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking)) ||
        !Advance(.12f))
    {
        return false;
    }
    TestTrue(TEXT("Real cooldown starts on commit"), IsCooling());
    TestEqual(TEXT("Duplicate event rejects"), Request(Target), 0);
    TestEqual(TEXT("No damage before release"), TargetASC->GetNumericAttribute(Health), 100.0f);
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("Pause fixture active"), World->IsPaused());
    const bool bPausedTick = Advance(.6f);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    if (!bPausedTick)
    {
        return false;
    }
    TestEqual(TEXT("Pause did not release"), TargetASC->GetNumericAttribute(Health), 100.0f);
    if (!Advance(1.2f))
    {
        return false;
    }
    TestEqual(TEXT("One release hit after resume"), TargetASC->GetNumericAttribute(Health), 88.0f);
    TestEqual(TEXT("Health base after one release"), TargetASC->GetNumericAttributeBase(Health), 88.0f);
    TestFalse(TEXT("Montage and attacking tag ended"),
              IsCasting() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
    TestTrue(TEXT("Cooldown outlives montage"), IsCooling());
    TestEqual(TEXT("Cooldown rejects after montage"), Request(Target), 0);
    if (!Advance(2.5f))
    {
        return false;
    }
    TestFalse(TEXT("Cooldown expires"), IsCooling());

    if (!TestEqual(TEXT("Next cast accepted"), Request(Target), 1) || !Advance(.12f) ||
        !TestTrue(TEXT("Actual HitReact interrupts ranged windup"), ASC->TryActivateAbility(Hit, false)))
    {
        return false;
    }
    TestFalse(TEXT("Interrupted cast releases attacking tag"),
              IsCasting() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
    TestTrue(TEXT("Interrupted windup keeps cooldown"), IsCooling());
    if (!Advance(1.0f))
    {
        return false;
    }
    TestEqual(TEXT("Interrupted windup never fires"), TargetASC->GetNumericAttribute(Health), 88.0f);
    if (!Advance(2.5f) || !TestEqual(TEXT("Third windup accepted"), Request(Target), 1))
    {
        return false;
    }
    Target->SetActorLocation(FVector(1600.0, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
    if (!Advance(1.2f))
    {
        return false;
    }
    TestEqual(TEXT("Target out of range by release takes no damage"),
              TargetASC->GetNumericAttribute(Health), 88.0f);
    TestTrue(TEXT("Whiff keeps cooldown"), IsCooling());

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 5. 构建与索引

保存源码，在UE关闭时，从仓库根目录依次执行。第一条正常构建Editor并刷新CLion索引，第二条单独构建Game Development。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

```powershell
Set-Location 'E:\UE_Project\Demo'
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion重载CMake后，Core应从95增加到99个cpp。

## 6. 分组验收

隔离工程已通过UE 5.5.4 Editor Development构建与47项全组自动化。最终报告在`Artifacts/M4_01C2/AutomationReport_Verified/index.json`。正式工程按以下两组验收。

### 6.1 发射与能力

在UE的Output Log命令栏逐条执行，上一条结束后再执行下一条。

```text
Automation RunTests EmberTrial.M4.Combat.RangedShot
Automation RunTests EmberTrial.M4.Combat.Projectile
```

第一个前缀匹配`RangedShot`和`RangedShotAbility`，第二个匹配`ProjectileFlight`和`ProjectileDamage`。预期四项均为Success。

| 检查 | 预期 |
| --- | --- |
| 起手与冷却 | 无目标拒绝，无冷却。合法事件持有`State.Attacking`并开始真实冷却。连发和冷却期间请求被拒绝 |
| 通知与伤害 | 通知前生命100。暂停后仍100，恢复播放后仅一发实际命中，生命Current与Base均为88 |
| 中断与重判 | 第二次起手由正式HitReact打断，保持88和既有冷却。第三次起手后目标移出射程，通知尝试失败，保持88 |
| 兼容 | 原生发射、投射物飞行和共享伤害专项继续通过 |

### 6.2 正式地图短查

1. 打开`L_ET_VerticalSlice`，点Play，确认进入Entry并能移动。
2. 按P打开暂停菜单。点击继续，再移动一次并停止PIE。本批正式地图沿用近战守卫，远程能力由专项授予。

反馈七新文件、两Tag文件与测试friend、双Build与索引、四项自动化、地图短查、保存状态、手动时间和构建等待。报错贴首条编译错误，或测试名和首条`Expected`。

## 7. Commit / Push（历史）

下列文档和实现已提交。当前阶段清单见[M4-01C3操作指南](M4_01C3_RangedShotAssets.md)。

```text
Docs/Reports/M4_01C1_Verification.md
Docs/M4_01C1_RangedShot.md
Docs/M4_01C2_RangedShotAbility.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

- 英文 `docs: verify ranged firing and plan montage-driven casting`
- 中文 `docs: 完成远程发射验收并规划蒙太奇驱动施放`

手动实现与验收后，Stage以下十份Core文件。检查Staged Diff再Commit与Push。

```text
EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRangedShotCooldownEffect.h
EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRangedShotCooldownEffect.cpp
EmberTrial/Source/EmberTrial/Public/Animation/ETAN_RangedShotRelease.h
EmberTrial/Source/EmberTrial/Private/Animation/ETAN_RangedShotRelease.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_RangedShot.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_RangedShot.cpp
EmberTrial/Source/EmberTrial/Tests/ETRangedShotAbilityTests.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
```

- 英文 `feat: add ranged attack ability with montage release and cooldown`
- 中文 `feat: 新增带蒙太奇发射点与冷却的远程攻击能力`

隔离候选、构建日志和自动化报告留在本地`Artifacts`。
