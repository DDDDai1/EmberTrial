#include "AI/ETBTTask_PursuitMove.h"
#include "AI/ETAIController.h"
#include "Characters/EmberTrialCharacter.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPursuitMove, Log, All);

namespace
{
    const FName TargetActorKey(TEXT("TargetActor"));
    const FName StopDistanceKey(TEXT("PursuitStopDistance"));

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
    BlackboardKey.SelectedKeyName = TargetActorKey;
    AcceptableRadius = 150.0f;
    AcceptableRadius.SetKey(StopDistanceKey);
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
    const FString ControllerName = GetNameSafe(OwnerComp.GetAIOwner());
    // Always retain native UAITask / observer cleanup, including rejection.
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
    UE_LOG(LogETPursuitMove, Display,
        TEXT("[M1-05B2] Move finished Controller=%s Result=%s"),
        *ControllerName, DescribeMoveResult(TaskResult));
}