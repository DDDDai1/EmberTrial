#include "Characters/ETPlayerAnimInstance.h"
#include "Characters/ETPlayerMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "GameFramework/Character.h"

void UETPlayerAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    bTrackingStop = false;
    RemainingStopDistance = 0.0f;
    SettledStopTime = 0.0f;
    PreviousSpeed = 0.0f;
    LastPivot = 0;
    bDriven = false;
    bSprintDrive = false;
}

void UETPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const ACharacter* Body = Cast<ACharacter>(TryGetPawnOwner());
    const auto* Move = Body ? Cast<UETPlayerMovementComponent>(Body->GetCharacterMovement()) : nullptr;
    const bool bPreviouslyDriven = bDriven;
    const bool bPreviouslySprinting = bSprintDrive;
    const FVector Velocity = Move ? Move->Velocity : FVector::ZeroVector;
    const FVector Local = Body ? Body->GetActorTransform().InverseTransformVectorNoScale(Velocity)
                               : FVector::ZeroVector;
    ForwardSpeed = Local.X;
    SideSpeed = Local.Y;
    bGroundControl = Move && Move->HasGroundControl();
    bLockedGait = bGroundControl && Move->IsStrafing();
    DesiredMoveDirection = bGroundControl ? Move->GetCurrentAcceleration().GetSafeNormal2D() : FVector::ZeroVector;
    IntentAngle = Move ? Move->GetIntentAngle() : 0.0f;
    bDriven = bGroundControl && !DesiredMoveDirection.IsNearlyZero();
    bSprintDrive = bDriven && Move->IsSprinting();

    const bool bFree = bGroundControl && !bLockedGait;
    const bool bStarting = bFree && bDriven && !bPreviouslyDriven && PreviousSpeed < 110.0f;
    const bool bStopping = bFree && !bDriven && bPreviouslyDriven && PreviousSpeed >= 90.0f;
    bRunStart = bStarting && !bSprintDrive;
    bSprintStart = bStarting && bSprintDrive;
    bSprintStop = bStopping && bPreviouslySprinting;
    bRunStop = bStopping && !bPreviouslySprinting;
    if (bStopping)
    {
        bTrackingStop = true;
        SettledStopTime = 0.0f;
    }
    if (bDriven || !bFree)
    {
        bTrackingStop = false;
    }
    if (bTrackingStop)
    {
        RemainingStopDistance = Move->GetRemainingStopDistance();
        if (Velocity.IsNearlyZero())
        {
            SettledStopTime = FMath::Min(1.0f, SettledStopTime + DeltaSeconds);
        }
    }
    const bool bNewPivot = bFree && Move->GetPivotSerial() != LastPivot;
    bPivotLeft = bNewPivot && Move->IsPivotLeft();
    bPivotRight = bNewPivot && !bPivotLeft;
    LastPivot = Move ? Move->GetPivotSerial() : 0;
    PreviousSpeed = Velocity.Size2D();
}


float UETPlayerAnimInstance::GetStopPoseTime(const UAnimSequence* StopClip) const
{
    if (!StopClip)
    {
        return 0.0f;
    }

    static const FName DistanceCurve(TEXT("StopDistanceRemaining"));
    const float Duration = StopClip->GetPlayLength();
    const float Target = FMath::Max(RemainingStopDistance, 0.1f);
    float Start = 0.0f;
    float End = Duration;
    // Monotonic curve sampled from this exact clip. 10 bisections give sub-frame precision.
    for (int32 Index = 0; Index < 10; ++Index)
    {
        const float Mid = (Start + End) * 0.5f;
        if (StopClip->EvaluateCurveData(DistanceCurve, Mid) > Target)
        {
            Start = Mid;
        }
        else
        {
            End = Mid;
        }
    }
    return FMath::Min(End + SettledStopTime, Duration);
}
