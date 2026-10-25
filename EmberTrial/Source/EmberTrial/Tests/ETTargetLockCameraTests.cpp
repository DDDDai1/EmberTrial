#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetLockCameraComponent.h"
#include "Misc/AutomationTest.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Framework/ETPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETTargetLockCameraRulesTest,
                                 "EmberTrial.M2.TargetLock.CameraRules",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETTargetLockCameraRulesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UETTargetLockCameraComponent* Camera = NewObject<UETTargetLockCameraComponent>();
    if (!TestNotNull(TEXT("Camera rules object"), Camera))
    {
        return false;
    }
    // 保留原有零肩位角度基线，再单独验证新的右肩几何 / 生命周期。
    Camera->PitchOffset = -25.0f;
    Camera->YawOffset = 25.0f;
    auto Calculate = [Camera](const FRotator& Current, const FVector& Source, const FVector& Target,
                              float Seconds, FRotator& Out)
    {
        return Camera->CalculateRotationDelta(Current, Source, Target, FVector::ZeroVector, Seconds,
                                              Out);
    };
    const FVector Origin = FVector::ZeroVector;
    const FVector Right(0.0, 600.0, 0.0);
    FRotator Delta = FRotator::ZeroRotator;

    // 无World / BeginPlay的组件不能接管输入，失败输出必须清零。
    Delta = FRotator(1.0, 2.0, 3.0);
    TestFalse(TEXT("Unbound context cannot own the camera"),
              Camera->TryMakeRotationDelta(nullptr, .016f, Delta));
    TestTrue(TEXT("Rejected context clears output"), Delta.IsZero());
    TestTrue(TEXT("Valid right target step"),
             Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    TestTrue(TEXT("Yaw approaches the framed angle without snapping"),
             Delta.Yaw > 0.0 && Delta.Yaw < 115.0);
    TestTrue(TEXT("Level target applies negative framing pitch"),
             Delta.Pitch < 0.0 && Delta.Pitch > -25.0);
    TestTrue(TEXT("Camera does not add roll"), Delta.Roll == 0.0);

    const FVector Across = FRotator(0.0, -179.0, 0.0).Vector() * 600.0;
    TestTrue(TEXT("Angle wrap step is valid"),
             Calculate(FRotator(0.0, 179.0, 0.0), Origin, Across, .1f, Delta));
    TestTrue(TEXT("Wrapped yaw plus framing takes the positive short arc"),
             Delta.Yaw > 0.0 && Delta.Yaw < 27.0);

    // 同一静止目标、同一游戏秒；不是用纯规则冒充真实Viewport或运动目标手感。
    auto Simulate = [&](int32 Frames)
    {
        FRotator Current = FRotator::ZeroRotator;
        for (int32 I = 0; I < Frames; ++I)
        {
            FRotator Step = FRotator::ZeroRotator;
            const bool bValid = Calculate(Current, Origin, Right, 1.0f / Frames, Step);
            TestTrue(TEXT("Every simulation step is valid"), bValid);
            Current = (Current + Step).GetNormalized();
        }
        return Current;
    };
    const FRotator At30 = Simulate(30);
    const FRotator At60 = Simulate(60);
    const FRotator At120 = Simulate(120);
    TestTrue(TEXT("30 and 60 subdivisions converge equally"), At30.Equals(At60, .001));
    TestTrue(TEXT("60 and 120 subdivisions converge equally"), At60.Equals(At120, .001));
    TestTrue(TEXT("One second approaches target"), FMath::Abs(At60.Yaw - 115.0) < .05);

    TestTrue(TEXT("High target step"),
             Calculate(FRotator::ZeroRotator, Origin, FVector(600, 0, 10000), 1.0f, Delta));
    TestTrue(TEXT("Desired high pitch is clamped"), Delta.Pitch > 14.9 && Delta.Pitch <= 15.0);
    TestTrue(TEXT("Low target step"),
             Calculate(FRotator::ZeroRotator, Origin, FVector(600, 0, -10000), 1.0f, Delta));
    TestTrue(TEXT("Desired low pitch is clamped"), Delta.Pitch < -44.9 && Delta.Pitch >= -45.0);
    TestTrue(TEXT("Capture from free high pitch"),
             Calculate(FRotator(70, 0, 0), Origin, FVector(600, 0, 0), .016f, Delta));
    TestTrue(TEXT("Capture approaches bounds smoothly"),
             70.0 + Delta.Pitch > 15.0 && Delta.Pitch < 0.0);

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    for (float BadTime : {0.0f, -1.0f, NaN, Infinity})
    {
        Delta = FRotator(1, 2, 3);
        TestFalse(TEXT("Invalid game delta is rejected"),
                  Calculate(FRotator::ZeroRotator, Origin, Right, BadTime, Delta));
        TestTrue(TEXT("Failed step clears output"), Delta.IsZero());
    }
    TestFalse(TEXT("Coincident targets have no yaw"),
              Calculate(FRotator::ZeroRotator, Origin, Origin, .1f, Delta));
    TestFalse(TEXT("Directly vertical targets have no yaw"),
              Calculate(FRotator::ZeroRotator, Origin, FVector(0, 0, 100), .1f, Delta));
    TestFalse(TEXT("Nonfinite current is rejected"),
              Calculate(FRotator(NaN, 0, 0), Origin, Right, .1f, Delta));
    TestFalse(TEXT("Nonfinite source is rejected"),
              Calculate(FRotator::ZeroRotator, FVector(Infinity, 0, 0), Right, .1f, Delta));
    TestFalse(TEXT("Nonfinite target is rejected"),
              Calculate(FRotator::ZeroRotator, Origin, FVector(0, NaN, 0), .1f, Delta));
    Camera->FollowSpeed = 0.0f;
    TestFalse(TEXT("Invalid speed rejected"),
              Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->FollowSpeed = 8.0f;
    Camera->MinPitch = Camera->MaxPitch;
    TestFalse(TEXT("Invalid pitch range rejected"),
              Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->MinPitch = -45.0f;
    Camera->PitchOffset = NaN;
    TestFalse(TEXT("Nonfinite framing offset rejected"),
              Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->PitchOffset = -25.0f;

    // 左右构图均走同一最短角路径；0恢复原同轴算法，不创建额外模式。
    Camera->YawOffset = -25.0f;
    TestTrue(TEXT("Negative framing offset is supported"),
             Calculate(FRotator::ZeroRotator, Origin, Right, 1.0f, Delta));
    TestTrue(TEXT("Negative framing approaches 65 degrees"), Delta.Yaw > 64.9 && Delta.Yaw < 65.0);
    Camera->YawOffset = 0.0f;
    TestTrue(TEXT("Zero offset remains supported"),
             Calculate(FRotator::ZeroRotator, Origin, Right, 1.0f, Delta));
    TestTrue(TEXT("Zero offset approaches original 90 degrees"),
             Delta.Yaw > 89.9 && Delta.Yaw < 90.0);
    for (float BadYaw : {NaN, Infinity, 36.0f, -36.0f})
    {
        Camera->YawOffset = BadYaw;
        Delta = FRotator(1, 2, 3);
        TestFalse(TEXT("Invalid framing offset is rejected"),
                  Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
        TestTrue(TEXT("Invalid framing clears output"), Delta.IsZero());
    }
    Camera->YawOffset = 25.0f;
    // ---- 远景精确注视 / 近景交战轴限幅；近景允许目标略离中心 ----
    Camera->PitchOffset = 0.0f;
    Camera->YawOffset = 0.0f;
    const FVector Focus(600, 0, 45);
    TestTrue(TEXT("Right shoulder geometry solved"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Focus,
                                            Camera->ShoulderOffset, 10.0f, Delta));
    const FRotator View = Delta.GetNormalized();
    const FVector CameraPosition =
        -View.Vector() * 400.0 + FRotationMatrix(View).TransformVector(Camera->ShoulderOffset);
    const FVector FocusInView = View.UnrotateVector(Focus - CameraPosition);
    const FVector PlayerInView = View.UnrotateVector(Origin - CameraPosition);
    TestTrue(TEXT("Right-side camera puts player left of center"), PlayerInView.Y < -70.0);
    TestTrue(TEXT("Focus remains forward and centered horizontally"),
             FocusInView.X > 0.0 && FMath::Abs(FocusInView.Y) < .001);
    TestTrue(TEXT("Height compensation centers the focus"), FMath::Abs(FocusInView.Z) < .001);
    TestTrue(TEXT("Very close target remains finite"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(10, 0, 45),
                                            Camera->ShoulderOffset, .1f, Delta) &&
                 !Delta.ContainsNaN());
    TestFalse(TEXT("Nonfinite socket offset is rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Focus,
                                             FVector(0, NaN, 0), .1f, Delta));

    // 贴脸时保持背后小偏角与俯视；旧算法在70cm可补偿约72度。
    Camera->PitchOffset = -18.0f;
    TestEqual(TEXT("Native downward framing default"),
              GetDefault<UETTargetLockCameraComponent>()->PitchOffset, -18.0f);
    TestEqual(TEXT("Native shoulder yaw cap default"),
              GetDefault<UETTargetLockCameraComponent>()->MaxShoulderYaw, 12.0f);
    for (double Distance : {70.0, 84.0, 100.0, 200.0, 600.0})
    {
        TestTrue(TEXT("Melee framing solves"),
                 Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin,
                                                FVector(Distance, 0, 45), Camera->ShoulderOffset,
                                                10.0f, Delta));
        TestTrue(TEXT("Shoulder compensation stays within combat axis cap"),
                 FMath::Abs(Delta.Yaw) <= 12.001);
        TestTrue(TEXT("Level melee uses moderate downward pitch"),
                 Delta.Pitch >= -35.0 && Delta.Pitch <= -18.0);
        const FVector Position = -Delta.Vector() * 400.0 +
                                 FRotationMatrix(Delta).TransformVector(Camera->ShoulderOffset);
        const FVector InView = Delta.UnrotateVector(FVector(Distance, 0, 45) - Position);
        TestTrue(TEXT("Melee focus remains forward inside a conservative frame"),
                 InView.X > 0.0 && FMath::Abs(InView.Y / InView.X) < .35 &&
                     FMath::Abs(InView.Z / InView.X) < .55);
    }
    for (float InvalidCap : {NaN, Infinity, -1.0f, 31.0f})
    {
        Camera->MaxShoulderYaw = InvalidCap;
        TestFalse(TEXT("Invalid shoulder yaw cap rejected"),
                  Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
        TestTrue(TEXT("Invalid shoulder cap clears output"), Delta.IsZero());
    }
    Camera->MaxShoulderYaw = 12.0f;

    // ---- 真实Controller / Pawn换控与组件结束；不创建Viewport或冒充视觉验收 ----
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AETPlayerController* PC = World->SpawnActor<AETPlayerController>();
    AEmberTrialCharacter* First =
        World->SpawnActor<AEmberTrialCharacter>(FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    AEmberTrialCharacter* Second =
        World->SpawnActor<AEmberTrialCharacter>(FVector(600, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Controller fixture"), PC) || !TestNotNull(TEXT("First body"), First) ||
        !TestNotNull(TEXT("Second body"), Second))
    {
        return false;
    }
    PC->Possess(First);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UETTargetLockCameraComponent* Rig = PC->FindComponentByClass<UETTargetLockCameraComponent>();
    if (!TestNotNull(TEXT("Single native camera component"), Rig))
    {
        return false;
    }
    USpringArmComponent* FirstBoom = First->GetCameraBoom();
    USpringArmComponent* SecondBoom = Second->GetCameraBoom();
    const FVector FirstBase(11, 12, 13);
    const FVector SecondBase(1, -2, 3);
    FirstBoom->SocketOffset = FirstBase;
    SecondBoom->SocketOffset = SecondBase;
    Rig->UpdateShoulderOffset(FirstBoom, .1f);
    TestTrue(TEXT("Entry smoothly moves toward the right shoulder"),
             FirstBoom->SocketOffset.Y > FirstBase.Y &&
                 FirstBoom->SocketOffset.Y < FirstBase.Y + Rig->ShoulderOffset.Y);
    Rig->UpdateShoulderOffset(FirstBoom, 10.0f);
    Rig->UpdateShoulderOffset(FirstBoom, 10.0f);
    TestTrue(TEXT("Repeated updates never accumulate offset"),
             FirstBoom->SocketOffset.Equals(FirstBase + Rig->ShoulderOffset, .001));
    Rig->ReturnShoulderOffset(.05f);
    TestTrue(TEXT("Unlock return is smooth"), FirstBoom->SocketOffset.Y > FirstBase.Y);
    for (int32 I = 0; I < 30; ++I)
    {
        Rig->ReturnShoulderOffset(.05f);
    }
    TestTrue(TEXT("Unlock restores exact saved base"), FirstBoom->SocketOffset == FirstBase);
    TestFalse(TEXT("Completed return releases old boom"), Rig->ShoulderBoom.IsValid());

    Rig->UpdateShoulderOffset(FirstBoom, .1f);
    Rig->Deactivate();
    TestTrue(TEXT("Deactivate restores exact base immediately"),
             FirstBoom->SocketOffset == FirstBase);
    Rig->Activate(false);
    Rig->UpdateShoulderOffset(FirstBoom, .1f);
    PC->Possess(Second);
    TestTrue(TEXT("Real possession restores old body's boom"),
             FirstBoom->SocketOffset == FirstBase);
    TestFalse(TEXT("New body cannot inherit old shoulder ownership"), Rig->ShoulderBoom.IsValid());

    Rig->UpdateShoulderOffset(SecondBoom, .1f);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pause fixture"), Pauser))
    {
        return false;
    }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    const FVector PausedOffset = SecondBoom->SocketOffset;
    TestFalse(TEXT("Paused camera does not own rotation"),
              Rig->TryMakeRotationDelta(PC->GetTargetLockComponent(), .5f, Delta));
    TestTrue(TEXT("Pause freezes shoulder interpolation"),
             SecondBoom->SocketOffset == PausedOffset);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    Rig->Deactivate();
    Rig->Activate(false);
    Rig->UpdateShoulderOffset(SecondBoom, .1f);
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay releases boom reference"), Rig->ShoulderBoom.IsValid());
    TestTrue(TEXT("EndPlay restores second body's exact base"),
             SecondBoom->SocketOffset == SecondBase);
    return !HasAnyErrors();
}
#endif
