#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EmberTrialCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETEncounter.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "Flow/ETTrialExit.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETTrialVictoryTest, "EmberTrial.M4.Trial.Victory",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETTrialVictoryTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    UWorld* World = Fixture.GetTestWorld();
    UETRunSessionSubsystem* Session = World->GetGameInstance()->GetSubsystem<UETRunSessionSubsystem>();
    APlayerController* PC = World->SpawnActor<APlayerController>();
    AEmberTrialCharacter* Player = World->SpawnActor<AEmberTrialCharacter>(
        FVector(-500, 0, 0), FRotator::ZeroRotator);
    AETTrialExit* Exit = World->SpawnActor<AETTrialExit>();
    AETEncounter* Intro = World->SpawnActor<AETEncounter>(FVector(600, 0, 0), FRotator::ZeroRotator);
    AETEncounter* Mixed = World->SpawnActor<AETEncounter>(FVector(1200, 0, 0), FRotator::ZeroRotator);
    AETEncounter* Guardian = World->SpawnActor<AETEncounter>(FVector(1800, 0, 0), FRotator::ZeroRotator);
    if (!TestTrue(TEXT("Fixture session and actors"), Session && PC && Player && Exit && Intro && Mixed && Guardian))
    {
        return false;
    }

    Player->GetCapsuleComponent()->SetGenerateOverlapEvents(true);
    PC->Possess(Player);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Session->GameplayPackage = UETRunSessionSubsystem::GetWorldPackage(World);
    Session->ActiveCheckpointId = FName(TEXT("Checkpoint_B"));
    Intro->State = AETEncounter::EState::Cleared;
    Mixed->State = AETEncounter::EState::Cleared;
    Exit->RequiredEncounters = {Intro, Mixed, Guardian};

    int32 Notifications = 0;
    const FDelegateHandle Handle = Session->OnTrialCompleted.AddLambda([&Notifications, World](UWorld* Source)
    {
        if (Source == World)
        {
            ++Notifications;
        }
    });
    const auto CrossExit = [Player]()
    {
        Player->SetActorLocation(FVector(-500, 0, 0));
        Player->SetActorLocation(FVector::ZeroVector);
    };

    CrossExit();
    TestEqual(TEXT("Waiting guardian prevents completion"), Notifications, 0);
    Guardian->State = AETEncounter::EState::Failed;
    CrossExit();
    TestEqual(TEXT("A gate that failed open cannot count as cleared"), Notifications, 0);
    Guardian->State = AETEncounter::EState::Cleared;
    Exit->RequiredEncounters = {Intro, Intro, Guardian};
    CrossExit();
    TestEqual(TEXT("A repeated encounter reference cannot complete the trial"), Notifications, 0);
    Exit->RequiredEncounters = {Intro, Mixed, Guardian};
    CrossExit();
    CrossExit();
    TestEqual(TEXT("Repeated exit overlap only announces once"), Notifications, 1);
    TestEqual(TEXT("The completed result contains all three encounters"), Session->GetCompletedEncounterCount(World), 3);

    // A confirmed new arrival clears the finished run and commits the new Entry.
    Session->PendingIntent = EETRunTravelIntent::NewRun;
    Session->PendingTargetPackage = Session->GameplayPackage;
    Session->PendingCheckpointId = UETRunSessionSubsystem::GetEntryCheckpointId();
    Session->bPendingGameplayReady = true;
    Session->bTravelPending = true;
    Session->HandlePostLoadMap(World);
    TestEqual(TEXT("A confirmed new run clears the result"), Session->GetCompletedEncounterCount(World), 0);
    TestEqual(TEXT("A confirmed new run uses Entry"), Session->GetSpawnCheckpointId(World),
              UETRunSessionSubsystem::GetEntryCheckpointId());
    Session->OnTrialCompleted.Remove(Handle);

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
