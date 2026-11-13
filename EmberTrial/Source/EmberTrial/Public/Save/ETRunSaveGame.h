#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ETRunSaveGame.generated.h"

enum class EETRunSaveValidation : uint8
{
    Valid,
    InvalidData,
    UnsupportedVersion
};

USTRUCT()
struct EMBERTRIAL_API FETRunSaveSnapshot
{
    GENERATED_BODY()

    static constexpr int32 LegacySchemaVersion = 1;
    static constexpr int32 CurrentSchemaVersion = 2;

    EETRunSaveValidation Validate() const;
    EETRunSaveValidation UpgradeFromDisk();

    // Zero defaults allow an absent version or sequence to fail validation.
    UPROPERTY(SaveGame)
    int32 SchemaVersion = 0;

    UPROPERTY(SaveGame)
    int64 Sequence = 0;

    UPROPERTY(SaveGame)
    FDateTime SavedAtUtc;

    // Only write logical IDs. Resolve them against the current content when restoring.
    UPROPERTY(SaveGame)
    FString MapPackage;

    UPROPERTY(SaveGame)
    FName CheckpointId;

    UPROPERTY(SaveGame)
    TArray<FName> ClearedEncounterIds;

    UPROPERTY(SaveGame)
    TArray<FName> ClaimedRewardIds;

    UPROPERTY(SaveGame)
    TArray<FName> OwnedRelicIds;

    UPROPERTY(SaveGame)
    FName EquippedRelicA;

    UPROPERTY(SaveGame)
    FName EquippedRelicB;

    UPROPERTY(SaveGame)
    int32 HealingCharges = INDEX_NONE;

    // Keep this property name for UE's tagged deserialization of v1 files.
    UPROPERTY(SaveGame)
    bool bRetired = false;

    UPROPERTY(SaveGame)
    bool bRunClosed = false;
};

UCLASS()
class EMBERTRIAL_API UETRunSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    FETRunSaveSnapshot Snapshot;
};
