# M2-06C：左右切换目标与交战收口

更新2026-10-25，UE5.5.4。用户已报告本章检查完成，随后发现出生首帧陷地与贴身镜头侧转，已明确授权助手直接补修。**本轮双Build / 四项相关自动化与实际PIE取景通过；首次冷PIE创建等待仍保留。**具体原因、证据和提交范围见10.1及11节，本轮不进入下一章节，也不把窄采样扩写成完整性能或独立包验收。普通后续C++仍按原手动协作约定。

预计手动60–100分钟：代码35–60、资产15–25、检查10–15，构建等待另记。代码长的部分给完整文件，Controller只给精准增量；不搭临时Controller函数或测试蓝图。

## 1. 最终体验与职责

Tab仍负责获取 / 解除；**Z切向当前画面左侧，C切向右侧**。方向相对当前目标在同一相机视点下的水平角，不是世界X / Y、角色左右，也不是按Actor名字循环。

| 规则 | 本章行为 |
| --- | --- |
| 前置 | 当前已锁定、玩家Ready且存活、组件活动、非Pause / 菜单；无锁定时Z / C不代替Tab |
| 候选 | 复用A：同World、Enemy、Ready、存活、身体1200cm内、真实视点±60°三维圆锥、Visibility无遮挡，按Actor去重 |
| 排序 | 剔除当前目标；相对当前目标向指定侧的水平角差必须大于3°，选角差最小者；同角度桶选身体距离更近者，再同分按对象路径稳定决胜 |
| 水平角 | 使用相机前 / 右向量的Atan2；目标中心在镜头前才比较。圆锥不等同屏幕矩形，可选范围仍沿用A；不引入UI投影依赖 |
| 不存在候选 | 保持原目标 / 标记 / 肩位，不绕到另一侧、不主动解除、不排队 |
| 限频 | 两方向共享.20游戏秒；有效搜索失败也占本次间隔，间隔内直接拒绝且不查询；长按Started只一次 |
| 切换事务 | 解绑旧目标 / 废弃旧Serial / 清旧Timer，装配新目标 / 一份Timer，最后只广播一次Previous→Current；不广播中间None |
| 当前目标失效 | 沿用A死亡 / Ready撤销 / 销毁 / 越界 / 遮挡宽限清锁；本版不自动跳到旁边敌人，重新Tab获取 |
| 动作中切换 | 允许更新目标与镜头；不改攻击 / 闪避已采样方向，不自动朝目标攻击，不改变命中 / GE / 成本 |
| 生命周期 | Clear / 换身体 / 失活 / 结束清限频；Pause冻结世界时间，不消耗间隔，也不补执行按键 |

Core只拥有一份锁定目标；B1标记订阅既有OnTargetChanged，B2相机继续读取同一目标，原切换功能**不改UI模块、角色、GA、伤害、Build.cs或共享Config**；收尾另按授权调整了右肩近景规则和GameMode出生贴地，见10.1。维持仍每.10游戏秒只查当前目标，无目标无Timer；切换不新增常驻扫描、Tick或Timer，查询为按需线性选优。没有新测量，不宣称帧率提高。

当前实际157 Source / 86cpp、Core60 / UI23 / Editor3、23项自动化、来源75 / 本地294份UE资产。切换原五旧Source不增数量；出生补修新增SpawnPlacement测试。两个IA实物、Controller引用与原IMC的Tab / Z / C映射已只读核对并登记来源；用户未提供C实际分项工时，不补填估算值。

## 2. 保存与五个文件

1. Stop PIE → Save All → 关闭UE，CLion保存。新增枚举 / UFUNCTION / UPROPERTY后走正常UBT / UHT，不用Live Coding。
2. 基线是**右肩补修后的当前工作区**，不要从63abd20还原或覆盖B2相机三个文件。右肩未提交也不阻止本章开发；Commit分批见最后一节。
3. 下表均在`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial`，是已有文件，不创建同名副本。

| 文件 | 操作 |
| --- | --- |
| Public/TargetLock/ETTargetLockComponent.h | 第3节全文替换 |
| Private/TargetLock/ETTargetLockComponent.cpp | 第4节全文替换，原A生命周期已保留 |
| Public/Framework/ETPlayerController.h | 5.1只加两个IA引用与两个处理函数 |
| Private/Framework/ETPlayerController.cpp | 5.2加两函数，5.3替换SetupInputComponent；其余保留 |
| Tests/ETTargetLockTests.cpp | 第6节加一个include及一整块用例；原用例保留 |

上表是原切换功能的五文件范围；本轮授权补修另涉及Camera三文件、GameMode两文件及新SpawnPlacement测试，均已直接写入并构建，不再手动重复录入。不要把相机换回早期Yaw25方案；最终完整范围见11节。

## 3. ETTargetLockComponent.h全文

用下方完整内容替换本文件。generated.h仍是最后一条include；枚举在class之前。原六参数保留，新增Switch Interval / Switch Min Angle在同一组件分类下。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETTargetLockComponent.generated.h"

UENUM(BlueprintType)
enum class EETTargetLockSwitchDirection : uint8
{
    Left,
    Right
};

class APawn;
class APlayerController;
class AEmberTrialCharacter;
class UETCombatLifeComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETLockTargetChanged, AEmberTrialCharacter*,
                                     AEmberTrialCharacter*);

// Controller拥有一份目标身份；不转镜头、不改朝向、不替能力结算伤害。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETTargetLockComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETTargetLockComponent();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Target Lock")
    bool ToggleTargetLock();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Target Lock")
    AEmberTrialCharacter* GetLockedTarget() const;

    // 只切换已有锁定；没有候选时保留当前目标，不缓存未来输入。
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Target Lock")
    bool SwitchTarget(EETTargetLockSwitchDirection Direction);

    FETLockTargetChanged OnTargetChanged;
    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETTargetLockSelectionTest;

    // 搜索与脱锁半径形成迟滞；视角评分只影响首次选择，不在维持时抢换目标。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "1.0", Units = "cm"))
    float AcquireRadius = 1200.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "1.0", Units = "cm"))
    float BreakRadius = 1500.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "1.0", ClampMax = "89.0", Units = "deg"))
    float AcquireHalfAngle = 60.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ViewWeight = .75f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.02", ClampMax = "1.0", Units = "s"))
    float MonitorInterval = .10f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.0", ClampMax = "3.0", Units = "s"))
    float OcclusionGrace = .35f;

    // 两个方向共享同一个游戏时间门限；不添加输入Timer。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.05", ClampMax = "1.0", Units = "s"))
    float SwitchInterval = .20f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.0", ClampMax = "20.0", Units = "deg"))
    float SwitchMinAngle = 3.0f;

    double NextSwitchAllowedAt = 0.0;

    TWeakObjectPtr<APlayerController> BoundController;
    TWeakObjectPtr<AEmberTrialCharacter> BoundSource;
    TWeakObjectPtr<AEmberTrialCharacter> LockedTarget;
    TWeakObjectPtr<UETCombatLifeComponent> SourceLife;
    TWeakObjectPtr<UETCombatLifeComponent> TargetLife;
    FDelegateHandle SourceDeathHandle;
    FDelegateHandle TargetDeathHandle;
    FTimerHandle MonitorTimer;
    double OccludedSince = -1.0;
    uint64 LockSerial = 0;
    bool bEndingPlay = false;

    bool HasValidConfiguration() const;
    bool ResolveRequestContext(AEmberTrialCharacter*& OutSource,
                               APlayerController*& OutController) const;
    bool HasValidSwitchConfiguration() const;
    AEmberTrialCharacter* ResolveSource() const;
    bool IsTargetEligible(const AEmberTrialCharacter* Source, const AEmberTrialCharacter* Target,
                          float Radius) const;
    bool HasVisibility(const AEmberTrialCharacter* Source, const AEmberTrialCharacter* Target,
                       const FVector& ViewLocation) const;
    AEmberTrialCharacter* FindBestTarget(const AEmberTrialCharacter* Source,
                                         const FVector& ViewLocation,
                                         const FVector& ViewForward) const;

    void GatherCandidates(const AEmberTrialCharacter* Source, const FVector& ViewLocation,
                          const FVector& ViewForward,
                          TArray<AEmberTrialCharacter*>& OutCandidates) const;
    AEmberTrialCharacter* FindSwitchTarget(const AEmberTrialCharacter* Source,
                                           const AEmberTrialCharacter* Current,
                                           const FVector& ViewLocation,
                                           const FRotator& ViewRotation,
                                           EETTargetLockSwitchDirection Direction) const;

    // 切换先解绑旧目标再装配新目标，外部只收到一次Previous -> Current。
    void ResetTargetBindings();
    void RebindSource();
    void ClearSourceBindings();
    void SetLockedTarget(AEmberTrialCharacter* Target);
    void ClearTarget(const TCHAR* Reason);
    void MaintainLock(uint64 Serial);
    void HandleDeathSealed(UETCombatLifeComponent* Life);
    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);
    UFUNCTION()
    void HandleBodyReadyChanged(AEmberTrialCharacter* Body, bool bReady);
    UFUNCTION()
    void HandleBodyEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
};
```


## 4. ETTargetLockComponent.cpp全文

用下方完整内容替换本文件。重点阅读三个分区：GatherCandidates复用筛选；FindSwitchTarget按相机水平角线性选优；ResetTargetBindings只做内部释放，由Set / Clear决定对外广播，避免两次事件。

```cpp
#include "TargetLock/ETTargetLockComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogETTargetLock, Log, All);

