#include "Attributes/ETCombatAttributeSet.h"

namespace ETCombatAttributeSetPrivate
{
    float FiniteOrFallback(const float Value, const float Fallback)
    {
        return FMath::IsFinite(Value) ? Value : (FMath::IsFinite(Fallback) ? Fallback : 0.0f);
    }
}

UETCombatAttributeSet::UETCombatAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitLightAttackBonus(0.0f);
    InitHealingBonus(0.0f);
}

// Base与Current分别回退，不能将包含活动GE的Current再次写入Base。
void UETCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                  float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetBaseValue());
    }
    else if (Attribute == GetHealingBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, HealingBonus.GetBaseValue());
    }
}

void UETCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetCurrentValue());
    }
    else if (Attribute == GetHealingBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, HealingBonus.GetCurrentValue());
    }
    // 有限负值合法；最终伤害 / 治疗是否可用，由对应能力预检。
}