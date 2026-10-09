#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ETPlayerController.generated.h"

class APawn;

UCLASS()
class EMBERTRIAL_API AETPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};