#include "Characters/ETPlayerMovementComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Framework/ETPlayerController.h"
#include "TargetLock/ETTargetLockComponent.h"

namespace
{
const AETPlayerController* GetGaitController(const AEmberTrialCharacter* Body)
{
    if (!Body || !Body->UsesPlayerGaits())
    {
        return nullptr;
    }
    return Cast<AETPlayerController>(Body->GetController());
}
}

void UETPlayerMovementComponent::BeginSprint()
{
    const AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CharacterOwner);
    const AETPlayerController* PC = GetGaitController(Body);
    bSprintHeld = PC && !PC->IsPaused() && !PC->IsMoveInputIgnored() && IsMovingOnGround() &&
        Body->CanAcceptLocomotionInput() && !PC->GetTargetLockComponent()->GetLockedTarget();
}

void UETPlayerMovementComponent::ReleaseSprint()
{
    bSprintHeld = false;
}

void UETPlayerMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
    const AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CharacterOwner);
    const AETPlayerController* PC = GetGaitController(Body);
    bGroundControl = PC && IsMovingOnGround() && !PC->IsMoveInputIgnored() &&
        Body->CanAcceptLocomotionInput();
    LockTarget = bGroundControl ? PC->GetTargetLockComponent()->GetLockedTarget() : nullptr;
    if (!bGroundControl || LockTarget.IsValid())
    {
        LastDrivenDeceleration = 0.0f;
        ReleaseSprint();
        bPivotArmed = true;
        IntentAngle = 0.0f;
        return;
    }

    // Measure before friction bends Velocity toward the new acceleration.
    const FVector Intent = Acceleration.GetSafeNormal2D();
    if (!Intent.IsNearlyZero())
    {
        LastDrivenDeceleration = IsSprinting() ? SprintStopDeceleration : RunStopDeceleration;
    }

    const FVector Heading = Velocity.GetSafeNormal2D();
    const float Speed = Velocity.Size2D();
    IntentAngle = (Intent.IsNearlyZero() || Heading.IsNearlyZero()) ? 0.0f :
        FMath::FindDeltaAngleDegrees(Heading.Rotation().Yaw, Intent.Rotation().Yaw);
    if (Speed >= 200.0f && FMath::Abs(IntentAngle) >= 110.0f && bPivotArmed)
    {
        bPivotLeft = IntentAngle < 0.0f;
        ++PivotSerial;
        bPivotArmed = false;
    }
    else if (Speed < 80.0f || FMath::Abs(IntentAngle) < 45.0f)
    {
        bPivotArmed = true;
    }
}

void UETPlayerMovementComponent::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
    // Let CharacterMovement sweep the capsule and resolve floors and walls as usual.
    if (HasGroundControl() && !IsStrafing() && !IsCrouching() && Acceleration.IsZero() &&
        !bHasRequestedVelocity && !CurrentRootMotion.HasActiveRootMotionSources() && LastDrivenDeceleration > 0.0f)
    {
        Super::ApplyVelocityBraking(DeltaTime, 0.0f, LastDrivenDeceleration);
        return;
    }
    Super::ApplyVelocityBraking(DeltaTime, Friction, BrakingDeceleration);
}

float UETPlayerMovementComponent::GetRemainingStopDistance() const
{
    return LastDrivenDeceleration > 0.0f ? Velocity.SizeSquared2D() / (2.0f * LastDrivenDeceleration) : 0.0f;
}

float UETPlayerMovementComponent::GetMaxSpeed() const
{
    const float BaseSpeed = Super::GetMaxSpeed();
    if (IsFalling())
    {
        return FMath::Max(BaseSpeed, TakeoffSpeed);
    }
    if (!IsCrouching() && HasGroundControl())
    {
        if (IsStrafing())
        {
            return FMath::Min(BaseSpeed, StrafeSpeed);
        }
        if (IsSprinting())
        {
            return SprintSpeed;
        }
    }
    return BaseSpeed;
}

void UETPlayerMovementComponent::OnMovementModeChanged(EMovementMode PreviousMode, uint8 PreviousCustomMode)
{
    const bool bLeavingGround = PreviousMode == MOVE_Walking || PreviousMode == MOVE_NavWalking;
    TakeoffSpeed = IsFalling() && bLeavingGround && bGroundControl && bSprintHeld
        ? FMath::Min<float>(Velocity.Size2D(), SprintSpeed) : 0.0f;
    if (!IsMovingOnGround())
    {
        LastDrivenDeceleration = 0.0f;
        ReleaseSprint();
        LockTarget.Reset();
        bGroundControl = false;
    }
    Super::OnMovementModeChanged(PreviousMode, PreviousCustomMode);
}

FRotator UETPlayerMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation,
    float DeltaTime, FRotator& DeltaRotation) const
{
    if (IsStrafing())
    {
        const FVector ToTarget = LockTarget->GetActorLocation() - CharacterOwner->GetActorLocation();
        return ToTarget.SizeSquared2D() > FMath::Square(10.0f)
            ? FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f) : CurrentRotation;
    }
    return Super::ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRotation);
}

