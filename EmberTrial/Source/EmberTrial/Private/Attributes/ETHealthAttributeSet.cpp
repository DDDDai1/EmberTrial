
#include "Attributes/ETHealthAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/ETGameplayTags.h"

UETHealthAttributeSet::UETHealthAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitMaxHealth(100.0f);
    InitHealth(0.0f);
}

void UETHealthAttributeSet::ClampAttribute(
    const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetMaxHealthAttribute())
    {
        NewValue = FMath::IsFinite(NewValue)
            ? FMath::Max(1.0f, NewValue)
            : FMath::Max(1.0f, GetMaxHealth());
    }
    else if (Attribute == GetHealthAttribute())
    {
        const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
        if (IsValid(ASC) &&
            ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
        {
            NewValue = 0.0f;
            return;
        }
        const float UpperBound = FMath::Max(1.0f, GetMaxHealth());
        NewValue = FMath::IsFinite(NewValue)
            ? FMath::Clamp(NewValue, 0.0f, UpperBound)
            : FMath::Clamp(GetHealth(), 0.0f, UpperBound);
    }
}

void UETHealthAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
    {
        if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
        {
            ASC->SetNumericAttributeBase(GetHealthAttribute(), NewValue);
        }
    }
}