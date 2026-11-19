#include "Widgets/ETRelicPanelWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/VerticalBox.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Inventory/ETRelicPresenter.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Templates/UnrealTemplate.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicPanel, Log, All);

bool UETRelicPanelWidget::HasRequiredLayout() const
{
    return RelicList && InventoryStatusText && DetailNameText && DetailDescriptionText && PreviewText &&
           SlotText && SlotAButton && SlotBButton && SearchBox && FilterCombo && SortCombo &&
           CommandContent && ConfirmEquipButton && UnequipButton && CommandResultText;
}

void UETRelicPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!HasRequiredLayout())
    {
        UE_LOG(LogETRelicPanel, Warning, TEXT("[M3-03B] Missing required panel layout Widget=%s"), *GetName());
        return;
    }
    if (!Presenter)
    {
        Presenter = NewObject<UETRelicPresenter>(this);
    }
    InitializeQueryControls();
    SearchBox->OnTextChanged.AddUniqueDynamic(this, &ThisClass::HandleSearchChanged);
    FilterCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleQuerySelectionChanged);
    SortCombo->OnSelectionChanged.AddUniqueDynamic(this, &ThisClass::HandleQuerySelectionChanged);
    ConfirmEquipButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
    UnequipButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleUnequip);
    SlotAButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotA);
    SlotBButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotB);
    RelicList->OnItemSelectionChanged().Remove(SelectionHandle);
    SelectionHandle = RelicList->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
    Presenter->OnChanged.Remove(PresenterHandle);
    PresenterHandle = Presenter->OnChanged.AddUObject(this, &ThisClass::Render);
    ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    Presenter->StartObserving(LocalPlayer ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>() : nullptr);
    Presenter->SetCommands(CanSubmitCommand, SubmitCommand);
    Render(Presenter->GetSnapshot());
    UE_LOG(LogETRelicPanel, Display, TEXT("[M3-03B] Panel subscribed Widget=%s"), *GetName());
}

void UETRelicPanelWidget::NativeDestruct()
{
    CanSubmitCommand.Unbind();
    SubmitCommand.Unbind();
    if (ConfirmEquipButton)
    {
        ConfirmEquipButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleConfirm);
    }
    if (UnequipButton)
    {
        UnequipButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleUnequip);
    }
    // 先断开输入意图，再结束观察；不让关闭期间的控件回调重新进入Presenter。
    if (SearchBox)
    {
        SearchBox->OnTextChanged.RemoveDynamic(this, &ThisClass::HandleSearchChanged);
    }
    if (FilterCombo)
    {
        FilterCombo->OnSelectionChanged.RemoveDynamic(this, &ThisClass::HandleQuerySelectionChanged);
    }
    if (SortCombo)
    {
        SortCombo->OnSelectionChanged.RemoveDynamic(this, &ThisClass::HandleQuerySelectionChanged);
    }
    if (Presenter)
    {
        Presenter->OnChanged.Remove(PresenterHandle);
        Presenter->SetCommands({}, {});
        Presenter->Shutdown();
    }
    PresenterHandle.Reset();
    if (RelicList)
    {
        RelicList->OnItemSelectionChanged().Remove(SelectionHandle);
        RelicList->ClearListItems();
    }
    SelectionHandle.Reset();
    if (SlotAButton)
    {
        SlotAButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlotA);
    }
    if (SlotBButton)
    {
        SlotBButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlotB);
    }
    UE_LOG(LogETRelicPanel, Display, TEXT("[M3-03B] Panel unsubscribed Widget=%s"), *GetName());
    Super::NativeDestruct();
}

void UETRelicPanelWidget::HandleSelectionChanged(UObject* Item)
{
    if (!bRendering && Presenter)
    {
        if (const UETRelicListItem* Relic = Cast<UETRelicListItem>(Item))
        {
            Presenter->SelectId(Relic->GetSnapshot().RelicId);
        }
    }
}

