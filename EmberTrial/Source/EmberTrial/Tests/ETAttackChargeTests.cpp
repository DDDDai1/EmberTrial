#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/ETANS_AttackWindow.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAttackChargeTest, "EmberTrial.M4.Guardian.ChargeWindow",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAttackChargeTest::RunTest(const FString& Parameters)
{
    const UAnimMontage* Template = LoadObject<UAnimMontage>(
        nullptr, TEXT("/Game/EmberTrial/Animations/Guardian/AM_ET_GuardianSweep_01.AM_ET_GuardianSweep_01"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestTrue(TEXT("Saved guardian montage and skeleton mesh available"), Template && Quinn))
    {
        return false;
    }
    TStrongObjectPtr<UAnimMontage> Montage(DuplicateObject<UAnimMontage>(Template, GetTransientPackage()));
    FAnimNotifyEvent* Event = Montage->Notifies.FindByPredicate([](const FAnimNotifyEvent& Notify)
    {
        return Notify.NotifyStateClass && Notify.NotifyStateClass->IsA<UETANS_AttackWindow>();
    });
    if (!TestNotNull(TEXT("Saved montage has an attack notify"), Event))
    {
        return false;
    }

    // Keep the saved montage's length and use a long enough window for the full burst.
    Event->NotifyStateClass = NewObject<UETANS_ChargeAttackWindow>(Montage.Get());
    Montage->RateScale = 0.60f;
    Event->SetTime(8.0f / 30.0f);
    Event->SetDuration(7.0f / 30.0f);
    Montage->RefreshCacheData();

    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    const auto AddBox = [World](const FVector& Location, const FVector& Extent)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetRelativeLocation(Location);
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->RegisterComponent();
        return Box;
    };
    AddBox(FVector(0.0, 0.0, -50.0), FVector(1000.0, 1000.0, 50.0));
    UBoxComponent* Wall = AddBox(FVector(125.0, 0.0, 100.0), FVector(10.0, 300.0, 100.0));
    Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    const FVector Start(0.0, 0.0, 98.0);
    AEmberTrialCharacter* Source = World->SpawnActor<AEmberTrialCharacter>(Start, FRotator::ZeroRotator);
    AEmberTrialCharacter* Target = World->SpawnActor<AEmberTrialCharacter>(
        FVector(220.0, 0.0, 98.0), FRotator::ZeroRotator);
    if (!TestTrue(TEXT("Charge fixture bodies spawned"), Source && Target))
    {
        return false;
    }

    Source->GetMesh()->SetSkeletalMeshAsset(Quinn);
    Source->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Source->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Source->GetCharacterMovement()->bRunPhysicsWithNoController = true;
    Source->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);

    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UETCombatTraceComponent* Trace = Source->FindComponentByClass<UETCombatTraceComponent>();
    UAnimInstance* Anim = Source->GetMesh()->GetAnimInstance();
    UCharacterMovementComponent* Move = Source->GetCharacterMovement();
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
    if (!TestTrue(TEXT("Trace and animation instance ready"), Trace && Anim) || !Advance(0.25f) ||
        !TestTrue(TEXT("Fixture on floor"), Move->IsMovingOnGround()))
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
    const auto Reset = [&](uint32 Serial)
    {
        Trace->EndAttack(Serial);
        Anim->Montage_Stop(0.0f, Montage.Get());
        Source->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
        return Advance(0.05f);
    };

    uint32 Serial = Arm();
    if (!TestTrue(TEXT("Clear path montage armed"), Serial != 0) || !Advance(0.24f))
    {
        return false;
    }
    TestTrue(TEXT("Windup has no root motion or hit"), !Move->HasRootMotionSources() && Candidates.IsEmpty());
    if (!Advance(1.0f))
    {
        return false;
    }
    TestEqual(TEXT("Moving charge dispatches one candidate across its whole path"), Candidates.FindRef(Serial), 1);
    TestTrue(TEXT("Body advances and charge leaves no active source"), Source->GetActorLocation().X > 100.0 &&
             !Move->HasRootMotionSources() && !Trace->IsComponentTickEnabled());
    if (!Reset(Serial))
    {
        return false;
    }

    Wall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Serial = Arm();
    if (!TestTrue(TEXT("Wall trial armed"), Serial != 0) || !Advance(1.1f))
    {
        return false;
    }
    TestTrue(TEXT("Wall blocks travel and prevents damage behind it"), Source->GetActorLocation().X < 100.0 &&
             !Candidates.Contains(Serial) && !Move->HasRootMotionSources());
    Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (!Reset(Serial))
    {
        return false;
    }

    Serial = Arm();
    if (!TestTrue(TEXT("Cancel trial armed"), Serial != 0) || !Advance(0.50f))
    {
        return false;
    }
    TestTrue(TEXT("Charge active before cancellation"), Move->HasRootMotionSources());
    Trace->EndAttack(Serial);
    const double CancelX = Source->GetActorLocation().X;
    if (!Advance(0.65f))
    {
        return false;
    }
    TestTrue(TEXT("Cancellation stops displacement and late hits"), FMath::Abs(Source->GetActorLocation().X - CancelX) < 1.0 &&
             !Candidates.Contains(Serial) && !Trace->IsComponentTickEnabled() && !Move->HasRootMotionSources());

    Target->SetActorLocation(FVector(220.0, 240.0, 98.0));
    if (!Reset(Serial))
    {
        return false;
    }
    Serial = Arm();
    if (!TestTrue(TEXT("Open floor trial armed"), Serial != 0) || !Advance(0.50f))
    {
        return false;
    }
    Source->SetActorRotation(FRotator(0.0, 90.0, 0.0));
    if (!Advance(0.65f))
    {
        return false;
    }
    const FVector End = Source->GetActorLocation();
    TestTrue(TEXT("Open floor reaches 240 cm along the original direction"),
             FMath::Abs(End.X - 240.0) < 2.0 && FMath::Abs(End.Y) < 1.0 &&
             !Candidates.Contains(Serial) && !Move->HasRootMotionSources());
    Trace->EndAttack(Serial);
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
