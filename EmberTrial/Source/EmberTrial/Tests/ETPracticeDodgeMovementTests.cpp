#if WITH_DEV_AUTOMATION_TESTS

#include "../ETPracticeActionComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETPracticeDodgeMovementTest,
    "EmberTrial.M0.PracticeAction.DodgeMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

    bool FETPracticeDodgeMovementTest::RunTest(const FString& Parameters)
{
    using State = EETPracticeActionState;
    for (const int32 StepRate : {30, 60, 120})
    {
        AddInfo(FString::Printf(TEXT("[DodgeMovement] Begin StepRate=%d"), StepRate));
        FTestWorldWrapper TestWorld;
        if (!TestWorld.CreateTestWorld(EWorldType::Game))
        {
            TestWorld.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = TestWorld.GetTestWorld();
        AActor* Floor = World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Temporary floor actor"), Floor)) return false;
        UBoxComponent* FloorBox = NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(FloorBox);
        Floor->SetRootComponent(FloorBox);
        FloorBox->SetMobility(EComponentMobility::Static);
        FloorBox->SetBoxExtent(FVector(1000.0f, 1000.0f, 50.0f));
        FloorBox->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
        FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FloorBox->SetCollisionObjectType(ECC_WorldStatic);
        FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
        FloorBox->RegisterComponent();

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ACharacter* Character = World->SpawnActor<ACharacter>(
            FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator, SpawnParameters);
        if (!TestNotNull(TEXT("Temporary moving character"), Character)) return false;
        UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
        // This fixture has no player controller; real movement must still run.
        Movement->bRunPhysicsWithNoController = true;
        USkeletalMeshComponent* Mesh = Character->GetMesh();
        USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
        if (!TestNotNull(TEXT("Quinn mesh"), Quinn)) return false;
        Mesh->SetSkeletalMeshAsset(Quinn);
        Mesh->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

        UETPracticeActionComponent* Component = NewObject<UETPracticeActionComponent>(Character);
        Character->AddInstanceComponent(Component);
        Component->RegisterComponent();
        Component->DodgeDistance = 150.0f;
        Component->DodgeDuration = 0.30f;
        Component->DodgeMontage = LoadObject<UAnimMontage>(nullptr,
            TEXT("/Game/EmberTrial/Animations/Practice/Montages/AM_M0_Dodge_01_Montage.AM_M0_Dodge_01_Montage"));
        Component->HitReactMontage = LoadObject<UAnimMontage>(nullptr,
            TEXT("/Game/EmberTrial/Animations/Practice/Montages/AM_M0_HitResult_01_Montage.AM_M0_HitResult_01_Montage"));
        if (!TestNotNull(TEXT("Dodge montage"), Component->DodgeMontage.Get())
            || !TestNotNull(TEXT("Hit montage"), Component->HitReactMontage.Get())) return false;
        if (!TestWorld.BeginPlayInTestWorld())
        {
            TestWorld.ForwardErrorMessages(this);
            return false;
        }
        // Start above the floor; let real collision resolve landing.
        Movement->SetMovementMode(MOVE_Falling);
        const float Step = 1.0f / static_cast<float>(StepRate);
        auto Advance = [&TestWorld, Step, StepRate](float Seconds)
            {
                const int32 Count = FMath::RoundToInt(Seconds * StepRate);
                for (int32 Index = 0; Index < Count; ++Index)
                {
                    if (!TestWorld.TickTestWorld(Step)) return false;
                }
                return true;
            };
        if (!Advance(1.0f))
        {
            TestWorld.ForwardErrorMessages(this);
            return false;
        }
        if (!TestTrue(TEXT("Movement tick enabled"), Movement->IsComponentTickEnabled())
            || !TestTrue(TEXT("Character landed on real floor"), Movement->IsMovingOnGround())
            || !TestNotNull(TEXT("Animation instance"), Mesh->GetAnimInstance())) return false;

        // Full movement at each fixed step; observe the source without driving it.
        const FVector Start = Character->GetActorLocation();
        const FVector ExpectedEnd = Start + Character->GetActorRightVector() * 150.0f;
        if (!TestTrue(TEXT("Dodge starts"), Component->RequestPracticeAction(State::Dodging))) return false;
        TSharedPtr<FRootMotionSource> Source = Component->DodgeSource;
        if (!TestTrue(TEXT("Dodge owns a source"), Source.IsValid())) return false;
        if (!Advance(0.4f)) return false;
        const FVector StoppedAt = Character->GetActorLocation();
        const float Travel = FVector::Dist2D(Start, StoppedAt);
        TestTrue(TEXT("Dodge reaches right-side endpoint within 1 cm"),
            FVector::Dist2D(StoppedAt, ExpectedEnd) <= 1.0f);
        TestTrue(TEXT("Source simulates 0.30 seconds"),
            FMath::IsNearlyEqual(Source->GetTime(), 0.30f, 0.002f));
        TestFalse(TEXT("Movement source ownership released"), Component->DodgeSource.IsValid());
        TestFalse(TEXT("Movement source drained from movement component"), Movement->HasRootMotionSources());
        TestTrue(TEXT("State remains Dodging after movement ends"),
            Component->GetPracticeActionState() == State::Dodging);
        if (!Advance(0.6f)) return false;
        TestTrue(TEXT("Montage later returns Idle"), Component->IsPracticeActionIdle());
        TestTrue(TEXT("No horizontal drift after movement ends"),
            FVector::Dist2D(StoppedAt, Character->GetActorLocation()) <= 1.0f);
        TestTrue(TEXT("No residual horizontal velocity"), Movement->Velocity.Size2D() <= 1.0f);
        AddInfo(FString::Printf(TEXT("[DodgeMovement] StepRate=%d Travel2D=%.2f SourceTime=%.4f"),
            StepRate, Travel, Source->GetTime()));
        TestWorld.ForwardErrorMessages(this);
        if (HasAnyErrors()) return false;

        if (StepRate == 60)
        {
            // Interrupt after actual displacement has started.
            const FVector HitStart = Character->GetActorLocation();
            if (!TestTrue(TEXT("Dodge before hit"), Component->RequestPracticeAction(State::Dodging))) return false;
            if (!Advance(0.1f)) return false;
            const FVector HitAt = Character->GetActorLocation();
            const float BeforeHit = FVector::Dist2D(HitStart, HitAt);
            TestTrue(TEXT("Hit occurs during partial displacement"), BeforeHit > 1.0f && BeforeHit < 149.0f);
            if (!TestTrue(TEXT("Hit interrupts moving dodge"), Component->RequestPracticeAction(State::HitReact))) return false;
            TestFalse(TEXT("Hit immediately releases owned source"), Component->DodgeSource.IsValid());
            TestTrue(TEXT("State becomes HitReact"), Component->GetPracticeActionState() == State::HitReact);
            if (!Advance(1.0f)) return false;
            TestTrue(TEXT("Hit leaves no further horizontal displacement"),
                FVector::Dist2D(HitAt, Character->GetActorLocation()) <= 1.0f);
            TestFalse(TEXT("Hit leaves no root motion source"), Movement->HasRootMotionSources());
            TestTrue(TEXT("Hit eventually returns Idle"), Component->IsPracticeActionIdle());
            if (HasAnyErrors()) return false;

            // Pause halfway through movement, then resume the same source.
            const FVector PauseStart = Character->GetActorLocation();
            if (!TestTrue(TEXT("Dodge before pause"), Component->RequestPracticeAction(State::Dodging))) return false;
            if (!Advance(0.1f)) return false;
            TSharedPtr<FRootMotionSource> PausedSource = Component->DodgeSource;
            if (!TestTrue(TEXT("Source still active before pause"), PausedSource.IsValid())) return false;
            APlayerState* Pauser = World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Pause owner"), Pauser)) return false;
            UAnimInstance* Anim = Mesh->GetAnimInstance();
            const FVector PausedAt = Character->GetActorLocation();
            const float PausedSourceTime = PausedSource->GetTime();
            const float PausedMontageTime = Anim->Montage_GetPosition(Component->DodgeMontage);
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            const bool bPauseTicksSucceeded = Advance(0.5f);
            TestFalse(TEXT("Paused request is rejected"), Component->RequestPracticeAction(State::HitReact));
            TestTrue(TEXT("Pause freezes location"),
                FVector::Dist(PausedAt, Character->GetActorLocation()) <= 0.01f);
            TestTrue(TEXT("Pause freezes source time"),
                FMath::IsNearlyEqual(PausedSource->GetTime(), PausedSourceTime, 0.0001f));
            TestTrue(TEXT("Pause freezes montage time"),
                FMath::IsNearlyEqual(Anim->Montage_GetPosition(Component->DodgeMontage), PausedMontageTime, 0.0001f));
            // Always unpause before checking errors or leaving this test case.
            World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            TestWorld.ForwardErrorMessages(this);
            if (!bPauseTicksSucceeded || HasAnyErrors()) return false;
            if (!Advance(1.0f)) return false;
            TestTrue(TEXT("Resumed dodge covers 150 cm total"),
                FMath::Abs(FVector::Dist2D(PauseStart, Character->GetActorLocation()) - 150.0f) <= 1.0f);
            TestTrue(TEXT("Resumed source finishes at 0.30 seconds"),
                FMath::IsNearlyEqual(PausedSource->GetTime(), 0.30f, 0.002f));
            TestTrue(TEXT("Resume eventually returns Idle"), Component->IsPracticeActionIdle());
            TestFalse(TEXT("Resume releases owned source"), Component->DodgeSource.IsValid());
            TestFalse(TEXT("Resume leaves no root motion source"), Movement->HasRootMotionSources());
            TestTrue(TEXT("Resume leaves no horizontal velocity"), Movement->Velocity.Size2D() <= 1.0f);
        }
        TestWorld.ForwardErrorMessages(this);
        if (HasAnyErrors()) return false;
    }
    return true;
}

#endif