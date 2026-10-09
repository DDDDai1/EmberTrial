# M1-05B1：视觉感知与有效黑板目标

日期：2026-10-08；UE **5.5.4**。前置：[M1-05A已验收](Reports/M1_05A_Verification.md)。**状态：用户实施完成，B1验收通过。**[验收记录](Reports/M1_05B1_Verification.md)已核对双Build / 索引23cpp、47Source / 38资产；第7.4节由助手经授权补查有效目标的暂停 / 结束 / 重开闭环，已停止PIE并Save All。代码仍由用户手动输入，助手未改C++或运行新Build；下文保留完整实施步骤。

## 1. 成果、时间与边界

敌人能在前方范围内发现存活玩家，将其写入自己Blackboard的`TargetActor`；视线丢失 / 超出保持距离 / 玩家死亡 / 目标结束时清除目标。敌人自身死亡或Ready撤销时也清理。只识别当前由本地PlayerController控制的合法身体，不把旁边训练目标或自己当玩家。

**本批仍不移动 / 攻击。**B2复用这里的感知、黑板与身体接Behavior Tree / Move To，不用为了观察黑板先造一个临时BT或PlayerController测试函数。Light Attack Ability Class保持None，玩家Trace配置不改。

预计手动65–110分钟：代码30–50、资产25–40、首次感知检查10–20；构建等待 / 排障另计。只修改既有AIController两文件，Source仍47份 / 23cpp；新增两份资产：BB_ETMeleeEnemy、BP_ETMeleeAIController，修改原敌人BP的Controller Class。旧战斗、UI、输入和Build.cs均保留。

## 2. 数据与生命周期规则

| 对象 | 本批职责 |
| --- | --- |
| 原生EnemyPerception / SightConfig | 视觉范围 / 角度 / 遮挡检测，事件通知；不直接扣血或发Move To |
| ETAIController | 校验当前身体 / 存活本地玩家、维护唯一弱目标与自己的ASC死亡事件句柄、主动解绑 |
| BB_ETMeleeEnemy | TargetActor的决策快照；Object / Actor类型，Instance Synced=False，各Controller独立 |
| BP_ETMeleeAIController | 选择Blackboard资产；无Event Graph逻辑，不额外Add感知组件 |
| BP_ETMeleeEnemy | 改AI Controller Class，其余已验收装配继续使用 |

视线丢失事件到达即清除，不等待Max Age记忆过期，也不按最后位置穿墙追。感知由引擎按预算更新，存在帧 / 更新延迟；不承诺刚跨边界的同一毫秒必定收到事件。死亡清理使用当前ASC的State.Dead事件，不通过Ready=false隐藏玩家0血HUD。

先订阅 / 提交目标引用，再写黑板；切目标先移除旧EndPlay / 死亡句柄。结束先标记Ending、解绑，再调用Super。源身体、目标ASC各拥有一个句柄，不能因为使用同一个回调就混用句柄或清除别人的Tag。这里只读生命 / Tag，不增加或移除State.Dead。

本批不接入UnPossess / 重生全流程；Getter每次校验目标仍被本地玩家控制，不把旧身体当合法目标。正式身体替换与异步Ready后的主动重选将在对应阶段补齐；当前初始化同步、离线单人，不宣称已支持联机 / 多玩家。

## 3. 保存、关闭UE，替换原Controller两文件

Save All后关闭UE，CLion保留原工程。以下**整体替换现有两文件**，不是新增同名类，也不是在旧函数下面再粘一套。原A日志 / Ready快照与结束保护已包含在全文。

路径分别为：

- `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\AI\ETAIController.h`
- `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETAIController.cpp`

### 3.1 ETAIController.h全文

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

UCLASS()
class EMBERTRIAL_API AETAIController : public AAIController
{
    GENERATED_BODY()

public:
    AETAIController(const FObjectInitializer& ObjectInitializer =
        FObjectInitializer::Get());

    UFUNCTION(BlueprintPure, Category = "EmberTrial|AI")
    AEmberTrialCharacter* GetSensedPlayer() const;

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

