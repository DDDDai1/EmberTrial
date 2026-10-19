#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GameplayTagContainer.h"
#include "ETAIController.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBlackboardData;
class UBehaviorTree;

// 每个敌人的感知 / 黑板 / 追击决策所有者；BT节点不缓存另一名敌人的状态。
UCLASS()
class EMBERTRIAL_API AETAIController : public AAIController
{
    GENERATED_BODY()

public:
    AETAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // 查询当前有效本地玩家；感知事件与ASC回调负责更新，不向蓝图开放写入。
    UFUNCTION(BlueprintPure, Category = "EmberTrial|AI")
    AEmberTrialCharacter* GetSensedPlayer() const;

    // Service / Move / Melee共用的实时重判入口；不替代各任务的执行前复核。
    void UpdatePursuitDecision();
    bool IsPursuitAllowed() const;
    bool IsMeleeAttackAllowed() const;
    void NotifyMeleeAttackAttemptFinished();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // ---- 感知与树资源装配；保留命名子对象 / 反射字段以兼容现有资产 ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|AI",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAIPerceptionComponent> SightPerception;

    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|AI")
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI")
    TObjectPtr<UBlackboardData> EnemyBlackboardAsset;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI")
    TObjectPtr<UBehaviorTree> EnemyBehaviorTree;

    // 停止 / 恢复形成迟滞；近战与冷却另有判断，不能只保留一个距离。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI", meta = (ClampMin = "1.0", Units = "cm"))
    float PursuitStopDistance = 90.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI", meta = (ClampMin = "1.0", Units = "cm"))
    float PursuitResumeDistance = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI", meta = (ClampMin = "1.0", Units = "cm"))
    float MeleeAttackDistance = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI", meta = (ClampMin = "0.1", Units = "s"))
    float MeleeRetryDelay = 0.5f;

    // ---- 非拥有身份与本Controller的订阅；不延长旧身体 / 目标寿命 ----
    TWeakObjectPtr<AEmberTrialCharacter> ObservedBody;
    TWeakObjectPtr<AEmberTrialCharacter> SensedPlayer;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedBodyASC;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedTargetASC;
    FDelegateHandle BodyDeadHandle;
    FDelegateHandle BodyAttackingHandle;
    FDelegateHandle BodyHitReactHandle;
    FDelegateHandle TargetDeadHandle;

    // ---- 生命周期、一次启动、距离保持与决策分别建模 ----
    bool bEndingPlay = false;
    bool bBlackboardReady = false;
    bool bControllerBegunPlay = false;
    // 已尝试但无效的配置不应在每次Service Tick重复启动。
    bool bTreeStartAttempted = false;
    bool bTreeStarted = false;
    bool bPursuitAllowed = false;
    bool bHoldingDistance = false;
    bool bMeleeAttackAllowed = false;
    // 强制停树会同步触发任务 / Tag回调，期间拒绝重新观察和启动。
    bool bStoppingTree = false;
    double NextMeleeAttemptTime = 0.0;

    // ---- 引擎 / ASC事件适配；回调必须重新核对当前身份 ----
    UFUNCTION()
    void HandleBodyReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);

    UFUNCTION()
    void HandleSightUpdated(AActor* SensedActor, FAIStimulus Stimulus);

    UFUNCTION()
    void HandleTargetEndPlay(AActor* EndedTarget, EEndPlayReason::Type PlayEndReason);

    void HandleDeadTagChanged(FGameplayTag ChangedTag, int32 NewCount);
    void HandleBodyActionTagChanged(FGameplayTag ChangedTag, int32 NewCount);

    // ---- 黑板初始化与执行资格 ----
    bool InitializeEnemyBlackboard();
    bool CanObservePlayer() const;
    bool HasValidPursuitConfig() const;

    // ---- 当前身体 / ASC订阅 ----
    void ObserveBodyState();
    void RemoveBodyStateSubscriptions();

    // ---- 感知目标与身份替换 ----
    void RefreshVisiblePlayer();
    void SetSensedPlayer(AEmberTrialCharacter* NewPlayer, const TCHAR* Reason);
    void ClearSensedPlayer(const TCHAR* Reason);

    // ---- 决策发布与原生移动停止 ----
    void SetPursuitAllowed(bool bNewAllowed, const TCHAR* Reason, float Distance,
                           bool bForceStop = false);
    void SetMeleeAttackAllowed(bool bNewAllowed, const TCHAR* Reason, float Distance);
    // 调用前已提交本地决策状态；黑板观察者可以同步回调。
    void UpdateBlackboardDecision(const FName& KeyName, bool bNewAllowed);
    void StopPursuitMovement();

    // ---- 树与控制生命周期收尾 ----
    void TryStartEnemyBehaviorTree();
    void StopEnemyBehaviorTree(const TCHAR* Reason);
    void StopObservingBody();
    void LogBodySnapshot(const TCHAR* Reason) const;
};
