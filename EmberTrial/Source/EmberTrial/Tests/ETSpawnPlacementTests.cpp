#if WITH_DEV_AUTOMATION_TESTS
#include "Framework/ETGameMode.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETSpawnPlacementTest, "EmberTrial.Framework.SpawnPlacement",
                                EAutomationTestFlags::EditorContext |
                                    EAutomationTestFlags::EngineFilter)

bool FETSpawnPlacementTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AETGameMode* Mode = World->SpawnActor<AETGameMode>();
    AActor* Floor = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Rules"), Mode) || !TestNotNull(TEXT("Floor"), Floor))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
    Floor->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(500, 500, 10));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Floor->SetActorLocation(FVector(0, 0, -10));

    const AEmberTrialCharacter* Body = GetDefault<AEmberTrialCharacter>();
    const double HalfHeight = Body->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const double Gap = (UCharacterMovementComponent::MIN_FLOOR_DIST +
                        UCharacterMovementComponent::MAX_FLOOR_DIST) * .5;
    const FTransform Start(FRotator(0, 45, 0), FVector(0, 0, 120));
    const FTransform Grounded = Mode->MakeGroundedSpawnTransform(Start, Body);
    TestTrue(TEXT("Initial player is above floor at native walking clearance"),
             FMath::IsNearlyEqual(Grounded.GetLocation().Z, HalfHeight + Gap, .01));
    TestTrue(TEXT("Grounding preserves horizontal spawn and rotation"),
             Grounded.GetLocation().X == Start.GetLocation().X &&
                 Grounded.GetLocation().Y == Start.GetLocation().Y &&
                 Grounded.GetRotation().Equals(Start.GetRotation(), .001));
    TestTrue(TEXT("Already grounded start remains stable"),
             Mode->MakeGroundedSpawnTransform(Grounded, Body).Equals(Grounded, .01));

    const FTransform High(FVector(0, 0, 1000));
    TestTrue(TEXT("High spawn is not pulled beyond probe distance"),
             Mode->MakeGroundedSpawnTransform(High, Body).Equals(High));
    const FTransform Embedded(FVector(0, 0, 20));
    TestTrue(TEXT("Penetrating start remains for normal spawn collision handling"),
             Mode->MakeGroundedSpawnTransform(Embedded, Body).Equals(Embedded));
    TestTrue(TEXT("Non-character default falls back unchanged"),
             Mode->MakeGroundedSpawnTransform(Start, nullptr).Equals(Start));
    for (float Invalid : {0.0f, -1.0f, 501.0f, std::numeric_limits<float>::quiet_NaN()})
    {
        Mode->SpawnGroundProbeDistance = Invalid;
        TestTrue(TEXT("Disabled or invalid probe preserves original spawn"),
                 Mode->MakeGroundedSpawnTransform(Start, Body).Equals(Start));
    }
    Mode->SpawnGroundProbeDistance = 100.0f;
    Floor->Destroy();
    TestTrue(TEXT("Missing floor preserves original spawn"),
             Mode->MakeGroundedSpawnTransform(Start, Body).Equals(Start));
    return !HasAnyErrors();
}
#endif
