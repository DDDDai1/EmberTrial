#include "Widgets/ETRelicPanelWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
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
           SlotText && SlotAButton && SlotBButton && SearchBox && FilterCombo && SortCombo;
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
    SlotAButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotA);
    SlotBButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotB);
    RelicList->OnItemSelectionChanged().Remove(SelectionHandle);
    SelectionHandle = RelicList->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
    Presenter->OnChanged.Remove(PresenterHandle);
    PresenterHandle = Presenter->OnChanged.AddUObject(this, &ThisClass::Render);
    ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    Presenter->StartObserving(LocalPlayer ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>() : nullptr);
    Render(Presenter->GetSnapshot());
    UE_LOG(LogETRelicPanel, Display, TEXT("[M3-03B] Panel subscribed Widget=%s"), *GetName());
}

void UETRelicPanelWidget::NativeDestruct()
{
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
        ? NSLOCTEXT("ETRelics", "ReadyCount", "遗物 · 只读查看（{0} 件）")
        : NSLOCTEXT("ETRelics", "WaitingCount", "遗物（{0} 件）· 当前身体尚不可预览"), Count));
    DetailNameText->SetText(Data.SelectedId.IsNone()
        ? NSLOCTEXT("ETRelics", "NoMatches", "没有符合条件的遗物") : Data.Name);
    DetailDescriptionText->SetText(Data.Description);
    SlotText->SetText(FText::FromString(Data.bSlotB ? TEXT("比较目标：槽 B") : TEXT("比较目标：槽 A")));
    if (Data.PreviewState == EETRelicPreviewState::Available ||
        Data.PreviewState == EETRelicPreviewState::AlreadyEquipped)
    {
        const FNumberFormattingOptions NumberFormat = FNumberFormattingOptions().SetMaximumFractionalDigits(1);
        PreviewText->SetText(FText::Format(FText::FromString(TEXT("最大生命 {0} → {1}\n首段轻击 {2} → {3}{4}")),
            FText::AsNumber(Data.BeforeMax, &NumberFormat), FText::AsNumber(Data.AfterMax, &NumberFormat),
            FText::AsNumber(Data.BeforeAttack, &NumberFormat), FText::AsNumber(Data.AfterAttack, &NumberFormat),
            Data.PreviewState == EETRelicPreviewState::AlreadyEquipped
                ? FText::FromString(TEXT("\n该槽已装备此遗物")) : FText::GetEmpty()));
    }
    else
    {
        PreviewText->SetText(FText::FromString(Data.PreviewState == EETRelicPreviewState::Rejected
            ? TEXT("此方案不符合装备规则，例如同一遗物不能占两槽。") : TEXT("当前没有可用的属性比较。")));
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