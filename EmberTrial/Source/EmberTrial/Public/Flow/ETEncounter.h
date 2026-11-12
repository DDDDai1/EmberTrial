#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "ETEncounter.generated.h"

class AEmberTrialCharacter;
class ATargetPoint;
class UBoxComponent;
class UETCombatLifeComponent;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FETEncounterSpawn
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Encounter")
    TSubclassOf<AEmberTrialCharacter> EnemyClass;

    UPROPERTY(EditInstanceOnly, Category = "Encounter")
    TObjectPtr<ATargetPoint> Point;
};

// One wave per world instance. The gate opens after all registered deaths.
UCLASS()
class EMBERTRIAL_API AETEncounter : public AActor
{
    GENERATED_BODY()

public:
    AETEncounter();
    bool HasCleared() const { return State == EState::Cleared; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
    friend class FETMixedEncounterTest;
    friend class FETEncounterCheckpointResumeTest;
    friend class FETTrialVictoryTest;

    enum class EState : uint8 { Waiting, Active, Cleared, Failed };

    struct FTrackedEnemy
    {
        TWeakObjectPtr<UETCombatLifeComponent> Life;
        FDelegateHandle DeathHandle;
    };

    UPROPERTY(VisibleAnywhere, Category = "Encounter")
    TObjectPtr<UBoxComponent> EntryTrigger;

    UPROPERTY(VisibleAnywhere, Category = "Encounter")
    TObjectPtr<UStaticMeshComponent> ExitGate;

    UPROPERTY(EditInstanceOnly, Category = "Encounter")
    TArray<FETEncounterSpawn> Spawns;

    // Checkpoints beyond this gate. Checked only on world entry.
    UPROPERTY(EditInstanceOnly, Category = "Encounter")
    TArray<FName> ClearedOnSpawnCheckpointIds;

    EState State = EState::Waiting;
    FTimerHandle InitialOverlapTimer;
    TMap<TWeakObjectPtr<AEmberTrialCharacter>, FTrackedEnemy> Remaining;

    UFUNCTION()
    void HandleEntry(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComponent,
                     int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);

    UFUNCTION()
    void HandleEnemyEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

    void ProbeInitialOverlap();
    bool RestoreClearedEncounter();
    void TryStart(AEmberTrialCharacter* Player);
    bool HasValidSpawns() const;
    bool SpawnEnemy(const FETEncounterSpawn& Spawn);
    void HandleDeath(UETCombatLifeComponent* Life);
    void Fail(const TCHAR* Reason);
    void OpenGate();
    void DetachEnemies(bool bDestroy);
};
