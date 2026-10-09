#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ETBTService_Pursuit.generated.h"

UCLASS()
class UETBTService_Pursuit : public UBTService
{
    GENERATED_BODY()

public:
    UETBTService_Pursuit(const FObjectInitializer& ObjectInitializer =
        FObjectInitializer::Get());

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory, float DeltaSeconds) override;
};