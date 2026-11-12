#pragma once

#include "CoreMinimal.h"

class UCharacterMovementComponent;
struct FRootMotionSource;

namespace ETGroundBurst
{
    bool IsConfigurationValid(float Distance, float Duration);

    // A horizontal override that leaves vertical physics and collision to CharacterMovement.
    TSharedPtr<FRootMotionSource> Start(UCharacterMovementComponent& Move, FName SourceName,
                                       const FVector& Direction, float Distance, float Duration);

    // Match both ID and instance to avoid removing a different source after ID reuse.
    void Release(UCharacterMovementComponent& Move, const TSharedPtr<FRootMotionSource>& Source);
}
