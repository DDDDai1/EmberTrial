#pragma once

#include "CoreMinimal.h"
#include "ETAttackCharge.generated.h"

USTRUCT()
struct FETAttackCharge
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "cm"))
    float TravelDistance = 240.0f;

    UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "0.05", ClampMax = "0.50", Units = "s"))
    float TravelDuration = 0.30f;

    UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "1.0", Units = "cm"))
    float ContactRadius = 28.0f;

    UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "0.0", Units = "cm"))
    float ContactLead = 62.0f;

    bool IsValid() const;
};
