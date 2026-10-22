#include "Abilities/Effects/ETPoiseDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETPoiseAttributeSet.h"

UETPoiseDeltaEffect::UETPoiseDeltaEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = ETGameplayTags::SetByCaller_PoiseDelta;
    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UETPoiseAttributeSet::GetPoiseAttribute();
    Modifier.ModifierOp = EGameplayModOp::AddBase;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}