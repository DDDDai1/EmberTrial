#include "Presenters/ETStaminaPresenter.h"
#include "Presenters/ETResourceViewMath.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameFramework/Pawn.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStaminaPresenter, Log, All);

// ---- 观察生命周期与GC兜底 ----

void UETStaminaPresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
{
    // 保留View订阅；只清理本Presenter对数据源的订阅。
    StopObservingInternal(false);
    if (!IsValid(Subsystem))
    {
        PublishSnapshot(TEXT("MissingSubsystem"), true);
        return;
    }

    BoundSubsystem = Subsystem;
    bObserving = true;
    Subsystem->OnObservedPawnChanged.AddUniqueDynamic(
        this, &UETStaminaPresenter::HandleObservedPawnChanged);
    RebindObservedPawn(TEXT("StartSnapshot"));
}

void UETStaminaPresenter::Shutdown()
{
    StopObservingInternal(true);
}

void UETStaminaPresenter::BeginDestroy()
{
    // GC兜底不广播给View，正常清理由Bootstrap的EndPlay负责。
    StopObservingInternal(false);
    Super::BeginDestroy();
}

FETStaminaViewData UETStaminaPresenter::GetSnapshot() const
{
    return Snapshot;
}

void UETStaminaPresenter::StopObservingInternal(bool bNotify)
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(
            this, &UETStaminaPresenter::HandleObservedPawnChanged);
    }
    ClearPawnBindings();
    BoundSubsystem.Reset();

    if (bWasObserving)
    {
        UE_LOG(LogETStaminaPresenter, Display, TEXT("[M2-01B2] Stopped Presenter=%s"),
               *GetNameSafe(this));
    }
    if (bNotify)
    {
        PublishSnapshot(TEXT("Shutdown"));
    }
    else
    {
        Snapshot = FETStaminaViewData{};
    }
}

// ---- 属性 / 身体身份重绑；旧数据源不能影响当前View ----

void UETStaminaPresenter::ClearAttributeBindings()
{
    ++ObservationSerial;
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UETStaminaAttributeSet::GetStaminaAttribute())
            .Remove(StaminaHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(
               UETStaminaAttributeSet::GetMaxStaminaAttribute())
            .Remove(MaxStaminaHandle);
    }
    StaminaHandle.Reset();
    MaxStaminaHandle.Reset();
    BoundASC.Reset();
}

void UETStaminaPresenter::ClearPawnBindings()
{
    ClearAttributeBindings();
    if (AEmberTrialCharacter* Character = BoundCharacter.Get())
    {
        Character->OnGameplayReadyChanged.RemoveDynamic(
            this, &UETStaminaPresenter::HandleGameplayReadyChanged);
    }
    BoundCharacter.Reset();
}

void UETStaminaPresenter::RebindObservedPawn(const TCHAR* Reason)
{
    ClearPawnBindings();
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    AEmberTrialCharacter* Character =
        Subsystem ? Cast<AEmberTrialCharacter>(Subsystem->GetObservedPawn()) : nullptr;
    if (IsValid(Character))
    {
        BoundCharacter = Character;
        // 先订阅，再查询，兼容就绪通知早于Presenter创建。
        Character->OnGameplayReadyChanged.AddUniqueDynamic(
            this, &UETStaminaPresenter::HandleGameplayReadyChanged);
    }
    RefreshReadyBinding(Reason);
}

void UETStaminaPresenter::RefreshReadyBinding(const TCHAR* Reason)
{
    AEmberTrialCharacter* Character = BoundCharacter.Get();
    UAbilitySystemComponent* ASC = Character && Character->IsGameplayReady()
                                       ? Character->GetAbilitySystemComponent()
                                       : nullptr;

    if (!IsValid(ASC))
    {
        ClearAttributeBindings();
    }
    else if (BoundASC.Get() != ASC || !StaminaHandle.IsValid() || !MaxStaminaHandle.IsValid())
    {
        ClearAttributeBindings();
        BoundASC = ASC;
        // Payload也是弱引用，用于拒绝旧ASC回调。
        StaminaHandle = ASC->GetGameplayAttributeValueChangeDelegate(
                               UETStaminaAttributeSet::GetStaminaAttribute())
                            .AddUObject(this, &UETStaminaPresenter::HandleAttributeChanged,
                                        BoundASC, ObservationSerial);
        MaxStaminaHandle = ASC->GetGameplayAttributeValueChangeDelegate(
                                  UETStaminaAttributeSet::GetMaxStaminaAttribute())
                               .AddUObject(this, &UETStaminaPresenter::HandleAttributeChanged,
                                           BoundASC, ObservationSerial);
    }
    PublishSnapshot(Reason);
}

