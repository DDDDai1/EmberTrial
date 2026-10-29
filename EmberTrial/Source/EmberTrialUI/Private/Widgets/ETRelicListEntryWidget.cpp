#include "Widgets/ETRelicListEntryWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Inventory/ETRelicViewModels.h"

void UETRelicListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    UnbindItem();
    BoundItem = Cast<UETRelicListItem>(ListItemObject);
    if (UETRelicListItem* Item = BoundItem.Get())
    {
        ItemHandle = Item->OnChanged.AddUObject(this, &ThisClass::HandleRowChanged, BoundItem);
        RenderRow(Item->GetSnapshot());
    }
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
}

void UETRelicListEntryWidget::NativeOnItemSelectionChanged(const bool bIsSelected)
{
    if (SelectionBorder)
    {
        SelectionBorder->SetBrushColor(bIsSelected ? FLinearColor(.22f, .16f, .07f, 1.0f)
                                                  : FLinearColor(.055f, .055f, .055f, 1.0f));
    }
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
}

void UETRelicListEntryWidget::NativeOnEntryReleased()
{
    UnbindItem();
    NativeOnItemSelectionChanged(false);
    IUserObjectListEntry::NativeOnEntryReleased();
}

void UETRelicListEntryWidget::NativeDestruct()
{
    UnbindItem();
    Super::NativeDestruct();
}

void UETRelicListEntryWidget::UnbindItem()
{
    if (UETRelicListItem* Item = BoundItem.Get())
    {
        Item->OnChanged.Remove(ItemHandle);
    }
    ItemHandle.Reset();
    BoundItem.Reset();
    RenderRow({});
}

void UETRelicListEntryWidget::HandleRowChanged(const FETRelicListRow& Row,
    TWeakObjectPtr<UETRelicListItem> Source)
{
    if (Source.IsValid() && Source == BoundItem)
    {
        RenderRow(Row);
    }
}

void UETRelicListEntryWidget::RenderRow(const FETRelicListRow& Row)
{
    if (ItemNameText)
    {
        ItemNameText->SetText(Row.DisplayName);
    }
    if (EquippedText)
    {
        EquippedText->SetText(FText::FromString(Row.bEquippedInSlotA ? TEXT("槽 A 已装备") :
            Row.bEquippedInSlotB ? TEXT("槽 B 已装备") : TEXT("未装备")));
        if (Row.RelicId.IsNone())
        {
            EquippedText->SetText(FText::GetEmpty());
        }
    }
}