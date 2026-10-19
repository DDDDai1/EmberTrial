#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ETBTService_Pursuit.generated.h"

// BT共享Service模板只做间隔调度；每次从OwnerComp取得当前敌人，不保存Pawn。
UCLASS()
class UETBTService_Pursuit : public UBTService
{
    GENERATED_BODY()

public:
    UETBTService_Pursuit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
    // 必须调用Super维护原生下一次间隔，不把决策改成每帧轮询。
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                          float DeltaSeconds) override;
};
