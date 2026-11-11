#include "AI/ETBTTask_RangedAttack.h"

#include "AI/ETAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UETBTTask_RangedAttack::UETBTTask_RangedAttack()
{
    NodeName = TEXT("ET Ranged Attack");
}

EBTNodeResult::Type UETBTTask_RangedAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    (void)NodeMemory;
    AETAIController* Controller = Cast<AETAIController>(OwnerComp.GetAIOwner());
    return IsValid(Controller) && Controller->TryRangedAttack()
        ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
