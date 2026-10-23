#include "Presenters/ETBreakPulsePresenter.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogETBreakPulsePresenter, Log, All);

void UETBreakPulsePresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
{
    StopObservingInternal(false);
    if (!IsValid(Subsystem))
    {
        RefreshSnapshot(TEXT("MissingSubsystem"));
        return;
    }
    bObserving = true;
    BoundSubsystem = Subsystem;
    Subsystem->OnObservedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandleObservedPawnChanged);
    RebindObservedPawn(TEXT("StartSnapshot"));
}

void UETBreakPulsePresenter::Shutdown()
{
    StopObservingInternal(true);
}

void UETBreakPulsePresenter::BeginDestroy()
{
    StopObservingInternal(false);
    Super::BeginDestroy();
}

FETBreakPulseViewData UETBreakPulsePresenter::GetSnapshot() const
{
    return Snapshot;
}

void UETBreakPulsePresenter::SetPresentationEnabled(bool bEnabled)
{
    bPresentationEnabled = bEnabled;
    RefreshSnapshot(TEXT("PresentationChanged"));
}

// ---- 身份先失效，再释放旧来源的三个Tag委托与自己的Timer ----
void UETBreakPulsePresenter::StopObservingInternal(bool bNotify)
{
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(this, &ThisClass::HandleObservedPawnChanged);
    }
    ClearPawnBindings();
    BoundSubsystem.Reset();
    // 保留旧快照到下一次Refresh比较，失败重绑也能发布Ready→Unavailable；不清View订阅。
    if (bNotify)
    {
        RefreshSnapshot(TEXT("Shutdown"));
    }
}

void UETBreakPulsePresenter::StopTimers()
{
    const bool bHadPeriodic = PeriodicRefreshTimer.IsValid();
    if (UWorld* World = TimerWorld.Get())
    {
        World->GetTimerManager().ClearTimer(DeferredRefreshTimer);
        World->GetTimerManager().ClearTimer(PeriodicRefreshTimer);
    }
    DeferredRefreshTimer.Invalidate();
    PeriodicRefreshTimer.Invalidate();
    if (bHadPeriodic)
    {
        UE_LOG(LogETBreakPulsePresenter, Display,
               TEXT("[M2-05D] RefreshTimer Presenter=%s Active=0"), *GetName());
    }
}

void UETBreakPulsePresenter::ClearPawnBindings()
{
    ++ObservationSerial;
    StopTimers();
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->UnregisterGameplayTagEvent(CooldownChangedHandle, ETGameplayTags::Cooldown_BreakPulse);
        ASC->UnregisterGameplayTagEvent(CastingChangedHandle,
                                        ETGameplayTags::State_CastingBreakPulse);
        ASC->UnregisterGameplayTagEvent(DeathChangedHandle, ETGameplayTags::State_Dead);
    }
    CooldownChangedHandle.Reset();
    CastingChangedHandle.Reset();
    DeathChangedHandle.Reset();
    if (AEmberTrialCharacter* Body = BoundCharacter.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleGameplayReadyChanged);
    }
    BoundCharacter.Reset();
    BoundASC.Reset();
    TimerWorld.Reset();
}

