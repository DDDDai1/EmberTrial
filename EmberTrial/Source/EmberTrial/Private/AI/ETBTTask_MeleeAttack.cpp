#include "AI/ETBTTask_MeleeAttack.h"
#include "AI/ETAIController.h"
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpec.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMeleeTask, Log, All);

namespace
{
    const FName MeleeTargetKey(TEXT("TargetActor"));

    bool IsMeleeBodyAlive(const AEmberTrialCharacter* BodyCharacter)
    {
        float Health = 0.0f;
        float MaxHealth = 0.0f;
        return IsValid(BodyCharacter) && !BodyCharacter->IsActorBeingDestroyed() &&
            !BodyCharacter->IsCombatDead() &&
            BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth) &&
            FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) &&
            MaxHealth >= 1.0f && Health > 0.0f && Health <= MaxHealth;
    }

    void ApplyMeleeYaw(AETAIController* EnemyController,
        AEmberTrialCharacter* BodyCharacter, float Yaw)
    {
        const FRotator Facing(0.0f, Yaw, 0.0f);
        EnemyController->SetControlRotation(Facing);
        BodyCharacter->SetActorRotation(Facing);
    }
}

UETBTTask_MeleeAttack::UETBTTask_MeleeAttack(
    const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    NodeName = TEXT("ET Melee Attack");
    bCreateNodeInstance = true;
    bNotifyTick = true;
    bNotifyTaskFinished = true;
}

bool UETBTTask_MeleeAttack::HasValidSettings() const
{
    return FMath::IsFinite(TurnRate) && TurnRate >= 1.0f &&
        FMath::IsFinite(FacingTolerance) && FacingTolerance >= 0.1f &&
        FacingTolerance <= 30.0f &&
        FMath::IsFinite(PreparationTimeout) && PreparationTimeout >= 0.1f &&
        FMath::IsFinite(AbilityWatchdog) && AbilityWatchdog >= 0.1f;
}

bool UETBTTask_MeleeAttack::GetLiveContext(UBehaviorTreeComponent& OwnerComp,
    AETAIController*& OutController, AEmberTrialCharacter*& OutBody,
    AEmberTrialCharacter*& OutTarget, UAbilitySystemComponent*& OutASC) const
{
    OutController = ActiveController.Get();
    OutBody = ActiveBody.Get();
    OutTarget = ActiveTarget.Get();
    OutASC = ActiveASC.Get();
    const UBlackboardComponent* Board = OwnerComp.GetBlackboardComponent();
    const UWorld* World = OwnerComp.GetWorld();
    return ActiveOwner.Get() == &OwnerComp && IsValid(OutController) &&
        OwnerComp.GetAIOwner() == OutController && OutController->HasAuthority() &&
        IsMeleeBodyAlive(OutBody) && IsMeleeBodyAlive(OutTarget) &&
        OutController->GetPawn() == OutBody && OutBody->GetController() == OutController &&
        OutController->GetSensedPlayer() == OutTarget &&
        IsValid(Board) && Board->GetValueAsObject(MeleeTargetKey) == OutTarget &&
        OutBody->CanDealCombatDamageTo(OutTarget) && IsValid(OutASC) &&
        OutBody->GetAbilitySystemComponent() == OutASC &&
        OutBody->GetLightAttackSpecHandle() == ActiveSpecHandle &&
        ActiveSpecHandle.IsValid() && IsValid(World) &&
        World->GetNetMode() == NM_Standalone;
}

EBTNodeResult::Type UETBTTask_MeleeAttack::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    ReleaseRequest(true);
    AETAIController* EnemyController = Cast<AETAIController>(OwnerComp.GetAIOwner());
    if (!HasValidSettings() || !IsValid(EnemyController))
    {
        UE_LOG(LogETMeleeTask, Warning,
            TEXT("[M1-05C2] Task configuration rejected Controller=%s"),
            *GetNameSafe(EnemyController));
        return EBTNodeResult::Failed;
    }
    EnemyController->UpdatePursuitDecision();
    AEmberTrialCharacter* BodyCharacter =
        Cast<AEmberTrialCharacter>(EnemyController->GetPawn());
    AEmberTrialCharacter* PlayerBody = EnemyController->GetSensedPlayer();
    UAbilitySystemComponent* BodyASC = IsValid(BodyCharacter)
        ? BodyCharacter->GetAbilitySystemComponent() : nullptr;
    const FGameplayAbilitySpecHandle SpecHandle = IsValid(BodyCharacter)
        ? BodyCharacter->GetLightAttackSpecHandle() : FGameplayAbilitySpecHandle();
    const FGameplayAbilitySpec* Spec = IsValid(BodyASC)
        ? BodyASC->FindAbilitySpecFromHandle(SpecHandle) : nullptr;
    if (!EnemyController->IsMeleeAttackAllowed() || !IsValid(PlayerBody) ||
        !Spec || Spec->IsActive())
    {
        EnemyController->NotifyMeleeAttackAttemptFinished();
        UE_LOG(LogETMeleeTask, Display,
            TEXT("[M1-05C2] Task rejected Controller=%s Reason=GateOrExistingAbility"),
            *GetNameSafe(EnemyController));
        return EBTNodeResult::Failed;
    }

    ActiveOwner = &OwnerComp;
    ActiveController = EnemyController;
    ActiveBody = BodyCharacter;
    ActiveTarget = PlayerBody;
    ActiveASC = BodyASC;
    ActiveSpecHandle = SpecHandle;
    bRunning = true;
    Phase = EPhase::Preparing;
    PhaseElapsed = 0.0f;
    EnemyController->StopMovement();
    if (UCharacterMovementComponent* Movement = BodyCharacter->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
    UE_LOG(LogETMeleeTask, Display,
        TEXT("[M1-05C2] Prepare Body=%s Target=%s Handle=%s"),
        *GetNameSafe(BodyCharacter), *GetNameSafe(PlayerBody),
        *SpecHandle.ToString());
    // Request on the first task tick, after BT has made this task active.
    return EBTNodeResult::InProgress;
}