    TWeakObjectPtr<AEmberTrialCharacter> ObservedBody;
    TWeakObjectPtr<AEmberTrialCharacter> SensedPlayer;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedBodyASC;
    TWeakObjectPtr<UAbilitySystemComponent> ObservedTargetASC;
    FDelegateHandle BodyDeadHandle;
    FDelegateHandle TargetDeadHandle;
    bool bEndingPlay = false;
    bool bBlackboardReady = false;

    UFUNCTION()
    void HandleBodyReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);

    UFUNCTION()
    void HandleSightUpdated(AActor* SensedActor, FAIStimulus Stimulus);

    UFUNCTION()
    void HandleTargetEndPlay(AActor* EndedTarget,
        EEndPlayReason::Type PlayEndReason);

    void HandleDeadTagChanged(FGameplayTag ChangedTag, int32 NewCount);
    bool InitializeEnemyBlackboard();
    bool CanObservePlayer() const;
    void ObserveBodyDeath();
    void RefreshVisiblePlayer();
    void SetSensedPlayer(AEmberTrialCharacter* NewPlayer, const TCHAR* Reason);
    void ClearSensedPlayer(const TCHAR* Reason);
    void StopObservingBody();
    void LogBodySnapshot(const TCHAR* Reason) const;
};
```

### 3.2 ETAIController.cpp全文

```cpp
#include "AI/ETAIController.h"
#include "Characters/EmberTrialCharacter.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/PlayerController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"

DEFINE_LOG_CATEGORY_STATIC(LogETAIController, Log, All);

namespace
{
    const FName TargetActorKey(TEXT("TargetActor"));

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

    void RemoveDeadSubscription(TWeakObjectPtr<UAbilitySystemComponent>& WeakASC,
        FDelegateHandle& Subscription)
    {
        UAbilitySystemComponent* OldASC = WeakASC.Get();
        const FDelegateHandle OldSubscription = Subscription;
        WeakASC.Reset();
        Subscription.Reset();
        if (IsValid(OldASC) && OldSubscription.IsValid())
        {
            OldASC->UnregisterGameplayTagEvent(OldSubscription,
                ETGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved);
        }
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
    SightPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(
        this, &ThisClass::HandleSightUpdated);
    RefreshVisiblePlayer();
}

void AETAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
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
    ObserveBodyDeath();
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] Possessed Controller=%s Body=%s Authority=%d"),
        *GetNameSafe(this), *GetNameSafe(BodyCharacter), HasAuthority() ? 1 : 0);
    LogBodySnapshot(TEXT("PossessSnapshot"));
    RefreshVisiblePlayer();
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
    Board->ClearValue(TargetActorKey);
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
        ClearSensedPlayer(TEXT("BodyNotReady"));
        RemoveDeadSubscription(ObservedBodyASC, BodyDeadHandle);
        StopMovement();
    }
    else
    {
        ObserveBodyDeath();
        RefreshVisiblePlayer();
    }
    LogBodySnapshot(TEXT("ReadyChanged"));
}

void AETAIController::ObserveBodyDeath()
{
    AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    if (bEndingPlay || !IsValid(BodyCharacter) || !BodyCharacter->IsGameplayReady())
    {
        return;
    }
    UAbilitySystemComponent* BodyASC = BodyCharacter->GetAbilitySystemComponent();
    if (!IsValid(BodyASC) ||
        (ObservedBodyASC.Get() == BodyASC && BodyDeadHandle.IsValid()))
    {
        return;
    }
    RemoveDeadSubscription(ObservedBodyASC, BodyDeadHandle);
    ObservedBodyASC = BodyASC;
    BodyDeadHandle = BodyASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleDeadTagChanged);
    if (BodyASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
    {
        ClearSensedPlayer(TEXT("BodyAlreadyDead"));
        StopMovement();
    }
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
}

void AETAIController::ClearSensedPlayer(const TCHAR* Reason)
{
    AEmberTrialCharacter* OldPlayer = SensedPlayer.Get();
    const bool bHadTarget = !SensedPlayer.IsExplicitlyNull();
    const FString OldPlayerName = GetNameSafe(OldPlayer);
    SensedPlayer.Reset();
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
        StopMovement();
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
    RemoveDeadSubscription(ObservedBodyASC, BodyDeadHandle);
    if (IsValid(OldBody))
    {
        OldBody->OnGameplayReadyChanged.RemoveDynamic(
            this, &ThisClass::HandleBodyReadyChanged);
    }
}

