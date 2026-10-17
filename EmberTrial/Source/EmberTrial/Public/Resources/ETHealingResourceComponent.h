#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETHealingResourceComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;

DECLARE_MULTICAST_DELEGATE(FETHealingResourceChanged);

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETHealingResourceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETHealingResourceComponent();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

    bool HasValidConfiguration() const;
    bool GetHealingSnapshot(int32& OutCharges, int32& OutMaxCharges) const;
    bool CanApplyHealing(float Amount) const;
    bool TryApplyHealing(float Amount);
    FETHealingResourceChanged OnSnapshotChanged;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETHealingResourceTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing",
        meta = (ClampMin = "1", ClampMax = "9"))
    int32 MaxCharges = 3;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Healing",
        meta = (ClampMin = "0", ClampMax = "9"))
    int32 InitialCharges = 3;

    bool ReadBody(AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const;
    void RefreshAvailability();

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Body, bool bReady);

    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    int32 CurrentCharges = 0;
    bool bInitialized = false;
    bool bAvailable = false;
    bool bApplying = false;
    bool bEndingPlay = false;
};