UETTargetLockComponent::UETTargetLockComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UETTargetLockComponent::BeginPlay()
{
    Super::BeginPlay();
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (IsValid(PC) && PC->IsLocalController())
    {
        BoundController = PC;
        PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnChanged);
        RebindSource();
    }
}

void UETTargetLockComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    ClearTarget(TEXT("EndPlay"));
    ClearSourceBindings();
    if (APlayerController* PC = BoundController.Get())
    {
        PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
    }
    BoundController.Reset();
    Super::EndPlay(EndPlayReason);
}

void UETTargetLockComponent::Deactivate()
{
    // 先封闭请求，防止目标变化监听者同步重入请求新锁定。
    Super::Deactivate();
    ClearTarget(TEXT("Deactivated"));
}

bool UETTargetLockComponent::HasValidConfiguration() const
{
    return FMath::IsFinite(AcquireRadius) && AcquireRadius > 0.0f && FMath::IsFinite(BreakRadius) &&
           BreakRadius >= AcquireRadius && FMath::IsFinite(AcquireHalfAngle) &&
           AcquireHalfAngle >= 1.0f && AcquireHalfAngle <= 89.0f && FMath::IsFinite(ViewWeight) &&
           ViewWeight >= 0.0f && ViewWeight <= 1.0f && FMath::IsFinite(MonitorInterval) &&
           MonitorInterval >= .02f && MonitorInterval <= 1.0f && FMath::IsFinite(OcclusionGrace) &&
           OcclusionGrace >= 0.0f && OcclusionGrace <= 3.0f;
}

AEmberTrialCharacter* UETTargetLockComponent::ResolveSource() const
{
    APlayerController* PC = BoundController.Get();
    AEmberTrialCharacter* Source = PC ? Cast<AEmberTrialCharacter>(PC->GetPawn()) : nullptr;
    return IsValid(PC) && PC->IsLocalController() && IsValid(Source) &&
                   Source == BoundSource.Get() &&
                   Source->GetCombatTeam() == EETCombatTeam::Player && Source->IsGameplayReady() &&
                   !Source->IsCombatDead()
               ? Source
               : nullptr;
}

AEmberTrialCharacter* UETTargetLockComponent::GetLockedTarget() const
{
    AEmberTrialCharacter* Target = LockedTarget.Get();
    return IsValid(Target) && Target->IsGameplayReady() && !Target->IsCombatDead() ? Target
                                                                                   : nullptr;
}

bool UETTargetLockComponent::IsTargetEligible(const AEmberTrialCharacter* Source,
                                              const AEmberTrialCharacter* Target,
                                              float Radius) const
{
    return IsValid(Source) && IsValid(Target) && Target != Source &&
           Target->GetWorld() == Source->GetWorld() &&
           Target->GetCombatTeam() == EETCombatTeam::Enemy && Target->IsGameplayReady() &&
           !Target->IsCombatDead() && !Source->GetActorLocation().ContainsNaN() &&
           !Target->GetActorLocation().ContainsNaN() &&
           FVector::DistSquared(Source->GetActorLocation(), Target->GetActorLocation()) <=
               FMath::Square(static_cast<double>(Radius));
}

bool UETTargetLockComponent::HasVisibility(const AEmberTrialCharacter* Source,
                                           const AEmberTrialCharacter* Target,
                                           const FVector& ViewLocation) const
{
    UWorld* World = GetWorld();
    if (!IsValid(World) || ViewLocation.ContainsNaN() || !IsValid(Target))
    {
        return false;
    }
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ETTargetLockVisibility), false, Source);
    const bool bBlocked = World->LineTraceSingleByChannel(
        Hit, ViewLocation, Target->GetActorLocation(), ECC_Visibility, Params);
    return !bBlocked || Hit.GetActor() == Target;
}

// ---- 请求只读取当前世界 / 身体，不在暂停或菜单中保留输入 ----
bool UETTargetLockComponent::ResolveRequestContext(AEmberTrialCharacter*& OutSource,
                                                   APlayerController*& OutController) const
{
    OutSource = ResolveSource();
    OutController = BoundController.Get();
    const UWorld* World = GetWorld();
    return HasBegunPlay() && !bEndingPlay && IsActive() && IsValid(World) &&
           !World->bIsTearingDown && !World->IsPaused() && IsValid(OutController) &&
           !OutController->bShowMouseCursor && IsValid(OutSource) && HasValidConfiguration();
}

bool UETTargetLockComponent::HasValidSwitchConfiguration() const
{
    return FMath::IsFinite(SwitchInterval) && SwitchInterval >= .05f && SwitchInterval <= 1.0f &&
           FMath::IsFinite(SwitchMinAngle) && SwitchMinAngle >= 0.0f && SwitchMinAngle <= 20.0f;
}

// ---- 获取 / 切换复用同一候选资格，Overlap只在有效请求中执行一次 ----
void UETTargetLockComponent::GatherCandidates(const AEmberTrialCharacter* Source,
                                              const FVector& ViewLocation,
                                              const FVector& ViewForward,
                                              TArray<AEmberTrialCharacter*>& OutCandidates) const
{
    OutCandidates.Reset();
    UWorld* World = GetWorld();
    if (!IsValid(World) || !HasValidConfiguration() || !IsValid(Source) ||
        ViewLocation.ContainsNaN() || ViewForward.ContainsNaN() || ViewForward.IsNearlyZero())
    {
        return;
    }
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ETTargetLockAcquire), false, Source);
    TArray<FOverlapResult> Hits;
    World->OverlapMultiByObjectType(Hits, Source->GetActorLocation(), FQuat::Identity, Objects,
                                    FCollisionShape::MakeSphere(AcquireRadius), Params);
    TSet<AEmberTrialCharacter*> Seen;
    const double MinDot =
        FMath::Cos(FMath::DegreesToRadians(static_cast<double>(AcquireHalfAngle)));
    const FVector Forward = ViewForward.GetSafeNormal();
    for (const FOverlapResult& Hit : Hits)
    {
        AEmberTrialCharacter* Target = Cast<AEmberTrialCharacter>(Hit.GetActor());
        if (!IsTargetEligible(Source, Target, AcquireRadius) || Seen.Contains(Target))
        {
            continue;
        }
        Seen.Add(Target);
        const FVector Offset = Target->GetActorLocation() - ViewLocation;
        if (!Offset.IsNearlyZero() &&
            FVector::DotProduct(Forward, Offset.GetSafeNormal()) >= MinDot &&
            HasVisibility(Source, Target, ViewLocation))
        {
            OutCandidates.Add(Target);
        }
    }
}

