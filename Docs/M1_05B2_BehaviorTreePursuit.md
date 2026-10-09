# M1-05B2：行为树真实追击、忙碌停走与失败退路

日期：2026-10-08；引擎固定 **UE 5.5.4**。前置：[B1感知验收完成](Reports/M1_05B1_Verification.md)。**B2验收通过：20:09活动Move受击通过，20:27等待父连接补修通过，全部待补关闭。**[本轮复核](Reports/M1_05B2_Verification.md)与第13节记录最终结果：手动32 / 等待5分钟。第11–12节保留历史操作，无需重做；后续准备M1-05C。所有C++默认用户手动，助手没有改Source / UE资产或接管编辑器。

本批沿用[方案B分层验收](AcceptanceWorkflow_Options.md)：复用现有敌人、左键攻击、数字4伤害、P暂停与引擎调试入口，不再搭PlayerController测试函数。本批第一次接真实路径跟随，四组新增检查预计15–25分钟；后续普通批次仍以5–15分钟为目标。

## 1. 本批完成后能看到什么

一个存活的敌人看见玩家后沿NavMesh追击，并跟随移动中的目标。在平地接近身体中心约150cm时停止；玩家重新离开至200cm后恢复追击。受击期间立即停止路径跟随及残留速度，动画结束后重新判断是否追击。失去视觉 / 玩家死亡时停止追击，但行为树保留等待分支；敌人自己死亡、失去控制或结束游戏时停止整棵树。

有可达路径就绕行；没有完整路径就进入0.5秒等待，再尝试。不会每帧反复提交Move To，也不会用SetActorLocation模拟寻路。**敌人本批仍不主动攻击**；C批再接朝向 / 攻击距离 / 共享能力的离线AI执行。

建议预算：代码与理解40–60分钟，BT / 黑板配置20–30分钟，首次新增检查15–25分钟，合计约75–115分钟；Build / 索引等待另记。你的实际操作可能更快，反馈时记录实耗，不将预算计入累计工时。

## 2. 分工、数据与文件范围

| 层 | 本批职责 | 不承担的职责 |
| --- | --- | --- |
| 原生AIController | 身体 / 目标生命周期；订阅身体GAS动作标签；距离迟滞；写黑板；启动 / 停止BT | 不播放受击、不扣血、不写HUD |
| 原生BT Service | 根Selector激活时每0.2秒请求一次距离决策 | 不存每个敌人的可变状态，不每帧寻路 |
| 原生BT Task | 继承引擎Move To；执行前验证门控；记录开始 / 完成结果 | 不重写导航、Task内存、路径取消或UAITask生命周期 |
| Blackboard / BT资产 | 独立Controller的决策缓存；分支、观察中止、失败等待 | 不复制Health或Dead等GAS真值 |
| 现有Character / ASC / CharacterMovement | 正式动作是否空闲、生命、受击、移动执行 | 不依赖UI模块 |

距离是角色Actor中心的**水平距离**，不是两胶囊表面距离。150 / 200cm是追击占位参数，不等于下一批的最终攻击射程。停止判定允许1cm数值容差，防止引擎完成到达时的小误差造成重复短请求。停止后只有距离达到200cm才释放停走锁存；目标换人 / 丢失则重置锁存。

配置源只有Controller：`PursuitStopDistance=150`、`PursuitResumeDistance=200`。前者在初始化时写入Float黑板键`PursuitStopDistance`，Move To从该键读取，避免在两个地方分别填写停止半径。身体忙碌来自现有`IsFormalActionIdle()`与GAS标签，黑板Bool只是当前决策结果。

本批手动修改 **2份既有文件**：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\AI\ETAIController.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETAIController.cpp
```

手动新增 **4份文件**，实现用的头文件也放Private，不扩大模块公开接口：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTService_Pursuit.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTService_Pursuit.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.cpp
```

当前核心模块已有Public `AIModule`，固定5.5.4的AIModule又公开依赖NavigationSystem / GameplayTasks；本批只通过AIModule导出的BT / AI接口使用移动，**Build.cs、Target、插件列表不用修改**。以后若直接使用NavigationSystem API，再按实际include补显式依赖。

当前验收基线47份Source / 23cpp、资产清单38项。本批成功实施后预计51份Source / 25cpp，并新增1项BT资产；这些数字现在只表示计划，不提前登记为已实现。

## 3. 手动输入C++：先保存、关闭UE

停止PIE，Compile / Save当前资产，关闭UE Editor。CLion可保持打开。创建四个新文件可直接在CLion项目根目录下的`Private/AI`操作；**不使用UE新建类向导生成另一套同名文件**。文件后缀不能成为`.h.txt`。既有Controller文件按3.1 / 3.2全文替换，其他Source保留。

### 3.1 替换ETAIController.h全文

```cpp
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

UCLASS()
class EMBERTRIAL_API AETAIController : public AAIController
{
    GENERATED_BODY()

public:
    AETAIController(const FObjectInitializer& ObjectInitializer =
        FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "EmberTrial|AI")
    AEmberTrialCharacter* GetSensedPlayer() const;

    // Native BT nodes request a fresh decision; mutable state belongs here.
    void UpdatePursuitDecision();
    bool IsPursuitAllowed() const;

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|AI",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAIPerceptionComponent> SightPerception;

    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|AI")
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI")
    TObjectPtr<UBlackboardData> EnemyBlackboardAsset;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI")
    TObjectPtr<UBehaviorTree> EnemyBehaviorTree;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI",
        meta = (ClampMin = "1.0", Units = "cm"))
    float PursuitStopDistance = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|AI",
        meta = (ClampMin = "1.0", Units = "cm"))
    float PursuitResumeDistance = 200.0f;

    TWeakObjectPtr<AEmberTrialCharacter> ObservedBody;
    TWeakObjectPtr<AEmberTrialCharacter> SensedPlayer;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedBodyASC;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedTargetASC;
    FDelegateHandle BodyDeadHandle;
    FDelegateHandle BodyAttackingHandle;
    FDelegateHandle BodyHitReactHandle;
    FDelegateHandle TargetDeadHandle;
    bool bEndingPlay = false;
    bool bBlackboardReady = false;
    bool bControllerBegunPlay = false;
    bool bTreeStartAttempted = false;
    bool bTreeStarted = false;
    bool bPursuitAllowed = false;
    bool bHoldingDistance = false;

    UFUNCTION()
    void HandleBodyReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);

    UFUNCTION()
    void HandleSightUpdated(AActor* SensedActor, FAIStimulus Stimulus);

    UFUNCTION()
    void HandleTargetEndPlay(AActor* EndedTarget,
        EEndPlayReason::Type PlayEndReason);

    void HandleDeadTagChanged(FGameplayTag ChangedTag, int32 NewCount);
    void HandleBodyActionTagChanged(FGameplayTag ChangedTag, int32 NewCount);
    bool InitializeEnemyBlackboard();
    bool CanObservePlayer() const;
    bool HasValidPursuitConfig() const;
    void ObserveBodyState();
    void RemoveBodyStateSubscriptions();
    void RefreshVisiblePlayer();
    void SetSensedPlayer(AEmberTrialCharacter* NewPlayer, const TCHAR* Reason);
    void ClearSensedPlayer(const TCHAR* Reason);
    void SetPursuitAllowed(bool bNewAllowed, const TCHAR* Reason,
        float Distance, bool bForceStop = false);
    void StopPursuitMovement();
    void TryStartEnemyBehaviorTree();
    void StopEnemyBehaviorTree(const TCHAR* Reason);
    void StopObservingBody();
    void LogBodySnapshot(const TCHAR* Reason) const;
};
```

### 3.2 替换ETAIController.cpp全文

