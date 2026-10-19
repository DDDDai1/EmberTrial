#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETPracticeActionComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UAnimMontage;
class UCharacterMovementComponent;
struct FRootMotionSource;

UENUM(BlueprintType)
enum class EETPracticeActionState : uint8
{
    Idle,
    Attacking,
    Dodging,
    HitReact
};

// M0兼容动作与回归入口；正式GAS仍通过Idle / Cancel桥接，未完成蓝图审计前保留。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETPracticeActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // ---- 激活 / 请求 / 只读状态；拒绝输入不排队 ----
    UETPracticeActionComponent();

    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

    /** One request per input Started event. Rejected requests are not queued. */
    UFUNCTION(BlueprintCallable, Category = "M0|Practice Action")
    bool RequestPracticeAction(EETPracticeActionState RequestedState);

    UFUNCTION(BlueprintCallable, Category = "M0|Practice Action")
    void CancelPracticeAction();

    UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
    EETPracticeActionState GetPracticeActionState() const;

    UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
    bool IsPracticeActionIdle() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ---- 既有资源与配置；名称 / Category / 默认值属于蓝图契约 ----
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "M0|Practice Action|Animation",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "M0|Practice Action|Animation",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "M0|Practice Action|Animation",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> HitReactMontage;

    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action", meta = (ClampMin = "0.0"))
    float AttackMaxHorizontalSpeed = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action|Dodge",
              meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "cm"))
    float DodgeDistance = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action|Dodge",
              meta = (ClampMin = "0.05", ClampMax = "0.50", Units = "s"))
    float DodgeDuration = 0.30f;

    // ---- 单次动作身份；旧Serial回调不能结束新动作 ----
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "M0|Practice Action",
              meta = (AllowPrivateAccess = "true"))
    EETPracticeActionState CurrentState = EETPracticeActionState::Idle;

    UPROPERTY(Transient)
    TWeakObjectPtr<ACharacter> CharacterOwner;

    UPROPERTY(Transient)
    TWeakObjectPtr<UAnimInstance> ActiveAnimInstance;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    // 使用精确实例ID，避免同一Montage的淡出实例与下一轮混淆。
    int32 ActiveMontageInstanceID = INDEX_NONE;
    uint32 ActionSerial = 0;
    bool bEndingPlay = false;

    // ---- 本组件拥有的位移源；位移结束与动画解锁相互独立 ----
    UPROPERTY(Transient)
    TWeakObjectPtr<UCharacterMovementComponent> DodgeMovement;

    TSharedPtr<FRootMotionSource> DodgeSource;
    FVector DodgeStartLocation = FVector::ZeroVector;
    float DodgeStartWorldTime = 0.0f;

    // ---- 请求解析 / 回调身份与统一释放 ----
    UAnimMontage* ResolveMontage(EETPracticeActionState State) const;
    bool IsCurrentActionCallback(UAnimMontage* Montage, uint32 Serial) const;
    void ReleaseAction(bool bStopMontage, const TCHAR* Reason);
    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, uint32 Serial);
    bool RejectRequest(EETPracticeActionState RequestedState, const TCHAR* Reason) const;

    // ---- CharacterMovement RMS申请、身份复核与幂等释放 ----
    bool StartDodgeMovement(ACharacter* Character, UCharacterMovementComponent* Movement);
    void StopDodgeMovement(const TCHAR* Reason);
    void UpdateDodgeMovement();
    void HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, uint32 Serial);

#if WITH_DEV_AUTOMATION_TESTS
    // 现有真实动画 / 位移夹具；不增加产品测试开关。
    friend class FETPracticeActionLifecycleTest;
    friend class FETPracticeDodgeMovementTest;
#endif
};
