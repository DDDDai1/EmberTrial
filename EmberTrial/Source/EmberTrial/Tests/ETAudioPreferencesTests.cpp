#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETAudioSettingsTypes.h"
#include "Misc/AutomationTest.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAudioPreferencesTest,
    "EmberTrial.M3.Settings.AudioPreferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAudioPreferencesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETAudioPreferences Value;
    float Music = -1.0f;
    float Effects = -1.0f;
    TestTrue(TEXT("Defaults are valid"), Value.IsValid());
    TestTrue(TEXT("Defaults produce unity gains"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Default music"), Music, 1.0f);
    TestEqual(TEXT("Default effects"), Effects, 1.0f);

    Value.MasterVolume = 0.5f;
    Value.MusicVolume = 0.4f;
    Value.EffectsVolume = 0.2f;
    TestTrue(TEXT("Independent channel values accepted"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Master applied once to music"), Music, 0.2f);
    TestEqual(TEXT("Master applied once to effects"), Effects, 0.1f);
    Value.MasterVolume = 0.0f;
    TestTrue(TEXT("Mute is a valid preference"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Muted music gain"), Music, 0.0f);
    TestEqual(TEXT("Muted effects gain"), Effects, 0.0f);
    TestEqual(TEXT("Mute retains channel preference"), Value.MusicVolume, 0.4f);
    Value.MasterVolume = 1.0f;
    TestTrue(TEXT("Unmute retains independent values"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Music restored from preference"), Music, 0.4f);

    Value.MusicVolume = -0.1f;
    TestFalse(TEXT("Negative volume rejected"), Value.TryGetEffectiveVolumes(Music, Effects));
    TestEqual(TEXT("Failure clears music output"), Music, 0.0f);
    TestEqual(TEXT("Failure clears effects output"), Effects, 0.0f);
    Value.MusicVolume = 1.1f;
    TestFalse(TEXT("Above unity rejected without clamping"), Value.IsValid());
    Value.MusicVolume = 0.4f;
    Value.MasterVolume = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN master rejected"), Value.IsValid());
    Value.MasterVolume = 1.0f;
    Value.EffectsVolume = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("Infinite effects rejected"), Value.IsValid());
    Value.EffectsVolume = 0.2f;
    Value.MasterVolume = 0.5f;
    TestTrue(TEXT("Self-field output alias supported"),
        Value.TryGetEffectiveVolumes(Value.MusicVolume, Value.EffectsVolume));
    TestEqual(TEXT("Alias uses original music input"), Value.MusicVolume, 0.2f);
    TestEqual(TEXT("Alias uses original effects input"), Value.EffectsVolume, 0.1f);
    return true;
}
#endif