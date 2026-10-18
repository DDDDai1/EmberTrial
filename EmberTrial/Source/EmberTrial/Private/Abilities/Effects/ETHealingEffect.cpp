#include "Abilities/Effects/ETHealingEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETHealthAttributeSet.h"

UETHealingEffect::UETHealingEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // ---- 固定生效方式与外部数值契约 ----
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = ETGameplayTags::SetByCaller_HealthDelta;

    // ---- 唯一Health Modifier；上下限 / 死亡夹限仍由AttributeSet处理 ----
    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UETHealthAttributeSet::GetHealthAttribute();
    Modifier.ModifierOp = EGameplayModOp::AddBase;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}
