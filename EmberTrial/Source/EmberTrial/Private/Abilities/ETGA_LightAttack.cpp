#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "Animation/ETANS_AttackWindow.h"
#include "Animation/ETANS_ComboWindow.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/ETCombatTraceComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "GameplayEffect.h"
#include "AIController.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogETLightAttack, Log, All);

UETGA_LightAttack::UETGA_LightAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;

    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Attack_Light);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Healing);
}

bool UETGA_LightAttack::HasValidDamageConfiguration() const
{
    if (!FMath::IsFinite(AttackDamage) || AttackDamage <= 0.0f ||
        !IsValid(DamageEffectClass.Get()))
    {
        return false;
    }

    const UGameplayEffect* Effect = DamageEffectClass.GetDefaultObject();
    if (!IsValid(Effect) ||
        Effect->DurationPolicy != EGameplayEffectDurationType::Instant ||
        Effect->Modifiers.Num() != 1 || Effect->Executions.Num() != 0)
    {
        return false;
    }

    const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
    if (Modifier.Attribute != UETHealthAttributeSet::GetHealthAttribute() ||
        Modifier.ModifierOp != EGameplayModOp::AddBase ||
        Modifier.ModifierMagnitude.GetMagnitudeCalculationType() !=
            EGameplayEffectMagnitudeCalculation::SetByCaller)
    {
        return false;
    }

    return Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag ==
        ETGameplayTags::SetByCaller_HealthDelta;
}

bool UETGA_LightAttack::HasValidComboConfiguration() const
{
    if (!bEnableCombo) { return true; }
    if (!FMath::IsFinite(ComboBufferSeconds) || ComboBufferSeconds < .05f ||
        ComboBufferSeconds > .50f || ComboFollowUps.Num() != 2 || !IsValid(AttackMontage)) { return false; }
    TArray<UAnimMontage*> Montages{AttackMontage.Get()};
    for (const FETLightAttackStage& Stage : ComboFollowUps)
    {
        if (!IsValid(Stage.Montage) || !FMath::IsFinite(Stage.DamageMultiplier) ||
            Stage.DamageMultiplier <= 0.0f || !FMath::IsFinite(AttackDamage * Stage.DamageMultiplier) ||
            Montages.Contains(Stage.Montage.Get())) { return false; }
        Montages.Add(Stage.Montage.Get());
    }
    for (int32 I=0; I<Montages.Num(); ++I)
    {
        const UAnimMontage* Montage=Montages[I];
        const float Length=Montage->GetPlayLength();
        if (!IsValid(Montage->GetSkeleton()) || Montage->GetSkeleton()!=AttackMontage->GetSkeleton() ||
            !FMath::IsFinite(Length) || Length<=0.0f || Montage->HasRootMotion() ||
            !FMath::IsNearlyEqual(Montage->RateScale,1.0f) ||
            !Montage->SlotAnimTracks.ContainsByPredicate([](const FSlotAnimationTrack& T)
                { return T.SlotName==FName(TEXT("DefaultSlot")); })) { return false; }
        int32 HitCount=0,ComboCount=0;
        float HitEnd=0.0f,ComboBegin=0.0f,ComboEnd=0.0f;
        for (const FAnimNotifyEvent& Event : Montage->Notifies)
        {
            const UAnimNotifyState* State=Event.NotifyStateClass;
            if (!IsValid(State)) { continue; }
            const bool bHit=State->IsA<UETANS_AttackWindow>();
            const bool bCombo=State->IsA<UETANS_ComboWindow>();
            if (!bHit && !bCombo) { continue; }
            const float Begin=Event.GetTime(),Duration=Event.GetDuration(),End=Begin+Duration;
            if (!FMath::IsFinite(Begin) || !FMath::IsFinite(Duration) || Begin<0 ||
                Duration<=0 || End>Length+KINDA_SMALL_NUMBER) { return false; }
            if (bHit) { ++HitCount;HitEnd=End; }
            if (bCombo) { ++ComboCount;ComboBegin=Begin;ComboEnd=End; }
        }
        if (HitCount!=1 || ComboCount!=(I<2 ? 1 : 0)) { return false; }
        if (I<2 && (ComboBegin<HitEnd || ComboEnd>=Length-Montage->BlendOut.GetBlendTime())) { return false; }
    }
    return true;
}

