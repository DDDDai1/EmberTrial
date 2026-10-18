#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"

UETStaminaDeltaEffect::UETStaminaDeltaEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 不持有Duration状态，也不修改Health；属性夹限仍由AttributeSet负责。
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = ETGameplayTags::SetByCaller_StaminaDelta;

    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UETStaminaAttributeSet::GetStaminaAttribute();
    Modifier.ModifierOp = EGameplayModOp::AddBase;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}
