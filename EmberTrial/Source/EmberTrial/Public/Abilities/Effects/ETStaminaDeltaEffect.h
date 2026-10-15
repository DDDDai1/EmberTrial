#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETStaminaDeltaEffect.generated.h"

UCLASS()
class EMBERTRIAL_API UETStaminaDeltaEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETStaminaDeltaEffect(const FObjectInitializer& ObjectInitializer);
};