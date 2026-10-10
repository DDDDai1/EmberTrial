#include "Framework/ETMenuGameMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMenuGameMode, Log, All);

AETMenuGameMode::AETMenuGameMode()
{
    DefaultPawnClass = nullptr;
    SpectatorClass = nullptr;
    HUDClass = nullptr;
}

void AETMenuGameMode::HandleStartingNewPlayer_Implementation(
    APlayerController* NewPlayer)
{
    // Menu players have no Pawn. Do not call the default RestartPlayer path.
    UE_LOG(LogETMenuGameMode, Display,
        TEXT("[M1-06A] Menu player ready Controller=%s Pawn=%s"),
        *GetNameSafe(NewPlayer),
        *GetNameSafe(IsValid(NewPlayer) ? NewPlayer->GetPawn() : nullptr));
}