bool UETGA_LightAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !Super::CanActivateAbility(
        Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    AEmberTrialCharacter* BodyCharacter =
        Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    if (!IsValid(BodyCharacter) || BodyCharacter->IsActorBeingDestroyed() ||
        BodyCharacter->IsCombatDead() || !BodyCharacter->IsGameplayReady() ||
        ActorInfo->OwnerActor.Get() != BodyCharacter ||
        BodyCharacter->GetAbilitySystemComponent() !=
            ActorInfo->AbilitySystemComponent.Get())
    {
        return false;
    }

    UWorld* World = BodyCharacter->GetWorld();
    if (!IsValid(World) || World->GetNetMode() != NM_Standalone ||
        !BodyCharacter->HasAuthority() ||
        UGameplayStatics::IsGamePaused(BodyCharacter))
    {
        return false;
    }

    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (!BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth) ||
        !FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) ||
        MaxHealth < 1.0f || Health <= 0.0f || Health > MaxHealth)
    {
        return false;
    }

    AController* BodyController = BodyCharacter->GetController();
    if (!IsValid(BodyController) || BodyController->GetPawn() != BodyCharacter)
    {
        return false;
    }
    if (const APlayerController* PlayerController =
        Cast<APlayerController>(BodyController))
    {
        if (!PlayerController->IsLocalController() ||
            PlayerController->IsMoveInputIgnored() ||
            BodyCharacter->GetCombatTeam() != EETCombatTeam::Player)
        {
            return false;
        }
    }
    else if (!Cast<AAIController>(BodyController) || bEnableCombo ||
        BodyCharacter->GetCombatTeam() != EETCombatTeam::Enemy)
    {
        return false;
    }

    UCharacterMovementComponent* Movement = BodyCharacter->GetCharacterMovement();
    const FVector Velocity = BodyCharacter->GetVelocity();
    if (!IsValid(Movement) || !Movement->IsMovingOnGround() ||
        Velocity.ContainsNaN() || !FMath::IsFinite(AttackMaxHorizontalSpeed) ||
        AttackMaxHorizontalSpeed < 0.0f ||
        Velocity.SizeSquared2D() > FMath::Square(AttackMaxHorizontalSpeed))
    {
        return false;
    }

    const UETPracticeActionComponent* Practice =
        BodyCharacter->FindComponentByClass<UETPracticeActionComponent>();
    if (Practice && !Practice->IsPracticeActionIdle())
    {
        return false;
    }

    if (!HasValidDamageConfiguration())
    {
        UE_LOG(LogETLightAttack, Warning,
            TEXT("[M1-04B2] Invalid damage config Character=%s Effect=%s Damage=%.1f"),
            *GetNameSafe(BodyCharacter), *GetNameSafe(DamageEffectClass.Get()),
            AttackDamage);
        return false;
    }

    const UETCombatTraceComponent* Trace =
        BodyCharacter->FindComponentByClass<UETCombatTraceComponent>();
    UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
    return !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Healing) &&
        HasValidComboConfiguration() && IsValid(AttackMontage) && IsValid(AnimInstance) &&
        IsValid(Trace) && Trace->CanStartAttack() &&
        AnimInstance->GetCurrentActiveMontage() == nullptr;
}

bool UETGA_LightAttack::HasNextStage() const
{
    return bEnableCombo && ActiveStageIndex>=0 && RuntimeStages.IsValidIndex(ActiveStageIndex+1);
}

bool UETGA_LightAttack::CanAcceptComboInput() const
{
    if (!IsActive() || !CurrentActorInfo || !bEnableCombo || bEndingAttack ||
        bStartingStage || !HasNextStage()) { return false; }
    const AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    const APlayerController* PC=IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UAbilitySystemComponent* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    const UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    const FAnimMontageInstance* Instance=IsValid(Anim) && IsValid(ActiveMontage)
        ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    return IsValid(Body) && !Body->IsActorBeingDestroyed() && Body->IsGameplayReady() &&
        !Body->IsCombatDead() && Body->HasAuthority() && Body->GetCombatTeam()==EETCombatTeam::Player &&
        CurrentActorInfo->OwnerActor.Get()==Body && Body->GetAbilitySystemComponent()==ASC &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn()==Body && !PC->IsMoveInputIgnored() &&
        !UGameplayStatics::IsGamePaused(Body) && IsValid(ASC) && ASC->GetAnimatingAbility()==this &&
        ASC->GetCurrentMontage()==ActiveMontage && Instance && Instance->GetInstanceID()==ActiveMontageInstanceID;
}

