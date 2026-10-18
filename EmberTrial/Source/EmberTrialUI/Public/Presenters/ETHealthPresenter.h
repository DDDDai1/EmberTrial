#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ETHealthPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETUILocalPlayerSubsystem;
struct FOnAttributeChangeData;

// 可用性与数值分开：Ready允许资源为0；没有Pawn不能显示成“满值”。
UENUM(BlueprintType)
enum class EETHealthViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Ready
};

// ---- View只读快照；不含ASC / Actor所有权，不按显示整数回写数值 ----
USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETHealthViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    EETHealthViewState State = EETHealthViewState::Unavailable;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    FString PawnName;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Health = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float MaxHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float HealthFraction = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FETHealthViewChanged, const FETHealthViewData&, Data);

// 只观察当前LocalPlayer的数据源；不初始化资源，不持有Widget，不开Tick。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETHealthPresenter : public UObject
{
    GENERATED_BODY()

public:
    // 重启只释放数据源订阅，保留已有View监听；Shutdown按原去重规则发布不可用状态。
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETHealthViewData GetSnapshot() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETHealthViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    // ---- Subsystem → Pawn Ready → ASC属性：分层解绑，先订阅再读当前值 ----
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void ClearAttributeBindings();
    void RebindObservedPawn(const TCHAR* Reason);
    void RefreshReadyBinding(const TCHAR* Reason);
    FETHealthViewData BuildSnapshot() const;
    void PublishSnapshot(const TCHAR* Reason, bool bForce = false);

    // ---- 回调只触发当前快照重建；不把事件Payload当作资源真值 ----
    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
                                TWeakObjectPtr<UAbilitySystemComponent> SourceASC);

    // ---- 仅弱引用数据源；Handle只移除本Presenter的委托，不清共享事件 ----
    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHealthHandle;
    FETHealthViewData Snapshot;
    bool bObserving = false;
};