void AETAIController::OnUnPossess()
{
    const FString OldBodyName = GetNameSafe(ObservedBody.Get());
    ClearSensedPlayer(TEXT("BodyUnpossess"));
    StopMovement();
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
    ClearSensedPlayer(TEXT("ControllerEndPlay"));
    StopMovement();
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

Native Tag回调不需要UFUNCTION；两个动态感知 / EndPlay回调需要。generated.h始终最后一条include，保留BodyCharacter命名，不改回Character。`Get(true)`只用于销毁时比对身份，不借它访问已经失效的目标。

900 / 1100cm分别为初次发现与已发现后的保持半径，70°是视锥半角、总角度140°。Auto Success=-1关闭“附近旧位置自动看见”；Max Age=2秒是引擎记忆配置，不是本代码的延迟丢失。Affiliation三项True是为了能看见当前NoTeam玩家，再由原生规则过滤合法玩家；不等于所有Pawn都能成为目标，不提前搭阵营系统。

## 4. 正常Build与索引

保存两文件，UE关闭。PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后Reload原CLion CMake Project，索引仍23cpp；Ctrl+B确认AIPerceptionTypes.h / BlackboardData.h能导航。再执行：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

成功后打开UE。不改Build.cs / Target / 插件列表；A已加Public AIModule，GameplayTags / GameplayAbilities已存在。失败先发第一条具体错误文字；未成功前不要Play或继续创建配置资产，不清缓存。

## 5. Blackboard资产：只有一个新增目标键

1. Content Browser创建`/Game/EmberTrial/AI/Blackboards`，目录空白处右键 → **Artificial Intelligence → Blackboard**，命名`BB_ETMeleeEnemy`。
2. 打开资产，Parent保持None。左侧Blackboard面板点击**New Key → Object**，命名**TargetActor**，大小写精确一致；不要选择Class / Vector / Enum。
3. 选中TargetActor，在右侧Key Details：Key Type展开，**Base Class=Actor**；Default Value保持None；**Instance Synced=False**，Entry Description可填`Currently visible live local player`。
4. 自动SelfActor项若存在就保留；没有则不手动补。除了TargetActor不加Health / Dead / Attacking或测试键。
5. Save。Blackboard编辑器默认显示None并不证明运行时没选目标；本批通过C++写入后的实际读取日志验证，无需先运行BT才能观察。

完整资产路径`/Game/EmberTrial/AI/Blackboards/BB_ETMeleeEnemy`，磁盘路径`E:\UE_Project\Demo\EmberTrial\Content\EmberTrial\AI\Blackboards\BB_ETMeleeEnemy.uasset`。

## 6. Controller Blueprint与敌人装配

### 6.1 从原生Controller创建配置资产

1. 创建`/Game/EmberTrial/AI/Controllers`，右键 → Blueprint Class → All Classes，搜索并选择原生**ETAIController**，命名`BP_ETMeleeAIController`。不选默认AIController / ETPlayerController，不Duplicate身体资产。
2. Class Settings确认Parent Class=ETAIController。
3. 点击**Class Defaults**，搜索`Enemy Blackboard Asset`，选择第5节的**BB_ETMeleeEnemy**，这是Blackboard对象资产，不是Blueprint类。清空搜索后检查AI分类：Start AI Logic on Possess=False，Stop AI Logic on Unposses=True，保留原生默认。
4. Components应有一个继承感知组件：原生子对象名**EnemyPerception**，界面也可能按成员名显示**SightPerception / Sight Perception**。选中后应为AIPerception Component；这几种名称指同一个组件，不另Add AIPerception。Senses Config只有一项AISenseConfig_Sight，检查Sight Radius900、Lose Sight Radius1100、Peripheral Vision Angle70、三Affiliation勾选、Dominant Sense=AISense_Sight。这些已有原生默认，本批不另调值；没看到Senses Config时清空搜索并展开组件AI Perception分类。
5. Event Graph保持无业务执行线，**不接BeginPlay → Run Behavior Tree / Move To / SetValue**。Compile → Save。

完整资产路径`/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController`。配置BP是为了让原生逻辑选择资源，不把资产路径写死进C++构造函数。

### 6.2 修改现有敌人的Controller Class

1. 打开原`BP_ETMeleeEnemy`，**Class Defaults → AI Controller Class**改为刚创建的**BP_ETMeleeAIController**。
2. Auto Possess AI仍Placed in World or Spawned，Auto Possess Player / Auto Receive Input仍Disabled；初值100/100、Light Attack Ability Class=None、Hit Class原GA、单组件 / 胶囊 / Quinn / Movement220、Debug False均保持。
3. Compile → Save。正式地图选MeleeEnemy_A，检查实例AI Controller Class也显示BP_ETMeleeAIController；若仍旧ETAIController且有黄色恢复箭头，对**这一项**Restore to Default，不改整个Actor Transform。
4. Save All。原GameMode / 玩家 / PlayerStart / 导航体保持。新Controller只在PIE由Auto Possess生成，不把Controller Blueprint拖进地图。

本批没有Blueprint白色执行线、Target或IsValid要搭建；所有事件连线在C++，BP只负责以上资源 / 参数。无需新建任何验收变量 / 函数。

## 7. 最小分组检查，首次感知约10–20分钟

Output Log筛选`M1-05B1`；需要观察死亡 / HUD时取消筛选。保留磁盘日志，不需要手抄所有行。敌人站在A默认600 / 250，Yaw180朝-X；玩家默认-600 / 0，距离约12m，因此**初次出生不应立即获取目标**。若你的摆放与此不同，以实际9m / 11m范围解释，不为了得到相同日志改已保存布局。

### 7.1 范围、合法玩家与目标去重

**准备：**正式地图单敌人，新Controller BP / 黑板配置完成；无临时挡板，100/100，正常速度。Default Player Start。

**操作：**Play，等约1秒；向敌人的-X一侧走近，进入约9m前方范围后站定2秒；再退远到11m以外，停约1秒，然后再走近。不要使用P作编辑器导航显示，此时P是游戏暂停。

**预期：**一条Blackboard ready，出生Ready100正常，初始无Target acquired；靠近出现Target acquired，Player和BlackboardTarget均是**BP_ThirdPersonCharacter...**。站定不持续重复acquired；退远SightLost / BlackboardTarget=None，重进再次acquired。旁边TrainingTarget没有玩家Controller，不能写入TargetActor。敌人仍原地站立，不追击 / 反击，玩家HUD100。

**清理：**本组不改配置；Stop后进入下一组准备。没有acquired先看Controller实际为新BP、Blackboard ready与Key类型、敌人面朝玩家、距离；不要把感知半径改成99999掩盖问题。

### 7.2 墙遮挡与视线重新获得

**准备：**Stop，添加一个临时Cube名`SM_ET_TempSightWall`：Location **430 / 250 / 150**，Rotation0 / 0 / 0，Scale **0.1 / 4 / 3**，Static / BlockAll / Simulate Physics=False。这是一块厚10、宽400、高300cm的挡板，位于敌人西侧170cm，不穿进胶囊。

**操作：**Play，玩家在挡板西侧、约X300 / Y250处站立，使视线穿过墙；再从同一西侧绕到约X300 / Y650，确保敌人到玩家的直线绕过墙的北边缘；然后回到墙后。用场景位置作参考即可，不为精确坐标新搭打印函数 / 改出生点。

**预期：**墙后不能新获取 / 当前目标丢失；绕开遮挡且仍在前方范围时获得，回墙后SightLost清空。不是要看到敌人走绕墙，本批没有寻路。不要把墙Visibility改Ignore或给Pawn增加临时Stimuli组件试错。

**清理：**Stop，删除临时墙，Build → Build Paths恢复正式房间导航、Save All。保持玩家与敌人 / 导航原位置，不保存临时挡板。

### 7.3 两种死亡清理，复用正式输入

**准备：**无临时墙。新PIE，先靠近取得玩家目标。

**操作A：**原数字4对玩家扣25，等受击结束后再按，累计四次到0；不改4图或GE。随后Stop。

**预期A：**玩家HUD100→75→50→25→0，现有Death处理正常；Controller有一次Target cleared / **Reason=TargetDead**，之后不重新获得尸体。Tag事件与Death committed日志是同步调用链，先后不强求一致；Ready仍可True，不能因此继续选择0血玩家。

**操作B：**再Play、玩家 / 敌人100重置，走近先取得目标；用正式左键四次命中敌人，每次等动作结束，沿用A已确认站位。敌人到0后看B1日志。

**预期B：**正式受击 / 死亡仍沿用A，Controller清目标**Reason=BodyDead**，不再acquired；玩家HUD100。只补新增AI死亡订阅的结果，不重复A整章的所有碰撞 / 动画检查。

**清理：**Stop，默认值100/100未改。此组只证明感知 / 黑板清理；“玩家死亡后停止追击 / 当前攻击”待B2 / C / D接入实际行动后验证，不提前标为通过。

### 7.4 带有效目标停止、重开与短基线

**准备：**新PIE取得目标，两个身体存活。

**操作：**P打开 / 继续一次，恢复移动转向；在目标仍有效时Stop，再Play走近正常取得一次目标。最后Stop，双BP Compile → Save，Save All。

**预期：**暂停菜单与输入恢复正常；结束清理目标 / 两份死亡句柄 / 感知订阅，End后没有同轮新acquired。下一轮独立Blackboard / 身体100，无旧事件串入。结束可能先TargetEndPlay、BodyNotReady、BodyUnpossess或ControllerEndPlay清理，**不要求固定Reason / 日志先后**；每次已经清空后重复Clear不再打印一条旧目标。没有Accessed None / 依赖Error。感知事件不是每帧距离日志。

**清理：**无临时墙、导航恢复、单敌人 / 单导航体，原3 / 4保留，100/100 / Debug False保存；没有新测试节点要删除。不重跑HUD重建、换身体、MaxHealth矩阵、闪避帧率或新包。

## 8. 反馈与错误材料

```text
M1-05B1 完成 / 卡在第__节
Editor Build / 索引23cpp / Game Build：
黑板TargetActor Object-Actor / 非同步 / 新Controller父类与资源：
原敌人新AI Class / 单EnemyPerception / 无蓝图业务：
范围获得-丢失-重获 / 站定不重复 / 不选训练目标：
墙遮挡与重获，挡板已删除 / 导航已恢复：
玩家死亡TargetDead / 敌人死亡BodyDead：
带目标停止重开 / P与输入短基线：
默认100/100、Debug False、资产已保存：
失败或特别说明：
大致完成时间（完整时分）：
实际用时：代码__ / 资产__ / 检查__；构建等待__分钟
```

编译发第一条实际错误文字；Blackboard配置错误发TargetActor的Key Type / Base Class与Controller Class Defaults截图；无感知发新Controller组件Sight设置和敌人朝向 / 实际摆放截图。短视频仅用于难以描述的进出范围 / 遮挡现象，日志由助手按时间读取。普通报错不自动授权助手直接改C++。

## 9. 接口核对与下一批

已对照本机5.5.4的AIController / AIPerceptionComponent / AISense_Sight、BlackboardComponent / BlackboardData / Object Key、ASC Tag注册 / 注销及Actor.OnEndPlay声明核对；反射回调参数避免AController.Character / Pawn等继承成员重名。默认Sight自动登记Pawn来源，项目配置没有关闭该选项，因此不额外给玩家Add Stimuli Sources。以后改变自动来源策略时再显式接入，不能把默认条件误称为所有项目通用保证。

接口核对原为静态设计；现在用户双Build / 运行与助手7.4补查均已完成，见[验收记录](Reports/M1_05B1_Verification.md)。Source仍47，新增BB / Controller后资产清单38，已保存[B1基线](Reports/Raw/M1_05B1_Baseline.json)，实际手动15 / 等待5分钟。B2将使用同黑板和Getter，增加BT决策与真实Move To，接忙碌停走 / 恢复和无路径退路；追击通过后C才扩展共享GA到离线AI攻击。上文代码与原预算保留，不把B2提前记为完成。
