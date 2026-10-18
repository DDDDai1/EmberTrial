#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETStaminaAttributeSet.generated.h"

// ASC管理的体力数值；Character只保存启动配置，UI只读事件快照。
UCLASS()
class EMBERTRIAL_API UETStaminaAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETStaminaAttributeSet(const FObjectInitializer& ObjectInitializer);

    // 对外只提供属性与数值查询；初始化宏留在private，修改仍经ASC / GE。
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETStaminaAttributeSet, Stamina)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Stamina)
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETStaminaAttributeSet, MaxStamina)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxStamina)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                     float NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Stamina",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Stamina;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Stamina",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxStamina;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(Stamina)
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxStamina)

    // Base与Current共用夹限入口；上限改变后的同步由PostAttributeChange负责。
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};