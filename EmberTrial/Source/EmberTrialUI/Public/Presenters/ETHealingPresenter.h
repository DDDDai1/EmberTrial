#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ETHealingPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UETHealingResourceComponent;
class UETUILocalPlayerSubsystem;

// Ready允许次数为0；它不等于当前允许开始治疗。
UENUM(BlueprintType)
enum class EETHealingViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Ready
};

// 仅保存展示快照，不延长身体或资源组件的生命周期。
USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETHealingViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    EETHealingViewState State = EETHealingViewState::Unavailable;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    FString PawnName;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    int32 Charges = 0;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    int32 MaxCharges = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FETHealingViewChanged, const FETHealingViewData&, Data);

// 观察LocalPlayer当前身体；不治疗、不初始化资源、不管理Widget。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETHealingPresenter : public UObject
{
    GENERATED_BODY()

public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETHealingViewData GetSnapshot() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETHealingViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    // 数据源订阅分层释放；重启保留已有View对本Presenter的订阅。
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void ClearResourceBinding();
    void RebindObservedPawn(const TCHAR* Reason);
    void RefreshResourceBinding(const TCHAR* Reason);
    FETHealingViewData BuildSnapshot() const;
    void PublishSnapshot(const TCHAR* Reason);

    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    // Native无参事件附带弱来源与代次，拒绝旧组件 / 旧一轮观察的回调。
    void HandleResourceChanged(TWeakObjectPtr<UETHealingResourceComponent> Source, uint64 Serial);

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UETHealingResourceComponent> BoundResource;
    FDelegateHandle ResourceChangedHandle;
    FETHealingViewData Snapshot;
    uint64 ObservationSerial = 0;
    bool bObserving = false;
};