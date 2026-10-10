#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/PlayerController.h"
#include "ETMainMenuController.generated.h"

class UETMainMenuWidget;
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

    void HandleTravelFailure(UWorld* FailedWorld,
        ETravelFailure::Type FailureType, const FString& ErrorString);
    void RecoverAfterFailure(const FText& Message);
    void RemoveMenuWidget();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Menu")
    TSubclassOf<UETMainMenuWidget> MainMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Menu")
    TSoftObjectPtr<UWorld> GameplayMap;

    UPROPERTY(Transient)
    TObjectPtr<UETMainMenuWidget> MenuWidget;

    FDelegateHandle TravelFailureHandle;
    bool bTransitionPending = false;
    bool bEndingPlay = false;
};