```cpp
#include "AI/ETAIController.h"
#include "Characters/EmberTrialCharacter.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"

DEFINE_LOG_CATEGORY_STATIC(LogETAIController, Log, All);

namespace
{
    const FName TargetActorKey(TEXT("TargetActor"));
    const FName CanPursueKey(TEXT("CanPursue"));
    const FName StopDistanceKey(TEXT("PursuitStopDistance"));
    constexpr float ArrivalTolerance = 1.0f;

    bool IsLiveBody(const AEmberTrialCharacter* BodyCharacter)
    {
        float Health = 0.0f;
        float MaxHealth = 0.0f;
        return IsValid(BodyCharacter) && !BodyCharacter->IsActorBeingDestroyed() &&
            !BodyCharacter->IsCombatDead() &&
            BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth) &&
            FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) &&
            MaxHealth >= 1.0f && Health > 0.0f && Health <= MaxHealth;
    }

    bool IsLiveLocalPlayer(const AEmberTrialCharacter* BodyCharacter)
    {
        if (!IsLiveBody(BodyCharacter))
        {
            return false;
        }
        const APlayerController* PlayerController =
            Cast<APlayerController>(BodyCharacter->GetController());
        return IsValid(PlayerController) && PlayerController->IsLocalController() &&
            PlayerController->GetPawn() == BodyCharacter;
    }

    void RemoveTagSubscription(UAbilitySystemComponent* OldASC,
        FDelegateHandle& Subscription, FGameplayTag Tag)
    {
        const FDelegateHandle OldSubscription = Subscription;
        Subscription.Reset();
        if (IsValid(OldASC) && OldSubscription.IsValid())
        {
            OldASC->UnregisterGameplayTagEvent(OldSubscription, Tag,
                EGameplayTagEventType::NewOrRemoved);
        }
    }

    void RemoveDeadSubscription(TWeakObjectPtr<UAbilitySystemComponent>& WeakASC,
        FDelegateHandle& Subscription)
    {
        UAbilitySystemComponent* OldASC = WeakASC.Get();
        WeakASC.Reset();
        RemoveTagSubscription(OldASC, Subscription, ETGameplayTags::State_Dead);
    }
}

AETAIController::AETAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bStartAILogicOnPossess = false;
    bStopAILogicOnUnposses = true;

    SightPerception = CreateDefaultSubobject<UAIPerceptionComponent>(
        TEXT("EnemyPerception"));
    SetPerceptionComponent(*SightPerception);
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 900.0f;
    SightConfig->LoseSightRadius = 1100.0f;
    SightConfig->PeripheralVisionAngleDegrees = 70.0f;
    SightConfig->AutoSuccessRangeFromLastSeenLocation = -1.0f;
    SightConfig->SetMaxAge(2.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightPerception->ConfigureSense(*SightConfig);
    SightPerception->SetDominantSense(UAISense_Sight::StaticClass());
}

void AETAIController::BeginPlay()
{
    Super::BeginPlay();
    bControllerBegunPlay = true;
    SightPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(
        this, &ThisClass::HandleSightUpdated);
    RefreshVisiblePlayer();
    UpdatePursuitDecision();
    TryStartEnemyBehaviorTree();
}

void AETAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    StopEnemyBehaviorTree(TEXT("BodyPossess"));
    ClearSensedPlayer(TEXT("BodyPossess"));
    StopObservingBody();
    bBlackboardReady = false;
    if (bEndingPlay)
    {
        return;
    }

    AEmberTrialCharacter* BodyCharacter = Cast<AEmberTrialCharacter>(GetPawn());
    if (!IsValid(BodyCharacter) || GetPawn() != InPawn)
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05A] Unsupported body Controller=%s Pawn=%s"),
            *GetNameSafe(this), *GetNameSafe(GetPawn()));
        return;
    }

    ObservedBody = BodyCharacter;
    BodyCharacter->OnGameplayReadyChanged.AddUniqueDynamic(
        this, &ThisClass::HandleBodyReadyChanged);
    bBlackboardReady = InitializeEnemyBlackboard();
    ObserveBodyState();
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] Possessed Controller=%s Body=%s Authority=%d"),
        *GetNameSafe(this), *GetNameSafe(BodyCharacter), HasAuthority() ? 1 : 0);
    LogBodySnapshot(TEXT("PossessSnapshot"));
    RefreshVisiblePlayer();
    UpdatePursuitDecision();
    TryStartEnemyBehaviorTree();
}

bool AETAIController::InitializeEnemyBlackboard()
{
    UBlackboardComponent* Board = nullptr;
    if (!IsValid(EnemyBlackboardAsset) ||
        !UseBlackboard(EnemyBlackboardAsset.Get(), Board) || !IsValid(Board) ||
        Board->GetBlackboardAsset() != EnemyBlackboardAsset.Get())
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05B1] Blackboard unavailable Controller=%s Asset=%s"),
            *GetNameSafe(this), *GetNameSafe(EnemyBlackboardAsset.Get()));
        return false;
    }

    const FBlackboard::FKey KeyID = Board->GetKeyID(TargetActorKey);
    const FBlackboardEntry* Entry = KeyID != FBlackboard::InvalidKey
        ? EnemyBlackboardAsset->GetKey(KeyID) : nullptr;
    const UBlackboardKeyType_Object* ObjectKey = Entry
        ? Cast<UBlackboardKeyType_Object>(Entry->KeyType.Get()) : nullptr;
    if (!ObjectKey || !IsValid(ObjectKey->BaseClass) ||
        !AEmberTrialCharacter::StaticClass()->IsChildOf(ObjectKey->BaseClass.Get()))
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05B1] Invalid TargetActor key Controller=%s Expected=ObjectWithCharacterCompatibleBase"),
            *GetNameSafe(this));
        return false;
    }

    const FBlackboard::FKey GateID = Board->GetKeyID(CanPursueKey);
    const FBlackboard::FKey DistanceID = Board->GetKeyID(StopDistanceKey);
    if (GateID == FBlackboard::InvalidKey ||
        DistanceID == FBlackboard::InvalidKey ||
        Board->GetKeyType(GateID) != UBlackboardKeyType_Bool::StaticClass() ||
        Board->GetKeyType(DistanceID) != UBlackboardKeyType_Float::StaticClass())
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05B2] Invalid pursuit keys Controller=%s Expected=CanPursue:Bool,PursuitStopDistance:Float"),
            *GetNameSafe(this));
        return false;
    }
    Board->SetValueAsBool(CanPursueKey, false);
    Board->ClearValue(TargetActorKey);
    Board->SetValueAsFloat(StopDistanceKey, PursuitStopDistance);
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05B1] Blackboard ready Controller=%s Asset=%s Target=None"),
        *GetNameSafe(this), *GetNameSafe(EnemyBlackboardAsset.Get()));
    return true;
}

bool AETAIController::CanObservePlayer() const
{
    return !bEndingPlay && bBlackboardReady && HasAuthority() &&
        ObservedBody.Get() == GetPawn() && IsLiveBody(ObservedBody.Get());
}

AEmberTrialCharacter* AETAIController::GetSensedPlayer() const
{
    AEmberTrialCharacter* PlayerBody = SensedPlayer.Get();
    return CanObservePlayer() && IsLiveLocalPlayer(PlayerBody) ? PlayerBody : nullptr;
}

bool AETAIController::HasValidPursuitConfig() const
{
    return FMath::IsFinite(PursuitStopDistance) &&
        FMath::IsFinite(PursuitResumeDistance) && PursuitStopDistance >= 1.0f &&
        PursuitResumeDistance > PursuitStopDistance + ArrivalTolerance;
}

bool AETAIController::IsPursuitAllowed() const
{
    const AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    const AEmberTrialCharacter* PlayerBody = GetSensedPlayer();
    const UBlackboardComponent* Board = GetBlackboardComponent();
    const UCharacterMovementComponent* Movement = IsValid(BodyCharacter)
        ? BodyCharacter->GetCharacterMovement() : nullptr;
    return bPursuitAllowed && HasValidPursuitConfig() &&
        IsValid(PlayerBody) && IsValid(Board) &&
        Board->GetValueAsObject(TargetActorKey) == PlayerBody &&
        BodyCharacter->IsFormalActionIdle() && IsValid(Movement) &&
        Movement->IsMovingOnGround();
}

void AETAIController::UpdatePursuitDecision()
{
    AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    AEmberTrialCharacter* PlayerBody = GetSensedPlayer();
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!CanObservePlayer() || !IsValid(Board))
    {
        SetPursuitAllowed(false, TEXT("BodyUnavailable"), -1.0f);
        return;
    }
    if (!HasValidPursuitConfig())
    {
        SetPursuitAllowed(false, TEXT("InvalidDistanceConfig"), -1.0f);
        return;
    }
    if (!IsValid(PlayerBody) ||
        Board->GetValueAsObject(TargetActorKey) != PlayerBody)
    {
        bHoldingDistance = false;
        SetPursuitAllowed(false, TEXT("NoTarget"), -1.0f);
        return;
    }

    const float Distance = FVector::Dist2D(BodyCharacter->GetActorLocation(),
        PlayerBody->GetActorLocation());
    if (!FMath::IsFinite(Distance))
    {
        SetPursuitAllowed(false, TEXT("InvalidDistance"), -1.0f);
        return;
    }
    if (Distance <= PursuitStopDistance + ArrivalTolerance)
    {
        bHoldingDistance = true;
    }
    else if (Distance >= PursuitResumeDistance)
    {
        bHoldingDistance = false;
    }

    UCharacterMovementComponent* Movement = BodyCharacter->GetCharacterMovement();
    if (!BodyCharacter->IsFormalActionIdle())
    {
        SetPursuitAllowed(false, TEXT("Busy"), Distance);
    }
    else if (!IsValid(Movement) || !Movement->IsMovingOnGround())
    {
        SetPursuitAllowed(false, TEXT("OffGround"), Distance);
    }
    else
    {
        SetPursuitAllowed(!bHoldingDistance,
            bHoldingDistance ? TEXT("NearTarget") : TEXT("CanChase"), Distance);
    }
}

void AETAIController::SetPursuitAllowed(bool bNewAllowed, const TCHAR* Reason,
    float Distance, bool bForceStop)
{
    const bool bChanged = bPursuitAllowed != bNewAllowed;
    const bool bWasAllowed = bPursuitAllowed;
    // Commit before Blackboard observers / path completion callbacks run.
    bPursuitAllowed = bNewAllowed;
    if (bBlackboardReady)
    {
        if (UBlackboardComponent* Board = GetBlackboardComponent())
        {
            if (Board->GetValueAsBool(CanPursueKey) != bNewAllowed)
            {
                Board->SetValueAsBool(CanPursueKey, bNewAllowed);
            }
        }
    }
    if (!bNewAllowed && (bWasAllowed || bForceStop))
    {
        StopPursuitMovement();
    }
    if (bChanged || bForceStop)
    {
        UE_LOG(LogETAIController, Display,
            TEXT("[M1-05B2] Pursuit gate Controller=%s Allowed=%d Reason=%s Distance2D=%.1f Holding=%d"),
            *GetNameSafe(this), bNewAllowed ? 1 : 0, Reason, Distance,
            bHoldingDistance ? 1 : 0);
    }
}

void AETAIController::StopPursuitMovement()
{
    StopMovement();
    if (AEmberTrialCharacter* BodyCharacter = ObservedBody.Get())
    {
        if (UCharacterMovementComponent* Movement =
            BodyCharacter->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
}

void AETAIController::TryStartEnemyBehaviorTree()
{
    if (!bControllerBegunPlay || bTreeStartAttempted || !CanObservePlayer())
    {
        return;
    }
    bTreeStartAttempted = true;
    if (!HasValidPursuitConfig() || !IsValid(EnemyBehaviorTree) ||
        EnemyBehaviorTree->BlackboardAsset.Get() != EnemyBlackboardAsset.Get() ||
        !IsValid(EnemyBehaviorTree->RootNode))
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05B2] Tree config rejected Controller=%s Tree=%s Stop=%.1f Resume=%.1f Expected=ValidRootAndSameBlackboard"),
            *GetNameSafe(this), *GetNameSafe(EnemyBehaviorTree.Get()),
            PursuitStopDistance, PursuitResumeDistance);
        return;
    }
    UpdatePursuitDecision();
    bTreeStarted = true;
    const bool bAccepted = RunBehaviorTree(EnemyBehaviorTree.Get());
    if (!bAccepted || !CanObservePlayer())
    {
        StopEnemyBehaviorTree(TEXT("StartRejectedOrBodyUnavailable"));
        // Avoid retrying invalid configuration every service tick.
        bTreeStartAttempted = true;
    }
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05B2] Tree start requested Controller=%s Tree=%s Accepted=%d Stop=%.1f Resume=%.1f"),
        *GetNameSafe(this), *GetNameSafe(EnemyBehaviorTree.Get()),
        bAccepted && bTreeStarted ? 1 : 0,
        PursuitStopDistance, PursuitResumeDistance);
}

void AETAIController::StopEnemyBehaviorTree(const TCHAR* Reason)
{
    const bool bWasStarted = bTreeStarted;
    bTreeStarted = false;
    bTreeStartAttempted = false;
    bHoldingDistance = false;
    SetPursuitAllowed(false, Reason, -1.0f);
    if (UBehaviorTreeComponent* TreeComponent =
        Cast<UBehaviorTreeComponent>(GetBrainComponent()))
    {
        TreeComponent->StopTree(EBTStopMode::Forced);
    }
    StopPursuitMovement();
    if (bWasStarted)
    {
        UE_LOG(LogETAIController, Display,
            TEXT("[M1-05B2] Tree stopped Controller=%s Reason=%s"),
            *GetNameSafe(this), Reason);
    }
}

void AETAIController::HandleBodyReadyChanged(
    AEmberTrialCharacter* BodyCharacter, bool bReady)
{
    if (bEndingPlay || !IsValid(BodyCharacter) ||
        ObservedBody.Get() != BodyCharacter || GetPawn() != BodyCharacter)
    {
        return;
    }
    if (!bReady)
    {
        StopEnemyBehaviorTree(TEXT("BodyNotReady"));
        ClearSensedPlayer(TEXT("BodyNotReady"));
        RemoveBodyStateSubscriptions();
    }
    else
    {
        ObserveBodyState();
        RefreshVisiblePlayer();
        UpdatePursuitDecision();
        TryStartEnemyBehaviorTree();
    }
    LogBodySnapshot(TEXT("ReadyChanged"));
}

void AETAIController::ObserveBodyState()
{
    AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    if (bEndingPlay || !IsValid(BodyCharacter) || !BodyCharacter->IsGameplayReady())
    {
        return;
    }
    UAbilitySystemComponent* BodyASC = BodyCharacter->GetAbilitySystemComponent();
    if (!IsValid(BodyASC) || (ObservedBodyASC.Get() == BodyASC &&
        BodyDeadHandle.IsValid() && BodyAttackingHandle.IsValid() &&
        BodyHitReactHandle.IsValid()))
    {
        return;
    }
    RemoveBodyStateSubscriptions();
    ObservedBodyASC = BodyASC;
    BodyDeadHandle = BodyASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleDeadTagChanged);
    BodyAttackingHandle = BodyASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_Attacking, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleBodyActionTagChanged);
    BodyHitReactHandle = BodyASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleBodyActionTagChanged);
    if (BodyASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
    {
        ClearSensedPlayer(TEXT("BodyAlreadyDead"));
        StopEnemyBehaviorTree(TEXT("BodyAlreadyDead"));
    }
}

void AETAIController::RemoveBodyStateSubscriptions()
{
    UAbilitySystemComponent* OldASC = ObservedBodyASC.Get();
    ObservedBodyASC.Reset();
    RemoveTagSubscription(OldASC, BodyDeadHandle, ETGameplayTags::State_Dead);
    RemoveTagSubscription(OldASC, BodyAttackingHandle, ETGameplayTags::State_Attacking);
    RemoveTagSubscription(OldASC, BodyHitReactHandle, ETGameplayTags::State_HitReact);
}

void AETAIController::HandleBodyActionTagChanged(FGameplayTag ChangedTag,
    int32 NewCount)
{
    if (bEndingPlay || (ChangedTag != ETGameplayTags::State_Attacking &&
        ChangedTag != ETGameplayTags::State_HitReact))
    {
        return;
    }
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05B2] Body action tag Controller=%s Tag=%s Count=%d"),
        *GetNameSafe(this), *ChangedTag.ToString(), NewCount);
    UpdatePursuitDecision();
}

void AETAIController::RefreshVisiblePlayer()
{
    if (!CanObservePlayer())
    {
        return;
    }
    TArray<AActor*> VisibleActors;
    SightPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(),
        VisibleActors);
    for (AActor* VisibleActor : VisibleActors)
    {
        AEmberTrialCharacter* PlayerBody = Cast<AEmberTrialCharacter>(VisibleActor);
        if (IsLiveLocalPlayer(PlayerBody))
        {
            SetSensedPlayer(PlayerBody, TEXT("PerceptionSnapshot"));
            return;
        }
    }
}

void AETAIController::HandleSightUpdated(AActor* SensedActor, FAIStimulus Stimulus)
{
    if (bEndingPlay || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
    {
        return;
    }
    if (!Stimulus.WasSuccessfullySensed())
    {
        if (SensedPlayer.Get(true) == SensedActor)
        {
            ClearSensedPlayer(TEXT("SightLost"));
        }
        return;
    }
    AEmberTrialCharacter* PlayerBody = Cast<AEmberTrialCharacter>(SensedActor);
    if (CanObservePlayer() && IsLiveLocalPlayer(PlayerBody))
    {
        SetSensedPlayer(PlayerBody, TEXT("SightAcquired"));
    }
}

void AETAIController::SetSensedPlayer(AEmberTrialCharacter* NewPlayer,
    const TCHAR* Reason)
{
    if (!CanObservePlayer() || !IsLiveLocalPlayer(NewPlayer) ||
        SensedPlayer.Get() == NewPlayer)
    {
        return;
    }
    UAbilitySystemComponent* TargetASC = NewPlayer->GetAbilitySystemComponent();
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!IsValid(TargetASC) || !IsValid(Board))
    {
        return;
    }
    ClearSensedPlayer(TEXT("TargetChanged"));
    const FString NewPlayerName = GetNameSafe(NewPlayer);
    SensedPlayer = NewPlayer;
    ObservedTargetASC = TargetASC;
    NewPlayer->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleTargetEndPlay);
    TargetDeadHandle = TargetASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleDeadTagChanged);
    Board->SetValueAsObject(TargetActorKey, NewPlayer);
    if (SensedPlayer.Get() != NewPlayer)
    {
        return;
    }
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05B1] Target acquired Controller=%s Player=%s Reason=%s BlackboardTarget=%s"),
        *GetNameSafe(this), *NewPlayerName, Reason,
        *GetNameSafe(Board->GetValueAsObject(TargetActorKey)));
    UpdatePursuitDecision();
}

void AETAIController::ClearSensedPlayer(const TCHAR* Reason)
{
    AEmberTrialCharacter* OldPlayer = SensedPlayer.Get();
    const bool bHadTarget = !SensedPlayer.IsExplicitlyNull();
    const FString OldPlayerName = GetNameSafe(OldPlayer);
    SensedPlayer.Reset();
    bHoldingDistance = false;
    // Abort the old move before clearing its goal key.
    SetPursuitAllowed(false, Reason, -1.0f, bHadTarget);
    RemoveDeadSubscription(ObservedTargetASC, TargetDeadHandle);
    if (IsValid(OldPlayer))
    {
        OldPlayer->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleTargetEndPlay);
    }
    if (UBlackboardComponent* Board = GetBlackboardComponent())
    {
        if (bBlackboardReady)
        {
            Board->ClearValue(TargetActorKey);
        }
    }
    if (bHadTarget)
    {
        UE_LOG(LogETAIController, Display,
            TEXT("[M1-05B1] Target cleared Controller=%s OldPlayer=%s Reason=%s BlackboardTarget=None"),
            *GetNameSafe(this), *OldPlayerName, Reason);
    }
}

void AETAIController::HandleDeadTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
    if (bEndingPlay || ChangedTag != ETGameplayTags::State_Dead || NewCount <= 0)
    {
        return;
    }
    if (!IsLiveBody(ObservedBody.Get()))
    {
        ClearSensedPlayer(TEXT("BodyDead"));
        StopEnemyBehaviorTree(TEXT("BodyDead"));
    }
    else if (!SensedPlayer.IsExplicitlyNull() &&
        !IsLiveLocalPlayer(SensedPlayer.Get()))
    {
        ClearSensedPlayer(TEXT("TargetDead"));
    }
}

void AETAIController::HandleTargetEndPlay(AActor* EndedTarget,
    EEndPlayReason::Type PlayEndReason)
{
    (void)PlayEndReason;
    // Get(true) is used only for identity comparison during target destruction.
    if (!bEndingPlay && SensedPlayer.Get(true) == EndedTarget)
    {
        ClearSensedPlayer(TEXT("TargetEndPlay"));
    }
}

void AETAIController::StopObservingBody()
{
    AEmberTrialCharacter* OldBody = ObservedBody.Get();
    ObservedBody.Reset();
    RemoveBodyStateSubscriptions();
    if (IsValid(OldBody))
    {
        OldBody->OnGameplayReadyChanged.RemoveDynamic(
            this, &ThisClass::HandleBodyReadyChanged);
    }
}

void AETAIController::OnUnPossess()
{
    const FString OldBodyName = GetNameSafe(ObservedBody.Get());
    StopEnemyBehaviorTree(TEXT("BodyUnpossess"));
    ClearSensedPlayer(TEXT("BodyUnpossess"));
    StopObservingBody();
    bBlackboardReady = false;
    Super::OnUnPossess();
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] Unpossessed Controller=%s OldBody=%s Ending=%d"),
        *GetNameSafe(this), *OldBodyName, bEndingPlay ? 1 : 0);
}

void AETAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    const FString OldBodyName = GetNameSafe(ObservedBody.Get());
    SightPerception->OnTargetPerceptionUpdated.RemoveDynamic(
        this, &ThisClass::HandleSightUpdated);
    StopEnemyBehaviorTree(TEXT("ControllerEndPlay"));
    ClearSensedPlayer(TEXT("ControllerEndPlay"));
    StopObservingBody();
    bBlackboardReady = false;
    Super::EndPlay(EndPlayReason);
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] End Controller=%s OldBody=%s Reason=%d"),
        *GetNameSafe(this), *OldBodyName, static_cast<int32>(EndPlayReason));
}

void AETAIController::LogBodySnapshot(const TCHAR* Reason) const
{
    const AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    if (bEndingPlay || !IsValid(BodyCharacter) || GetPawn() != BodyCharacter)
    {
        return;
    }
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    const bool bSnapshotValid = BodyCharacter->GetGameplayHealthSnapshot(
        Health, MaxHealth);
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] Body snapshot Controller=%s Body=%s Reason=%s Ready=%d Snapshot=%d Health=%.1f MaxHealth=%.1f Dead=%d"),
        *GetNameSafe(this), *GetNameSafe(BodyCharacter), Reason,
        BodyCharacter->IsGameplayReady() ? 1 : 0,
        bSnapshotValid ? 1 : 0, Health, MaxHealth,
        BodyCharacter->IsCombatDead() ? 1 : 0);
}
```

