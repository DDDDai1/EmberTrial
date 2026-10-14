#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETStaminaAttributeSet.generated.h"

UCLASS()
class EMBERTRIAL_API UETStaminaAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETStaminaAttributeSet(const FObjectInitializer& ObjectInitializer);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETStaminaAttributeSet, Stamina)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Stamina)
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETStaminaAttributeSet, MaxStamina)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxStamina)

    virtual void PreAttributeBaseChange(
        const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(
        const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(
        const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Stamina",
        meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Stamina;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Stamina",
        meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxStamina;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(Stamina)
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxStamina)

    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};