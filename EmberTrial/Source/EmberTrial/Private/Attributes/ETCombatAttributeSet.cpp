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
}

void UETCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                  float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        // Base回退必须读Base，不能把含活动GE的Current写回Base再多叠一层。
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetBaseValue());
    }
}

void UETCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        // 有限负值合法，最终攻击是否仍为正数由能力 / 装备预检决定。
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetCurrentValue());
    }
}