几个容易漏掉的边界：

- `BodyCharacter`不能改名为`Character`，继续避开AController继承成员造成的UHT遮蔽。
- `BeginPlay`与身体Ready都可能先到，树启动需要两者成立；`bTreeStartAttempted`避免同一生命周期重复提交启动或重复报配置错误。
- 忙碌标签变化是事件驱动的，不等0.2秒Service才取消移动。距离只由Service低频采样；碰撞 / 移动执行仍由引擎更新。
- `StopMovement()`取消AI路径，`StopMovementImmediately()`清当前速度；没有用DisableMovement / DisableInput封死存活敌人。死亡的移动封锁仍归现有CombatLife。
- 失去玩家只撤销追击，等待分支继续运行；自身死亡 / Ready撤销 / UnPossess / EndPlay才停止整树。结束先提交标记与门控，再取消任务、清目标、解绑身体，防止结束回调重新追击。
- Body ASC的三个订阅共享同一数据源，清理时全部解绑；目标死亡订阅独立。BT节点模板不缓存Pawn或委托句柄。

### 3.3 新建ETBTService_Pursuit.h全文

```cpp
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ETBTService_Pursuit.generated.h"

UCLASS()
class UETBTService_Pursuit : public UBTService
{
    GENERATED_BODY()

public:
    UETBTService_Pursuit(const FObjectInitializer& ObjectInitializer =
        FObjectInitializer::Get());

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory, float DeltaSeconds) override;
};
```

