#pragma once

#include "CoreMinimal.h"
#include "Inventory/ETRelicTypes.h"
#include "ETRelicLoadout.generated.h"

class UETRelicDefinition;

/** 逻辑槽位快照；只含稳定ID，不含定义对象、ASC或GE句柄。 */
USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETRelicLoadout
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Relic")
    FName SlotA = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Relic")
    FName SlotB = NAME_None;

    bool operator==(const FETRelicLoadout& Other) const
    {
        return SlotA == Other.SlotA && SlotB == Other.SlotB;
    }
};

// 原生计算结果；当前只处理定义支持的两种加法，不模拟任意动态GAS效果。
struct EMBERTRIAL_API FETRelicModifierTotals
{
    float MaxHealthAdditive = 0.0f;
    float LightAttackBonus = 0.0f;
};

struct EMBERTRIAL_API FETRelicLoadoutPreview
{
    float MaxHealth = 0.0f;
    float LightAttackBaseDamage = 0.0f;
};

struct EMBERTRIAL_API FETRelicLoadoutPlan
{
    FETRelicLoadout Before;
    FETRelicLoadout After;
    FETRelicModifierTotals BeforeModifiers;
    FETRelicModifierTotals AfterModifiers;
    FETRelicLoadoutPreview BeforePreview;
    FETRelicLoadoutPreview AfterPreview;
};

enum class EETRelicLoadoutResult : uint8
{
    Success,
    NoChange,
    InvalidSlot,
    NotOwned,
    MissingDefinition,
    InvalidDefinition,
    SlotIncompatible,
    DuplicateRelic,
    NumericOverflow,
    InvalidBaseValues,
    InvalidFinalValues
};

namespace ETRelicLoadoutRules
{
// 调用期间借用目录；函数不加载资产、不持有这些对象，也不修改输入。
using FDefinitionMap = TMap<FName, const UETRelicDefinition*>;

EMBERTRIAL_API EETRelicLoadoutResult Evaluate(
    const FETRelicLoadout& Loadout, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, FETRelicModifierTotals& OutTotals);

// Base参数必须不包含装备加成，否则会重复计算。失败输出清空。
EMBERTRIAL_API EETRelicLoadoutResult MakePreview(
    float BaseMaxHealth, float BaseLightAttackDamage,
    const FETRelicModifierTotals& Totals, FETRelicLoadoutPreview& OutPreview);

// RequestedId=None明确表示卸下该槽；只生成方案，不提交库存或应用GE。
// 只有Success携带可用Plan；NoChange / 失败输出均清空，调用方必须检查结果。
EMBERTRIAL_API EETRelicLoadoutResult BuildReplacementPlan(
    const FETRelicLoadout& Current, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, EETRelicSlot RequestedSlot, FName RequestedId,
    float BaseMaxHealth, float BaseLightAttackDamage, FETRelicLoadoutPlan& OutPlan);
}