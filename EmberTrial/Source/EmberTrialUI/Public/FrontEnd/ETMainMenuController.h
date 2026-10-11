#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ETMainMenuController.generated.h"

class UETMainMenuWidget;
class UETRunSessionSubsystem;
class UWorld;

UCLASS()
class EMBERTRIALUI_API AETMainMenuController : public APlayerController
{
    GENERATED_BODY()

public:
    AETMainMenuController();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleStartRequested();

    UFUNCTION()
    void HandleQuitRequested();

    void HandleSessionFailure(UWorld* FailedWorld, const FText& Message);
    void RecoverAfterFailure(const FText& Message);
    void RemoveMenuWidget();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Menu")
    TSubclassOf<UETMainMenuWidget> MainMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Menu")
    TSoftObjectPtr<UWorld> GameplayMap;

    UPROPERTY(Transient)
    TObjectPtr<UETMainMenuWidget> MenuWidget;

    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    FDelegateHandle SessionFailureHandle;
    bool bTransitionPending = false;
    bool bEndingPlay = false;
};