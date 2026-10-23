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