#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayAbilitySpecHandle.h"
#include "ETBTTask_MeleeAttack.generated.h"

class AETAIController;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
struct FAbilityEndedData;

// 每个AI单独实例化；一次任务只管理自己请求的轻攻击Spec与结束订阅。
UCLASS()
class UETBTTask_MeleeAttack : public UBTTaskNode
{
    GENERATED_BODY()

public:
    // ---- BT调度入口；激活能力延后到首次Tick ----
    UETBTTask_MeleeAttack(const FObjectInitializer& ObjectInitializer);

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
                                            uint8* NodeMemory) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp,
                                          uint8* NodeMemory) override;

protected:
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                          float DeltaSeconds) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                EBTNodeResult::Type TaskResult) override;

private:
    // ---- 阶段与编辑器配置；正常结束由能力事件驱动，超时只作兜底 ----
    enum class EPhase : uint8
    {
        Idle,
        Preparing,
        WaitingForAbility
    };

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI", meta = (ClampMin = "1.0", Units = "deg/s"))
    float TurnRate = 360.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI",
              meta = (ClampMin = "0.1", ClampMax = "30.0", Units = "deg"))
    float FacingTolerance = 5.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI", meta = (ClampMin = "0.1", Units = "s"))
    float PreparationTimeout = 1.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|AI", meta = (ClampMin = "0.1", Units = "s"))
    float AbilityWatchdog = 5.0f;

    // ---- 本次执行身份；弱引用不延长世界 / 身体 / ASC生命周期 ----
    TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwner;
    TWeakObjectPtr<AETAIController> ActiveController;
    TWeakObjectPtr<AEmberTrialCharacter> ActiveBody;
    TWeakObjectPtr<AEmberTrialCharacter> ActiveTarget;
    TWeakObjectPtr<UAbilitySystemComponent> ActiveASC;
    FGameplayAbilitySpecHandle ActiveSpecHandle;
    FDelegateHandle AbilityEndedHandle;

    // ---- 阶段时钟与发招后固定朝向 ----
    EPhase Phase = EPhase::Idle;
    float PhaseElapsed = 0.0f;
    float LockedYaw = 0.0f;

    // 同步Tag / 结束回调可重入；序号区分旧调用栈与新一轮任务。
    uint64 ExecutionSerial = 0;
    bool bRunning = false;

    // 仅在准备好请求该Spec后取得取消权；不取消外部已在运行的能力。
    bool bOwnsAbility = false;

    // Request返回前收到的结束事件先暂存，避免尚在请求栈内就结束BT任务。
    bool bRequestingAbility = false;
    bool bEndReceived = false;
    bool bEndCancelled = false;

    // ---- 上下文校验、结束通知与幂等资源释放 ----
    bool HasValidSettings() const;
    bool GetLiveContext(UBehaviorTreeComponent& OwnerComp, AETAIController*& OutController,
                        AEmberTrialCharacter*& OutBody, AEmberTrialCharacter*& OutTarget,
                        UAbilitySystemComponent*& OutASC) const;
    void HandleAbilityEnded(const FAbilityEndedData& EndData);
    void FinishAttempt(EBTNodeResult::Type Result, const TCHAR* Reason, bool bCancelOwnedAbility);
    void ReleaseRequest(bool bCancelOwnedAbility);
};
