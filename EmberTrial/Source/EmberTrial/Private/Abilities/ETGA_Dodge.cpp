#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETDodge, Log, All);

UETGA_Dodge::UETGA_Dodge()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Dodge);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
}

bool UETGA_Dodge::ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost)
{
    OutCost = 0.0f;
    if (!IsValid(Effect) || !FMath::IsFinite(Level) || Level < 1.0f ||
        Effect->DurationPolicy != EGameplayEffectDurationType::Instant ||
        Effect->Modifiers.Num() != 1 || Effect->Executions.Num() != 0)
    {
        return false;
    }
    const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
    float Magnitude = 0.0f;
    if (Modifier.Attribute != UETStaminaAttributeSet::GetStaminaAttribute() ||
        Modifier.ModifierOp != EGameplayModOp::AddBase ||
        Modifier.ModifierMagnitude.GetMagnitudeCalculationType() !=
            EGameplayEffectMagnitudeCalculation::ScalableFloat ||
        !Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(Level, Magnitude) ||
        !FMath::IsFinite(Magnitude) || Magnitude > -1.0f || Magnitude < -100.0f)
    {
        return false;
    }
    OutCost = -Magnitude;
    return true;
}

bool UETGA_Dodge::CheckCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    if (!IsValid(ASC) || !IsValid(Body) || Body->IsActorBeingDestroyed() ||
        !Body->HasAuthority() || Body->IsCombatDead() ||
        ActorInfo->OwnerActor.Get() != Body || Body->GetAbilitySystemComponent() != ASC ||
        !Body->GetWorld() || Body->GetWorld()->GetNetMode() != NM_Standalone ||
        UGameplayStatics::IsGamePaused(Body) ||
        !Body->GetGameplayStaminaSnapshot(Stamina, MaxStamina) ||
        !FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina) ||
        MaxStamina < 1.0f || Stamina < 0.0f || Stamina > MaxStamina ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost) ||
        Stamina < Cost)
    {
        return false;
    }
    return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UETGA_Dodge::ApplyCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    if (bCostSubmitted || !CheckCost(Handle, ActorInfo, nullptr)) { return; }
    bCostSubmitted = true; // Guard BEFORE the GE can synchronously notify observers.
    Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

bool UETGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !CheckCost(Handle, ActorInfo, OptionalRelevantTags) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    APlayerController* PC = IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UCharacterMovementComponent* Movement = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice = IsValid(Body)
        ? Body->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    return IsValid(Body) && Body->GetCombatTeam() == EETCombatTeam::Player &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Body && !PC->IsMoveInputIgnored() &&
        Body->IsFormalActionIdle() && (!Practice || Practice->IsPracticeActionIdle()) &&
        IsValid(Movement) && Movement->IsMovingOnGround() && !Body->GetVelocity().ContainsNaN() &&
        IsValid(DodgeMontage) && FMath::IsFinite(DodgeMontage->GetPlayLength()) &&
        DodgeMontage->GetPlayLength() > 0.0f && IsValid(Anim) && !Anim->GetCurrentActiveMontage();
}

void UETGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    const uint32 Serial = ++ActivationSerial;
    bCostSubmitted = false;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    if (!IsValid(Body) || !IsValid(ASC) || !IsValid(DodgeMontage) || Body->IsCombatDead() ||
        !Body->GetGameplayStaminaSnapshot(Before, MaxStamina) ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    if (UCharacterMovementComponent* Movement = Body->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, DodgeMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!MontageTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->ReadyForActivation();
    if (Serial != ActivationSerial || !IsActive()) { return; }
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    if (!IsValid(Anim) || !Anim->GetActiveInstanceForMontage(DodgeMontage) ||
        ASC->GetAnimatingAbility() != this || !Anim->Montage_IsPlaying(DodgeMontage))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    // A real montage instance exists. Commit now; any earlier failure spent nothing.
    const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
    if (Serial != ActivationSerial || !IsActive()) { return; }
    float After = 0.0f;
    if (!bCommitted || !bCostSubmitted || Body->IsCombatDead() ||
        !Body->GetGameplayStaminaSnapshot(After, MaxStamina) ||
        !FMath::IsNearlyEqual(Before - After, Cost, 0.001f))
    {
        UE_LOG(LogETDodge, Warning, TEXT("[M2-02A] Commit failed Body=%s Submitted=%d"),
            *GetNameSafe(Body), bCostSubmitted ? 1 : 0);
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] Begin Body=%s Serial=%u Before=%.1f After=%.1f Cost=%.1f Dodging=%d"),
        *GetNameSafe(Body), Serial, Before, After, Cost, ASC->GetTagCount(ETGameplayTags::State_Dodging));
}

void UETGA_Dodge::HandleMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_Dodge::HandleMontageAborted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UETGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }
    ++ActivationSerial;
    if (IsValid(MontageTask))
    {
        MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
        MontageTask->OnCancelled.RemoveAll(this);
    }
    MontageTask = nullptr;
    const FString BodyName = GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    const TWeakObjectPtr<UAbilitySystemComponent> ASC = ActorInfo
        ? ActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    const bool bSubmitted = bCostSubmitted;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] End Body=%s Cancelled=%d CostSubmitted=%d Dodging=%d"),
        *BodyName, bWasCancelled ? 1 : 0, bSubmitted ? 1 : 0,
        ASC.IsValid() ? ASC->GetTagCount(ETGameplayTags::State_Dodging) : 0);
}