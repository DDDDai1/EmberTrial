#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETHealingEffect.generated.h"

UCLASS()
class EMBERTRIAL_API UETHealingEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETHealingEffect(const FObjectInitializer& ObjectInitializer);
};