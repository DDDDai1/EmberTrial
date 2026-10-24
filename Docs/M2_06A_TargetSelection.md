# M2-06A：锁定目标筛选与生命周期

2026-10-24，UE5.5.4。前置[M2-05模块验收通过](Reports/M2_05_Verification.md)，无待补项；[锁定路线](M2_06_TargetLockRoadmap.md)。**本章待用户手动实施**，助手仅在忽略目录准备代码候选，未改实际Source / UE资产或执行Build / 自动化。

预算手动100–150分钟：代码60–95、资产 / 配置25–40、检查8–12，构建等待另记。代码较长可分段保存，全部完成再正常Build，不用Live Coding。

## 1. 本批做什么、什么留下一批

按**Tab**锁定镜头前方一个存活且Ready的Enemy，再按一次解除；长按只触发一次。Neutral训练目标不能锁，未找到候选时不保存未来请求。无锁定时没有检查Timer；锁定后只检查当前目标，不逐帧扫描关卡或自动抢换更优目标。

**本批先交付目标身份 / 日志，镜头与标记还不会变化**。锁定状态可由日志和原生只读GetLockedTarget查询。下一批再做明确标记与镜头跟随，避免把选错目标、镜头插值与UI投影同时混在一次排错里。攻击 / 闪避方向不因本批锁定自动调整，伤害仍由既有能力 / Trace / GE决定。

| 规则 | 默认值与语义 |
| --- | --- |
| 首次搜索 | 以玩家身体中心做一次1200cm Pawn对象Overlap，按Actor去重 |
| 合法目标 | 同World、Enemy阵营、Ready、存活、中心在搜索范围内；不选自身 / Neutral / Player |
| 视角 | 目标中心在真实玩家视点前向±60°的三维圆锥内；俯仰也参与，不等同屏幕矩形边界 |
| 评分 | 角度罚分75% + 身体距离罚分25%，越小越优；角度按圆锥归一，距离按1200归一；评分先以1/10000精度量化，同分按对象路径稳定决胜 |
| 可见性 | 从实际相机视点到目标中心做Visibility射线，忽略自身；未阻挡或命中目标自身才可选 |
| 维持 | 每.10游戏秒检查当前目标；中心距离超过1500cm脱锁，1200–1500形成迟滞 |
| 遮挡 | 获得时必须可见；已有目标连续遮挡约.35游戏秒后脱锁，实际加上至多一个采样间隔；Pause不消耗宽限 |
| 身份失效 | 玩家 / 目标死亡封口、Ready撤销、EndPlay、换Pawn、组件失活与结束均释放锁定 / Timer；新身体不自动继承 |
| 临时状态 | 受击 / 破势 / 无敌不等同死亡，单独出现时不强制脱锁；目标是否允许承伤仍由能力判断 |

已有目标移出首次视角圆锥不会单独脱锁，因为本批镜头仍自由；维持看距离 / 可见性 / 身份，B再实现镜头跟随。无需新模型 / 动画 / 材质或付费资源。

所有权：Controller创建唯一TargetLock原生组件；组件弱持有当前身体 / 目标，发布目标变化原生事件。Gameplay规则在Core，UI后续只观察事件，不反向改变属性。TargetLock不把6秒冷却、Poise或动作状态再缓存一份。

## 2. 保存、关闭与六份Source路径

Stop PIE → Save All → 关闭UE → CLion保存。下表相对`E:\UE_Project\Demo\EmberTrial\Source`，不是仓库根。

| 类型 | 路径 | 操作 |
| --- | --- | --- |
| 新 | EmberTrial/Public/TargetLock/ETTargetLockComponent.h | 3.1全文 |
| 新 | EmberTrial/Private/TargetLock/ETTargetLockComponent.cpp | 3.2全文 |
| 新 | EmberTrial/Tests/ETTargetLockTests.cpp | 第5节全文 |
| 旧 | EmberTrial/Public/Framework/ETPlayerController.h | 第4节构造 / 组件 / IA声明 |
| 旧 | EmberTrial/Private/Framework/ETPlayerController.cpp | 第4节构造 / 唯一Started绑定 |
| 旧 | EmberTrial/Public/Characters/EmberTrialCharacter.h | 只加专项friend，不改玩法 |

本次测试确实属于**Core**：完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETTargetLockTests.cpp`。上一章ETBreakPulseHUDTests属于UI，继续保留在EmberTrialUI/Private/Tests；不要混放，也不新建同名副本。新TargetLock目录在Public和Private各一份。

可在CLion正确物理目录创建.h/.cpp，或用UE向导创建ActorComponent后核对所属模块 / Public / Private路径，再替换完整内容。测试直接建.cpp，不用创建UObject测试类。无需改Build.cs、Target或uproject；Core已有EnhancedInput / Engine依赖，继续禁止Core依赖UI。

## 3. 新增原生锁定组件

### 3.1 `EmberTrial/Public/TargetLock/ETTargetLockComponent.h`

新文件全文：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETTargetLockComponent.generated.h"

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
    AEmberTrialCharacter* ResolveSource() const;
    bool IsTargetEligible(const AEmberTrialCharacter* Source, const AEmberTrialCharacter* Target,
                          float Radius) const;
    bool HasVisibility(const AEmberTrialCharacter* Source, const AEmberTrialCharacter* Target,
                       const FVector& ViewLocation) const;
    AEmberTrialCharacter* FindBestTarget(const AEmberTrialCharacter* Source,
                                         const FVector& ViewLocation,
                                         const FVector& ViewForward) const;

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

### 3.2 `EmberTrial/Private/TargetLock/ETTargetLockComponent.cpp`

新文件全文：

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

// ---- 按请求Overlap一次；角度 / 距离归一评分，近似同分按稳定对象路径决胜 ----
AEmberTrialCharacter* UETTargetLockComponent::FindBestTarget(const AEmberTrialCharacter* Source,
                                                             const FVector& ViewLocation,
                                                             const FVector& ViewForward) const
{
    UWorld* World = GetWorld();
    if (!IsValid(World) || !HasValidConfiguration() || !IsValid(Source) ||
        ViewLocation.ContainsNaN() || ViewForward.ContainsNaN() || ViewForward.IsNearlyZero())
    {
        return nullptr;
    }
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ETTargetLockAcquire), false, Source);
    TArray<FOverlapResult> Hits;
    World->OverlapMultiByObjectType(Hits, Source->GetActorLocation(), FQuat::Identity, Objects,
                                    FCollisionShape::MakeSphere(AcquireRadius), Params);
    TSet<AEmberTrialCharacter*> Seen;
    AEmberTrialCharacter* Best = nullptr;
    int64 BestScore = TNumericLimits<int64>::Max();
    // 先量化为同分桶，再按路径决胜；避免近似相等关系导致遍历顺序影响结果。
    constexpr double ScorePrecision = 10000.0;
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
        if (Offset.IsNearlyZero())
        {
            continue;
        }
        const double Dot =
            FMath::Clamp(FVector::DotProduct(Forward, Offset.GetSafeNormal()), -1.0, 1.0);
        if (Dot < MinDot || !HasVisibility(Source, Target, ViewLocation))
        {
            continue;
        }
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

bool UETTargetLockComponent::ToggleTargetLock()
{
    UWorld* World = GetWorld();
    APlayerController* PC = BoundController.Get();
    AEmberTrialCharacter* Source = ResolveSource();
    if (bEndingPlay || !IsActive() || !IsValid(World) || World->IsPaused() || !IsValid(PC) ||
        PC->bShowMouseCursor || !Source || !HasValidConfiguration())
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
    // 调用者只在无锁定状态进入；先提交身份、委托和Timer，再最后广播。
    LockedTarget = Target;
    OccludedSince = -1.0;
    const uint64 Serial = ++LockSerial;
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
    UE_LOG(LogETTargetLock, Display, TEXT("[M2-06A] Locked Source=%s Target=%s Timer=1"),
           *GetNameSafe(BoundSource.Get()), *GetNameSafe(Target));
    OnTargetChanged.Broadcast(nullptr, Target);
}

void UETTargetLockComponent::ClearTarget(const TCHAR* Reason)
{
    const bool bHadIdentity = LockedTarget.IsValid() || LockedTarget.IsStale();
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

## 4. 修改三个既有文件

每处原代码都是当前D基线的唯一锚点。只替换列出的片段，保留P / 资源命令、既有Possess与EndPlay、UI桥接。新增构造的替换保留了原SetupInputComponent签名，其原函数体不删除；不要把构造函数塞进原函数体。

### 4.1 `EmberTrial/Public/Framework/ETPlayerController.h`

**第1处：前向声明**。原位置：

```cpp
class APawn;
```

替换为：

```cpp
class APawn;
class UInputAction;
class UETTargetLockComponent;
```

**第2处：构造与只读组件入口**。原位置：

```cpp
public:
    // Core emits
