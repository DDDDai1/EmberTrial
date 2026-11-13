#if WITH_DEV_AUTOMATION_TESTS

#include "Save/ETRunSaveGame.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

namespace
{
FETRunSaveSnapshot MakeRunSaveSnapshotFixture()
{
    FETRunSaveSnapshot Snapshot;
    Snapshot.SchemaVersion = FETRunSaveSnapshot::CurrentSchemaVersion;
    Snapshot.Sequence = 20;
    Snapshot.SavedAtUtc = FDateTime(2026, 1, 2, 3, 4, 5);
    Snapshot.MapPackage = TEXT("/Game/EmberTrial/Maps/L_ET_TrialRoute");
    Snapshot.CheckpointId = TEXT("Checkpoint_B");
    Snapshot.ClearedEncounterIds = {TEXT("intro"), TEXT("mixed")};
    Snapshot.ClaimedRewardIds = {TEXT("intro_reward")};
    Snapshot.OwnedRelicIds = {TEXT("life_ember"), TEXT("shatter_seal")};
    Snapshot.EquippedRelicA = TEXT("life_ember");
    Snapshot.EquippedRelicB = TEXT("shatter_seal");
    Snapshot.HealingCharges = 1;
    return Snapshot;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRunSaveValidationTest, "EmberTrial.M5.Save.Validation",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRunSaveValidationTest::RunTest(const FString& Parameters)
{
    const FETRunSaveSnapshot Valid = MakeRunSaveSnapshotFixture();
    TestTrue(TEXT("Complete snapshot is valid"), Valid.Validate() == EETRunSaveValidation::Valid);
    TestTrue(TEXT("Default snapshot is invalid"),
             FETRunSaveSnapshot().Validate() == EETRunSaveValidation::InvalidData);

    FETRunSaveSnapshot Candidate = Valid;
    ++Candidate.SchemaVersion;
    TestTrue(TEXT("Future schema gets a distinct result"),
             Candidate.Validate() == EETRunSaveValidation::UnsupportedVersion);

    const auto Reject = [this, &Candidate, &Valid](const TCHAR* Label)
    {
        TestTrue(Label, Candidate.Validate() == EETRunSaveValidation::InvalidData);
        Candidate = Valid;
    };

    Candidate = Valid;
    Candidate.Sequence = 0;
    Reject(TEXT("Zero sequence is invalid"));
    Candidate.SavedAtUtc = FDateTime::MinValue();
    Reject(TEXT("Missing timestamp is invalid"));
    Candidate.MapPackage += TEXT(".L_ET_TrialRoute");
    Reject(TEXT("Object path is not a map package name"));
    Candidate.CheckpointId = NAME_None;
    Reject(TEXT("Missing checkpoint ID is invalid"));
    Candidate.HealingCharges = -1;
    Reject(TEXT("Negative healing charges are invalid"));
    Candidate.ClearedEncounterIds.Add(Valid.ClearedEncounterIds[0]);
    Reject(TEXT("Duplicate cleared encounter is invalid"));
    Candidate.ClaimedRewardIds.Add(NAME_None);
    Reject(TEXT("Empty claimed reward ID is invalid"));
    Candidate.OwnedRelicIds.Add(Valid.OwnedRelicIds[0]);
    Reject(TEXT("Duplicate owned relic is invalid"));
    Candidate.EquippedRelicA = TEXT("unowned");
    Reject(TEXT("Equipped item must be owned"));
    Candidate.EquippedRelicB = Candidate.EquippedRelicA;
    Reject(TEXT("One relic cannot occupy both slots"));

    Candidate.EquippedRelicA = NAME_None;
    Candidate.EquippedRelicB = NAME_None;
    Candidate.HealingCharges = 0;
    TestTrue(TEXT("Empty slots and exhausted healing are valid"),
             Candidate.Validate() == EETRunSaveValidation::Valid);

    Candidate = Valid;
    Candidate.SchemaVersion = FETRunSaveSnapshot::LegacySchemaVersion;
    Candidate.bRetired = true;
    TestTrue(TEXT("A v1 retirement marker upgrades"),
             Candidate.UpgradeFromDisk() == EETRunSaveValidation::Valid);
    TestTrue(TEXT("The migrated checkpoint remains closed"), Candidate.bRunClosed);
    TestFalse(TEXT("The legacy flag is cleared"), Candidate.bRetired);
    TestEqual(TEXT("Migration preserves sequence"), Candidate.Sequence, Valid.Sequence);
    TestEqual(TEXT("Migration preserves checkpoint"), Candidate.CheckpointId, Valid.CheckpointId);
    TestEqual(TEXT("Migration preserves healing charges"), Candidate.HealingCharges, Valid.HealingCharges);

    Candidate = Valid;
    Candidate.SchemaVersion = FETRunSaveSnapshot::LegacySchemaVersion;
    Candidate.HealingCharges = -1;
    TestTrue(TEXT("Broken v1 data fails after validation"),
             Candidate.UpgradeFromDisk() == EETRunSaveValidation::InvalidData);
    TestEqual(TEXT("Rejected data remains at v1"), Candidate.SchemaVersion,
              FETRunSaveSnapshot::LegacySchemaVersion);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRunSaveRoundTripTest, "EmberTrial.M5.Save.RoundTrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRunSaveRoundTripTest::RunTest(const FString& Parameters)
{
    UETRunSaveGame* Source = NewObject<UETRunSaveGame>();
    Source->Snapshot = MakeRunSaveSnapshotFixture();
    Source->Snapshot.bRunClosed = true;
    const FETRunSaveSnapshot& Expected = Source->Snapshot;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Serialize UE SaveGame to memory"),
                  UGameplayStatics::SaveGameToMemory(Source, Bytes) && !Bytes.IsEmpty()))
    {
        return false;
    }

    const UETRunSaveGame* Restored = Cast<UETRunSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Deserialize native save class"), Restored))
    {
        return false;
    }

    const FETRunSaveSnapshot& Actual = Restored->Snapshot;
    TestTrue(TEXT("Restored snapshot is valid"), Actual.Validate() == EETRunSaveValidation::Valid);
    TestEqual(TEXT("Schema"), Actual.SchemaVersion, Expected.SchemaVersion);
    TestEqual(TEXT("Sequence"), Actual.Sequence, Expected.Sequence);
    TestTrue(TEXT("UTC timestamp"), Actual.SavedAtUtc == Expected.SavedAtUtc);
    TestEqual(TEXT("Map package"), Actual.MapPackage, Expected.MapPackage);
    TestEqual(TEXT("Checkpoint"), Actual.CheckpointId, Expected.CheckpointId);
    TestTrue(TEXT("Cleared encounters"), Actual.ClearedEncounterIds == Expected.ClearedEncounterIds);
    TestTrue(TEXT("Claimed rewards"), Actual.ClaimedRewardIds == Expected.ClaimedRewardIds);
    TestTrue(TEXT("Owned relics"), Actual.OwnedRelicIds == Expected.OwnedRelicIds);
    TestEqual(TEXT("Slot A"), Actual.EquippedRelicA, Expected.EquippedRelicA);
    TestEqual(TEXT("Slot B"), Actual.EquippedRelicB, Expected.EquippedRelicB);
    TestEqual(TEXT("Healing charges"), Actual.HealingCharges, Expected.HealingCharges);
    TestTrue(TEXT("Closed trial survives round trip"), Actual.bRunClosed);
    return !HasAnyErrors();
}

#endif