bool UETGA_LightAttack::CanCancelIntoDodge() const
{
    if (!IsActive() || !CanBeCanceled() || !bEnableCombo || !CurrentActorInfo ||
        bStartingStage || bEndingAttack || !RuntimeStages.IsValidIndex(ActiveStageIndex)) { return false; }
    const AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    const APlayerController* PC=IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UAbilitySystemComponent* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    const UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    const FAnimMontageInstance* Instance=IsValid(Anim) && IsValid(ActiveMontage)
        ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    if (!IsValid(Body) || Body->IsActorBeingDestroyed() || !Body->IsGameplayReady() ||
        Body->IsCombatDead() || !Body->HasAuthority() || Body->GetCombatTeam()!=EETCombatTeam::Player ||
        !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn()!=Body || PC->IsMoveInputIgnored() ||
        CurrentActorInfo->OwnerActor.Get()!=Body || Body->GetAbilitySystemComponent()!=ASC ||
        !IsValid(ASC) || UGameplayStatics::IsGamePaused(Body) || ASC->GetAnimatingAbility()!=this ||
        ASC->GetCurrentMontage()!=ActiveMontage || !Instance ||
        Instance->GetInstanceID()!=ActiveMontageInstanceID ||
        Anim->GetCurrentActiveMontage()!=ActiveMontage || !Anim->Montage_IsPlaying(ActiveMontage)) { return false; }
    float HitEnd=0.0f;int32 Hits=0;
    for (const FAnimNotifyEvent& Event : ActiveMontage->Notifies)
    {
        if (IsValid(Event.NotifyStateClass) && Event.NotifyStateClass->IsA<UETANS_AttackWindow>())
        { ++Hits;HitEnd=Event.GetTime()+Event.GetDuration(); }
    }
    const float Position=Instance->GetPosition();
    const float RecoveryEnd=ActiveMontage->GetPlayLength()-ActiveMontage->BlendOut.GetBlendTime();
    return Hits==1 && FMath::IsFinite(HitEnd) && FMath::IsFinite(Position) &&
        FMath::IsFinite(RecoveryEnd) && HitEnd>0.0f && Position>=HitEnd && Position<RecoveryEnd;
}

void UETGA_LightAttack::ClearBufferedInput()
{
    bHasBufferedInput=false;
    BufferedInputExpiresAt=0.0;
}

bool UETGA_LightAttack::TryBufferComboInput()
{
    if (!CanAcceptComboInput() || bTransitionPending || bComboWindowPassed) { return false; }
    UWorld* World=GetWorld();
    if (!IsValid(World)) { return false; }
    bHasBufferedInput=true;
    BufferedInputExpiresAt=double(World->GetTimeSeconds())+ComboBufferSeconds;
    UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Input buffered Stage=%d Expires=%.3f"),
        ActiveStageIndex+1,BufferedInputExpiresAt);
    TryConsumeBufferedInput();
    return true; // Accepted input can still expire before the window opens.
}

void UETGA_LightAttack::HandleComboWindow(USkeletalMeshComponent* Mesh,
    UAnimSequenceBase* Animation,int32 MontageInstanceID,UAnimNotifyState* WindowToken,bool bOpen)
{
    const AEmberTrialCharacter* Body=CurrentActorInfo
        ? Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
    if (!IsActive() || bStartingStage || bEndingAttack || !HasNextStage() || !IsValid(Body) ||
        Mesh!=Body->GetMesh() || Animation!=ActiveMontage || MontageInstanceID!=ActiveMontageInstanceID ||
        !IsValid(WindowToken) || !WindowToken->IsA<UETANS_ComboWindow>()) { return; }
    if (bOpen)
    {
        if (bComboWindowOpen || bComboWindowPassed) { return; }
        ActiveComboWindow=WindowToken;bComboWindowOpen=true;
        UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Combo window open Stage=%d Ticket=%u"),ActiveStageIndex+1,StageSerial);
        TryConsumeBufferedInput();
    }
    else if (bComboWindowOpen && ActiveComboWindow.Get()==WindowToken)
    {
        bComboWindowOpen=false;bComboWindowPassed=true;ActiveComboWindow.Reset();ClearBufferedInput();
    }
}

