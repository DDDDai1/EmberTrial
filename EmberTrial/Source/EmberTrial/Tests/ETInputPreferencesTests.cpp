#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/ETGameUserSettings.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETInputPreferencesTest,
    "EmberTrial.M3.Settings.InputPreferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETInputPreferencesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    TestNotNull(TEXT("Engine selects the configured project settings class"),
        UETGameUserSettings::GetETGameUserSettings());

    // 独立对象只改内存，不调用SaveSettings / LoadSettings或全局ApplySettings。
    UETGameUserSettings* Settings = NewObject<UETGameUserSettings>();
    if (!TestNotNull(TEXT("Isolated settings object"), Settings))
    {
        return false;
    }
    Settings->SetToDefaults();
    const FETInputPreferences Defaults = Settings->GetInputPreferences();
    TestTrue(TEXT("Input defaults are legal"), Defaults.IsValid());
    TestEqual(TEXT("Default scale preserves template feel"), Defaults.MouseSensitivity, 1.0f);
    TestFalse(TEXT("Default pitch is not inverted"), Defaults.bInvertMouseY);

    // 草稿是副本；构造 / 修改副本不会偷偷提交，更不需要模拟Widget来验证。
    FETInputPreferences Draft = Defaults;
    Draft.MouseSensitivity = 0.5f;
    Draft.bInvertMouseY = true;
    TestEqual(TEXT("Editing a copy does not apply it"),
        Settings->GetInputPreferences().MouseSensitivity, 1.0f);
    TestTrue(TEXT("Valid input commit accepted"), Settings->ApplyInputPreferences(Draft));
    TestTrue(TEXT("Applied multiplier and pitch inversion both take effect"),
        Settings->GetInputPreferences().TransformLook(FVector2D(4.0, 2.0)).Equals(
            FVector2D(2.0, -1.0)));
    TestTrue(TEXT("Repeated confirmation is accepted"), Settings->ApplyInputPreferences(Draft));

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    for (const float Bad : {0.0f, -1.0f, 0.24f, 3.01f, NaN, Infinity})
    {
        FETInputPreferences Invalid = Draft;
        Invalid.MouseSensitivity = Bad;
        TestFalse(TEXT("Illegal multiplier is rejected"), Settings->ApplyInputPreferences(Invalid));
        TestEqual(TEXT("Rejection preserves applied value"),
            Settings->GetInputPreferences().MouseSensitivity, 0.5f);
        TestTrue(TEXT("Rejection preserves applied inversion"),
            Settings->GetInputPreferences().bInvertMouseY);
    }
    for (const float Limit : {FETInputPreferences::MinSensitivity,
        FETInputPreferences::MaxSensitivity})
    {
        FETInputPreferences Boundary;
        Boundary.MouseSensitivity = Limit;
        TestTrue(TEXT("Inclusive boundary accepted"), Settings->ApplyInputPreferences(Boundary));
    }

    TestTrue(TEXT("Default transform preserves both signs"),
        Defaults.TransformLook(FVector2D(-3.0, 2.0)).Equals(FVector2D(-3.0, 2.0)));
    TestTrue(TEXT("Zero input stays zero"), Draft.TransformLook(FVector2D::ZeroVector).IsZero());
    TestTrue(TEXT("Nonfinite source input is suppressed"),
        Draft.TransformLook(FVector2D(NaN, 1.0)).IsZero());
    TestTrue(TEXT("Infinite source input is suppressed"),
        Draft.TransformLook(FVector2D(1.0, Infinity)).IsZero());
    FETInputPreferences Corrupt;
    Corrupt.MouseSensitivity = NaN;
    Corrupt.bInvertMouseY = true;
    TestTrue(TEXT("Corrupt preferences fall back to complete input defaults"),
        Corrupt.TransformLook(FVector2D(2.0, -3.0)).Equals(FVector2D(2.0, -3.0)));
    TestTrue(TEXT("Finite input that overflows after scaling is suppressed"),
        Settings->GetInputPreferences().TransformLook(
            FVector2D(std::numeric_limits<double>::max(), 1.0)).IsZero());

    TestTrue(TEXT("Input-only defaults can be committed"),
        Settings->ApplyInputPreferences(FETInputPreferences{}));
    TestTrue(TEXT("Input-only reset restores template look"),
        Settings->GetInputPreferences().TransformLook(FVector2D(4.0, -2.0)).Equals(
            FVector2D(4.0, -2.0)));
    return true;
}
#endif