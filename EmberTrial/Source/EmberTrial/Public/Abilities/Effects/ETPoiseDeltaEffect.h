#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETPoiseDeltaEffect.generated.h"

// 同一条Instant通道用于未来攻击者Spec和当前身体恢复；属性集负责夹限。
UCLASS()
class EMBERTRIAL_API UETPoiseDeltaEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETPoiseDeltaEffect(const FObjectInitializer& ObjectInitializer);
};