// ---- 首次获取保持A的角度 / 距离评分 ----
AEmberTrialCharacter* UETTargetLockComponent::FindBestTarget(const AEmberTrialCharacter* Source,
                                                             const FVector& ViewLocation,
                                                             const FVector& ViewForward) const
{
    TArray<AEmberTrialCharacter*> Candidates;
    GatherCandidates(Source, ViewLocation, ViewForward, Candidates);
    AEmberTrialCharacter* Best = nullptr;
    int64 BestScore = TNumericLimits<int64>::Max();
    constexpr double ScorePrecision = 10000.0;
    const double MinDot =
        FMath::Cos(FMath::DegreesToRadians(static_cast<double>(AcquireHalfAngle)));
    const FVector Forward = ViewForward.GetSafeNormal();
    for (AEmberTrialCharacter* Target : Candidates)
    {
        const FVector Offset = Target->GetActorLocation() - ViewLocation;
        const double Dot =
            FMath::Clamp(FVector::DotProduct(Forward, Offset.GetSafeNormal()), -1.0, 1.0);
        const double AnglePenalty = (1.0 - Dot) / (1.0 - MinDot);
        const double DistancePenalty =
            FVector::Distance(Source->GetActorLocation(), Target->GetActorLocation()) /
            AcquireRadius;
        const double WeightedScore =
            ViewWeight * AnglePenalty + (1.0 - ViewWeight) * DistancePenalty;
        const int64 Score = FMath::RoundToInt64(WeightedScore * ScorePrecision);
        if (!Best || Score < BestScore ||
            (Score == BestScore && Target->GetPathName().Compare(Best->GetPathName()) < 0))
        {
            Best = Target;
            BestScore = Score;
        }
    }
    return Best;
}

// ---- 以同一视点的水平角排序：相对当前目标向左 / 右找最近一个，不从边缘绕回 ----
AEmberTrialCharacter* UETTargetLockComponent::FindSwitchTarget(
    const AEmberTrialCharacter* Source, const AEmberTrialCharacter* Current,
    const FVector& ViewLocation, const FRotator& ViewRotation,
    EETTargetLockSwitchDirection Direction) const
{
    if (!IsValid(Current) || !HasValidSwitchConfiguration() || ViewRotation.ContainsNaN() ||
        (Direction != EETTargetLockSwitchDirection::Left &&
         Direction != EETTargetLockSwitchDirection::Right))
    {
        return nullptr;
    }
    const FVector Forward = ViewRotation.Vector();
    const FVector Right = ViewRotation.RotateVector(FVector::RightVector);
    const FVector CurrentOffset = Current->GetActorLocation() - ViewLocation;
    if (CurrentOffset.ContainsNaN() ||
        FVector::DotProduct(CurrentOffset, Forward) <= UE_SMALL_NUMBER)
    {
        return nullptr;
    }
    auto HorizontalAngle = [&Forward, &Right](const FVector& Offset)
    {
        return FMath::RadiansToDegrees(
            FMath::Atan2(FVector::DotProduct(Offset, Right), FVector::DotProduct(Offset, Forward)));
    };
    const double CurrentAngle = HorizontalAngle(CurrentOffset);
    const double Sign = Direction == EETTargetLockSwitchDirection::Right ? 1.0 : -1.0;
    TArray<AEmberTrialCharacter*> Candidates;
    GatherCandidates(Source, ViewLocation, Forward, Candidates);
    AEmberTrialCharacter* Best = nullptr;
    int64 BestAngle = TNumericLimits<int64>::Max();
    double BestDistance = TNumericLimits<double>::Max();
    constexpr double AnglePrecision = 10000.0;
    for (AEmberTrialCharacter* Target : Candidates)
    {
        if (Target == Current)
        {
            continue;
        }
        const double Gap =
            Sign * (HorizontalAngle(Target->GetActorLocation() - ViewLocation) - CurrentAngle);
        if (Gap <= SwitchMinAngle)
        {
            continue;
        }
        const int64 AngleScore = FMath::RoundToInt64(Gap * AnglePrecision);
        const double Distance =
            FVector::DistSquared(Source->GetActorLocation(), Target->GetActorLocation());
        if (!Best || AngleScore < BestAngle ||
            (AngleScore == BestAngle &&
             (Distance < BestDistance || (Distance == BestDistance &&
                                          Target->GetPathName().Compare(Best->GetPathName()) < 0))))
        {
            Best = Target;
            BestAngle = AngleScore;
            BestDistance = Distance;
        }
    }
    return Best;
}

bool UETTargetLockComponent::SwitchTarget(EETTargetLockSwitchDirection Direction)
{
    AEmberTrialCharacter* Source = nullptr;
    APlayerController* PC = nullptr;
    AEmberTrialCharacter* Current = GetLockedTarget();
    if (!ResolveRequestContext(Source, PC) || !HasValidSwitchConfiguration() ||
        !IsTargetEligible(Source, Current, BreakRadius) ||
        (Direction != EETTargetLockSwitchDirection::Left &&
         Direction != EETTargetLockSwitchDirection::Right))
    {
        return false;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (!FMath::IsFinite(Now) || Now < NextSwitchAllowedAt)
    {
        return false;
    }
    // 搜索失败也消耗本次限频；先写门限，再查询 / 广播，拒绝同步重入与排队。
    NextSwitchAllowedAt = Now + SwitchInterval;
    FVector ViewLocation;
    FRotator ViewRotation;
    PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
    AEmberTrialCharacter* Next =
        FindSwitchTarget(Source, Current, ViewLocation, ViewRotation, Direction);
    if (!Next)
    {
        UE_LOG(LogETTargetLock, Display,
               TEXT("[M2-06C] Switch rejected Reason=NoCandidate Direction=%s Current=%s"),
               Direction == EETTargetLockSwitchDirection::Right ? TEXT("Right") : TEXT("Left"),
               *GetNameSafe(Current));
        return false;
    }
    SetLockedTarget(Next);
    return true;
}

bool UETTargetLockComponent::ToggleTargetLock()
{
    AEmberTrialCharacter* Source = nullptr;
    APlayerController* PC = nullptr;
    if (!ResolveRequestContext(Source, PC))
    {
        return false;
    }
    if (LockedTarget.IsValid() || LockedTarget.IsStale())
    {
        ClearTarget(TEXT("ToggleOff"));
        return true;
    }
    FVector ViewLocation;
    FRotator ViewRotation;
    PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
    AEmberTrialCharacter* Target = FindBestTarget(Source, ViewLocation, ViewRotation.Vector());
    if (!Target)
    {
        UE_LOG(LogETTargetLock, Display, TEXT("[M2-06A] Acquire rejected Reason=NoCandidate"));
        return false;
    }
    SetLockedTarget(Target);
    return true;
}

// ---- 身体 / 目标各自订阅；换控永不自动继承旧身体的锁定 ----
void UETTargetLockComponent::ClearSourceBindings()
{
    if (AEmberTrialCharacter* Source = BoundSource.Get())
    {
        Source->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleBodyReadyChanged);
        Source->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleBodyEndPlay);
    }
    if (UETCombatLifeComponent* Life = SourceLife.Get())
    {
        Life->OnDeathSealed.Remove(SourceDeathHandle);
    }
    SourceDeathHandle.Reset();
    SourceLife.Reset();
    BoundSource.Reset();
}

void UETTargetLockComponent::RebindSource()
{
    ClearTarget(TEXT("PawnChanged"));
    ClearSourceBindings();
    APlayerController* PC = BoundController.Get();
    AEmberTrialCharacter* Source = PC ? Cast<AEmberTrialCharacter>(PC->GetPawn()) : nullptr;
    if (bEndingPlay || !IsValid(Source) || Source->GetCombatTeam() != EETCombatTeam::Player)
    {
        return;
    }
    BoundSource = Source;
    Source->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleBodyReadyChanged);
    Source->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleBodyEndPlay);
    SourceLife = Source->FindComponentByClass<UETCombatLifeComponent>();
    if (UETCombatLifeComponent* Life = SourceLife.Get())
    {
        SourceDeathHandle = Life->OnDeathSealed.AddUObject(this, &ThisClass::HandleDeathSealed);
    }
}