// ---- 事件驱动刷新，不做Timer / Tick轮询 ----

void UETStaminaPresenter::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (bObserving)
    {
        RebindObservedPawn(TEXT("PawnChanged"));
    }
}

void UETStaminaPresenter::HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady)
{
    if (!bObserving || Character != BoundCharacter.Get())
    {
        return;
    }
    if (!bReady)
    {
        ClearAttributeBindings();
        PublishSnapshot(TEXT("ReadyFalse"));
        return;
    }
    RefreshReadyBinding(TEXT("ReadyTrue"));
}

void UETStaminaPresenter::HandleAttributeChanged(const FOnAttributeChangeData& Change,
                                                 TWeakObjectPtr<UAbilitySystemComponent> SourceASC,
                                                 uint64 Serial)
{
    if (Serial != ObservationSerial || !bObserving || !SourceASC.IsValid() ||
        SourceASC.Get() != BoundASC.Get())
    {
        return;
    }
    PublishSnapshot(Change.Attribute == UETStaminaAttributeSet::GetStaminaAttribute()
                        ? TEXT("StaminaChanged")
                        : TEXT("MaxStaminaChanged"));
}

// ---- 当前快照、合法区间与去重发布；共享数值检查，不合并两种资源状态 ----

FETStaminaViewData UETStaminaPresenter::BuildSnapshot() const
{
    FETStaminaViewData Data;
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    if (!bObserving || !IsValid(Subsystem))
    {
        return Data;
    }
    APawn* Pawn = Subsystem->GetObservedPawn();
    if (!IsValid(Pawn))
    {
        Data.State = EETStaminaViewState::NoPawn;
        return Data;
    }
    Data.PawnName = Pawn->GetName();
    AEmberTrialCharacter* Character = Cast<AEmberTrialCharacter>(Pawn);
    if (!IsValid(Character))
    {
        Data.State = EETStaminaViewState::UnsupportedPawn;
        return Data;
    }
    if (!Character->IsGameplayReady())
    {
        Data.State = EETStaminaViewState::WaitingForGameplayReady;
        return Data;
    }

    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    if (Character != BoundCharacter.Get() || !BoundASC.IsValid() ||
        Character->GetAbilitySystemComponent() != BoundASC.Get() ||
        !Character->GetGameplayStaminaSnapshot(Stamina, MaxStamina) ||
        !ETResourceViewMath::IsValidRange(Stamina, MaxStamina))
    {
        return Data;
    }
    Data.State = EETStaminaViewState::Ready;
    Data.Stamina = Stamina;
    Data.MaxStamina = MaxStamina;
    Data.StaminaFraction = FMath::Clamp(Stamina / MaxStamina, 0.0f, 1.0f);
    return Data;
}

void UETStaminaPresenter::PublishSnapshot(const TCHAR* Reason, bool bForce)
{
    const FETStaminaViewData NewData = BuildSnapshot();
    const bool bSame = Snapshot.State == NewData.State && Snapshot.PawnName == NewData.PawnName &&
                       Snapshot.Stamina == NewData.Stamina &&
                       Snapshot.MaxStamina == NewData.MaxStamina &&
                       Snapshot.StaminaFraction == NewData.StaminaFraction;
    if (bSame && !bForce)
    {
        return;
    }
    Snapshot = NewData;
    UE_LOG(LogETStaminaPresenter, Verbose,
           TEXT("[M2-01B2] Snapshot Presenter=%s State=%s Pawn=%s Stamina=%.1f MaxStamina=%.1f "
                "Fraction=%.3f Reason=%s"),
           *GetNameSafe(this),
           *StaticEnum<EETStaminaViewState>()->GetNameStringByValue(
               static_cast<int64>(Snapshot.State)),
           *Snapshot.PawnName, Snapshot.Stamina, Snapshot.MaxStamina, Snapshot.StaminaFraction,
           Reason);
    const FETStaminaViewData Published = Snapshot;
    OnViewDataChanged.Broadcast(Published);
}
