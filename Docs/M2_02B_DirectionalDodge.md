# M2-02B：方向闪避位移与2键正式迁移

2026-10-15，固定 **UE 5.5.4**。前置：[M2-02A已验收](Reports/M2_02A_Verification.md)。**代码仅写入本指南，尚未改工程Source、资产或执行本批Build / 测试。**实际仍90Source / 46cpp / 来源51项；本批实施后预计93Source / 48cpp，新增来源项预计0，资产哈希待实物验收更新。

预计手动 **100–170分钟**（代码 / 理解约70–115、资产与连线10–20、相关检查10–15、保存记录5–10），构建等待另列。可分两次完成；预算不要求做满，也不提前记为已用工时。全部C++仍由你手动录入；有编译问题发第一条真实错误，我再按当次请求处理。

## 1. 本批结果与分工

按键盘上方2，使用当前移动输入对应的世界方向闪避；没有移动输入时按**角色前方**，不是相机前方。方向在开始时固定，位移期间转镜头或改WASD不改变这一段轨迹。默认150cm / 0.30世界秒 / 一次25体力；斜向先归一化，不多走一倍。

| 情况 | 本批预期 |
| --- | --- |
| W / S / A / D配2 | 对应镜头水平前 / 后 / 左 / 右 |
| W+A等斜向配2 | 固定斜向，仍150cm |
| 松开全部WASD后按2 | 按角色此刻前方，不能沿上一次输入滑走 |
| 位移完成、Montage还没结束 | 原地等待Montage结束，仍Dodging，不提前移动 / 再闪 / 恢复体力 |
| 墙 / 斜墙 / 可走坡面 | CharacterMovement真实碰撞 / 滑动；墙边实际距离可小于150cm |
| 走出可行走地面 | 位移Task中止并取消动作，不悬空持续冲刺 |
| 3 / 真实受击 / 死亡 / EndPlay取消 | 只清本Task持有的Source，不退款、不残留水平速度 |

**本批仍没有无敌。**敌人打中可以扣血并取消；C才接窗口。动画仍复用一段Quinn侧闪占位，不强行转角色对准每个方向，因此前后 / 斜向的姿势匹配只保证占位可辨认，不宣称已完成八方向动画。不同方向的正式美术改善后续安排，不为本批重烘焙。

责任：Character只读当前Enhanced Input并转换世界方向；Dodge Ability仍拥有动作、Montage和成本；新AbilityTask独占本次RMS和移动结束。位移自然结束只停Task，Montage结束才解除Dodging。任务只在活动时Tick，不新增角色 / UI常驻Tick，不新增Timer或全局移动状态。

采用专用Task，是为了沿用M0已验证的末帧比例、所有权核对、离地中止与日志规则。引擎标准ConstantForce Task本机实现按世界EndTime结束、没有本项目的离地中止 / 所有权日志；本批不直接照搬其结束方式。Runtime仍通过CharacterMovement的RMS走真实运动，不每帧SetActorLocation。

启动顺序改为：合法性 → 读取固定方向 → 启动并核对Montage → 准备RMS → CommitAbility一次 → 后续移动Tick。准备失败不扣；若提交期间取消，EndAbility清掉刚准备的RMS。同步调用内还没有运动Tick；本批没有资源回滚或第二套成本。

本机核对了GameplayTasks的EndTask / OnDestroy、RootMotionSource GetByID包含Pending源、EnhancedPlayerInput先计算Action状态再派发委托。方向读取当前MoveAction值，**不依赖Move()是否已执行，也不使用LastMovementInputVector**。官方参考：[Task清理约定](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/Abilities/Tasks/UAbilityTask/OnDestroy?application_version=5.5)、[当前输入查询](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/EnhancedInput/UEnhancedPlayerInput/GetActionValue?lang=en-US)；网页可能显示较新版本，实际签名以本机5.5.4头文件为准。任务结束先复制委托、EndTask，最后广播副本，避免结束后访问已标记清理的Task。

## 2. 文件与操作顺序

Stop PIE → Save All → 关闭UE → CLion保存。共 **三新 / 五旧Source**，不改Build.cs / Target / uproject / UI / 旧Practice与资源实现。GameplayAbilities已公开依赖GameplayTasks，本模块现有依赖足够。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Abilities\Tasks\ETAbilityTask_DodgeMove.h        新
├─ EmberTrial\Private\Abilities\Tasks\ETAbilityTask_DodgeMove.cpp    新
├─ EmberTrial\Tests\ETDodgeMovementTests.cpp                         新
├─ EmberTrial\Public\Abilities\ETGA_Dodge.h                          完整替换
├─ EmberTrial\Private\Abilities\ETGA_Dodge.cpp                       完整替换
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h                局部添加
├─ EmberTrial\Private\Characters\EmberTrialCharacter.cpp             局部添加
└─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp  一处添加
```

Tasks必须在Abilities下；Tests不是Texts。新Task是继承UAbilityTask的原生UCLASS；可直接在CLion指定路径新建h / cpp，不通过UE向导生成ActorComponent。头文件最后一个include必须是本文件的.generated.h，不手建generated文件。测试cpp不是UCLASS。

按第3–6节完成八份文件再Build；第4节仅覆盖Dodge两文件，**不能覆盖整份Character或Validator**。本批不迁移目录，不删除历史M0测试。源码编译通过后先配置 / ke小检查，再按第8节迁移2键；不要提前断开旧2。


## 3. 新建移动Task（两个完整文件）


### 3.1 Public/Abilities/Tasks/ETAbilityTask_DodgeMove.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ETAbilityTask_DodgeMove.generated.h"

class UCharacterMovementComponent;
struct FRootMotionSource;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETDodgeMoveSignal);

UCLASS()
class EMBERTRIAL_API UETAbilityTask_DodgeMove : public UAbilityTask
{
    GENERATED_BODY()

public:
    UETAbilityTask_DodgeMove(const FObjectInitializer& ObjectInitializer);

    static UETAbilityTask_DodgeMove* CreateDodgeMove(UGameplayAbility* OwningAbility,
        const FVector& WorldDirection, float Distance, float Duration);
    static bool IsConfigurationValid(float Distance, float Duration);
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

    UPROPERTY(Transient)
    TWeakObjectPtr<UCharacterMovementComponent> Movement;

    TSharedPtr<FRootMotionSource> Source;
    FVector Direction = FVector::ZeroVector;
    FVector StartLocation = FVector::ZeroVector;
    float MoveDistance = 0.0f;
    float MoveDuration = 0.0f;
    float StartWorldTime = 0.0f;
    bool bFinished = false;

    void FinishMove(bool bSucceeded, const TCHAR* Reason);
    void ReleaseSource(const TCHAR* Reason);
};
```


