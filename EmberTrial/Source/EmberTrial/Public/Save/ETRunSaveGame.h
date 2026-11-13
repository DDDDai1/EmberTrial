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

    static constexpr int32 CurrentSchemaVersion = 1;

    EETRunSaveValidation Validate() const;

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

    // Saves written before retirement was introduced default to an active checkpoint.
    UPROPERTY(SaveGame)
    bool bRetired = false;
};

UCLASS()
class EMBERTRIAL_API UETRunSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    FETRunSaveSnapshot Snapshot;
};
