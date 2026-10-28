#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"

UETRelicEquipmentEffect::UETRelicEquipmentEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    Period = FScalableFloat(0.0f);
    StackingType = EGameplayEffectStackingType::None;

    FSetByCallerFloat HealthMagnitude;
    HealthMagnitude.DataTag = ETGameplayTags::SetByCaller_EquipmentMaxHealth;
    FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
    HealthModifier.Attribute = UETHealthAttributeSet::GetMaxHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::AddBase;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthMagnitude);

    FSetByCallerFloat AttackMagnitude;
    AttackMagnitude.DataTag = ETGameplayTags::SetByCaller_EquipmentLightAttackBonus;
    FGameplayModifierInfo& AttackModifier = Modifiers.AddDefaulted_GetRef();
    AttackModifier.Attribute = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    AttackModifier.ModifierOp = EGameplayModOp::AddBase;
    AttackModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackMagnitude);
}