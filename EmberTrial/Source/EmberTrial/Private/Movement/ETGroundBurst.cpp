#include "Movement/ETGroundBurst.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"

bool ETGroundBurst::IsConfigurationValid(float Distance, float Duration)
{
    return FMath::IsFinite(Distance) && Distance >= 1.0f && Distance <= 300.0f &&
           FMath::IsFinite(Duration) && Duration >= 0.05f && Duration <= 0.50f;
}

TSharedPtr<FRootMotionSource> ETGroundBurst::Start(UCharacterMovementComponent& Move, FName SourceName,
                                                  const FVector& Direction, float Distance, float Duration)
{
    if (!Move.IsMovingOnGround() || Move.HasRootMotionSources() || Direction.ContainsNaN() ||
        Direction.IsNearlyZero() || !IsConfigurationValid(Distance, Duration))
    {
        return nullptr;
    }

    TSharedPtr<FRootMotionSource_ConstantForce> Source = MakeShared<FRootMotionSource_ConstantForce>();
    Source->InstanceName = SourceName;
    Source->Priority = 500;
    Source->AccumulateMode = ERootMotionAccumulateMode::Override;
    Source->bInLocalSpace = false;
    Source->Force = Direction * (Distance / Duration);
    Source->Duration = Duration;
    Source->Settings.UnSetFlag(ERootMotionSourceSettingsFlags::DisablePartialEndTick);
    Source->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
    Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
    Source->FinishVelocityParams.ClampVelocity = 0.0f;

    const uint16 ID = Move.ApplyRootMotionSource(Source);
    if (ID == static_cast<uint16>(ERootMotionSourceID::Invalid))
    {
        return nullptr;
    }
    return Source;
}

void ETGroundBurst::Release(UCharacterMovementComponent& Move, const TSharedPtr<FRootMotionSource>& Source)
{
    const bool bOwned = Move.GetRootMotionSourceByID(Source->LocalID) == Source;
    if (bOwned)
    {
        Move.RemoveRootMotionSourceByID(Source->LocalID);
    }
    if (bOwned || !Move.HasRootMotionSources())
    {
        Move.Velocity.X = 0.0f;
        Move.Velocity.Y = 0.0f;
    }
}
