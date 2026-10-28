#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicLoadout.h"
#include "Inventory/ETRelicDefinition.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicLoadoutTest, "EmberTrial.M3.Inventory.LoadoutRules",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETRelicLoadoutTest::RunTest(const FString& Parameters)
{
    using EResult = EETRelicLoadoutResult;
    TStrongObjectPtr<UETRelicDefinition> Life(NewObject<UETRelicDefinition>());
    TStrongObjectPtr<UETRelicDefinition> Edge(NewObject<UETRelicDefinition>());
    auto Configure = [](UETRelicDefinition* Definition, const FName Id,
                        const EETRelicModifierKind Kind, const float Value)
    {
        Definition->RelicId = Id;
        Definition->DisplayName = FText::FromString(TEXT("Fixture"));
        Definition->Description = FText::FromString(TEXT("Loadout rules fixture"));
        FETRelicModifier Modifier;
        Modifier.Kind = Kind;
        Modifier.AdditiveValue = Value;
        Definition->Modifiers = {Modifier};
    };
    const FName LifeId(TEXT("fixture_life"));
    const FName EdgeId(TEXT("fixture_edge"));
    Configure(Life.Get(), LifeId, EETRelicModifierKind::MaxHealth, 20.0f);
    Configure(Edge.Get(), EdgeId, EETRelicModifierKind::LightAttackBonus, 5.0f);
    const TSet<FName> Owned = {LifeId, EdgeId};
    ETRelicLoadoutRules::FDefinitionMap Catalog;
    Catalog.Add(LifeId, Life.Get());
    Catalog.Add(EdgeId, Edge.Get());
    const FETRelicLoadout Empty;
    FETRelicLoadoutPlan Plan;

    // ---- 装入 / 替换 / 卸下与输入不变；预览数值独立给出期望 ----
    auto Build = [&](const FETRelicLoadout& Current, const EETRelicSlot Slot, const FName Id)
    {
        return ETRelicLoadoutRules::BuildReplacementPlan(
            Current, Owned, Catalog, Slot, Id, 100.0f, 25.0f, Plan);
    };
    TestTrue(TEXT("Life plan accepted"), Build(Empty, EETRelicSlot::First, LifeId) == EResult::Success);
    TestTrue(TEXT("Input remains empty"), Empty.SlotA.IsNone() && Empty.SlotB.IsNone());
    TestEqual(TEXT("Before health"), Plan.BeforePreview.MaxHealth, 100.0f);
    TestEqual(TEXT("After health"), Plan.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Life does not increase damage"), Plan.AfterPreview.LightAttackBaseDamage, 25.0f);
    TestTrue(TEXT("Output alias is safe"),
             Build(Plan.After, EETRelicSlot::Second, EdgeId) == EResult::Success);
    const FETRelicLoadout Both = Plan.After;
    TestTrue(TEXT("Two independent slots retained"), Both.SlotA == LifeId && Both.SlotB == EdgeId);
    TestEqual(TEXT("Combined health"), Plan.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Combined damage"), Plan.AfterPreview.LightAttackBaseDamage, 30.0f);
    TestTrue(TEXT("Unequip life accepted"), Build(Both, EETRelicSlot::First, NAME_None) == EResult::Success);
    TestEqual(TEXT("Unequip health"), Plan.AfterPreview.MaxHealth, 100.0f);
    TestEqual(TEXT("Other slot damage retained"), Plan.AfterPreview.LightAttackBaseDamage, 30.0f);
    TestTrue(TEXT("Both input unchanged"), Both.SlotA == LifeId && Both.SlotB == EdgeId);
    TestTrue(TEXT("Replacement recomputes instead of adding old bonus"),
             Build(Plan.After, EETRelicSlot::Second, LifeId) == EResult::Success);
    TestEqual(TEXT("Replacement clears old damage bonus"), Plan.AfterPreview.LightAttackBaseDamage, 25.0f);
    TestEqual(TEXT("Replacement adds life once"), Plan.AfterPreview.MaxHealth, 120.0f);

    // ---- 拒绝不复用旧输出，NoChange不是可提交的新方案 ----
    TestTrue(TEXT("Duplicate in two slots rejected"),
             Build(Both, EETRelicSlot::Second, LifeId) == EResult::DuplicateRelic);
    TestTrue(TEXT("Rejected plan cleared"), Plan.After.SlotA.IsNone() && Plan.After.SlotB.IsNone());
    TestTrue(TEXT("Same item same slot is NoChange"),
             Build(Both, EETRelicSlot::Second, EdgeId) == EResult::NoChange);
    TestEqual(TEXT("NoChange output cleared"), Plan.AfterPreview.MaxHealth, 0.0f);
    TestTrue(TEXT("Unknown slot rejected"),
             Build(Empty, static_cast<EETRelicSlot>(255), LifeId) == EResult::InvalidSlot);
    TestTrue(TEXT("Unowned item rejected"),
             Build(Empty, EETRelicSlot::First, FName(TEXT("not_owned"))) == EResult::NotOwned);
    Catalog.Remove(LifeId);
    TestTrue(TEXT("Owned but missing definition rejected"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::MissingDefinition);
    Catalog.Add(LifeId, Life.Get());
    Life->AllowedSlots = {EETRelicSlot::First};
    TestTrue(TEXT("Incompatible slot rejected"),
             Build(Empty, EETRelicSlot::Second, LifeId) == EResult::SlotIncompatible);
    Life->AllowedSlots = {EETRelicSlot::First, EETRelicSlot::Second};
    Life->Modifiers[0].AdditiveValue = 0.0f;
    TestTrue(TEXT("Invalid definition rejected"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::InvalidDefinition);
    Life->Modifiers[0].AdditiveValue = 20.0f;
    FETRelicLoadout Broken;
    Broken.SlotA = LifeId;
    Broken.SlotB = LifeId;
    TestTrue(TEXT("Broken current state is not silently repaired"),
             Build(Broken, EETRelicSlot::Second, NAME_None) == EResult::DuplicateRelic);

    // ---- 合法有符号取舍、非法最终值、NaN基值与合计溢出 ----
    Life->Modifiers[0].AdditiveValue = -10.0f;
    TestTrue(TEXT("Legal tradeoff accepted"), Build(Empty, EETRelicSlot::First, LifeId) == EResult::Success);
    TestEqual(TEXT("Tradeoff preview"), Plan.AfterPreview.MaxHealth, 90.0f);
    Life->Modifiers[0].AdditiveValue = -110.0f;
    TestTrue(TEXT("Final nonpositive health rejected"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::InvalidFinalValues);
    Life->Modifiers[0].AdditiveValue = 20.0f;
    TestTrue(TEXT("NaN base rejected"), ETRelicLoadoutRules::BuildReplacementPlan(
        Empty, Owned, Catalog, EETRelicSlot::First, LifeId,
        std::numeric_limits<float>::quiet_NaN(), 25.0f, Plan) == EResult::InvalidBaseValues);
    Life->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    Edge->Modifiers[0].Kind = EETRelicModifierKind::MaxHealth;
    Edge->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    FETRelicModifierTotals Totals;
    TestTrue(TEXT("Finite inputs cannot overflow result silently"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::NumericOverflow);
    TestEqual(TEXT("Overflow output cleared"), Totals.MaxHealthAdditive, 0.0f);
    return !HasAnyErrors();
}

#endif