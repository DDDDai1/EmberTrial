#include "Abilities/Effects/ETRangedShotCooldownEffect.h"

#include "Abilities/ETGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UETRangedShotCooldownEffect::UETRangedShotCooldownEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.5f));

    UTargetTagsGameplayEffectComponent* Tags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("RangedShotCooldownTags"));
    GEComponents.Add(Tags);
    FInheritedTagContainer Granted;
    Granted.AddTag(ETGameplayTags::Cooldown_RangedShot);
    Tags->SetAndApplyTargetTagChanges(Granted);
}