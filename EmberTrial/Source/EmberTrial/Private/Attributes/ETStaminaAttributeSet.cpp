#include "Attributes/ETStaminaAttributeSet.h"
#include "AbilitySystemComponent.h"

UETStaminaAttributeSet::UETStaminaAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 仅是构造基线；正式初值由Character验证后统一发布Ready。
    InitMaxStamina(100.0f);
    InitStamina(0.0f);
}

// ---- 数值夹限与非有限输入回退，不保存另一份实时体力 ----

void UETStaminaAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute,
                                            float& NewValue) const
{
    const float SafeMax =
        FMath::IsFinite(GetMaxStamina()) ? FMath::Max(1.0f, GetMaxStamina()) : 1.0f;

    if (Attribute == GetMaxStaminaAttribute())
    {
        NewValue = FMath::IsFinite(NewValue) ? FMath::Max(1.0f, NewValue) : SafeMax;
    }
    else if (Attribute == GetStaminaAttribute())
    {
        const float SafeCurrent =
            FMath::IsFinite(GetStamina()) ? FMath::Clamp(GetStamina(), 0.0f, SafeMax) : 0.0f;
        NewValue = FMath::IsFinite(NewValue) ? FMath::Clamp(NewValue, 0.0f, SafeMax) : SafeCurrent;
    }
}

// ---- Base与Current写入均经过同一入口 ----

void UETStaminaAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                    float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETStaminaAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute,
                                                float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

// 最大值下降时核对Base和Current；恢复上限不会补回之前被夹掉的体力。
void UETStaminaAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute,
                                                 float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxStaminaAttribute())
    {
        if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
        {
            const float Base = ASC->GetNumericAttributeBase(GetStaminaAttribute());
            if (Base > NewValue || GetStamina() > NewValue)
            {
                ASC->SetNumericAttributeBase(GetStaminaAttribute(),
                                             FMath::Clamp(Base, 0.0f, NewValue));
            }
        }
    }
}