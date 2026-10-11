#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "ETRunSessionSubsystem.generated.h"

class AEmberTrialCharacter;
class APlayerStart;
class UWorld;

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FETRunTravelFailed, UWorld*, const FText&);
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FETRunCheckpointChanged, UWorld*, FName);

enum class EETRunTravelIntent : uint8
{
    None,
    NewRun,
    Retry,
    Continue,
    MainMenu
};

UCLASS()
class EMBERTRIAL_API UETRunSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    static FName GetEntryCheckpointId() { return FName(TEXT("Entry")); }
    FName GetSpawnCheckpointId(UWorld* SpawnWorld) const;
    void NotifyGameplayWorldReady(UWorld* GameplayWorld, FName SpawnId);
    bool TryActivateCheckpoint(UWorld* RequestWorld, APlayerStart* CheckpointStart,
        AEmberTrialCharacter* PlayerBody);
    bool IsCheckpointActive(UWorld* CheckWorld, FName CheckpointId) const;
    bool HasContinueSnapshot() const;
    bool SetCheckpointIdForDevelopmentTest(UWorld* TestWorld, FName TestId);

    bool RequestNewRun(UWorld* RequestWorld, const FSoftObjectPath& Map,
        FText& OutError);
    bool RequestRetry(UWorld* RequestWorld, FText& OutError);
    bool RequestContinue(UWorld* RequestWorld, FText& OutError);
    bool RequestMainMenu(UWorld* RequestWorld, const FSoftObjectPath& Map,
        FText& OutError);
    bool IsTravelPending() const { return bTravelPending; }

    FETRunTravelFailed OnTravelFailed;
    FETRunCheckpointChanged OnCheckpointChanged;

private:
    bool BeginTravel(UWorld* RequestWorld, const FString& TargetPackage,
        FName SpawnId, EETRunTravelIntent Intent, FText& OutError);
    bool RejectRequest(const TCHAR* Reason, FText& OutError);
    void HandlePostLoadMap(UWorld* LoadedWorld);
    void HandleTravelFailure(UWorld* FailedWorld,
        ETravelFailure::Type FailureType, const FString& ErrorString);
    void ResetPendingTravel();
    static FString GetWorldPackage(UWorld* World);
    static bool IsGameplayIntent(EETRunTravelIntent Intent);
    static const TCHAR* GetIntentName(EETRunTravelIntent Intent);

    FString GameplayPackage;
    FName ActiveCheckpointId = NAME_None;
    FString PendingTargetPackage;
    FName PendingCheckpointId = NAME_None;
    EETRunTravelIntent PendingIntent = EETRunTravelIntent::None;
    TWeakObjectPtr<UWorld> OriginWorld;
    FDelegateHandle PostLoadHandle;
    FDelegateHandle TravelFailureHandle;
    bool bPendingGameplayReady = false;
    bool bTravelPending = false;
    bool bShuttingDown = false;
};