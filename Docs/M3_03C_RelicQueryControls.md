# M3-03C：遗物搜索、筛选与排序控件

2026-10-30，UE **5.5.4**。[M3-03B及Tick补修已验收](Reports/M3_03B_Verification.md)。本章现为**实施中，尚未完成整章验收**；C++由你手动输入，助手按你的当次要求完成7.1层级装配及7.4校验报错复核，实际记录见对应小节。下文代码与步骤保留作学习和复现依据。

## 1. 目标、范围与时间

P打开暂停页后，遗物列表上方增加搜索框及两个下拉框：按名称 / 稳定ID搜索，按全部 / 槽A资格 / 槽B资格 / 已装备筛选，按设计顺序 / ID顺序排列。无结果时清除旧详情和预览，清空条件后恢复列表。仍是只读浏览；比较按钮继续只比较，不提交换装。

这是M3-03展示阶段的最后一批查询接入，随后进入M3-04安全祭坛与正式确认。现有查询规则已经在03A实现，本章不另写排序 / 搜索算法，不复制库存或GE逻辑。

| 手动工作 | 具体范围 | 预计 |
| --- | --- | --- |
| C++ | 五份旧文件：UI Build.cs、Presenter.h、Panel.h / cpp、原Presenter测试 | 60–90分钟 |
| Designer | 修改现有WBP_ETRelicPanel，新增三个绑定控件；不创建新WBP | 20–30分钟 |
| 检查 | 两项既有自动化各一次，实际查询与焦点短查 | 5–10分钟 |
| 构建 / 索引 | Editor / 三模块索引 / Game，各按本机实际记录 | 另计等待 |

当前186 Source / 104cpp（Core72 / UI29 / Editor3）、31测试、298 Content / 来源79；本批无新增文件 / 测试注册 / 资产，数量预计不变。不改Core、角色蓝图、IMC、地图和暂停Flow。ListEntry保持已修复的**Auto**，普通Panel / Pause仍Never。

## 2. 先理解三个约定

- 查询状态只归当前Panel持有的Presenter；Widget把输入转换为Query，Presenter沿用稳定Item和原生投影。筛选 / 排序不会装备、消费资源或推进库存Revision。
- 搜索匹配名称或RelicId，忽略大小写、去两端空白，不搜索描述。全部条件同时生效。当前两件都兼容A / B，因此切换这两个筛选后仍为两行属于正常。
- 当前P每次打开会新建暂停页，查询回默认“空搜索 / 全部 / 设计顺序 / 比较槽A”；不把短期浏览条件写入GI / 磁盘。同一面板刷新不抢焦点、相同内容不重设搜索文本。

输入边界：搜索框内的P是字符，**不会关闭暂停页**；按Esc只离开搜索框并把焦点交给“比较槽A”，不执行比较、不恢复游戏；之后松开P才按原流程继续。Enter提交文字不执行换装 / 继续，Tab沿现有UMG焦点路径移动。该边界复用UIOnly，不向世界补发攻击或移动输入。

## 3. 准备与依赖

Save All，关闭UE；CLion保留打开。所有步骤都在现有文件上改，不通过UE新建类。先完成第3–5节，再统一Build。

### 3.1 EmberTrialUI.Build.cs

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\EmberTrialUI.Build.cs`。

公开Widget头需要使用ComboBox的选择事件类型，因此将`"Slate"`、`"SlateCore"`从Private列表**移动**到Public列表；保留其他条目，两边不重复。两个列表最终应为：

```csharp
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "Slate",
            "SlateCore"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EmberTrial",
            "GameplayAbilities",
            "GameplayTags",
            "InputCore"
        });
```

将原“Slate / Input只服务实现”的注释改成“Core / GAS / Input只用于UI实现；公开Widget事件签名所需Slate依赖在Public声明”。类名、构造函数和PCH配置保持。

### 3.2 ETRelicPresenter.h

路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Inventory\ETRelicPresenter.h`。

在public中找到`bool SetQuery(const FETRelicListQuery& NewQuery);`，保留它，在下一行新增这个只读查询；其余声明与cpp不改：

```cpp
    const FETRelicListQuery& GetQuery() const { return Query; }
```

