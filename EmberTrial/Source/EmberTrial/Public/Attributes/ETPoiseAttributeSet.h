#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETPoiseAttributeSet.generated.h"

// ASC管理的韧性数值；Character只保存启动配置，UI只读事件快照。
UCLASS()
class EMBERTRIAL_API UETPoiseAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETPoiseAttributeSet(const FObjectInitializer& ObjectInitializer);

    // 对外只提供属性与数值查询；初始化宏留在private，修改仍经ASC / GE。
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETPoiseAttributeSet, Poise)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Poise)
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETPoiseAttributeSet, MaxPoise)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxPoise)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                     float NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Poise",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Poise;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Poise",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxPoise;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(Poise)
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxPoise)

    // Base与Current共用夹限入口；上限改变后的同步由PostAttributeChange负责。
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};