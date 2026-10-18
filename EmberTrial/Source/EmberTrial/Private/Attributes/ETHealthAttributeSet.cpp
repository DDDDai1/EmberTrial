
#include "Attributes/ETHealthAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/ETGameplayTags.h"

UETHealthAttributeSet::UETHealthAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 构造值不代表GameplayReady；正式初值由Character校验后经ASC一次写入。
    InitMaxHealth(100.0f);
    InitHealth(0.0f);
}

// ---- Base / Current共同约束：合法区间与死亡后禁止回填生命 ----

void UETHealthAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute,
                                           float& NewValue) const
{
    if (Attribute == GetMaxHealthAttribute())
    {
        NewValue = FMath::IsFinite(NewValue) ? FMath::Max(1.0f, NewValue)
                                             : FMath::Max(1.0f, GetMaxHealth());
    }
    else if (Attribute == GetHealthAttribute())
    {
        const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
        if (IsValid(ASC) && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
        {
            NewValue = 0.0f;
            return;
        }
        const float UpperBound = FMath::Max(1.0f, GetMaxHealth());
        NewValue = FMath::IsFinite(NewValue) ? FMath::Clamp(NewValue, 0.0f, UpperBound)
                                             : FMath::Clamp(GetHealth(), 0.0f, UpperBound);
    }
}

// ---- 两种属性写入路径都必须夹限，避免Base保留不可见的越界值 ----

void UETHealthAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                   float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

// 上限下降只压低越界生命；上限上升不自动治疗。通过ASC更新以传播真实事件。
void UETHealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                                float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
    {
        if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
        {
            ASC->SetNumericAttributeBase(GetHealthAttribute(), NewValue);
        }
    }
}