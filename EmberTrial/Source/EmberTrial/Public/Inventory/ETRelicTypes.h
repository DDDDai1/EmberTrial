#pragma once

#include "CoreMinimal.h"
#include "ETRelicTypes.generated.h"

// 槽位是规则标识，不使用UI中的数组下标或显示文字作为存储ID。
UENUM(BlueprintType)
enum class EETRelicSlot : uint8
{
    First UMETA(DisplayName = "Slot A"),
    Second UMETA(DisplayName = "Slot B")
};

// 先实现两种确有消费者计划的加法效果；新机制在对应批次扩展。
UENUM(BlueprintType)
enum class EETRelicModifierKind : uint8
{
    MaxHealth UMETA(DisplayName = "Max Health"),
    LightAttackBonus UMETA(DisplayName = "Light Attack Bonus")
};

USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETRelicModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
    EETRelicModifierKind Kind = EETRelicModifierKind::MaxHealth;

    // 加法参数，允许代价型负值；最终属性合法下限由装备事务 / 属性集保证。
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic")
    float AdditiveValue = 0.0f;
};