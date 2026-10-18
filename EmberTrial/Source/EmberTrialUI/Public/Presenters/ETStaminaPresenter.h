#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ETStaminaPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETUILocalPlayerSubsystem;
struct FOnAttributeChangeData;

// 可用性与数值分开：Ready允许资源为0；没有Pawn不能显示成“满值”。
UENUM(BlueprintType)
enum class EETStaminaViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Ready
};

// ---- View只读快照；不含ASC / Actor所有权，不按显示整数回写数值 ----
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FETStaminaViewChanged, const FETStaminaViewData&, Data);

// 只观察当前LocalPlayer的数据源；不初始化资源，不持有Widget，不开Tick。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETStaminaPresenter : public UObject
{
    GENERATED_BODY()

public:
    // 重启只释放数据源订阅，保留已有View监听；Shutdown按原去重规则发布不可用状态。
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETStaminaViewData GetSnapshot() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETStaminaViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    // ---- Subsystem → Pawn Ready → ASC属性：分层解绑，先订阅再读当前值 ----
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void ClearAttributeBindings();
    void RebindObservedPawn(const TCHAR* Reason);
    void RefreshReadyBinding(const TCHAR* Reason);
    FETStaminaViewData BuildSnapshot() const;
    void PublishSnapshot(const TCHAR* Reason, bool bForce = false);

    // ---- 回调只触发当前快照重建；不把事件Payload当作资源真值 ----
    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
                                TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);

    // ---- 仅弱引用数据源；Handle只移除本Presenter的委托，不清共享事件 ----
    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle StaminaHandle;
    FDelegateHandle MaxStaminaHandle;
    FETStaminaViewData Snapshot;
    // 同一个ASC重新订阅时，仍拒绝旧一轮回调；与身体身份检查互补。
    uint64 ObservationSerial = 0;
    bool bObserving = false;
};