### 3.2 Private/Abilities/Tasks/ETAbilityTask_DodgeMove.cpp

```cpp
#include "Abilities/Tasks/ETAbilityTask_DodgeMove.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETDodgeMove, Log, All);

UETAbilityTask_DodgeMove::UETAbilityTask_DodgeMove(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bTickingTask = true;
}

UETAbilityTask_DodgeMove* UETAbilityTask_DodgeMove::CreateDodgeMove(
    UGameplayAbility* OwningAbility, const FVector& WorldDirection, float Distance, float Duration)
{
    UETAbilityTask_DodgeMove* Task = NewAbilityTask<UETAbilityTask_DodgeMove>(OwningAbility);
    Task->Direction = WorldDirection;
    Task->MoveDistance = Distance;
    Task->MoveDuration = Duration;
    return Task;
}

bool UETAbilityTask_DodgeMove::IsConfigurationValid(float Distance, float Duration)
{
    return FMath::IsFinite(Distance) && Distance >= 1.0f && Distance <= 300.0f &&
        FMath::IsFinite(Duration) && Duration >= 0.05f && Duration <= 0.50f;
}

bool UETAbilityTask_DodgeMove::IsMovementReady() const
{
    UCharacterMovementComponent* Move = Movement.Get();
    return !bFinished && IsValid(Move) && Source.IsValid() &&
        Move->GetRootMotionSourceByID(Source->LocalID) == Source;
}

void UETAbilityTask_DodgeMove::Activate()
{
    Super::Activate();
    ACharacter* Body = Cast<ACharacter>(GetAvatarActor());
    UCharacterMovementComponent* Move = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    if (!IsValid(Body) || !Body->HasAuthority() || Body->IsActorBeingDestroyed() ||
        !Body->GetWorld() || Body->GetWorld()->GetNetMode() != NM_Standalone ||
        UGameplayStatics::IsGamePaused(Body) || !IsValid(Move) ||
        !Move->IsMovingOnGround() || Move->HasRootMotionSources() ||
        !IsConfigurationValid(MoveDistance, MoveDuration) || Direction.ContainsNaN())
    {
        FinishMove(false, TEXT("InvalidStart"));
        return;
    }
    Direction = Direction.GetSafeNormal2D();
    if (Direction.IsNearlyZero())
    {
        FinishMove(false, TEXT("InvalidDirection"));
        return;
    }
    TSharedPtr<FRootMotionSource_ConstantForce> Candidate = MakeShared<FRootMotionSource_ConstantForce>();
    Candidate->InstanceName = FName(TEXT("ETFormalDodge"));
    Candidate->Priority = 500;
    Candidate->AccumulateMode = ERootMotionAccumulateMode::Override;
    Candidate->bInLocalSpace = false;
    Candidate->Force = Direction * (MoveDistance / MoveDuration);
    Candidate->Duration = MoveDuration;
    Candidate->Settings.UnSetFlag(ERootMotionSourceSettingsFlags::DisablePartialEndTick);
    Candidate->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
    Candidate->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
    Candidate->FinishVelocityParams.ClampVelocity = 0.0f;
    const uint16 ID = Move->ApplyRootMotionSource(Candidate);
    if (ID == static_cast<uint16>(ERootMotionSourceID::Invalid))
    {
        FinishMove(false, TEXT("SourceRejected"));
        return;
    }
    Movement = Move;
    Source = Candidate;
    StartLocation = Body->GetActorLocation();
    StartWorldTime = GetWorld()->GetTimeSeconds();
    UE_LOG(LogETDodgeMove, Display,
        TEXT("[M2-02B] Move begin Body=%s ID=%u Distance=%.1f Duration=%.3f Direction=%s"),
        *GetNameSafe(Body), ID, MoveDistance, MoveDuration, *Direction.ToCompactString());
}

void UETAbilityTask_DodgeMove::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);
    if (bFinished || UGameplayStatics::IsGamePaused(GetAvatarActor())) { return; }
    UCharacterMovementComponent* Move = Movement.Get();
    if (!IsValid(Move) || !Source.IsValid())
    {
        FinishMove(false, TEXT("MovementLost"));
        return;
    }
    // The movement component may already have removed a naturally finished source.
    if (Source->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
    {
        FinishMove(true, TEXT("WindowEnded"));
    }
    else if (!Move->IsMovingOnGround())
    {
        FinishMove(false, TEXT("LeftGround"));
    }
    else if (Source->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval) ||
        Move->GetRootMotionSourceByID(Source->LocalID) != Source)
    {
        FinishMove(false, TEXT("SourceRemoved"));
    }
}

void UETAbilityTask_DodgeMove::ReleaseSource(const TCHAR* Reason)
{
    if (!Source.IsValid()) { return; }
    UCharacterMovementComponent* Move = Movement.Get();
    const ACharacter* Body = IsValid(Move) ? Move->GetCharacterOwner() : nullptr;
    const uint16 ID = Source->LocalID;
    const float SourceTime = Source->GetTime();
    const float Travel = IsValid(Body) ? FVector::Dist2D(StartLocation, Body->GetActorLocation()) : 0.0f;
    const float Elapsed = GetWorld() ? GetWorld()->GetTimeSeconds() - StartWorldTime : 0.0f;
    if (IsValid(Move))
    {
        const bool bOwned = Move->GetRootMotionSourceByID(ID) == Source;
        if (bOwned) { Move->RemoveRootMotionSourceByID(ID); }
        if (bOwned || !Move->HasRootMotionSources())
        {
            Move->Velocity.X = 0.0f;
            Move->Velocity.Y = 0.0f;
        }
    }
    Source.Reset();
    Movement.Reset();
    UE_LOG(LogETDodgeMove, Display,
        TEXT("[M2-02B] Move end Body=%s ID=%u Reason=%s Travel2D=%.2f SourceTime=%.4f Elapsed=%.3f"),
        *GetNameSafe(Body), ID, Reason, Travel, SourceTime, Elapsed);
}

void UETAbilityTask_DodgeMove::FinishMove(bool bSucceeded, const TCHAR* Reason)
{
    if (bFinished) { return; }
    bFinished = true;
    ReleaseSource(Reason);
    // EndTask marks this UObject as garbage. Copy the callback before ending it.
    const bool bNotify = ShouldBroadcastAbilityTaskDelegates();
    FETDodgeMoveSignal Callback = bSucceeded ? OnCompleted : OnAborted;
    EndTask();
    if (bNotify) { Callback.Broadcast(); }
}

void UETAbilityTask_DodgeMove::OnDestroy(bool bInOwnerFinished)
{
    bFinished = true;
    ReleaseSource(bInOwnerFinished ? TEXT("AbilityEnd") : TEXT("TaskEnd"));
    Super::OnDestroy(bInOwnerFinished);
}
```


