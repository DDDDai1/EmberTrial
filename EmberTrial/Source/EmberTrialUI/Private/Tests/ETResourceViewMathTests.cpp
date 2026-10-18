#if WITH_DEV_AUTOMATION_TESTS

#include "Presenters/ETResourceViewMath.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETResourceViewRangeTest, "EmberTrial.UI.ResourceView.Range",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETResourceViewRangeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    const float Largest = std::numeric_limits<float>::max();
    struct FRangeCase
    {
        const TCHAR* Label;
        float Current;
        float Maximum;
        bool bExpected;
    };
    // 明确列出展示契约，避免把资源为0误当不可用，或让NaN进入进度条比例。
    const FRangeCase Cases[] = {{TEXT("Full resource"), 100.0f, 100.0f, true},
                                {TEXT("Zero resource remains valid"), 0.0f, 100.0f, true},
                                {TEXT("Partial resource"), 75.0f, 100.0f, true},
                                {TEXT("Fractional positive maximum"), 0.25f, 0.5f, true},
                                {TEXT("Signed zero current"), -0.0f, 100.0f, true},
                                {TEXT("Large finite range"), Largest, Largest, true},
                                {TEXT("Negative resource"), -1.0f, 100.0f, false},
                                {TEXT("Above maximum"), 101.0f, 100.0f, false},
                                {TEXT("Zero maximum"), 0.0f, 0.0f, false},
                                {TEXT("Negative maximum"), 0.0f, -1.0f, false},
                                {TEXT("Positive current with zero maximum"), 1.0f, 0.0f, false},
                                {TEXT("NaN current"), NaN, 100.0f, false},
                                {TEXT("NaN maximum"), 1.0f, NaN, false},
                                {TEXT("Infinite current"), Infinity, 100.0f, false},
                                {TEXT("Infinite maximum"), 1.0f, Infinity, false},
                                {TEXT("Negative infinite current"), -Infinity, 100.0f, false},
                                {TEXT("Negative infinite maximum"), 1.0f, -Infinity, false}};
    for (const FRangeCase& Case : Cases)
    {
        TestEqual(FString(Case.Label), ETResourceViewMath::IsValidRange(Case.Current, Case.Maximum),
                  Case.bExpected);
    }
    AddInfo(FString::Printf(
        TEXT("Resource view ranges: %d cases, zero, fractions, finite limits, NaN and infinity"),
        static_cast<int32>(UE_ARRAY_COUNT(Cases))));
    return !HasAnyErrors();
}
#endif