void UETBreakPulsePresenter::RebindObservedPawn(const TCHAR* Reason)
{
    ClearPawnBindings();
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    AEmberTrialCharacter* Body =
        Subsystem ? Cast<AEmberTrialCharacter>(Subsystem->GetObservedPawn()) : nullptr;
    if (IsValid(Body) && Body->GetCombatTeam() == EETCombatTeam::Player)
    {
        BoundCharacter = Body;
        TimerWorld = Body->GetWorld();
        Body->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleGameplayReadyChanged);
        if (UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent())
        {
            BoundASC = ASC;
            CooldownChangedHandle =
                ASC->RegisterGameplayTagEvent(ETGameplayTags::Cooldown_BreakPulse)
                    .AddUObject(this, &ThisClass::HandleTagChanged, BoundASC, ObservationSerial);
            CastingChangedHandle =
                ASC->RegisterGameplayTagEvent(ETGameplayTags::State_CastingBreakPulse)
                    .AddUObject(this, &ThisClass::HandleTagChanged, BoundASC, ObservationSerial);
            DeathChangedHandle =
                ASC->RegisterGameplayTagEvent(ETGameplayTags::State_Dead)
                    .AddUObject(this, &ThisClass::HandleTagChanged, BoundASC, ObservationSerial);
        }
    }
    const uint64 Serial = ObservationSerial;
    RefreshSnapshot(Reason);
    if (Serial == ObservationSerial && bObserving)
    {
        RequestDeferredRefresh();
    }
}

void UETBreakPulsePresenter::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (bObserving)
    {
        RebindObservedPawn(TEXT("PawnChanged"));
    }
}

void UETBreakPulsePresenter::HandleGameplayReadyChanged(AEmberTrialCharacter* Character,
                                                        bool bReady)
{
    if (!bObserving || Character != BoundCharacter.Get())
    {
        return;
    }
    // Ready广播早于BeginPlay最后的能力授予；等一次NextTick，不永久轮询未授予状态。
    if (bReady)
    {
        RequestDeferredRefresh();
    }
    else
    {
        RefreshSnapshot(TEXT("ReadyWithdrawn"));
    }
}

void UETBreakPulsePresenter::HandleTagChanged(FGameplayTag Tag, int32 Count,
                                              TWeakObjectPtr<UAbilitySystemComponent> Source,
                                              uint64 Serial)
{
    (void)Tag;
    (void)Count;
    if (!bObserving || Serial != ObservationSerial || !Source.IsValid() || Source != BoundASC)
    {
        return;
    }
    // Tag回调可能位于GE应用 / 取消的中间；合并到下一游戏Tick再查最终Spec / 时长。
    RequestDeferredRefresh();
}

void UETBreakPulsePresenter::RequestDeferredRefresh()
{
    UWorld* World = TimerWorld.Get();
    if (!bObserving || !IsValid(World) || World->bIsTearingDown || DeferredRefreshTimer.IsValid())
    {
        return;
    }
    DeferredRefreshTimer = World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &ThisClass::HandleDeferredRefresh, ObservationSerial));
}

void UETBreakPulsePresenter::HandleDeferredRefresh(uint64 Serial)
{
    if (!bObserving || Serial != ObservationSerial)
    {
        return;
    }
    DeferredRefreshTimer.Invalidate();
    RefreshSnapshot(TEXT("EventSnapshot"));
}

void UETBreakPulsePresenter::HandlePeriodicRefresh(uint64 Serial)
{
    if (bObserving && Serial == ObservationSerial)
    {
        RefreshSnapshot(TEXT("CooldownRefresh"));
    }
}

// ---- 快照可用性不等于CanActivate；冷却就绪不能承诺移动 / 忙碌时允许施放 ----
FETBreakPulseViewData UETBreakPulsePresenter::BuildSnapshot() const
{
    FETBreakPulseViewData Data;
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    if (!bObserving || !IsValid(Subsystem))
    {
        return Data;
    }
    APawn* Pawn = Subsystem->GetObservedPawn();
    if (!IsValid(Pawn))
    {
        Data.State = EETBreakPulseViewState::NoPawn;
        return Data;
    }
    Data.PawnName = Pawn->GetName();
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(Pawn);
    if (!IsValid(Body) || Body->GetCombatTeam() != EETCombatTeam::Player)
    {
        Data.State = EETBreakPulseViewState::UnsupportedPawn;
        return Data;
    }
    if (!Body->IsGameplayReady())
    {
        Data.State = EETBreakPulseViewState::WaitingForGameplayReady;
        return Data;
    }
    if (Body->IsCombatDead())
    {
        Data.State = EETBreakPulseViewState::Dead;
        return Data;
    }
    UAbilitySystemComponent* ASC = BoundASC.Get();
    if (Body != BoundCharacter.Get() || !IsValid(ASC) || Body->GetAbilitySystemComponent() != ASC ||
        !Body->GetBreakPulseCooldownSnapshot(Data.bCoolingDown, Data.Remaining, Data.Duration))
    {
        return Data;
    }
    Data.State = EETBreakPulseViewState::Ready;
    Data.bCasting = ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse);
    Data.ReadyFraction = Data.bCoolingDown ? 1.0f - Data.Remaining / Data.Duration : 1.0f;
    Data.ReadyFraction = FMath::Clamp(Data.ReadyFraction, 0.0f, 1.0f);
    return Data;
}

