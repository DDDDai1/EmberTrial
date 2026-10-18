#include "Presenters/ETHealthPresenter.h"
#include "Presenters/ETResourceViewMath.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameFramework/Pawn.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHealthPresenter, Log, All);

// ---- 观察生命周期与GC兜底 ----

void UETHealthPresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
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
        this, &UETHealthPresenter::HandleObservedPawnChanged);
    RebindObservedPawn(TEXT("StartSnapshot"));
}

void UETHealthPresenter::Shutdown()
{
    StopObservingInternal(true);
}

void UETHealthPresenter::BeginDestroy()
{
    // GC兜底不广播给View，正常清理由Bootstrap的EndPlay负责。
    StopObservingInternal(false);
    Super::BeginDestroy();
}

FETHealthViewData UETHealthPresenter::GetSnapshot() const
{
    return Snapshot;
}

void UETHealthPresenter::StopObservingInternal(bool bNotify)
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(
            this, &UETHealthPresenter::HandleObservedPawnChanged);
    }
    ClearPawnBindings();
    BoundSubsystem.Reset();

    if (bWasObserving)
    {
        UE_LOG(LogETHealthPresenter, Display, TEXT("[M1-03B1] Stopped Presenter=%s"),
               *GetNameSafe(this));
    }
    if (bNotify)
    {
        PublishSnapshot(TEXT("Shutdown"));
    }
    else
    {
        Snapshot = FETHealthViewData{};
    }
}

// ---- 属性 / 身体身份重绑；旧数据源不能影响当前View ----

void UETHealthPresenter::ClearAttributeBindings()
{
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetHealthAttribute())
            .Remove(HealthHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetMaxHealthAttribute())
            .Remove(MaxHealthHandle);
    }
    HealthHandle.Reset();
    MaxHealthHandle.Reset();
    BoundASC.Reset();
}

void UETHealthPresenter::ClearPawnBindings()
{
    ClearAttributeBindings();
    if (AEmberTrialCharacter* Character = BoundCharacter.Get())
    {
        Character->OnGameplayReadyChanged.RemoveDynamic(
            this, &UETHealthPresenter::HandleGameplayReadyChanged);
    }
    BoundCharacter.Reset();
}

void UETHealthPresenter::RebindObservedPawn(const TCHAR* Reason)
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
            this, &UETHealthPresenter::HandleGameplayReadyChanged);
    }
    RefreshReadyBinding(Reason);
}

void UETHealthPresenter::RefreshReadyBinding(const TCHAR* Reason)
{
    AEmberTrialCharacter* Character = BoundCharacter.Get();
    UAbilitySystemComponent* ASC = Character && Character->IsGameplayReady()
                                       ? Character->GetAbilitySystemComponent()
                                       : nullptr;

    if (!IsValid(ASC))
    {
        ClearAttributeBindings();
    }
    else if (BoundASC.Get() != ASC || !HealthHandle.IsValid() || !MaxHealthHandle.IsValid())
    {
        ClearAttributeBindings();
        BoundASC = ASC;
        // Payload也是弱引用，用于拒绝旧ASC回调。
        HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(
                              UETHealthAttributeSet::GetHealthAttribute())
                           .AddUObject(this, &UETHealthPresenter::HandleAttributeChanged, BoundASC);
        MaxHealthHandle =
            ASC->GetGameplayAttributeValueChangeDelegate(
                   UETHealthAttributeSet::GetMaxHealthAttribute())
                .AddUObject(this, &UETHealthPresenter::HandleAttributeChanged, BoundASC);
    }
    PublishSnapshot(Reason);
}

// ---- 事件驱动刷新，不做Timer / Tick轮询 ----

void UETHealthPresenter::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (bObserving)
    {
        RebindObservedPawn(TEXT("PawnChanged"));
    }
}

void UETHealthPresenter::HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady)
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

void UETHealthPresenter::HandleAttributeChanged(const FOnAttributeChangeData& Change,
                                                TWeakObjectPtr<UAbilitySystemComponent> SourceASC)
{
    if (!bObserving || !SourceASC.IsValid() || SourceASC.Get() != BoundASC.Get())
    {
        return;
    }
    PublishSnapshot(Change.Attribute == UETHealthAttributeSet::GetHealthAttribute()
                        ? TEXT("HealthChanged")
                        : TEXT("MaxHealthChanged"));
}

// ---- 当前快照、合法区间与去重发布；共享数值检查，不合并两种资源状态 ----

FETHealthViewData UETHealthPresenter::BuildSnapshot() const
{
    FETHealthViewData Data;
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    if (!bObserving || !IsValid(Subsystem))
    {
        return Data;
    }
    APawn* Pawn = Subsystem->GetObservedPawn();
    if (!IsValid(Pawn))
    {
        Data.State = EETHealthViewState::NoPawn;
        return Data;
    }
    Data.PawnName = Pawn->GetName();
    AEmberTrialCharacter* Character = Cast<AEmberTrialCharacter>(Pawn);
    if (!IsValid(Character))
    {
        Data.State = EETHealthViewState::UnsupportedPawn;
        return Data;
    }
    if (!Character->IsGameplayReady())
    {
        Data.State = EETHealthViewState::WaitingForGameplayReady;
        return Data;
    }

    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (Character != BoundCharacter.Get() || !BoundASC.IsValid() ||
        Character->GetAbilitySystemComponent() != BoundASC.Get() ||
        !Character->GetGameplayHealthSnapshot(Health, MaxHealth) ||
        !ETResourceViewMath::IsValidRange(Health, MaxHealth))
    {
        return Data;
    }
    Data.State = EETHealthViewState::Ready;
    Data.Health = Health;
    Data.MaxHealth = MaxHealth;
    Data.HealthFraction = FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
    return Data;
}

void UETHealthPresenter::PublishSnapshot(const TCHAR* Reason, bool bForce)
{
    const FETHealthViewData NewData = BuildSnapshot();
    const bool bSame = Snapshot.State == NewData.State && Snapshot.PawnName == NewData.PawnName &&
                       Snapshot.Health == NewData.Health &&
                       Snapshot.MaxHealth == NewData.MaxHealth &&
                       Snapshot.HealthFraction == NewData.HealthFraction;
    if (bSame && !bForce)
    {
        return;
    }
    Snapshot = NewData;
    UE_LOG(
        LogETHealthPresenter, Display,
        TEXT("[M1-03B1] Snapshot Presenter=%s State=%s Pawn=%s Health=%.1f MaxHealth=%.1f "
             "Fraction=%.3f Reason=%s"),
        *GetNameSafe(this),
        *StaticEnum<EETHealthViewState>()->GetNameStringByValue(static_cast<int64>(Snapshot.State)),
        *Snapshot.PawnName, Snapshot.Health, Snapshot.MaxHealth, Snapshot.HealthFraction, Reason);
    OnViewDataChanged.Broadcast(Snapshot);
}