Query仍是private原有成员，不增加第二份查询状态，不加Blueprint可写属性。

## 4. 替换Panel两个文件

只完整替换以下两个文件，不动ListEntry / Presenter.cpp。新增控件是必需绑定，Build后要完成第7节Designer再运行；在补齐控件前出现Missing required panel layout说明尚未完成装配。

### 4.1 ETRelicPanelWidget.h（完整内容）

`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETRelicPanelWidget.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "ETRelicPanelWidget.generated.h"

class UButton;
class UEditableTextBox;
class UListView;
class UTextBlock;
class UETRelicPresenter;
struct FETRelicPanelData;

// 页面只发浏览意图，不拥有暂停 / 装备权限；查询状态由自己的Presenter保存。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETRelicPanelWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry,
                                         const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry,
                                const FKeyEvent& InKeyEvent) override;

private:
    // 控件同步受保护，SetText / SetSelectedIndex产生的回调不能再发一次查询。
    bool HasRequiredLayout() const;
    bool IsSearchFocused() const;
    void InitializeQueryControls();
    void SyncQueryControls();
    void ApplyQueryFromControls();
    void Render(const FETRelicPanelData& Data);
    void HandleSelectionChanged(UObject* Item);

    UFUNCTION()
    void HandleSearchChanged(const FText& Text);

    UFUNCTION()
    void HandleQuerySelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleSlotA();

    UFUNCTION()
    void HandleSlotB();

    // ---- 页面持有Presenter；以下11个BindWidget对应Designer的名字与类型 ----
    UPROPERTY(Transient)
    TObjectPtr<UETRelicPresenter> Presenter;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UEditableTextBox> SearchBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> FilterCombo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UComboBoxString> SortCombo;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UListView> RelicList;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> InventoryStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailDescriptionText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PreviewText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SlotText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotAButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotBButton;

    FDelegateHandle PresenterHandle;
    FDelegateHandle SelectionHandle;
    bool bSyncingQueryControls = false;
    bool bRendering = false;
};
```

### 4.2 ETRelicPanelWidget.cpp（完整内容）

`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETRelicPanelWidget.cpp`

```cpp
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
```

这里的选项序号只在View里转换成枚举；遗物本身仍按RelicId识别。程序同步控件由保护位阻止递归回调；相同查询结果不必让Presenter重复广播。没有新增业务Tick / Timer；两件数据每次文本变化立即查询，未来大列表是否加输入防抖由实际测量决定。

搜索P的处理在内层Panel的KeyUp截断冒泡；外层Pause的原KeyUp / 恢复逻辑保留。Esc在PreviewKeyDown转移焦点，不触发SlotA按钮OnClicked。本机5.5.4已核对ComboBoxString、EditableTextBox事件签名及UserWidget两个按键覆写接口；代码仍需你正常Build与实机焦点检查，不能把API阅读写成已编译通过。

## 5. 扩展原测试，不创建新测试文件

`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Tests\ETRelicPresenterTests.cpp`

原include / 测试注册 / 原有断言全部保留。找到原文：

```cpp
    int32 ViewEvents = 0;
```

在它**前面**插入以下整块。此时LifeItem / EdgeItem / Inventory / Equipment刚通过有效性检查，还没有执行Real life commit。块内另用SearchQuery并有独立花括号，避免与下文原Query重定义。

