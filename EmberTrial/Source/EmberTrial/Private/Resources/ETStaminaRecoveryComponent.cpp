#include "Resources/ETStaminaRecoveryComponent.h"
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "Containers/ArrayView.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStaminaRecovery, Log, All);

namespace
{
TConstArrayView<FGameplayTag> GetStaminaRecoveryBlockingTags()
{
    // 首次组件使用时原生Tag已注册；之后只返回只读视图，注册 / 解绑不复制两份名单。
    static const FGameplayTag Tags[] = {
        ETGameplayTags::State_Attacking, ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead,
        ETGameplayTags::State_Dodging, ETGameplayTags::State_Healing};
    return Tags;
}
} // namespace

// ---- 组件生命周期与观察开关 ----

UETStaminaRecoveryComponent::UETStaminaRecoveryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UETStaminaRecoveryComponent::BeginPlay()
{
    Super::BeginPlay();
    bEndingPlay = false;
    BodyOwner = Cast<AEmberTrialCharacter>(GetOwner());
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleGameplayReadyChanged);
    }
    RefreshObservation();
}

void UETStaminaRecoveryComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    if (HasBegunPlay() && !bEndingPlay)
    {
        RefreshObservation();
    }
}

void UETStaminaRecoveryComponent::Deactivate()
{
    Super::Deactivate();
    StopObserving();
}

void UETStaminaRecoveryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleGameplayReadyChanged);
    }
    StopObserving();
    BodyOwner.Reset();
    Super::EndPlay(EndPlayReason);
}

// ---- 按需Timer与ASC订阅的配对清理 ----

bool UETStaminaRecoveryComponent::IsRecoveryScheduled() const
{
    const UWorld* World = GetWorld();
    return World && World->GetTimerManager().TimerExists(RecoveryTimer);
}

void UETStaminaRecoveryComponent::ClearRecoveryTimer(const TCHAR* Reason)
{
    const bool bHadTimer = IsRecoveryScheduled();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RecoveryTimer);
    }
    RecoveryTimer.Invalidate();
    if (bHadTimer || bRecovering)
    {
        UE_LOG(LogETStaminaRecovery, Display, TEXT("[M2-01B1] Timer stopped Owner=%s Reason=%s"),
               *GetNameSafe(GetOwner()), Reason);
    }
    bRecovering = false;
}

void UETStaminaRecoveryComponent::StopObserving()
{
    // 先作废上一轮回调，再释放Timer / 委托；重复清理安全。
    ++ObservationSerial;
    ClearRecoveryTimer(TEXT("Unsubscribe"));
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UETStaminaAttributeSet::GetStaminaAttribute())
            .Remove(StaminaHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(
               UETStaminaAttributeSet::GetMaxStaminaAttribute())
            .Remove(MaxStaminaHandle);
        const TConstArrayView<FGameplayTag> Tags = GetStaminaRecoveryBlockingTags();
        for (int32 Index = 0; Index < ActionTagHandles.Num(); ++Index)
        {
            ASC->RegisterGameplayTagEvent(Tags[Index], EGameplayTagEventType::NewOrRemoved)
                .Remove(ActionTagHandles[Index]);
        }
        UE_LOG(LogETStaminaRecovery, Display, TEXT("[M2-01B1] Observe stopped Owner=%s"),
               *GetNameSafe(GetOwner()));
    }
    StaminaHandle.Reset();
    MaxStaminaHandle.Reset();
    ActionTagHandles.Reset();
    BoundASC.Reset();
}

// 不缓存资源读数；每次都重新确认身体、ASC与ActorInfo是同一组身份。
bool UETStaminaRecoveryComponent::ReadBody(AEmberTrialCharacter*& Body,
                                           UAbilitySystemComponent*& ASC, float& Stamina,
                                           float& MaxStamina) const
{
    Body = BodyOwner.Get();
    ASC = BoundASC.Get();
    return !bEndingPlay && IsActive() && IsValid(Body) && !Body->IsActorBeingDestroyed() &&
           Body->HasAuthority() && Body->IsGameplayReady() && IsValid(ASC) &&
           Body->GetAbilitySystemComponent() == ASC && ASC->AbilityActorInfo.IsValid() &&
           ASC->AbilityActorInfo->OwnerActor.Get() == Body &&
           ASC->AbilityActorInfo->AvatarActor.Get() == Body &&
           Body->GetGameplayStaminaSnapshot(Stamina, MaxStamina) && FMath::IsFinite(Stamina) &&
           FMath::IsFinite(MaxStamina) && MaxStamina >= 1.0f && Stamina >= 0.0f &&
           Stamina <= MaxStamina;
}

