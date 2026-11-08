#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETCombatAttributeSet.generated.h"

// 有符号加成由GAS聚合；能力保存基础数值并决定最终结算，不重复存储运行值。
UCLASS()
class EMBERTRIAL_API UETCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETCombatAttributeSet(const FObjectInitializer& ObjectInitializer);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, LightAttackBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(LightAttackBonus)

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, HealingBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(HealingBonus)

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, StaminaRecoveryBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(StaminaRecoveryBonus)

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, BreakPulsePoiseBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(BreakPulsePoiseBonus)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

private:
    // 基础伤害加成；段倍率仍由轻击能力配置。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData LightAttackBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(LightAttackBonus)

    // 正式Q的固定治疗加成，不改变次数、最大生命或其他治疗来源。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData HealingBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(HealingBonus)

    // 体力每秒恢复的固定加成，不改变消费、上限或消费后延迟。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData StaminaRecoveryBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(StaminaRecoveryBonus)

    // 正式F施放时快照的固定破势加成，单位为架势点。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData BreakPulsePoiseBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(BreakPulsePoiseBonus)
};