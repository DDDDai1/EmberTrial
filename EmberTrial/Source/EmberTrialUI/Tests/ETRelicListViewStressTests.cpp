#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformTime.h"
#include "Inventory/ETRelicViewModels.h"
#include "Layout/Geometry.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/ETRelicListEntryWidget.h"
#include "Widgets/SWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicListViewStressTest,
    "EmberTrial.UI.Relics.ListViewStress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicListViewStressTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    if (!TestTrue(TEXT("Slate is initialized in the editor"), FSlateApplication::IsInitialized()))
    {
        return false;
    }
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    UClass* PanelClass = LoadClass<UUserWidget>(nullptr, TEXT(
        "/Game/EmberTrial/ui/Inventory/WBP_ETRelicPanel.WBP_ETRelicPanel_C"));
    if (!TestNotNull(TEXT("Production relic panel Blueprint"), PanelClass))
    {
        return false;
    }

    // 数据只进入测试列表。生产Presenter、库存、装备及地图保持原路径。
    constexpr int32 ItemCount = 500;
    TArray<TStrongObjectPtr<UETRelicListItem>> ItemOwners;
    TArray<UETRelicListItem*> Items;
    ItemOwners.Reserve(ItemCount);
    Items.Reserve(ItemCount);
    for (int32 Index = 0; Index < ItemCount; ++Index)
    {
        UETRelicListItem* Item = NewObject<UETRelicListItem>();
        FETRelicListRow Row;
        Row.RelicId = FName(*FString::Printf(TEXT("stress_row_r%03d"), Index));
        Row.DisplayName = FText::FromString(FString::Printf(TEXT("压测遗物 %03d"), Index));
        Row.bEquippedInSlotA = Index == 0;
        Row.bEquippedInSlotB = Index == ItemCount - 1;
        Item->AssignRow(Row);
        ItemOwners.Emplace(Item);
        Items.Add(Item);
    }

    // 用固定几何驱动真实的Slate ListView。采样只包含布局预处理与列表Tick。
    const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(300.0, 360.0), FSlateLayoutTransform());
    constexpr float DeltaTime = 1.0f / 60.0f;
    int32 Samples = 0;
    double TotalMs = 0.0;
    double MaxMs = 0.0;
    int32 OverallPeakDisplayed = 0;
    int32 MaxUniquePerPanel = 0;
    int32 ReusedAssignments = 0;

    for (int32 Pass = 0; Pass < 2; ++Pass)
    {
        TStrongObjectPtr<UUserWidget> Panel(CreateWidget<UUserWidget>(World, PanelClass));
        if (!TestNotNull(TEXT("Create production panel instance"), Panel.Get()))
        {
            return false;
        }
        UListView* List = Cast<UListView>(Panel->GetWidgetFromName(TEXT("RelicList")));
        if (!TestNotNull(TEXT("Real panel list"), List) ||
            !TestTrue(TEXT("Real list uses the production entry widget"),
                      List->GetEntryWidgetClass() &&
                      List->GetEntryWidgetClass()->IsChildOf(UETRelicListEntryWidget::StaticClass())))
        {
            return false;
        }
        const TSharedRef<SWidget> SlateList = List->TakeWidget();
        TMap<UUserWidget*, UETRelicListItem*> PreviousBindings;
        TMap<UUserWidget*, UETRelicListItem*> SeenBindings;
        UUserWidget* HighlightedEntry = nullptr;
        FLinearColor Unselected;
        bool bReusedHighlightedEntry = false;
        auto TickList = [&]()
        {
            const double Start = FPlatformTime::Seconds();
            SlateList->SlatePrepass(1.0f);
            SlateList->Tick(Geometry, Samples * static_cast<double>(DeltaTime), DeltaTime);
            const double ElapsedMs = (FPlatformTime::Seconds() - Start) * 1000.0;
            ++Samples;
            TotalMs += ElapsedMs;
            MaxMs = FMath::Max(MaxMs, ElapsedMs);
        };
        auto CheckVisible = [&]()
        {
            const TArray<UUserWidget*>& Entries = List->GetDisplayedEntryWidgets();
            OverallPeakDisplayed = FMath::Max(OverallPeakDisplayed, Entries.Num());
            if (!TestTrue(TEXT("Visible entries remain virtualized"), Entries.Num() > 0 && Entries.Num() < 50))
            {
                return false;
            }
            TMap<UUserWidget*, UETRelicListItem*> CurrentBindings;
            for (UUserWidget* Entry : Entries)
            {
                const IUserObjectListEntry* Interface = Cast<IUserObjectListEntry>(Entry);
                UETRelicListItem* Item = Interface ? Interface->GetListItem<UETRelicListItem>() : nullptr;
                const UTextBlock* Name = Cast<UTextBlock>(Entry->GetWidgetFromName(TEXT("ItemNameText")));
                const UTextBlock* Badge = Cast<UTextBlock>(Entry->GetWidgetFromName(TEXT("EquippedText")));
                if (!TestTrue(TEXT("Displayed widget has a live item and bound labels"),
                              IsValid(Item) && IsValid(Name) && IsValid(Badge)))
                {
                    return false;
                }
                const FETRelicListRow Row = Item->GetSnapshot();
                const FString ExpectedBadge = Row.bEquippedInSlotA ? TEXT("槽 A 已装备") :
                    Row.bEquippedInSlotB ? TEXT("槽 B 已装备") : TEXT("未装备");
                TestTrue(TEXT("Recycled row name matches its current item"), Name->GetText().EqualTo(Row.DisplayName));
                TestEqual(TEXT("Recycled badge matches its current item"), Badge->GetText().ToString(), ExpectedBadge);
                TestTrue(TEXT("Current row observes its item"), Item->OnChanged.IsBoundToObject(Entry));
                if (UETRelicListItem** Seen = SeenBindings.Find(Entry); Seen && *Seen != Item)
                {
                    ++ReusedAssignments;
                    TestFalse(TEXT("Reused widget removed the previous item delegate"),
                              (*Seen)->OnChanged.IsBoundToObject(Entry));
                }
                if (Entry == HighlightedEntry && Item != Items[0])
                {
                    bReusedHighlightedEntry = true;
                    const UBorder* Border = Cast<UBorder>(Entry->GetWidgetFromName(TEXT("SelectionBorder")));
                    TestTrue(TEXT("Reused selected row clears its old highlight"),
                             IsValid(Border) && Border->GetBrushColor().Equals(Unselected));
                }
                SeenBindings.Add(Entry, Item);
                CurrentBindings.Add(Entry, Item);
            }
            for (const auto& Previous : PreviousBindings)
            {
                if (CurrentBindings.FindRef(Previous.Key) != Previous.Value)
                {
                    TestFalse(TEXT("Offscreen item no longer notifies the released widget"),
                              Previous.Value->OnChanged.IsBoundToObject(Previous.Key));
                }
            }
            PreviousBindings = MoveTemp(CurrentBindings);
            return true;
        };

        List->SetListItems(Items);
        TickList();
        if (!CheckVisible())
        {
            return false;
        }
        TestEqual(TEXT("List contains all 500 items"), List->GetNumItems(), ItemCount);
        UUserWidget* FirstEntry = List->GetEntryWidgetFromItem(Items[0]);
        const UBorder* FirstBorder = FirstEntry
            ? Cast<UBorder>(FirstEntry->GetWidgetFromName(TEXT("SelectionBorder"))) : nullptr;
        if (!TestNotNull(TEXT("First visible row selection border"), FirstBorder))
        {
            return false;
        }
        IUserListEntry::UpdateItemSelection(*FirstEntry, false);
        Unselected = FirstBorder->GetBrushColor();
        List->SetSelectedIndex(0);
        TestTrue(TEXT("First data item selected in Slate"),
                 CastChecked<IUserObjectListEntry>(FirstEntry)->IsListItemSelected());
        // 手动Tick列表不绘制子行，显式发送与行活动计时器相同的选中通知。
        IUserListEntry::UpdateItemSelection(*FirstEntry, true);
        TestTrue(TEXT("Selection highlights the first row"), !FirstBorder->GetBrushColor().Equals(Unselected));
        HighlightedEntry = FirstEntry;

        for (int32 Offset = 40; Offset < ItemCount; Offset += 40)
        {
            List->SetScrollOffset(static_cast<float>(Offset));
            TickList();
            if (!CheckVisible())
            {
                return false;
            }
        }
        List->ScrollToBottom();
        TickList();
        if (!CheckVisible() ||
            !TestNotNull(TEXT("Last data item becomes visible"), List->GetEntryWidgetFromItem(Items.Last())))
        {
            return false;
        }
        List->ScrollToTop();
        TickList();
        if (!CheckVisible())
        {
            return false;
        }
        TestNotNull(TEXT("First item visible again"), List->GetEntryWidgetFromItem(Items[0]));
        TestTrue(TEXT("The previously highlighted entry was recycled"), bReusedHighlightedEntry);
        MaxUniquePerPanel = FMath::Max(MaxUniquePerPanel, SeenBindings.Num());
        TestTrue(TEXT("Widget pool stays far smaller than data set"), SeenBindings.Num() < 100);

        List->ClearListItems();
        TickList();
        TestTrue(TEXT("Cleared list releases all displayed widgets"), List->GetDisplayedEntryWidgets().IsEmpty());
        TestTrue(TEXT("All data delegates detach before creating the next panel"),
                 !Items.ContainsByPredicate([](const UETRelicListItem* Item)
                 {
                     return Item->OnChanged.IsBound();
                 }));
    }

    TestTrue(TEXT("At least one actual entry widget was reused"), ReusedAssignments > 0);
    AddInfo(FString::Printf(TEXT("[M3-06C2] Rows=%d Panels=2 PeakDisplayed=%d MaxUniquePerPanel=%d "
                                 "ReusedAssignments=%d Samples=%d MeanMs=%.3f MaxMs=%.3f"),
                           ItemCount, OverallPeakDisplayed, MaxUniquePerPanel, ReusedAssignments,
                           Samples, TotalMs / Samples, MaxMs));
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif