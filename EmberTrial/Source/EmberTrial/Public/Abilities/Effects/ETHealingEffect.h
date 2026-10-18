#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETHealingEffect.generated.h"

// 固定Instant治疗Effect：只声明Health增量，次数事务与合法性检查由资源组件负责。
UCLASS()
class EMBERTRIAL_API UETHealingEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    // 治疗量来自每次Spec的SetByCaller，不在共享CDO缓存本次玩家数值。
    UETHealingEffect(const FObjectInitializer& ObjectInitializer);
};
