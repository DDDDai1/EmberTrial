#pragma once

#include "CoreMinimal.h"
#include "ETAttackSector.generated.h"

class AActor;
class UWorld;
struct FHitResult;

// Horizontal sector measured between capsule centers. Height has its own limit.
USTRUCT()
struct FETAttackSector
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Sector", meta = (ClampMin = "1.0", Units = "cm"))
    float Range = 155.0f;

    UPROPERTY(EditAnywhere, Category = "Sector",
              meta = (ClampMin = "1.0", ClampMax = "90.0", Units = "deg"))
    float HalfAngle = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Sector", meta = (ClampMin = "0.0", Units = "cm"))
    float HeightTolerance = 70.0f;

    bool IsValid() const;
};

namespace ETAttackSector
{
    // One sample at the notify window. World blockers are tested separately for each target.
    void GatherHits(const UWorld& World, const AActor& Source, const FETAttackSector& Sector,
                    TArray<FHitResult>& OutHits);
}
