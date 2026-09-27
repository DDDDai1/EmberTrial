
#include "ETPracticeActionComponent.h"

#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPracticeAction, Log, All);

UETPracticeActionComponent::UETPracticeActionComponent()
{
	
	PrimaryComponentTick.bCanEverTick = false;


}

EETPracticeActionState UETPracticeActionComponent::GetPracticeActionState() const
{
	return CurrentState;
}

bool UETPracticeActionComponent::IsPracticeActionIdle() const
{
	return CurrentState == EETPracticeActionState::Idle;
}


// Called when the game starts
void UETPracticeActionComponent::BeginPlay()
{
	Super::BeginPlay();

	const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());

	if (!IsValid(CharacterOwner))
	{
		UE_LOG(LogETPracticeAction, Error, TEXT("[M0-05C2] Component requires an ACharacter owner: %s"),
			*GetNameSafe(GetOwner()));
		return;
	}

	UE_LOG(LogETPracticeAction, Display,
		TEXT("[M0-05C2] Component ready Owner=%s State=Idle"),
		*GetNameSafe(CharacterOwner));
}



