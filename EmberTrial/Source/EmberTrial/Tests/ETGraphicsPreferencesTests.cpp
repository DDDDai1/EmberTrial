#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETGraphicsSettingsTypes.h"
#include "Misc/AutomationTest.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETGraphicsPreferencesTest,
    "EmberTrial.M3.Settings.GraphicsPreferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETGraphicsPreferencesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const int32 Uniform[] = {2, 2, 2};
    const int32 Mixed[] = {2, 1, 2};
    const int32 Broken[] = {2, 1, 5};
    const int32 Cinematic[] = {4, 4};
    TestEqual(TEXT("Uniform native groups retain their tier"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Uniform)), 2);
    TestEqual(TEXT("Mixed groups are custom"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Mixed)), -1);
    TestEqual(TEXT("Invalid group is rejected even after mixed values"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Broken)), -2);
    TestEqual(TEXT("Empty groups are unavailable"),
        FETGraphicsSettingsRules::FindUniformQuality(TConstArrayView<int32>()), -2);
    TestEqual(TEXT("Stored cinematic tier is readable"),
        FETGraphicsSettingsRules::FindUniformQuality(MakeArrayView(Cinematic)), 4);

    FETGraphicsPreferences Current;
    Current.QualityLevel = 2;
    Current.FrameRateLimit = 60.0f;
    FETGraphicsPreferences Request = Current;
    Request.QualityLevel = 0;
    Request.FrameRateLimit = 30.0f;
    Request.bUseVSync = true;
    FETGraphicsPreferences Out;
    TestTrue(TEXT("Legal request resolves"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestEqual(TEXT("Requested quality retained"), Out.QualityLevel, 0);
    TestEqual(TEXT("Requested cap retained"), Out.FrameRateLimit, 30.0f);
    TestTrue(TEXT("Requested VSync retained"), Out.bUseVSync);
    TestEqual(TEXT("Current snapshot unchanged"), Current.QualityLevel, 2);

    Request.QualityLevel = -1;
    Request.FrameRateLimit = 0.0f;
    TestTrue(TEXT("Custom request keeps existing tier and allows unlimited"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestEqual(TEXT("Current tier preserved"), Out.QualityLevel, 2);
    TestEqual(TEXT("Zero means no settings cap"), Out.FrameRateLimit, 0.0f);
    Current.QualityLevel = -1;
    TestTrue(TEXT("Mixed native state remains mixed"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestEqual(TEXT("Mixed groups will not be overwritten"), Out.QualityLevel, -1);
    TestTrue(TEXT("Output may alias request"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Request));

    Request.FrameRateLimit = -1.0f;
    TestFalse(TEXT("Negative cap rejected"),
        FETGraphicsSettingsRules::TryResolve(Request, Current, Out));
    TestFalse(TEXT("Rejected output invalidated"), Out.IsValid());
    Request.FrameRateLimit = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN cap rejected"), Request.IsValid());
    Request.FrameRateLimit = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("Infinite cap rejected"), Request.IsValid());
    Request.FrameRateLimit = 59.94f;
    TestTrue(TEXT("Finite custom cap can be preserved"), Request.IsValid());
    Request.QualityLevel = 5;
    TestFalse(TEXT("Unknown quality rejected"), Request.IsValid());
    Request.QualityLevel = 1;
    TestFalse(TEXT("Unavailable current state rejected"),
        FETGraphicsSettingsRules::TryResolve(Request, FETGraphicsPreferences{}, Out));
    return true;
}
#endif