void UETBreakPulsePresenter::SyncPeriodicRefresh(const FETBreakPulseViewData& Data)
{
    UWorld* World = TimerWorld.Get();
    const bool bNeeded = bObserving && bPresentationEnabled &&
                         Data.State == EETBreakPulseViewState::Ready && Data.bCoolingDown &&
                         IsValid(World) && !World->bIsTearingDown;
    if (!bNeeded)
    {
        if (PeriodicRefreshTimer.IsValid())
        {
            if (IsValid(World))
            {
                World->GetTimerManager().ClearTimer(PeriodicRefreshTimer);
            }
            PeriodicRefreshTimer.Invalidate();
            UE_LOG(LogETBreakPulsePresenter, Display,
                   TEXT("[M2-05D] RefreshTimer Presenter=%s Active=0"), *GetName());
        }
        return;
    }
    if (!World->GetTimerManager().IsTimerActive(PeriodicRefreshTimer))
    {
        World->GetTimerManager().SetTimer(
            PeriodicRefreshTimer,
            FTimerDelegate::CreateUObject(this, &ThisClass::HandlePeriodicRefresh,
                                          ObservationSerial),
            .10f, true);
        UE_LOG(LogETBreakPulsePresenter, Display,
               TEXT("[M2-05D] RefreshTimer Presenter=%s Active=1 Interval=0.10"), *GetName());
    }
}

void UETBreakPulsePresenter::RefreshSnapshot(const TCHAR* Reason)
{
    const FETBreakPulseViewData NewData = BuildSnapshot();
    SyncPeriodicRefresh(NewData);
    if (Snapshot.State == NewData.State && Snapshot.PawnName == NewData.PawnName &&
        Snapshot.bCasting == NewData.bCasting && Snapshot.bCoolingDown == NewData.bCoolingDown &&
        FMath::IsNearlyEqual(Snapshot.Remaining, NewData.Remaining, .001f) &&
        FMath::IsNearlyEqual(Snapshot.Duration, NewData.Duration, .001f))
    {
        return;
    }
    const bool bStateChanged =
        Snapshot.State != NewData.State || Snapshot.PawnName != NewData.PawnName ||
        Snapshot.bCasting != NewData.bCasting || Snapshot.bCoolingDown != NewData.bCoolingDown;
    Snapshot = NewData;
    if (bStateChanged)
    {
        UE_LOG(LogETBreakPulsePresenter, Display,
               TEXT("[M2-05D] Snapshot Presenter=%s State=%s Pawn=%s Casting=%d Cooling=%d "
                    "Remaining=%.2f Reason=%s"),
               *GetName(),
               *StaticEnum<EETBreakPulseViewState>()->GetNameStringByValue(
                   static_cast<int64>(Snapshot.State)),
               *Snapshot.PawnName, Snapshot.bCasting ? 1 : 0, Snapshot.bCoolingDown ? 1 : 0,
               Snapshot.Remaining, Reason);
    }
    const FETBreakPulseViewData Published = Snapshot;
    // 广播可能关View / 重启观察；之后不再改实例字段或启动旧一轮Timer。
    OnViewDataChanged.Broadcast(Published);
}