```cpp
    // ---- 查询改变可见集合，不改变领域库存或丢弃仍拥有的Item ----
    {
        const auto BeforeQuery = Inventory->GetSnapshot();
        FETRelicListQuery SearchQuery;
        SearchQuery.SearchText = TEXT("  LIFE_  ");
        TestTrue(TEXT("Search accepted"), Presenter->SetQuery(SearchQuery));
        TestTrue(TEXT("Query retained without changing text"),
            Presenter->GetQuery().SearchText == SearchQuery.SearchText);
        TestTrue(TEXT("Search keeps matching stable item"), Presenter->GetItems().Num() == 1 &&
            Presenter->GetItems()[0] == LifeItem && Presenter->GetSnapshot().SelectedId == LifeId);

        SearchQuery.SearchText = TEXT("no_matching_relic");
        Presenter->SetQuery(SearchQuery);
        TestTrue(TEXT("Empty result clears selection and preview"), Presenter->GetItems().IsEmpty() &&
            Presenter->GetSnapshot().SelectedId.IsNone() &&
            Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::Unavailable);
        TestTrue(TEXT("Filtered items remain owned by presenter"),
            Presenter->FindItem(LifeId) == LifeItem && Presenter->FindItem(EdgeId) == EdgeItem);

        SearchQuery = {};
        SearchQuery.Filter = EETRelicListFilter::Equipped;
        Presenter->SetQuery(SearchQuery);
        TestTrue(TEXT("Unequipped inventory has valid empty filter"), Presenter->GetItems().IsEmpty() &&
            Presenter->GetSnapshot().State == EETRelicPanelState::Ready);
        Presenter->SetQuery({});
        TestTrue(TEXT("Clearing query restores rows and default selection"),
            Presenter->GetItems().Num() == 2 && Presenter->GetSnapshot().SelectedId == LifeId &&
            Presenter->FindItem(LifeId) == LifeItem && Presenter->FindItem(EdgeId) == EdgeItem);
        TestFalse(TEXT("Same query is a no-op"), Presenter->SetQuery({}));
        TestTrue(TEXT("Queries never commit inventory"),
            Inventory->GetSnapshot().Revision == BeforeQuery.Revision &&
            Inventory->GetSnapshot().Equipped == BeforeQuery.Equipped);
        TestEqual(TEXT("Queries keep maximum100"),
            Body->GetAbilitySystemComponent()->GetNumericAttribute(
                UETHealthAttributeSet::GetMaxHealthAttribute()), 100.0f);
        TestEqual(TEXT("Queries keep health100"),
            Body->GetAbilitySystemComponent()->GetNumericAttribute(
                UETHealthAttributeSet::GetHealthAttribute()), 100.0f);
    }
```

新断言验证搜索、空结果、清条件后Item身份恢复及无库存副作用；实际输入法、下拉框、P / Esc与Slate Tick由第8节短实机覆盖。没有临时蓝图测试图或第二个Viewport。

## 6. 一次正常构建与索引

所有五个文件保存，确认UE已关闭。在PowerShell依次执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本包含正常Editor Build与三模块索引刷新。成功后，在CLion原项目Reload CMake，查看EmberTrialUICodeModel及ComboBox / EditableTextBox头文件导航。再执行Game构建：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

成功后正常打开工程。不要在Live Coding旧进程中新增反射绑定。若失败，发第一条具体error及前后约10行；仅UBT exit code 6不能定位原因。

## 7. Designer逐项装配

### 7.1 打开与层级

**2026-10-30 实际操作记录：**助手按你的要求通过 computer-use 添加并命名了 SearchBox、QueryOptionsRow、FilterCombo、SortCombo；三个绑定控件均勾选 Is Variable，原有内容保留。实际层级中 RelicList 和 DetailsColumn 位于 BodyColumns 内，查询控件已放在 BodyColumns 上方。WBP_ETRelicPanel 已 Compile（绿色）并保存。第一次先保存时项目校验器提示 `ET-UI-001`（Widget 类 / 树尚未就绪），随后 Compile 再保存；此记录不代表本章功能或后续布局验收已完成。你可以直接从 7.2 继续，不必重复创建控件。

1. Content Browser进入`/Game/EmberTrial/ui/Inventory`，打开**WBP_ETRelicPanel**，右上切到Designer。父类保持ETRelicPanelWidget。
2. 在Hierarchy找到PanelSize → PanelBorder → **Vertical**。它直接包含InventoryStatusText和BodyColumns；RelicList / DetailsColumn位于BodyColumns内。不要选外层WBP_PauseMenu的横排，也不要改ListEntry。
3. 从Palette搜索**Editable Text Box**（不是Text，也不是Multi-Line Editable Text Box），拖到Vertical中，放在InventoryStatusText后、BodyColumns前。F2改名为**SearchBox**，勾Is Variable。
4. 再把一个**Horizontal Box**拖到SearchBox下面、BodyColumns上面，命名QueryOptionsRow（布局名，无BindWidget）。向它依次加入两个**ComboBox (String)**，F2命名为**FilterCombo**、**SortCombo**，两者勾Is Variable。
5. 原先详情、比较按钮保持顺序。最终主干应为：

