#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETRangedShotCooldownEffect.generated.h"

UCLASS(NotBlueprintable)
class EMBERTRIAL_API UETRangedShotCooldownEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETRangedShotCooldownEffect(const FObjectInitializer& ObjectInitializer);
};