void UETTargetLockComponent::SetLockedTarget(AEmberTrialCharacter* Target)
{
    AEmberTrialCharacter* Previous = LockedTarget.Get();
    if (!IsValid(Target) || Target == Previous)
    {
        return;
    }
    // 内部撤销旧代次 / 委托 / Timer，不向View广播中间的None。
    ResetTargetBindings();
    LockedTarget = Target;
    const uint64 Serial = LockSerial;
    Target->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleBodyReadyChanged);
    Target->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleBodyEndPlay);
    TargetLife = Target->FindComponentByClass<UETCombatLifeComponent>();
    if (UETCombatLifeComponent* Life = TargetLife.Get())
    {
        TargetDeathHandle = Life->OnDeathSealed.AddUObject(this, &ThisClass::HandleDeathSealed);
    }
    GetWorld()->GetTimerManager().SetTimer(
        MonitorTimer, FTimerDelegate::CreateUObject(this, &ThisClass::MaintainLock, Serial),
        MonitorInterval, true);
    if (IsValid(Previous))
    {
        UE_LOG(LogETTargetLock, Display, TEXT("[M2-06C] Switched Previous=%s Target=%s Timer=1"),
               *GetNameSafe(Previous), *GetNameSafe(Target));
    }
    else
    {
        UE_LOG(LogETTargetLock, Display, TEXT("[M2-06A] Locked Source=%s Target=%s Timer=1"),
               *GetNameSafe(BoundSource.Get()), *GetNameSafe(Target));
    }
    OnTargetChanged.Broadcast(Previous, Target);
}

void UETTargetLockComponent::ResetTargetBindings()
{
    AEmberTrialCharacter* Previous = LockedTarget.Get();
    ++LockSerial;
    LockedTarget.Reset();
    OccludedSince = -1.0;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(MonitorTimer);
    }
    MonitorTimer.Invalidate();
    if (IsValid(Previous))
    {
        Previous->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleBodyReadyChanged);
        Previous->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleBodyEndPlay);
    }
    if (UETCombatLifeComponent* Life = TargetLife.Get())
    {
        Life->OnDeathSealed.Remove(TargetDeathHandle);
    }
    TargetLife.Reset();
    TargetDeathHandle.Reset();
}

void UETTargetLockComponent::ClearTarget(const TCHAR* Reason)
{
    const bool bHadIdentity = LockedTarget.IsValid() || LockedTarget.IsStale();
    AEmberTrialCharacter* Previous = LockedTarget.Get();
    ResetTargetBindings();
    NextSwitchAllowedAt = 0.0;
    if (bHadIdentity)
    {
        UE_LOG(LogETTargetLock, Display, TEXT("[M2-06A] Cleared Target=%s Reason=%s Timer=0"),
               *GetNameSafe(Previous), Reason);
        OnTargetChanged.Broadcast(Previous, nullptr);
    }
}

// ---- 仅已有目标的检查；不重新Overlap，不因旁边出现更优目标自动切换 ----
void UETTargetLockComponent::MaintainLock(uint64 Serial)
{
    if (bEndingPlay || Serial != LockSerial)
    {
        return;
    }
    AEmberTrialCharacter* Source = ResolveSource();
    AEmberTrialCharacter* Target = LockedTarget.Get();
    if (!IsActive() || !HasValidConfiguration() || !Source ||
        !IsTargetEligible(Source, Target, BreakRadius))
    {
        ClearTarget(TEXT("SourceOrTargetInvalid"));
        return;
    }
    FVector ViewLocation;
    FRotator ViewRotation;
    BoundController->GetPlayerViewPoint(ViewLocation, ViewRotation);
    if (HasVisibility(Source, Target, ViewLocation))
    {
        OccludedSince = -1.0;
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (OccludedSince < 0.0)
    {
        OccludedSince = Now;
    }
    if (Now - OccludedSince >= OcclusionGrace)
    {
        ClearTarget(TEXT("Occluded"));
    }
}

void UETTargetLockComponent::HandlePossessedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (!bEndingPlay)
    {
        RebindSource();
    }
}

void UETTargetLockComponent::HandleBodyReadyChanged(AEmberTrialCharacter* Body, bool bReady)
{
    if (!bReady && (Body == BoundSource.Get() || Body == LockedTarget.Get()))
    {
        ClearTarget(TEXT("ReadyWithdrawn"));
    }
}

void UETTargetLockComponent::HandleBodyEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
    (void)Reason;
    if (Actor == BoundSource.Get())
    {
        ClearTarget(TEXT("SourceEndPlay"));
        ClearSourceBindings();
    }
    else if (Actor == LockedTarget.Get())
    {
        ClearTarget(TEXT("TargetEndPlay"));
    }
}

void UETTargetLockComponent::HandleDeathSealed(UETCombatLifeComponent* Life)
{
    if (Life == SourceLife.Get() || Life == TargetLife.Get())
    {
        ClearTarget(TEXT("DeathSealed"));
    }
}
```


## 5. Controller只转发输入

### 5.1 ETPlayerController.h：只添加以下内容

Ctrl+F找到**已有**`void HandleTargetLockStarted();`。在这一行后、下一段“镜头规则”注释前，加入以下内容，仍在private区。原TargetLockAction、TargetLockComponent、Camera引用与其余声明全部保留；不重复添加HandleTargetLockStarted。

```cpp
// 仅转发左右意图，方向键位由现有IMC配置。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock")
    TObjectPtr<UInputAction> TargetSwitchLeftAction;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock")
    TObjectPtr<UInputAction> TargetSwitchRightAction;

    void HandleTargetSwitchLeftStarted();
    void HandleTargetSwitchRightStarted();
```


### 5.2 ETPlayerController.cpp：加两个函数

找到已有`void AETPlayerController::HandleTargetLockStarted()`的整个函数，在它结束的右花括号之后、`SetupInputComponent`之前添加下面两个函数。不要把函数写进另一个函数的大括号内部；顶部已经include TargetLock组件，无需再加include。

```cpp
void AETPlayerController::HandleTargetSwitchLeftStarted()
{
    if (IsValid(TargetLockComponent.Get()))
    {
        TargetLockComponent->SwitchTarget(EETTargetLockSwitchDirection::Left);
    }
}

void AETPlayerController::HandleTargetSwitchRightStarted()
{
    if (IsValid(TargetLockComponent.Get()))
    {
        TargetLockComponent->SwitchTarget(EETTargetLockSwitchDirection::Right);
    }
}
```


### 5.3 只替换SetupInputComponent整个函数

从`void AETPlayerController::SetupInputComponent()`到其配对的最外层右花括号，整体换成下面代码。**不是在旧函数中再追加一套绑定**。P仍Released / 非暂停触发，三个锁定IA均Started；输入帮助Lambda只是收拢相同校验，不保存未来请求。

构造函数、UpdateRotation、HandleTargetLockStarted、HandlePauseReleased、BeginPlay / Possess / EndPlay及ET_Stamina / ET_Healing全部保留。特别是右肩B2的UpdateRotation仍只调用一次Super，不改它。

```cpp
void AETPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (!InputComponent)
    {
        return;
    }
    FInputKeyBinding& PauseBinding =
        InputComponent->BindKey(EKeys::P, IE_Released, this, &ThisClass::HandlePauseReleased);
    PauseBinding.bConsumeInput = true;
    PauseBinding.bExecuteWhenPaused = false;

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
    auto BindStarted = [this, EnhancedInput](UInputAction* Action,
                                             void (AETPlayerController::*Handler)(),
                                             const TCHAR* Purpose)
    {
        if (EnhancedInput && IsValid(Action) && Action->ValueType == EInputActionValueType::Boolean)
        {
            EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, Handler);
        }
        else
        {
            UE_LOG(LogETPlayerController, Display,
                   TEXT("[M2-06C] Lock input unavailable Purpose=%s Controller=%s"), Purpose,
                   *GetName());
        }
    };
    BindStarted(TargetLockAction.Get(), &ThisClass::HandleTargetLockStarted, TEXT("Toggle"));

    // 同一IA不能同时变成切左 / 切右 / 解除。非法配置只停用切换，不破坏原Tab / P。
    UInputAction* Left = TargetSwitchLeftAction.Get();
    UInputAction* Right = TargetSwitchRightAction.Get();
    if ((Left && (Left == Right || Left == TargetLockAction.Get())) ||
        (Right && Right == TargetLockAction.Get()))
    {
        UE_LOG(LogETPlayerController, Warning,
               TEXT("[M2-06C] Switch input rejected Reason=DuplicateAction Controller=%s"),
               *GetName());
        return;
    }
    BindStarted(Left, &ThisClass::HandleTargetSwitchLeftStarted, TEXT("SwitchLeft"));
    BindStarted(Right, &ThisClass::HandleTargetSwitchRightStarted, TEXT("SwitchRight"));
}
```


## 6. 扩展原Selection自动化，不新建测试文件

### 6.1 顶部加ScopeExit头

打开`Tests/ETTargetLockTests.cpp`，在现有`#include "Misc/AutomationTest.h"`后新增一行，仍处于原`#if WITH_DEV_AUTOMATION_TESTS`之内：