## 4. 完整替换Dodge Ability（两个既有文件）

A的成本Schema、CheckCost / 幂等ApplyCost、Tag和Montage回调全部保留。新增Movement配置与Task，不把位移完成回调接到正常EndAbility。


### 4.1 Public/Abilities/ETGA_Dodge.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
class UETAbilityTask_DodgeMove;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Dodge : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Dodge();
    static bool ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost);
    bool HasValidMovementConfiguration() const;

protected:
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    friend class FETDodgeCostContractTest;
    friend class FETDodgeMovementTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge")
    TObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge",
        meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "cm"))
    float DodgeDistance = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge",
        meta = (ClampMin = "0.05", ClampMax = "0.50", Units = "s"))
    float DodgeDuration = 0.30f;

    UPROPERTY(Transient)
    TObjectPtr<UETAbilityTask_DodgeMove> MoveTask;

    uint32 ActivationSerial = 0;
    // UE declares ApplyCost const. This flag belongs only to the current attempt.
    mutable bool bCostSubmitted = false;

    UFUNCTION()
    void HandleMontageCompleted();
    UFUNCTION()
    void HandleMontageAborted();

    UFUNCTION()
    void HandleMoveCompleted();
    UFUNCTION()
    void HandleMoveAborted();
};
```


### 4.2 Private/Abilities/ETGA_Dodge.cpp

```cpp
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/ETAbilityTask_DodgeMove.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETDodge, Log, All);

UETGA_Dodge::UETGA_Dodge()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Dodge);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
}

bool UETGA_Dodge::ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost)
{
    OutCost = 0.0f;
    if (!IsValid(Effect) || !FMath::IsFinite(Level) || Level < 1.0f ||
        Effect->DurationPolicy != EGameplayEffectDurationType::Instant ||
        Effect->Modifiers.Num() != 1 || Effect->Executions.Num() != 0)
    {
        return false;
    }
    const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
    float Magnitude = 0.0f;
    if (Modifier.Attribute != UETStaminaAttributeSet::GetStaminaAttribute() ||
        Modifier.ModifierOp != EGameplayModOp::AddBase ||
        Modifier.ModifierMagnitude.GetMagnitudeCalculationType() !=
            EGameplayEffectMagnitudeCalculation::ScalableFloat ||
        !Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(Level, Magnitude) ||
        !FMath::IsFinite(Magnitude) || Magnitude > -1.0f || Magnitude < -100.0f)
    {
        return false;
    }
    OutCost = -Magnitude;
    return true;
}

bool UETGA_Dodge::HasValidMovementConfiguration() const
{
    return UETAbilityTask_DodgeMove::IsConfigurationValid(DodgeDistance, DodgeDuration) &&
        IsValid(DodgeMontage) && !DodgeMontage->HasRootMotion() &&
        FMath::IsFinite(DodgeMontage->GetPlayLength()) && DodgeMontage->GetPlayLength() > DodgeDuration;
}

bool UETGA_Dodge::CheckCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    if (!IsValid(ASC) || !IsValid(Body) || Body->IsActorBeingDestroyed() ||
        !Body->HasAuthority() || Body->IsCombatDead() ||
        ActorInfo->OwnerActor.Get() != Body || Body->GetAbilitySystemComponent() != ASC ||
        !Body->GetWorld() || Body->GetWorld()->GetNetMode() != NM_Standalone ||
        UGameplayStatics::IsGamePaused(Body) ||
        !Body->GetGameplayStaminaSnapshot(Stamina, MaxStamina) ||
        !FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina) ||
        MaxStamina < 1.0f || Stamina < 0.0f || Stamina > MaxStamina ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost) ||
        Stamina < Cost)
    {
        return false;
    }
    return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UETGA_Dodge::ApplyCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    if (bCostSubmitted || !CheckCost(Handle, ActorInfo, nullptr)) { return; }
    bCostSubmitted = true; // Guard BEFORE the GE can synchronously notify observers.
    Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

bool UETGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !CheckCost(Handle, ActorInfo, OptionalRelevantTags) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    APlayerController* PC = IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UCharacterMovementComponent* Movement = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice = IsValid(Body)
        ? Body->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    return IsValid(Body) && Body->GetCombatTeam() == EETCombatTeam::Player &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Body && !PC->IsMoveInputIgnored() &&
        Body->IsFormalActionIdle() && (!Practice || Practice->IsPracticeActionIdle()) &&
        IsValid(Movement) && Movement->IsMovingOnGround() && !Movement->HasRootMotionSources() &&
        !Body->GetVelocity().ContainsNaN() && HasValidMovementConfiguration() &&
        !Body->GetDodgeWorldDirection().IsNearlyZero() &&
        IsValid(Anim) && !Anim->GetCurrentActiveMontage();
}

void UETGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    const uint32 Serial = ++ActivationSerial;
    bCostSubmitted = false;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    const FVector Direction = IsValid(Body) ? Body->GetDodgeWorldDirection() : FVector::ZeroVector;
    UCharacterMovementComponent* Movement = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    if (!IsValid(Body) || !IsValid(ASC) || !HasValidMovementConfiguration() || Body->IsCombatDead() ||
        Direction.IsNearlyZero() || !IsValid(Movement) || !Movement->IsMovingOnGround() ||
        Movement->HasRootMotionSources() ||
        !Body->GetGameplayStaminaSnapshot(Before, MaxStamina) ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    Movement->StopMovementImmediately();
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, DodgeMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!MontageTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->ReadyForActivation();
    if (Serial != ActivationSerial || !IsActive()) { return; }
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    if (!IsValid(Anim) || !Anim->GetActiveInstanceForMontage(DodgeMontage) ||
        ASC->GetAnimatingAbility() != this || !Anim->Montage_IsPlaying(DodgeMontage))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    // Prepare our source before charging. No movement tick runs inside this synchronous call.
    MoveTask = UETAbilityTask_DodgeMove::CreateDodgeMove(this, Direction, DodgeDistance, DodgeDuration);
    if (!MoveTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MoveTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMoveCompleted);
    MoveTask->OnAborted.AddDynamic(this, &ThisClass::HandleMoveAborted);
    MoveTask->ReadyForActivation();
    if (Serial != ActivationSerial || !IsActive()) { return; }
    if (!IsValid(MoveTask) || !MoveTask->IsMovementReady())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    // Montage and source exist. Commit once; cancellation cleans the prepared source.
    const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
    if (Serial != ActivationSerial || !IsActive()) { return; }
    float After = 0.0f;
    if (!bCommitted || !bCostSubmitted || Body->IsCombatDead() ||
        !Body->GetGameplayStaminaSnapshot(After, MaxStamina) ||
        !FMath::IsNearlyEqual(Before - After, Cost, 0.001f))
    {
        UE_LOG(LogETDodge, Warning, TEXT("[M2-02A] Commit failed Body=%s Submitted=%d"),
            *GetNameSafe(Body), bCostSubmitted ? 1 : 0);
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] Begin Body=%s Serial=%u Before=%.1f After=%.1f Cost=%.1f Dodging=%d"),
        *GetNameSafe(Body), Serial, Before, After, Cost, ASC->GetTagCount(ETGameplayTags::State_Dodging));
}

void UETGA_Dodge::HandleMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_Dodge::HandleMontageAborted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UETGA_Dodge::HandleMoveCompleted()
{
    MoveTask = nullptr; // Montage still owns action lifetime and State.Dodging.
}

void UETGA_Dodge::HandleMoveAborted()
{
    MoveTask = nullptr;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UETGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }
    ++ActivationSerial;
    if (IsValid(MontageTask))
    {
        MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
        MontageTask->OnCancelled.RemoveAll(this);
    }
    MontageTask = nullptr;
    if (IsValid(MoveTask))
    {
        MoveTask->OnCompleted.RemoveAll(this);
        MoveTask->OnAborted.RemoveAll(this);
    }
    MoveTask = nullptr; // Super ends active tasks; their OnDestroy releases their own source.
    const FString BodyName = GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    const TWeakObjectPtr<UAbilitySystemComponent> ASC = ActorInfo
        ? ActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    const bool bSubmitted = bCostSubmitted;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] End Body=%s Cancelled=%d CostSubmitted=%d Dodging=%d"),
        *BodyName, bWasCancelled ? 1 : 0, bSubmitted ? 1 : 0,
        ASC.IsValid() ? ASC->GetTagCount(ETGameplayTags::State_Dodging) : 0);
}
```


## 5. Character与校验器局部添加

本节保留其余原代码，只在指定锚点添加；不删除IsCombatDead等已有函数实现。


### 5.1 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

在头文件已有friend class FETStaminaFoundationTest后加一行friend；两行都保留。仅供测试实例设置阵营，不修改资产。

找到这段（应只有一处）：

```cpp
friend class FETStaminaFoundationTest;
```

替换为：

```cpp
friend class FETStaminaFoundationTest;
    friend class FETDodgeMovementTest;
```


### 5.2 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

在public区域既有RequestDodge声明后加两个普通C++函数声明；不要把UFUNCTION标记复制到这两个函数上。

找到这段（应只有一处）：

```cpp
bool RequestDodge();
```

替换为：

```cpp
bool RequestDodge();

    FVector GetDodgeWorldDirection() const;
    static FVector MakeDodgeWorldDirection(const FVector2D& Axis, float ControlYaw, const FVector& Forward);
```


### 5.3 `EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

在cpp原EnhancedInputSubsystems.h include后新增两条include，其余不动。

找到这段（应只有一处）：

```cpp
#include "EnhancedInputSubsystems.h"
```

替换为：

```cpp
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
```


### 5.4 `EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

在cpp完整RequestDodge函数的结束大括号之后、GrantLightAttackAbility函数之前插入下方两个完整函数。下方最后一行GrantLightAttackAbility只是定位锚点；对照旧行整体替换可避免重复写函数头。

找到这段（应只有一处）：

```cpp
void AEmberTrialCharacter::GrantLightAttackAbility()
```

替换为：

```cpp
FVector AEmberTrialCharacter::MakeDodgeWorldDirection(
    const FVector2D& Axis, float ControlYaw, const FVector& Forward)
{
    if (!FMath::IsFinite(Axis.X) || !FMath::IsFinite(Axis.Y) ||
        !FMath::IsFinite(ControlYaw) || Forward.ContainsNaN())
    {
        return FVector::ZeroVector;
    }
    if (Axis.SizeSquared() <= FMath::Square(0.1f))
    {
        return Forward.GetSafeNormal2D();
    }
    const FRotationMatrix Yaw(FRotator(0.0f, ControlYaw, 0.0f));
    return (Yaw.GetUnitAxis(EAxis::X) * Axis.Y + Yaw.GetUnitAxis(EAxis::Y) * Axis.X).GetSafeNormal2D();
}

FVector AEmberTrialCharacter::GetDodgeWorldDirection() const
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    const UEnhancedPlayerInput* Input = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
    FVector2D Axis = FVector2D::ZeroVector;
    if (IsValid(Input) && IsValid(MoveAction) && MoveAction->ValueType == EInputActionValueType::Axis2D)
    {
        Axis = Input->GetActionValue(MoveAction).Get<FVector2D>();
    }
    const float Yaw = PC ? PC->GetControlRotation().Yaw : GetActorRotation().Yaw;
    return MakeDodgeWorldDirection(Axis, Yaw, GetActorForwardVector());
}

void AEmberTrialCharacter::GrantLightAttackAbility()
```


### 5.5 `EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`

在Validator的ValidateAbility内，找到Defaults->IsA<UETGA_Dodge>()分支；在既有Cost GE检查结束后、该分支return前加入移动配置检查。不要放到后面的LightAttack分支。

找到这段（应只有一处）：

```cpp
AddIssue(Blueprint, TEXT("ET-DODGE-001"), TEXT("Invalid Dodge Cost GE; use one negative fixed Stamina modifier."), Issues);
        }
        return;
```

替换为：

```cpp
AddIssue(Blueprint, TEXT("ET-DODGE-001"), TEXT("Invalid Dodge Cost GE; use one negative fixed Stamina modifier."), Issues);
        }
        if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidMovementConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-002"),
                TEXT("Dodge requires finite Distance 1..300, Duration .05...50, and an in-place montage longer than Duration."), Issues);
        }
        return;
```


## 6. 新建长期移动自动化测试

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETDodgeMovementTests.cpp`。完整保存下方内容；保留原CostContract与两资源测试。

新测试在临时世界真正TryActivate原生能力，播放已有Montage、应用真实成本GE、运行CharacterMovement；三步长、暂停 / 取消 / 死亡 / EndPlay的时点由测试驱动。方向数学检查与真实运动分别断言，不把键盘输入或蓝图连线冒充自动覆盖。不会修改GA资产或CDO，不生成测试场景。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/ETAbilityTask_DodgeMove.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETDodgeMovementTest,
    "EmberTrial.M2.Dodge.Movement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETDodgeMovementTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    // Direction conversion is independent of this frame's Move callback ordering.
    for (const FVector2D Axis : {FVector2D(0,1), FVector2D(0,-1), FVector2D(1,0), FVector2D(-1,0),
        FVector2D(1,1), FVector2D(-1,1), FVector2D(1,-1), FVector2D(-1,-1)})
    {
        const FVector Expected = FVector(Axis.Y, Axis.X, 0.0f).GetSafeNormal2D();
        const FVector Result = AEmberTrialCharacter::MakeDodgeWorldDirection(Axis, 0.0f, FVector::ForwardVector);
        TestTrue(TEXT("Eight input directions normalize correctly"), Result.Equals(Expected, 0.0001f));
        const FVector Rotated = AEmberTrialCharacter::MakeDodgeWorldDirection(Axis, 90.0f, FVector::ForwardVector);
        TestTrue(TEXT("Control yaw rotates input into world space"),
            Rotated.Equals(FRotator(0,90,0).RotateVector(Expected), 0.0001f));
    }
    TestTrue(TEXT("No input uses body forward, not camera forward"),
        AEmberTrialCharacter::MakeDodgeWorldDirection(FVector2D::ZeroVector, 90.0f, FVector::ForwardVector)
        .Equals(FVector::ForwardVector, 0.0001f));
    TestTrue(TEXT("Default movement configuration"), UETAbilityTask_DodgeMove::IsConfigurationValid(150,0.30f));
    TestFalse(TEXT("Zero duration rejected"), UETAbilityTask_DodgeMove::IsConfigurationValid(150,0));
    TestFalse(TEXT("Excessive distance rejected"), UETAbilityTask_DodgeMove::IsConfigurationValid(301,0.30f));

    UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr,
        TEXT("/Game/EmberTrial/Animations/Practice/Montages/AM_M0_Dodge_01_Montage.AM_M0_Dodge_01_Montage"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UClass* CostClass = LoadClass<UGameplayEffect>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost.GE_ET_DodgeCost_C"));
    if (!TestNotNull(TEXT("Saved dodge montage"), Montage) || !TestNotNull(TEXT("Quinn"), Quinn) ||
        !TestNotNull(TEXT("Saved formal cost GE"), CostClass)) { return false; }

    for (const int32 Rate : {30,60,120})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this); return false; }
        UWorld* World = Fixture.GetTestWorld();
        auto Block = [World](const FVector& Center, const FVector& HalfSize)
        {
            AActor* Actor = World->SpawnActor<AActor>();
            if (!Actor) { return Actor; }
            UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
            Actor->AddInstanceComponent(Box);
            Actor->SetRootComponent(Box);
            Box->SetMobility(EComponentMobility::Static);
            Box->SetBoxExtent(HalfSize);
            Box->SetRelativeLocation(Center);
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->RegisterComponent();
            return Actor;
        };
        if (!TestNotNull(TEXT("Real floor"), Block(FVector(0,0,-50), FVector(2000,2000,50)))) { return false; }
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(FVector(0,0,200), FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Native body"), Body) || !TestNotNull(TEXT("Standalone controller"), PC)) { return false; }
        Body->CombatTeam = EETCombatTeam::Player; // Transient fixture only; no asset/CDO writes.
        USkeletalMeshComponent* Mesh = Body->GetMesh();
        Mesh->SetSkeletalMeshAsset(Quinn);
        Mesh->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        PC->Possess(Body);
        UCharacterMovementComponent* Move = Body->GetCharacterMovement();
        Move->bRunPhysicsWithNoController = true;
        if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
        const float Step = 1.0f / static_cast<float>(Rate);
        auto Advance = [&Fixture,Step,Rate](float Seconds)
        {
            const int32 Count = FMath::Max(1, FMath::RoundToInt(Seconds * Rate));
            for (int32 Index=0; Index<Count; ++Index)
            {
                if (!Fixture.TickTestWorld(Step)) { return false; }
            }
            return true;
        };
        Move->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) || !TestTrue(TEXT("Landed on floor"), Move->IsMovingOnGround()) ||
            !TestTrue(TEXT("Ready and possessed"), Body->IsGameplayReady() && PC->GetPawn()==Body)) { return false; }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(UETGA_Dodge::StaticClass(),1));
        FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
        UETGA_Dodge* Ability = Spec ? Cast<UETGA_Dodge>(Spec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Granted native ability instance"), Ability)) { return false; }
        Ability->DodgeMontage = Montage;
        Ability->CostGameplayEffectClass = CostClass;
        const FGameplayAttribute Stamina = UETStaminaAttributeSet::GetStaminaAttribute();
        auto Refill = [ASC,Stamina]() { ASC->SetNumericAttributeBase(Stamina,100.0f); };
        auto Start = [this,ASC,Handle,Ability]()
        {
            return TestTrue(TEXT("Full GAS activation"), ASC->TryActivateAbility(Handle,false)) &&
                TestTrue(TEXT("Ability and owned movement are active"), Ability->IsActive() &&
                    IsValid(Ability->MoveTask) && Ability->MoveTask->IsMovementReady());
        };
        const FVector StartAt = Body->GetActorLocation();
        const FVector Direction = Body->GetActorForwardVector().GetSafeNormal2D();
        if (!Start()) { return false; }
        TSharedPtr<FRootMotionSource> Source = Ability->MoveTask->Source;
        TestEqual(TEXT("Full activation pays once"), ASC->GetNumericAttribute(Stamina),75.0f);
        TestFalse(TEXT("Busy duplicate is rejected"), ASC->TryActivateAbility(Handle,false));
        TestEqual(TEXT("Busy duplicate pays nothing"), ASC->GetNumericAttribute(Stamina),75.0f);
        if (!Advance(0.1f)) { return false; }
        PC->SetControlRotation(FRotator(0,90,0));
        Body->SetActorRotation(FRotator(0,90,0));
        if (!Advance(0.3f)) { return false; }
        const FVector StoppedAt = Body->GetActorLocation();
        TestTrue(TEXT("Endpoint follows cached forward within 1 cm"),
            FVector::Dist2D(StoppedAt,StartAt+Direction*150.0f)<=1.0f);
        TestTrue(TEXT("Source time is .30 seconds"), FMath::IsNearlyEqual(Source->GetTime(),0.30f,0.002f));
        TestTrue(TEXT("Movement completes before montage action"), Ability->IsActive() && !Ability->MoveTask &&
            ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        TestEqual(TEXT("Recovery remains blocked after movement ends"), ASC->GetNumericAttribute(Stamina),75.0f);
        if (!Advance(0.6f)) { return false; }
        TestFalse(TEXT("Montage later ends ability"), Ability->IsActive());
        TestFalse(TEXT("No leftover tag"), ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        TestFalse(TEXT("No leftover source"), Move->HasRootMotionSources());
        TestTrue(TEXT("No drift"), FVector::Dist2D(StoppedAt,Body->GetActorLocation())<=1.0f && Move->Velocity.Size2D()<=1.0f);
        AddInfo(FString::Printf(TEXT("[M2-02B] Rate=%d Travel2D=%.2f SourceTime=%.4f"),
            Rate,FVector::Dist2D(StartAt,StoppedAt),Source->GetTime()));

        if (Rate==60)
        {
            Refill();
            Ability->DodgeDuration=0.0f;
            TestFalse(TEXT("Invalid config rejects before payment"), ASC->TryActivateAbility(Handle,false));
            TestEqual(TEXT("Invalid config is free"), ASC->GetNumericAttribute(Stamina),100.0f);
            Ability->DodgeDuration=0.30f;
            TSharedPtr<FRootMotionSource_ConstantForce> Foreign=MakeShared<FRootMotionSource_ConstantForce>();
            Foreign->InstanceName=FName(TEXT("ETDodgeForeignFixture"));
            Foreign->Duration=1.0f;
            Foreign->Force=FVector::ZeroVector;
            const uint16 ForeignID=Move->ApplyRootMotionSource(Foreign);
            TestFalse(TEXT("Foreign source rejects before payment"), ASC->TryActivateAbility(Handle,false));
            TestTrue(TEXT("Foreign source is not removed"), Move->GetRootMotionSourceByID(ForeignID)==Foreign);
            TestEqual(TEXT("Foreign source rejection is free"), ASC->GetNumericAttribute(Stamina),100.0f);
            Move->RemoveRootMotionSourceByID(ForeignID);
            if (!Advance(Step)) { return false; }

            if (!Start() || !Advance(0.1f)) { return false; }
            const FVector CancelAt=Body->GetActorLocation();
            ASC->CancelAbilityHandle(Handle);
            TestTrue(TEXT("Cancel immediately clears movement owner"), Ability->MoveTask==nullptr);
            if (!Advance(0.4f)) { return false; }
            TestTrue(TEXT("Cancel stops partial displacement"), FVector::Dist2D(CancelAt,Body->GetActorLocation())<=1.0f);
            TestFalse(TEXT("Cancel drains source"), Move->HasRootMotionSources());
            TestEqual(TEXT("Cancel does not refund"), ASC->GetNumericAttribute(Stamina),75.0f);

            // Reset the fixture position, not the runtime dodge implementation.
            Body->SetActorLocation(FVector(0,0,Body->GetActorLocation().Z),false,nullptr,ETeleportType::TeleportPhysics);
            Body->SetActorRotation(FRotator::ZeroRotator);
            PC->SetControlRotation(FRotator::ZeroRotator);
            AActor* Wall=Block(FVector(120,0,100),FVector(10,500,200));
            if (!TestNotNull(TEXT("Blocking wall"), Wall)) { return false; }
            Refill();
            if (!Start() || !Advance(1.0f)) { return false; }
            TestTrue(TEXT("Capsule stops before wall, no teleport through it"),
                Body->GetActorLocation().X>1.0f && Body->GetActorLocation().X<=70.0f);
            TestFalse(TEXT("Blocked movement still cleans source"), Move->HasRootMotionSources());
            Wall->Destroy();
            if (!Advance(Step)) { return false; }

            Refill();
            const FVector PauseStart=Body->GetActorLocation();
            if (!Start() || !Advance(0.1f)) { return false; }
            TSharedPtr<FRootMotionSource> PausedSource=Ability->MoveTask->Source;
            const FVector PauseAt=Body->GetActorLocation();
            const float PauseTime=PausedSource->GetTime();
            const float MontageTime=Mesh->GetAnimInstance()->Montage_GetPosition(Montage);
            APlayerState* Pauser=World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Pause owner"), Pauser)) { return false; }
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            const bool bPausedTicks=Advance(0.5f);
            TestTrue(TEXT("Pause freezes location"), FVector::Dist(PauseAt,Body->GetActorLocation())<=0.01f);
            TestTrue(TEXT("Pause freezes source"), FMath::IsNearlyEqual(PauseTime,PausedSource->GetTime(),0.0001f));
            TestTrue(TEXT("Pause freezes montage"), FMath::IsNearlyEqual(MontageTime,Mesh->GetAnimInstance()->Montage_GetPosition(Montage),0.0001f));
            World->GetWorldSettings()->SetPauserPlayerState(nullptr); // Restore even if assertions failed.
            if (!bPausedTicks || !Advance(1.0f)) { return false; }
            TestTrue(TEXT("Resume completes original 150 cm"),
                FMath::Abs(FVector::Dist2D(PauseStart,Body->GetActorLocation())-150.0f)<=1.0f);
            TestFalse(TEXT("Resume leaves no source"), Move->HasRootMotionSources());
            TestFalse(TEXT("Resume ends action"), Ability->IsActive());

            Refill();
            if (!Start() || !Advance(0.1f)) { return false; }
            const FVector DeathAt=Body->GetActorLocation();
            ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(),0.0f);
            TestTrue(TEXT("Real health loss seals death"), Body->IsCombatDead());
            TestFalse(TEXT("Death cancels active ability"), Ability->IsActive());
            TestTrue(TEXT("Death releases task owner"), Ability->MoveTask==nullptr);
            if (!Advance(0.4f)) { return false; }
            TestTrue(TEXT("Death leaves no displacement"), FVector::Dist2D(DeathAt,Body->GetActorLocation())<=1.0f);
            TestFalse(TEXT("Death drains source"), Move->HasRootMotionSources());
        }
        else
        {
            Refill();
            if (!Start() || !Advance(0.1f)) { return false; }
        }
        // At 30/120, EndPlay happens while the actual source is moving.
        if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this); return false; }
        TestFalse(TEXT("EndPlay cancels ability"), Ability->IsActive());
        TestTrue(TEXT("EndPlay releases owned task"), Ability->MoveTask==nullptr);
        TestFalse(TEXT("EndPlay clears Dodging"), ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        Fixture.ForwardErrorMessages(this);
        if (HasAnyErrors()) { return false; }
    }
    return !HasAnyErrors();
}

#endif
```