```text
原根VerticalBox
├─ InventoryStatusText           原有
├─ SearchBox                     新：Editable Text Box
├─ QueryOptionsRow               新：Horizontal Box
│  ├─ FilterCombo                新：ComboBox (String)
│  └─ SortCombo                  新：ComboBox (String)
└─ BodyColumns                   原有Horizontal Box
   ├─ RelicList                  原有，Entry Class保持
   └─ DetailsColumn              原有详情与比较控件，不删
```

原8个必需绑定保留，本章新增3个，总计11个。它们的名字必须与4.1一一对应；QueryOptionsRow不勾Is Variable也可以。

### 7.2 SearchBox与查询行（选中对应行后看Details）

| 控件 | Details设置 | 目的 |
| --- | --- | --- |
| SearchBox | Content → Text留空；Hint Text填`搜索名称或 ID（Esc 退出输入）`；无Text Binding | 初始不筛选，提示文字不是查询内容 |
| SearchBox | Is Read Only=False；Is Password=False；Is Enabled=True；Visibility=Visible | 能正常编辑并接收输入 |
| SearchBox | Font沿用项目能显示中文的字体链，Size=18；Foreground可读 | 中文输入 / 提示不缺字 |
| SearchBox | Slot (Vertical Box Slot) → Size=Auto；Padding Left0 / Top8 / Right0 / Bottom6；Horizontal Alignment=Fill | 仅按自身高度占位 |
| QueryOptionsRow | Slot (Vertical Box Slot) → Size=Auto；Padding Bottom8，其余0；Horizontal Alignment=Fill | 两下拉框横排 |
| FilterCombo | Slot (Horizontal Box Slot) → Size=Fill，值1；Padding Right6；Vertical Alignment=Fill | 左半宽 |
| SortCombo | Slot (Horizontal Box Slot) → Size=Fill，值1；Padding Left6；Vertical Alignment=Fill | 右半宽 |

在SearchBox的Details展开**Behavior → Advanced**，将**Clear Keyboard Focus on Commit**、**Select All Text on Commit**、**Revert Text on Escape**设False。可用Details搜索`Commit` / `Escape`定位；这些确为本机5.5.4 EditableTextBox的可编辑属性。C++初始化也用正式Setter固定同样配置，所以Enter保留输入焦点、Esc不回滚已经应用的查询；Esc转移焦点由Panel实现。

### 7.3 两个ComboBox (String)

1. 依次选FilterCombo、SortCombo：Is Focusable=True，Is Enabled=True，Visibility=Visible；字体沿用中文链，Size=18。把Content Padding设左右8 / 上下4；下拉列表Max List Height可设200。
2. Default Options保持空、Selected Option不手填。四个筛选与两个排序选项由C++在每个实例Construct时统一创建并选默认，避免Designer与枚举映射分叉。
3. 不点Details → Events的`+`创建OnSelectionChanged / OnTextChanged蓝图事件，也不接Event Tick；这些均在原生代码绑定 / 解绑。若误创建空事件，可删除，保留原有无业务Graph状态。
4. 保留RelicList Single选择模式、Entry Class和原有焦点 / 鼠标配置；SlotA / B仍可聚焦。原Panel Is Focusable沿用，不要求依靠Panel本身聚焦来输入。
5. 若新增两行导致底部比较按钮被挤出：先确认SearchBox / QueryOptionsRow都是Auto，没有Fill。RelicList是BodyColumns的子项，不是Vertical Box直接子项；先检查其父容器及PanelSize的实际尺寸，再按预览调整，不要盲目修改不存在的列表高度或隐藏详情 / 按钮。设计预览以现有暂停页实际尺寸检查。

### 7.4 编译、父页面与Tick

1. Compile WBP_ETRelicPanel，确认无缺绑定、类型或Native Tick警告；Save。
2. 打开原WBP_PauseMenu，Designer确认原嵌入的RelicPanel仍在右列，原继续 / 返回菜单 / StatusText不变；Compile / Save，使嵌入布局刷新。可能产生Pause的正常重存差异，Push时按实际加入。
3. ListEntry保持**Auto**；Panel / Pause保持**Never**；无属性Binding、无业务Tick。不要把上一轮兼容修复回退。
4. 返回正式地图。无需新增角色引用 / Blueprint节点，也不新增UIBootstrap或Presenter组件。新增控件不走独立世界暂停分支。

