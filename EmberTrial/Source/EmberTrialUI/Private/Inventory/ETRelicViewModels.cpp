#include "Inventory/ETRelicViewModels.h"
#include "Inventory/ETRelicDefinition.h"
#include "Inventory/ETRelicInventorySubsystem.h"

bool FETRelicListRow::operator==(const FETRelicListRow& Other) const
{
    return RelicId == Other.RelicId && DisplayName.EqualTo(Other.DisplayName) &&
           Description.EqualTo(Other.Description) && Icon == Other.Icon && SortOrder == Other.SortOrder &&
           bSupportsSlotA == Other.bSupportsSlotA && bSupportsSlotB == Other.bSupportsSlotB &&
           bEquippedInSlotA == Other.bEquippedInSlotA && bEquippedInSlotB == Other.bEquippedInSlotB;
}

// ---- 完整投影后再排序 / 筛选；过滤不能掩盖损坏的拥有数据 ----
EETRelicRowsResult ETRelicPresentation::BuildRows(
    const FETRelicInventorySnapshot& InventorySnapshot, const UETRelicInventorySubsystem* Catalog,
    const FETRelicListQuery& Query, TArray<FETRelicListRow>& OutRows)
{
    OutRows.Reset();
    if (!InventorySnapshot.bReady || !IsValid(Catalog) || !Catalog->GetSnapshot().bReady)
    {
        return EETRelicRowsResult::Unavailable;
    }
    if ((Query.Filter != EETRelicListFilter::All &&
         Query.Filter != EETRelicListFilter::CompatibleWithSlotA &&
         Query.Filter != EETRelicListFilter::CompatibleWithSlotB &&
         Query.Filter != EETRelicListFilter::Equipped) ||
        (Query.Sort != EETRelicListSort::CatalogOrder && Query.Sort != EETRelicListSort::StableId))
    {
        return EETRelicRowsResult::InvalidQuery;
    }
    if (InventorySnapshot.Revision == 0)
    {
        return EETRelicRowsResult::InvalidSnapshot;
    }

    TSet<FName> Owned;
    ETRelicLoadoutRules::FDefinitionMap Definitions;
    TArray<FETRelicListRow> Rows;
    Rows.Reserve(InventorySnapshot.OwnedIds.Num());
    for (const FName Id : InventorySnapshot.OwnedIds)
    {
        if (Id.IsNone() || Owned.Contains(Id))
        {
            return EETRelicRowsResult::InvalidSnapshot;
        }
        const UETRelicDefinition* Definition = Catalog->FindDefinition(Id);
        TArray<FText> Errors;
        if (!IsValid(Definition) || Definition->GetRelicId() != Id ||
            !Definition->CollectValidationErrors(Errors))
        {
            return EETRelicRowsResult::InvalidCatalog;
        }
        Owned.Add(Id);
        Definitions.Add(Id, Definition);
        FETRelicListRow& Row = Rows.AddDefaulted_GetRef();
        Row.RelicId = Id;
        Row.DisplayName = Definition->GetDisplayName();
        Row.Description = Definition->GetDescription();
        Row.Icon = Definition->GetIcon();
        Row.SortOrder = Definition->GetSortOrder();
        Row.bSupportsSlotA = Definition->SupportsSlot(EETRelicSlot::First);
        Row.bSupportsSlotB = Definition->SupportsSlot(EETRelicSlot::Second);
        Row.bEquippedInSlotA = InventorySnapshot.Equipped.SlotA == Id;
        Row.bEquippedInSlotB = InventorySnapshot.Equipped.SlotB == Id;
    }
    FETRelicModifierTotals IgnoredTotals;
    if (ETRelicLoadoutRules::Evaluate(InventorySnapshot.Equipped, Owned, Definitions, IgnoredTotals) !=
        EETRelicLoadoutResult::Success)
    {
        return EETRelicRowsResult::InvalidSnapshot;
    }

    const FString Search = Query.SearchText.TrimStartAndEnd();
    Rows.RemoveAll([&](const FETRelicListRow& Row)
    {
        const bool bCompatible =
            (Query.Filter != EETRelicListFilter::CompatibleWithSlotA || Row.bSupportsSlotA) &&
            (Query.Filter != EETRelicListFilter::CompatibleWithSlotB || Row.bSupportsSlotB) &&
            (Query.Filter != EETRelicListFilter::Equipped || Row.bEquippedInSlotA || Row.bEquippedInSlotB);
        const bool bMatchesSearch = Search.IsEmpty() ||
            Row.RelicId.ToString().Contains(Search, ESearchCase::IgnoreCase) ||
            Row.DisplayName.ToString().Contains(Search, ESearchCase::IgnoreCase);
        return !bCompatible || !bMatchesSearch;
    });
    Rows.Sort([&](const FETRelicListRow& A, const FETRelicListRow& B)
    {
        if (Query.Sort == EETRelicListSort::CatalogOrder && A.SortOrder != B.SortOrder)
        {
            return A.SortOrder < B.SortOrder;
        }
        // 相同SortOrder时也使用稳定ID破同分，避免依赖TMap遍历或本地化文本。
        return A.RelicId.LexicalLess(B.RelicId);
    });
    OutRows = MoveTemp(Rows);
    return EETRelicRowsResult::Success;
}

// ---- 数据对象更新；View先解绑旧Item再绑定新Item，不能等待GC代替解绑 ----
bool UETRelicListItem::AssignRow(const FETRelicListRow& NewRow)
{
    if (Row == NewRow)
    {
        return false;
    }
    Row = NewRow;
    // 向同轮观察者发送值快照；观察者不能通过引用修改本对象的Row。
    const FETRelicListRow Published = Row;
    OnChanged.Broadcast(Published);
    return true;
}