```

替换为：

```cpp
public:
    AETPlayerController();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Target Lock")
    UETTargetLockComponent* GetTargetLockComponent() const;

    // Core emits
```

**第3处：组件所有权 / 输入资产**。原位置：

```cpp
private:
    void HandlePauseReleased();
```

替换为：

```cpp
private:
    void HandlePauseReleased();

    // Controller跨Possess存在；目标身份由组件在换身体时主动清空。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Target Lock",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETTargetLockComponent> TargetLockComponent;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock")
    TObjectPtr<UInputAction> TargetLockAction;
    void HandleTargetLockStarted();
```

### 4.2 `EmberTrial/Private/Framework/ETPlayerController.cpp`

**第1处：直接includes**。原位置：

```cpp
#include "Framework/ETPlayerController.h"
```

替换为：

```cpp
#include "Framework/ETPlayerController.h"
#include "TargetLock/ETTargetLockComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
```

**第2处：原生子对象与Started处理**。原位置：

```cpp
void AETPlayerController::SetupInputComponent()
```

替换为：

```cpp
AETPlayerController::AETPlayerController()
{
    TargetLockComponent = CreateDefaultSubobject<UETTargetLockComponent>(TEXT("TargetLock"));
}

UETTargetLockComponent* AETPlayerController::GetTargetLockComponent() const
{
    return TargetLockComponent.Get();
}

void AETPlayerController::HandleTargetLockStarted()
{
    if (IsValid(TargetLockComponent.Get()))
    {
        TargetLockComponent->ToggleTargetLock();
    }
}

void AETPlayerController::SetupInputComponent()
```

**第3处：保留P，追加唯一Enhanced Input绑定**。原位置：

```cpp
        PauseBinding.bExecuteWhenPaused = false;
```

替换为：

```cpp
        PauseBinding.bExecuteWhenPaused = false;

        UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
        if (EnhancedInput && IsValid(TargetLockAction.Get()) &&
            TargetLockAction->ValueType == EInputActionValueType::Boolean)
        {
            EnhancedInput->BindAction(TargetLockAction.Get(), ETriggerEvent::Started,
                                      this, &ThisClass::HandleTargetLockStarted);
        }
        else
        {
            UE_LOG(LogETPlayerController, Display,
                   TEXT("[M2-06A] Target lock input unavailable Controller=%s"), *GetName());
        }
```

### 4.3 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

**第1处：专项出生前配置friend**。原位置：

```cpp
    friend class FETBreakPulseHUDTest;
```

替换为：

```cpp
    friend class FETBreakPulseHUDTest;
    friend class FETTargetLockSelectionTest;
```

### 4.4 完成后核对

- AETPlayerController新增一个构造和一个GetTargetLockComponent实现，CreateDefaultSubobject名称固定TargetLock，不能Blueprint再Add一份。
- P原IE_Released绑定保持；新IA只Started一次，不在Blueprint叠加Tab事件，不接Triggered / Tick。
- InputComponent沿共享DefaultInput.ini使用EnhancedInputComponent；绑定失败有[M2-06A] Target lock input unavailable日志，先查Controller Class Defaults的IA引用，不清缓存。
- Character只增加FETTargetLockSelectionTest的friend；旧19项测试和动作 / 资源 / 相机实现不变。

## 5. 新增选择与生命周期专项

新测试使用真实World、原生身体 / ASC / CombatLife、Controller和Overlap / 射线；无新Probe或资产依赖。检查前向评分 / Neutral排除、背向、同分稳定性、Waiting、非法迟滞、真实Toggle / 单Timer、换控、遮挡宽限 / Pause、目标死亡 / 销毁、旧代次、距离、失活、源死亡与EndPlay。初始算法用显式视点，另走Controller真实视点的Toggle入口；不证明真实键鼠焦点、屏幕标记或未来镜头手感。

新文件全文：

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetLockComponent.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Framework/ETPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETTargetLockSelectionTest, "EmberTrial.M2.TargetLock.Selection",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETTargetLockSelectionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto SpawnBody = [World, Spawn](FVector Location, EETCombatTeam Team)
    {
        AEmberTrialCharacter* Body =
            World->SpawnActor<AEmberTrialCharacter>(Location, FRotator::ZeroRotator, Spawn);
        if (Body)
        {
            Body->CombatTeam = Team;
            Body->GetCharacterMovement()->DisableMovement();
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
        }
        return Body;
    };
    AEmberTrialCharacter* Source = SpawnBody(FVector(0, 0, 200), EETCombatTeam::Player);
    AEmberTrialCharacter* Front = SpawnBody(FVector(600, 0, 200), EETCombatTeam::Enemy);
    AEmberTrialCharacter* Side = SpawnBody(FVector(350, 400, 200), EETCombatTeam::Enemy);
    AEmberTrialCharacter* Rear = SpawnBody(FVector(-800, 0, 200), EETCombatTeam::Enemy);
    AEmberTrialCharacter* Neutral = SpawnBody(FVector(250, 0, 200), EETCombatTeam::Neutral);
    AETPlayerController* PC = World->SpawnActor<AETPlayerController>();
    if (!TestNotNull(TEXT("Source"), Source) || !TestNotNull(TEXT("Front"), Front) ||
        !TestNotNull(TEXT("Side"), Side) || !TestNotNull(TEXT("Rear"), Rear) ||
        !TestNotNull(TEXT("Neutral"), Neutral) || !TestNotNull(TEXT("PC"), PC))
    {
        return false;
    }
    PC->Possess(Source);
    PC->SetControlRotation(FRotator::ZeroRotator);
    PC->SetViewTarget(Source);
    Source->GetCharacterMovement()->DisableMovement();
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    auto Advance = [&Fixture](float Seconds)
    {
        for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
        {
            if (!Fixture.TickTestWorld(.01f))
            {
                return false;
            }
        }
        return true;
    };
    UETTargetLockComponent* Lock = PC->GetTargetLockComponent();
    if (!TestNotNull(TEXT("Native single component"), Lock) || !Advance(.05f))
    {
        return false;
    }
    TestTrue(TEXT("Native source is ready"), Source->IsGameplayReady());
    TestFalse(TEXT("Idle has no monitor timer"), Lock->MonitorTimer.IsValid());
    const FVector View(-400, 0, 200);
    TestTrue(TEXT("View weighting beats shorter distance; neutral excluded"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector) == Front);
    TestTrue(TEXT("Opposite view selects rear enemy"),
             Lock->FindBestTarget(Source, View, -FVector::ForwardVector) == Rear);
    AEmberTrialCharacter* Twin = World->SpawnActorDeferred<AEmberTrialCharacter>(
        AEmberTrialCharacter::StaticClass(), FTransform(FVector(600, 0, 200)), nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!TestNotNull(TEXT("Tie body"), Twin))
    {
        return false;
    }
    Twin->CombatTeam = EETCombatTeam::Enemy;
    Twin->FinishSpawning(FTransform(FVector(600, 0, 200)));
    Twin->GetCharacterMovement()->DisableMovement();
    Twin->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
    AEmberTrialCharacter* TieExpected =
        Front->GetPathName().Compare(Twin->GetPathName()) < 0 ? Front : Twin;
    TestTrue(TEXT("Equal score uses stable object path"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector) == TieExpected);
    Twin->Destroy();
    Front->SetGameplayReady(false);
    TestTrue(TEXT("Waiting body excluded"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector) == Side);
    Front->SetGameplayReady(true);
    Lock->BreakRadius = Lock->AcquireRadius - 1.0f;
    TestFalse(TEXT("Invalid hysteresis rejected"), Lock->ToggleTargetLock());
    Lock->BreakRadius = 1500.0f;
    if (!TestTrue(TEXT("Real toggle acquires"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    TestTrue(TEXT("Actual camera selects front"), Lock->GetLockedTarget() == Front);
    TestTrue(TEXT("One active target has monitor"),
             World->GetTimerManager().IsTimerActive(Lock->MonitorTimer));
    TestTrue(TEXT("Second toggle releases"), Lock->ToggleTargetLock());
    TestNull(TEXT("Released target"), Lock->GetLockedTarget());
    TestFalse(TEXT("Release stops timer"), Lock->MonitorTimer.IsValid());
    if (!TestTrue(TEXT("Acquire before unpossess"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    PC->UnPossess();
    TestNull(TEXT("Unpossess releases immediately"), Lock->GetLockedTarget());
    PC->Possess(Source);
    Source->GetCharacterMovement()->DisableMovement();
    TestNull(TEXT("Repossess does not inherit a lock"), Lock->GetLockedTarget());
    if (!Lock->ToggleTargetLock())
    {
        return false;
    }
    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Wall"), Wall))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(20, 1500, 1500));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(200, 0, 200));
    TestNull(TEXT("Acquisition never selects through a blocking wall"),
             Lock->FindBestTarget(Source, View, FVector::ForwardVector));
    if (!Advance(.20f))
    {
        return false;
    }
    TestTrue(TEXT("Brief occlusion retains identity"), Lock->GetLockedTarget() == Front);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestFalse(TEXT("Paused request is rejected"), Lock->ToggleTargetLock());
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("Pause does not spend occlusion grace"), Lock->GetLockedTarget() == Front);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    if (!Advance(.5f))
    {
        return false;
    }
    TestNull(TEXT("Sustained occlusion releases"), Lock->GetLockedTarget());
    TestFalse(TEXT("Occlusion stops monitor"), Lock->MonitorTimer.IsValid());
    Wall->Destroy();
    if (!Advance(.05f) || !TestTrue(TEXT("Acquire after wall removal"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    Front->GetAbilitySystemComponent()->SetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestNull(TEXT("Target death releases immediately"), Lock->GetLockedTarget());
    if (!TestTrue(TEXT("Dead front excluded; side acquired"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    TestTrue(TEXT("Remaining enemy selected"), Lock->GetLockedTarget() == Side);
    const uint64 OldSerial = Lock->LockSerial;
    Side->Destroy();
    TestNull(TEXT("Target EndPlay releases"), Lock->GetLockedTarget());
    // 销毁后不再读Side / 旧ASC；只检查新身份是否受旧代次影响。
    Rear->SetActorLocation(FVector(600, 0, 200));
    if (!TestTrue(TEXT("Another living enemy acquired"), Lock->ToggleTargetLock()))
    {
        return false;
    }
    Lock->MaintainLock(OldSerial);
    TestTrue(TEXT("Stale monitor cannot clear new target"), Lock->GetLockedTarget() == Rear);
    Rear->SetActorLocation(FVector(2000, 0, 200));
    if (!Advance(.20f))
    {
        return false;
    }
    TestNull(TEXT("Break radius releases"), Lock->GetLockedTarget());
    Rear->SetActorLocation(FVector(600, 0, 200));
    if (!Lock->ToggleTargetLock())
    {
        return false;
    }
    Lock->Deactivate();
    TestNull(TEXT("Deactivate releases"), Lock->GetLockedTarget());
    TestFalse(TEXT("Inactive cannot reacquire"), Lock->ToggleTargetLock());
    Lock->Activate(false);
    if (!Lock->ToggleTargetLock())
    {
        return false;
    }
    Source->GetAbilitySystemComponent()->SetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestNull(TEXT("Source death releases"), Lock->GetLockedTarget());
    TestFalse(TEXT("Dead source cannot reacquire"), Lock->ToggleTargetLock());
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay clears timer"), Lock->MonitorTimer.IsValid());
    return !HasAnyErrors();
}
#endif
```

