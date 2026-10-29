#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicViewModels.h"
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
    const FETRelicInventorySnapshot Original = Inventory->GetSnapshot();
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    FETRelicListQuery Query;
    TArray<FETRelicListRow> Rows;
    if (!TestTrue(TEXT("Default rows succeed"),
                  ETRelicPresentation::BuildRows(Original, Inventory, Query, Rows) == EResult::Success) ||
        !TestEqual(TEXT("Two actual owned items"), Rows.Num(), 2))
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
        Inventory->GetSnapshot().Revision == Original.Revision && Inventory->GetSnapshot().Equipped == Original.Equipped);

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
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif