#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ETPlayerController.generated.h"

class APawn;

DECLARE_MULTICAST_DELEGATE(FETPauseRequested);

UCLASS()
class EMBERTRIAL_API AETPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// Core emits an intent; the UI module decides whether to present a menu.
	FETPauseRequested OnPauseRequested;

	UFUNCTION(BlueprintImplementableEvent, Category = "EmberTrial|Flow")
	void OnGameMenuStateChanged();

	UFUNCTION(Exec, meta = (DevelopmentOnly))
	void ET_Stamina(float Delta);

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void HandlePauseReleased();
};