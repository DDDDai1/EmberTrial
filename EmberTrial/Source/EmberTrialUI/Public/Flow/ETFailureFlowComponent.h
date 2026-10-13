#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETFailureFlowComponent.generated.h"

class APawn;
class AETPlayerController;
class UWorld;
class UETCombatLifeComponent;
class UETFailureWidget;
class UETPauseWidget;
class UETRunSessionSubsystem;
class UETUILocalPlayerSubsystem;

// Preserve the existing native class and serialized component configuration.
// This single Controller component now coordinates both pause and failure UI.
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

    UFUNCTION()
    void HandleResumeRequested();

    void HandlePauseRequested();
    void PresentPause();
    void RemovePauseWidget();
    void BindPawn(APawn* CurrentPawn);
    void StopObservingLife();
    void HandleDeathSealed(UETCombatLifeComponent* SourceLife);
    bool HasCurrentContext() const;
    bool HasCurrentBody(bool bExpectedDead) const;
    bool HasCurrentDeadBody() const;
    void QueueFailure();
    void PresentFailure();
    void RecoverActivePanel(const FText& Message);
    void HandleSessionFailure(UWorld* FailedWorld, const FText& Message);
    void RemoveFailureWidget();
    void RestoreOwnedInput();
    void CancelPresentation();
    void NotifyMenuState();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Pause")
    TSubclassOf<UETPauseWidget> PauseWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSubclassOf<UETFailureWidget> FailureWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSoftObjectPtr<UWorld> MainMenuMap;

    UPROPERTY(Transient)
    TObjectPtr<UETPauseWidget> PauseWidget;

    UPROPERTY(Transient)
    TObjectPtr<UETFailureWidget> FailureWidget;

    TWeakObjectPtr<AETPlayerController> ControllerOwner;
    TWeakObjectPtr<UETUILocalPlayerSubsystem> UIContext;
    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    TWeakObjectPtr<UETCombatLifeComponent> BoundLife;
    FDelegateHandle PauseRequestHandle;
    FDelegateHandle DeathHandle;
    FDelegateHandle SessionFailureHandle;
    FTimerHandle PresentationHandle;
    bool bPresentationQueued = false;
    bool bCommandPending = false;
    bool bOwnsPause = false;
    bool bEndingPlay = false;
};