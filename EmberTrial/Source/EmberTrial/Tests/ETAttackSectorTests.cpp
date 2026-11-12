#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ETAttackSector.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/ETANS_AttackWindow.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAttackSectorTest, "EmberTrial.M4.Guardian.SweepSector",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAttackSectorTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    UWorld* World = Fixture.GetTestWorld();
    const auto Spawn = [World](const FVector& Position)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        return World->SpawnActor<AEmberTrialCharacter>(Position, FRotator::ZeroRotator, Params);
    };
    AEmberTrialCharacter* Source = Spawn(FVector::ZeroVector);
    AEmberTrialCharacter* Ahead = Spawn(FVector(145.0, 0.0, 0.0));
    AEmberTrialCharacter* Flank = Spawn(FVector(80.0, 100.0, 0.0));
    AEmberTrialCharacter* Behind = Spawn(FVector(-110.0, 0.0, 0.0));
    AEmberTrialCharacter* Far = Spawn(FVector(160.0, 0.0, 0.0));
    AEmberTrialCharacter* Above = Spawn(FVector(110.0, -60.0, 80.0));
    if (!TestTrue(TEXT("Source and targets spawned"), Source && Ahead && Flank && Behind && Far && Above))
    {
        return false;
    }

    AActor* Wall = World->SpawnActor<AActor>();
    UBoxComponent* Barrier = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Barrier);
    Barrier->InitBoxExtent(FVector(8.0, 18.0, 60.0));
    Barrier->SetCollisionProfileName(TEXT("BlockAll"));
    Barrier->RegisterComponent();
    Wall->SetActorLocation(FVector(75.0, 0.0, 0.0));
    Barrier->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    FETAttackSector Sector;
    TArray<FHitResult> Hits;
    const auto Sample = [&]() { ETAttackSector::GatherHits(*World, *Source, Sector, Hits); };
    const auto HasTarget = [&Hits](const AActor* Target)
    {
        return Hits.ContainsByPredicate([Target](const FHitResult& Hit) { return Hit.GetActor() == Target; });
    };

    Sample();
    TestEqual(TEXT("One candidate per target in sector"), Hits.Num(), 2);
    TestTrue(TEXT("Center and flank are hit"), HasTarget(Ahead) && HasTarget(Flank));
    TestFalse(TEXT("Rear, range, height and source are excluded"),
              HasTarget(Behind) || HasTarget(Far) || HasTarget(Above) || HasTarget(Source));
    TestFalse(TEXT("Area candidates do not block each other in the trace dispatcher"),
              Hits.ContainsByPredicate([](const FHitResult& Hit) { return Hit.bBlockingHit; }));

    Barrier->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sample();
    TestTrue(TEXT("Wall blocks only its occluded target"), Hits.Num() == 1 && HasTarget(Flank));

    Source->SetActorRotation(FRotator(0.0, 180.0, 0.0));
    Sample();
    TestTrue(TEXT("Rotating source moves the sector"), Hits.Num() == 1 && HasTarget(Behind));

    Sector.Range = 0.0f;
    Sample();
    TestTrue(TEXT("Invalid configuration clears candidates"), Hits.IsEmpty());

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAttackSectorWindowTest, "EmberTrial.M4.Guardian.SweepWindow",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAttackSectorWindowTest::RunTest(const FString& Parameters)
{
    const UAnimMontage* Template = LoadObject<UAnimMontage>(
        nullptr, TEXT("/Game/EmberTrial/Animations/Combo/Montages/AM_ET_Light_03.AM_ET_Light_03"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestTrue(TEXT("Montage and compatible skeleton mesh available"), Template && Quinn))
    {
        return false;
    }

    TStrongObjectPtr<UAnimMontage> Montage(DuplicateObject<UAnimMontage>(Template, GetTransientPackage()));
    FAnimNotifyEvent* Event = Montage->Notifies.FindByPredicate([](const FAnimNotifyEvent& Notify)
    {
        return Notify.NotifyStateClass && Notify.NotifyStateClass->IsA<UETANS_AttackWindow>();
    });
    if (!TestNotNull(TEXT("Template has a real attack notify"), Event))
    {
        return false;
    }
    Event->NotifyStateClass = NewObject<UETANS_SectorAttackWindow>(Montage.Get());
    Montage->RefreshCacheData();

    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AEmberTrialCharacter* Source = World->SpawnActor<AEmberTrialCharacter>();
    AEmberTrialCharacter* Target = World->SpawnActor<AEmberTrialCharacter>(
        FVector(125.0, 0.0, 0.0), FRotator::ZeroRotator);
    if (!TestTrue(TEXT("Sweep fixture bodies spawned"), Source && Target))
    {
        return false;
    }
    Source->GetMesh()->SetSkeletalMeshAsset(Quinn);
    Source->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Source->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    UETCombatTraceComponent* Trace = Source->FindComponentByClass<UETCombatTraceComponent>();
    UAnimInstance* Anim = Source->GetMesh()->GetAnimInstance();
    if (!TestTrue(TEXT("Trace and anim instance ready"), Trace && Anim))
    {
        return false;
    }

    TMap<uint32, int32> Candidates;
    Trace->OnTraceHit.AddLambda([&Candidates, Target](uint32 Serial, const FHitResult& Hit)
    {
        if (Hit.GetActor() == Target)
        {
            ++Candidates.FindOrAdd(Serial);
        }
    });
    const auto Arm = [&]()
    {
        if (Anim->Montage_Play(Montage.Get()) <= 0.0f)
        {
            return uint32(0);
        }
        const FAnimMontageInstance* Instance = Anim->GetActiveInstanceForMontage(Montage.Get());
        return Instance ? Trace->BeginAttack(Montage.Get(), Instance->GetInstanceID()) : 0;
    };
    const auto Advance = [&Fixture](float Duration)
    {
        for (int32 Frame = 0; Frame < FMath::CeilToInt(Duration / 0.01f); ++Frame)
        {
            if (!Fixture.TickTestWorld(0.01f))
            {
                return false;
            }
        }
        return true;
    };

    const uint32 Cancelled = Arm();
    if (!TestTrue(TEXT("Montage armed for cancellation"), Cancelled != 0) || !Advance(0.05f))
    {
        return false;
    }
    TestTrue(TEXT("Anticipation has no candidate and no trace tick"), Candidates.IsEmpty() &&
             !Trace->IsComponentTickEnabled());
    Trace->EndAttack(Cancelled);
    if (!Advance(Montage->GetPlayLength()))
    {
        return false;
    }
    TestFalse(TEXT("Retired montage cannot dispatch late hits"), Candidates.Contains(Cancelled));

    const uint32 Active = Arm();
    if (!TestTrue(TEXT("New montage instance arms a fresh serial"), Active != 0 && Active != Cancelled) ||
        !Advance(Montage->GetPlayLength()))
    {
        return false;
    }
    TestEqual(TEXT("A real notify dispatches exactly one candidate"), Candidates.FindRef(Active), 1);
    TestFalse(TEXT("Sector sampling disables trace ticking again"), Trace->IsComponentTickEnabled());
    Trace->EndAttack(Active);
    Trace->OnTraceHit.Clear();

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