#### 7.4.1 ET-UI-001旧消息复核与处理（2026-10-30，已关闭）

用户看到的消息为：

```text
[ET-UI-001] Widget class / tree not ready; Compile and Save this Widget first.
```

该消息来自本项目`ETProjectAssetValidator::ValidateWidget`：生成类 / WidgetTree无效，或Blueprint处于BS_Dirty / BS_Error时会拒绝校验。7.1装配期间助手先保存、后Compile，第一次保存触发了这条检查。**Message Log会保留历史错误，后续校验通过不会自动移除先前红行。**本次确认它是旧消息，不需要修改校验器规则。

助手已通过computer-use关闭历史Message Log窗口，对当前WBP_ETRelicPanel重新执行**Compile → Save**，Compile为绿色、资产显示已保存；随后读取完整运行日志核对时间（引擎UTC加8小时为北京时间）：

| 2026-10-30北京时间 | 实际记录 |
| --- | --- |
| 16:21:44.053 | 首次保存产生ET-UI-001，Issues=1 |
| 16:22:26.389 / 16:29:06.579 / 16:34:25.948 | 后续保存已连续得到本项目Valid |
| **16:36:35.129** | 本次重新Compile / Save后再次得到`Valid Asset=/Game/EmberTrial/ui/Inventory/WBP_ETRelicPanel.WBP_ETRelicPanel`，没有新增编译或校验错误 |

本次只确认当前面板编译 / 结构校验正常，功能检查仍按第8节完成。不需要重复Build C++或改动BindWidget / 校验器源码。

以后遇到同样提示，先补齐控件并Compile至绿色，再Save；以Output Log中该资产**最新时间的Valid / Invalid**判断结果。Message Log的旧红行可关闭窗口后保留，不将旧行误认为本次失败；若最新时间出现新的Invalid，则按新消息继续定位。

## 8. 本批分组检查（只覆盖新增范围）

### 8.1 两项自动化，各一次

非PIE时，在UE Output Log的命令输入栏分别执行，等上一项完成再执行下一项：

```text
Automation RunTests EmberTrial.UI.Relics.ViewModels
Automation RunTests EmberTrial.UI.Relics.Presenter
```

两项都应Success。复用原有投影及生命周期专项，只有Presenter增加了断言；不跑全31项、不重建测试地图。自动化失败请发失败断言全文和文件行号，长日志由我读取。

### 8.2 查询与文本焦点（约3–5分钟）

| 步骤 | 具体操作 | 应看到的结果 |
| --- | --- | --- |
| 1 初始 | 正式地图Play，P打开 | 两行 / 只读2件，空搜索、全部、设计顺序；原生命100 |
| 2 搜索 | 点SearchBox，输入`life`；Ctrl+A后输入`LIFE_`，再试中文`生息` | 都只显示生息余烬；大小写 / 中文正常，详情与比较对应 |
| 3 无结果 | Ctrl+A输入`zzz` | 0件、无选中行，“没有符合条件的遗物”，旧详情与数值不残留 |
| 4 P字符 | Ctrl+A输入`p`并松开P | 框内出现p，暂停页保持；不能恢复世界或补发游戏输入 |
| 5 离开输入 | Ctrl+A → Backspace清空；按Esc | 两行恢复，焦点到比较槽A；不自动执行比较 / 继续；此时松P能关闭，立即可移动 |
| 6 排序 / 选择 | 再P，点砺锋残片，排序改ID顺序，再改回设计顺序 | ID顺序为砺锋 / 生息；切回为生息 / 砺锋；选中仍为砺锋，不按数组序号串行 |
| 7 筛选 | 依次选可用于A、可用于B、已装备，再全部 | 前两者各两行；空槽已装备为0；回全部恢复。它不改变“比较目标槽”或真实装备 |

文本框内按Enter仅提交文本、保留编辑焦点并保持暂停；中文输入法确认候选也不得恢复游戏。若焦点意外离开或触发原继续按钮，按本批失败窄项反馈，不视为正常默认行为。无需给我手抄每一步日志。