```cpp
#include "Misc/ScopeExit.h"
```


### 6.2 在明确锚点前插入完整块

Ctrl+F搜索这条**现有**断言，只应找到一次：

```cpp
TestTrue(TEXT("Second toggle releases"), Lock->ToggleTargetLock());
```


就在这一行**前面**，插入下面整块。它位于RunTest函数内，不能放文件结尾或挪到原Front已死亡之后。原断言及其后的暂停 / 遮挡 / 死亡 / 换控 / 清理用例全部保留。

本块自带一对外层大括号，作用域退出时ON_SCOPE_EXIT先解绑测试Lambda；即便中途return false，也不会在测试World销毁时回调已经失效的局部变量。不要删掉这个保护；不需要新friend或Blueprint函数。

```cpp
// ---- C：只扩已有Selection专项；离开本作用域先解绑，失败返回也不留下悬空Lambda ----
    {
        const FVector SavedSideLocation = Side->GetActorLocation();
        Side->SetActorLocation(FVector(3000, 400, 200));
        AEmberTrialCharacter* Left = SpawnBody(FVector(600, -300, 200), EETCombatTeam::Enemy);
        AEmberTrialCharacter* Right = SpawnBody(FVector(600, 300, 200), EETCombatTeam::Enemy);
        if (!TestNotNull(TEXT("Switch left"), Left) || !TestNotNull(TEXT("Switch right"), Right))
        {
            return false;
        }
        TestTrue(TEXT("Find screen left"),
                 Lock->FindSwitchTarget(Source, Front, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Left) == Left);
        TestTrue(TEXT("Find screen right"),
                 Lock->FindSwitchTarget(Source, Front, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Right) == Right);
        TestNull(TEXT("No wrap from right edge"),
                 Lock->FindSwitchTarget(Source, Right, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Right));
        Left->SetActorLocation(FVector(600, 0, 350));
        TestNull(TEXT("Same horizontal angle is not a left candidate"),
                 Lock->FindSwitchTarget(Source, Front, View, FRotator::ZeroRotator,
                                        EETTargetLockSwitchDirection::Left));
        Left->SetActorLocation(FVector(600, -300, 200));

        int32 Changes = 0;
        AEmberTrialCharacter* LastPrevious = nullptr;
        AEmberTrialCharacter* LastCurrent = nullptr;
        bool bReentrantAccepted = false;
        const FDelegateHandle ChangedHandle = Lock->OnTargetChanged.AddLambda(
            [&](AEmberTrialCharacter* Previous, AEmberTrialCharacter* Current)
            {
                ++Changes;
                LastPrevious = Previous;
                LastCurrent = Current;
                bReentrantAccepted |= Lock->SwitchTarget(EETTargetLockSwitchDirection::Left);
            });
        ON_SCOPE_EXIT
        {
            Lock->OnTargetChanged.Remove(ChangedHandle);
        };

        const uint64 BeforeSwitchSerial = Lock->LockSerial;
        TestTrue(TEXT("Real right request accepted"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));
        TestTrue(TEXT("One direct old to new event"),
                 Changes == 1 && LastPrevious == Front && LastCurrent == Right);
        TestFalse(TEXT("Broadcast cannot reenter switch"), bReentrantAccepted);
        TestTrue(TEXT("One new target monitor"),
                 World->GetTimerManager().IsTimerActive(Lock->MonitorTimer));
        TestFalse(TEXT("Both directions share throttle"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        Lock->MaintainLock(BeforeSwitchSerial);
        Front->SetGameplayReady(false);
        TestTrue(TEXT("Old serial and old Ready cannot clear new target"),
                 Lock->GetLockedTarget() == Right);
        Front->SetGameplayReady(true);
        if (!Advance(.21f))
        {
            return false;
        }
        TestTrue(TEXT("Left chooses nearest intermediate target"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        TestTrue(TEXT("Nearest is original front"), Lock->GetLockedTarget() == Front);
        if (!Advance(.21f))
        {
            return false;
        }
        Right->SetGameplayReady(false);
        const int32 BeforeRejectedChanges = Changes;
        TestFalse(TEXT("No candidate keeps current"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));
        TestTrue(TEXT("Failure has no None event"),
                 Lock->GetLockedTarget() == Front && Changes == BeforeRejectedChanges);
        Right->SetGameplayReady(true);
        TestFalse(TEXT("Failed search also throttles"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));
        if (!Advance(.21f))
        {
            return false;
        }
        TestTrue(TEXT("Rejected input was not queued"), Lock->GetLockedTarget() == Front);
        TestTrue(TEXT("Fresh request switches"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Right));

        APlayerState* SwitchPauser = World->SpawnActor<APlayerState>();
        if (!TestNotNull(TEXT("Switch pauser"), SwitchPauser))
        {
            return false;
        }
        World->GetWorldSettings()->SetPauserPlayerState(SwitchPauser);
        const double Deadline = Lock->NextSwitchAllowedAt;
        TestFalse(TEXT("Pause rejects switch"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        if (!Advance(.25f))
        {
            return false;
        }
        TestTrue(TEXT("Pause does not alter switch deadline"),
                 Lock->NextSwitchAllowedAt == Deadline);
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        TestFalse(TEXT("Pause did not spend switch interval"),
                  Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        if (!Advance(.21f))
        {
            return false;
        }
        TestTrue(TEXT("Resume does not replay request"), Lock->GetLockedTarget() == Right);
        TestTrue(TEXT("Fresh left restores original target"),
                 Lock->SwitchTarget(EETTargetLockSwitchDirection::Left));
        TestTrue(TEXT("Front restored"), Lock->GetLockedTarget() == Front);
        Left->Destroy();
        Right->Destroy();
        TestTrue(TEXT("Previous target EndPlay cannot clear current"),
                 Lock->GetLockedTarget() == Front);
        Side->SetActorLocation(SavedSideLocation);
    }
```


本块覆盖左右 / 不绕回 / 同水平角拒绝、真实请求、单次旧→新事件、双向共享限频与同步重入拒绝、旧Serial / Ready隔离、失败保留 / 不排队、Pause冻结。原Selection的Neutral / Waiting / 遮挡 / 死亡 / 销毁 / 越界 / 换身体 / 失活清理继续保留。测试仍叫`EmberTrial.M2.TargetLock.Selection`，没有第23个测试，也不删除旧测试来换Success。

### 6.3 拆墙后获取失败：固定无地板测试世界中的身体

2026-10-24实测发现：原测试在World启动前调用DisableMovement，后续PostInitializeComponents可能重新设置移动模式。新增切换 / Pause用例延长模拟时间后，没有地板的目标持续下落；失败时Front从Z200掉到约-1117.9，Side约-731，射线已经畅通，但距离 / 视角不再合法。错误发生在测试夹具，原Acquire断言和玩法范围不应放宽。