### 3.4 新建ETBTService_Pursuit.cpp全文

```cpp
#include "AI/ETBTService_Pursuit.h"
#include "AI/ETAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UETBTService_Pursuit::UETBTService_Pursuit(
    const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    NodeName = TEXT("ET Pursuit Decision");
    INIT_SERVICE_NODE_NOTIFY_FLAGS();
    Interval = 0.2f;
    RandomDeviation = 0.0f;
    bCallTickOnSearchStart = true;
    bRestartTimerOnEachActivation = false;
}

void UETBTService_Pursuit::TickNode(UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory, float DeltaSeconds)
{
    // Super schedules the next interval; omitting it can cause excess ticks.
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    if (AETAIController* EnemyController =
        Cast<AETAIController>(OwnerComp.GetAIOwner()))
    {
        EnemyController->UpdatePursuitDecision();
    }
}
```

### 3.5 新建ETBTTask_PursuitMove.h全文

```cpp
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "ETBTTask_PursuitMove.generated.h"

UCLASS()
class UETBTTask_PursuitMove : public UBTTask_MoveTo
{
    GENERATED_BODY()

public:
    UETBTTask_PursuitMove(const FObjectInitializer& ObjectInitializer =
        FObjectInitializer::Get());

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
};
```

### 3.6 新建ETBTTask_PursuitMove.cpp全文

