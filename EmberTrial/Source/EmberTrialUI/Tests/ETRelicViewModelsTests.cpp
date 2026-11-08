#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicViewModels.h"
#include "Inventory/ETRelicDefinition.h"
#include "HAL/PlatformTime.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicViewModelsTest, "EmberTrial.UI.Relics.ViewModels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicViewModelsTest::RunTest(const FString& Parameters)
{
    using EResult = EETRelicRowsResult;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UGameInstance* GI = Fixture.GetTestWorld()->GetGameInstance();
    UETRelicInventorySubsystem* Inventory = IsValid(GI) ? GI->GetSubsystem<UETRelicInventorySubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Real configured inventory"), Inventory))
    {
        return false;
    }
    const FETRelicInventorySnapshot DomainBefore = Inventory->GetSnapshot();
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    FETRelicListQuery Query;
    TArray<FETRelicListRow> Rows;
    if (!TestTrue(TEXT("Entire configured inventory projects"),
                  ETRelicPresentation::BuildRows(DomainBefore, Inventory, Query, Rows) == EResult::Success) ||
        !TestEqual(TEXT("One row for every configured item"), Rows.Num(), DomainBefore.OwnedIds.Num()) ||
        !TestTrue(TEXT("Reference items remain owned"),
                  DomainBefore.OwnedIds.Contains(LifeId) && DomainBefore.OwnedIds.Contains(EdgeId)))
    {
        return false;
    }
    // 以下是独立的两项纯值样本，专测既有排序 / 搜索 / 过滤；不改真实库存。
    FETRelicInventorySnapshot Original = DomainBefore;
    Original.OwnedIds = {LifeId, EdgeId};
    Original.Equipped = {};
    if (!TestTrue(TEXT("Default rows succeed"),
                  ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows) == EResult::Success) ||
        !TestEqual(TEXT("Two explicit projection sample items"), Rows.Num(), 2))
    {
        return false;
    }
    TestTrue(TEXT("Default catalog order"), Rows[0].RelicId == LifeId && Rows[1].RelicId == EdgeId);
    TestFalse(TEXT("Initial rows are not equipped"), Rows[0].bEquippedInSlotA || Rows[0].bEquippedInSlotB);
    TestTrue(TEXT("Text copied"), !Rows[0].DisplayName.IsEmpty() && !Rows[0].Description.IsEmpty());
    Query.Sort = EETRelicListSort::StableId;
    ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows);
    TestTrue(TEXT("Stable ID order"), Rows.Num() == 2 &&
        Rows[0].RelicId == EdgeId && Rows[1].RelicId == LifeId);
    Query.SearchText = TEXT("  LIFE_  ");
    ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows);
    TestTrue(TEXT("Trimmed case insensitive search"), Rows.Num() == 1 && Rows[0].RelicId == LifeId);
    Query.SearchText = TEXT("没有这个遗物");
    TestTrue(TEXT("No match is valid empty result"),
        ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows) == EResult::Success && Rows.IsEmpty());
    Query = {};
    Query.Filter = EETRelicListFilter::CompatibleWithSlotA;
    ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows);
    TestEqual(TEXT("Both compatible with slotA"), Rows.Num(), 2);
    Query.Filter = EETRelicListFilter::CompatibleWithSlotB;
    ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows);
    TestEqual(TEXT("Both compatible with slotB"), Rows.Num(), 2);

    // 只投影一份值快照，不伪造真实库存提交 / 身体效果。
    FETRelicInventorySnapshot Captured = Original;
    Captured.Equipped.SlotA = LifeId;
    Query.Filter = EETRelicListFilter::Equipped;
    ETRelicPresentation::BuildRows(Captured, Inventory, Query, Rows);
    TestTrue(TEXT("Equipped filter reads captured IDs"), Rows.Num() == 1 &&
        Rows[0].RelicId == LifeId && Rows[0].bEquippedInSlotA && !Rows[0].bEquippedInSlotB);
    Query = {};
    Captured.OwnedIds.Add(LifeId);
    TestTrue(TEXT("Duplicate owned rejected and output cleared"),
        ETRelicPresentation::BuildRows(Captured, Inventory, Query, Rows) == EResult::InvalidSnapshot && Rows.IsEmpty());
    Captured = Original;
    Captured.OwnedIds.Add(FName(TEXT("missing")));
    Query.SearchText = TEXT("life");
    TestTrue(TEXT("Search cannot hide broken catalog"),
        ETRelicPresentation::BuildRows(Captured, Inventory, Query, Rows) == EResult::InvalidCatalog && Rows.IsEmpty());
    Captured = Original;
    Captured.bReady = false;
    TestTrue(TEXT("Unavailable does not leave old rows"),
        ETRelicPresentation::BuildRows(Captured, Inventory, Query, Rows) == EResult::Unavailable && Rows.IsEmpty());
    Query = {};
    Query.Filter = static_cast<EETRelicListFilter>(255);
    TestTrue(TEXT("Invalid enum rejected"),
        ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows) == EResult::InvalidQuery);
    TestTrue(TEXT("Projection never changes domain"),
        Inventory->GetSnapshot().Revision == DomainBefore.Revision && Inventory->GetSnapshot().Equipped == DomainBefore.Equipped);

    // 数据Item和未来池化Widget是不同对象。相同数据去重，解除监听后无旧回调。
    TStrongObjectPtr<UETRelicListItem> Item(NewObject<UETRelicListItem>());
    FETRelicListRow Row;
    Row.RelicId = LifeId;
    Row.DisplayName = FText::FromString(TEXT("测试名称"));
    Row.Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Tests/T_UnloadedIcon.T_UnloadedIcon")));
    int32 Changes = 0;
    const FDelegateHandle Handle = Item->OnChanged.AddLambda([&](const FETRelicListRow&) { ++Changes; });
    TestTrue(TEXT("Assign row"), Item->AssignRow(Row));
    TestFalse(TEXT("Same row deduplicated"), Item->AssignRow(Row));
    TestEqual(TEXT("One event"), Changes, 1);
    TestTrue(TEXT("Copy retains soft path without loading"),
        Item->GetSnapshot().Icon.ToSoftObjectPath() == Row.Icon.ToSoftObjectPath() && !Row.Icon.IsValid());
    Item->OnChanged.Remove(Handle);
    TestTrue(TEXT("Clear data identity"), Item->AssignRow({}));
    TestTrue(TEXT("Empty item cleared previous fields"), Item->GetSnapshot().RelicId.IsNone() &&
        Item->GetSnapshot().DisplayName.IsEmpty() && Item->GetSnapshot().Icon.IsNull());
    TestEqual(TEXT("No event to detached listener"), Changes, 1);
    // ---- 500条合成数据只进入本测试GI的定义表；真实拥有列表与装备不参与改写 ----
    constexpr int32 StressCount = 500;
    const auto StressId = [](const int32 Index)
    {
        return FName(*FString::Printf(TEXT("stress_relic_r%03d"), Index));
    };
    FETRelicInventorySnapshot Stress = DomainBefore;
    Stress.OwnedIds.Reset(StressCount);
    Stress.Equipped.SlotA = StressId(0);
    Stress.Equipped.SlotB = StressId(499);
    const FSoftObjectPath IconPath(TEXT("/Game/Tests/T_UnloadedStressIcon.T_UnloadedStressIcon"));
    for (int32 Index = StressCount - 1; Index >= 0; --Index)
    {
        const FName Id = StressId(Index);
        if (Inventory->Definitions.Contains(Id))
        {
            AddError(FString::Printf(TEXT("Synthetic relic ID already exists: %s"), *Id.ToString()));
            return false;
        }
        UETRelicDefinition* Definition = NewObject<UETRelicDefinition>(Inventory);
        Definition->RelicId = Id;
        Definition->DisplayName = FText::FromString(FString::Printf(TEXT("压测遗物 %03d"), Index));
        Definition->Description = FText::FromString(TEXT("临时列表压力样本"));
        Definition->SortOrder = Index % 5;
        Definition->AllowedSlots = {Index % 2 == 0 ? EETRelicSlot::First : EETRelicSlot::Second};
        Definition->Modifiers = {{EETRelicModifierKind::MaxHealth, 1.0f}};
        Definition->Icon = TSoftObjectPtr<UTexture2D>(IconPath);
        Inventory->Definitions.Add(Id, Definition);
        Stress.OwnedIds.Add(Id);
    }

    Query = {};
    if (!TestTrue(TEXT("Project 500 synthetic rows"),
                  ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success) ||
        !TestEqual(TEXT("Unfiltered stress count"), Rows.Num(), StressCount))
    {
        return false;
    }
    TestTrue(TEXT("Sort order ties break by stable ID"),
             Rows[0].RelicId == StressId(0) && Rows[1].RelicId == StressId(5) &&
             Rows.Last().RelicId == StressId(499));
    TestTrue(TEXT("Stress icons keep the soft path and stay unloaded"),
             Rows[0].Icon.ToSoftObjectPath() == IconPath && Rows[0].Icon.IsPending());

    Query.Sort = EETRelicListSort::StableId;
    TestTrue(TEXT("Stable sort ignores design-order groups"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == StressCount && Rows[1].RelicId == StressId(1) &&
             Rows.Last().RelicId == StressId(499));
    Query.SearchText = TEXT("  STRESS_RELIC_R499  ");
    TestTrue(TEXT("Trimmed ID search on 500 rows"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 1 && Rows[0].RelicId == StressId(499));
    Query.SearchText = TEXT("压测遗物 042");
    TestTrue(TEXT("Chinese display-name search on 500 rows"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 1 && Rows[0].RelicId == StressId(42));

    Query.SearchText.Reset();
    Query.Filter = EETRelicListFilter::CompatibleWithSlotA;
    TestTrue(TEXT("Slot A returns exactly the even sample set"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 250 && Rows[0].RelicId == StressId(0) &&
             Rows.Last().RelicId == StressId(498));
    Query.Filter = EETRelicListFilter::CompatibleWithSlotB;
    TestTrue(TEXT("Slot B returns exactly the odd sample set"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 250 && Rows[0].RelicId == StressId(1) &&
             Rows.Last().RelicId == StressId(499));
    Query.Filter = EETRelicListFilter::Equipped;
    TestTrue(TEXT("Equipped filter preserves both slot badges"),
             ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows) == EResult::Success &&
             Rows.Num() == 2 && Rows[0].RelicId == StressId(0) && Rows[0].bEquippedInSlotA &&
             Rows[1].RelicId == StressId(499) && Rows[1].bEquippedInSlotB);

    Query = {};
    Query.SearchText = TEXT("stress_relic_r499");
    FETRelicInventorySnapshot Broken = Stress;
    Broken.OwnedIds.Last() = FName(TEXT("missing_stress_definition"));
    TestTrue(TEXT("Search does not hide a missing definition in the 500-row sample"),
             ETRelicPresentation::BuildRows(Broken, Inventory, Query, Rows) ==
                 EResult::InvalidCatalog && Rows.IsEmpty());

    // 热路径取样只量BuildRows，不含构造500个定义、断言或Slate。
    constexpr int32 SampleCount = 30;
    double TotalMs = 0.0;
    double MaxMs = 0.0;
    for (int32 Index = 0; Index < SampleCount; ++Index)
    {
        const bool bSearch = Index % 2 != 0;
        Query.SearchText = bSearch ? TEXT("stress_relic_r499") : TEXT("");
        const double Start = FPlatformTime::Seconds();
        const EResult Result = ETRelicPresentation::BuildRows(Stress, Inventory, Query, Rows);
        const double ElapsedMs = (FPlatformTime::Seconds() - Start) * 1000.0;
        if (!TestTrue(TEXT("Repeated projection returns the expected count"),
                      Result == EResult::Success && Rows.Num() == (bSearch ? 1 : StressCount)))
        {
            return false;
        }
        TotalMs += ElapsedMs;
        MaxMs = FMath::Max(MaxMs, ElapsedMs);
    }
    AddInfo(FString::Printf(TEXT("[M3-06C1] Rows=%d Samples=%d MeanMs=%.3f MaxMs=%.3f"),
                           StressCount, SampleCount, TotalMs / SampleCount, MaxMs));
    const FETRelicInventorySnapshot DomainAfterStress = Inventory->GetSnapshot();
    TestTrue(TEXT("Stress leaves session inventory untouched"),
             DomainAfterStress.OwnedIds == DomainBefore.OwnedIds &&
             DomainAfterStress.Equipped == DomainBefore.Equipped &&
             DomainAfterStress.Revision == DomainBefore.Revision);
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif