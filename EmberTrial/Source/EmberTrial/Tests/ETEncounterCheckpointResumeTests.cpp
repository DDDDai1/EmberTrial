#if WITH_DEV_AUTOMATION_TESTS

#include "Flow/ETEncounter.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETEncounterCheckpointResumeTest, "EmberTrial.M4.Encounter.CheckpointResume",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETEncounterCheckpointResumeTest::RunTest(const FString& Parameters)
{
    using EState = AETEncounter::EState;
    const FName Entry = UETRunSessionSubsystem::GetEntryCheckpointId();
    const FName CheckpointA(TEXT("Checkpoint_A"));
    const FName CheckpointB(TEXT("Checkpoint_B"));
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Gate test mesh"), Cube))
    {
        return false;
    }

    struct FCase
    {
        FName SpawnId;
        bool bSnapshotConfirmed;
        bool bIntroRestored;
        bool bMixedRestored;
        bool bDiskRestore = false;
    };
    const FCase Cases[] = {
        {Entry, true, false, false},
        {CheckpointA, true, true, false},
        {CheckpointB, true, true, true},
        {CheckpointB, false, false, false},
        {CheckpointB, true, true, true, true},
        {CheckpointB, true, false, false, true}
    };

    for (const FCase& Case : Cases)
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = Fixture.GetTestWorld();
        UGameInstance* Instance = World->GetGameInstance();
        UETRunSessionSubsystem* Session = Instance ? Instance->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
        if (!TestNotNull(TEXT("Run session in fixture world"), Session))
        {
            return false;
        }

        // Emulate the already committed snapshot. The test world has no saved map package.
        Session->GameplayPackage = UETRunSessionSubsystem::GetWorldPackage(World);
        Session->ActiveCheckpointId = Case.SpawnId;
        Session->bTravelPending = !Case.bSnapshotConfirmed;
        AETEncounter* Intro = World->SpawnActor<AETEncounter>();
        AETEncounter* Mixed = World->SpawnActor<AETEncounter>(FVector(800, 0, 0), FRotator::ZeroRotator);
        AETEncounter* Guardian = World->SpawnActor<AETEncounter>(FVector(1600, 0, 0), FRotator::ZeroRotator);
        if (!TestTrue(TEXT("Three fixture encounters"), Intro && Mixed && Guardian))
        {
            return false;
        }

        Intro->ClearedOnSpawnCheckpointIds = {CheckpointA, CheckpointB};
        Mixed->ClearedOnSpawnCheckpointIds = {CheckpointB, Entry};
        Intro->EncounterId = TEXT("intro");
        Mixed->EncounterId = TEXT("mixed");
        Guardian->EncounterId = TEXT("guardian");
        if (Case.bDiskRestore)
        {
            Session->RestoredClearedIds.Emplace();
            if (Case.bIntroRestored)
            {
                Session->RestoredClearedIds->Add(Intro->EncounterId);
            }
            if (Case.bMixedRestored)
            {
                Session->RestoredClearedIds->Add(Mixed->EncounterId);
            }
        }
        for (AETEncounter* Encounter : {Intro, Mixed, Guardian})
        {
            Encounter->ExitGate->SetStaticMesh(Cube);
        }

        if (!Fixture.BeginPlayInTestWorld() || !Fixture.TickTestWorld(0.02f) ||
            !Fixture.TickTestWorld(0.02f))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }

        const auto Verify = [this, &Case](const TCHAR* Room, AETEncounter& Encounter, bool bRestored)
        {
            TestTrue(FString::Printf(TEXT("%s at %s, confirmed %d"), Room, *Case.SpawnId.ToString(),
                                     Case.bSnapshotConfirmed ? 1 : 0),
                     Encounter.State == (bRestored ? EState::Cleared : EState::Waiting) &&
                     Encounter.ExitGate->IsCollisionEnabled() == !bRestored && Encounter.Remaining.IsEmpty());
        };
        Verify(TEXT("Intro"), *Intro, Case.bIntroRestored);
        Verify(TEXT("Mixed"), *Mixed, Case.bMixedRestored);
        Verify(TEXT("Guardian"), *Guardian, false);
        Session->bTravelPending = false;

        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        Fixture.ForwardErrorMessages(this);
    }
    return !HasAnyErrors();
}

#endif
