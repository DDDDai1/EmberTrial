#include "Flow/ETEncounter.h"

#include "AIController.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogETEncounter, Log, All);

AETEncounter::AETEncounter()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));

    EntryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryTrigger"));
    EntryTrigger->SetupAttachment(GetRootComponent());
    EntryTrigger->SetBoxExtent(FVector(120.0, 160.0, 140.0));
    EntryTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    EntryTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    EntryTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    EntryTrigger->SetGenerateOverlapEvents(true);
    EntryTrigger->SetCanEverAffectNavigation(false);

    ExitGate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExitGate"));
    ExitGate->SetupAttachment(GetRootComponent());
    ExitGate->SetCollisionProfileName(TEXT("BlockAll"));
    ExitGate->SetGenerateOverlapEvents(false);
    ExitGate->SetCanEverAffectNavigation(false);
}

void AETEncounter::BeginPlay()
{
    Super::BeginPlay();
    EntryTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleEntry);
    InitialOverlapTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::ProbeInitialOverlap);
}

void AETEncounter::HandleEntry(UPrimitiveComponent* Overlapped, AActor* Other,
                               UPrimitiveComponent* OtherComponent, int32 BodyIndex,
                               bool bFromSweep, const FHitResult& Hit)
{
    TryStart(Cast<AEmberTrialCharacter>(Other));
}

void AETEncounter::ProbeInitialOverlap()
{
    InitialOverlapTimer.Invalidate();
    if (RestoreClearedEncounter())
    {
        return;
    }
    TArray<AActor*> Bodies;
    EntryTrigger->GetOverlappingActors(Bodies, AEmberTrialCharacter::StaticClass());
    for (AActor* Body : Bodies)
    {
        TryStart(Cast<AEmberTrialCharacter>(Body));
    }
}

bool AETEncounter::RestoreClearedEncounter()
{
    if (State != EState::Waiting || !HasAuthority())
    {
        return false;
    }

    UWorld* World = GetWorld();
    UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
    const UETRunSessionSubsystem* Session = Instance ? Instance->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!Session)
    {
        return false;
    }

    bool bDiskStateKnown = false;
    if (Session->IsSavedEncounterCleared(World, EncounterId, bDiskStateKnown))
    {
        State = EState::Cleared;
        OpenGate();
        UE_LOG(LogETEncounter, Display, TEXT("Encounter restored from disk Id=%s"), *EncounterId.ToString());
        return true;
    }
    if (bDiskStateKnown)
    {
        return false;
    }

    const FName* RestoredId = ClearedOnSpawnCheckpointIds.FindByPredicate([Session, World](FName Id)
    {
        return Id != UETRunSessionSubsystem::GetEntryCheckpointId() && Session->IsCheckpointActive(World, Id);
    });
    if (!RestoredId)
    {
        return false;
    }

    State = EState::Cleared;
    OpenGate();
    UE_LOG(LogETEncounter, Display, TEXT("Encounter restored Actor=%s Checkpoint=%s"),
           *GetName(), *RestoredId->ToString());
    return true;
}

bool AETEncounter::HasValidSpawns() const
{
    if (Spawns.IsEmpty())
    {
        return false;
    }

    TSet<const ATargetPoint*> Seen;
    for (const FETEncounterSpawn& Spawn : Spawns)
    {
        const UClass* Class = Spawn.EnemyClass.Get();
        const AEmberTrialCharacter* Defaults = Spawn.EnemyClass.GetDefaultObject();
        const ATargetPoint* Point = Spawn.Point.Get();
        if (!IsValid(Class) || Class->HasAnyClassFlags(CLASS_Abstract) ||
            !Defaults || Defaults->GetCombatTeam() != EETCombatTeam::Enemy ||
            !IsValid(Point) || Point->GetWorld() != GetWorld() || Seen.Contains(Point))
        {
            return false;
        }
        Seen.Add(Point);
    }
    return true;
}

