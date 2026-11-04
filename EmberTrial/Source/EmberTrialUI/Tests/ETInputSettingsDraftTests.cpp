#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETInputSettingsDraft.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETInputSettingsDraftTest,
    "EmberTrial.UI.Settings.InputDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETInputSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETInputSettingsDraft Draft;
    FETInputPreferences Candidate;
    const FETInputPreferences Defaults;
    TestFalse(TEXT("Closed draft cannot edit"), Draft.SetSensitivity(0.5f));
    TestFalse(TEXT("Closed draft cannot submit"), Draft.TryMakeApply(Defaults, Candidate));

    TestTrue(TEXT("Open from applied snapshot"), Draft.Open(Defaults));
    TestFalse(TEXT("New snapshot has no pending change"), Draft.IsDirty());
    TestTrue(TEXT("Initial defaults recognised"), Draft.IsDefault());
    TestTrue(TEXT("Edit sensitivity"), Draft.SetSensitivity(0.5f));
    TestTrue(TEXT("Edit inversion"), Draft.SetInvertY(true));
    TestTrue(TEXT("Editing creates pending changes"), Draft.IsDirty());
    TestEqual(TEXT("Source value was not changed by editing"), Defaults.MouseSensitivity, 1.0f);
    TestTrue(TEXT("Same applied baseline yields candidate"), Draft.TryMakeApply(Defaults, Candidate));
    TestEqual(TEXT("Candidate keeps chosen sensitivity"), Candidate.MouseSensitivity, 0.5f);
    TestTrue(TEXT("Candidate keeps chosen inversion"), Candidate.bInvertMouseY);

    // 候选只是一份值；测试不调用全局Apply / Save，不改本机个人偏好。
    const FETInputPreferences Committed = Candidate;
    TestTrue(TEXT("Successful application can become the new baseline"), Draft.Open(Committed));
    TestFalse(TEXT("New applied snapshot is clean"), Draft.IsDirty());
    Draft.UseDefaults();
    TestTrue(TEXT("Defaults only changes the draft"), Draft.IsDirty() && Draft.IsDefault());
    TestEqual(TEXT("Previous applied value stays unchanged"), Committed.MouseSensitivity, 0.5f);
    TestTrue(TEXT("Cancel rereads current applied state"), Draft.Open(Committed));
    TestTrue(TEXT("Cancel preserves inversion"), Draft.GetValue().bInvertMouseY);
    TestFalse(TEXT("Cancel removes dirty state"), Draft.IsDirty());

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    for (const float Bad : {0.0f, 3.01f, NaN})
    {
        TestFalse(TEXT("Invalid draft edit rejected"), Draft.SetSensitivity(Bad));
        TestEqual(TEXT("Rejected edit retains old draft"), Draft.GetValue().MouseSensitivity, 0.5f);
    }
    TestTrue(TEXT("Valid edit before external change"), Draft.SetSensitivity(0.75f));
    FETInputPreferences External = Committed;
    External.MouseSensitivity = 1.5f;
    Candidate = Committed;
    TestFalse(TEXT("External update rejects a stale confirmation"), Draft.TryMakeApply(External, Candidate));
    TestEqual(TEXT("Rejected candidate is cleared"), Candidate.MouseSensitivity, 1.0f);
    TestFalse(TEXT("Rejected candidate clears inversion"), Candidate.bInvertMouseY);
    TestTrue(TEXT("Reload external state"), Draft.Open(External));
    TestFalse(TEXT("Reload has no pending old values"), Draft.IsDirty());

    Draft.Close();
    TestFalse(TEXT("Close invalidates even a held draft"), Draft.IsOpen());
    TestFalse(TEXT("Close rejects inversion edits"), Draft.SetInvertY(false));
    Draft.UseDefaults();
    TestFalse(TEXT("Defaults cannot reopen closed draft"), Draft.TryMakeApply(External, Candidate));
    FETInputPreferences Corrupt;
    Corrupt.MouseSensitivity = NaN;
    TestFalse(TEXT("Invalid baseline is unavailable"), Draft.Open(Corrupt));
    TestFalse(TEXT("Invalid open leaves draft closed"), Draft.IsOpen());
    TestTrue(TEXT("Next page can open cleanly"), Draft.Open(Defaults));
    TestFalse(TEXT("Next page has no old pending change"), Draft.IsDirty());
    return true;
}
#endif