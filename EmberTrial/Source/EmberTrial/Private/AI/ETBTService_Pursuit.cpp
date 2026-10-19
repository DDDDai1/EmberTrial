#include "AI/ETBTService_Pursuit.h"
#include "AI/ETAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UETBTService_Pursuit::UETBTService_Pursuit(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeName = TEXT("ET Pursuit Decision");
    INIT_SERVICE_NODE_NOTIFY_FLAGS();
    // 既有0.2秒决策采样；搜索起点可立即采样，激活不反复重置计时。
    Interval = 0.2f;
    RandomDeviation = 0.0f;
    bCallTickOnSearchStart = true;
    bRestartTimerOnEachActivation = false;
}

void UETBTService_Pursuit::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                    float DeltaSeconds)
{
    // Super schedules the next interval; omitting it can cause excess ticks.
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    // 无跨敌人实例状态：共享节点每次读取本次OwnerComp的AI。
    if (AETAIController* EnemyController = Cast<AETAIController>(OwnerComp.GetAIOwner()))
    {
        EnemyController->UpdatePursuitDecision();
    }
}
