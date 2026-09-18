// Copyright Epic Games, Inc. All Rights Reserved.

#include "EmberTrialGameMode.h"
#include "EmberTrialCharacter.h"
#include "UObject/ConstructorHelpers.h"

AEmberTrialGameMode::AEmberTrialGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
