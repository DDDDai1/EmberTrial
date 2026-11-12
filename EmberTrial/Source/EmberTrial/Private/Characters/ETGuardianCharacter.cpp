#include "Characters/ETGuardianCharacter.h"

#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#define LOCTEXT_NAMESPACE "ETGuardianCharacter"

DEFINE_LOG_CATEGORY_STATIC(LogETGuardian, Log, All);

AETGuardianCharacter::AETGuardianCharacter()
{
    GetCharacterMovement()->MaxWalkSpeed = 220.0f;

    PhaseMarker = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PhaseMarker"));
    PhaseMarker->SetupAttachment(GetRootComponent());
    PhaseMarker->SetRelativeLocation(FVector(0.0f, 0.0f, 135.0f));
    PhaseMarker->SetUsingAbsoluteRotation(true);
    PhaseMarker->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    PhaseMarker->SetHorizontalAlignment(EHTA_Center);
    PhaseMarker->SetWorldSize(23.0f);
    PhaseMarker->SetText(LOCTEXT("PhaseOne", "GUARDIAN I"));
}

void AETGuardianCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (IsCombatDead())
    {
        PhaseMarker->SetVisibility(false);
        return;
    }

    if (UETCombatLifeComponent* Life = FindComponentByClass<UETCombatLifeComponent>())
    {
        DeathHandle = Life->OnDeathSealed.AddUObject(this, &ThisClass::HandleDeath);
    }
    OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleReadyChanged);
    HandleReadyChanged(this, IsGameplayReady());
}

void AETGuardianCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleReadyChanged);
    if (UETCombatLifeComponent* Life = FindComponentByClass<UETCombatLifeComponent>())
    {
        Life->OnDeathSealed.Remove(DeathHandle);
    }
    StopObservingPhase();
    Super::EndPlay(EndPlayReason);
}

void AETGuardianCharacter::HandleReadyChanged(AEmberTrialCharacter*, bool bReady)
{
    if (!bReady)
    {
        StopObservingPhase();
        return;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    if (bPhaseTwo || IsCombatDead() || HealthHandle.IsValid() || !IsValid(ASC))
    {
        return;
    }

    HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetHealthAttribute())
        .AddUObject(this, &ThisClass::HandleHealthChanged);
    MaxHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetMaxHealthAttribute())
        .AddUObject(this, &ThisClass::HandleHealthChanged);
    TagHandle = ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &ThisClass::HandleTagChanged);
    QueuePhaseCheck();
}

void AETGuardianCharacter::StopObservingPhase()
{
    GetWorldTimerManager().ClearTimer(PhaseCheckTimer);
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthHandle);
        ASC->RegisterGenericGameplayTagEvent().Remove(TagHandle);
    }

    HealthHandle.Reset();
    MaxHealthHandle.Reset();
    TagHandle.Reset();
}

void AETGuardianCharacter::HandleHealthChanged(const FOnAttributeChangeData&)
{
    QueuePhaseCheck();
}

void AETGuardianCharacter::HandleTagChanged(FGameplayTag, int32 Count)
{
    if (Count == 0)
    {
        QueuePhaseCheck();
    }
}

void AETGuardianCharacter::HandleDeath(UETCombatLifeComponent*)
{
    PhaseMarker->SetVisibility(false);
    StopObservingPhase();
}

void AETGuardianCharacter::QueuePhaseCheck()
{
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (!bPhaseTwo && !PhaseCheckTimer.IsValid() && IsBelowThreshold(Health, MaxHealth))
    {
        // Attribute and tag delegates can run before hit reaction and death finish updating.
        PhaseCheckTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::EvaluatePhase);
    }
}

void AETGuardianCharacter::EvaluatePhase()
{
    PhaseCheckTimer.Invalidate();

    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (!IsBelowThreshold(Health, MaxHealth) || !IsFormalActionIdle())
    {
        return;
    }

    bPhaseTwo = true;
    GetCharacterMovement()->MaxWalkSpeed = PhaseTwoWalkSpeed;
    PhaseMarker->SetText(LOCTEXT("PhaseTwo", "GUARDIAN II"));
    PhaseMarker->SetTextRenderColor(FColor(255, 145, 56));
    StopObservingPhase();

    UE_LOG(LogETGuardian, Display, TEXT("Guardian phase entered Body=%s Phase=2 Health=%.1f/%.1f WalkSpeed=%.1f"),
           *GetNameSafe(this), Health, MaxHealth, PhaseTwoWalkSpeed);
}

bool AETGuardianCharacter::IsBelowThreshold(float& Health, float& MaxHealth) const
{
    return !IsCombatDead() && GetGameplayHealthSnapshot(Health, MaxHealth) &&
           FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) && Health > 0.0f && MaxHealth > 0.0f &&
           FMath::IsFinite(PhaseTwoHealthFraction) && PhaseTwoHealthFraction > 0.0f && PhaseTwoHealthFraction < 1.0f &&
           FMath::IsFinite(PhaseTwoWalkSpeed) && PhaseTwoWalkSpeed > 0.0f &&
           Health <= MaxHealth * PhaseTwoHealthFraction;
}

#undef LOCTEXT_NAMESPACE