**助手已直接把下面两处写入实际ETTargetLockTests.cpp，本次用户无需重复录入。**后续从本文复建工程时也须包含本节。

第一处：找到RunTest开头的`PC->Possess(Source);`到`auto Advance`之前，把这段初始化替换为下面完整内容。删除旧的“只在BeginPlay前禁用Source移动”一行，改为World启动后统一冻结最初五个身体；SpawnBody中原DisableMovement保留，负责运行中新增的左右目标。

```cpp
    PC->Possess(Source);
    PC->SetControlRotation(FRotator::ZeroRotator);
    PC->SetViewTarget(Source);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    // 世界启动的PostInitializeComponents可能恢复默认移动模式。
    // 这个无地板夹具只验证锁定：启动后冻结初始身体，保留碰撞 / Timer / 生命周期。
    for (AEmberTrialCharacter* Body : {Source, Front, Side, Rear, Neutral})
    {
        Body->GetCharacterMovement()->DisableMovement();
    }
```

接下来仍是原`auto Advance = [&Fixture](float Seconds)`及其整个Lambda，不删除它。冻结的只是本测试身体移动模式，不停World / Controller / Timer，不修改正式角色、AI或地图重力。

第二处：在原遮挡用例中找到`Wall->Destroy();`以及紧随的`Acquire after wall removal`判断，将这两处及判断的大括号整体替换为下面代码，之后原Front真实死亡用例保留：

```cpp
    Wall->Destroy();
    if (!Advance(.05f))
    {
        return false;
    }
    TestTrue(TEXT("Stationary fixture target keeps its spawn position"),
             Front->GetActorLocation().Equals(FVector(600, 0, 200), .01));
    if (!TestTrue(TEXT("Acquire after wall removal"), Lock->ToggleTargetLock()))
    {
        return false;
    }
```

新增位置断言明确检查“长时间模拟后目标仍在测试位置”，原拆墙 / 遮挡 / 死亡 / 取消断言全部保留。临时诊断日志只留在忽略Raw，最终Source无LockDiag输出；本节不要求增加新测试文件或扩大Acquire Radius / Half Angle。

**本次窄补修结果：**助手已正常完成Editor Build（90.62秒）与Game Development Build（59.92秒）；2026-10-24北京时间21:28:14，CameraRules / Selection / MarkerRules三项均Success，导出结果3成功 / 0警告 / 0失败 / 0未执行。新增位置断言与原Acquire after wall removal断言均保留并通过，其余19项沿用。原始诊断 / 最终日志与JSON仅留本地`Docs/Reports/Raw/M2_06C_SelectionFix_2026-10-24`。

本次只修改实际`EmberTrial/Source/EmberTrial/Tests/ETTargetLockTests.cpp`，没有改变玩法Source、资产 / Config或Git索引；没有创建C整章验收报告、打包或声明键鼠已验收。助手操作不追加用户手动工时。你可直接继续9.2实机检查，不必重录本节或重复三项专项。

## 7. 正常Build与索引

相关源码全保存、UE已关闭后，在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

应完成Editor Build并刷新Core / UI / Editor三模块元数据，补修后的cpp数量Core60 / UI23 / Editor3。然后CLion Reload CMake Project，确认五个文件归属Core目标，`ScopeExit.h`和`EnhancedInputComponent.h`能导航。CMake服务索引，不能用构建CodeModel替代UBT。

继续正常Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

双目标成功后打开工程。若UBT脚本只显示exit code6，发它上方**第一条UHT / 编译 / 链接Error**及文件行号，不只截脚本throw。

下发前已按本机5.5.4的EnhancedInput BindAction、World游戏时间、Timer与现有接口审查，并在忽略Raw的完整Source隔离副本运行UHT（ExitCode0），三份候选.cpp通过MSVC `/Zs`语法检查（均ExitCode0、无诊断）。生成反射输出只在副本目录；**实际Source / Content / Config保持不变**。该检查没有链接正式Editor / Game、没有运行新增Selection或真实IA输入，不能写作本章Build / 自动化Success；首次正式双目标Build仍以你这一步为准。

## 8. 两个IA、原IMC与原Controller

### 8.1 创建两个独立Input Action

1. Content Browser进入`/Game/EmberTrial/Input/Actions`。空白处右键 → Input → Input Action，新建**IA_ET_SwitchTargetLeft**。
2. 打开资产，在Details设置Value Type=**Digital (bool) / Boolean**；Triggers=0项、Modifiers=0项；Trigger When Paused=False，Consume Input=True。不添加Hold / Pressed Trigger。
3. Save，关闭此资产页。用同样方法新建**IA_ET_SwitchTargetRight**，逐项同样设置并Save。
4. 两个IA必须不同对象，也不能用原IA_ET_LockTarget重复充当左 / 右输入。无需Montage、Notify、Widget或Graph。

### 8.2 只给原IMC_Default增加两行

1. 打开`/Game/ThirdPerson/Input/IMC_Default`；不要新建IMC，不改角色Default Mapping Context。
2. 在Mappings点一次+，该行Action选**IA_ET_SwitchTargetLeft**；展开这行的Key选择器，选择键盘**Z**。该行Triggers / Modifiers也保持0项。
3. 再点一次+，Action选**IA_ET_SwitchTargetRight**，键盘**C**，该行Triggers / Modifiers保持0项。
4. 收起再展开Mappings逐项核对：Left只有Z一行，Right只有C一行；原IA_ET_LockTarget仍唯一Tab。WASD / Space / 左键 / 2 / Q / F不动，也不重新添加P。
5. 如果实际已有其他正式动作占用Z / C，先发该映射行截图，不盲删原动作。当前计划未安排这两键的其他正式用途。Save。

### 8.3 原Controller Class Defaults：三个Action引用

1. 打开`/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`，父类仍**ETPlayerController**，不要Reparent。
2. **先点工具栏Class Defaults**，再在右侧Details搜索`Target`。若之前选中了Components中的某个组件，先点Class Defaults，否则会看不到Controller字段。
3. 在EmberTrial → Target Lock分类中逐项设置：

| Class Defaults字段 | 值 |
| --- | --- |
| Target Lock Action | 原IA_ET_LockTarget，保持 |
| Target Switch Left Action | IA_ET_SwitchTargetLeft |
| Target Switch Right Action | IA_ET_SwitchTargetRight |

4. 搜不到时清空搜索，再按完整字段名搜索；仍没有则核对7节Editor Build、实际父类与.h声明。不要在EventGraph右键搜来替代Details赋值。

### 8.4 原TargetLock组件：两个新参数

1. 清空Details搜索，在左侧**Components点唯一继承的TargetLock**，不是TargetLockCamera / TargetLockMarker。
2. 搜索`Switch`，应看到Switch Interval=**0.20秒**、Switch Min Angle=**3度**。若已有正确原生继承值，不制造实例覆盖。
3. 清空搜索，原Acquire1200 / Break1500 / HalfAngle60 / ViewWeight.75 / Monitor.10 / OcclusionGrace.35和Auto Activate=True保留。
4. 另点TargetLockCamera，只核对当前右肩值8 / -45 / 15 / Pitch-18 / Yaw0 / MaxShoulderYaw12、Shoulder0-75-55 / Focus45 / AutoActivate=True，**不新增相机组件**。Marker、UIBootstrap、FailureFlow和各Class原样保留。
5. EventGraph不接Z / C / Switch Target；输入已经由C++绑定。Controller点Compile → 确认无Error → Save；两个IA和IMC也Save。回正式地图`L_ET_VerticalSlice`。

## 9. 方案B短验收：10–15分钟

### 9.1 相关自动化（本轮四项已由助手完成）

非PIE状态，在Output Log底部命令栏逐条执行，上一条Success再下一条：

```text
Automation RunTests EmberTrial.M2.TargetLock.Selection
Automation RunTests EmberTrial.M2.TargetLock.CameraRules
Automation RunTests EmberTrial.UI.TargetLock.MarkerRules
Automation RunTests EmberTrial.Framework.SpawnPlacement
```