## 6. 正常构建与索引

六Source全部保存，UE仍关闭。在PowerShell逐条运行，首个Error出现就停止，发首条错误上下文：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

CLion Reload CMake Project。预期Core57 / UI20 / Editor3，共80cpp / 147Source，自动化20项（Core17 / UI3）；当前实际仍D的144 / 78 / 19，这些是实施后的预期。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标通过再打开UE。没有UI模块依赖变化，也不用重新生成整个项目 / 清DerivedDataCache。此指南只完成文本 / 引擎接口静态核对，助手未代编译或运行新专项。

## 7. 创建唯一Tab输入并配置原Controller

### 7.1 新Input Action

1. Content Browser进入`/Game/EmberTrial/Input/Actions`，右键 → Input → Input Action，命名**IA_ET_LockToggle**，不用Duplicate旧F / Q。
2. 打开，Value Type=**Digital (bool) / Boolean**；Triggers数组0项、Modifiers数组0项。不要增加Hold / Pressed Trigger，本次Started已经表示一次按下。
3. Trigger When Paused=False。Consume Input保持True，其他字段沿默认；Save。

### 7.2 在现有IMC_Default加一行

1. 打开`/Game/ThirdPerson/Input/IMC_Default`，路径与目前角色Default Mapping Context一致。**不是新建IMC，也不把它配到菜单Controller。**
2. Mappings点一次+，Action选择IA_ET_LockToggle，Key展开键盘选**Tab**；该行Triggers / Modifiers均0。
3. 核对只有一个IA_ET_LockToggle映射、只有这一条游戏内Tab锁定入口。原WASD / Space / 左键 / 2 / Q / F映射保留，不加P旧映射，不改菜单Tab导航。
4. Save。若搜索Tab看见旧正式动作也占用Tab，先报告该行内容，勿盲删既有功能；目前基线没有已设计的游戏内Tab动作。