void UETGA_LightAttack::TryConsumeBufferedInput()
{
    if (!bComboWindowOpen || !bHasBufferedInput || bTransitionPending || !CanAcceptComboInput()) { return; }
    UWorld* World=GetWorld();
    if (!IsValid(World)) { return; }
    if (double(World->GetTimeSeconds())>BufferedInputExpiresAt)
    {
        ClearBufferedInput();
        UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Buffered input expired Stage=%d"),ActiveStageIndex+1);
        return;
    }
    ClearBufferedInput();bTransitionPending=true;
    TransitionTimer=World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
        this,&ThisClass::HandleQueuedTransition,ActivationSerial,StageSerial));
}

void UETGA_LightAttack::HandleQueuedTransition(uint32 ExpectedActivation,uint32 ExpectedStage)
{
    if (!IsActive() || bEndingAttack || ExpectedActivation!=ActivationSerial ||
        ExpectedStage!=StageSerial || !bTransitionPending) { return; }
    TransitionTimer.Invalidate();
    UWorld* World=GetWorld();
    if (IsValid(World) && UGameplayStatics::IsGamePaused(this))
    {
        TransitionTimer=World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
            this,&ThisClass::HandleQueuedTransition,ExpectedActivation,ExpectedStage));
        return;
    }
    if (!CanAcceptComboInput())
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);return;
    }
    const int32 Next=ActiveStageIndex+1;
    const bool bStarted=StartStage(Next);
    if (ExpectedActivation!=ActivationSerial || !IsActive()) { return; }
    if (!bStarted) { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true); }
}

void UETGA_LightAttack::ClearStageResources()
{
    const uint32 ClearingActivation=ActivationSerial;
    const uint32 ClearingStage=StageSerial;
    UAbilityTask_PlayMontageAndWait* OldTask=MontageTask;
    UAnimMontage* OldMontage=ActiveMontage;
    const int32 OldID=ActiveMontageInstanceID;
    const uint32 OldTraceSerial=ActiveTraceSerial;
    MontageTask=nullptr;ActiveMontage=nullptr;ActiveMontageInstanceID=INDEX_NONE;
    ActiveTraceSerial=0;ActiveStageDamage=0.0f;
    bComboWindowOpen=false;bComboWindowPassed=false;ActiveComboWindow.Reset();
    if (UETCombatTraceComponent* Trace=ActiveTrace.Get()) { Trace->EndAttack(OldTraceSerial); }
    UAbilitySystemComponent* ASC=CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
    UAnimInstance* Anim=CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
    FAnimMontageInstance* Instance=IsValid(Anim) && OldID!=INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    if (IsValid(OldTask))
    {
        OldTask->OnCompleted.RemoveAll(this);OldTask->OnInterrupted.RemoveAll(this);OldTask->OnCancelled.RemoveAll(this);
        // Native task callbacks can otherwise fire after EndTask while a new stage is active.
        if (Instance)
        {
            if (Instance->OnMontageBlendedInEnded.IsBoundToObject(OldTask)) { Instance->OnMontageBlendedInEnded.Unbind(); }
            if (Instance->OnMontageBlendingOutStarted.IsBoundToObject(OldTask)) { Instance->OnMontageBlendingOutStarted.Unbind(); }
            if (Instance->OnMontageEnded.IsBoundToObject(OldTask)) { Instance->OnMontageEnded.Unbind(); }
        }
        OldTask->EndTask();
        // Finish normal cancellation-listener / task-owner cleanup first.
        // A queued copy of a native montage delegate may still call this retired task.
        // Its old Ability must not point at a new activation using the same montage.
        OldTask->Ability=nullptr;
    }
    if (ClearingActivation==ActivationSerial && ClearingStage==StageSerial &&
        IsValid(ASC) && IsValid(OldMontage) && ASC->GetAnimatingAbility()==this &&
        ASC->GetCurrentMontage()==OldMontage && (Instance || OldID==INDEX_NONE))
    {
        ASC->CurrentMontageStop(.10f);
        // Stop can synchronously cancel or reactivate this instanced ability.
        if (ClearingActivation==ActivationSerial && ClearingStage==StageSerial && IsValid(ASC) &&
            ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==OldMontage)
        { ASC->ClearAnimatingAbility(this); }
    }
}

