#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETStaminaRecoveryComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;
struct FGameplayTag;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETStaminaRecoveryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETStaminaRecoveryComponent();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;
    bool IsRecoveryScheduled() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETStaminaRecoveryTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
        meta = (ClampMin = "0.01", ClampMax = "1000.0"))
    float RecoveryRate = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
        meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
    float RecoveryDelay = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
        meta = (ClampMin = "0.05", ClampMax = "0.5", Units = "s"))
    float RecoveryInterval = 0.10f;

    void RefreshObservation();
    void StopObserving();
    void SyncRecoveryTimer();
    void ClearRecoveryTimer(const TCHAR* Reason);
    bool ReadBody(AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC,
        float& Stamina, float& MaxStamina) const;
    void HandleRecoveryTimer(uint64 Serial);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Body, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
        TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);
    void HandleActionTagChanged(const FGameplayTag Tag, int32 NewCount,
        TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);

    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle StaminaHandle;
    FDelegateHandle MaxStaminaHandle;
    TArray<FDelegateHandle> ActionTagHandles;
    FTimerHandle RecoveryTimer;
    uint64 ObservationSerial = 0;
    uint64 DecreaseRevision = 0;
    double RecoveryNotBefore = 0.0;
    double LastSampleTime = 0.0;
    bool bRecovering = false;
    bool bEndingPlay = false;
};