#include "Combat/ETCombatPoiseComponent.h"
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Effects/ETPoiseDeltaEffect.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCombatPoise, Log, All);

// ---- 生命周期与正式身体身份 ----
UETCombatPoiseComponent::UETCombatPoiseComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UETCombatPoiseComponent::BeginPlay()
{
    Super::BeginPlay();
    bEndingPlay = false;
    BodyOwner = Cast<AEmberTrialCharacter>(GetOwner());
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleReadyChanged);
    }
    StartObserving();
}

void UETCombatPoiseComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    if (HasBegunPlay())
    {
        StartObserving();
    }
}

void UETCombatPoiseComponent::Deactivate()
{
    Super::Deactivate();
    StopObserving();
}

bool UETCombatPoiseComponent::HasValidConfiguration() const
{
    return StaggerAbilityClass && !StaggerAbilityClass->HasAnyClassFlags(CLASS_Abstract) &&
           StaggerAbilityClass->GetDefaultObject<UETGA_Stagger>()->HasValidConfiguration();
}

bool UETCombatPoiseComponent::ReadBody(AEmberTrialCharacter*& Body,
                                       UAbilitySystemComponent*& ASC) const
{
    Body = BodyOwner.Get();
    ASC = IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (bEndingPlay || !HasBegunPlay() || !IsActive() || !IsValid(Body) ||
        Body->IsActorBeingDestroyed() || !Body->HasAuthority() || Body->IsCombatDead() ||
        !IsValid(ASC) || !ASC->AbilityActorInfo.IsValid() ||
        ASC->AbilityActorInfo->OwnerActor.Get() != Body ||
        ASC->AbilityActorInfo->AvatarActor.Get() != Body ||
        !Body->GetGameplayHealthSnapshot(Health, MaxHealth) || Health <= 0.0f ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
    {
        return false;
    }
    TInlineComponentArray<UETCombatPoiseComponent*> Components;
    Body->GetComponents(Components);
    return Components.Num() == 1 && Components[0] == this;
}

bool UETCombatPoiseComponent::IsCurrent(UAbilitySystemComponent* Source, uint64 Serial) const
{
    return !bEndingPlay && IsActive() && Serial == ObservationSerial && IsValid(Source) &&
           Source == BoundASC.Get();
}

bool UETCombatPoiseComponent::IsOwnReactionActive() const
{
    const UAbilitySystemComponent* ASC = BoundASC.Get();
    const FGameplayAbilitySpec* Spec =
        IsValid(ASC) ? ASC->FindAbilitySpecFromHandle(StaggerHandle) : nullptr;
    return Spec && Spec->IsActive();
}

// ---- 订阅后快照：既处理后续扣减，也处理出生0和重新激活时已有的0 ----
void UETCombatPoiseComponent::StartObserving()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!HasValidConfiguration() || !ReadBody(Body, ASC))
    {
        return;
    }
    if (BoundASC.Get() == ASC && StaggerHandle.IsValid())
    {
        return;
    }
    StopObserving();
    BoundASC = ASC;
    const uint64 Serial = ObservationSerial;
    const TWeakObjectPtr<UAbilitySystemComponent> Source = ASC;
    StaggerHandle =
        ASC->GiveAbility(FGameplayAbilitySpec(StaggerAbilityClass.Get(), 1, INDEX_NONE, this));
    if (!IsCurrent(ASC, Serial) || !StaggerHandle.IsValid())
    {
        return;
    }
    PoiseChangedHandle =
        ASC->GetGameplayAttributeValueChangeDelegate(UETPoiseAttributeSet::GetPoiseAttribute())
            .AddUObject(this, &ThisClass::HandlePoiseChanged, Source, Serial);
    StunnedChangedHandle = ASC->RegisterGameplayTagEvent(ETGameplayTags::State_Stunned,
                                                         EGameplayTagEventType::NewOrRemoved)
                               .AddUObject(this, &ThisClass::HandleStunnedChanged, Source, Serial);
    AbilityEndedHandle =
        ASC->OnAbilityEnded.AddUObject(this, &ThisClass::HandleAbilityEnded, Source, Serial);
    UE_LOG(LogETCombatPoise, Display, TEXT("[M2-05B2] Reaction ready Body=%s Granted=1"),
           *GetNameSafe(Body));
    QueueEvaluation();
}

