#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETRelicEquipmentEffect.generated.h"

// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供两个SetByCaller，即使是0。
UCLASS()
class EMBERTRIAL_API UETRelicEquipmentEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETRelicEquipmentEffect(const FObjectInitializer& ObjectInitializer);
};