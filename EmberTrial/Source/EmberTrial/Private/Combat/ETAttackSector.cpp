#include "Combat/ETAttackSector.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

bool FETAttackSector::IsValid() const
{
    return FMath::IsFinite(Range) && Range >= 1.0f &&
           FMath::IsFinite(HalfAngle) && HalfAngle >= 1.0f && HalfAngle <= 90.0f &&
           FMath::IsFinite(HeightTolerance) && HeightTolerance >= 0.0f;
}

void ETAttackSector::GatherHits(const UWorld& World, const AActor& Source, const FETAttackSector& Sector,
                                TArray<FHitResult>& OutHits)
{
    OutHits.Reset();
    const FVector Origin = Source.GetActorLocation();
    const FVector Forward = Source.GetActorForwardVector().GetSafeNormal2D();
    const float SearchRadius = FMath::Sqrt(FMath::Square(Sector.Range) + FMath::Square(Sector.HeightTolerance));
    if (!Sector.IsValid() || !FMath::IsFinite(SearchRadius) || Origin.ContainsNaN() || Forward.IsNearlyZero())
    {
        return;
    }

    const double MinDot = FMath::Cos(FMath::DegreesToRadians(static_cast<double>(Sector.HalfAngle)));
    const double RangeSquared = FMath::Square(static_cast<double>(Sector.Range));
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(ETAttackSector), false, &Source);
    TArray<FOverlapResult> Overlaps;
    World.OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
                                   FCollisionShape::MakeSphere(SearchRadius), Query);

    TSet<const APawn*> TestedTargets;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        APawn* Target = Cast<APawn>(Overlap.GetActor());
        if (!IsValid(Target) || Target == &Source || TestedTargets.Contains(Target))
        {
            continue;
        }
        TestedTargets.Add(Target);

        const FVector TargetLocation = Target->GetActorLocation();
        const FVector Offset = TargetLocation - Origin;
        if (Offset.ContainsNaN() || FMath::Abs(Offset.Z) > Sector.HeightTolerance ||
            Offset.SizeSquared2D() > RangeSquared)
        {
            continue;
        }

        const FVector Direction = Offset.GetSafeNormal2D();
        if (!Direction.IsNearlyZero() && FVector::DotProduct(Forward, Direction) + UE_DOUBLE_SMALL_NUMBER < MinDot)
        {
            continue;
        }

        FCollisionQueryParams Sight = Query;
        Sight.AddIgnoredActor(Target);
        if (!World.LineTraceTestByChannel(Origin, TargetLocation, ECC_Visibility, Sight))
        {
            OutHits.Emplace(Target, Overlap.GetComponent(), TargetLocation, -Direction);
        }
    }
}