```cpp
#include "AI/ETBTTask_PursuitMove.h"
#include "AI/ETAIController.h"
#include "Characters/EmberTrialCharacter.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPursuitMove, Log, All);

namespace
{
    const FName TargetActorKey(TEXT("TargetActor"));
    const FName StopDistanceKey(TEXT("PursuitStopDistance"));

    const TCHAR* DescribeMoveResult(EBTNodeResult::Type Result)
    {
        switch (Result)
        {
        case EBTNodeResult::Succeeded: return TEXT("Succeeded");
        case EBTNodeResult::Failed: return TEXT("Failed");
        case EBTNodeResult::Aborted: return TEXT("Aborted");
        case EBTNodeResult::InProgress: return TEXT("InProgress");
        default: return TEXT("Unknown");
        }
    }
}

UETBTTask_PursuitMove::UETBTTask_PursuitMove(
    const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    NodeName = TEXT("ET Pursuit Move To");
    bNotifyTaskFinished = true;
    BlackboardKey.SelectedKeyName = TargetActorKey;
    AcceptableRadius = 150.0f;
    AcceptableRadius.SetKey(StopDistanceKey);
    bAllowPartialPath = false;
    bTrackMovingGoal = true;
    bRequireNavigableEndLocation = true;
    bProjectGoalLocation = false;
    bReachTestIncludesAgentRadius = false;
    bReachTestIncludesGoalRadius = false;
    bAllowStrafe = false;
    bStartFromPreviousPath = false;
    bObserveBlackboardValue = false;
}

EBTNodeResult::Type UETBTTask_PursuitMove::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AETAIController* EnemyController =
        Cast<AETAIController>(OwnerComp.GetAIOwner());
    UBlackboardComponent* Board = OwnerComp.GetBlackboardComponent();
    if (IsValid(EnemyController))
    {
        EnemyController->UpdatePursuitDecision();
    }
    if (!IsValid(EnemyController) || !IsValid(Board) ||
        !EnemyController->IsPursuitAllowed() ||
        Board->GetValueAsObject(BlackboardKey.SelectedKeyName) !=
            EnemyController->GetSensedPlayer())
    {
        UE_LOG(LogETPursuitMove, Display,
            TEXT("[M1-05B2] Move rejected Controller=%s Reason=InvalidGateOrGoal"),
            *GetNameSafe(EnemyController));
        return EBTNodeResult::Failed;
    }
    const FString GoalName = GetNameSafe(
        Board->GetValueAsObject(BlackboardKey.SelectedKeyName));
    const float Radius = AcceptableRadius.GetValue(OwnerComp);
    const EBTNodeResult::Type Result = Super::ExecuteTask(OwnerComp, NodeMemory);
    UE_LOG(LogETPursuitMove, Display,
        TEXT("[M1-05B2] Move request Controller=%s Goal=%s Radius=%.1f Result=%s"),
        *GetNameSafe(EnemyController), *GoalName, Radius, DescribeMoveResult(Result));
    return Result;
}

void UETBTTask_PursuitMove::OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    const FString ControllerName = GetNameSafe(OwnerComp.GetAIOwner());
    // Always retain native UAITask / observer cleanup, including rejection.
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
    UE_LOG(LogETPursuitMove, Display,
        TEXT("[M1-05B2] Move finished Controller=%s Result=%s"),
        *ControllerName, DescribeMoveResult(TaskResult));
}
```

Task不设置`bCreateNodeInstance=true`，也不新增运行中可变成员；引擎父类使用每个BT实例的NodeMemory保存请求 / UAITask，继续负责Abort、消息、内存与清理。不能删掉`Super::OnTaskFinished`，也不要另写计时器假装移动成功。

## 4. 正常Build、CLion索引与Game Build

保存上述六文件，确认UE关闭。PowerShell：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后在**原工程根工作空间**Reload CMake Project；预期索引由23cpp变为25cpp。检查两个新cpp属于EmberTrial CodeModel；Ctrl+B分别对`BTService.h`、`BTTask_MoveTo.h`能导航。`.generated.h`由UHT生成，不手动创建。接着：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标成功后再打开UE。若失败，发**第一条具体UHT / C++错误及前后约10行文本**，保留文件路径 / 行号；最后一行`exit code 6`本身不能定位。不要先清缓存或继续配资产。

## 5. 给既有黑板增加两个键

打开`/Game/EmberTrial/AI/Blackboards/BB_ETMeleeEnemy`，保留Parent=None、既有`TargetActor` Object / Base Class Actor及可选SelfActor。

1. 左侧Blackboard面板 **New Key → Bool**，命名`CanPursue`。
2. 选中它，右侧Key Details中**Instance Synced=False**；Description可填`Controller pursuit decision`。Bool运行时由Controller初始化为False，不添加蓝图赋值。
3. **New Key → Float**，命名`PursuitStopDistance`。同样**Instance Synced=False**，Description可填`Controller-owned arrival distance in cm`。
4. Save。不添加`Health / Dead / Busy`，不修改TargetActor类型。这里只定义键；运行中的150由Controller写入，不寻找黑板资产内的浮点默认值编辑框。

## 6. 新建BT与逐项连线

### 6.1 创建资产并指定同一个黑板

1. Content Browser创建目录`/Game/EmberTrial/AI/BehaviorTrees`。
2. 目录空白处右键 → **Artificial Intelligence → Behavior Tree**，命名`BT_ETMeleeEnemy`。
3. 打开BT。工具栏切至Behavior Tree页签，**选中图中ROOT节点本体**，在右侧Details的**AI / BehaviorTree → Blackboard Asset**选择既有`BB_ETMeleeEnemy`。不要选下方Selector或点击空白处后寻找该字段；固定5.5.4的可编辑黑板属性属于ROOT图节点。若Details没显示，菜单Window → Details恢复该面板。
4. 左侧Blackboard列表应能看到TargetActor、CanPursue、PursuitStopDistance；再Save。不能创建另一份同名黑板，也不能把黑板设成None。

### 6.2 主干连接：左追击、右等待

行为树连接线是执行结构；本批不需要Event Graph白色执行线或Target引脚。

1. 从**ROOT下方输出连接点**拖到空白处，在弹出的菜单中选择**Composites → Selector**，作为唯一根组合节点；其Node Name可改为`PursuitOrWait`。
2. 从Selector的**下方输出连接点**拖至**左下方**空白处，选择**Composites → Sequence**，Node Name改为`PursuePlayer`。
3. 再从Selector下方输出点拖至**右下方**，选择**Tasks → Wait**，Node Name改为`IdleOrRetry`。
4. 选中右侧Wait：**Wait Time=0.5、Random Deviation=0.0**，两个值都使用常量`None`而不是黑板绑定。
5. 从左侧Sequence下方输出点拖至它的**左下方**，选择**Tasks → ET Pursuit Move To**。搜索也可输入`ETBTTask_PursuitMove`；必须是本批原生类，不是普通`Move To`或`Move Directly Toward`。
6. 再从左侧Sequence下方输出点拖至它的**右下方**，选择**Tasks → Wait**，Node Name改为`ArrivalSettle`。配置**Wait Time=0.25、Random Deviation=0.0**，使用常量None。
7. 节点执行优先级按横向从左到右：Selector先看左Sequence，再看右Wait；Sequence先Move，再ArrivalSettle。看节点右上小编号 / 连接顺序确认，不要把右Wait移到追击分支左侧。

**保存前沿线核对父节点：**Selector下方必须分出**两条线**，分别通向Sequence顶部与IdleOrRetry顶部；Sequence下方也必须分出**两条线**，分别通向Move顶部与ArrivalSettle顶部。IdleOrRetry摆在右侧还不够，它的上端必须连Selector，不能连Sequence。若Sequence下面有三个任务，连线错误；Move失败会跳过这个Sequence后面的全部Wait，形成高频重试。

结构必须如下；Service / Decorator在下一节附着，不是独立连到下方的任务：

```text
ROOT
  └─ Selector: PursuitOrWait
       [Service: ET Pursuit Decision]
       ├─ Sequence: PursuePlayer                 ← 左，优先
       │    [Decorator: Blackboard CanPursue Is Set, Aborts Both]
       │    ├─ ET Pursuit Move To                ← 左，先执行
       │    └─ Wait: ArrivalSettle (0.25s)        ← 右，成功后执行
       └─ Wait: IdleOrRetry (0.5s)                ← 右，空闲 / 失败退路
```

### 6.3 附着Service与观察Decorator

1. **右键根Selector节点本体 → Add Service → ET Pursuit Decision**，可搜索`ETBTService_Pursuit`。它应显示为Selector内部的绿色附着条；不能放在只追击时才激活的Sequence上，否则等待时无法重新判断距离。
2. 单独选中该Service附着条，Details：**Interval=0.2、Random Deviation=0.0**。展开Service的高级项，**Call Tick on Search Start=True、Restart Timer on Each Activation=False**。默认已由C++设置，仍核对，不新增Tick Event。
3. **右键左侧Sequence本体 → Add Decorator → Blackboard**。选中它的蓝色附着条。
4. Details逐项：**Blackboard Key=CanPursue**；**Key Query=Is Set**（Bool表示True）；**Notify Observer=On Result Change**；**Observer Aborts=Both**。
5. `Both`包含当前追击分支被门控撤销时的Self中止，以及等待期间门控变True时的Lower Priority中止。右侧等待必须比左Sequence优先级低，否则这个选项的意义被破坏。
6. 不再添加另一个距离蓝图Decorator。目标是否有效已参与CanPursue，Task执行前再次查询；目标丢失会先关门控，再清TargetActor。