## 7. 正常Build与索引

确认三新 / 五旧共八文件全部保存，UE仍关闭。PowerShell：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

它先正常Editor Build，再刷新CLion CMake；成功后在CLion Reload CMake，三个CodeModel应共 **48cpp**（Core32 / UI13 / Editor3）。新Task与测试属于EmberTrialCodeModel；不以CMake成功替代UBT构建。随后执行独立Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标成功后打开UE工程。不要用Live Coding替代这次新UCLASS / 属性构建，不清缓存 / 全删Intermediate。若失败，发第一条UHT / C++ / linker错误及前后10–20行；末尾UBT exit6不是根因。

## 8. 配置与2键迁移（无新资产）

### 8.1 配置GA并先用ke确认新位移

1. 打开`/Game/EmberTrial/Abilities/GA_ET_Dodge`，Class Defaults，Details搜索`Dodge`。不是角色组件PracticeAction的同名字段。
2. 确认Dodge Montage仍为`AM_M0_Dodge_01_Montage`，新增 **Dodge Distance=150.0、Dodge Duration=0.30**；Costs的Cost Gameplay Effect Class仍GE_ET_DodgeCost。EventGraph保持空。
3. 本Montage应是原地动画，无启用动画Root Motion的Sequence；不修改既有动画。新增原生检查会拒绝具有动画Root Motion的Montage，避免与Task双重位移。
4. Compile → Save → Content Browser右键GA → Asset Actions → Validate Assets，应有本项目Valid / Issues0。新规则ET-DODGE-002检查范围、时长及原地Montage；不要求再建Probe。
5. Play，在Entry空地松开WASD，Output Log执行：

   ```text
   ke BP_ThirdPersonCharacter_C RequestDodge
   ```

   应从原地动画变成**角色前方150cm**，日志Move begin → A Begin扣25 → Move end → A End。Move end的SourceTime约0.3000，平地Travel2D约150；Elapsed受Tick顺序影响，不要求恰好0.300。Montage结束晚于Move end是正确结果。若本步仍原地，先停在这里发日志，不迁移2键。

