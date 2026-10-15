#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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
    else if (!Cast<AAIController>(BodyController) ||
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
    return IsValid(AttackMontage) && IsValid(AnimInstance) &&
        IsValid(Trace) && Trace->CanStartAttack() &&
        AnimInstance->GetCurrentActiveMontage() == nullptr;
}

void UETGA_LightAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    AEmberTrialCharacter* Character = ActorInfo
        ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UETCombatTraceComponent* Trace = IsValid(Character)
        ? Character->FindComponentByClass<UETCombatTraceComponent>() : nullptr;
    if (!IsValid(Character) || Character->IsCombatDead() ||
    !IsValid(AttackMontage) || !IsValid(Trace) ||
        !Trace->CanStartAttack() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }

    Character->StopJumping();
    Character->ConsumeMovementInputVector();
    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }

    ActiveTrace = Trace;
    ActiveTraceSerial = 0;
    TraceHitHandle = Trace->OnTraceHit.AddUObject(this, &ThisClass::HandleTraceHit);
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, AttackMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!MontageTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageAborted);
    UE_LOG(LogETLightAttack, Display,
        TEXT("[M1-04A] Begin Character=%s Ability=%s Montage=%s"),
        *GetNameSafe(Character), *GetNameSafe(this), *GetNameSafe(AttackMontage.Get()));
    MontageTask->ReadyForActivation();
    if (!IsActive())
    {
        return;
    }

    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    FAnimMontageInstance* Instance = IsValid(Anim)
        ? Anim->GetActiveInstanceForMontage(AttackMontage) : nullptr;
    UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!Instance || !IsValid(ASC) || ASC->GetAnimatingAbility() != this)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    ActiveTraceSerial = Trace->BeginAttack(AttackMontage, Instance->GetInstanceID());
    if (ActiveTraceSerial == 0)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
    }
}

void UETGA_LightAttack::HandleMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_LightAttack::HandleMontageAborted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UETGA_LightAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo))
    {
        return;
    }
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo,
            bReplicateEndAbility, bWasCancelled);
        return;
    }

    if (UETCombatTraceComponent* Trace = ActiveTrace.Get())
    {
        Trace->OnTraceHit.Remove(TraceHitHandle);
        Trace->EndAttack(ActiveTraceSerial);
    }
    TraceHitHandle.Reset();
    ActiveTraceSerial = 0;
    ActiveTrace.Reset();

    const FString CharacterName = GetNameSafe(ActorInfo->AvatarActor.Get());
    const TWeakObjectPtr<UAbilitySystemComponent> ASC = ActorInfo->AbilitySystemComponent;
    MontageTask = nullptr;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo,
        bReplicateEndAbility, bWasCancelled);

    UE_LOG(LogETLightAttack, Display,
        TEXT("[M1-04A] End Character=%s Cancelled=%d AttackingTag=%d"),
        *CharacterName, bWasCancelled ? 1 : 0,
        ASC.IsValid() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ? 1 : 0);
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
    if (!HasValidDamageConfiguration())
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
    const float RequestedDamage = AttackDamage;
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