### 8.3 一次真实装备徽记与收尾（约2–5分钟）

1. P关闭，在未暂停的游戏控制台执行`ET_Relic 1 life_ember`，关闭控制台。再P，筛选“已装备”：只显示生息余烬，槽A徽记正确。切换全部 / 两种排序不会卸装或治疗，原实际生命保持100/120。
2. 搜索任意无结果，再清空，确认徽记与详情恢复；比较槽A显示已装备，槽B沿用重复遗物拒绝。只做一次，不复做旧Continue / Retry矩阵。
3. 开关暂停页两轮，每次只有一份控件，重新打开回默认查询。最后P关闭、移动 / 左键 / 2 / E短查并Stop；Save All。长日志由我检查订阅 / 解绑及无新Ensure，不要求你数日志。
4. 本章没有更改默认数值 / AI / slomo / Debug；保持原值。开发命令仅影响本轮会话，Stop后空槽恢复；不需要为了清理再开一整轮测试。

若P字符意外关闭、输入法确认继续了游戏、下拉框中文缺字、控件溢出或Tick Ensure复现，停在对应窄项，发操作路径与截图；无需从头重验全部能力。

## 9. 简短反馈与学习重点

```text
M3-03C 完成 / 卡在第__节第__步
五旧Source；关闭UE后Editor Build / 三索引 / Game Build：
原Panel新增3绑定、两Combo选项由C++生成；Panel-Pause Compile / Save：
ViewModels / Presenter各一次：
搜索中文与大小写 / 无结果清空 / 排序保留选择 / 四筛选：
搜索内P字符、Esc离开再P继续 / Enter与中文输入确认不继续：
真实已装备筛选 / 100-120无治疗 / 两轮开关回默认 / 无新Ensure：
左键-2-E与移动短查、Stop / Source资产Save All：
最明显布局或失败问题（无填无）：
完成时分 / 手动代码 / Designer / 检查 / 构建等待：
```

本章应能解释：为什么文本框事件不直接改库存，为什么过滤后仍保留Item身份，为什么排序不能用数组下标保留选择，为什么同步控件要防回调递归，以及P字符和暂停快捷键如何沿UMG事件路径分流。掌握这些比新增一套临时验收蓝图更有面试价值。

## 10. 本批 Commit / Push（固定放在最后）

**当前仅准备文档**：提交本章指南、03B验收 / 补修收尾、AssetManifest及进度 / 路线 / 架构 / 工作流 / Git / 入口同步文档；本地b1c9579已包含Auto资产，不重复Stage无变化Source或资产。

当前文档差异清单：

```text
Docs/Reports/M3_03B_Verification.md
Docs/M3_03B_RelicPausePanel.md
Docs/M3_03C_RelicQueryControls.md
Docs/AssetManifest.csv
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/M2_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

- 当前文档建议英文：`docs: close relic panel tick fix and prepare inventory query controls`
- 中文译文：`docs: 完成遗物面板 Tick 补修验收并准备库存查询控件`

**你实现并检查完成后**，Stage下面五份旧Source及资产的最新差异，同时加入实际更新的本章记录 / 验收报告 / 来源清单。原Panel源文件是替换，不删除重建新类：

```text
EmberTrial/Source/EmberTrialUI/EmberTrialUI.Build.cs
EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETRelicPanelWidget.h
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp
EmberTrial/Content/EmberTrial/ui/Inventory/WBP_ETRelicPanel.uasset
EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset（仅确有重存差异时）
Docs/M3_03C_RelicQueryControls.md
Docs/AssetManifest.csv（验收后按实际SHA更新）
```

- 实现建议英文：`feat: add relic search filters and stable list sorting`
- 中文译文：`feat: 新增遗物搜索筛选与基于稳定标识的列表排序`

CLion刷新Changes → 将最新Unstaged差异再次Stage → Review Staged Diff → Commit / Push由你操作。Artifacts中的文档校对候选不属于正式Source，不Force Add；生成文件、日志、缓存、包留本地。英文建议与中文译文可择一作为提交标题，不需要把两句硬拼成一个标题。
