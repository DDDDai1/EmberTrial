#include "Attributes/ETPoiseAttributeSet.h"
#include "AbilitySystemComponent.h"

UETPoiseAttributeSet::UETPoiseAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 仅是构造基线；正式初值由Character验证后统一发布Ready。
    InitMaxPoise(100.0f);
    InitPoise(0.0f);
}

// ---- 数值夹限与非有限输入回退，不保存另一份实时韧性 ----

void UETPoiseAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute,
                                          float& NewValue) const
{
    const float SafeMax = FMath::IsFinite(GetMaxPoise()) ? FMath::Max(1.0f, GetMaxPoise()) : 1.0f;

    if (Attribute == GetMaxPoiseAttribute())
    {
        NewValue = FMath::IsFinite(NewValue) ? FMath::Max(1.0f, NewValue) : SafeMax;
    }
    else if (Attribute == GetPoiseAttribute())
    {
        const float SafeCurrent =
            FMath::IsFinite(GetPoise()) ? FMath::Clamp(GetPoise(), 0.0f, SafeMax) : 0.0f;
        NewValue = FMath::IsFinite(NewValue) ? FMath::Clamp(NewValue, 0.0f, SafeMax) : SafeCurrent;
    }
}

// ---- Base与Current写入均经过同一入口 ----

void UETPoiseAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                  float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETPoiseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

// 最大值下降时核对Base和Current；提高上限不会补回之前被夹掉的韧性。
void UETPoiseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                               float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxPoiseAttribute())
    {
        if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
        {
            const float Base = ASC->GetNumericAttributeBase(GetPoiseAttribute());
            if (Base > NewValue || GetPoise() > NewValue)
            {
                ASC->SetNumericAttributeBase(GetPoiseAttribute(),
                                             FMath::Clamp(Base, 0.0f, NewValue));
            }
        }
    }
}