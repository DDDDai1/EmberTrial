#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETAudioSettingsDraft.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAudioSettingsDraftTest,
    "EmberTrial.UI.Settings.AudioDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAudioSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETAudioSettingsDraft Draft;
    FETAudioPreferences Applied;
    Applied.MasterVolume = 0.6f;
    Applied.MusicVolume = 0.3f;
    Applied.EffectsVolume = 0.8f;
    FETAudioPreferences Request = Applied;
    Request.MasterVolume = 0.0f;
    FETAudioPreferences Out;
    TestFalse(TEXT("Closed view rejects edits"), Draft.SetValues(Request));
    TestTrue(TEXT("Open saved values"), Draft.Open(Applied));
    TestFalse(TEXT("Snapshot is clean"), Draft.IsDirty());
    TestFalse(TEXT("Unchanged view cannot apply"), Draft.TryMakeApply(Applied, Out));
    TestTrue(TEXT("Mute only changes draft"), Draft.SetValues(Request));
    TestEqual(TEXT("Browsing keeps applied master"), Applied.MasterVolume, 0.6f);
    TestTrue(TEXT("Edited view proposes valid request"), Draft.TryMakeApply(Applied, Out));
    TestEqual(TEXT("Candidate mute"), Out.MasterVolume, 0.0f);
    TestEqual(TEXT("Candidate retains child volume"), Out.MusicVolume, 0.3f);
    FETAudioPreferences Changed = Applied;
    Changed.EffectsVolume = 0.5f;
    TestFalse(TEXT("External change rejects stale draft"), Draft.TryMakeApply(Changed, Out));
    TestTrue(TEXT("Rejected draft stays edited"), Draft.IsDirty());
    Request.MusicVolume = 1.1f;
    TestFalse(TEXT("Invalid edit rejected"), Draft.SetValues(Request));
    TestEqual(TEXT("Invalid edit preserves prior draft"), Draft.GetValue().MusicVolume, 0.3f);
    Draft.UseDefaults();
    TestTrue(TEXT("Default affects draft only"), Draft.IsDefault());
    TestEqual(TEXT("Defaults do not overwrite current"), Applied.MasterVolume, 0.6f);
    TestTrue(TEXT("Defaults require explicit apply"), Draft.IsDirty());
    TestTrue(TEXT("Reopen supports own value alias"), Draft.Open(Draft.GetValue()));
    TestFalse(TEXT("Reopened snapshot is clean"), Draft.IsDirty());
    FETAudioPreferences Invalid;
    Invalid.EffectsVolume = -1.0f;
    TestFalse(TEXT("Invalid open closes old session"), Draft.Open(Invalid));
    TestFalse(TEXT("Failed open cannot commit"), Draft.TryMakeApply(Applied, Out));
    Draft.Close();
    TestFalse(TEXT("Closed old view cannot commit"), Draft.TryMakeApply(Applied, Out));
    return true;
}
#endif