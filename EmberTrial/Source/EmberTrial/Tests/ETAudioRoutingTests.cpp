#if WITH_DEV_AUTOMATION_TESTS
#include "Settings/ETAudioSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETAudioRoutingTest,
    "EmberTrial.M3.Settings.AudioRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETAudioRoutingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    TStrongObjectPtr<USoundClass> Master(NewObject<USoundClass>());
    TStrongObjectPtr<USoundClass> Music(NewObject<USoundClass>());
    TStrongObjectPtr<USoundClass> Effects(NewObject<USoundClass>());
    TStrongObjectPtr<USoundMix> Mix(NewObject<USoundMix>());
    Music->ParentClass = Effects->ParentClass = Master.Get();
    Master->ChildClasses = { Music.Get(), Effects.Get() };
    Mix->Duration = -1.0f;
    Mix->InitialDelay = Mix->FadeInTime = Mix->FadeOutTime = 0.0f;
    FETAudioPreferences Value;
    Value.MasterVolume = 0.5f;
    Value.MusicVolume = 0.4f;
    Value.EffectsVolume = 0.2f;
    TArray<FSoundClassAdjuster> Entries;
    auto Build = [&]()
    {
        return UETAudioSubsystem::BuildAdjusters(Value, Master.Get(),
            Music.Get(), Effects.Get(), Mix.Get(), Entries);
    };
    if (!TestTrue(TEXT("Dedicated three-class tree accepted"), Build()) ||
        !TestEqual(TEXT("Three independent overrides"), Entries.Num(), 3))
    {
        return false;
    }
    TestTrue(TEXT("Master propagates exactly once"), Entries[0].bApplyToChildren != 0);
    TestFalse(TEXT("Music remains a leaf override"), Entries[1].bApplyToChildren != 0);
    TestFalse(TEXT("Effects remains a leaf override"), Entries[2].bApplyToChildren != 0);
    TestEqual(TEXT("Raw music avoids multiplying master twice"), Entries[1].VolumeAdjuster, 0.4f);
    TestEqual(TEXT("Raw effects retained"), Entries[2].VolumeAdjuster, 0.2f);

    Effects->ParentClass = nullptr;
    TestFalse(TEXT("Unrelated effects class rejected"), Build());
    TestEqual(TEXT("Failure publishes no partial entries"), Entries.Num(), 0);
    Effects->ParentClass = Master.Get();
    Master->Properties.Volume = 0.5f;
    TestFalse(TEXT("Non-unity asset gain rejected"), Build());
    Master->Properties.Volume = 1.0f;
    Mix->SoundClassEffects.AddDefaulted();
    TestFalse(TEXT("Static mix cannot double-adjust volume"), Build());
    Mix->SoundClassEffects.Reset();
    Mix->Duration = 1.0f;
    TestFalse(TEXT("Expiring user mix rejected"), Build());
    Mix->Duration = -1.0f;
    Value.EffectsVolume = -1.0f;
    TestFalse(TEXT("Invalid request has no partial route"), Build());
    Value.EffectsVolume = 0.2f;
    Value.MasterVolume = 0.0f;
    if (!TestTrue(TEXT("Mute retains route"), Build()))
    {
        return false;
    }
    TestEqual(TEXT("Muted master is zero"), Entries[0].VolumeAdjuster, 0.0f);
    TestEqual(TEXT("Music preference survives mute"), Entries[1].VolumeAdjuster, 0.4f);
    return true;
}
#endif