void UETBTTask_MeleeAttack::TickTask(UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory, float DeltaSeconds)
{
    if (!bRunning)
    {
        return;
    }
    AETAIController* EnemyController = nullptr;
    AEmberTrialCharacter* BodyCharacter = nullptr;
    AEmberTrialCharacter* PlayerBody = nullptr;
    UAbilitySystemComponent* BodyASC = nullptr;
    if (!GetLiveContext(OwnerComp, EnemyController, BodyCharacter, PlayerBody, BodyASC))
    {
        FinishAttempt(EBTNodeResult::Failed, TEXT("ContextLost"), true);
        return;
    }
    if (OwnerComp.GetWorld()->IsPaused())
    {
        return;
    }
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.0f)
    {
        FinishAttempt(EBTNodeResult::Failed, TEXT("InvalidDeltaTime"), true);
        return;
    }
    PhaseElapsed += DeltaSeconds;

    if (Phase == EPhase::Preparing)
    {
        const uint64 PreparationSerial = ExecutionSerial;
        EnemyController->UpdatePursuitDecision();
        if (!bRunning || ExecutionSerial != PreparationSerial)
        {
            return;
        }
        if (!EnemyController->IsMeleeAttackAllowed())
        {
            FinishAttempt(EBTNodeResult::Failed, TEXT("PreparationGateClosed"), false);
            return;
        }
        if (PhaseElapsed > PreparationTimeout)
        {
            FinishAttempt(EBTNodeResult::Failed, TEXT("PreparationTimeout"), false);
            return;
        }
        const FVector Offset = PlayerBody->GetActorLocation() - BodyCharacter->GetActorLocation();
        const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
        if (!FMath::IsFinite(DesiredYaw))
        {
            FinishAttempt(EBTNodeResult::Failed, TEXT("InvalidFacing"), false);
            return;
        }
        const float NewYaw = FMath::FixedTurn(BodyCharacter->GetActorRotation().Yaw,
            DesiredYaw, TurnRate * DeltaSeconds);
        ApplyMeleeYaw(EnemyController, BodyCharacter, NewYaw);
        if (FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, DesiredYaw)) > FacingTolerance)
        {
            return;
        }
        const FGameplayAbilitySpec* Spec = BodyASC->FindAbilitySpecFromHandle(ActiveSpecHandle);
        if (!Spec || Spec->IsActive())
        {
            FinishAttempt(EBTNodeResult::Failed, TEXT("SpecMissingOrAlreadyActive"), false);
            return;
        }

        LockedYaw = NewYaw;
        Phase = EPhase::WaitingForAbility;
        PhaseElapsed = 0.0f;
        AbilityEndedHandle = BodyASC->OnAbilityEnded.AddUObject(
            this, &ThisClass::HandleAbilityEnded);
        bOwnsAbility = true;
        bRequestingAbility = true;
        const uint64 RequestSerial = ExecutionSerial;
        const bool bAccepted = BodyCharacter->RequestLightAttack();
        // Ability / tag callbacks can abort or replace this execution synchronously.
        if (!bRunning || ExecutionSerial != RequestSerial)
        {
            return;
        }
        bRequestingAbility = false;
        if (!bAccepted)
        {
            bOwnsAbility = false;
            FinishAttempt(EBTNodeResult::Failed, TEXT("AbilityRejected"), false);
            return;
        }
        UE_LOG(LogETMeleeTask, Display,
            TEXT("[M1-05C2] Requested Body=%s Handle=%s LockedYaw=%.1f"),
            *GetNameSafe(BodyCharacter), *ActiveSpecHandle.ToString(), LockedYaw);
        if (bEndReceived)
        {
            FinishAttempt(bEndCancelled ? EBTNodeResult::Failed : EBTNodeResult::Succeeded,
                bEndCancelled ? TEXT("SynchronousCancelled") : TEXT("SynchronousEnded"), false);
            return;
        }
        Spec = BodyASC->FindAbilitySpecFromHandle(ActiveSpecHandle);
        if (!Spec || !Spec->IsActive())
        {
            FinishAttempt(EBTNodeResult::Failed, TEXT("InactiveWithoutEndEvent"), true);
        }
        return;
    }

    if (Phase == EPhase::WaitingForAbility)
    {
        // Distance / movement of a still-valid target does not bend this punch.
        ApplyMeleeYaw(EnemyController, BodyCharacter, LockedYaw);
        if (PhaseElapsed > AbilityWatchdog)
        {
            FinishAttempt(EBTNodeResult::Failed, TEXT("AbilityWatchdog"), true);
            return;
        }
        const FGameplayAbilitySpec* Spec = BodyASC->FindAbilitySpecFromHandle(ActiveSpecHandle);
        if (!Spec || !Spec->IsActive())
        {
            FinishAttempt(EBTNodeResult::Failed, TEXT("InactiveWithoutEndEvent"), true);
        }
    }
}

