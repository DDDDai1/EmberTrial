#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ETStaminaPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETUILocalPlayerSubsystem;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EETStaminaViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Ready
};

USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETStaminaViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    EETStaminaViewState State = EETStaminaViewState::Unavailable;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    FString PawnName;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Stamina = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float MaxStamina = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float StaminaFraction = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FETStaminaViewChanged, const FETStaminaViewData&, Data);

UCLASS(BlueprintType)
class EMBERTRIALUI_API UETStaminaPresenter : public UObject
{
    GENERATED_BODY()

public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETStaminaViewData GetSnapshot() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETStaminaViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void ClearAttributeBindings();
    void RebindObservedPawn(const TCHAR* Reason);
    void RefreshReadyBinding(const TCHAR* Reason);
    FETStaminaViewData BuildSnapshot() const;
    void PublishSnapshot(const TCHAR* Reason, bool bForce = false);

    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
        TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle StaminaHandle;
    FDelegateHandle MaxStaminaHandle;
    FETStaminaViewData Snapshot;
    uint64 ObservationSerial = 0;
    bool bObserving = false;
};