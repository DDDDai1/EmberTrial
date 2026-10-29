#include "Widgets/ETRelicPanelWidget.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Inventory/ETRelicPresenter.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Templates/UnrealTemplate.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicPanel, Log, All);

bool UETRelicPanelWidget::HasRequiredLayout() const
{
    return RelicList && InventoryStatusText && DetailNameText && DetailDescriptionText && PreviewText &&
           SlotText && SlotAButton && SlotBButton;
}

void UETRelicPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!HasRequiredLayout())
    {
        UE_LOG(LogETRelicPanel, Warning, TEXT("[M3-03B] Missing required panel layout Widget=%s"), *GetName());
        return;
    }
    SlotAButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotA);
    SlotBButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotB);
    RelicList->OnItemSelectionChanged().Remove(SelectionHandle);
    SelectionHandle = RelicList->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
    if (!Presenter)
    {
        Presenter = NewObject<UETRelicPresenter>(this);
    }
    Presenter->OnChanged.Remove(PresenterHandle);
    PresenterHandle = Presenter->OnChanged.AddUObject(this, &ThisClass::Render);
    ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    Presenter->StartObserving(LocalPlayer ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>() : nullptr);
    Render(Presenter->GetSnapshot());
    UE_LOG(LogETRelicPanel, Display, TEXT("[M3-03B] Panel subscribed Widget=%s"), *GetName());
}

void UETRelicPanelWidget::NativeDestruct()
{
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
    TArray<UObject*> Objects;
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
    InventoryStatusText->SetText(FText::FromString(Data.State == EETRelicPanelState::Ready
        ? TEXT("遗物 · 只读查看") : TEXT("当前身体或装备尚不可预览")));
    DetailNameText->SetText(Data.SelectedId.IsNone() ? FText::FromString(TEXT("没有可显示的遗物")) : Data.Name);
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