void UETStaminaRecoveryComponent::RefreshObservation()
{
    StopObserving();
    AEmberTrialCharacter* Body = BodyOwner.Get();
    if (bEndingPlay || !IsActive() || !IsValid(Body) || !Body->HasAuthority() ||
        !Body->IsGameplayReady())
    {
        return;
    }
    TArray<UETStaminaRecoveryComponent*> Components;
    Body->GetComponents(Components);
    if (Components.Num() != 1 || !FMath::IsFinite(RecoveryRate) || RecoveryRate <= 0.0f ||
        RecoveryRate > 1000.0f || !FMath::IsFinite(RecoveryDelay) || RecoveryDelay < 0.0f ||
        RecoveryDelay > 10.0f || !FMath::IsFinite(RecoveryInterval) || RecoveryInterval < 0.05f ||
        RecoveryInterval > 0.5f)
    {
        UE_LOG(LogETStaminaRecovery, Warning,
               TEXT("[M2-01B1] Invalid recovery settings Owner=%s Count=%d Rate=%.2f Delay=%.2f "
                    "Interval=%.2f"),
               *GetNameSafe(Body), Components.Num(), RecoveryRate, RecoveryDelay, RecoveryInterval);
        return;
    }
    BoundASC = Body->GetAbilitySystemComponent();
    AEmberTrialCharacter* CheckedBody = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    if (!ReadBody(CheckedBody, ASC, Stamina, MaxStamina))
    {
        BoundASC.Reset();
        return;
    }
    StaminaHandle =
        ASC->GetGameplayAttributeValueChangeDelegate(UETStaminaAttributeSet::GetStaminaAttribute())
            .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC, ObservationSerial);
    MaxStaminaHandle =
        ASC->GetGameplayAttributeValueChangeDelegate(
               UETStaminaAttributeSet::GetMaxStaminaAttribute())
            .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC, ObservationSerial);
    const TConstArrayView<FGameplayTag> Tags = GetStaminaRecoveryBlockingTags();
    for (const FGameplayTag Tag : Tags)
    {
        ActionTagHandles.Add(
            ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
                .AddUObject(this, &ThisClass::HandleActionTagChanged, BoundASC, ObservationSerial));
    }
    LastSampleTime = GetWorld()->GetTimeSeconds();
    RecoveryNotBefore = LastSampleTime + RecoveryDelay;
    UE_LOG(LogETStaminaRecovery, Display,
           TEXT("[M2-01B1] Observe ready Owner=%s Rate=%.1f Delay=%.2f Interval=%.2f "
                "Stamina=%.1f/%.1f"),
           *GetNameSafe(Body), RecoveryRate, RecoveryDelay, RecoveryInterval, Stamina, MaxStamina);
    SyncRecoveryTimer();
}

// ---- 外部事件只更新截止时间 / Timer状态，不在回调中直接补体力 ----

void UETStaminaRecoveryComponent::HandleGameplayReadyChanged(AEmberTrialCharacter* Body,
                                                             bool bReady)
{
    if (bEndingPlay || Body != BodyOwner.Get())
    {
        return;
    }
    if (bReady)
    {
        RefreshObservation();
    }
    else
    {
        StopObserving();
    }
}

void UETStaminaRecoveryComponent::HandleAttributeChanged(
    const FOnAttributeChangeData& Change, TWeakObjectPtr<UAbilitySystemComponent> SourceASC,
    uint64 Serial)
{
    if (bEndingPlay || Serial != ObservationSerial || !SourceASC.IsValid() ||
        SourceASC.Get() != BoundASC.Get())
    {
        return;
    }
    if (Change.Attribute == UETStaminaAttributeSet::GetStaminaAttribute() &&
        Change.NewValue < Change.OldValue)
    {
        ++DecreaseRevision;
        LastSampleTime = GetWorld()->GetTimeSeconds();
        RecoveryNotBefore = LastSampleTime + RecoveryDelay;
        bRecovering = false;
        UE_LOG(LogETStaminaRecovery, Display,
               TEXT("[M2-01B1] Delayed Owner=%s Stamina=%.1f NotBefore=%.3f"),
               *GetNameSafe(GetOwner()), Change.NewValue, RecoveryNotBefore);
    }
    SyncRecoveryTimer();
}