bool UETCombatPoiseComponent::RequestStagger()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    bool bAccepted = false;
    if (HasValidConfiguration() && ReadBody(Body, ASC) && BoundASC.Get() == ASC &&
        StaggerHandle.IsValid() && !UGameplayStatics::IsGamePaused(Body))
    {
        const uint64 Serial = ObservationSerial;
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(StaggerHandle);
        if (Spec && Spec->Ability && Spec->Ability->GetClass() == StaggerAbilityClass.Get())
        {
            bAccepted = ASC->TryActivateAbility(StaggerHandle) && IsCurrent(ASC, Serial) &&
                        IsOwnReactionActive();
        }
    }
    UE_LOG(LogETCombatPoise, Display, TEXT("[M2-05B2] Stagger request Body=%s Accepted=%d"),
           *GetNameSafe(BodyOwner.Get()), bAccepted ? 1 : 0);
    return bAccepted;
}

void UETCombatPoiseComponent::HandlePoiseChanged(const FOnAttributeChangeData& Data,
                                                 TWeakObjectPtr<UAbilitySystemComponent> Source,
                                                 uint64 Serial)
{
    (void)Data;
    if (!IsCurrent(Source.Get(), Serial))
    {
        return;
    }
    // 用即时快照而非事件内旧值，避免其他观察者同步改值后误判。
    float Poise = 0.0f;
    float MaxPoise = 0.0f;
    if (AEmberTrialCharacter* Body = BodyOwner.Get();
        Body && Body->GetGameplayPoiseSnapshot(Poise, MaxPoise) && Poise > 0.0f)
    {
        bZeroAttempted = false;
    }
    QueueEvaluation();
}

void UETCombatPoiseComponent::HandleStunnedChanged(FGameplayTag Tag, int32 Count,
                                                   TWeakObjectPtr<UAbilitySystemComponent> Source,
                                                   uint64 Serial)
{
    (void)Tag;
    if (!IsCurrent(Source.Get(), Serial) || Count != 0 || IsOwnReactionActive())
    {
        return;
    }
    // 外部Stunned撤销后允许一次补判；自身撤销时Spec仍Active，由Ended负责恢复。
    bZeroAttempted = false;
    QueueEvaluation();
}

void UETCombatPoiseComponent::HandleAbilityEnded(const FAbilityEndedData& Data,
                                                 TWeakObjectPtr<UAbilitySystemComponent> Source,
                                                 uint64 Serial)
{
    if (!IsCurrent(Source.Get(), Serial) || Data.AbilitySpecHandle != StaggerHandle)
    {
        return;
    }
    const UETGA_Stagger* Reaction = Cast<UETGA_Stagger>(Data.AbilityThatEnded.Get());
    if (!Reaction || !Reaction->WasReactionStarted())
    {
        return;
    }
    // 不在GAS的结束调用栈内写GE；自然结束 / 存活取消均只恢复一次。
    bRestorePending = true;
    QueueEvaluation();
}

// ---- 一次性延后派发，避开GE / Ability ScopeLock，无常驻Tick或失败轮询 ----
void UETCombatPoiseComponent::QueueEvaluation()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Poise = 0.0f;
    float MaxPoise = 0.0f;
    if (!ReadBody(Body, ASC) || ASC != BoundASC.Get() || EvaluationTimer.IsValid() ||
        !Body->GetGameplayPoiseSnapshot(Poise, MaxPoise) ||
        (!bRestorePending && (Poise > 0.0f || bZeroAttempted || IsOwnReactionActive())))
    {
        return;
    }
    if (UWorld* World = Body->GetWorld())
    {
        EvaluationWorld = World;
        EvaluationTimer = World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &ThisClass::Evaluate, ObservationSerial));
    }
}

