#include "Inventory/ETRelicLoadout.h"
#include "Inventory/ETRelicDefinition.h"
#include "Math/NumericLimits.h"

namespace ETRelicLoadoutPrivate
{
bool FitsFloat(const double Value)
{
    return FMath::IsFinite(Value) && FMath::Abs(Value) <= TNumericLimits<float>::Max();
}
}

EETRelicLoadoutResult ETRelicLoadoutRules::Evaluate(
    const FETRelicLoadout& Loadout, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, FETRelicModifierTotals& OutTotals)
{
    OutTotals = {};
    if (!Loadout.SlotA.IsNone() && Loadout.SlotA == Loadout.SlotB)
    {
        return EETRelicLoadoutResult::DuplicateRelic;
    }

    // 双精度中间和避免float加法先溢出；最终仍输出项目使用的float。
    double HealthAdditive = 0.0;
    double AttackAdditive = 0.0;
    const FName Ids[] = {Loadout.SlotA, Loadout.SlotB};
    const EETRelicSlot Slots[] = {EETRelicSlot::First, EETRelicSlot::Second};
    for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Ids)); ++Index)
    {
        const FName Id = Ids[Index];
        if (Id.IsNone())
        {
            continue;
        }
        if (!OwnedIds.Contains(Id))
        {
            return EETRelicLoadoutResult::NotOwned;
        }
        const UETRelicDefinition* const* Found = Definitions.Find(Id);
        if (!Found || !IsValid(*Found))
        {
            return EETRelicLoadoutResult::MissingDefinition;
        }
        const UETRelicDefinition* Definition = *Found;
        TArray<FText> Errors;
        if (Definition->GetRelicId() != Id || !Definition->CollectValidationErrors(Errors))
        {
            return EETRelicLoadoutResult::InvalidDefinition;
        }
        if (!Definition->SupportsSlot(Slots[Index]))
        {
            return EETRelicLoadoutResult::SlotIncompatible;
        }
        for (const FETRelicModifier& Modifier : Definition->GetModifiers())
        {
            switch (Modifier.Kind)
            {
            case EETRelicModifierKind::MaxHealth:
                HealthAdditive += Modifier.AdditiveValue;
                break;
            case EETRelicModifierKind::LightAttackBonus:
                AttackAdditive += Modifier.AdditiveValue;
                break;
            default:
                return EETRelicLoadoutResult::InvalidDefinition;
            }
        }
    }
    if (!ETRelicLoadoutPrivate::FitsFloat(HealthAdditive) ||
        !ETRelicLoadoutPrivate::FitsFloat(AttackAdditive))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
    OutTotals.MaxHealthAdditive = static_cast<float>(HealthAdditive);
    OutTotals.LightAttackBonus = static_cast<float>(AttackAdditive);
    return EETRelicLoadoutResult::Success;
}

EETRelicLoadoutResult ETRelicLoadoutRules::MakePreview(
    const float BaseMaxHealth, const float BaseLightAttackDamage,
    const FETRelicModifierTotals& Totals, FETRelicLoadoutPreview& OutPreview)
{
    OutPreview = {};
    if (!FMath::IsFinite(BaseMaxHealth) || BaseMaxHealth < 1.0f ||
        !FMath::IsFinite(BaseLightAttackDamage) || BaseLightAttackDamage <= 0.0f)
    {
        return EETRelicLoadoutResult::InvalidBaseValues;
    }
    const double Health = static_cast<double>(BaseMaxHealth) + Totals.MaxHealthAdditive;
    const double Damage = static_cast<double>(BaseLightAttackDamage) + Totals.LightAttackBonus;
    if (!ETRelicLoadoutPrivate::FitsFloat(Health) || !ETRelicLoadoutPrivate::FitsFloat(Damage))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
    if (Health < 1.0 || Damage <= 0.0)
    {
        return EETRelicLoadoutResult::InvalidFinalValues;
    }
    OutPreview.MaxHealth = static_cast<float>(Health);
    OutPreview.LightAttackBaseDamage = static_cast<float>(Damage);
    return EETRelicLoadoutResult::Success;
}

EETRelicLoadoutResult ETRelicLoadoutRules::BuildReplacementPlan(
    const FETRelicLoadout& Current, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, const EETRelicSlot RequestedSlot, const FName RequestedId,
    const float BaseMaxHealth, const float BaseLightAttackDamage, FETRelicLoadoutPlan& OutPlan)
{
    // 先复制，允许Current来自上一次OutPlan.After；清输出不会抹掉输入。
    const FETRelicLoadout Before = Current;
    OutPlan = {};
    if (RequestedSlot != EETRelicSlot::First && RequestedSlot != EETRelicSlot::Second)
    {
        return EETRelicLoadoutResult::InvalidSlot;
    }

    FETRelicLoadoutPlan Candidate;
    Candidate.Before = Before;
    EETRelicLoadoutResult Result = Evaluate(Before, OwnedIds, Definitions, Candidate.BeforeModifiers);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }
    Result = MakePreview(BaseMaxHealth, BaseLightAttackDamage,
                         Candidate.BeforeModifiers, Candidate.BeforePreview);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }

    Candidate.After = Before;
    FName& Destination = RequestedSlot == EETRelicSlot::First
                             ? Candidate.After.SlotA : Candidate.After.SlotB;
    Destination = RequestedId;
    if (Candidate.After == Before)
    {
        return EETRelicLoadoutResult::NoChange;
    }
    Result = Evaluate(Candidate.After, OwnedIds, Definitions, Candidate.AfterModifiers);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }
    Result = MakePreview(BaseMaxHealth, BaseLightAttackDamage,
                         Candidate.AfterModifiers, Candidate.AfterPreview);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }
    OutPlan = Candidate;
    return EETRelicLoadoutResult::Success;
}