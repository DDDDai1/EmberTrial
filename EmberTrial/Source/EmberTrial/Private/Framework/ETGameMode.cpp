#include "Framework/ETGameMode.h"
#include "Framework/ETPlayerController.h"
#include "Characters/EmberTrialCharacter.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGameMode, Log, All);

AETGameMode::AETGameMode()
{
    PlayerControllerClass = AETPlayerController::StaticClass();
    DefaultPawnClass = AEmberTrialCharacter::StaticClass();
}

void AETGameMode::StartPlay()
{
    Super::StartPlay();

    UE_LOG(LogETGameMode, Display,
        TEXT("[M1-02B] Rules ready GameMode=%s ControllerClass=%s PawnClass=%s"),
        *GetNameSafe(this),
        *GetNameSafe(PlayerControllerClass.Get()),
        *GetNameSafe(DefaultPawnClass.Get()));
}