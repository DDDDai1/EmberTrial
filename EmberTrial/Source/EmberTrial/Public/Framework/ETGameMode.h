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
};