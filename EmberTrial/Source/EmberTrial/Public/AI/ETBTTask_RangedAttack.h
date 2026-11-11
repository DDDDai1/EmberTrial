#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ETBTTask_RangedAttack.generated.h"

UCLASS()
class EMBERTRIAL_API UETBTTask_RangedAttack : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UETBTTask_RangedAttack();
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
