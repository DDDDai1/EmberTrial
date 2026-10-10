#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpecHandle.h"
#include "ETBTTask_MeleeAttack.generated.h"

class AETAIController;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
struct FAbilityEndedData;

UCLASS()
class UETBTTask_MeleeAttack : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UETBTTask_MeleeAttack(const FObjectInitializer& ObjectInitializer);

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;

protected:
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory, float DeltaSeconds) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:
    enum class EPhase : uint8
    {
        Idle,
        Preparing,
        WaitingForAbility
    };

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI",
        meta = (ClampMin = "1.0", Units = "deg/s"))
    float TurnRate = 360.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI",
        meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "deg"))
    float FacingTolerance = 5.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI",
        meta = (ClampMin = "0.1", Units = "s"))
    float PreparationTimeout = 1.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI",
        meta = (ClampMin = "0.1", Units = "s"))
    float AbilityWatchdog = 5.0f;

    TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwner;
    TWeakObjectPtr<AETAIController> ActiveController;
    TWeakObjectPtr<AEmberTrialCharacter> ActiveBody;
    TWeakObjectPtr<AEmberTrialCharacter> ActiveTarget;
    TWeakObjectPtr<UAbilitySystemComponent> ActiveASC;
    FGameplayAbilitySpecHandle ActiveSpecHandle;
    FDelegateHandle AbilityEndedHandle;
    EPhase Phase = EPhase::Idle;
    float PhaseElapsed = 0.0f;
    float LockedYaw = 0.0f;
    uint64 ExecutionSerial = 0;
    bool bRunning = false;
    bool bOwnsAbility = false;
    bool bRequestingAbility = false;
    bool bEndReceived = false;
    bool bEndCancelled = false;

    bool HasValidSettings() const;
    bool GetLiveContext(UBehaviorTreeComponent& OwnerComp,
        AETAIController*& OutController, AEmberTrialCharacter*& OutBody,
        AEmberTrialCharacter*& OutTarget, UAbilitySystemComponent*& OutASC) const;
    void HandleAbilityEnded(const FAbilityEndedData& EndData);
    void FinishAttempt(EBTNodeResult::Type Result, const TCHAR* Reason,
        bool bCancelOwnedAbility);
    void ReleaseRequest(bool bCancelOwnedAbility);
};