void UETStaminaRecoveryComponent::HandleActionTagChanged(
    const FGameplayTag Tag, int32 NewCount, TWeakObjectPtr<UAbilitySystemComponent> SourceASC,
    uint64 Serial)
{
    (void)Tag;
    (void)NewCount;
    if (bEndingPlay || Serial != ObservationSerial || !SourceASC.IsValid() ||
        SourceASC.Get() != BoundASC.Get())
    {
        return;
    }
    LastSampleTime = GetWorld()->GetTimeSeconds();
    SyncRecoveryTimer();
}

// ---- 恢复资格与真实世界时间积分 ----

void UETStaminaRecoveryComponent::SyncRecoveryTimer()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    if (!ReadBody(Body, ASC, Stamina, MaxStamina))
    {
        ClearRecoveryTimer(TEXT("Unavailable"));
        return;
    }
    if (Body->IsCombatDead() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
    {
        ClearRecoveryTimer(TEXT("Dead"));
        return;
    }
    if (!Body->IsFormalActionIdle())
    {
        ClearRecoveryTimer(TEXT("Busy"));
        return;
    }
    if (Stamina >= MaxStamina)
    {
        ClearRecoveryTimer(TEXT("Full"));
        return;
    }
    if (!IsRecoveryScheduled())
    {
        LastSampleTime = GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(
            RecoveryTimer,
            FTimerDelegate::CreateUObject(this, &ThisClass::HandleRecoveryTimer, ObservationSerial),
            RecoveryInterval, true, RecoveryInterval);
    }
}

void UETStaminaRecoveryComponent::HandleRecoveryTimer(uint64 Serial)
{
    if (Serial != ObservationSerial || bEndingPlay)
    {
        return;
    }
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    if (!ReadBody(Body, ASC, Before, MaxStamina) || Body->IsCombatDead() ||
        !Body->IsFormalActionIdle() || Before >= MaxStamina)
    {
        SyncRecoveryTimer();
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    // 同时裁掉已结算时间和延迟；按经过的世界秒计算，不按回调次数加固定值。
    const double EligibleTime =
        FMath::Max(0.0, Now - FMath::Max(LastSampleTime, RecoveryNotBefore));
    const float Amount =
        FMath::Min(MaxStamina - Before, static_cast<float>(EligibleTime * RecoveryRate));
    // Keep accumulating time until the float resource can represent the increment.
    if (Amount <= 0.0f || Before + Amount <= Before)
    {
        return;
    }
    LastSampleTime = Now;
    FGameplayEffectSpecHandle Spec =
        ASC->MakeOutgoingSpec(UETStaminaDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
    if (!Spec.IsValid())
    {
        ClearRecoveryTimer(TEXT("MissingSpec"));
        return;
    }
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_StaminaDelta, Amount);
    if (!bRecovering)
    {
        bRecovering = true;
        UE_LOG(LogETStaminaRecovery, Display,
               TEXT("[M2-01B1] Recovery begin Owner=%s Time=%.3f Stamina=%.1f/%.1f"),
               *GetNameSafe(Body), Now, Before, MaxStamina);
    }
    // Apply可能同步消费、重绑或结束；旧积分结果不能覆盖新一轮计时。
    const uint64 Revision = DecreaseRevision;
    const TWeakObjectPtr<UAbilitySystemComponent> SourceASC = ASC;
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    // Applying a GE can synchronously trigger callbacks, shutdown, or a new spend.
    if (bEndingPlay || Serial != ObservationSerial || Revision != DecreaseRevision ||
        !SourceASC.IsValid() || SourceASC.Get() != BoundASC.Get())
    {
        return;
    }
    float After = 0.0f;
    if (!ReadBody(Body, ASC, After, MaxStamina))
    {
        return;
    }
    if (After <= Before)
    {
        ClearRecoveryTimer(TEXT("NoProgress"));
        return;
    }
    SyncRecoveryTimer();
}
