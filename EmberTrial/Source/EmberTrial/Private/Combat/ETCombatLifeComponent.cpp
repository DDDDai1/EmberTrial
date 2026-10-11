#include "Combat/ETCombatLifeComponent.h"
#include "Abilities/ETGA_HitReact.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Kismet/GameplayStatics.h"
#include "Abilities/ETGameplayTags.h"
#include "Animation/AnimInstance.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../../ETPracticeActionComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCombatLife, Log, All);

UETCombatLifeComponent::UETCombatLifeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UETCombatLifeComponent::BeginPlay()
{
    Super::BeginPlay();
    bEndingPlay = false;
    CharacterOwner = Cast<AEmberTrialCharacter>(GetOwner());
    if (AEmberTrialCharacter* Character = CharacterOwner.Get())
    {
        Character->OnGameplayReadyChanged.AddUniqueDynamic(
            this, &ThisClass::HandleReadyChanged);
        StartObserving();
    }
}

void UETCombatLifeComponent::HandleReadyChanged(
    AEmberTrialCharacter* Character, bool bReady)
{
    if (Character != CharacterOwner.Get() || bEndingPlay)
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

void UETCombatLifeComponent::StartObserving()
{
    AEmberTrialCharacter* Character = CharacterOwner.Get();
    if (bEndingPlay || !IsValid(Character) || !Character->IsGameplayReady())
    {
        return;
    }
    UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
    if (!IsValid(ASC) || !ASC->AbilityActorInfo.IsValid() ||
        ASC->AbilityActorInfo->OwnerActor.Get() != Character ||
        ASC->AbilityActorInfo->AvatarActor.Get() != Character ||
        !ASC->HasAttributeSetForAttribute(UETHealthAttributeSet::GetHealthAttribute()))
    {
        return;
    }
    if (BoundASC.Get() == ASC && HealthHandle.IsValid())
    {
        return;
    }
    StopObserving();
    BoundASC = ASC;
    HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(
        UETHealthAttributeSet::GetHealthAttribute()).AddUObject(
            this, &ThisClass::HandleHealthChanged);

    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (Character->GetGameplayHealthSnapshot(Health, MaxHealth) &&
        FMath::IsFinite(Health) && Health <= 0.0f)
    {
        CommitDeath(TEXT("ReadySnapshot"));
    }
    Character = CharacterOwner.Get();
    ASC = BoundASC.Get();
    if (bEndingPlay || !IsValid(Character) || !IsValid(ASC) ||
        !Character->IsGameplayReady())
    {
        return;
    }
    if (!bDead && Character->HasAuthority() && HitReactAbilityClass &&
        !HitReactAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        const FGameplayAbilitySpec Spec(HitReactAbilityClass.Get(), 1, INDEX_NONE, this);
        HitReactHandle = ASC->GiveAbility(Spec);
    }
    UE_LOG(LogETCombatLife, Display,
        TEXT("[M1-04C1] Life observing Character=%s ASC=%s HitClass=%s Granted=%d"),
        *GetNameSafe(Character), *GetNameSafe(ASC),
        *GetNameSafe(HitReactAbilityClass.Get()), HitReactHandle.IsValid() ? 1 : 0);
}

void UETCombatLifeComponent::StopObserving()
{
    UAbilitySystemComponent* ASC = BoundASC.Get();
    const FDelegateHandle OldHealthHandle = HealthHandle;
    const FGameplayAbilitySpecHandle OldHitHandle = HitReactHandle;
    BoundASC.Reset();
    HealthHandle.Reset();
    HitReactHandle = FGameplayAbilitySpecHandle();

    if (IsValid(ASC))
    {
        ASC->GetGameplayAttributeValueChangeDelegate(
            UETHealthAttributeSet::GetHealthAttribute()).Remove(OldHealthHandle);
        if (OldHitHandle.IsValid())
        {
            ASC->CancelAbilityHandle(OldHitHandle);
            ASC->ClearAbility(OldHitHandle);
        }
        UE_LOG(LogETCombatLife, Display,
            TEXT("[M1-04C1] Life stopped Character=%s"), *GetNameSafe(GetOwner()));
    }
}

bool UETCombatLifeComponent::RequestHitReact()
{
    return RequestHitReactInternal(TEXT("DiagnosticInput"));
}

bool UETCombatLifeComponent::RequestHitReactInternal(const TCHAR* Reason)
{
    AEmberTrialCharacter* Character = CharacterOwner.Get();
    UAbilitySystemComponent* ASC = BoundASC.Get();
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (bEndingPlay || bDead || !IsValid(Character) || !IsValid(ASC) ||
        Character->GetAbilitySystemComponent() != ASC ||
        !Character->GetGameplayHealthSnapshot(Health, MaxHealth) ||
        !FMath::IsFinite(Health) || Health <= 0.0f ||
        UGameplayStatics::IsGamePaused(Character) || !HitReactHandle.IsValid())
    {
        return false;
    }
    const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(HitReactHandle);
    if (!Spec || Spec->IsActive())
    {
        UE_LOG(LogETCombatLife, Display,
            TEXT("[M1-04C1] Hit request Character=%s Reason=%s Accepted=0 BusyOrMissing=1"),
            *GetNameSafe(Character), Reason);
        return false;
    }
    const FString CharacterName = GetNameSafe(Character);
    const bool bAccepted = ASC->TryActivateAbility(HitReactHandle, false);
    UE_LOG(LogETCombatLife, Display,
        TEXT("[M1-04C1] Hit request Character=%s Reason=%s Accepted=%d"),
        *CharacterName, Reason, bAccepted ? 1 : 0);
    return bAccepted;
}

void UETCombatLifeComponent::HandleHealthChanged(const FOnAttributeChangeData& Change)
{
    AEmberTrialCharacter* Character = CharacterOwner.Get();
    UAbilitySystemComponent* ASC = BoundASC.Get();
    if (bEndingPlay || bDead || !IsValid(Character) ||
        !Character->IsGameplayReady() || !IsValid(ASC) ||
        Character->GetAbilitySystemComponent() != ASC ||
        Change.Attribute != UETHealthAttributeSet::GetHealthAttribute() ||
        !FMath::IsFinite(Change.NewValue))
    {
        return;
    }
    if (Change.NewValue <= 0.0f)
    {
        CommitDeath(TEXT("HealthZero"));
        return;
    }
    if (!FMath::IsFinite(Change.OldValue) ||
        Change.OldValue <= Change.NewValue || !Change.GEModData)
    {
        return;
    }
    const FGameplayEffectModCallbackData& Mod = *Change.GEModData;
    if (&Mod.Target != ASC ||
        Mod.EvaluatedData.Attribute != UETHealthAttributeSet::GetHealthAttribute() ||
        Mod.EvaluatedData.ModifierOp != EGameplayModOp::AddBase ||
        !FMath::IsFinite(Mod.EvaluatedData.Magnitude) || Mod.EvaluatedData.Magnitude >= 0.0f)
    {
        return;
    }
    UE_LOG(LogETCombatLife, Display,
        TEXT("[M1-04C1] Health damage Character=%s Before=%.1f After=%.1f Actual=%.1f"),
        *GetNameSafe(Character), Change.OldValue, Change.NewValue,
        Change.OldValue - Change.NewValue);
    RequestHitReactInternal(TEXT("HealthLoss"));
}

void UETCombatLifeComponent::CommitDeath(const TCHAR* Reason)
{
    AEmberTrialCharacter* Character = CharacterOwner.Get();
    UAbilitySystemComponent* ASC = BoundASC.Get();
    if (bEndingPlay || bDead || !IsValid(Character) ||
        !Character->HasAuthority() || !Character->IsGameplayReady() ||
        !IsValid(ASC) || Character->GetAbilitySystemComponent() != ASC)
    {
        return;
    }
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (!Character->GetGameplayHealthSnapshot(Health, MaxHealth) ||
        !FMath::IsFinite(Health) || Health > 0.0f)
    {
        return;
    }

    bDead = true;
    DeadTagASC = ASC;
    bOwnsDeadTag = true;
    ASC->AddLooseGameplayTag(ETGameplayTags::State_Dead);
    Character = CharacterOwner.Get();
    ASC = DeadTagASC.Get();
    if (bEndingPlay || !IsValid(Character) || !IsValid(ASC) ||
        !Character->IsGameplayReady())
    {
        return;
    }
    const FString CharacterName = GetNameSafe(Character);
    UE_LOG(LogETCombatLife, Display,
        TEXT("[M1-04C2] Death committed Character=%s Reason=%s Health=%.1f DeadTag=%d"),
        *CharacterName, Reason, Health,
        ASC->GetTagCount(ETGameplayTags::State_Dead));

    UETCombatTraceComponent* Trace =
        Character->FindComponentByClass<UETCombatTraceComponent>();
    if (IsValid(Trace))
    {
        Trace->CancelActiveAttack();
    }
    ASC->CancelAllAbilities();
    // Ability cancellation can invoke other callbacks; reacquire the weak owner.
    Character = CharacterOwner.Get();
    if (bEndingPlay || !IsValid(Character) || !Character->IsGameplayReady())
    {
        return;
    }
    UETPracticeActionComponent* Practice =
        Character->FindComponentByClass<UETPracticeActionComponent>();
    if (IsValid(Practice))
    {
        Practice->CancelPracticeAction();
    }
    Character = CharacterOwner.Get();
    if (bEndingPlay || !IsValid(Character) || !Character->IsGameplayReady())
    {
        return;
    }
    Character->StopJumping();
    Character->ConsumeMovementInputVector();
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (IsValid(Movement))
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }
    USkeletalMeshComponent* Mesh = Character->GetMesh();
    if (IsValid(Mesh))
    {
        if (UAnimInstance* Anim = Mesh->GetAnimInstance())
        {
            Anim->StopAllMontages(0.0f);
        }
        if (bEndingPlay || !CharacterOwner.IsValid())
        {
            return;
        }
        Mesh = CharacterOwner->GetMesh();
        if (IsValid(Mesh))
        {
            Mesh->bPauseAnims = true;
        }
    }
    Character = CharacterOwner.Get();
    if (bEndingPlay || !IsValid(Character))
    {
        return;
    }
    Trace = Character->FindComponentByClass<UETCombatTraceComponent>();
    Practice = Character->FindComponentByClass<UETPracticeActionComponent>();
    Movement = Character->GetCharacterMovement();
    Mesh = Character->GetMesh();
    UE_LOG(LogETCombatLife, Display,
        TEXT("[M1-04C2] Death sealed Character=%s TraceTick=%d PracticeIdle=%d MoveMode=%d AnimPaused=%d"),
        *CharacterName,
        IsValid(Trace) && Trace->IsComponentTickEnabled() ? 1 : 0,
        !IsValid(Practice) || Practice->IsPracticeActionIdle() ? 1 : 0,
        IsValid(Movement) ? static_cast<int32>(Movement->MovementMode) : -1,
        IsValid(Mesh) && Mesh->bPauseAnims ? 1 : 0);

    OnDeathSealed.Broadcast(this);
}

void UETCombatLifeComponent::ClearOwnedDeadTag()
{
    UAbilitySystemComponent* ASC = DeadTagASC.Get();
    const bool bRemoveOwnCount = bOwnsDeadTag;
    bOwnsDeadTag = false;
    DeadTagASC.Reset();
    if (bRemoveOwnCount && IsValid(ASC))
    {
        if (ASC->GetTagCount(ETGameplayTags::State_Dead) > 0)
        {
            ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Dead);
        }
        UE_LOG(LogETCombatLife, Display,
            TEXT("[M1-04C2] Death tag released Character=%s Remaining=%d"),
            *GetNameSafe(GetOwner()), ASC->GetTagCount(ETGameplayTags::State_Dead));
    }
}

void UETCombatLifeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    OnDeathSealed.Clear();
    if (AEmberTrialCharacter* Character = CharacterOwner.Get())
    {
        Character->OnGameplayReadyChanged.RemoveDynamic(
            this, &ThisClass::HandleReadyChanged);
    }
    StopObserving();
    ClearOwnedDeadTag();
    CharacterOwner.Reset();
    Super::EndPlay(EndPlayReason);
}