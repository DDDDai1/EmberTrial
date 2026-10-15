#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ETAbilityTask_DodgeMove.generated.h"

class UCharacterMovementComponent;
struct FRootMotionSource;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETDodgeMoveSignal);

UCLASS()
class EMBERTRIAL_API UETAbilityTask_DodgeMove : public UAbilityTask
{
    GENERATED_BODY()

public:
    UETAbilityTask_DodgeMove(const FObjectInitializer& ObjectInitializer);

    static UETAbilityTask_DodgeMove* CreateDodgeMove(UGameplayAbility* OwningAbility,
        const FVector& WorldDirection, float Distance, float Duration);
    static bool IsConfigurationValid(float Distance, float Duration);
    bool IsMovementReady() const;

    UPROPERTY(BlueprintAssignable)
    FETDodgeMoveSignal OnCompleted;

    UPROPERTY(BlueprintAssignable)
    FETDodgeMoveSignal OnAborted;

    virtual void Activate() override;
    virtual void TickTask(float DeltaTime) override;

protected:
    virtual void OnDestroy(bool bInOwnerFinished) override;

private:
    friend class FETDodgeMovementTest;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCharacterMovementComponent> Movement;

    TSharedPtr<FRootMotionSource> Source;
    FVector Direction = FVector::ZeroVector;
    FVector StartLocation = FVector::ZeroVector;
    float MoveDistance = 0.0f;
    float MoveDuration = 0.0f;
    float StartWorldTime = 0.0f;
    bool bFinished = false;

    void FinishMove(bool bSucceeded, const TCHAR* Reason);
    void ReleaseSource(const TCHAR* Reason);
};