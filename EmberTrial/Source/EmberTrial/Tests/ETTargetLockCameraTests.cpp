#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetLockCameraComponent.h"
#include "Misc/AutomationTest.h"
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
    const FVector Origin = FVector::ZeroVector;
    const FVector Right(0.0, 600.0, 0.0);
    FRotator Delta = FRotator::ZeroRotator;

    // 无World / BeginPlay的组件不能接管输入，失败输出必须清零。
    Delta = FRotator(1.0, 2.0, 3.0);
    TestFalse(TEXT("Unbound context cannot own the camera"),
              Camera->TryMakeRotationDelta(nullptr, .016f, Delta));
    TestTrue(TEXT("Rejected context clears output"), Delta.IsZero());
    TestTrue(TEXT("Valid right target step"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    TestTrue(TEXT("Yaw turns toward target without snapping"), Delta.Yaw > 0.0 && Delta.Yaw < 90.0);
    TestTrue(TEXT("Level target applies negative framing pitch"),
             Delta.Pitch < 0.0 && Delta.Pitch > -12.0);
    TestTrue(TEXT("Camera does not add roll"), Delta.Roll == 0.0);

    const FVector Across = FRotator(0.0, -179.0, 0.0).Vector() * 600.0;
    TestTrue(TEXT("Angle wrap step is valid"),
             Camera->CalculateRotationDelta(FRotator(0.0, 179.0, 0.0), Origin, Across, .1f, Delta));
    TestTrue(TEXT("179 to -179 takes positive shortest arc"), Delta.Yaw > 0.0 && Delta.Yaw < 2.0);

    // 同一静止目标、同一游戏秒；不是用纯规则冒充真实Viewport或运动目标手感。
    auto Simulate = [&](int32 Frames)
    {
        FRotator Current = FRotator::ZeroRotator;
        for (int32 I = 0; I < Frames; ++I)
        {
            FRotator Step = FRotator::ZeroRotator;
            const bool bValid =
                Camera->CalculateRotationDelta(Current, Origin, Right, 1.0f / Frames, Step);
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
    TestTrue(TEXT("One second approaches target"), FMath::Abs(At60.Yaw - 90.0) < .04);

    TestTrue(TEXT("High target step"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(600, 0, 10000),
                                            1.0f, Delta));
    TestTrue(TEXT("Desired high pitch is clamped"), Delta.Pitch > 14.9 && Delta.Pitch <= 15.0);
    TestTrue(TEXT("Low target step"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(600, 0, -10000),
                                            1.0f, Delta));
    TestTrue(TEXT("Desired low pitch is clamped"), Delta.Pitch < -34.9 && Delta.Pitch >= -35.0);
    TestTrue(TEXT("Capture from free high pitch"),
             Camera->CalculateRotationDelta(FRotator(70, 0, 0), Origin, FVector(600, 0, 0), .016f,
                                            Delta));
    TestTrue(TEXT("Capture approaches bounds smoothly"),
             70.0 + Delta.Pitch > 15.0 && Delta.Pitch < 0.0);

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    for (float BadTime : {0.0f, -1.0f, NaN, Infinity})
    {
        Delta = FRotator(1, 2, 3);
        TestFalse(
            TEXT("Invalid game delta is rejected"),
            Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, BadTime, Delta));
        TestTrue(TEXT("Failed step clears output"), Delta.IsZero());
    }
    TestFalse(TEXT("Coincident targets have no yaw"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Origin, .1f, Delta));
    TestFalse(TEXT("Directly vertical targets have no yaw"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(0, 0, 100), .1f,
                                             Delta));
    TestFalse(TEXT("Nonfinite current is rejected"),
              Camera->CalculateRotationDelta(FRotator(NaN, 0, 0), Origin, Right, .1f, Delta));
    TestFalse(TEXT("Nonfinite source is rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, FVector(Infinity, 0, 0), Right,
                                             .1f, Delta));
    TestFalse(TEXT("Nonfinite target is rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(0, NaN, 0), .1f,
                                             Delta));
    Camera->FollowSpeed = 0.0f;
    TestFalse(TEXT("Invalid speed rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->FollowSpeed = 8.0f;
    Camera->MinPitch = Camera->MaxPitch;
    TestFalse(TEXT("Invalid pitch range rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->MinPitch = -35.0f;
    Camera->PitchOffset = NaN;
    TestFalse(TEXT("Nonfinite framing offset rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    return true;
}
#endif