void UETBTTask_MeleeAttack::HandleAbilityEnded(const FAbilityEndedData& EndData)
{
    if (!bRunning || !bOwnsAbility || EndData.AbilitySpecHandle != ActiveSpecHandle)
    {
        return;
    }
    if (bRequestingAbility)
    {
        bEndReceived = true;
        bEndCancelled = EndData.bWasCancelled;
        return;
    }
    FinishAttempt(EndData.bWasCancelled ? EBTNodeResult::Failed : EBTNodeResult::Succeeded,
        EndData.bWasCancelled ? TEXT("AbilityCancelled") : TEXT("AbilityEnded"), false);
}

void UETBTTask_MeleeAttack::FinishAttempt(EBTNodeResult::Type Result,
    const TCHAR* Reason, bool bCancelOwnedAbility)
{
    if (!bRunning)
    {
        return;
    }
    const uint64 FinishingSerial = ExecutionSerial;
    const TWeakObjectPtr<UBehaviorTreeComponent> OwnerSnapshot = ActiveOwner;
    if (AETAIController* EnemyController = ActiveController.Get())
    {
        EnemyController->NotifyMeleeAttackAttemptFinished();
    }
    if (!bRunning || ExecutionSerial != FinishingSerial)
    {
        return;
    }
    UE_LOG(LogETMeleeTask, Display,
        TEXT("[M1-05C2] Finish Body=%s Result=%s Reason=%s Handle=%s"),
        *GetNameSafe(ActiveBody.Get()),
        Result == EBTNodeResult::Succeeded ? TEXT("Succeeded") : TEXT("Failed"),
        Reason, *ActiveSpecHandle.ToString());
    ReleaseRequest(bCancelOwnedAbility);
    if (UBehaviorTreeComponent* OwnerComp = OwnerSnapshot.Get())
    {
        if (OwnerComp->GetTaskStatus(this) == EBTTaskStatus::Active)
        {
            FinishLatentTask(*OwnerComp, Result);
        }
    }
    // No writes after FinishLatentTask: a later execution may begin immediately.
}

void UETBTTask_MeleeAttack::ReleaseRequest(bool bCancelOwnedAbility)
{
    UAbilitySystemComponent* OldASC = ActiveASC.Get();
    const FGameplayAbilitySpecHandle OldSpec = ActiveSpecHandle;
    const FDelegateHandle OldSubscription = AbilityEndedHandle;
    const bool bShouldCancel = bCancelOwnedAbility && bOwnsAbility;

    ++ExecutionSerial;
    bRunning = false;
    bOwnsAbility = false;
    bRequestingAbility = false;
    bEndReceived = false;
    bEndCancelled = false;
    Phase = EPhase::Idle;
    PhaseElapsed = 0.0f;
    LockedYaw = 0.0f;
    AbilityEndedHandle.Reset();
    ActiveSpecHandle = FGameplayAbilitySpecHandle();
    ActiveOwner.Reset();
    ActiveController.Reset();
    ActiveBody.Reset();
    ActiveTarget.Reset();
    ActiveASC.Reset();

    if (IsValid(OldASC) && OldSubscription.IsValid())
    {
        OldASC->OnAbilityEnded.Remove(OldSubscription);
    }
    if (IsValid(OldASC) && bShouldCancel && OldSpec.IsValid())
    {
        const FGameplayAbilitySpec* Spec = OldASC->FindAbilitySpecFromHandle(OldSpec);
        if (Spec && Spec->IsActive())
        {
            OldASC->CancelAbilityHandle(OldSpec);
        }
    }
}

EBTNodeResult::Type UETBTTask_MeleeAttack::AbortTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    const TWeakObjectPtr<AETAIController> ControllerSnapshot = ActiveController;
    const bool bWasRunning = bRunning;
    if (bWasRunning)
    {
        UE_LOG(LogETMeleeTask, Display,
            TEXT("[M1-05C2] Abort Body=%s Handle=%s"),
            *GetNameSafe(ActiveBody.Get()), *ActiveSpecHandle.ToString());
    }
    ReleaseRequest(true);
    if (bWasRunning)
    {
        if (AETAIController* EnemyController = ControllerSnapshot.Get())
        {
            EnemyController->NotifyMeleeAttackAttemptFinished();
        }
    }
    return EBTNodeResult::Aborted;
}

void UETBTTask_MeleeAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    ReleaseRequest(true);
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}