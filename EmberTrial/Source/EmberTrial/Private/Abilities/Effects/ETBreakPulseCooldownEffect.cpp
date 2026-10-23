#include "Abilities/Effects/ETBreakPulseCooldownEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UETBreakPulseCooldownEffect::UETBreakPulseCooldownEffect(
    const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    // 正值只是CDO结构占位；运行Spec必须SetDuration，不在这里保存6秒平衡数值。
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f));
    Period = FScalableFloat(0.0f);
    StackingType = EGameplayEffectStackingType::None;
    UTargetTagsGameplayEffectComponent* Tags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("BreakPulseCooldownTags"));
    GEComponents.Add(Tags);
    FInheritedTagContainer Granted;
    Granted.AddTag(ETGameplayTags::Cooldown_BreakPulse);
    Tags->SetAndApplyTargetTagChanges(Granted);
}