### 7.3 Controller Class Defaults与继承组件

1. 打开`/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`。Class Settings核对原父类**ETPlayerController**，不要Reparent或新建Controller。
2. Components应自动出现一份继承**TargetLock**。不点Add Component；原唯一UIBootstrap与FailureFlow、五HUD Class / Pause / Failure配置全部保留。
3. 选TargetLock，Details搜索Target Lock，按表逐项核对：

| 字段 | 最终值 |
| --- | ---: |
| Acquire Radius | 1200cm |
| Break Radius | 1500cm |
| Acquire Half Angle | 60° |
| View Weight | .75 |
| Monitor Interval | .10s |
| Occlusion Grace | .35s |
| Auto Activate | True |

4. 点击工具栏**Class Defaults**，Details搜索**Target Lock Action**，选IA_ET_LockToggle。这个输入资产是Controller类属性，不在TargetLock组件的六个数值里；搜索Heal / F等角色属性也找不到它。
5. 不给Character / 敌人 / 菜单Controller添加TargetLock或新IA引用，不在EventGraph接Tab或Toggle函数。Compile → Save，IA与IMC也Save。
6. 回L_ET_VerticalSlice。本批没有新WBP，旧五HUD不变，也不做新的BindWidget校验规则。TargetLock不勾选Tick / Debug Draw。

## 8. 方案B：一项新自动化与实机短查（约8–12分钟）

### 8.1 自动化先行

Stop PIE，Output Log执行：

```text
Automation RunTests EmberTrial.M2.TargetLock.Selection
```

只需一次Success；本批没有改F / 治疗 / 连段状态机，旧19项沿用，不机械全组重跑。失败发首条断言 / 文件行号与完成时分；若闪退保留Saved/Crashes，先停，不反复跑同一崩溃。

### 8.2 Tab请求、筛选与相机边界（约3分钟）

1. Play，当前单卡与资源默认正常；朝向MeleeEnemy_A，距离约3–8米，目标中心在画面前方。按一下Tab，Output Log应出现**[M2-06A] Locked Source=当前玩家 Target=BP_ETMeleeEnemy... Timer=1**。
2. **镜头不自动转、没有锁定圆点是本章预期**。保持自由镜头；长按Tab不出现反复Locked / Cleared。松开再按一次Tab，出现Cleared Reason=ToggleOff Timer=0。
3. 无锁定时镜头转向没有敌人的方向，再Tab应NoCandidate；Neutral训练目标即使近且在前也不该成为Locked。再转回真正敌人可重新获取。
4. 持有锁定时敌人走到旁侧不应自动换目标；可自由转镜头，不因单独移出首次圆锥就解除。目标超过1500或长遮挡才脱锁；本批不要求你同时精确量距离 / .35秒，自动化已覆盖。

### 8.3 真实死亡、Pause与世界重建（约4–6分钟）

1. Tab锁定真正敌人，仍用正式左键三段 / 2 / F交战；锁定自身不改伤害。敌人真实死亡时应Cleared Reason=DeathSealed / Timer0，无旧敌人继续被锁。受击 / F破势后仍存活则允许保留锁定。
2. 重开或Retry使敌人恢复，Tab重新锁定。P暂停，菜单内按 / 松Tab，以鼠标或键盘继续，**不补切换锁定**；基础移动 / 镜头不受影响。无需搭临时Controller函数。
3. 敌人存活且锁定时，用原4或敌人致死玩家，Failure正常且目标清空；Retry新身体无旧锁，Tab可重新获取。冷却HUD / 生命 / 体力 / 次数新默认正常。
4. 锁定中Stop，重开无旧Locked身份 / Timer；最后左键-2-Q-E-P与移动 / 转向 / 跳跃短查。Q需真实受伤且有次数；满血拒绝正常，不为这一行重做旧矩阵。

