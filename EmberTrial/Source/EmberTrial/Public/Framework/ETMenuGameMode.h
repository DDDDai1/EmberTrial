#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ETMenuGameMode.generated.h"

class APlayerController;

UCLASS()
class EMBERTRIAL_API AETMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AETMenuGameMode();

protected:
    virtual void HandleStartingNewPlayer_Implementation(
        APlayerController* NewPlayer) override;
};