### 6.4 Move To全部配置与UE5.5属性位置

选中**ET Pursuit Move To任务本体**，不是Sequence或Decorator，查看Details。搜索框先清空；Node区域的高级项需展开。

| Details属性 | 最终配置 | 作用 |
| --- | --- | --- |
| Blackboard Key | **TargetActor** | 目标必须为实际Actor，不是Vector副本 |
| Acceptable Radius | 右侧绑定下拉 **PursuitStopDistance** | 运行时读取Controller写入的150 |
| Allow Partial Path | 常量None / **False** | 不能以未到达玩家的半截路径当成功 |
| Track Moving Goal | 常量None / **True** | 引擎路径更新追踪移动Actor |
| Require Navigable End Location | 常量None / **True** | 目标结束位置需可导航 |
| Project Goal Location | 常量None / **False** | 本批不把导航外目标投影为另一处可达位置 |
| Reach Test Includes Agent Radius | 常量None / **False** | 不额外加敌人胶囊半径 |
| Reach Test Includes Goal Radius | 常量None / **False** | 不额外加玩家胶囊半径 |
| Allow Strafe | 常量None / **False** | 保持当前随移动方向转向 |
| Start From Previous Path | 常量None / **False** | 新请求使用当前位置 |
| Filter Class | None | 使用当前默认导航过滤器 |

**本机5.5.4已从引擎源码核对：**Acceptable Radius等属性为`FValueOrBBKey`。同一行左侧是常量数值 / 勾选框，右侧是黑板键下拉，显示`None`时才编辑常量。为Radius选中`PursuitStopDistance`后，左数值可能灰色，属于正常绑定状态；运行日志应显示Radius=150.0。下拉没有该键时，先确认BT绑定正确黑板、键为Float且已保存，再重新选任务，不硬填一个字符串冒充绑定。

布尔项右侧保持None，左勾选按表。`Allow Partial Path / Track Moving Goal / Require Navigable End Location / Project Goal Location`是高级项，展开Node区域底部小箭头；Details搜索完整英文属性也可定位。`Observe Blackboard Value`在固定5.5.4头文件中不是EditAnywhere，本批由C++保持False，**无需在Details寻找这个开关**。目标Actor跟踪由Track Moving Goal完成；目标替换 / 清空由Controller撤销旧请求。

按Save；BT资产通常直接Save，没有Widget那样的Compile按钮。没有红色节点 / missing key，再进入Controller配置。

## 7. Controller配置与地图保留项

1. 打开`/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController`，点击**Class Defaults**。Details搜索`Enemy`：**Enemy Blackboard Asset仍为BB_ETMeleeEnemy**，**Enemy Behavior Tree=BT_ETMeleeEnemy**。
2. 搜索`Pursuit`：**Pursuit Stop Distance=150、Pursuit Resume Distance=200**。这是Class Defaults，不是在敌人身体或PIE对象上填。Compile → Save。
3. 打开`BP_ETMeleeEnemy`：AI Controller Class仍为上述BP、Auto Possess AI保持原配置；Health / MaxHealth=100/100，正式HitReact Class保留，LightAttack Class仍None；不添加PracticeAction / 感知第二组件 / Event BeginPlay Run Behavior Tree。Compile → Save。
4. 打开`L_ET_VerticalSlice`。保留一个MeleeEnemy_A，其原位置(600,250,120)、Yaw180和一个Nav_ET_Room；原训练目标也保留。地图Build → **Build Paths**，按P确认玩家与敌人可站立地面覆盖绿色，按P关闭导航显示，再Save All。
5. 本批不修改MaxWalkSpeed220、Orient Rotation to Movement=True、RotationRate Z=360；不新增Set Focus。C批再处理停步时朝向玩家与攻击。

## 8. 四组新增检查：预计15–25分钟

先确认Output Log过滤能看到`M1-05B2`及B1目标转换；不要设为仅Error。BT调试：PIE后按Shift+F1释放鼠标，打开BT，在上方Debug Filter选择本轮**BP_ETMeleeAIController**实例；界面能显示当前激活分支与黑板实时值。继续操作时回游戏视口点击一次获得输入。暂停检查最后另查无需点击视口的恢复输入。

`Tree start requested Accepted=1`只说明引擎接受启动请求；**BT有活跃分支 + 实际移动**才证明执行。请求Result=InProgress是正在寻路，并非已成功到达。受击 / 目标失效取消当前移动时，任务结束可能是Aborted或Failed，取决于黑板中止与原生路径取消的处理先后；结合Gate原因和角色停止判断，不只盯一个结果词。

### 8.1 追击、目标跟随、停止与恢复（约3–5分钟）

**准备：**正式地图，单敌人、默认150/200，无临时障碍。Play。

1. 出生应有每Controller一次`Tree start requested ... Accepted=1 Stop=150.0 Resume=200.0`。没看见玩家时右Wait活跃，TargetActor=None、CanPursue=False、StopDistance=150。
2. 朝敌人走近并进入它朝向的视锥。看到B1 `Target acquired`后，应有`Pursuit gate Allowed=1 Reason=CanChase`与`Move request ... Radius=150.0`；敌人沿地面靠近，而非原地踏步。
3. 在保持可见的前提下横向移动 / 后退，敌人应追踪当前玩家位置。不能不断冲向第一次看见玩家的旧点。短暂转身使SightLost时立即停是正常感知规则，不当成寻路故障。
4. 站定让敌人靠近：约150cm附近停止，Service记录`Allowed=0 Reason=NearTarget`，等待分支接管。静置3秒不应持续每帧发Move request。允许停止瞬间有一次Succeeded / Failed / Aborted收尾，不要求厘米级精确落点。
5. 慢慢离开，150–200cm范围内应保持停步；达到约200cm后`Allowed=1 Reason=CanChase`并恢复。做两轮即可。

**通过标准：**真实移动、Actor目标跟踪、停止后安定、离开后恢复；不要求新测试键测距离。Gate变化日志的Distance2D可辅助核对，Service采样有0.2秒延迟。

### 8.2 绕低障碍与无完整路径退路（约5–8分钟）

**先Stop PIE再改场景。不要改正在运行的世界当作最终资产配置。**

**A：可见目标的真实绕行。**

1. 从Place Actors添加基础Cube，标签`ET_TempNavBlock`，放在 **Location(300,250,40)、Rotation(0,0,0)、Scale(0.5,3.0,0.8)**。模板Cube100cm，因此实际50×300×80cm，顶面Z80。
2. Static Mesh Component设**Mobility=Static、Collision Presets=BlockAll、Can Ever Affect Navigation=True**。敌人身体的导航影响继续False。
3. Build Paths，按P检查箱体附近出现导航阻挡，左右仍有绿色绕路；确认箱顶低于双方视线且高于默认可跨台阶，不用B1挡住视线的高墙替代。关P。
4. Play，玩家走到箱体西侧、接近X0/Y250的区域，引出敌人。敌人应绕端部到玩家所在侧，不穿箱子。若日志是SightLost而非持续目标，先确认视线 / 实际箱高，不把感知丢失当无路径。
5. Stop，选中并删除**仅这个临时Cube**，保留房间墙 / 地面 / 导航体；Build Paths。

**B：目标在导航外，但仍可见。**

1. 先记下Nav_ET_Room原值：**Location(0,0,150)、Rotation0、Scale1；Brush Settings的X/Y/Z=2000/2000/500**。若你的原值经过合法调整，以自己记录的值为恢复依据。
2. 暂时将导航体**Location改为(600,0,150)**，Brush **X=600、Y=2000、Z=500**，Scale仍1。Build Paths。绿色X范围约300–900，覆盖敌人出生点；西侧玩家地面仍可物理行走，但不在导航覆盖中。
3. Play，玩家走到约X0/Y250进入敌人视野，自己不要踏入X≥300的绿色区。应有有效Target与Allowed=1，但Move request / finished为Failed，敌人不沿半路径冒充到达。
4. 保持该位置约3秒：失败之间进入右侧0.5秒Wait；稳定期间约每0.5秒再试，不得每帧刷满请求。只需一次短样本，不观察一整分钟。
5. Stop，**恢复原导航体全部记录值**，Build Paths；确认整房间覆盖恢复，P关闭显示，Save All。再Play一次确认普通追击恢复。

**通过标准：**有路绕行、无完整路径失败后等待、恢复导航后可追击。保留引擎可能出现的NoPath / invalid location信息用于排障；这个临时负例不是正常最终配置中的报错。

### 8.3 忙碌立即停走、结束重判与自身死亡（约3–6分钟）

