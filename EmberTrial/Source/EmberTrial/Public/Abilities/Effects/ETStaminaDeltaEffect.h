#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETStaminaDeltaEffect.generated.h"

// 只定义一次性Stamina AddBase通道；消费 / 恢复量由调用方的SetByCaller提供。
UCLASS()
class EMBERTRIAL_API UETStaminaDeltaEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETStaminaDeltaEffect(const FObjectInitializer& ObjectInitializer);
};
