#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "ETCheckpointStart.generated.h"

class AEmberTrialCharacter;
class UETRunSessionSubsystem;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class EMBERTRIAL_API AETCheckpointStart : public APlayerStart
{
    GENERATED_BODY()

public:
    AETCheckpointStart(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    void ProbeInitialOverlap();
    void HandleCheckpointChanged(UWorld* ChangedWorld, FName CheckpointId);
    void RefreshDisplay();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Checkpoint",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USphereComponent> ActivationTrigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Checkpoint",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Marker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Checkpoint",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTextRenderComponent> Label;

    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    FDelegateHandle CheckpointChangedHandle;
    FTimerHandle InitialProbeHandle;
    bool bEndingPlay = false;
};