### 8.2 只改IA_PracticeDodge的Started执行线

1. Stop PIE，打开`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` → EventGraph。
2. 在图里Ctrl+F，搜索 **IA_PracticeDodge**，双击结果定位事件。找的是Enhanced Input Action事件，拥有Started / Triggered / Completed输出；不是新建Keyboard 2事件。
3. 当前旧链应为：`Started → Branch(Is Formal Action Idle, Self) → True → Request Practice Action(Target=PracticeAction, Dodging)`。先认清这条白线，其他1 / 3 / 4 / E / P不动。
4. 按住Alt单击IA_PracticeDodge事件的**Started白色输出针脚**，断开它到旧Branch的执行线。只断这条事件的Started输出，不去断E的公共门控，也不删PracticeAction组件。
5. 在事件右边空白处右键，保持Context Sensitive，搜索 **Request Dodge**。选择继承自EmberTrialCharacter的原生函数；新节点 **Target=Self**。它不是Request Practice Action，Target不能接PracticeAction组件。
6. 从 **IA_PracticeDodge.Started** 白色输出，拖到新 **Request Dodge** 左侧白色执行输入。Return Value布尔输出可以留空；日志已记录Accepted，不增加Print / Delay / Gate / 重试。
7. 最终连线仅为：

   ```text
   IA_PracticeDodge
       Started ──→ Request Dodge
                       Target = Self
                       Return Value = 不连接
       Triggered / Ongoing / Completed / Canceled = 不接任何闪避调用
   ```

8. 原旧Branch / Dodging请求如仅服务这条2路线，可以删除已孤立节点及该链独用的PracticeAction Get；如你不确定某个查询是否被E共用，保留它但不接任何执行线。最终必须没有从IA_PracticeDodge任何执行输出到旧Request Practice Action(Dodging)的可达路线。
9. Compile → Save。My Blueprint中保留原交互 / 提示函数和组件，不改其他事件。

本批复用名字带Practice的IA资产，避免重命名造成引用 / Redirector额外工作；当前职责已是正式GAS输入。**名字保留不代表仍调用M0。**不增加第二份Action或另一条键盘2事件，不把Started换成Triggered。

### 8.3 检查输入配置与保存

1. 打开`/Game/EmberTrial/Input/Actions/IA_PracticeDodge`，Value Type保持Boolean，Trigger When Paused / 在暂停时触发=False；原Triggers / Modifiers不另加Hold或Pulse。若原来就是该值，无需制造保存差异。
2. 玩家当前Default Mapping Context仍是既有IMC_Default；其中IA_PracticeDodge保持键盘**上方2**一行，不复制第二行，不改WASD / 鼠标 / E映射。暂停P已迁出旧IMC，不能补回。
3. BP_ThirdPersonCharacter Class Defaults确认Dodge Ability Class仍GA_ET_Dodge；PracticeAction原三Montage / Auto Activate / 150 / .30保持，供原型历史用途，不是正式GA的移动配置。
4. GA与玩家BP各Compile → Save → 项目Validate Assets，两份Valid / Issues0即可；Cost GE未改，不重做六资产全选。Save All后再开始检查。

## 9. 分层检查（约10–15分钟）

### 9.1 一组命令、两项Success（约2–3分钟）

不Play，Output Log执行：

```text
Automation RunTests EmberTrial.M2.Dodge
```

