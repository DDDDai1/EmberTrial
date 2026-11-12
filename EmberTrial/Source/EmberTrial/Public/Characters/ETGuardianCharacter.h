#pragma once

#include "Characters/EmberTrialCharacter.h"
#include "TimerManager.h"
#include "ETGuardianCharacter.generated.h"

class UTextRenderComponent;
struct FGameplayTag;
struct FOnAttributeChangeData;

// Encounter-specific phase behavior. Combat, movement and death use the shared body.
UCLASS()
class EMBERTRIAL_API AETGuardianCharacter : public AEmberTrialCharacter
{
    GENERATED_BODY()

public:
    AETGuardianCharacter();

    bool IsInPhaseTwo() const { return bPhaseTwo; }
    float GetCloseTrackingRadius() const { return CloseTrackingRadius; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Guardian")
    TObjectPtr<UTextRenderComponent> PhaseMarker;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian", meta = (ClampMin = "0.01", ClampMax = "0.99"))
    float PhaseTwoHealthFraction = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian", meta = (ClampMin = "1.0", Units = "cm/s"))
    float PhaseTwoWalkSpeed = 330.0f;

    // Retains an already acquired, unobstructed player outside the sight cone.
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian", meta = (ClampMin = "0.0", Units = "cm"))
    float CloseTrackingRadius = 300.0f;

    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHealthHandle;
    FDelegateHandle TagHandle;
    FDelegateHandle DeathHandle;
    FTimerHandle PhaseCheckTimer;
    bool bPhaseTwo = false;

    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* Body, bool bReady);
    void HandleHealthChanged(const FOnAttributeChangeData& Change);
    void HandleTagChanged(FGameplayTag Tag, int32 Count);
    void HandleDeath(UETCombatLifeComponent* Life);

    void StopObservingPhase();
    void QueuePhaseCheck();
    void EvaluatePhase();
    bool IsBelowThreshold(float& Health, float& MaxHealth) const;
};