void UETRelicPanelWidget::HandleSlotA()
{
    if (Presenter)
    {
        Presenter->SetPreviewSlotB(false);
    }
}

void UETRelicPanelWidget::HandleSlotB()
{
    if (Presenter)
    {
        Presenter->SetPreviewSlotB(true);
    }
}

// ---- 渲染不回发选择意图；只有集合 / 顺序改变才重建ListView的数据集合 ----
void UETRelicPanelWidget::Render(const FETRelicPanelData& Data)
{
    if (!HasRequiredLayout() || !Presenter || bRendering)
    {
        return;
    }
    TGuardValue<bool> Guard(bRendering, true);
    SyncQueryControls();
    TArray<UObject*> Objects;
    Objects.Reserve(Presenter->GetItems().Num());
    for (UETRelicListItem* Item : Presenter->GetItems())
    {
        Objects.Add(Item);
    }
    if (RelicList->GetListItems() != Objects)
    {
        RelicList->SetListItems(Objects);
    }
    UETRelicListItem* Selected = Presenter->FindItem(Data.SelectedId);
    if (RelicList->GetSelectedItem() != Selected)
    {
        if (Selected)
        {
            RelicList->SetSelectedItem(Selected);
        }
        else
        {
            RelicList->ClearSelection();
        }
    }
    const FText Count = FText::AsNumber(Presenter->GetItems().Num());
    InventoryStatusText->SetText(FText::Format(Data.State == EETRelicPanelState::Ready
        ? (Data.bHasCommands ? NSLOCTEXT("ETRelics", "AltarCount", "遗物祭坛（{0} 件）")
                             : NSLOCTEXT("ETRelics", "ReadyCount", "遗物 · 只读查看（{0} 件）"))
        : NSLOCTEXT("ETRelics", "WaitingCount", "遗物（{0} 件）· 当前身体尚不可预览"), Count));
    DetailNameText->SetText(Data.SelectedId.IsNone()
        ? NSLOCTEXT("ETRelics", "NoMatches", "没有符合条件的遗物") : Data.Name);
    DetailDescriptionText->SetText(Data.Description);
    SlotText->SetText(FText::FromString(Data.bSlotB ? TEXT("比较槽 B") : TEXT("比较槽 A")));
    ShowSlot(SlotAButton, TEXT("槽 A"), Data.SlotAName, !Data.bSlotB);
    ShowSlot(SlotBButton, TEXT("槽 B"), Data.SlotBName, Data.bSlotB);
    CommandContent->SetVisibility(Data.bHasCommands ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ConfirmEquipButton->SetIsEnabled(!bCommandBusy && Data.bCanEquip);
    UnequipButton->SetIsEnabled(!bCommandBusy && Data.bCanUnequip);
    CommandResultText->SetText(Data.CommandMessage.IsEmpty()
        ? FText::FromString(TEXT("选择目标槽位与遗物后确认。")) : Data.CommandMessage);
    const bool bShowStats = Data.PreviewState == EETRelicPreviewState::Available ||
        Data.PreviewState == EETRelicPreviewState::AlreadyEquipped;
    if (ComparisonGrid)
    {
        ComparisonGrid->SetVisibility(bShowStats ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        if (bShowStats)
        {
            const float Before[] = {Data.BeforeMax, Data.BeforeAttack, Data.BeforeHealingBonus,
                Data.BeforeStaminaRecoveryBonus, Data.BeforeBreakPulsePoiseBonus};
            const float After[] = {Data.AfterMax, Data.AfterAttack, Data.AfterHealingBonus,
                Data.AfterStaminaRecoveryBonus, Data.AfterBreakPulsePoiseBonus};
            FNumberFormattingOptions Format;
            Format.SetMaximumFractionalDigits(1);
            // Row zero holds the headings. The first column holds the stat labels.
            for (int32 Index = 0; Index < ComparisonGrid->GetChildrenCount(); ++Index)
            {
                UTextBlock* Cell = Cast<UTextBlock>(ComparisonGrid->GetChildAt(Index));
                UGridSlot* CellSlot = Cell ? Cast<UGridSlot>(Cell->Slot) : nullptr;
                if (!CellSlot) continue;
                const int32 Row = CellSlot->GetRow() - 1;
                const int32 Col = CellSlot->GetColumn();
                if (Row < 0 || Row >= UE_ARRAY_COUNT(Before) || Col < 1 || Col > 3) continue;
                const float Delta = After[Row] - Before[Row];
                const float Value = Col == 1 ? Before[Row] : Col == 2 ? After[Row] : Delta;
                FText Text = FText::AsNumber(Value, &Format);
                if (Col == 3)
                {
                    if (FMath::IsNearlyZero(Delta)) Text = FText::FromString(TEXT("—"));
                    else if (Delta > 0) Text = FText::Format(INVTEXT("+{0}"), Text);
                    const FColor Tint = FMath::IsNearlyZero(Delta) ? FColor(189,182,171) :
                        Delta > 0 ? FColor(178,202,141) : FColor(229,161,140);
                    Cell->SetColorAndOpacity(FSlateColor(FLinearColor::FromSRGBColor(Tint)));
                }
                Cell->SetText(Text);
            }
        }
    }
    const bool bMessage = Data.PreviewState != EETRelicPreviewState::Available;
    PreviewText->SetVisibility(bMessage ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (bMessage)
        PreviewText->SetText(FText::FromString(bShowStats ? TEXT("该槽已装备此遗物") :
            Data.PreviewState == EETRelicPreviewState::Rejected ? TEXT("此方案不符合装备规则。") :
            TEXT("当前没有可用的属性比较。")));
}

void UETRelicPanelWidget::ShowSlot(UButton* Button, const TCHAR* Prefix, const FText& Item, bool bSelected)
{
    if (auto* Label = Cast<UTextBlock>(Button->GetContent()))
    {
        Label->SetText(FText::Format(INVTEXT("{0}  {1}"), FText::FromString(Prefix),
            Item.IsEmpty() ? INVTEXT("未装备") : Item));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::FromSRGBColor(
            bSelected ? FColor(242,200,135) : FColor(189,182,171))));
    }
}
// ---- 原生页面命令桥；View只发意图，不能自己Apply GE ----
void UETRelicPanelWidget::SetCommands(FETRelicCanSubmit CanSubmit, FETRelicSubmit Submit)
{
    CanSubmitCommand = MoveTemp(CanSubmit);
    SubmitCommand = MoveTemp(Submit);
    if (Presenter)
    {
        Presenter->SetCommands(CanSubmitCommand, SubmitCommand);
    }
}

void UETRelicPanelWidget::SetCommandBusy(const bool bBusy)
{
    bCommandBusy = bBusy;
    if (Presenter)
    {
        Render(Presenter->GetSnapshot());
    }
}

void UETRelicPanelWidget::HandleConfirm()
{
    if (!bCommandBusy && Presenter)
    {
        Presenter->ConfirmSelected();
    }
}

void UETRelicPanelWidget::HandleUnequip()
{
    if (!bCommandBusy && Presenter)
    {
        Presenter->UnequipPreviewSlot();
    }
}

// ---- 查询控件：显示文字只用于呈现，选项索引在View内显式转换为查询枚举 ----
void UETRelicPanelWidget::InitializeQueryControls()
{
    TGuardValue<bool> Guard(bSyncingQueryControls, true);
    // 提交文本保留编辑焦点；Esc由本Panel显式转移，不回滚已经应用的搜索。
    SearchBox->SetClearKeyboardFocusOnCommit(false);
    SearchBox->SetSelectAllTextOnCommit(false);
    SearchBox->SetRevertTextOnEscape(false);
    FilterCombo->ClearOptions();
    FilterCombo->AddOption(NSLOCTEXT("ETRelics", "FilterAll", "全部").ToString());
    FilterCombo->AddOption(NSLOCTEXT("ETRelics", "FilterA", "可用于槽 A").ToString());
    FilterCombo->AddOption(NSLOCTEXT("ETRelics", "FilterB", "可用于槽 B").ToString());
    FilterCombo->AddOption(NSLOCTEXT("ETRelics", "FilterEquipped", "已装备").ToString());
    SortCombo->ClearOptions();
    SortCombo->AddOption(NSLOCTEXT("ETRelics", "SortCatalog", "设计顺序").ToString());
    SortCombo->AddOption(NSLOCTEXT("ETRelics", "SortId", "ID 顺序").ToString());
    SyncQueryControls();
}

void UETRelicPanelWidget::SyncQueryControls()
{
    if (!HasRequiredLayout() || !Presenter)
    {
        return;
    }
    TGuardValue<bool> Guard(bSyncingQueryControls, true);
    const FETRelicListQuery& Query = Presenter->GetQuery();
    int32 FilterIndex = 0;
    switch (Query.Filter)
    {
    case EETRelicListFilter::CompatibleWithSlotA: FilterIndex = 1; break;
    case EETRelicListFilter::CompatibleWithSlotB: FilterIndex = 2; break;
    case EETRelicListFilter::Equipped: FilterIndex = 3; break;
    default: break;
    }
    const int32 SortIndex = Query.Sort == EETRelicListSort::StableId ? 1 : 0;
    if (FilterCombo->GetSelectedIndex() != FilterIndex)
    {
        FilterCombo->SetSelectedIndex(FilterIndex);
    }
    if (SortCombo->GetSelectedIndex() != SortIndex)
    {
        SortCombo->SetSelectedIndex(SortIndex);
    }
    // 相同内容不SetText，避免属性刷新打断输入法或重置编辑光标。
    if (SearchBox->GetText().ToString() != Query.SearchText)
    {
        SearchBox->SetText(FText::FromString(Query.SearchText));
    }
}

void UETRelicPanelWidget::ApplyQueryFromControls()
{
    if (bSyncingQueryControls || bRendering || !HasRequiredLayout() || !Presenter)
    {
        return;
    }
    FETRelicListQuery Query = Presenter->GetQuery();
    switch (FilterCombo->GetSelectedIndex())
    {
    case 0: Query.Filter = EETRelicListFilter::All; break;
    case 1: Query.Filter = EETRelicListFilter::CompatibleWithSlotA; break;
    case 2: Query.Filter = EETRelicListFilter::CompatibleWithSlotB; break;
    case 3: Query.Filter = EETRelicListFilter::Equipped; break;
    default: return;
    }
    switch (SortCombo->GetSelectedIndex())
    {
    case 0: Query.Sort = EETRelicListSort::CatalogOrder; break;
    case 1: Query.Sort = EETRelicListSort::StableId; break;
    default: return;
    }
    Query.SearchText = SearchBox->GetText().ToString();
    Presenter->SetQuery(Query);
    // 不同条件可能得到完全相同的行，此时Presenter无需广播；View仍立即同步显示。
    Render(Presenter->GetSnapshot());
}

void UETRelicPanelWidget::HandleSearchChanged(const FText& Text)
{
    ApplyQueryFromControls();
}

void UETRelicPanelWidget::HandleQuerySelectionChanged(FString SelectedItem,
                                                    ESelectInfo::Type SelectionType)
{
    ApplyQueryFromControls();
}

// ---- 文本焦点：P作为字符保留在搜索框；Esc只离开编辑，不恢复世界 ----
bool UETRelicPanelWidget::IsSearchFocused() const
{
    return SearchBox && (SearchBox->HasKeyboardFocus() || SearchBox->HasFocusedDescendants());
}

FReply UETRelicPanelWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry,
                                                 const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Escape && IsSearchFocused() && SlotAButton && GetOwningPlayer())
    {
        SlotAButton->SetUserFocus(GetOwningPlayer());
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UETRelicPanelWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::P && IsSearchFocused())
    {
        // 截断向外层PauseWidget的KeyUp冒泡，其余按键交还正常UMG路径。
        return FReply::Handled();
    }
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}