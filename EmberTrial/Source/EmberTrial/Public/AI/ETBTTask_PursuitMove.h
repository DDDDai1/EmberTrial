#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "ETBTTask_PursuitMove.generated.h"

// 复用原生MoveTo / NodeMemory，只增加资格复核和结束诊断；不自建位移。
UCLASS()
class UETBTTask_PursuitMove : public UBTTask_MoveTo
{
    GENERATED_BODY()

public:
    UETBTTask_PursuitMove(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 执行前重判当前黑板目标，原生任务负责路径跟随。
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                            uint8* NodeMemory) override;
    // 先保存结束快照，再执行原生清理；不绕过Super或在此重启决策。
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                EBTNodeResult::Type TaskResult) override;
};
