#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/ETAN_HealCommit.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Resources/ETHealingResourceComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHeal, Log, All);

UETGA_Heal::UETGA_Heal()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Heal);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Healing);
    const FGameplayTag BlockedTags[] = {ETGameplayTags::State_Healing, ETGameplayTags::State_Attacking,
        ETGameplayTags::State_Dodging, ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
    for (const FGameplayTag& Tag : BlockedTags)
    {
        ActivationBlockedTags.AddTag(Tag);
    }
}

bool UETGA_Heal::HasValidTagPolicy() const
{
    if (GetAssetTags().Num()!=1 || !GetAssetTags().HasTagExact(ETGameplayTags::Ability_Heal) ||
        ActivationOwnedTags.Num()!=1 || !ActivationOwnedTags.HasTagExact(ETGameplayTags::State_Healing) ||
        CancelAbilitiesWithTag.Num()!=0 || BlockAbilitiesWithTag.Num()!=0 || ActivationBlockedTags.Num()!=5) { return false; }
    const FGameplayTag BlockedTags[] = {ETGameplayTags::State_Healing, ETGameplayTags::State_Attacking,
        ETGameplayTags::State_Dodging, ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
    for (const FGameplayTag& Tag : BlockedTags)
    {
        if (!ActivationBlockedTags.HasTagExact(Tag)) { return false; }
    }
    return true;
}

bool UETGA_Heal::HasValidConfiguration() const
{
    if (!HasValidTagPolicy() || GetCostGameplayEffect() || GetCooldownGameplayEffect() ||
        !FMath::IsFinite(HealAmount) || HealAmount<1.0f || HealAmount>1000.0f ||
        !IsValid(HealMontage) || !IsValid(HealMontage->GetSkeleton()) || HealMontage->HasRootMotion() ||
        !FMath::IsNearlyEqual(HealMontage->RateScale,1.0f) || !HealMontage->bEnableAutoBlendOut ||
        HealMontage->SlotAnimTracks.Num()!=1 ||
        HealMontage->SlotAnimTracks[0].SlotName!=FName(TEXT("DefaultSlot")) ||
        HealMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num()!=1 ||
        HealMontage->CompositeSections.Num()!=1 ||
        !HealMontage->CompositeSections[0].NextSectionName.IsNone() || HealMontage->Notifies.Num()!=1)
    { return false; }
    const FAnimSegment& Segment=HealMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
    const UAnimSequenceBase* Clip=Segment.GetAnimReference();
    const FAnimNotifyEvent& Event=HealMontage->Notifies[0];
    const float Length=HealMontage->GetPlayLength();
    const float FadeIn=HealMontage->BlendIn.GetBlendTime();
    const float FadeOut=HealMontage->BlendOut.GetBlendTime();
    return IsValid(Clip) && Clip->GetSkeleton()==HealMontage->GetSkeleton() &&
        Clip->Notifies.Num()==0 && FMath::IsNearlyEqual(Clip->RateScale,1.0f) &&
        Segment.LoopingCount==1 && FMath::IsNearlyZero(Segment.StartPos) &&
        FMath::IsNearlyZero(Segment.AnimStartTime) && FMath::IsNearlyEqual(Segment.AnimPlayRate,1.0f) &&
        FMath::IsNearlyEqual(Segment.AnimEndTime,Clip->GetPlayLength(),.001f) &&
        FMath::IsFinite(Length) && Length>0.2f && FMath::IsFinite(FadeIn) && FadeIn>=0.0f &&
        FMath::IsFinite(FadeOut) && FadeOut>=0.0f &&
        IsValid(Event.Notify) && Event.Notify->IsA<UETAN_HealCommit>() && !Event.NotifyStateClass &&
        Event.IsBranchingPoint() && FMath::IsNearlyEqual(Event.NotifyTriggerChance,1.0f) &&
        Event.NotifyFilterType==ENotifyFilterType::NoFiltering && FMath::IsFinite(Event.GetTime()) &&
        Event.GetTime()>FMath::Max(.05f,FadeIn) && Event.GetTime()<Length-FadeOut;
}

bool UETGA_Heal::ReadContext(const FGameplayAbilityActorInfo* ActorInfo,
    AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const
{
    OutBody=ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    OutASC=ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    APlayerController* PC=IsValid(OutBody) ? Cast<APlayerController>(OutBody->GetController()) : nullptr;
    const UCharacterMovementComponent* Move=IsValid(OutBody) ? OutBody->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice=IsValid(OutBody)
        ? OutBody->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    return IsValid(OutBody) && !OutBody->IsActorBeingDestroyed() && OutBody->HasAuthority() &&
        OutBody->IsGameplayReady() && !OutBody->IsCombatDead() && OutBody->GetCombatTeam()==EETCombatTeam::Player &&
        ActorInfo->OwnerActor.Get()==OutBody && IsValid(OutASC) && OutBody->GetAbilitySystemComponent()==OutASC &&
        OutBody->GetWorld() && OutBody->GetWorld()->GetNetMode()==NM_Standalone && !OutBody->GetWorld()->IsPaused() &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn()==OutBody && !PC->IsMoveInputIgnored() &&
        IsValid(Move) && Move->IsMovingOnGround() && !Move->HasRootMotionSources() &&
        !OutBody->GetVelocity().ContainsNaN() && OutBody->GetVelocity().SizeSquared2D()<=9.0 &&
        (!Practice || Practice->IsPracticeActionIdle()) && IsValid(ActorInfo->GetAnimInstance());
}

bool UETGA_Heal::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    AEmberTrialCharacter* Body=nullptr;UAbilitySystemComponent* ASC=nullptr;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo,Body,ASC) || !Body->IsFormalActionIdle() ||
        !Super::CanActivateAbility(Handle,ActorInfo,SourceTags,TargetTags,OptionalRelevantTags)) { return false; }
    const UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
    UAnimInstance* Anim=ActorInfo->GetAnimInstance();
    return IsValid(Resource) && Resource->CanApplyHealing(HealAmount) &&
        !Anim->GetCurrentActiveMontage() && Anim->GetSkelMeshComponent()->GetSkeletalMeshAsset() &&
        Anim->GetSkelMeshComponent()->GetSkeletalMeshAsset()->GetSkeleton()==HealMontage->GetSkeleton();
}

