# M3-06C2 500条遗物的 ListView 条目回收验证

2026-11-08下发，UE 5.5.4。预计手动20–35分钟，构建等待另计。本节新增一份Editor自动化测试文件。候选已在隔离工程编译、链接并运行成功。[C1验收报告](Reports/M3_06C1_Verification.md)

## 1. 验证范围

测试实例化正式`WBP_ETRelicPanel`，取其`RelicList`和原条目蓝图，在固定300×360 Slate几何下放入500个临时数据对象。两次创建面板，每次从顶部选中首行、跳转滚动、到底部、回顶部，再清空。检查活动条目数量、真实实例复用、姓名和槽位徽记、旧委托解绑与选中高亮清理。

隔离工程的实测输出如下。

```text
[M3-06C2] Rows=500 Panels=2 PeakDisplayed=6 MaxUniquePerPanel=12 ReusedAssignments=156 Samples=32 MeanMs=0.201 MaxMs=1.459
```

计时仅含Slate布局预处理与列表Tick。选中通知显式送入原条目回调，回收由实际ListView驱动。正式目录在实机短查时仍为六件。完整帧耗时按后续带渲染集成口径单独记录。

## 2. 新建测试文件

Stop PIE，Save All并关闭UE。CLion中在现有`EmberTrialUI/Tests`目录新建普通C++源文件，文件名为`ETRelicListViewStressTests.cpp`。将以下内容完整保存。

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Tests/ETRelicListViewStressTests.cpp`

```cpp
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
```

## 3. 正常构建

UE保持关闭，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本完成Editor Build与三模块索引。CLion出现提示时Reload CMake，确认新测试文件属于EmberTrialUI索引。再执行Game Development Build。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 4. ListView 专项

打开UE，在非Play状态下于Output Log底部运行。

```text
Automation RunTests EmberTrial.UI.Relics.ListViewStress
```

等到`Result={Success}`，复制`[M3-06C2]`开头的一整行。实测实例数和耗时可随编辑器状态变化，按日志记录。断言检查活动条目少于50、每个面板累计实例少于100、至少一次实际复用，以及两轮清空后各数据对象均已解绑。

## 5. 正式六件短查

1. Play正式地图`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。按P，核对六件。切换两次选择，查看名称、说明与选中状态，关闭P。
2. 重新打开P，检查六件与新页面选择，继续后移动正常。Stop并Save All。
3. 反馈双Build／三索引、专项Success与`[M3-06C2]`整行、六件及P重开、手动工时与构建等待。

## C2验收结果

2026-11-09通过。双Build、三索引、专项与正式六件、P重开均已确认。正式工程500条、两个面板、峰值6、每面板最多12、156次复用。手动25分钟，构建等待5分钟。提交前重新Stage完整测试。[验收报告](Reports/M3_06C2_Verification.md) · [C3操作指南](M3_06C3_SixRelicIntegratedPackage.md)

## 6. Commit / Push

C1实现已在本地提交`2d98975`。在CLion Review并Stage本轮11份文档，再Commit和Push。

```text
Docs/Reports/M3_06C1_Verification.md
Docs/M3_06C1_RelicProjectionStress.md
Docs/M3_06C2_RelicListViewStress.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文 `docs: verify relic projection and plan ListView recycling stress`
- 中文 `docs: 完成遗物数据投影验收并规划ListView条目回收压测`

C2操作通过后，测试提交包含以下文件。

```text
EmberTrial/Source/EmberTrialUI/Tests/ETRelicListViewStressTests.cpp
```

- 英文 `test: verify 500-row relic ListView recycling`
- 中文 `test: 验证500条遗物的ListView条目回收`
