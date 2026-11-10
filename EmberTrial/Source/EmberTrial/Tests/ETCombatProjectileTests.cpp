#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ETCombatProjectile.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETCombatProjectileFlightTest,
    "EmberTrial.M4.Combat.ProjectileFlight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETCombatProjectileFlightTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    auto SpawnBlocker = [World](const FVector& Position, const FVector& Extent,
                                const ECollisionChannel Channel) -> AActor*
    {
        AActor* Blocker = World->SpawnActor<AActor>();
        if (!IsValid(Blocker))
        {
            return nullptr;
        }
        UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
        Blocker->SetRootComponent(Box);
        Box->InitBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionObjectType(Channel);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->SetGenerateOverlapEvents(false);
        Box->RegisterComponent();
        Blocker->SetActorLocation(Position);
        return Blocker;
    };
    AActor* Source = SpawnBlocker(FVector::ZeroVector, FVector(40.0), ECC_Pawn);
    AActor* Wall = SpawnBlocker(FVector(400.0, 0.0, 0.0), FVector(3.0, 80.0, 80.0), ECC_WorldStatic);
    AETCombatProjectile* HitProjectile = World->SpawnActor<AETCombatProjectile>();
    AETCombatProjectile* MissProjectile = World->SpawnActor<AETCombatProjectile>(
        FVector(-200.0, 0.0, 0.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Source collision at launch origin"), Source) ||
        !TestNotNull(TEXT("Thin blocking wall"), Wall) ||
        !TestNotNull(TEXT("Wall projectile"), HitProjectile) ||
        !TestNotNull(TEXT("Miss projectile"), MissProjectile) ||
        !Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    const FVector Origin = HitProjectile->GetActorLocation();
    TestFalse(TEXT("Reject null source"), HitProjectile->TryLaunch(nullptr, FVector::ForwardVector));
    TestFalse(TEXT("Reject zero direction"), HitProjectile->TryLaunch(Source, FVector::ZeroVector));
    if (!Fixture.TickTestWorld(0.10f))
    {
        return false;
    }
    TestTrue(TEXT("Unlaunched projectile remains inert"), HitProjectile->GetActorLocation().Equals(Origin));

    int32 WallImpacts = 0;
    int32 MissImpacts = 0;
    AActor* FirstHit = nullptr;
    AActor* ReportedSource = nullptr;
    bool bRelaunchFromImpact = false;
    HitProjectile->OnImpact.AddLambda([&](AETCombatProjectile* Projectile, const FHitResult& Hit)
    {
        ++WallImpacts;
        FirstHit = Hit.GetActor();
        ReportedSource = Projectile->GetSourceActor();
        bRelaunchFromImpact = Projectile->TryLaunch(Source, FVector::RightVector);
    });
    MissProjectile->OnImpact.AddLambda([&](AETCombatProjectile*, const FHitResult&) { ++MissImpacts; });

    const TWeakObjectPtr<AETCombatProjectile> HitRef(HitProjectile);
    const TWeakObjectPtr<AETCombatProjectile> MissRef(MissProjectile);
    if (!TestTrue(TEXT("Launch overlapping source"), HitProjectile->TryLaunch(Source, FVector::ForwardVector)) ||
        !TestTrue(TEXT("Launch down the clear lane"), MissProjectile->TryLaunch(Source, FVector::RightVector)))
    {
        return false;
    }
    TestFalse(TEXT("Reject second launch"), HitProjectile->TryLaunch(Source, FVector::RightVector));
    if (!Fixture.TickTestWorld(0.10f) ||
        !TestTrue(TEXT("First projectile clears its source and is in flight"), HitRef.IsValid()) ||
        !TestTrue(TEXT("Second projectile is in flight"), MissRef.IsValid()))
    {
        return false;
    }
    TestTrue(TEXT("First projectile moved forward"), HitRef->GetActorLocation().X > 40.0);
    TestTrue(TEXT("Second projectile moved along its own direction"), MissRef->GetActorLocation().Y > 40.0);
    TestEqual(TEXT("No impact before the wall"), WallImpacts, 0);

    for (int32 Step = 0; Step < 34; ++Step)
    {
        if (!Fixture.TickTestWorld(0.10f))
        {
            return false;
        }
    }
    TestEqual(TEXT("Only one blocking impact"), WallImpacts, 1);
    TestTrue(TEXT("First hit is the wall, not the source"), FirstHit == Wall);
    TestTrue(TEXT("Impact carries the original source"), ReportedSource == Source);
    TestFalse(TEXT("Impact listener cannot relaunch"), bRelaunchFromImpact);
    TestFalse(TEXT("Wall projectile is removed"), HitRef.IsValid());
    TestFalse(TEXT("Miss projectile expires by lifetime"), MissRef.IsValid());
    TestEqual(TEXT("Lifetime expiry emits no impact"), MissImpacts, 0);

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif