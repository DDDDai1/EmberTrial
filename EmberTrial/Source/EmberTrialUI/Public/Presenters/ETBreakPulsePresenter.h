#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TimerManager.h"
#include "ETBreakPulsePresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETUILocalPlayerSubsystem;
class UWorld;
struct FGameplayTag;

UENUM(BlueprintType)
enum class EETBreakPulseViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Dead,
    Ready
};

USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETBreakPulseViewData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    EETBreakPulseViewState State = EETBreakPulseViewState::Unavailable;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    FString PawnName;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    bool bCasting = false;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    bool bCoolingDown = false;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Remaining = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Duration = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float ReadyFraction = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FETBreakPulseViewChanged, const FETBreakPulseViewData&,
                                            Data);

// 一个本地身体 / 一个HUD View；只观察GAS，不决定施放或修改冷却。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETBreakPulsePresenter : public UObject
{
    GENERATED_BODY()
public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();
    void SetPresentationEnabled(bool bEnabled);
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETBreakPulseViewData GetSnapshot() const;
    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETBreakPulseViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    friend class FETBreakPulseHUDTest;
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void StopTimers();
    void RebindObservedPawn(const TCHAR* Reason);
    void RequestDeferredRefresh();
    void HandleDeferredRefresh(uint64 Serial);
    void HandlePeriodicRefresh(uint64 Serial);
    void RefreshSnapshot(const TCHAR* Reason);
    void SyncPeriodicRefresh(const FETBreakPulseViewData& Data);
    FETBreakPulseViewData BuildSnapshot() const;
    void HandleTagChanged(FGameplayTag Tag, int32 Count,
                          TWeakObjectPtr<UAbilitySystemComponent> Source, uint64 Serial);
    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);
    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    TWeakObjectPtr<UWorld> TimerWorld;
    FDelegateHandle CooldownChangedHandle;
    FDelegateHandle CastingChangedHandle;
    FDelegateHandle DeathChangedHandle;
    FTimerHandle DeferredRefreshTimer;
    FTimerHandle PeriodicRefreshTimer;
    FETBreakPulseViewData Snapshot;
    uint64 ObservationSerial = 0;
    bool bObserving = false;
    bool bPresentationEnabled = false;
};