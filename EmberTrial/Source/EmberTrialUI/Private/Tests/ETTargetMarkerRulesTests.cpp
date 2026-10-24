#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetMarkerRules.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETTargetMarkerRulesTest, "EmberTrial.UI.TargetLock.MarkerRules",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETTargetMarkerRulesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using ETTargetMarkerRules::ShouldShow;
    const FVector2D Size(1280, 720);
    const FVector2D Center(640, 360);
    TestTrue(TEXT("Valid local point"), ShouldShow(true, Center, Size, 24.0f));
    TestFalse(TEXT("Failed / behind projection"), ShouldShow(false, Center, Size, 24.0f));
    TestTrue(TEXT("Inset boundary is inclusive"), ShouldShow(true, FVector2D(24, 24), Size, 24.0f));
    for (const FVector2D Point : {FVector2D(23, 360), FVector2D(1257, 360), FVector2D(640, 23),
                                  FVector2D(640, 697), FVector2D(-1, -1)})
    {
        TestFalse(TEXT("Outside safe viewport"), ShouldShow(true, Point, Size, 24.0f));
    }
    TestFalse(TEXT("No geometry yet"), ShouldShow(true, Center, FVector2D::ZeroVector, 24.0f));
    TestFalse(TEXT("Viewport smaller than inset"),
              ShouldShow(true, FVector2D(20, 20), FVector2D(40, 40), 24.0f));
    TestFalse(TEXT("Negative inset"), ShouldShow(true, Center, Size, -1.0f));
    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("NaN position"), ShouldShow(true, FVector2D(NaN, 100), Size, 24.0f));
    TestFalse(TEXT("Infinite size"), ShouldShow(true, Center, FVector2D(Infinity, 720), 24.0f));
    TestFalse(TEXT("NaN inset"), ShouldShow(true, Center, Size, NaN));
    TestTrue(TEXT("Another UMG canvas size"),
             ShouldShow(true, FVector2D(960, 540), FVector2D(1920, 1080), 24.0f));
    // DPI转换 / 相机 / 真实Viewport由引擎和实机检查负责，不把纯规则当成投影集成通过。
    return !HasAnyErrors();
}
#endif