void UETGA_Heal::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    if (!IsActive()) { return; }
    ++ActivationSerial;if (ActivationSerial==0) { ++ActivationSerial; }
    const uint32 Serial=ActivationSerial;
    bEnding=false;bStarting=true;bCommitAttempted=false;
    AEmberTrialCharacter* Body=nullptr;UAbilitySystemComponent* ASC=nullptr;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo,Body,ASC))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    if (ActorInfo->GetAnimInstance()->GetCurrentActiveMontage() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    const UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
    if (!IsValid(Resource) || !Resource->CanApplyHealing(HealAmount))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    ActiveMontage=HealMontage;ActiveHealAmount=HealAmount;
    ActiveCommitToken=ActiveMontage->Notifies[0].Notify;
    ActiveCommitTime=ActiveMontage->Notifies[0].GetTime();
    Body->StopJumping();Body->ConsumeMovementInputVector();
    Body->GetCharacterMovement()->StopMovementImmediately();
    UAbilityTask_PlayMontageAndWait* Task=UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this,NAME_None,ActiveMontage,1.0f,NAME_None,true,1.0f,0.0f,true);
    if (!Task) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    MontageTask=Task;
    Task->OnCompleted.AddDynamic(this,&ThisClass::HandleCompleted);
    Task->OnInterrupted.AddDynamic(this,&ThisClass::HandleAborted);
    Task->OnCancelled.AddDynamic(this,&ThisClass::HandleAborted);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC=ASC;
    const TWeakObjectPtr<UAnimInstance> WeakAnim=ActorInfo->GetAnimInstance();
    UAnimMontage* StartingMontage=ActiveMontage;
    Task->ReadyForActivation();
    ASC=WeakASC.Get();UAnimInstance* Anim=WeakAnim.Get();
    if (!IsActive() || Serial!=ActivationSerial)
    {
        // PlayMontage may assign ownership after a synchronous cancellation.
        if (!IsActive() && IsValid(ASC) && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==StartingMontage)
        {
            ASC->CurrentMontageStop(0.0f);
            if (!IsActive() && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==StartingMontage)
            {
                ASC->ClearAnimatingAbility(this);
            }
        }
        return;
    }
    FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    if (!IsValid(ASC) || !Instance || !Anim->Montage_IsPlaying(ActiveMontage) ||
        ASC->GetAnimatingAbility()!=this || MontageTask!=Task)
    {
        EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return;
    }
    ActiveMontageInstanceID=Instance->GetInstanceID();
    bStarting=false;
    UE_LOG(LogETHeal,Display,TEXT("[M2-04B2] Begin Body=%s Serial=%u MontageID=%d CommitAt=%.3f Amount=%.1f"),
        *GetNameSafe(Body),Serial,ActiveMontageInstanceID,ActiveCommitTime,ActiveHealAmount);
}

