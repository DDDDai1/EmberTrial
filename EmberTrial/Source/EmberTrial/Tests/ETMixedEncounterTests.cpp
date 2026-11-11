#if WITH_DEV_AUTOMATION_TESTS

#include "Flow/ETEncounter.h"

#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETMixedEncounterTest, "EmberTrial.M4.Encounter.Mixed",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETMixedEncounterTest::RunTest(const FString& Parameters)
{
    using EState = AETEncounter::EState;
    const TSubclassOf<AEmberTrialCharacter> MeleeClass = LoadClass<AEmberTrialCharacter>(
        nullptr, TEXT("/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy.BP_ETMeleeEnemy_C"));
    const TSubclassOf<AEmberTrialCharacter> RangedClass = LoadClass<AEmberTrialCharacter>(
        nullptr, TEXT("/Game/EmberTrial/Characters/Enemies/BP_ETRangedGuard_Native.BP_ETRangedGuard_Native_C"));
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestTrue(TEXT("Both saved enemy classes plus cube"), MeleeClass && RangedClass && Cube))
    {
        return false;
    }

    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    if (!TestTrue(TEXT("AI system initialized"), World->CreateAISystem() != nullptr))
    {
        return false;
    }

    AETEncounter* Encounter = World->SpawnActor<AETEncounter>();
    AETEncounter* Invalid = World->SpawnActor<AETEncounter>(FVector(0, -700, 0), FRotator::ZeroRotator);
    ATargetPoint* Melee = World->SpawnActor<ATargetPoint>(FVector(450, 0, 140), FRotator::ZeroRotator);
    ATargetPoint* Ranged = World->SpawnActor<ATargetPoint>(FVector(750, 500, 140), FRotator::ZeroRotator);
    APlayerController* PC = World->SpawnActor<APlayerController>();
    AEmberTrialCharacter* Player = World->SpawnActor<AEmberTrialCharacter>(
        FVector(-500, 0, 0), FRotator::ZeroRotator);
    AEmberTrialCharacter* Neutral = World->SpawnActor<AEmberTrialCharacter>(
        FVector(-500, 350, 0), FRotator::ZeroRotator);
    if (!TestTrue(TEXT("All fixture actors spawned"), Encounter && Invalid && Melee && Ranged && PC && Player && Neutral))
    {
        return false;
    }

    Encounter->Spawns = {{MeleeClass, Melee}, {RangedClass, Ranged}};
    Invalid->Spawns = {{MeleeClass, Melee}, {RangedClass, Melee}};
    for (AETEncounter* Actor : {Encounter, Invalid})
    {
        Actor->ExitGate->SetStaticMesh(Cube);
        Actor->ExitGate->SetRelativeLocation(FVector(1300, 0, 0));
    }
    // Use the native player body because the isolated world has no viewport for Blueprint HUD creation.
    Player->CombatTeam = EETCombatTeam::Player;
    Player->GetCapsuleComponent()->SetGenerateOverlapEvents(true);
    PC->Possess(Player);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    Neutral->SetActorLocation(FVector(0, 0, 0));
    TestTrue(TEXT("Neutral overlap cannot start"), Encounter->State == EState::Waiting);
    TestTrue(TEXT("Exit starts blocked"), Encounter->ExitGate->IsCollisionEnabled());
    Neutral->SetActorLocation(FVector(-500, 350, 0));
    Player->SetActorLocation(FVector(0, 0, 0));
    if (!TestTrue(TEXT("Player overlap spawns exactly two"), Encounter->State == EState::Active &&
                                                           Encounter->Remaining.Num() == 2))
    {
        return false;
    }

    TArray<TWeakObjectPtr<AEmberTrialCharacter>> Enemies;
    Encounter->Remaining.GetKeys(Enemies);
    const auto Kill = [](AEmberTrialCharacter* Body)
    {
        Body->GetAbilitySystemComponent()->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    };
    Kill(Neutral);
    TestEqual(TEXT("An unrelated death does not count"), Encounter->Remaining.Num(), 2);
    Kill(Enemies[0].Get());
    TestTrue(TEXT("First death leaves gate blocked"), Encounter->State == EState::Active &&
                                                      Encounter->Remaining.Num() == 1 &&
                                                      Encounter->ExitGate->IsCollisionEnabled());

    Player->SetActorLocation(FVector(-500, 0, 0));
    Player->SetActorLocation(FVector(0, 0, 0));
    TestEqual(TEXT("Reentry during a wave cannot spawn again"), Encounter->Remaining.Num(), 1);
    Kill(Enemies[1].Get());
    TestTrue(TEXT("Second death opens exit"), Encounter->State == EState::Cleared &&
                                              Encounter->Remaining.IsEmpty() &&
                                              !Encounter->ExitGate->IsCollisionEnabled());
    Player->SetActorLocation(FVector(-500, 0, 0));
    Player->SetActorLocation(FVector(0, 0, 0));
    TestTrue(TEXT("Reentry after clearing remains cleared"), Encounter->State == EState::Cleared &&
                                                             Encounter->Remaining.IsEmpty());

    AddExpectedMessagePlain(TEXT("Reason=InvalidSpawns"), ELogVerbosity::Warning,
                            EAutomationExpectedMessageFlags::Contains, 1);
    Player->SetActorLocation(FVector(0, -700, 0));
    TestTrue(TEXT("Duplicate spawn points fail before spawning and leave exit passable"),
             Invalid->State == EState::Failed && Invalid->Remaining.IsEmpty() && !Invalid->ExitGate->IsCollisionEnabled());
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
