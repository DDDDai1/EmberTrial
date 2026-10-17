#include "Resources/ETHealingResourceComponent.h"
#include "Abilities/Effects/ETHealingEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Templates/UnrealTemplate.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHealingResource, Log, All);

UETHealingResourceComponent::UETHealingResourceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

bool UETHealingResourceComponent::HasValidConfiguration() const
{
    return bAutoActivate && MaxCharges >= 1 && MaxCharges <= 9 &&
        InitialCharges >= 0 && InitialCharges <= MaxCharges;
}

void UETHealingResourceComponent::BeginPlay()
{
    Super::BeginPlay();
    BodyOwner = Cast<AEmberTrialCharacter>(GetOwner());
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.AddUniqueDynamic(
            this, &ThisClass::HandleGameplayReadyChanged);
    }
    RefreshAvailability();
}

void UETHealingResourceComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    if (HasBegunPlay() && !bEndingPlay) { RefreshAvailability(); }
}

void UETHealingResourceComponent::Deactivate()
{
    Super::Deactivate();
    RefreshAvailability();
}

bool UETHealingResourceComponent::ReadBody(AEmberTrialCharacter*& OutBody,
    UAbilitySystemComponent*& OutASC) const
{
    OutBody = BodyOwner.Get();
    OutASC = IsValid(OutBody) ? OutBody->GetAbilitySystemComponent() : nullptr;
    return !bEndingPlay && HasBegunPlay() && IsActive() && IsValid(OutBody) &&
        !OutBody->IsActorBeingDestroyed() && OutBody->HasAuthority() &&
        OutBody->GetCombatTeam() == EETCombatTeam::Player && OutBody->IsGameplayReady() &&
        IsValid(OutASC) && OutASC->AbilityActorInfo.IsValid() &&
        OutASC->AbilityActorInfo->OwnerActor.Get() == OutBody &&
        OutASC->AbilityActorInfo->AvatarActor.Get() == OutBody;
}

void UETHealingResourceComponent::RefreshAvailability()
{
    const bool bWasAvailable = bAvailable;
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    bAvailable = ReadBody(Body, ASC);
    if (bAvailable)
    {
        TArray<UETHealingResourceComponent*> Components;
        Body->GetComponents(Components);
        bAvailable = Components.Num() == 1 && HasValidConfiguration();
        if (!bAvailable)
        {
            UE_LOG(LogETHealingResource, Warning,
                TEXT("[M2-04A] Invalid healing settings Owner=%s Count=%d Initial=%d Max=%d"),
                *GetNameSafe(Body), Components.Num(), InitialCharges, MaxCharges);
        }
    }
    if (bAvailable && !bInitialized)
    {
        CurrentCharges = InitialCharges;
        bInitialized = true;
        UE_LOG(LogETHealingResource, Display,
            TEXT("[M2-04A] Healing initialized Owner=%s Charges=%d/%d"),
            *GetNameSafe(Body), CurrentCharges, MaxCharges);
    }
    if (bWasAvailable != bAvailable) { OnSnapshotChanged.Broadcast(); }
}

void UETHealingResourceComponent::HandleGameplayReadyChanged(
    AEmberTrialCharacter* Body, bool bReady)
{
    (void)bReady;
    if (!bEndingPlay && Body == BodyOwner.Get()) { RefreshAvailability(); }
}

bool UETHealingResourceComponent::GetHealingSnapshot(
    int32& OutCharges, int32& OutMaxCharges) const
{
    OutCharges = 0;
    OutMaxCharges = 0;
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!bInitialized || !bAvailable || !ReadBody(Body, ASC)) { return false; }
    OutCharges = CurrentCharges;
    OutMaxCharges = MaxCharges;
    return true;
}

bool UETHealingResourceComponent::CanApplyHealing(float Amount) const
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    return !bApplying && bInitialized && bAvailable && CurrentCharges > 0 &&
        FMath::IsFinite(Amount) && Amount >= 1.0f && Amount <= 1000.0f &&
        ReadBody(Body, ASC) && GetWorld() && !GetWorld()->IsPaused() &&
        !Body->IsCombatDead() && !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead) &&
        Body->GetGameplayHealthSnapshot(Health, MaxHealth) &&
        FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) &&
        Health > 0.0f && Health < MaxHealth && Health + Amount > Health;
}

bool UETHealingResourceComponent::TryApplyHealing(float Amount)
{
    if (!CanApplyHealing(Amount)) { return false; }
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadBody(Body, ASC)) { return false; }
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
        UETHealingEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
    if (!Spec.IsValid()) { return false; }
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_HealthDelta, Amount);

    // GE callbacks and the count notification may synchronously request another heal.
    TGuardValue<bool> ApplyingGuard(bApplying, true);
    const TWeakObjectPtr<AEmberTrialCharacter> SourceBody = Body;
    const FActiveGameplayEffectHandle Result = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    if (!Result.WasSuccessfullyApplied()) { return false; }

    // Successful Instant effects need not have a valid active handle. Do not refund
    // because a later callback cancelled an action or deactivated this component.
    if (!bEndingPlay && SourceBody.IsValid() && !SourceBody->IsActorBeingDestroyed())
    {
        --CurrentCharges;
        UE_LOG(LogETHealingResource, Display,
            TEXT("[M2-04A] Healing applied Owner=%s Amount=%.1f Charges=%d/%d"),
            *GetNameSafe(SourceBody.Get()), Amount, CurrentCharges, MaxCharges);
        if (bAvailable) { OnSnapshotChanged.Broadcast(); }
    }
    return true;
}

void UETHealingResourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(
            this, &ThisClass::HandleGameplayReadyChanged);
    }
    const bool bWasAvailable = bAvailable;
    bAvailable = false;
    BodyOwner.Reset();
    if (bWasAvailable) { OnSnapshotChanged.Broadcast(); }
    OnSnapshotChanged.Clear();
    Super::EndPlay(EndPlayReason);
}