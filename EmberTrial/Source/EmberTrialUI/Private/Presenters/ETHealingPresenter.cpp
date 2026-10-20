#include "Presenters/ETHealingPresenter.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameFramework/Pawn.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "Resources/ETHealingResourceComponent.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHealingPresenter, Log, All);

// ---- 观察生命周期与GC兜底 ----
void UETHealingPresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
{
    StopObservingInternal(false);
    if (!IsValid(Subsystem))
    {
        PublishSnapshot(TEXT("MissingSubsystem"));
        return;
    }
    BoundSubsystem = Subsystem;
    bObserving = true;
    Subsystem->OnObservedPawnChanged.AddUniqueDynamic(
        this, &UETHealingPresenter::HandleObservedPawnChanged);
    RebindObservedPawn(TEXT("StartSnapshot"));
}

void UETHealingPresenter::Shutdown()
{
    StopObservingInternal(true);
}

void UETHealingPresenter::BeginDestroy()
{
    StopObservingInternal(false);
    Super::BeginDestroy();
}

FETHealingViewData UETHealingPresenter::GetSnapshot() const
{
    return Snapshot;
}

void UETHealingPresenter::StopObservingInternal(bool bNotify)
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(
            this, &UETHealingPresenter::HandleObservedPawnChanged);
    }
    ClearPawnBindings();
    BoundSubsystem.Reset();
    if (bWasObserving)
    {
        UE_LOG(LogETHealingPresenter, Display, TEXT("[M2-04C] Stopped Presenter=%s"),
               *GetNameSafe(this));
    }
    if (bNotify)
    {
        PublishSnapshot(TEXT("Shutdown"));
    }
    else
    {
        Snapshot = FETHealingViewData{};
    }
}

// ---- 当前Pawn与资源组件的订阅所有权 ----
void UETHealingPresenter::ClearResourceBinding()
{
    ++ObservationSerial;
    if (UETHealingResourceComponent* Resource = BoundResource.Get())
    {
        Resource->OnSnapshotChanged.Remove(ResourceChangedHandle);
    }
    ResourceChangedHandle.Reset();
    BoundResource.Reset();
}

void UETHealingPresenter::ClearPawnBindings()
{
    ClearResourceBinding();
    if (AEmberTrialCharacter* Character = BoundCharacter.Get())
    {
        Character->OnGameplayReadyChanged.RemoveDynamic(
            this, &UETHealingPresenter::HandleGameplayReadyChanged);
    }
    BoundCharacter.Reset();
}

void UETHealingPresenter::RebindObservedPawn(const TCHAR* Reason)
{
    ClearPawnBindings();
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    AEmberTrialCharacter* Character =
        Subsystem ? Cast<AEmberTrialCharacter>(Subsystem->GetObservedPawn()) : nullptr;
    if (IsValid(Character))
    {
        BoundCharacter = Character;
        Character->OnGameplayReadyChanged.AddUniqueDynamic(
            this, &UETHealingPresenter::HandleGameplayReadyChanged);
    }
    RefreshResourceBinding(Reason);
}

void UETHealingPresenter::RefreshResourceBinding(const TCHAR* Reason)
{
    AEmberTrialCharacter* Character = BoundCharacter.Get();
    UETHealingResourceComponent* Resource =
        Character && Character->GetCombatTeam() == EETCombatTeam::Player
            ? Character->FindComponentByClass<UETHealingResourceComponent>()
            : nullptr;

    if (!IsValid(Resource))
    {
        ClearResourceBinding();
    }
    else if (BoundResource.Get() != Resource || !ResourceChangedHandle.IsValid())
    {
        ClearResourceBinding();
        BoundResource = Resource;
        // 不以快照当前不可用为由退订；之后Activate仍会发布可用事件。
        ResourceChangedHandle = Resource->OnSnapshotChanged.AddUObject(
            this, &UETHealingPresenter::HandleResourceChanged, BoundResource, ObservationSerial);
    }
    PublishSnapshot(Reason);
}

// ---- 事件只重读当前快照，不把旧事件Payload当作资源真值 ----
void UETHealingPresenter::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (bObserving)
    {
        RebindObservedPawn(TEXT("PawnChanged"));
    }
}

void UETHealingPresenter::HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady)
{
    (void)bReady;
    if (bObserving && Character == BoundCharacter.Get())
    {
        RefreshResourceBinding(TEXT("ReadyChanged"));
    }
}

void UETHealingPresenter::HandleResourceChanged(TWeakObjectPtr<UETHealingResourceComponent> Source,
                                                uint64 Serial)
{
    if (!bObserving || Serial != ObservationSerial || !Source.IsValid() ||
        Source.Get() != BoundResource.Get())
    {
        return;
    }
    PublishSnapshot(TEXT("ResourceChanged"));
}

// ---- 状态与整数快照；0次仍Ready，不能显示成未初始化 ----
FETHealingViewData UETHealingPresenter::BuildSnapshot() const
{
    FETHealingViewData Data;
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    if (!bObserving || !IsValid(Subsystem))
    {
        return Data;
    }
    APawn* Pawn = Subsystem->GetObservedPawn();
    if (!IsValid(Pawn))
    {
        Data.State = EETHealingViewState::NoPawn;
        return Data;
    }
    Data.PawnName = Pawn->GetName();
    AEmberTrialCharacter* Character = Cast<AEmberTrialCharacter>(Pawn);
    if (!IsValid(Character) || Character->GetCombatTeam() != EETCombatTeam::Player)
    {
        Data.State = EETHealingViewState::UnsupportedPawn;
        return Data;
    }
    if (!Character->IsGameplayReady())
    {
        Data.State = EETHealingViewState::WaitingForGameplayReady;
        return Data;
    }
    UETHealingResourceComponent* Resource = BoundResource.Get();
    int32 Charges = 0;
    int32 MaxCharges = 0;
    if (Character != BoundCharacter.Get() || !IsValid(Resource) ||
        Resource->GetOwner() != Character ||
        Character->FindComponentByClass<UETHealingResourceComponent>() != Resource ||
        !Resource->GetHealingSnapshot(Charges, MaxCharges) || MaxCharges <= 0 || Charges < 0 ||
        Charges > MaxCharges)
    {
        return Data;
    }
    Data.State = EETHealingViewState::Ready;
    Data.Charges = Charges;
    Data.MaxCharges = MaxCharges;
    return Data;
}

void UETHealingPresenter::PublishSnapshot(const TCHAR* Reason)
{
    const FETHealingViewData NewData = BuildSnapshot();
    if (Snapshot.State == NewData.State && Snapshot.PawnName == NewData.PawnName &&
        Snapshot.Charges == NewData.Charges && Snapshot.MaxCharges == NewData.MaxCharges)
    {
        return;
    }
    Snapshot = NewData;
    UE_LOG(LogETHealingPresenter, Display,
           TEXT("[M2-04C] Snapshot Presenter=%s State=%s Pawn=%s Charges=%d/%d Reason=%s"),
           *GetNameSafe(this),
           *StaticEnum<EETHealingViewState>()->GetNameStringByValue(
               static_cast<int64>(Snapshot.State)),
           *Snapshot.PawnName, Snapshot.Charges, Snapshot.MaxCharges, Reason);
    const FETHealingViewData Published = Snapshot;
    OnViewDataChanged.Broadcast(Published);
}