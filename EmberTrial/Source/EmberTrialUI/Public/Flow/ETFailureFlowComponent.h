#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETFailureFlowComponent.generated.h"

class APawn;
class APlayerController;
class UWorld;
class UETCombatLifeComponent;
class UETFailureWidget;
class UETRunSessionSubsystem;
class UETUILocalPlayerSubsystem;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETFailureFlowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETFailureFlowComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleRetryRequested();

    UFUNCTION()
    void HandleMainMenuRequested();

    void BindPawn(APawn* CurrentPawn);
    void StopObservingLife();
    void HandleDeathSealed(UETCombatLifeComponent* SourceLife);
    bool HasCurrentDeadBody() const;
    void QueueFailure();
    void PresentFailure();
    void RecoverFailure(const FText& Message);
    void HandleSessionFailure(UWorld* FailedWorld, const FText& Message);
    void RemoveFailureWidget();
    void RestoreOwnedInput();
    void CancelPresentation();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSubclassOf<UETFailureWidget> FailureWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSoftObjectPtr<UWorld> MainMenuMap;

    UPROPERTY(Transient)
    TObjectPtr<UETFailureWidget> FailureWidget;

    TWeakObjectPtr<APlayerController> ControllerOwner;
    TWeakObjectPtr<UETUILocalPlayerSubsystem> UIContext;
    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    TWeakObjectPtr<UETCombatLifeComponent> BoundLife;
    FDelegateHandle DeathHandle;
    FDelegateHandle SessionFailureHandle;
    FTimerHandle PresentationHandle;
    bool bPresentationQueued = false;
    bool bCommandPending = false;
    bool bOwnsPause = false;
    bool bEndingPlay = false;
};