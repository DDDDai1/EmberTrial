#include "Flow/ETTrialExit.h"

#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETEncounter.h"
#include "Flow/ETRunSessionSubsystem.h"

AETTrialExit::AETTrialExit()
{
    PrimaryActorTick.bCanEverTick = false;
    ExitTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ExitTrigger"));
    SetRootComponent(ExitTrigger);
    ExitTrigger->SetBoxExtent(FVector(90.0, 160.0, 130.0));
    ExitTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ExitTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    ExitTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ExitTrigger->SetGenerateOverlapEvents(true);
    ExitTrigger->SetCanEverAffectNavigation(false);
}

void AETTrialExit::BeginPlay()
{
    Super::BeginPlay();
    ExitTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleOverlap);
}

void AETTrialExit::HandleOverlap(UPrimitiveComponent* Component, AActor* Other,
                                UPrimitiveComponent* OtherComponent, int32 BodyIndex,
                                bool bFromSweep, const FHitResult& Hit)
{
    AEmberTrialCharacter* Player = Cast<AEmberTrialCharacter>(Other);
    UWorld* World = GetWorld();
    if (!HasAuthority() || !IsValid(Player) || !World || RequiredEncounters.IsEmpty())
    {
        return;
    }

    TSet<const AETEncounter*> Cleared;
    for (const AETEncounter* Encounter : RequiredEncounters)
    {
        if (!IsValid(Encounter) || Encounter->GetWorld() != World ||
            !Encounter->HasCleared() || Cleared.Contains(Encounter))
        {
            return;
        }
        Cleared.Add(Encounter);
    }

    if (UGameInstance* Instance = World->GetGameInstance())
    {
        if (UETRunSessionSubsystem* Session = Instance->GetSubsystem<UETRunSessionSubsystem>())
        {
            Session->TryCompleteTrial(World, Player, Cleared.Num());
        }
    }
}