void UETCombatPoiseComponent::Evaluate(uint64 Serial)
{
    if (Serial != ObservationSerial)
    {
        return;
    }
    EvaluationTimer.Invalidate();
    EvaluationWorld.Reset();
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadBody(Body, ASC) || ASC != BoundASC.Get())
    {
        return;
    }
    if (bRestorePending)
    {
        bRestorePending = false;
        RestorePoise(Body, ASC);
        return;
    }
    float Poise = 0.0f;
    float MaxPoise = 0.0f;
    if (!Body->GetGameplayPoiseSnapshot(Poise, MaxPoise) || Poise > 0.0f || bZeroAttempted ||
        IsOwnReactionActive())
    {
        return;
    }
    bZeroAttempted = true;
    const bool bAccepted = RequestStagger();
    UE_LOG(LogETCombatPoise, Display, TEXT("[M2-05B2] Depleted Body=%s Accepted=%d"),
           *GetNameSafe(Body), bAccepted ? 1 : 0);
}

void UETCombatPoiseComponent::RestorePoise(AEmberTrialCharacter* Body, UAbilitySystemComponent* ASC)
{
    float Poise = 0.0f;
    float MaxPoise = 0.0f;
    if (IsOwnReactionActive() || !Body->GetGameplayPoiseSnapshot(Poise, MaxPoise) ||
        Poise >= MaxPoise)
    {
        return;
    }
    const uint64 Serial = ObservationSerial;
    const FString BodyName = Body->GetName();
    FGameplayEffectSpecHandle Spec =
        ASC->MakeOutgoingSpec(UETPoiseDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
    if (!Spec.IsValid())
    {
        return;
    }
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_PoiseDelta, MaxPoise - Poise);
    const bool bApplied =
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).WasSuccessfullyApplied();
    // GE事件可能同步停观察 / 销毁身体；之后只记录本轮局部值。
    UE_LOG(LogETCombatPoise, Display,
           TEXT("[M2-05B2] Restore Body=%s Applied=%d Max=%.1f CurrentSource=%d"), *BodyName,
           bApplied ? 1 : 0, MaxPoise, IsCurrent(ASC, Serial) ? 1 : 0);
}

// ---- 先撤销身份与订阅，再取消能力；结束不回填、不留下旧世界Timer ----
void UETCombatPoiseComponent::StopObserving()
{
    ++ObservationSerial;
    UAbilitySystemComponent* ASC = BoundASC.Get();
    const FGameplayAbilitySpecHandle OldHandle = StaggerHandle;
    BoundASC.Reset();
    StaggerHandle = FGameplayAbilitySpecHandle();
    bRestorePending = false;
    bZeroAttempted = false;
    if (UWorld* World = EvaluationWorld.Get())
    {
        World->GetTimerManager().ClearTimer(EvaluationTimer);
    }
    EvaluationTimer.Invalidate();
    EvaluationWorld.Reset();
    if (IsValid(ASC))
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UETPoiseAttributeSet::GetPoiseAttribute())
            .Remove(PoiseChangedHandle);
        ASC->RegisterGameplayTagEvent(ETGameplayTags::State_Stunned,
                                      EGameplayTagEventType::NewOrRemoved)
            .Remove(StunnedChangedHandle);
        ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
    }
    PoiseChangedHandle.Reset();
    StunnedChangedHandle.Reset();
    AbilityEndedHandle.Reset();
    if (IsValid(ASC) && OldHandle.IsValid())
    {
        ASC->CancelAbilityHandle(OldHandle);
        ASC->ClearAbility(OldHandle);
    }
}

void UETCombatPoiseComponent::HandleReadyChanged(AEmberTrialCharacter* Body, bool bReady)
{
    if (Body != BodyOwner.Get() || bEndingPlay)
    {
        return;
    }
    if (bReady)
    {
        StartObserving();
    }
    else
    {
        StopObserving();
    }
}

void UETCombatPoiseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleReadyChanged);
    }
    StopObserving();
    BodyOwner.Reset();
    Super::EndPlay(EndPlayReason);
}