bool UETGA_LightAttack::StartStage(int32 Index)
{
    if (!IsActive() || bEndingAttack || !RuntimeStages.IsValidIndex(Index) || !CurrentActorInfo) { return false; }
    const uint32 Activation=ActivationSerial;
    bStartingStage=true;bTransitionPending=false;ClearBufferedInput();
    ++StageSerial;if (StageSerial==0) { ++StageSerial; }
    const uint32 Ticket=StageSerial;
    ClearStageResources();
    if (!IsActive() || Activation!=ActivationSerial || Ticket!=StageSerial || bEndingAttack) { return false; }
    UAbilitySystemComponent* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    UETCombatTraceComponent* Trace=ActiveTrace.Get();
    UAnimMontage* Montage=RuntimeStages[Index].Montage;
    if (!IsValid(ASC) || !IsValid(Anim) || !IsValid(Trace) || !Trace->CanStartAttack() || !IsValid(Montage)) { return false; }
    ActiveStageIndex=Index;ActiveMontage=Montage;
    ActiveStageDamage=AttackDamage*RuntimeStages[Index].DamageMultiplier;
    UAbilityTask_PlayMontageAndWait* Task=UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this,NAME_None,Montage,1.0f,NAME_None,true,1.0f,0.0f,true);
    if (!Task) { return false; }
    MontageTask=Task;
    Task->OnCompleted.AddDynamic(this,&ThisClass::HandleMontageCompleted);
    Task->OnInterrupted.AddDynamic(this,&ThisClass::HandleMontageAborted);
    Task->OnCancelled.AddDynamic(this,&ThisClass::HandleMontageAborted);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC(ASC);
    const TWeakObjectPtr<UAnimInstance> WeakAnim(Anim);
    Task->ReadyForActivation();
    ASC=WeakASC.Get();Anim=WeakAnim.Get();
    if (!IsActive() || Activation!=ActivationSerial || Ticket!=StageSerial)
    {
        // PlayMontage may assign ASC ownership after a synchronous cancellation.
        // Do not touch a new activation of this instanced ability.
        if (!IsActive() && IsValid(ASC) && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==Montage)
        {
            ASC->CurrentMontageStop(0.0f);
            if (!IsActive() && IsValid(ASC) && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==Montage)
            { ASC->ClearAnimatingAbility(this); }
        }
        return false;
    }
    FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
    if (!IsValid(ASC) || !Instance || !Anim->Montage_IsPlaying(Montage) || MontageTask!=Task || ASC->GetAnimatingAbility()!=this) { return false; }
    ActiveMontageInstanceID=Instance->GetInstanceID();
    ActiveTraceSerial=Trace->BeginAttack(Montage,ActiveMontageInstanceID);
    bStartingStage=false;
    if (ActiveTraceSerial==0) { return false; }
    UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Stage begin Body=%s Stage=%d/%d Ticket=%u TraceSerial=%u Damage=%.1f Montage=%s"),
        *GetNameSafe(GetAvatarActorFromActorInfo()),Index+1,RuntimeStages.Num(),Ticket,ActiveTraceSerial,ActiveStageDamage,*GetNameSafe(Montage));
    return true;
}

void UETGA_LightAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    ++ActivationSerial;if (ActivationSerial==0) { ++ActivationSerial; }
    const uint32 Activation=ActivationSerial;
    bEndingAttack=false;bStartingStage=true;bTransitionPending=false;ClearBufferedInput();
    ActiveStageIndex=INDEX_NONE;RuntimeStages.Reset();
    AEmberTrialCharacter* Body=ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UETCombatTraceComponent* Trace=IsValid(Body) ? Body->FindComponentByClass<UETCombatTraceComponent>() : nullptr;
    if (!IsValid(Body) || Body->IsCombatDead() || !IsValid(Trace) || !Trace->CanStartAttack() ||
        !HasValidDamageConfiguration() || !HasValidComboConfiguration() || !IsValid(AttackMontage))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    FETLightAttackStage First;First.Montage=AttackMontage;First.DamageMultiplier=1.0f;
    RuntimeStages.Add(First);
    if (bEnableCombo) { RuntimeStages.Append(ComboFollowUps); }
    const bool bCommitted=CommitAbility(Handle,ActorInfo,ActivationInfo);
    if (!IsActive() || Activation!=ActivationSerial) { return; }
    if (!bCommitted) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    Body=Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    if (!IsValid(Body) || Body->IsCombatDead()) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    Body->StopJumping();Body->ConsumeMovementInputVector();
    if (UCharacterMovementComponent* Movement=Body->GetCharacterMovement()) { Movement->StopMovementImmediately(); }
    ActiveTrace=Trace;TraceHitHandle=Trace->OnTraceHit.AddUObject(this,&ThisClass::HandleTraceHit);
    const bool bStarted=StartStage(0);
    if (!IsActive() || Activation!=ActivationSerial) { return; }
    if (!bStarted) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    UE_LOG(LogETLightAttack,Display,TEXT("[M1-04A] Begin Character=%s Ability=%s Montage=%s"),
        *GetNameSafe(Body),*GetNameSafe(this),*GetNameSafe(AttackMontage));
}

