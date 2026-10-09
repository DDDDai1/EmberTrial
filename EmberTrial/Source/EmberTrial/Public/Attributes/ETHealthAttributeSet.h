#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETHealthAttributeSet.generated.h"

UCLASS()
class EMBERTRIAL_API UETHealthAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETHealthAttributeSet(const FObjectInitializer& ObjectInitializer);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETHealthAttributeSet, Health)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Health)
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETHealthAttributeSet, MaxHealth)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Health", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Health;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Health", meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxHealth;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(Health)
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxHealth)

    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};