#include "Combat/ETCombatPoiseComponent.h"
#include "Abilities/ETGA_Stagger.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCombatPoise, Log, All);

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
        !Body->GetGameplayHealthSnapshot(Health, MaxHealth) || Health <= 0.0f)
    {
        return false;
    }
    TInlineComponentArray<UETCombatPoiseComponent*> Components;
    Body->GetComponents(Components);
    return Components.Num() == 1 && Components[0] == this;
}

void UETCombatPoiseComponent::StartObserving()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    // 无资源配置的原生测试身体可保留属性；正式资产由Validator要求配置。
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
    StaggerHandle =
        ASC->GiveAbility(FGameplayAbilitySpec(StaggerAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETCombatPoise, Display, TEXT("[M2-05B1] Reaction ready Body=%s Granted=%d"),
           *GetNameSafe(Body), StaggerHandle.IsValid() ? 1 : 0);
}

bool UETCombatPoiseComponent::RequestStagger()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    bool bAccepted = false;
    if (HasValidConfiguration() && ReadBody(Body, ASC) && BoundASC.Get() == ASC &&
        StaggerHandle.IsValid() && !UGameplayStatics::IsGamePaused(Body))
    {
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(StaggerHandle);
        if (Spec && Spec->Ability && Spec->Ability->GetClass() == StaggerAbilityClass.Get())
        {
            bAccepted = ASC->TryActivateAbility(StaggerHandle);
        }
    }
    UE_LOG(LogETCombatPoise, Display, TEXT("[M2-05B1] Stagger request Body=%s Accepted=%d"),
           *GetNameSafe(BodyOwner.Get()), bAccepted ? 1 : 0);
    return bAccepted;
}

void UETCombatPoiseComponent::StopObserving()
{
    UAbilitySystemComponent* ASC = BoundASC.Get();
    const FGameplayAbilitySpecHandle OldHandle = StaggerHandle;
    BoundASC.Reset();
    StaggerHandle = FGameplayAbilitySpecHandle();
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