void AETEncounter::TryStart(AEmberTrialCharacter* Player)
{
    const UWorld* World = GetWorld();
    const APlayerController* PC = IsValid(Player) ? Cast<APlayerController>(Player->GetController()) : nullptr;
    if (State != EState::Waiting || !HasAuthority() || !World || World->bIsTearingDown ||
        World->IsPaused() || !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn() != Player ||
        Player->GetWorld() != World || Player->GetCombatTeam() != EETCombatTeam::Player ||
        !Player->IsGameplayReady() || Player->IsCombatDead())
    {
        return;
    }

    if (!HasValidSpawns())
    {
        Fail(TEXT("InvalidSpawns"));
        return;
    }

    // Commit the state before spawning to reject overlap reentry.
    State = EState::Active;
    for (const FETEncounterSpawn& Spawn : Spawns)
    {
        if (!SpawnEnemy(Spawn))
        {
            Fail(TEXT("EnemySpawnFailed"));
            return;
        }
    }

    UE_LOG(LogETEncounter, Display, TEXT("Encounter started Actor=%s Enemies=%d"),
           *GetName(), Remaining.Num());
}

bool AETEncounter::SpawnEnemy(const FETEncounterSpawn& Spawn)
{
    if (!IsValid(Spawn.Point.Get()))
    {
        return false;
    }
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    AEmberTrialCharacter* Enemy = GetWorld()->SpawnActor<AEmberTrialCharacter>(
        Spawn.EnemyClass, Spawn.Point->GetActorLocation(), Spawn.Point->GetActorRotation(), Params);
    if (!IsValid(Enemy))
    {
        return false;
    }

    UETCombatLifeComponent* Life = Enemy->FindComponentByClass<UETCombatLifeComponent>();
    if (!IsValid(Life) || !Enemy->IsGameplayReady() || Enemy->IsCombatDead() ||
        !IsValid(Cast<AAIController>(Enemy->GetController())))
    {
        Enemy->Destroy();
        return false;
    }

    FTrackedEnemy& Tracked = Remaining.Add(Enemy);
    Tracked.Life = Life;
    Tracked.DeathHandle = Life->OnDeathSealed.AddUObject(this, &ThisClass::HandleDeath);
    Enemy->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleEnemyEndPlay);
    return true;
}

void AETEncounter::HandleDeath(UETCombatLifeComponent* Life)
{
    AEmberTrialCharacter* Enemy = Cast<AEmberTrialCharacter>(Life->GetOwner());
    const FTrackedEnemy* Tracked = Remaining.Find(Enemy);
    if (State != EState::Active || !Tracked || Tracked->Life.Get() != Life || !Life->IsDead())
    {
        return;
    }

    Life->OnDeathSealed.Remove(Tracked->DeathHandle);
    Enemy->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleEnemyEndPlay);
    Remaining.Remove(Enemy);
    UE_LOG(LogETEncounter, Display, TEXT("Encounter enemy defeated Actor=%s Remaining=%d"),
           *GetName(), Remaining.Num());

    if (Remaining.IsEmpty())
    {
        State = EState::Cleared;
        OpenGate();
        UE_LOG(LogETEncounter, Display, TEXT("Encounter cleared Actor=%s"), *GetName());
    }
}

void AETEncounter::HandleEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
    if (State == EState::Active && Reason == EEndPlayReason::Destroyed &&
        !GetWorld()->bIsTearingDown)
    {
        Fail(TEXT("EnemyDestroyedBeforeDeath"));
    }
}

void AETEncounter::Fail(const TCHAR* Reason)
{
    State = EState::Failed;
    DetachEnemies(true);
    OpenGate();
    UE_LOG(LogETEncounter, Warning, TEXT("Encounter failed Actor=%s Reason=%s"), *GetName(), Reason);
}

void AETEncounter::OpenGate()
{
    ExitGate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExitGate->SetHiddenInGame(true);
}

void AETEncounter::DetachEnemies(bool bDestroy)
{
    for (const auto& Pair : Remaining)
    {
        if (UETCombatLifeComponent* Life = Pair.Value.Life.Get())
        {
            Life->OnDeathSealed.Remove(Pair.Value.DeathHandle);
        }
        if (AEmberTrialCharacter* Enemy = Pair.Key.Get())
        {
            Enemy->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleEnemyEndPlay);
            if (bDestroy)
            {
                Enemy->Destroy();
            }
        }
    }
    Remaining.Reset();
}

void AETEncounter::EndPlay(const EEndPlayReason::Type Reason)
{
    EntryTrigger->OnComponentBeginOverlap.RemoveDynamic(this, &ThisClass::HandleEntry);
    GetWorldTimerManager().ClearTimer(InitialOverlapTimer);
    const bool bDestroyEnemies = Reason == EEndPlayReason::Destroyed && !GetWorld()->bIsTearingDown;
    State = EState::Failed;
    DetachEnemies(bDestroyEnemies);
    Super::EndPlay(Reason);
}
