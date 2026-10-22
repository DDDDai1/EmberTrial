#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "ETCombatPoiseComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETGA_Stagger;
class UWorld;
struct FAbilityEndedData;
struct FOnAttributeChangeData;

// 属性事件只排一次处理；GAS持有Stunned，组件持有观察关系和自身Spec。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETCombatPoiseComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETCombatPoiseComponent();
    bool RequestStagger();
    bool HasValidConfiguration() const;
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETStaggerAbilityTest;
    friend class FETPoiseReactionTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Poise")
    TSubclassOf<UETGA_Stagger> StaggerAbilityClass;

    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    TWeakObjectPtr<UWorld> EvaluationWorld;
    FGameplayAbilitySpecHandle StaggerHandle;
    FDelegateHandle PoiseChangedHandle;
    FDelegateHandle StunnedChangedHandle;
    FDelegateHandle AbilityEndedHandle;
    FTimerHandle EvaluationTimer;
    uint64 ObservationSerial = 0;
    bool bEndingPlay = false;
    bool bZeroAttempted = false;
    bool bRestorePending = false;

    bool ReadBody(AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC) const;
    bool IsCurrent(UAbilitySystemComponent* Source, uint64 Serial) const;
    bool IsOwnReactionActive() const;
    void StartObserving();
    void StopObserving();
    void QueueEvaluation();
    void Evaluate(uint64 Serial);
    void RestorePoise(AEmberTrialCharacter* Body, UAbilitySystemComponent* ASC);
    void HandlePoiseChanged(const FOnAttributeChangeData& Data,
                            TWeakObjectPtr<UAbilitySystemComponent> Source, uint64 Serial);
    void HandleStunnedChanged(FGameplayTag Tag, int32 Count,
                              TWeakObjectPtr<UAbilitySystemComponent> Source, uint64 Serial);
    void HandleAbilityEnded(const FAbilityEndedData& Data,
                            TWeakObjectPtr<UAbilitySystemComponent> Source, uint64 Serial);

    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* Body, bool bReady);
};