void UETGA_LightAttack::HandleMontageCompleted()
{
    EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,false);
}
void UETGA_LightAttack::HandleMontageAborted()
{
    EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);
}
void UETGA_LightAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle,ActorInfo) || bEndingAttack) { return; }
    if (ScopeLockCount>0)
    { Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);return; }
    bEndingAttack=true;++ActivationSerial;++StageSerial;
    if (UWorld* World=GetWorld()) { World->GetTimerManager().ClearTimer(TransitionTimer); }
    TransitionTimer.Invalidate();bTransitionPending=false;bStartingStage=false;ClearBufferedInput();
    if (UETCombatTraceComponent* Trace=ActiveTrace.Get()) { Trace->OnTraceHit.Remove(TraceHitHandle); }
    TraceHitHandle.Reset();
    ClearStageResources();ActiveTrace.Reset();ActiveStageIndex=INDEX_NONE;RuntimeStages.Reset();
    const FString BodyName=GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    const TWeakObjectPtr<UAbilitySystemComponent> ASC=ActorInfo ? ActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);
    // No instance-field resets here: an End event may already have started a new activation.
    UE_LOG(LogETLightAttack,Display,TEXT("[M1-04A] End Character=%s Cancelled=%d AttackingTag=%d"),
        *BodyName,bWasCancelled ? 1 : 0,ASC.IsValid() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ? 1 : 0);
}

