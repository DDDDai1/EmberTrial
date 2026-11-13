#include "Save/ETRunSaveGame.h"

#include "Misc/PackageName.h"

namespace
{
bool HasUniqueSaveIds(const TArray<FName>& Ids)
{
    TSet<FName> Seen;
    Seen.Reserve(Ids.Num());
    for (const FName Id : Ids)
    {
        if (Id.IsNone() || Seen.Contains(Id))
        {
            return false;
        }
        Seen.Add(Id);
    }
    return true;
}
}

EETRunSaveValidation FETRunSaveSnapshot::UpgradeFromDisk()
{
    if (SchemaVersion != LegacySchemaVersion)
    {
        return Validate();
    }

    FETRunSaveSnapshot Upgraded = *this;
    Upgraded.SchemaVersion = CurrentSchemaVersion;
    Upgraded.bRunClosed = bRetired;
    Upgraded.bRetired = false;
    const EETRunSaveValidation Result = Upgraded.Validate();
    if (Result == EETRunSaveValidation::Valid)
    {
        *this = MoveTemp(Upgraded);
    }
    return Result;
}

EETRunSaveValidation FETRunSaveSnapshot::Validate() const
{
    if (SchemaVersion <= 0)
    {
        return EETRunSaveValidation::InvalidData;
    }
    if (SchemaVersion != CurrentSchemaVersion)
    {
        return EETRunSaveValidation::UnsupportedVersion;
    }

    if (bRetired || Sequence <= 0 || SavedAtUtc == FDateTime::MinValue() ||
        !FPackageName::IsValidLongPackageName(MapPackage) || CheckpointId.IsNone() ||
        HealingCharges < 0 || !HasUniqueSaveIds(ClearedEncounterIds) ||
        !HasUniqueSaveIds(ClaimedRewardIds) || !HasUniqueSaveIds(OwnedRelicIds))
    {
        return EETRunSaveValidation::InvalidData;
    }

    // Both slots may be empty. A real item must be owned and can appear only once.
    if ((!EquippedRelicA.IsNone() && !OwnedRelicIds.Contains(EquippedRelicA)) ||
        (!EquippedRelicB.IsNone() && !OwnedRelicIds.Contains(EquippedRelicB)) ||
        (!EquippedRelicA.IsNone() && EquippedRelicA == EquippedRelicB))
    {
        return EETRunSaveValidation::InvalidData;
    }

    return EETRunSaveValidation::Valid;
}
