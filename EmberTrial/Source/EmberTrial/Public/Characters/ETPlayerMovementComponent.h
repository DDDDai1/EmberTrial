#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "ETPlayerMovementComponent.generated.h"

class AEmberTrialCharacter;

// Only opted-in player bodies use the additional gaits. AI still uses native movement.
UCLASS()
class EMBERTRIAL_API UETPlayerMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    virtual float GetMaxSpeed() const override;
    void BeginSprint();
    void ReleaseSprint();

    bool HasGroundControl() const { return bGroundControl && IsMovingOnGround(); }
    bool IsSprinting() const { return HasGroundControl() && bSprintHeld && !LockTarget.IsValid(); }
    bool IsStrafing() const { return HasGroundControl() && LockTarget.IsValid(); }
    uint32 GetPivotSerial() const { return PivotSerial; }
    float GetIntentAngle() const { return IntentAngle; }
    float GetRemainingStopDistance() const;
    bool IsPivotLeft() const { return bPivotLeft; }

protected:
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration) override;
    virtual void OnMovementModeChanged(EMovementMode PreviousMode, uint8 PreviousCustomMode) override;
    virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation,
        float DeltaTime, FRotator& DeltaRotation) const override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "Player Gaits", meta = (ClampMin = "1", Units = "cm/s"))
    float SprintSpeed = 700.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Player Gaits", meta = (ClampMin = "1", Units = "cm/s"))
    float StrafeSpeed = 300.0f;

    // At full speed these give the run and sprint clips their captured stopping distances.
    UPROPERTY(EditDefaultsOnly, Category = "Player Gaits", meta = (ClampMin = "1", Units = "cm/s^2"))
    float RunStopDeceleration = 630.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Player Gaits", meta = (ClampMin = "1", Units = "cm/s^2"))
    float SprintStopDeceleration = 1120.0f;

    float LastDrivenDeceleration = 0.0f;
    TWeakObjectPtr<AEmberTrialCharacter> LockTarget;
    bool bGroundControl = false;
    bool bSprintHeld = false;
    bool bPivotArmed = true;
    bool bPivotLeft = false;
    uint32 PivotSerial = 0;
    float IntentAngle = 0.0f;
    float TakeoffSpeed = 0.0f;
};