void UETGA_LightAttack::HandleTraceHit(uint32 Serial, const FHitResult& Hit)
{
    if (!IsActive() || Serial == 0 || Serial != ActiveTraceSerial || !CurrentActorInfo)
    {
        return;
    }

    const FString TargetName = GetNameSafe(Hit.GetActor());
    const auto Reject = [Serial, &TargetName](const TCHAR* Reason)
    {
        UE_LOG(LogETLightAttack, Display,
            TEXT("[M1-04B2] Damage skipped Serial=%u Target=%s Reason=%s"),
            Serial, *TargetName, Reason);
    };

    AEmberTrialCharacter* Source =
        Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    UAbilitySystemComponent* SourceASC = CurrentActorInfo->AbilitySystemComponent.Get();
    float SourceHealth = 0.0f;
    float SourceMaxHealth = 0.0f;
    if (!IsValid(Source) || Source->IsCombatDead() || !Source->HasAuthority() || !IsValid(SourceASC) ||
        CurrentActorInfo->OwnerActor.Get() != Source ||
        Source->GetAbilitySystemComponent() != SourceASC ||
        !Source->GetGameplayHealthSnapshot(SourceHealth, SourceMaxHealth) ||
        !FMath::IsFinite(SourceHealth) || !FMath::IsFinite(SourceMaxHealth) ||
        SourceHealth <= 0.0f || SourceMaxHealth < 1.0f ||
        SourceHealth > SourceMaxHealth || UGameplayStatics::IsGamePaused(Source))
    {
        Reject(TEXT("SourceUnavailable"));
        return;
    }
    if (!HasValidDamageConfiguration() || !FMath::IsFinite(ActiveStageDamage) || ActiveStageDamage <= 0.0f)
    {
        Reject(TEXT("InvalidDamageConfig"));
        return;
    }

    AEmberTrialCharacter* Target = Cast<AEmberTrialCharacter>(Hit.GetActor());
    if (!IsValid(Target) || Target == Source)
    {
        Reject(TEXT("UnsupportedTarget"));
        return;
    }

    if (!Source->CanDealCombatDamageTo(Target))
    {
        Reject(TEXT("CombatTeam"));
        return;
    }

    if (Target->IsCombatDead())
    {
        Reject(TEXT("TargetDead"));
        return;
    }

    float HealthBefore = 0.0f;
    float MaxHealthBefore = 0.0f;
    if (!Target->GetGameplayHealthSnapshot(HealthBefore, MaxHealthBefore) ||
        !FMath::IsFinite(HealthBefore) || !FMath::IsFinite(MaxHealthBefore) ||
        MaxHealthBefore < 1.0f || HealthBefore < 0.0f || HealthBefore > MaxHealthBefore)
    {
        Reject(TEXT("TargetNotReady"));
        return;
    }
    if (HealthBefore <= 0.0f)
    {
        Reject(TEXT("TargetZeroHealth"));
        return;
    }

    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    if (!IsValid(TargetASC) || !TargetASC->AbilityActorInfo.IsValid() ||
        TargetASC->AbilityActorInfo->OwnerActor.Get() != Target ||
        TargetASC->AbilityActorInfo->AvatarActor.Get() != Target ||
        !TargetASC->HasAttributeSetForAttribute(UETHealthAttributeSet::GetHealthAttribute()))
    {
        Reject(TEXT("TargetASCUnavailable"));
        return;
    }

    const FString SourceName = GetNameSafe(Source);
    const float RequestedDamage = ActiveStageDamage;
    const TWeakObjectPtr<AEmberTrialCharacter> WeakTarget(Target);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakTargetASC(TargetASC);

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    if (!Context.IsValid())
    {
        Reject(TEXT("ContextUnavailable"));
        return;
    }
    Context.AddSourceObject(this);
    Context.AddHitResult(Hit, true);
    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
        DamageEffectClass, GetAbilityLevel(), Context);
    if (!Spec.IsValid())
    {
        Reject(TEXT("SpecUnavailable"));
        return;
    }
    Spec.Data->SetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_HealthDelta, -RequestedDamage);

    // This shared player/enemy combat entry checks current tags at actual submission time.
    // Diagnostic direct GE / SetNumericAttributeBase calls intentionally bypass this policy.
    if (TargetASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable))
    {
        Reject(TEXT("Invulnerable"));
        return;
    }

    const FActiveGameplayEffectHandle Result = SourceASC->ApplyGameplayEffectSpecToTarget(
        *Spec.Data.Get(), TargetASC);
    const bool bApplied = Result.WasSuccessfullyApplied();

    // Applying an effect can synchronously cancel abilities or destroy actors.
    // Do not reuse CurrentActorInfo or raw actor pointers after the call.
    float HealthAfter = 0.0f;
    float MaxHealthAfter = 0.0f;
    AEmberTrialCharacter* LiveTarget = WeakTarget.Get();
    UAbilitySystemComponent* LiveTargetASC = WeakTargetASC.Get();
    if (!IsValid(LiveTarget) || !IsValid(LiveTargetASC) ||
        LiveTarget->GetAbilitySystemComponent() != LiveTargetASC ||
        !LiveTarget->GetGameplayHealthSnapshot(HealthAfter, MaxHealthAfter))
    {
        UE_LOG(LogETLightAttack, Display,
            TEXT("[M1-04B2] Damage Serial=%u Source=%s Target=%s Requested=%.1f Applied=%d ActiveHandle=%d Snapshot=Unavailable"),
            Serial, *SourceName, *TargetName, RequestedDamage,
            bApplied ? 1 : 0, Result.IsValid() ? 1 : 0);
        return;
    }

    const float BaseAfter = LiveTargetASC->GetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute());
    UE_LOG(LogETLightAttack, Display,
        TEXT("[M1-04B2] Damage Serial=%u Source=%s Target=%s Requested=%.1f Applied=%d ActiveHandle=%d Before=%.1f After=%.1f BaseAfter=%.1f Max=%.1f Actual=%.1f"),
        Serial, *SourceName, *TargetName, RequestedDamage,
        bApplied ? 1 : 0, Result.IsValid() ? 1 : 0,
        HealthBefore, HealthAfter, BaseAfter, MaxHealthAfter,
        FMath::Max(0.0f, HealthBefore - HealthAfter));
}