**准备：**已恢复导航、没有箱体。先在敌人接近时用正式左键实际命中一次，确认旧伤害 / 受击仍正常。忙碌中不能继续滑着追击；结束后只有仍有目标且距玩家足够远时才恢复，近处保持停步是正确行为。

如果真实攻击难以恰好在敌人移动中命中，使用下面的**现有原生受击请求 + 引擎慢速**做这一项，不新增调试蓝图：

1. 重开本轮Play，按`~`打开游戏控制台，输入`slomo 0.2`并Enter，再按`~`关闭。慢速只改变游戏时间，便于观察，不保存到资产。
2. 走进敌人视野后，**在它距离你仍约400–600cm、已经追击时**打开控制台，输入：

```text
ke BP_ETMeleeEnemy_C RequestHitReact
```

3. Enter并关闭控制台。`BP_ETMeleeEnemy_C`是当前唯一敌人的蓝图生成类；`RequestHitReact`是已存在的BlueprintCallable原生接口，不是本批新作弊函数。此调用只请求正式受击，**不扣血**，不能替代上面的真实左键命中。
4. 预期`Body action tag ... State.HitReact Count=1`，紧接Gate `Allowed=0 Reason=Busy`与当前Move finished；身体立刻停步 / 不残留追击速度。受击结束Count=0后，在距离仍大于200cm / 仍可见时Gate重新Allowed=1，并有下一次Move request。不要把停止在150cm的受击当作“移动中取消”样本。
5. 输入`slomo 1`恢复再继续。若本轮受击请求未进入、已先到达150cm或失去视觉，只重做这一小项；明确反馈“没有取得移动中受击样本”，不为它反复改测试图。

然后恢复正常速度、靠近敌人，用正式左键将其生命扣到0：应一次BodyDead清目标、`Tree stopped ... Reason=BodyDead`；尸体不继续走，也不继续发Move request。只测这条新增BT停止，不重复整个Health夹限矩阵。停止PIE后重开，敌人100/100并能重新追击。

### 8.4 目标失效、暂停 / 重开与短基线（约3–6分钟）

1. 敌人追击时按P。世界暂停，目标可保持；等约2秒，鼠标点继续，能恢复原追击或按当前距离停步，玩家立即可操作。再做一次P → Enter / Space继续，不要求十次菜单重建。
2. 敌人存活且有目标时，复用数字4对玩家造成25伤害，按到玩家0血（当前正式默认值）。应有B1 `TargetDead`清目标、Gate关闭、当前路径取消；敌人留在等待，不再追尸体。树没有自己死亡，不要求`Tree stopped`。
3. Stop PIE，重开：双方100/100，初始黑板独立清空后重新感知；无上一轮Move回调 / 旧目标恢复。再取得一次目标，**在追击中Stop PIE**，应有整树停止和Controller结束清理，无结束后重新Move request。
4. 最后重开做一次短基线：HUD100/100、一份HUD；移动 / 转向 / 跳跃、E提示与P正常；敌人可感知并追击。Stop PIE。

**总清理：**`slomo 1`、无ET_TempNavBlock、Nav_ET_Room恢复并Build Paths、P导航显示关闭、双方默认100/100、数字4仍Damage25、Draw Debug=False、单敌人、单导航体；BT / BB / Controller BP及地图Save All。M0组合动作、HUD重建、换身体、三帧率闪避、打包均不重做。

## 9. 常见定位顺序与反馈

| 现象 | 优先检查 |
| --- | --- |
| 启动Invalid pursuit keys | 黑板两个新增键的精确名称 / Bool与Float类型 / 未同步；Controller是否仍配置同一BB |
| Tree config rejected | Enemy Behavior Tree是否赋值、BT黑板是否同一资产、ROOT是否连接合法Selector、Resume是否大于Stop+1 |
| 有Target但不追 | Gate日志原因、身体地面状态 / 忙碌标签、Service是否挂根Selector、左分支优先级、Decorator是否Both |
| Accepted=1但BT不活跃 | Debug Filter是否本轮Controller、根线是否接好、Service / Task是否原生新类；Accepted不代替运行观察 |
| Radius不是150 | Move To是否绑定Float键PursuitStopDistance，是否误用普通Move To，Controller参数是否150 |
| 约234cm停步 | 两个Reach Test Includes Radius是否错误勾选，导致150再加双方42cm胶囊 |
| Failed密集刷日志 | 右Wait是否0.5、Sequence / Selector位置是否正确、是否删除了等待节点；正常无路径不每帧重试 |
| 受击动画中仍移动 | Tag=State.HitReact是否出现、Controller三订阅是否完整、StopMovement与StopMovementImmediately是否都保留 |
| 永远原地转动 / 踏步 | 是否真实NavMesh覆盖、静态箱体是否影响导航、目标可导航、Enemy速度220及Movement mode；绿色覆盖仍需真实请求验证 |
| ke没有受击 | 使用游戏控制台而非只看Output Log；类名是否BP_ETMeleeEnemy_C；旧原生RequestHitReact是否存在 / 目标是否已死 / 仍忙碌 |

报错发文字为主：第一条具体编译错误或运行日志前后10–20行，说明所在章节、操作和实际现象。**BT树形连接 / Details属性**定位用截图更清楚：截图包含ROOT、左右分支、附着条和被选节点Details；移动滑行 / 停走异常可加5–15秒视频。不要只发`Build failed`或“全部没反应”。

不需要逐段摘完整日志。完成后发下面简表与**完整时分**，我读取本地日志核对关键顺序，区分你反馈和直接日志证据；若亚秒样本仍难取得，告诉我具体缺失，我再判断是否需要直接测试。

```text
M1-05B2 完成
Editor Build / 索引25cpp / Game Build：
BB三键 / 原生Service-Task / 根Service / Both / Radius绑定：
真实追击与移动目标 / 150停-200恢复：
低障碍绕行 / 无路径失败等待 / 导航已恢复：
移动中受击停走-结束重判 / 真实左键伤害：
敌人死亡停树 / 玩家死亡停追 / 重开恢复：
追击中P鼠标-键盘继续 / 停止清理 / 短基线：
slomo1、临时箱删除、导航与默认值恢复、全部保存：
失败或特别说明：
大致完成时间（完整时分，如21:18）：
实际用时：代码__ / 资产__ / 检查__；构建等待__分钟
```

## 10. 本批设计核查与后续边界

本指南生成时已读取当前Controller / Character / GAS标签，并对本机5.5.4的AIController、BTService、BTTask_MoveTo、Blackboard、ValueOrBBKey及编辑器属性定制实现做静态API核查。**未将指南代码输入Source，也未编译 / 运行；以上运行预期仍需要你实施验证。**源文件 / 资产仍以[B1已验收基线](Reports/Raw/M1_05B1_Baseline.json)为准。

上述为初版生成时的状态。后续用户已输入六文件并完成双Build / PIE；当前51Source / 25cpp / 清单39，六文件内容与指南一致，两个辅助头文件实际位于Public/AI。最新结果以[复核报告](Reports/M1_05B2_Verification.md)为准。第11节两项窄补与第12节连接补修已执行，20:27全部通过，最终结果见第13节；其他已通过项保留。

此设计便于面试解释：目标与状态真值归身体 / GAS，Controller提交决策，BT只组织意图，原生Move To管理异步任务与取消；节点模板共享而每个敌人的状态不共享；事件处理高优先级中断，低频Service处理距离；失败分支控制重试频率。这里只是设计上的成本控制，没有做Unreal Insights测量，不宣称已完成性能优化。

B2已由用户实施 / 补修并验收，见第13节；下一批准备C。C预计分批接停步朝向、AI可用轻攻击、玩家命中碰撞与Task完成 / 取消；D再组合交战与最终证据。不会用本批追击成功代替敌人已具备真实攻击。

## 11. 两项窄补：用户选择手动执行（历史，已完成）

2026-10-08选择记录：你选择亲自补测。原本已通过的追击 / 停走、伤害 / 两种死亡、暂停 / 重开全部保留；不重输C++、不Build、不重配BT、不重复四组全验收。预计额外3–6分钟，实际发生后另记。

### 11.1 用0.05慢速取得真正的“移动中受击”样本

1. 正式地图正常导航，**新开一次Play**，先按`~`打开游戏控制台（不是只看Output Log），粘贴`slomo 0.05` → Enter → `~`关闭。先设慢速再进入视野，避免敌人在你输入期间已经到达。
2. 走入敌人正面视野，看到敌人开始追击就**松开全部移动键、站定**。此时确认它还在行走、离你明显超过200cm；不要走到它面前。慢速下220cm/s变为约11cm/s现实速度，从600cm接近至150cm约40秒，足够操作。
3. 打开控制台，粘贴下行，Enter并关闭：

