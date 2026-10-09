#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ETUILocalPlayerSubsystem.generated.h"

class AActor;
class APawn;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FETUIObservedPawnChanged,
    APawn*, PreviousPawn,
    APawn*, CurrentPawn);

UCLASS(BlueprintType)
class EMBERTRIALUI_API UETUILocalPlayerSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void PlayerControllerChanged(APlayerController* NewController) override;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    APlayerController* GetObservedController() const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    APawn* GetObservedPawn() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETUIObservedPawnChanged OnObservedPawnChanged;

private:
    void RebindController(APlayerController* NewController);
    void ClearController(const TCHAR* Reason);
    void RefreshObservedPawn(const TCHAR* Reason);
    void SetObservedPawn(APawn* NewPawn, const TCHAR* Reason);

    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    UFUNCTION()
    void HandleControllerEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

    UFUNCTION()
    void HandlePawnEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

    TWeakObjectPtr<APlayerController> ObservedController;
    TWeakObjectPtr<APawn> ObservedPawn;
};