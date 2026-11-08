#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Inventory/ETRelicDefinition.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/AssetManager.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicDefinitionsTest, "EmberTrial.M3.Inventory.Definitions",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETRelicDefinitionsTest::RunTest(const FString& Parameters)
{
    // ---- 独立临时定义：不Spawn身体、不改CDO或磁盘资产 ----
    TStrongObjectPtr<UETRelicDefinition> Life(NewObject<UETRelicDefinition>());
    TStrongObjectPtr<UETRelicDefinition> Edge(NewObject<UETRelicDefinition>());
    auto Configure = [](UETRelicDefinition* Definition, const FName Id,
                        const EETRelicModifierKind Kind, const float Value)
    {
        Definition->RelicId = Id;
        Definition->DisplayName = FText::FromString(TEXT("Fixture"));
        Definition->Description = FText::FromString(TEXT("Definition contract fixture"));
        FETRelicModifier Modifier;
        Modifier.Kind = Kind;
        Modifier.AdditiveValue = Value;
        Definition->Modifiers = {Modifier};
    };
    Configure(Life.Get(), FName(TEXT("test_life")), EETRelicModifierKind::MaxHealth, 20.0f);
    Configure(Edge.Get(), FName(TEXT("test_edge")), EETRelicModifierKind::LightAttackBonus, 5.0f);

    TArray<FText> Errors;
    TestTrue(TEXT("Complete definition with placeholder icon is valid"),
             Life->CollectValidationErrors(Errors));
    TestTrue(TEXT("First slot supported"), Life->SupportsSlot(EETRelicSlot::First));
    TestTrue(TEXT("Second slot supported"), Life->SupportsSlot(EETRelicSlot::Second));
    TestFalse(TEXT("Unknown slot rejected"), Life->SupportsSlot(static_cast<EETRelicSlot>(255)));
    TestTrue(TEXT("Stable primary ID uses data ID"),
             Life->GetPrimaryAssetId() ==
                 FPrimaryAssetId(UETRelicDefinition::AssetType, FName(TEXT("test_life"))));

    Life->RelicId = NAME_None;
    TestFalse(TEXT("Empty ID rejected"), Life->CollectValidationErrors(Errors));
    TestFalse(TEXT("Empty ID has no valid primary ID"), Life->GetPrimaryAssetId().IsValid());
    Life->RelicId = FName(TEXT("bad.id"));
    TestFalse(TEXT("Punctuation ID rejected"), Life->CollectValidationErrors(Errors));
    Life->RelicId = FName(TEXT("test_life"));

    Life->AllowedSlots = {EETRelicSlot::First, EETRelicSlot::First};
    TestFalse(TEXT("Duplicate slot rejected"), Life->CollectValidationErrors(Errors));
    Life->AllowedSlots.Reset();
    TestFalse(TEXT("No compatible slot rejected"), Life->CollectValidationErrors(Errors));
    Life->AllowedSlots = {EETRelicSlot::First, EETRelicSlot::Second};
    Life->Modifiers[0].AdditiveValue = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN modifier rejected"), Life->CollectValidationErrors(Errors));
    Life->Modifiers[0].AdditiveValue = 0.0f;
    TestFalse(TEXT("Zero modifier rejected"), Life->CollectValidationErrors(Errors));
    Life->Modifiers[0].AdditiveValue = -10.0f;
    TestTrue(TEXT("Finite tradeoff value permitted"), Life->CollectValidationErrors(Errors));
    Life->Modifiers[0].AdditiveValue = 20.0f;
    // 先复制值，避免TArray扩容时传入指向自身元素的引用。
    const FETRelicModifier RepeatedModifier = Life->Modifiers[0];
    Life->Modifiers.Add(RepeatedModifier);
    TestFalse(TEXT("Repeated modifier kind rejected"), Life->CollectValidationErrors(Errors));
    Life->Modifiers.SetNum(1);

    TArray<const UETRelicDefinition*> Catalog = {Life.Get(), Edge.Get()};
    TestTrue(TEXT("Independent definitions form valid catalog"),
             UETRelicDefinition::CollectCatalogErrors(Catalog, Errors));
    Edge->RelicId = Life->RelicId;
    TestFalse(TEXT("Different objects with same ID rejected"),
              UETRelicDefinition::CollectCatalogErrors(Catalog, Errors));
    TestTrue(TEXT("Duplicate ID diagnostic is actionable"),
             Errors.ContainsByPredicate([](const FText& Error)
             {
                 return Error.ToString().Contains(TEXT("ET-RELIC-006"));
             }));
    Edge->RelicId = FName(TEXT("test_edge"));
    Catalog.Add(nullptr);
    TestFalse(TEXT("Null catalog entry rejected"),
              UETRelicDefinition::CollectCatalogErrors(Catalog, Errors));

    // ---- 实物目录 + 自动注册：从磁盘路径枚举，防PrimaryAssetId去重掩盖重复ID ----
    UAssetManager* Manager = UAssetManager::GetIfInitialized();
    if (!TestNotNull(TEXT("AssetManager initialized"), Manager))
    {
        return false;
    }
    FAssetRegistryModule& RegistryModule =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& Registry = RegistryModule.Get();
    const FString Directory(TEXT("/Game/EmberTrial/Items/Relics"));
    Registry.ScanPathsSynchronous({Directory});
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*Directory));
    Filter.ClassPaths.Add(UETRelicDefinition::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    TArray<FAssetData> Assets;
    Registry.GetAssets(Filter, Assets);
    TestTrue(TEXT("At least two saved relic definitions discovered"), Assets.Num() >= 2);

    TArray<TStrongObjectPtr<UETRelicDefinition>> LoadedDefinitions;
    Catalog.Reset();
    TSet<FName> RegisteredIds;
    for (const FAssetData& Asset : Assets)
    {
        UETRelicDefinition* Definition = Cast<UETRelicDefinition>(Asset.GetAsset());
        if (!TestNotNull(FString::Printf(TEXT("Load %s"), *Asset.GetObjectPathString()), Definition))
        {
            continue;
        }
        LoadedDefinitions.Emplace(Definition);
        Catalog.Add(Definition);
        RegisteredIds.Add(Definition->GetRelicId());
        TestTrue(FString::Printf(TEXT("Configured AssetManager path matches %s"),
                                 *Definition->GetRelicId().ToString()),
                 Manager->GetPrimaryAssetPath(Definition->GetPrimaryAssetId()) ==
                     FSoftObjectPath(Definition));
    }
    const bool bCatalogValid = UETRelicDefinition::CollectCatalogErrors(Catalog, Errors);
    for (const FText& Error : Errors)
    {
        AddError(Error.ToString());
    }
    TestTrue(TEXT("Saved definitions satisfy catalog contract"), bCatalogValid);
    TestTrue(TEXT("Life Ember registered"), RegisteredIds.Contains(FName(TEXT("life_ember"))));
    TestTrue(TEXT("Keen Edge registered"), RegisteredIds.Contains(FName(TEXT("keen_edge"))));

    // 实物配置合同：一件遗物可组合两种已支持效果，不为ID增加专用玩法分支。
    const UETRelicDefinition* Tradeoff = nullptr;
    for (const UETRelicDefinition* Definition : Catalog)
    {
        if (Definition->GetRelicId() == FName(TEXT("blood_pact")))
        {
            Tradeoff = Definition;
            break;
        }
    }
    if (!TestNotNull(TEXT("Blood Pact saved definition registered"), Tradeoff))
    {
        return false;
    }
    TestTrue(TEXT("Tradeoff supports both slots"), Tradeoff->SupportsSlot(EETRelicSlot::First) &&
             Tradeoff->SupportsSlot(EETRelicSlot::Second));
    const TArray<FETRelicModifier>& Modifiers = Tradeoff->GetModifiers();
    TestEqual(TEXT("Tradeoff has exactly two effects"), Modifiers.Num(), 2);
    const FETRelicModifier* HealthCost = Modifiers.FindByPredicate([](const FETRelicModifier& Modifier)
    { return Modifier.Kind == EETRelicModifierKind::MaxHealth; });
    const FETRelicModifier* AttackGain = Modifiers.FindByPredicate([](const FETRelicModifier& Modifier)
    { return Modifier.Kind == EETRelicModifierKind::LightAttackBonus; });
    TestTrue(TEXT("Tradeoff health cost is minus20"), HealthCost && HealthCost->AdditiveValue == -20.0f);
    TestTrue(TEXT("Tradeoff attack gain is plus10"), AttackGain && AttackGain->AdditiveValue == 10.0f);

    const UETRelicDefinition* Renewal = nullptr;
    for (const UETRelicDefinition* Definition : Catalog)
    {
        if (Definition->GetRelicId() == FName(TEXT("renewal_charm")))
        {
            Renewal = Definition;
            break;
        }
    }
    if (!TestNotNull(TEXT("Renewal Charm saved definition registered"), Renewal))
    {
        return false;
    }
    TestTrue(TEXT("Renewal supports both slots"), Renewal->SupportsSlot(EETRelicSlot::First) &&
             Renewal->SupportsSlot(EETRelicSlot::Second));
    TestEqual(TEXT("Renewal sort order forty"), Renewal->GetSortOrder(), 40);
    const TArray<FETRelicModifier>& HealingModifiers = Renewal->GetModifiers();
    if (TestEqual(TEXT("Renewal has exactly one effect"), HealingModifiers.Num(), 1))
    {
        TestTrue(TEXT("Renewal healing bonus is plus ten"),
                 HealingModifiers[0].Kind == EETRelicModifierKind::HealingBonus &&
                 HealingModifiers[0].AdditiveValue == 10.0f);
    }

    const UETRelicDefinition* Breath = nullptr;
    for (const UETRelicDefinition* Definition : Catalog)
    {
        if (Definition->GetRelicId() == FName(TEXT("breath_charm")))
        {
            Breath = Definition;
            break;
        }
    }
    if (!TestNotNull(TEXT("Breath Charm saved definition registered"), Breath))
    {
        return false;
    }
    TestTrue(TEXT("Breath Charm supports both slots"), Breath->SupportsSlot(EETRelicSlot::First) &&
             Breath->SupportsSlot(EETRelicSlot::Second));
    TestEqual(TEXT("Breath Charm sort order fifty"), Breath->GetSortOrder(), 50);
    const TArray<FETRelicModifier>& RecoveryModifiers = Breath->GetModifiers();
    if (TestEqual(TEXT("Breath Charm has exactly one effect"), RecoveryModifiers.Num(), 1))
    {
        TestTrue(TEXT("Breath Charm recovery bonus is plus five"),
                 RecoveryModifiers[0].Kind == EETRelicModifierKind::StaminaRecoveryBonus &&
                 RecoveryModifiers[0].AdditiveValue == 5.0f);
    }
    return !HasAnyErrors();
}

#endif