```text
ke BP_ETMeleeEnemy_C RequestHitReact
```

4. 观察敌人立即停追 / 受击。只需确认这段日志具备：**先Move request/InProgress → HitReact Count1 → Busy /Allowed0及当前Move结束 → Count0 → CanChase /Allowed1与新Move request**。Move finished与Gate日志的相互顺序可变；关键是受击前没有Succeeded / NearTarget先结束路径。Count0时玩家距敌人仍大于200cm，因此应该恢复。
5. 此慢速下原约0.63秒受击持续约13秒现实时间，耐心等结束，不连点受击。最后输入`slomo 1`，关闭控制台，Stop PIE。无需复制长日志，记下大致完整时分。

若一开始已有NearTarget，说明样本仍是原地受击：重开这一小项即可，不改代码。本次更慢且先设速度，避免上一轮19:48:57停步、19:49:01才请求的时点问题。

### 11.2 保持目标可见，稳定制造无路径

1. **停止PIE**，不放箱子。在正式地图选中`Nav_ET_Room`，记下原Location、Scale和Brush X/Y/Z。标准原值Location(0,0,150)、Scale1、Brush2000/2000/500。
2. 临时设**Location(600,0,150)、Scale1、Brush X/Y/Z=600/2000/500**。Build → Build Paths，按P查看：绿色主要在X300–900，敌人出生在绿区；西侧X0没有绿色。不能只看绿色颜色不核对范围；如果X0仍有绿色，先处理导航重建，不继续本项。
3. **新开Play，保持正常速度slomo1**。玩家站到约X0 /Y250、敌人西侧正前方；绿色区在敌人一侧，**玩家不要跨进X≥300**。双方距离约600cm、无箱 / 高墙遮挡，敌人朝向西边，目标应该持续可见。
4. 玩家松开移动键，站定约3秒。预期TargetActor一直为玩家、Gate Allowed1；日志连续出现至少**3次独立Move request Result=Failed与Move finished Failed**，相邻请求约0.5秒。期间不应夹有SightLost / Target cleared；若有，这仍是视觉取消样本，需要先回到敌人正面保持可见。
5. Stop，**恢复第一步记录的导航体全部原值** → Build Paths → P确认全房间覆盖恢复 → 再按P关闭显示 → Save All。最后Play一次确认普通追击可恢复，Stop；这一轮不是重跑其他旧功能。

反馈只需：

```text
M1-05B2 两项窄补验
活动Move → Busy取消 → 受击结束恢复新Move：通过 / 实际现象
目标稳定、无SightLost、三次Failed约0.5秒间隔：通过 / 实际现象
slomo1、导航恢复并Build Paths / Save All、普通追击恢复：
大致完成时间（完整时分）：
额外手动用时：__分钟
```

日志我自行读取。若仍无法得到其中一个条件，直接说缺哪一个，不反复盲试；源码与其他已通过项保持当前版本。

## 12. 20:09补测后的等待分支连线补修（历史，已完成）

**11.1已通过，不再重测。**日志确认活动Move在20:06:28被Busy取消，20:06:40受击结束后重新追击。11.2已取得持续可见目标的失败样本，但20:08:06.671–20:08:18.571提交了358次失败请求，相邻平均0.0333秒，未达到0.5秒。

助手只读解析当前已保存的BT运行节点，确认：根Selector只有一个子节点Sequence；Sequence有三个子任务，依次为Move、IdleOrRetry、ArrivalSettle。两个Wait常量分别0.5与0.25，**数值正确，IdleOrRetry的父节点错误**。Sequence遇到Move失败会直接返回，不执行它后面的Wait；这是本轮重试过快的原因。第6.2节已补充“沿线检查父节点”，避免仅凭节点摆放位置判断连接。

### 12.1 只移动IdleOrRetry的父连接，无需Build

1. **Stop PIE**。打开`/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy`。找到Node Name为`IdleOrRetry`、Wait Time为**0.5**的Wait；不要选0.25的`ArrivalSettle`。
2. 鼠标移到**IdleOrRetry节点顶部的输入连接点**，按住**Alt + 鼠标左键单击这个连接点**，断开它与Sequence的线。UE5.5.4引擎的GraphPin操作已核对支持此快捷方式。不要点Sequence底部输出点，否则会一起断开Move与ArrivalSettle。此时IdleOrRetry顶部应无连线，节点保留。
3. 将IdleOrRetry拖到**根Selector的右下方、左Sequence的右侧**。拖动只调整位置，不会自动改变父连接。
4. 从**Selector底部输出连接点**按住左键拖到**IdleOrRetry顶部输入连接点**，松开。确认新增线的上端在Selector，不在Sequence。
5. 沿线逐个核对，最终结构必须如下：

```text
ROOT
  └─ Selector: PursuitOrWait                  ← 下方分出两条线
       [Service: ET Pursuit Decision]
       ├─ Sequence: PursuePlayer              ← 左，下面仅两个任务
       │    [Decorator: CanPursue Is Set / Both]
       │    ├─ ET Pursuit Move To             ← 左
       │    └─ Wait: ArrivalSettle (0.25s)     ← 右
       └─ Wait: IdleOrRetry (0.5s)             ← 右，直接连接Selector
```

6. 两个Wait的Random Deviation仍为**0.0**、数值使用常量**None**。Service仍附在Selector内，Decorator仍附在Sequence内，Observer Aborts仍为Both；Move参数、黑板、Controller配置保留。
7. 点击BT工具栏**Save**。没有Widget式Compile步骤；**不需要关闭UE、修改C++、刷新索引或重新Build**。

### 12.2 仅复查失败等待并恢复导航（约3–5分钟）

1. 正式地图选`Nav_ET_Room`，记下当前完整原值。标准为Location(0,0,150)、Rotation0、Scale1、Brush X/Y/Z=2000/2000/500；合法调整过的场景以自己原值为准。
2. 临时设Location(600,0,150)、Scale1、Brush X/Y/Z=600/2000/500，Build → **Build Paths**。P查看绿色只覆盖约X300–900、包含敌人，X0不覆盖；再关P。
3. 新开Play，游戏控制台输入`slomo 1`。玩家站到约X0/Y250，处于敌人正面视野，双方无障碍遮挡；不进入X≥300绿区。松开移动键、站定**约3秒**，不打敌人、不制造死亡。
4. 目标应持续为当前玩家、CanPursue=True；Move失败后，BT调试视图应激活**右侧IdleOrRetry**，等约0.5秒再尝试左侧Move。日志应出现至少三次`Move request ... Result=Failed`，相邻约0.5秒而非0.033秒；普通帧调度允许少量偏差，期间没有SightLost / Target cleared。**不需要你手算或复制长日志，我会按完成时间读取。**
5. Stop，恢复导航体第一步记录的全部原值 → Build Paths → P确认覆盖整房间后关闭显示 → **Save All**。再Play一次看到普通追击即可，Stop。此轮不用重复受击、伤害、死亡、暂停和其他旧矩阵。

反馈格式：

```text
M1-05B2 等待分支补修
Selector两个子节点、Sequence两个任务；IdleOrRetry直连Selector并保存：是
目标稳定无SightLost，失败后右侧Wait等待再尝试：通过 / 实际现象
slomo1、导航恢复 / Build Paths / Save All，普通追击恢复：
大致完成时间（完整时分）：
额外手动用时：__分钟
```

只关闭这一个实际失败项后进入C。本次已发生的窄补5分钟已计入；补修与再测工时等反馈后再计，不提前标成功。

## 13. 最终验收：20:27补修通过，无需继续操作B2

用户已改接IdleOrRetry并保存，额外手动2分钟。助手核对已保存运行节点：Selector两个子节点为Sequence / IdleOrRetry，Sequence两个任务为Move / ArrivalSettle；IdleOrRetry的Parent已为Selector，Wait0.5保持正确。

日志20:27:21.845–24.595连续6次失败请求，5个间隔0.549–0.551秒、平均0.550秒，目标持续有效无SightLost；失败等待已生效。0.5是Wait本身，请求之间另有行为树完成 / 搜索与帧调度开销，不要求精确0.500秒。导航恢复重建与MapCheck零错误 / 警告有记录，最终Save All / 普通追击恢复据用户反馈通过。第11.1活动Move受击取消 / 恢复结果保留。

**B2全部待补关闭。**51Source不变、清单39资产仅BT / 地图更新哈希；本批手动25 + 窄补5 + 补修2 = **32分钟**，等待5，项目累计1451 / 等待110分钟。详见[最终报告](Reports/M1_05B2_Verification.md)与[验收基线](Reports/Raw/M1_05B2_Baseline.json)。不重做第11–12节，也无需本轮C++ / Build；下一开发批次为M1-05C：AI近战攻击与玩家受伤。
