#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameplayTagContainer.h"
#include "ETAIController.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBlackboardData;
class UBehaviorTree;

UCLASS()
class EMBERTRIAL_API AETAIController : public AAIController
{
    GENERATED_BODY()

public:
    AETAIController(const FObjectInitializer& ObjectInitializer =
        FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "EmberTrial|AI")
    AEmberTrialCharacter* GetSensedPlayer() const;

    // Native BT nodes request a fresh decision; mutable state belongs here.
    void UpdatePursuitDecision();
    bool IsPursuitAllowed() const;

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|AI",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAIPerceptionComponent> SightPerception;

    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|AI")
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI")
    TObjectPtr<UBlackboardData> EnemyBlackboardAsset;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI")
    TObjectPtr<UBehaviorTree> EnemyBehaviorTree;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI",
        meta = (ClampMin = "1.0", Units = "cm"))
    float PursuitStopDistance = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI",
        meta = (ClampMin = "1.0", Units = "cm"))
    float PursuitResumeDistance = 200.0f;

    TWeakObjectPtr<AEmberTrialCharacter> ObservedBody;
    TWeakObjectPtr<AEmberTrialCharacter> SensedPlayer;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedBodyASC;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedTargetASC;
    FDelegateHandle BodyDeadHandle;
    FDelegateHandle BodyAttackingHandle;
    FDelegateHandle BodyHitReactHandle;
    FDelegateHandle TargetDeadHandle;
    bool bEndingPlay = false;
    bool bBlackboardReady = false;
    bool bControllerBegunPlay = false;
    bool bTreeStartAttempted = false;
    bool bTreeStarted = false;
    bool bPursuitAllowed = false;
    bool bHoldingDistance = false;

    UFUNCTION()
    void HandleBodyReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);

    UFUNCTION()
    void HandleSightUpdated(AActor* SensedActor, FAIStimulus Stimulus);

    UFUNCTION()
    void HandleTargetEndPlay(AActor* EndedTarget,
        EEndPlayReason::Type PlayEndReason);

    void HandleDeadTagChanged(FGameplayTag ChangedTag, int32 NewCount);
    void HandleBodyActionTagChanged(FGameplayTag ChangedTag, int32 NewCount);
    bool InitializeEnemyBlackboard();
    bool CanObservePlayer() const;
    bool HasValidPursuitConfig() const;
    void ObserveBodyState();
    void RemoveBodyStateSubscriptions();
    void RefreshVisiblePlayer();
    void SetSensedPlayer(AEmberTrialCharacter* NewPlayer, const TCHAR* Reason);
    void ClearSensedPlayer(const TCHAR* Reason);
    void SetPursuitAllowed(bool bNewAllowed, const TCHAR* Reason,
        float Distance, bool bForceStop = false);
    void StopPursuitMovement();
    void TryStartEnemyBehaviorTree();
    void StopEnemyBehaviorTree(const TCHAR* Reason);
    void StopObservingBody();
    void LogBodySnapshot(const TCHAR* Reason) const;
};