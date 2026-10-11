#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ETGameMode.generated.h"

UCLASS()
class EMBERTRIAL_API AETGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AETGameMode();
	virtual void StartPlay() override;

	UFUNCTION(Exec, meta = (DevelopmentOnly))
	void ETDebugSetCheckpointId(FName TestId);

protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player,
		const FString& IncomingName) override;

private:
	FName SelectedSpawnId = NAME_None;
};