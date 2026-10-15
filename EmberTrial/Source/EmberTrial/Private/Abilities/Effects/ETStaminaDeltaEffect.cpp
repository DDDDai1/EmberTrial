#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"

UETStaminaDeltaEffect::UETStaminaDeltaEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = ETGameplayTags::SetByCaller_StaminaDelta;

    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UETStaminaAttributeSet::GetStaminaAttribute();
    Modifier.ModifierOp = EGameplayModOp::AddBase;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}