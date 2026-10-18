#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETHealthAttributeSet.generated.h"

// ASC管理的生命数值；Character只保存启动配置，UI只读事件快照。
UCLASS()
class EMBERTRIAL_API UETHealthAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETHealthAttributeSet(const FObjectInitializer& ObjectInitializer);

    // 对外只提供属性与数值查询；初始化宏留在private，修改仍经ASC / GE。
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETHealthAttributeSet, Health)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Health)
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETHealthAttributeSet, MaxHealth)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                     float NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Health",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Health;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Health",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxHealth;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(Health)
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxHealth)

    // Base与Current共用夹限入口；上限改变后的同步由PostAttributeChange负责。
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};