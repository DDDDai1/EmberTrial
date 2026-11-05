#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETGraphicsSettingsDraft.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETGraphicsSettingsDraftTest,
    "EmberTrial.UI.Settings.GraphicsDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETGraphicsSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETGraphicsSettingsDraft Draft;
    FETGraphicsPreferences Current = FETGraphicsSettingsDraft::Defaults();
    FETGraphicsPreferences Out;
    TestFalse(TEXT("Closed draft rejects edits"), Draft.SetQuality(0));
    TestFalse(TEXT("Unavailable snapshot rejected"), Draft.Open(FETGraphicsPreferences{}));
    TestTrue(TEXT("Open valid defaults"), Draft.Open(Current));
    TestFalse(TEXT("New draft is clean"), Draft.IsDirty());
    TestTrue(TEXT("Default presentation"), Draft.IsDefault());
    TestFalse(TEXT("Unchanged draft cannot save"), Draft.TryMakeApply(Current, Out));
    TestTrue(TEXT("Select low quality"), Draft.SetQuality(0));
    TestTrue(TEXT("Select 30 cap"), Draft.SetFrameRateLimit(30.0f));
    TestTrue(TEXT("Toggle VSync in draft"), Draft.SetVSync(true));
    TestEqual(TEXT("Browsing has not changed current"), Current.QualityLevel, 1);
    TestTrue(TEXT("Dirty draft produces request"), Draft.TryMakeApply(Current, Out));
    TestEqual(TEXT("Candidate low"), Out.QualityLevel, 0);
    TestEqual(TEXT("Candidate cap"), Out.FrameRateLimit, 30.0f);
    TestTrue(TEXT("Candidate VSync"), Out.bUseVSync);
    FETGraphicsPreferences Changed = Current;
    Changed.FrameRateLimit = 120.0f;
    TestFalse(TEXT("External change rejects old page"), Draft.TryMakeApply(Changed, Out));
    TestFalse(TEXT("Rejected output invalid"), Out.IsValid());
    TestFalse(TEXT("Unsupported quality rejected"), Draft.SetQuality(5));
    TestFalse(TEXT("Arbitrary cap not added by browsing"), Draft.SetFrameRateLimit(45.0f));
    TestTrue(TEXT("Rejected edits preserve existing draft"), Draft.IsDirty());
    Draft.UseDefaults();
    TestFalse(TEXT("Defaults match original baseline"), Draft.IsDirty());

    Current.QualityLevel = -1;
    Current.FrameRateLimit = 59.94f;
    TestTrue(TEXT("Mixed quality and custom cap readable"), Draft.Open(Current));
    TArray<float> Limits;
    Draft.GetFrameRateLimits(Limits);
    TestTrue(TEXT("Preserve existing custom cap"), Limits.Contains(59.94f));
    TestTrue(TEXT("Can propose preset"), Draft.SetQuality(3));
    TestTrue(TEXT("Can return to original mixed groups"), Draft.SetQuality(-1));
    TestTrue(TEXT("Can cancel cap"), Draft.SetFrameRateLimit(0.0f));
    TestTrue(TEXT("Mixed groups survive cap-only request"), Draft.TryMakeApply(Current, Out));
    TestEqual(TEXT("Do not overwrite mixed groups"), Out.QualityLevel, -1);
    TestTrue(TEXT("Open permits alias of draft value"), Draft.Open(Draft.GetValue()));
    TestFalse(TEXT("Alias reopen is clean"), Draft.IsDirty());
    Draft.Close();
    TestFalse(TEXT("Closed view cannot commit"), Draft.TryMakeApply(Current, Out));
    return true;
}
#endif