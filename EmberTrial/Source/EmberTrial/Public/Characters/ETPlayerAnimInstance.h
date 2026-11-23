#pragma once

#include "Animation/AnimInstance.h"
#include "ETPlayerAnimInstance.generated.h"

// Sample on the game thread. Transition rules and blend spaces only read these values.
UCLASS(Transient, Blueprintable)
class EMBERTRIAL_API UETPlayerAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    // Evaluated by the two stop states. The curve stores remaining forward travel in cm.
    UFUNCTION(BlueprintPure, Category = "Player Gaits", meta = (BlueprintThreadSafe))
    float GetStopPoseTime(const UAnimSequence* StopClip) const;

protected:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    FVector DesiredMoveDirection = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    float IntentAngle = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    float ForwardSpeed = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    float SideSpeed = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    bool bGroundControl = false;
    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    bool bLockedGait = false;
    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    bool bSprintDrive = false;
    UPROPERTY(BlueprintReadOnly, Category = "Player Gaits")
    bool bDriven = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stride Transitions")
    bool bRunStart = false;
    UPROPERTY(BlueprintReadOnly, Category = "Stride Transitions")
    bool bSprintStart = false;
    UPROPERTY(BlueprintReadOnly, Category = "Stride Transitions")
    bool bRunStop = false;
    UPROPERTY(BlueprintReadOnly, Category = "Stride Transitions")
    bool bSprintStop = false;
    UPROPERTY(BlueprintReadOnly, Category = "Stride Transitions")
    bool bPivotLeft = false;
    UPROPERTY(BlueprintReadOnly, Category = "Stride Transitions")
    bool bPivotRight = false;

private:
    float RemainingStopDistance = 0.0f;
    float SettledStopTime = 0.0f;
    bool bTrackingStop = false;
    uint32 LastPivot = 0;
    float PreviousSpeed = 0.0f;
};

