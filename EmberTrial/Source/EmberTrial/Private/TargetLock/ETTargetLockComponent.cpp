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