**本轮已由助手执行以上四项且全部Success，0警告 / 0失败，无需用户重复执行。**Selection覆盖切换，CameraRules / MarkerRules覆盖共享目标，SpawnPlacement覆盖新增出生策略；其余19项沿用，不机械跑全23项。它们不替代下面的真实Z / C、镜头 / 标记与交战检查。失败发第一条断言与完成时分；不删失败断言，也不用扩大阈值来掩盖。

### 9.2 一次双目标准备（约2分钟）

1. **Stop PIE**。Outliner找到唯一正式**MeleeEnemy_A**，记录其Actor Transform与Auto Possess AI原值；是场景实例Details，不打开BP改Class Defaults。
2. 选中它Ctrl+W复制一份实例，Outliner重命名**MeleeEnemy_SwitchProbe**。不Duplicate蓝图，不新建敌人类 / 测试函数。
3. 为了先稳定观察选择，**仅这两个场景实例**临时设置Auto Possess AI=Disabled。双方CombatTeam / 生命100 / Ready / 碰撞全部保留；不改BP默认、AI Controller Class、Tag或技能伤害。
4. 建议把A临时放在Actor Location **600 / 250 / 120**，Probe放**600 / -250 / 120**，旋转保持原值；若原地图有挡板，改放在同一块空旷可走地面，目标左右相隔约500cm、玩家前方约600cm。保存原A值，最后恢复。Mesh相对Transform不要动。
5. Play后先向+X走离Entry约500–600cm，让两个目标都处于前方可见范围，再站定。Entry(-600,0)到建议目标约1225cm，**刚出生直接Tab可能超1200获取半径**；先接近，不扩大Acquire Radius。
6. 敌人禁用AI只影响追击，不影响Enemy / Ready / 可锁定与承伤。若仍移动，Stop核对是两个实例的Auto Possess AI，而不是临时去断行为树白线。本小节临时地图改动无需提交。

### 9.3 左右、空候选与右肩一致性（约3分钟）

1. 初始未锁时各按Z / C一次：无标记、无自动获取。面向两敌人Tab一次，出现单菱形和原右肩构图。
2. 观察**未锁的那个敌人在当前菱形左边还是右边**：左边就按Z，右边就按C。镜头平滑转向它，菱形转移到同一新敌人，玩家仍偏左；不得有两枚标记 / 第二份HUD。日志应有`[M2-06C] Switched Previous=... Target=... Timer=1`。
3. 等镜头稳定约半秒，再观察旧敌人在当前目标哪侧，按对应键切回。不要靠Actor名字猜“固定A在左”；右肩转向后以当下画面为准。
4. 在只有左右两个敌人的场景，对外侧按键：该方向没有另一个候选时保留当前锁，日志NoCandidate正常。**最右边按C不绕到左边**，最左边按Z同理。
5. 长按某个方向约1秒：只处理Started一次；连续快速Z / C不会积压，停止按键后目标不会自行续切。精确.20秒和Pause剩余间隔由自动化负责，你无需掐毫秒。
6. 按Tab解除：立即可自由鼠标、肩位平滑回正；再按Z / C不获取；重新Tab仍正常。目标太近同水平角差≤3°时拒绝是预期，稍拉开左右间距再查。

### 9.4 一次遮挡（约2分钟）

旧目标Ready / EndPlay与旧Serial隔离已由本章Selection覆盖，不再为它搭DestroyActor蓝图或手动删除PIE实例。

1. 复用现有不透明墙：把视线调整到“当前目标可见、另一个目标中心被墙挡住”，按另一目标所在侧方向，应保持当前锁 / NoCandidate；不要改Visibility碰撞来让它通过。
2. 若现有布局做不出只挡一个的视线，Stop后临时放一个BasicShapes/Cube于另一目标视线中，记为临时挡板，9.6删除；不移动原墙 / 导航体。再Play接近 / Tab后重复一次即可，不重做多种墙角。
3. 当前目标持续被墙遮挡超过.35游戏秒加采样间隔时，沿用A脱锁；**不会自动跳到另一个**。重新露出后需Tab获取。这是设计边界，不补自动换人分支。

### 9.5 恢复单敌人后一次真实交战（约3–5分钟）

1. Stop。删除编辑器中的临时MeleeEnemy_SwitchProbe / 临时Cube，原A恢复记录的Transform与**Auto Possess AI=Placed in World or Spawned**（若原记录不同按原值），其他默认不动。Build Paths后确认原导航覆盖正常，不扩大Nav体 / 半径。
2. Play，靠近并Tab锁定正式敌人。用**左键、2、Q、F各一次**：左键仍真实25且连段按原规则，2只扣一次体力 / 方向固定；受伤后Q晚生效且扣一次次数，F仍真实Poise / 冷却。无需搭新伤害函数、强改生命或重复全部能力边界；满血Q本来应拒绝，先让敌人真实命中后再Q。
3. P暂停时按Z / C，鼠标或键盘任选一种继续；恢复不补执行切换 / 动作、不卡键，原肩位保持。没有候选时仍保持当前锁，不把NoCandidate当作输入损坏。
4. 杀死一次当前敌人：标记消失、镜头自由，无自动换人；或让玩家死亡 → Retry，新身体双100与五HUD、无旧锁。两种死亡中至少一项本次真实观察，另一项沿用Selection与前章；反馈写实际执行哪一种，不强迫反复死亡。
5. 最后一次E提示 / P继续与移动、转向、跳跃短查；锁定中Stop再Play初始无旧标记 / 目标。不要重做所有旧HUD重建、Pause焦点、检查点整章矩阵。

### 9.6 最终恢复与保存

Stop → 核对单MeleeEnemy_A / Auto Possess与原位置、无Probe / 临时挡板；三身体原100/100、正式伤害25 / 10、各GA原配置 / slomo1 / Debug False。Controller三IA引用、组件唯一、.20 / 3与右肩0-75-55保持。Compile / Save All。

地图若只为本章临时布置且已恢复，不强制提交；若UE正常保存后仍有真实字节变化，报告最终改动并按实际纳入清单，不能把残留Probe / Disabled AI当正式配置提交。两新IA与当前Controller / IMC / 地图SHA已核对并登记，AssetManifest现75条；本轮体验补修没有重存这些资产。

## 10. 完成反馈与排错

```text
M2-06C 完成
原五旧Core加收尾补修五旧一新 / 原Selection扩展；关闭UE后Editor Build / Core60-UI23-Editor3索引 / Game Build：
两IA Boolean / 无Triggers / 唯一Z-C映射；Controller三Action引用 / .20与3 / Compile-Save：
Selection / CameraRules / MarkerRules / SpawnPlacement（本轮助手已完成四Success）：
Tab保持 / Z-C真实左右 / 无锁不获取 / 空侧不绕回不丢锁 / 长按与快速按不排队：
单菱形与右肩一致 / 一次墙遮挡（旧目标退出隔离由Selection覆盖）：
恢复单敌人后左键-2-Q-F真实交战 / P不补执行与短基线：
本次真实执行的死亡路径（当前敌人死亡 / 玩家Failure-Retry）：
Probe与临时挡板删除，原A位置 / AI恢复，导航 / 原配置 / slomo1 / Debug False、Save All：
失败或手感问题（无填无）：
完成时分 / 手动代码 / 资产配置 / 检查 / 构建等待：
```

只给完成时分即可，我从日志关联，不要求抄成功长日志。编译错误发文字：第一条Error、完整文件路径 / 行号、相关函数前后约20行；Details / 映射问题发包含资产名与字段的截图。切换方向 / 镜头抖动给5–10秒视频、实际PIE方式和按键顺序；说明当时旧目标在菱形哪侧、是否有墙 / 近身，而不是只说“切换无效”。

本章通过后保存真实验收报告、更新两IA来源与计时，进入M2-07集成收口：最新独立包、必要性能窗口与复盘。未通过先收窄补修，不用旧包证明本章功能。

### 10.1 出生首帧与贴身镜头补修（已直接实施）

