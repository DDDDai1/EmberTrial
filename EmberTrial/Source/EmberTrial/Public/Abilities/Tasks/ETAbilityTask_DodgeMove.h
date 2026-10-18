#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ETAbilityTask_DodgeMove.generated.h"

class UCharacterMovementComponent;
struct FRootMotionSource;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETDodgeMoveSignal);

// 只拥有本次水平RootMotion Source；不扣费、不管理整段动画或无敌窗口。
UCLASS()
class EMBERTRIAL_API UETAbilityTask_DodgeMove : public UAbilityTask
{
    GENERATED_BODY()

public:
    UETAbilityTask_DodgeMove(const FObjectInitializer& ObjectInitializer);

    // 方向在激活时归一化并固定；外部输入或镜头变化不会重定向本次移动。
    static UETAbilityTask_DodgeMove* CreateDodgeMove(UGameplayAbility* OwningAbility,
                                                     const FVector& WorldDirection, float Distance,
                                                     float Duration);
    static bool IsConfigurationValid(float Distance, float Duration);
    // 同时核对LocalID和Source对象，避免将外部来源或复用ID误认成本次移动。
    bool IsMovementReady() const;

    UPROPERTY(BlueprintAssignable)
    FETDodgeMoveSignal OnCompleted;

    UPROPERTY(BlueprintAssignable)
    FETDodgeMoveSignal OnAborted;

    virtual void Activate() override;
    virtual void TickTask(float DeltaTime) override;

protected:
    virtual void OnDestroy(bool bInOwnerFinished) override;

private:
    friend class FETDodgeMovementTest;

    // 所有权：弱引用Movement，持有自己的Source；结束只移除对应对象。
    UPROPERTY(Transient)
    TWeakObjectPtr<UCharacterMovementComponent> Movement;

    TSharedPtr<FRootMotionSource> Source;
    // 激活快照：方向 / 起点 / 世界时间用于移动与诊断，不追随逐帧输入。
    FVector Direction = FVector::ZeroVector;
    FVector StartLocation = FVector::ZeroVector;
    float MoveDistance = 0.0f;
    float MoveDuration = 0.0f;
    float StartWorldTime = 0.0f;
    // 先置位再释放或广播，使自然完成、取消与销毁路径幂等。
    bool bFinished = false;

    // FinishMove负责单次通知；ReleaseSource也供OnDestroy复用，不广播业务回调。
    void FinishMove(bool bSucceeded, const TCHAR* Reason);
    void ReleaseSource(const TCHAR* Reason);
};