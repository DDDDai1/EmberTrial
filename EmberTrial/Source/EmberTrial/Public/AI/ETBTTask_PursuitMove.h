#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "ETBTTask_PursuitMove.generated.h"

UCLASS()
class UETBTTask_PursuitMove : public UBTTask_MoveTo
{
    GENERATED_BODY()

public:
    UETBTTask_PursuitMove(const FObjectInitializer& ObjectInitializer =
        FObjectInitializer::Get());

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
};