这次不临时复制第二敌人、不搭挡板矩阵；排序 / 同分 / 遮挡的稳定规则由新专项覆盖。真实镜头 / 标记留B，双目标切换再按路线接入，不提前声称全锁定体验完成。

### 8.4 保存恢复

Stop → Save All：TargetLock六默认值 / AutoActivate、单IA与Tab行、Controller唯一原生TargetLock / 原UI组件、默认地图 / 三正式身体100/100与原GA / slomo1 / Debug False保持。Source只改本章六文件；没有临时修改填“未改”。

## 9. 反馈与排错

```text
M2-06A 完成
三新 / 三旧Source；关闭UE后Editor Build / Core57-UI20-Editor3索引 / Game Build：
IA Boolean / 无Triggers / 唯一IMC Tab；原Controller单继承TargetLock / 六默认值 / Action引用 / Compile-Save：
TargetLock.Selection一次：
Tab获取真实敌人 / 长按单次 / 再按解除 / Neutral与背向拒绝：
真实敌人死亡清空；存活受击或破势保持；原能力伤害：
P鼠标或键盘继续 / Tab无补执行 / 玩家死亡Retry与Stop新身体无旧锁：
旧五HUD与左键-2-Q-E-P / 移动转向跳跃短基线：
Source / IA / IMC / Controller Save All，临时配置恢复或未改：
失败或特别说明（没有镜头跟随 / 标记属于本章预期）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

没有Locked日志先看[M2-06A] Target lock input unavailable、Controller的IA引用、原生继承组件和IMC映射，不给Core加UI依赖。已有NoCandidate表示请求已到组件，再核对真实Enemy、Ready / 存活、视角、距离和Visibility。Error发文字 / 路径行号 / 前后约20行；配置截图需含当前资产名与Details，画面异常给短视频与按键顺序。完成时分由我关联日志，不要求手抄成功段。

## 10. 本章 Commit / Push（固定放在最后）

**实施且验收后**Stage三新Source（均加前缀EmberTrial/Source/）：

```text
EmberTrial/Public/TargetLock/ETTargetLockComponent.h
EmberTrial/Private/TargetLock/ETTargetLockComponent.cpp
EmberTrial/Tests/ETTargetLockTests.cpp
```

三旧Source：

```text
EmberTrial/Public/Framework/ETPlayerController.h
EmberTrial/Private/Framework/ETPlayerController.cpp
EmberTrial/Public/Characters/EmberTrialCharacter.h
```

一新 / 两旧UE资产：

```text
EmberTrial/Content/EmberTrial/Input/Actions/IA_ET_LockToggle.uasset
EmberTrial/Content/ThirdPerson/Input/IMC_Default.uasset
EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset
```

文档：本指南、M2-06路线、完成后生成的A报告与实际来源（当前71，实物确认新增IA后才72）、Progress / Backlog / Architecture / 验收方案 / VersionControl / M2路线 / README / Introduction最新收尾及下一批指南；尚未提交的D / M2-05报告一起包含。未改旧地图 / 五HUD / 角色 / GA / Config不制造差异。现在没有的新文件 / 报告不提前建空骨架提交。

建议Commit（英文）：

```text
feat: add scored target selection and lock lifecycle
```

中文译文：

```text
feat: 新增锁定目标评分筛选与锁定生命周期管理
```

英文可作标题，中文译文供理解或作为提交正文；不需要把两条重复拼成同一行。后续每份操作文档都按这一格式提供术语准确的中英建议。

CLion刷新Changes → Stage最新完整内容（已Stage后又修改需要再Stage）→ 检查Staged新文件不为空、物理路径正确、资产由LFS管理 → Commit → Push。Raw / Saved / Binaries / Intermediate / 索引 / 日志 / 包体留本地，不Force Add。用户操作Git，助手未改暂存区、Commit、Push或联网核验远端。
