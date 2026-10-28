#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETCombatAttributeSet.generated.h"

// 有符号轻击加成由GAS聚合；基础伤害 / 段倍率仍由GA配置，不保存重复副本。
UCLASS()
class EMBERTRIAL_API UETCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETCombatAttributeSet(const FObjectInitializer& ObjectInitializer);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, LightAttackBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(LightAttackBonus)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData LightAttackBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(LightAttackBonus)
};