#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETBreakPulseCooldownEffect.generated.h"

// 固定结构只授予Cooldown.BreakPulse；每次Spec的时长来自施放GA配置。
UCLASS(NotBlueprintable)
class EMBERTRIAL_API UETBreakPulseCooldownEffect : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UETBreakPulseCooldownEffect(const FObjectInitializer& ObjectInitializer);
};