void UETGA_Heal::HandleHealCommit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
    int32 MontageInstanceID, UAnimNotify* NotifyToken)
{
    if (!IsActive() || bStarting || bEnding || bCommitAttempted || !CurrentActorInfo ||
        Animation!=ActiveMontage || MontageInstanceID!=ActiveMontageInstanceID ||
        !IsValid(NotifyToken) || ActiveCommitToken.Get()!=NotifyToken)
    {
        return;
    }
    AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetMontageInstanceForID(MontageInstanceID) : nullptr;
    if (!IsValid(Body) || Mesh!=Body->GetMesh() || !Instance || !Instance->IsActive() ||
        Instance->Montage!=ActiveMontage || Instance->GetPosition()+KINDA_SMALL_NUMBER<ActiveCommitTime ||
        !Body->GetWorld() || Body->GetWorld()->IsPaused())
    {
        return;
    }
    // Seal the attempt before any framework / attribute callbacks can re-enter.
    bCommitAttempted=true;
    const uint32 Serial=ActivationSerial;const FString BodyName=GetNameSafe(Body);
    UAbilitySystemComponent* ASC=nullptr;
    if (!ReadContext(CurrentActorInfo,Body,ASC) || ASC->GetAnimatingAbility()!=this ||
        ASC->GetCurrentMontage()!=ActiveMontage)
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);return;
    }
    const bool bFrameworkCommitted=CommitAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo);
    if (!IsActive() || Serial!=ActivationSerial || bEnding)
    {
        return;
    }
    if (!bFrameworkCommitted)
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);
        return;
    }
    if (!ReadContext(CurrentActorInfo,Body,ASC))
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);
        return;
    }
    UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
    const bool bApplied=IsValid(Resource) && Resource->TryApplyHealing(ActiveHealAmount);
    // Log the captured attempt even if the successful GE synchronously ended this ability.
    UE_LOG(LogETHeal,Display,TEXT("[M2-04B2] Commit Body=%s Serial=%u Applied=%d"),*BodyName,Serial,bApplied ? 1 : 0);
    if (!IsActive() || Serial!=ActivationSerial || bEnding)
    {
        return;
    }
    if (!bApplied)
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);
    }
}

void UETGA_Heal::ReleaseMontage()
{
    const uint32 Serial=ActivationSerial;
    UAbilityTask_PlayMontageAndWait* OldTask=MontageTask;
    UAnimMontage* OldMontage=ActiveMontage;const int32 OldID=ActiveMontageInstanceID;
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC=CurrentActorInfo
        ? CurrentActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    const TWeakObjectPtr<UAnimInstance> WeakAnim=CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
    UAnimInstance* Anim=WeakAnim.Get();
    FAnimMontageInstance* Instance=IsValid(Anim) && OldID!=INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    MontageTask=nullptr;ActiveMontage=nullptr;ActiveMontageInstanceID=INDEX_NONE;ActiveCommitToken.Reset();
    ActiveHealAmount=0.0f;ActiveCommitTime=0.0f;
    if (IsValid(OldTask))
    {
        OldTask->OnCompleted.RemoveAll(this);OldTask->OnInterrupted.RemoveAll(this);OldTask->OnCancelled.RemoveAll(this);
        if (Instance)
        {
            if (Instance->OnMontageBlendedInEnded.IsBoundToObject(OldTask)) { Instance->OnMontageBlendedInEnded.Unbind(); }
            if (Instance->OnMontageBlendingOutStarted.IsBoundToObject(OldTask)) { Instance->OnMontageBlendingOutStarted.Unbind(); }
            if (Instance->OnMontageEnded.IsBoundToObject(OldTask)) { Instance->OnMontageEnded.Unbind(); }
        }
        OldTask->EndTask();OldTask->Ability=nullptr;
    }
    UAbilitySystemComponent* ASC=WeakASC.Get();Anim=WeakAnim.Get();
    Instance=IsValid(Anim) && OldID!=INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    if (Serial==ActivationSerial && IsValid(ASC) && IsValid(OldMontage) &&
        ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==OldMontage && (Instance || OldID==INDEX_NONE))
    {
        ASC->CurrentMontageStop(.10f);
        if (Serial==ActivationSerial && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==OldMontage)
        { ASC->ClearAnimatingAbility(this); }
    }
}

void UETGA_Heal::HandleCompleted()
{
    if (IsActive() && !bEnding)
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,false);
    }
}
void UETGA_Heal::HandleAborted()
{
    if (IsActive() && !bEnding)
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);
    }
}
void UETGA_Heal::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle,ActorInfo) || bEnding) { return; }
    if (ScopeLockCount>0)
    {
        Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);return;
    }
    const uint32 EndedSerial=ActivationSerial;const bool bAttempted=bCommitAttempted;
    const FString BodyName=GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    const TWeakObjectPtr<UAbilitySystemComponent> ASC=ActorInfo ? ActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();

    bEnding=true;
    bStarting=false;
    ++ActivationSerial;
    ReleaseMontage();

    Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);
    // No instance-field resets after Super: an End callback may have begun a new activation.
    UE_LOG(LogETHeal,Display,TEXT("[M2-04B2] End Body=%s Serial=%u Cancelled=%d CommitAttempted=%d HealingTag=%d"),
        *BodyName,EndedSerial,bWasCancelled ? 1 : 0,bAttempted ? 1 : 0,
        ASC.IsValid() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing) ? 1 : 0);
}
