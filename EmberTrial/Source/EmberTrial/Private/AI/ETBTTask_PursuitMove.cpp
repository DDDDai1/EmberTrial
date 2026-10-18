#include "AI/ETBTTask_PursuitMove.h"
#include "AI/ETBlackboardKeys.h"
#include "AI/ETAIController.h"
#include "Characters/EmberTrialCharacter.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPursuitMove, Log, All);

namespace
{

    const TCHAR* DescribeMoveResult(EBTNodeResult::Type Result)
    {
        switch (Result)
        {
        case EBTNodeResult::Succeeded: return TEXT("Succeeded");
        case EBTNodeResult::Failed: return TEXT("Failed");
        case EBTNodeResult::Aborted: return TEXT("Aborted");
        case EBTNodeResult::InProgress: return TEXT("InProgress");
        default: return TEXT("Unknown");
        }
    }
}

UETBTTask_PursuitMove::UETBTTask_PursuitMove(
    const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    NodeName = TEXT("ET Pursuit Move To");
    bNotifyTaskFinished = true;
    BlackboardKey.SelectedKeyName = ETBlackboardKeys::TargetActor;
    AcceptableRadius = 150.0f;
    AcceptableRadius.SetKey(ETBlackboardKeys::PursuitStopDistance);
    bAllowPartialPath = false;
    bTrackMovingGoal = true;
    bRequireNavigableEndLocation = true;
    bProjectGoalLocation = false;
    bReachTestIncludesAgentRadius = false;
    bReachTestIncludesGoalRadius = false;
    bAllowStrafe = false;
    bStartFromPreviousPath = false;
    bObserveBlackboardValue = false;
}

EBTNodeResult::Type UETBTTask_PursuitMove::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AETAIController* EnemyController =
        Cast<AETAIController>(OwnerComp.GetAIOwner());
    UBlackboardComponent* Board = OwnerComp.GetBlackboardComponent();
    if (IsValid(EnemyController))
    {
        EnemyController->UpdatePursuitDecision();
    }
    if (!IsValid(EnemyController) || !IsValid(Board) ||
        !EnemyController->IsPursuitAllowed() ||
        Board->GetValueAsObject(BlackboardKey.SelectedKeyName) !=
            EnemyController->GetSensedPlayer())
    {
        UE_LOG(LogETPursuitMove, Display,
            TEXT("[M1-05B2] Move rejected Controller=%s Reason=InvalidGateOrGoal"),
            *GetNameSafe(EnemyController));
        return EBTNodeResult::Failed;
    }
    const FString GoalName = GetNameSafe(
        Board->GetValueAsObject(BlackboardKey.SelectedKeyName));
    const float Radius = AcceptableRadius.GetValue(OwnerComp);
    const EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);
    UE_LOG(LogETPursuitMove, Display,
        TEXT("[M1-05B2] Move request Controller=%s Goal=%s Radius=%.1f Result=%s"),
        *GetNameSafe(EnemyController), *GoalName, Radius, DescribeMoveResult(Result));
    return Result;
}

void UETBTTask_PursuitMove::OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    const AAIController* Controller = OwnerComp.GetAIOwner();
    const APawn* Body = IsValid(Controller) ? Controller->GetPawn() : nullptr;
    const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    const AActor* Goal = IsValid(Blackboard)
        ? Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName))
        : nullptr;

    const FString ControllerName = GetNameSafe(Controller);
    const FString GoalName = GetNameSafe(Goal);
    const double Distance2D = IsValid(Body) && IsValid(Goal)
        ? FVector::Dist2D(Body->GetActorLocation(), Goal->GetActorLocation()) : -1.0;
    const float Radius = IsValid(Blackboard) ? AcceptableRadius.GetValue(OwnerComp) : -1.0f;

    // Snapshot before cleanup; always retain native move task cleanup.
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
    UE_LOG(LogETPursuitMove, Display,
        TEXT("[M1-05B2] Move finished Controller=%s Result=%s Goal=%s Distance2D=%.1f Radius=%.1f"),
        *ControllerName, DescribeMoveResult(TaskResult), *GoalName, Distance2D, Radius);
}