应有 **CostContract / Movement各一次Success**。Movement覆盖8方向数学与镜头Yaw转换、无输入前方；完整GAS启动 / 一次成本、忙碌拒绝、真实运动30 / 60 / 120固定步长各150±1cm / SourceTime.30、位移结束仍Dodging / 恢复受阻、取消不退款、墙碰撞、暂停冻结 / 恢复、真实死亡取消、活动中EndPlay清理及非法配置 / 外部RMS拒绝。

固定步长是测试世界模拟步长，不冒充硬件实际30 / 60 / 120FPS测量。蓝图2键连接、真实输入 / 相机对应、坡面视觉及UI由下组实机检查；旧Practice位移测试不代替新GA测试。资源实现与旧Practice未改，不重复跑Resources / Lifecycle或旧墙坡帧率全矩阵。失败发测试名与日志路径，我读取细节，不删断言。

### 9.2 正式2键与方向（约3–4分钟）

1. 正式地图Play，在Entry空地，生命 / 体力100。松开WASD按一次2：角色前方位移、一次扣25、日志为`[M2-02B] Move begin/end`与A Begin/End；**不能出现这次输入的`[M0-05C2] DodgeMove Begin`**。
2. 依次W+2、A+2、S+2、D+2、W+A+2；每次等动作结束并在空地操作。按移动键先维持短暂一瞬再按2，观察朝向与镜头水平坐标关系。转镜头约90度，再做W+2，应跟新镜头前方。
3. 先按A移动、松开A并站定，再按2：按当前角色前方，不沿缓存的A向量。距离看日志，不要求拿尺测屏幕。
4. 长按2只开始一次；动作期间再按2被拒绝，不补播、不多扣。动作中尝试改变WASD / 转镜头，本段方向保持固定；位移停下到动画结束前仍不能移动 / 跳跃 / 攻击 / E，镜头可转。

本批持续按W等动作结束后继续走动属于正常输入恢复；观察“无漂移”时请松开WASD。若在墙边或敌人碰撞附近，Travel不能作为平地满距样本。

### 9.3 碰撞、取消与暂停（约3–5分钟）

1. 用现有场地高墙，朝墙按2，胶囊被阻挡，实际Travel可小于150；再斜向贴墙一次，应沿墙滑动、无穿墙。不要新搭测试蓝图或改正式碰撞通道。
2. 有现成可走坡面就上 / 下各一次；没有坡面时，临时拖一个Engine Cube到远离敌人的空地，Scale约`(6,3,0.3)`、Rotation Pitch约`12°`，用移动工具把低端嵌入地面，使中段可正常走上去再按2上下各一次。若普通步行都上不去，先调整斜块位置；这不是闪避失败。试完删除临时Cube，不改导航 / 不保存临时场景。
3. 可控制台`slomo 0.1`放慢，2后按3一次，观察立即停位移 / 进入受击、体力不退款；下一次2中按P，暂停时位置 / 动画冻结，继续后完成剩余位移。只试一种继续方式，不重演所有菜单焦点。具体.1秒时点由自动化负责，手动无需赶帧。
4. 恢复`slomo 1`。本批没有无敌，实战被击中扣血 / 取消正常；不要把4开发伤害键或躲开碰撞当无敌窗口证据。死亡与EndPlay精确时序已有自动化，无需再手搭临时致死入口。

### 9.4 最后短基线与恢复（约1–2分钟）

正常Play：左键真实25、E真实交互、P一种继续、移动 / 转向 / 跳跃与体力回满；确认2为新正式闪避，生命HUD保持正常。Stop后再Play，新身体双100 / 能再次2 / 无旧回调。临时Cube删除、slomo1、正式100/100 / 伤害25与10 / Debug False，GA150 / .30 / 成本25 / 原Montage，Compile → Save All。

不做新Cook / Insights；M1包不作为本批结果。地面 / 墙坡运动与生命周期成立后，C再单独接短无敌与相关集成，不在本批扩大连击 / 锁定 / 治疗。

## 10. 最简反馈与报错

```text
M2-02B 完成 / 卡在第__节第__步
三新 / 五旧Source；关闭UE后Editor Build / 索引48cpp / Game Build：
GA150 / .30 / Cost25 / 原Montage；GA与玩家Compile / Save / Valid：
IA_PracticeDodge.Started只到RequestDodge(Self)，旧2执行线断开：
CostContract / Movement各一次Success：
上方2正式移动并扣25 / 长按单次 / 忙碌无补播：
四向与斜向 / 镜头90度 / 无输入角色前方 / 方向固定：
位移先结束、Montage后解锁 / 无漂移 / 阻断恢复：
墙与斜墙 / 可走坡面上下 / 3取消不退款 / P冻结恢复：
Stop重开双100 / 左键-E-P与移动跳跃短基线：
临时Cube删除、slomo1 / 原配置与Source资产Save All：
失败或占位动画最明显问题（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

Build用文字贴第一条真实错误；蓝图连线问题截IA事件到调用的完整局部图，包含白色执行线与Target；配置问题截GA Class Defaults（不是PracticeAction），同时给资产路径。运行问题写“输入 / 当时状态 / 实际现象”，日志只给大致时分或本机路径，我自行读取；不要求手摘完整Begin / End。

## 11. 本批 Commit / Push（固定放在最后）

- 三新Source：Task.h / .cpp与ETDodgeMovementTests.cpp；五旧：ETGA_Dodge.h / .cpp、Character.h / .cpp、ETGameplayAssetValidator.cpp，必须全部保存再Stage。原CostContract / Resources / Practice源码没有计划修改。
- 旧资产实际差异：GA_ET_Dodge的距离 / 时长配置、BP_ThirdPersonCharacter的2路线；IA_PracticeDodge / IMC_Default仅在确有合法配置修改时提交，不制造无差异文件。没有新UE资产，原Montage / Cost GE复用，来源清单在实物验收时更新真实哈希。
- 本指南、M2 / 闪避路线、Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow / Introduction的最新状态；A验收报告 / 清单若尚未提交一并处理。本地c6b8a0f包含A实现，本轮未联网核验远端。
- 临时Cube删除、slomo1、150 / .30 / Cost25 / 原Montage恢复后Save All；Raw / 日志 / Binaries / Intermediate / Saved、生成CMake / IDE缓存与旧包留本地，不Force Add。

CLion刷新Changes → 重新Stage最新完整代码 / 合法配置 / 文档 → 检查Staged Diff → Commit → Push。实现后可用`feat: add directional GAS dodge movement and migrate input`；仅指南用`docs: prepare directional dodge movement`。仍由用户操作Git；C无敌尚未完成，本批不写成整个M2-02已交付。