用户已报告原章节检查完成；本节只记录随后两项体验问题。源码 / Editor与Game构建在2026-10-24完成，2026-10-25继续审查并正常结束短采样。没有追加用户手动工时或要求重做原整章验收。

**出生陷地原因与修复。**当前玩家BP的实际胶囊半高90cm、半径35cm，Mesh相对Z=-90；Entry / Checkpoint_A原Z=120保持。旧出生先有约28cm落差，胶囊首帧已落到Z92.15，而FootIK仍追赶旧高度，采到脚尖Z约-16至-19cm，下一帧恢复。它不是脚部碰撞体穿透地板。

`Public/Framework/ETGameMode.h` / `Private/Framework/ETGameMode.cpp`增加生成前贴地：按实际Pawn类CDO的胶囊尺寸和CharacterMovement可走规则，向下Sweep一次，默认探测100cm；命中非穿透、非Pawn且可走地面时，保持XY / 朝向，在生成和动画初始化前放到胶囊半高加原生行走间隙。半高不写死90 / 96；间隙来自引擎MIN_FLOOR_DIST与MAX_FLOOR_DIST均值。高空、无地板、非法 / 倾斜 / 非均匀Transform、非Character等回退原引擎出生；距离0可关闭。不会每帧吸地，也没有关闭FootIK、改Mesh / 胶囊或移动已保存出生点。

生成路径由正式GameMode共用，Entry / Retry / Continue均可经过；本轮真实采样只覆盖Entry，未另跑检查点完整重试矩阵。新`Tests/ETSpawnPlacementTests.cpp`验证尺寸 / 原生间隙、XY和旋转保持、重复稳定及禁用 / 非法 / 高空 / 穿透 / 无地板回退。

**贴身镜头原因与修复。**原肩位75cm，目标距离接近肩位半径时，为追求居中的完整反正弦偏航会转向侧面。Camera现集中设置Pitch Offset=-18°、Max Shoulder Yaw=12°，对肩位偏航解析补偿加上限；继续保持Yaw0、Shoulder(0,75,55)cm、Focus45、Follow8、Pitch范围-45至15、原400cm臂长 / FOV / SpringArm碰撞。近景允许敌人略离画面中心，以保持玩家后侧和适度俯视。不是《黑神话：悟空》的内部参数，也未导入其资产。

`ETTargetLockCameraComponent.h` / `.cpp`与CameraRules已同步，[B2第3节](M2_06B2_LockCamera.md#3-三个文件的完整代码)保留工程一致的完整代码。原插值、Pause、解锁 / 失活 / 换身体 / EndPlay偏移恢复和能力朝向不变；没有新增常驻Tick / Timer。

| 检查 | 本轮真实结果 / 限制 |
| --- | --- |
| 正常Editor / 三模块索引 | ExitCode0，Editor总185.25秒；Core60 / UI23 / Editor3 |
| 正常Game Development | ExitCode0，总172.35秒；没有新Cook / Package |
| 四相关专项 | 2026-10-24 22:13:43北京时间，Selection / CameraRules / MarkerRules / SpawnPlacement，4成功、0警告、0失败、0未执行；其余19项沿用 |
| 两次前台出生采样 | 首个可采样帧脚尖约+1.48 / +1.77cm，胶囊底+2.15cm；不再出现旧首帧陷地 |
| 三距离实际取景 | 600 / 200 / 79cm均保持目标；贴身Pitch约-21.01°、相对交战轴偏航约-12.00°，玩家后侧可见敌人 |
| 10/25短复核 | 三距离再次完成并正常Stop；首次脚尖仍在地板上方，未保存资产 / 设置 |
| 截图 | 本地Artifacts/M2_06C_CameraStartup/CloseCombat.png；引擎HighResShot不含UMG，不能用它证明HUD / 标记 |
| 恢复 | AI停止 / Teleport仅用于PIE取景，Stop后销毁；编辑器19Actor、单正式敌人、无Probe / 临时挡板，原地图仍All Saved |

**首次Play等待保留。**前台修复后一次首次PIE从请求到可采样身体仍约1.55秒，紧接第二次约0.10秒；后续暖启动约0.14秒。样本受冷加载、编辑器和系统负载影响，不是纯游戏帧耗时，不能与旧样本直接比较并宣称帧率提升或卡顿完全消除。并开IDE / UE时曾内存98%、可用约210MB；临时保存关闭CLion后仍约90–91%，只能确认内存紧张是干扰因素，未证明它是唯一瓶颈。本轮没有改系统、电源、画质或全局加载策略。若后续独立包也持续出现首秒卡顿，再在M2-07用匹配新版包的Insights范围定位，不为本次两项补修重复完整性能采集。

Raw保留Baseline、构建 / 测试JSON、前后骨骼位置、相机数值和短复核。前两份补采含脚本清理超时 / 相机对象中断错误，未删改；不把它们称作无错误全程采集。`CameraReview_2026-10-25.json`另记本次三距离完成与正常Stop；截图和此前有效数值仍可独立对应。本轮Source相对补修前为五旧一新，Content / Config / Git索引保持；没有替用户Stage / Commit / Push。

## 11. 本章 Commit / Push（固定放在最后）

当前C实现、未提交的B2右肩以及本节体验补修共享相机 / Controller依赖，建议这次合并提交最新完整版本。源码共十旧一新：

```text
EmberTrial/Source/EmberTrial/Public/TargetLock/ETTargetLockComponent.h
EmberTrial/Source/EmberTrial/Private/TargetLock/ETTargetLockComponent.cpp
EmberTrial/Source/EmberTrial/Public/Framework/ETPlayerController.h
EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp
EmberTrial/Source/EmberTrial/Tests/ETTargetLockTests.cpp
EmberTrial/Source/EmberTrial/Public/TargetLock/ETTargetLockCameraComponent.h
EmberTrial/Source/EmberTrial/Private/TargetLock/ETTargetLockCameraComponent.cpp
EmberTrial/Source/EmberTrial/Tests/ETTargetLockCameraTests.cpp
EmberTrial/Source/EmberTrial/Public/Framework/ETGameMode.h
EmberTrial/Source/EmberTrial/Private/Framework/ETGameMode.cpp
EmberTrial/Source/EmberTrial/Tests/ETSpawnPlacementTests.cpp
```

两新 / 三旧UE资产（来自用户已完成C配置，本次体验补修未再重存）：

```text
EmberTrial/Content/EmberTrial/Input/Actions/IA_ET_SwitchTargetLeft.uasset
EmberTrial/Content/EmberTrial/Input/Actions/IA_ET_SwitchTargetRight.uasset
EmberTrial/Content/ThirdPerson/Input/IMC_Default.uasset
EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset
EmberTrial/Content/EmberTrial/Maps/L_ET_VerticalSlice.umap
```

当前地图实物19Actor / 单MeleeEnemy_A，无临时Probe / 挡板，保持Entry / Checkpoint_A原Z120；不以恢复原位置为由丢弃用户正常保存的地图。没有改WBP / GA / Character / Config / Build.cs。文档精确清单：

```text
Docs/M2_06C_TargetSwitching.md
Docs/M2_06B2_LockCamera.md
Docs/Reports/M2_06B2_Verification.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AssetManifest.csv
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Docs/M2_06_TargetLockRoadmap.md
Docs/M2_Roadmap.md
Docs/M2_05_BreakPulseRoadmap.md
README.md
Introduction.md
```

本轮没有创建C完整验收报告，C工时未报；不提交空报告或把助手操作计作用户实耗。

建议Commit（英文）：`feat: add target switching and refine close-combat camera and spawning`。

中文译文：`feat: 新增锁定目标切换并优化贴身战斗镜头与出生贴地`。

CLion Refresh Changes → Stage最新完整内容（已Stage后又改须再次Stage）→ Review Diff → Commit → Push。确认新IA不是空文件，Controller三Action引用正确，地图没有临时对象。Raw / 日志 / 采样脚本 / 截图 / Saved / Binaries / Intermediate / CMake索引 / 包体留本地，不Force Add。用户自行Stage / Commit / Push；助手未修改Git索引或联网核验远端。
