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