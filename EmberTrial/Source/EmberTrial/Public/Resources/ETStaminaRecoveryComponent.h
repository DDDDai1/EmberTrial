#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETStaminaRecoveryComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;
struct FGameplayTag;

// 身体资源规则：事件驱动开关World Timer，实际增量统一交给Instant GE。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETStaminaRecoveryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETStaminaRecoveryComponent();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;
    // 查询Timer是否存在；默认满值、忙碌、死亡时均不保留恢复Timer。
    bool IsRecoveryScheduled() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETStaminaRecoveryTest;

    // ---- 可调规则；运行期仍复核有限值与范围，防止绕过编辑器约束 ----
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
              meta = (ClampMin = "0.01", ClampMax = "1000.0"))
    float RecoveryRate = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
              meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
    float RecoveryDelay = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
              meta = (ClampMin = "0.05", ClampMax = "0.5", Units = "s"))
    float RecoveryInterval = 0.10f;

    // ---- 数据源所有权与按需计时 ----
    void RefreshObservation();
    void StopObserving();
    void SyncRecoveryTimer();
    void ClearRecoveryTimer(const TCHAR* Reason);
    bool ReadBody(AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC, float& Stamina,
                  float& MaxStamina) const;
    void HandleRecoveryTimer(uint64 Serial);

    // ---- 身份 / 序号保护；拒绝上一轮观察的迟到回调 ----
    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Body, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
                                TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);
    void HandleActionTagChanged(const FGameplayTag Tag, int32 NewCount,
                                TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);

    // ---- 运行期状态；属性真值只在ASC中，不在组件复制一份 ----
    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle StaminaHandle;
    FDelegateHandle MaxStaminaHandle;
    // 与cpp的阻断Tag列表顺序配对，注册和解绑共用同一列表。
    TArray<FDelegateHandle> ActionTagHandles;
    FTimerHandle RecoveryTimer;
    // Observation隔离旧订阅；DecreaseRevision识别GE同步回调里的新消费。
    uint64 ObservationSerial = 0;
    uint64 DecreaseRevision = 0;
    // 世界时间，不使用墙钟；忙碌 / 暂停的整段时间不会在恢复时补量。
    double RecoveryNotBefore = 0.0;
    double LastSampleTime = 0.0;
    bool bRecovering = false;
    bool bEndingPlay = false;
};
