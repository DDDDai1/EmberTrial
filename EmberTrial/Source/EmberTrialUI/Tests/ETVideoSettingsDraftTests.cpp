#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETVideoSettingsDraft.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETVideoSettingsDraftTest,
    "EmberTrial.UI.Settings.VideoDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETVideoSettingsDraftTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETVideoSettingsDraft Draft;
    const FETVideoMode Actual{FIntPoint(1365, 767), EETVideoWindowMode::Windowed};
    FETVideoResolutionCatalogue Catalogue;
    Catalogue.Desktop = FIntPoint(2560, 1440);
    Catalogue.Windowed = {FIntPoint(1600, 900), FIntPoint::ZeroValue,
        FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(-1, 720)};
    Catalogue.Fullscreen = {FIntPoint(1920, 1080)};
    FETVideoMode Candidate = Actual;
    TestFalse(TEXT("Closed draft cannot edit"), Draft.SelectMode(EETVideoWindowMode::Windowed));
    TestFalse(TEXT("Closed draft cannot preview"), Draft.TryMakePreview(Actual, Catalogue, Candidate));
    TestFalse(TEXT("Rejected output cleared"), Candidate.IsWellFormed());

    TestTrue(TEXT("Open actual snapshot"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Initial view is clean"), Draft.IsDirty());
    TestFalse(TEXT("Unchanged view cannot preview"), Draft.TryMakePreview(Actual, Catalogue, Candidate));
    TArray<FIntPoint> Choices;
    Draft.GetResolutions(EETVideoWindowMode::Windowed, Choices);
    TestEqual(TEXT("Deduplicated catalogue includes actual"), Choices.Num(), 3);
    TestTrue(TEXT("Current size visible outside recommendations"), Choices.Contains(Actual.Resolution));
    TestTrue(TEXT("Choices sorted deterministically"), !Choices.IsEmpty() && Choices[0] == FIntPoint(1280, 720));
    TestEqual(TEXT("Input catalogue untouched"), Catalogue.Windowed.Num(), 5);

    TestTrue(TEXT("Edit windowed size"), Draft.SelectResolution(FIntPoint(1600, 900)));
    TestTrue(TEXT("Candidate from current baseline"), Draft.TryMakePreview(Actual, Catalogue, Candidate));
    TestTrue(TEXT("Candidate preserves intended size"), Candidate.Resolution == FIntPoint(1600, 900));
    TestTrue(TEXT("Editing leaves actual snapshot unchanged"), Actual.Resolution == FIntPoint(1365, 767));
    TestFalse(TEXT("Unlisted edit rejected"), Draft.SelectResolution(FIntPoint(1111, 777)));
    TestTrue(TEXT("Rejected edit retains choice"), Draft.GetValue() == Candidate);
    TestTrue(TEXT("Borderless selectable"), Draft.SelectMode(EETVideoWindowMode::Borderless));
    TestTrue(TEXT("Borderless uses desktop"), Draft.GetValue().Resolution == Catalogue.Desktop);
    TestFalse(TEXT("Borderless cannot pick separate size"), Draft.SelectResolution(FIntPoint(1280, 720)));
    TestTrue(TEXT("Return to windowed"), Draft.SelectMode(EETVideoWindowMode::Windowed));
    TestTrue(TEXT("Windowed draft choice remembered"), Draft.GetValue().Resolution == FIntPoint(1600, 900));

    FETVideoResolutionCatalogue Missing = Catalogue;
    Missing.Windowed = {FIntPoint(1280, 720)};
    TestFalse(TEXT("Removed candidate rejects stale preview"), Draft.TryMakePreview(Actual, Missing, Candidate));
    TestFalse(TEXT("Stale preview clears output"), Candidate.IsWellFormed());
    FETVideoMode Changed = Actual;
    Changed.Resolution = FIntPoint(1280, 720);
    TestFalse(TEXT("External actual change rejects old draft"), Draft.TryMakePreview(Changed, Catalogue, Candidate));
    FETVideoMode Aliased = Actual;
    TestTrue(TEXT("Aliased actual and output supported"), Draft.TryMakePreview(Aliased, Catalogue, Aliased));
    TestTrue(TEXT("Aliased output intact"), Aliased.Resolution == FIntPoint(1600, 900));

    TestTrue(TEXT("Select borderless for desktop change"), Draft.SelectMode(EETVideoWindowMode::Borderless));
    FETVideoResolutionCatalogue NewDesktop = Catalogue;
    NewDesktop.Desktop = FIntPoint(1920, 1080);
    TestFalse(TEXT("Desktop change cannot silently replace choice"), Draft.TryMakePreview(Actual, NewDesktop, Candidate));
    TestTrue(TEXT("Fullscreen selectable"), Draft.SelectMode(EETVideoWindowMode::Fullscreen));
    TestTrue(TEXT("Fullscreen uses supplied catalogue"), Draft.GetValue().Resolution == FIntPoint(1920, 1080));
    TestFalse(TEXT("Invalid mode rejected"), Draft.SelectMode(EETVideoWindowMode::Invalid));

    TestTrue(TEXT("Reopen cancels edits"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Reopened draft clean"), Draft.IsDirty());
    Catalogue.Fullscreen.Reset();
    TestTrue(TEXT("Open without fullscreen catalogue"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Unavailable fullscreen rejected"), Draft.SelectMode(EETVideoWindowMode::Fullscreen));
    TArray<EETVideoWindowMode> Modes;
    Draft.GetModes(Modes);
    TestEqual(TEXT("Only available modes offered"), Modes.Num(), 2);

    Draft.Close();
    Draft.GetResolutions(EETVideoWindowMode::Windowed, Choices);
    TestTrue(TEXT("Close clears choices"), Choices.IsEmpty());
    TestFalse(TEXT("Close rejects edits"), Draft.SelectResolution(FIntPoint(1600, 900)));
    TestFalse(TEXT("Invalid actual unavailable"), Draft.Open(FETVideoMode{}, Catalogue));
    TestFalse(TEXT("Invalid open leaves no old session"), Draft.IsOpen());
    TestTrue(TEXT("Next page opens cleanly"), Draft.Open(Actual, Catalogue));
    TestFalse(TEXT("Next page has no pending edits"), Draft.IsDirty());
    return true;
}
#endif