#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "ETRunSessionSubsystem.generated.h"

class UWorld;

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FETRunTravelFailed, UWorld*, const FText&);

enum class EETRunTravelIntent : uint8
{
    None,
    NewRun,
    Retry,
    MainMenu
};

UCLASS()
class EMBERTRIAL_API UETRunSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void NotifyGameplayWorldReady(UWorld* GameplayWorld);
    bool RequestNewRun(UWorld* RequestWorld, const FSoftObjectPath& Map,
        FText& OutError);
    bool RequestRetry(UWorld* RequestWorld, FText& OutError);
    bool RequestMainMenu(UWorld* RequestWorld, const FSoftObjectPath& Map,
        FText& OutError);
    bool IsTravelPending() const { return bTravelPending; }

    FETRunTravelFailed OnTravelFailed;

private:
    bool BeginTravel(UWorld* RequestWorld, const FString& TargetPackage,
        EETRunTravelIntent Intent, FText& OutError);
    bool RejectRequest(const TCHAR* Reason, FText& OutError);
    void HandlePostLoadMap(UWorld* LoadedWorld);
    void HandleTravelFailure(UWorld* FailedWorld,
        ETravelFailure::Type FailureType, const FString& ErrorString);
    void ResetPendingTravel();
    static FString GetWorldPackage(UWorld* World);
    static const TCHAR* GetIntentName(EETRunTravelIntent Intent);

    FString GameplayPackage;
    FString PendingTargetPackage;
    EETRunTravelIntent PendingIntent = EETRunTravelIntent::None;
    TWeakObjectPtr<UWorld> OriginWorld;
    FDelegateHandle PostLoadHandle;
    FDelegateHandle TravelFailureHandle;
    bool bTravelPending = false;
    bool bShuttingDown = false;
};