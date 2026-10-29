# M3-03A：遗物列表数据对象与只读属性预览

2026-10-29，UE **5.5.4**。[M3-02C2-02已验收](Reports/M3_02C2_02_Verification.md)，M3-02最小装备闭环收尾。本章仍由你手动输入代码；当前只是指南与本地候选，运行工程尚未实施、未Build或执行新测试。

## 1. 本章目标与工期

为接下来的真实遗物界面准备三项能力：把拥有ID转为有名称 / 描述 / 图标软引用的列表行；按目录顺序 / 稳定ID排序、按槽位资格 / 已装备 / 搜索过滤；用当前身体的真实属性生成只读换装比较。重复预览不Apply / Remove GE、不改血量、库存或当前攻击链。

本章提供`UETRelicListItem`数据对象，供下一批ListView消费；**数据Item和被池化的条目Widget是两种对象**。Item保存一行值，条目Widget在复用时先解绑旧Item、绑定新Item并立即取快照；实际UMG条目、Presenter与详情 / 确认按钮在下一批接入。本章Play画面沿用原HUD，验收不要求搭临时面板或蓝图图表。

范围为**三新UI Source / 三旧Core Source**，不改Build.cs、Config、输入、地图或资产。预计手动代码 **50–80分钟**、阅读 **10分钟**、检查 **2–5分钟**，构建等待另记。可以分次完成代码；第3–5节全部保存后再一次正常Build，不把中途缺声明的索引红线当成最终错误。

当前实际176 Source / 98cpp（Core72 / UI23 / Editor3）、29自动化、296 Content / 来源77；实施后预计**179 Source / 100cpp（Core72 / UI25 / Editor3）、30自动化**。新UI测试必须放UI模块，不放Core Tests。

## 2. 职责与输出契约

| 类型 / 接口 | 职责 | 调用结果 |
| --- | --- | --- |
| FETRelicListRow | 稳定ID、FText、图标软引用、槽位资格 / 装备徽记的值快照 | 不拥有身体、ASC或GE |
| FETRelicListQuery | 展示筛选、排序、搜索条件 | 改显示，不改配装 |
| ETRelicPresentation::BuildRows | 用库存快照和已加载目录投影列表；先检查全部拥有数据再过滤 | Success允许空列表；失败清空输出 |
| UETRelicListItem | ListView要求的UObject数据载体；相同数据去重通知 | Item存数据，未来Widget负责显示 / 解绑 |
| RelicEquipment::TryBuildPreview | 复用真实换装的基值 / 最终数值预检 | Success才有比较结果；NoChange / 失败清空输出 |

列表只使用Inventory已缓存的两个小定义，保留Icon为软引用；这里不加载图片。默认按SortOrder，破同分用稳定ID；StableId按ID字典顺序，不宣称按中文拼音或本地化名称排序。搜索匹配ID / 当前显示名称、忽略大小写并去除前后空格。

输入的库存快照可以是此前捕获的值；能展示旧快照不代表它仍有当前提交权限。确认时仍把观察到的Revision交给TryEquip重新验证。数组下标、中文名称、Item对象地址都不能充当RelicId或领域槽位。

预览允许存活身体在动作中读取，也允许暂停时读取；真正换装仍要求Idle、非暂停、非Pending和当前身体。NoChange表示已经是该件，不生成可以重复提交的方案；UI后续显示当前数值即可。现有25基础 / +5加成与已装备+20必须扣除自身贡献后重算，不能累计成140。

## 3. 先接Core只读预览（两个旧文件）

Save All并关闭UE，在CLion手动修改。下面路径均相对于`E:\UE_Project\Demo\EmberTrial\Source`，注意区分EmberTrial与EmberTrialUI。

### 3.1 EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h

第一段放在public区域，第二段跟随原ReadPreviewBases放private区域。保留generated.h为最后一条include，不增加反射函数。

**public只读预览声明**。找到以下原文：

```cpp
    FETRelicInventoryReply TryEquip(EETRelicSlot Slot, FName RelicId, uint64 ExpectedRevision);
```

保留原文，在其后插入：

```cpp
    // 只构建比较结果：暂停 / 动作中可读，不应用GE、不改库存或当前连段快照。
    FETRelicInventoryReply TryBuildPreview(EETRelicSlot Slot, FName RelicId,
        uint64 ExpectedRevision, FETRelicLoadoutPlan& OutPreview) const;
```

**private共享预检声明**。找到以下原文：

```cpp
    bool ReadPreviewBases(float& OutMaxHealth, float& OutAttackDamage) const;
```

保留原文，在其后插入：

```cpp
    FETRelicInventoryReply TryMakePlan(EETRelicSlot Slot, FName RelicId,
        uint64 ExpectedRevision, FETRelicInventoryPlan& OutPlan) const;
```

### 3.2 EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp

前面的生命周期、Restore、ReadPreviewBases等实现保留。只替换本文件末尾原TryEquip部分，改为共享预检 / 只读预览 / 真实提交三个函数。这样以后修改数值规则时，预览和确认不会分成两套公式。

**原文件末尾TryEquip替换为三个完整函数**。找到以下原文：

```cpp
// ---- 同步提交：先完成所有拒绝判断
```

从这条注释开始，到本文件最后一个`}`，整段替换为：

```cpp
// ---- 只读方案预检：比较与提交共用规则，但读操作不要求Idle / 非暂停 ----
FETRelicInventoryReply UETRelicEquipmentComponent::TryMakePlan(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision,
    FETRelicInventoryPlan& OutPlan) const
{
    OutPlan = {};
    if (bChanging)
    {
        return {EETRelicInventoryResult::Busy};
    }
    UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadBody(Body, ASC) || Body->IsCombatDead() || !IsEquipmentReady() || !IsValid(Inventory))
    {
        return {EETRelicInventoryResult::NotReady};
    }
    if (ExpectedRevision == 0 || ExpectedRevision != Inventory->GetSnapshot().Revision)
    {
        return {EETRelicInventoryResult::StalePlan};
    }
    float BaseMax = 0.0f;
    float BaseDamage = 0.0f;
    if (!ReadPreviewBases(BaseMax, BaseDamage))
    {
        return {EETRelicInventoryResult::RuleRejected, EETRelicLoadoutResult::InvalidBaseValues};
    }
    FETRelicInventoryPlan Candidate;
    const FETRelicInventoryReply Reply = Inventory->TryBuildPlan(
        Slot, RelicId, BaseMax, BaseDamage, Candidate);
    if (Reply.Result != EETRelicInventoryResult::Success)
    {
        return Reply;
    }
    const FETRelicModifierTotals& Totals = Candidate.GetLoadoutPlan().AfterModifiers;
    if (!AreModifiersValid(ASC, Totals) || !IsFinalDamageValid(Totals))
    {
        return {EETRelicInventoryResult::RuleRejected, EETRelicLoadoutResult::InvalidFinalValues};
    }
    OutPlan = Candidate;
    return Reply;
}

FETRelicInventoryReply UETRelicEquipmentComponent::TryBuildPreview(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision,
    FETRelicLoadoutPlan& OutPreview) const
{
    OutPreview = {};
    FETRelicInventoryPlan Plan;
    const FETRelicInventoryReply Reply = TryMakePlan(Slot, RelicId, ExpectedRevision, Plan);
    if (Reply.Result == EETRelicInventoryResult::Success)
    {
        OutPreview = Plan.GetLoadoutPlan();
    }
    return Reply;
}

// ---- 写入仍要求当前身体Idle / 非暂停；只读预览不能绕过确认的版本重验 ----
FETRelicInventoryReply UETRelicEquipmentComponent::TryEquip(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision)
{
    UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    if (bChanging || (IsValid(Inventory) && Inventory->bChanging))
    {
        return {EETRelicInventoryResult::Busy};
    }
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    if (!IsValid(Body) || !Body->IsFormalActionIdle() || UGameplayStatics::IsGamePaused(this))
    {
        return {EETRelicInventoryResult::NotReady};
    }
    FETRelicInventoryPlan Plan;
    const FETRelicInventoryReply Reply = TryMakePlan(Slot, RelicId, ExpectedRevision, Plan);
    if (Reply.Result != EETRelicInventoryResult::Success)
    {
        return Reply;
    }
    // 所有业务拒绝均已完成。提交回调保持C2-02协议，不先Apply再伪造回滚。
    return Inventory->TryCommitPlan(Plan,
        [this](const FETRelicLoadoutPlan& Committed, const uint64 Revision)
        {
            TGuardValue<bool> ChangeGuard(bChanging, true);
            AppliedLoadout = Committed.After;
            AppliedRevision = Revision;
            WriteModifiersUnchecked(Committed.AfterModifiers);
        });
}
```

TryMakePlan只读检查，不接受任意外部写入器；TryEquip保留原提交点、Busy / 暂停 / Idle条件。没有为了允许预览而放松真实换装权限。

## 4. 新建UI列表数据模型（两个新文件）

在CLion创建普通C++头 / 源文件，不使用UE类向导、不创建蓝图子类。需要的文件夹没有就手动创建。两个路径均在**EmberTrialUI模块**：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Inventory\ETRelicViewModels.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Inventory\ETRelicViewModels.cpp
```

### 4.1 ETRelicViewModels.h完整内容

本文件定义反射结构体 / 数据UObject，generated.h由UHT生成，不要自己创建该文件。公开UI头只前置声明Core类型，真正Core include留在.cpp；沿用现有私有Core依赖。


```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPtr.h"
#include "ETRelicViewModels.generated.h"

class UETRelicInventorySubsystem;
class UTexture2D;
struct FETRelicInventorySnapshot;

// UI自己的查询选项；不把显示用数组下标或文字当作领域槽位ID。
UENUM(BlueprintType)
enum class EETRelicListFilter : uint8
{
    All,
    CompatibleWithSlotA,
    CompatibleWithSlotB,
    Equipped
};

UENUM(BlueprintType)
enum class EETRelicListSort : uint8
{
    CatalogOrder,
    StableId
};

USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETRelicListQuery
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relics")
    EETRelicListFilter Filter = EETRelicListFilter::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relics")
    EETRelicListSort Sort = EETRelicListSort::CatalogOrder;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relics")
    FString SearchText;
};

// 列表与详情共用的值快照；没有Actor / ASC / GE句柄，不同步加载图标。
USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETRelicListRow
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    FName RelicId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    int32 SortOrder = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bSupportsSlotA = false;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bSupportsSlotB = false;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bEquippedInSlotA = false;

    UPROPERTY(BlueprintReadOnly, Category = "Relics")
    bool bEquippedInSlotB = false;

    bool operator==(const FETRelicListRow& Other) const;
};

enum class EETRelicRowsResult : uint8
{
    Success,
    Unavailable,
    InvalidQuery,
    InvalidSnapshot,
    InvalidCatalog
};

namespace ETRelicPresentation
{
// 只投影调用者给出的库存快照；旧快照不因此获得当前提交权限。
// Success可以是空列表；失败清空输出，不显示上一份成功结果。
EMBERTRIALUI_API EETRelicRowsResult BuildRows(
    const FETRelicInventorySnapshot& InventorySnapshot,
    const UETRelicInventorySubsystem* Catalog,
    const FETRelicListQuery& Query, TArray<FETRelicListRow>& OutRows);
}

DECLARE_MULTICAST_DELEGATE_OneParam(FETRelicListItemChanged, const FETRelicListRow&);

// UListView的Item必须是UObject。此对象承载一行数据，不是被池化的条目Widget。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETRelicListItem : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Relics")
    FETRelicListRow GetSnapshot() const { return Row; }

    // 原生Presenter更新数据；相同数据不通知。空行用于解除数据身份。
    bool AssignRow(const FETRelicListRow& NewRow);
    FETRelicListItemChanged OnChanged;

private:
    UPROPERTY(Transient)
    FETRelicListRow Row;
};
```

### 4.2 ETRelicViewModels.cpp完整内容

所有行在本地数组中完整构建，只有成功才移交输出；过滤条件不能把未知ID / 重复拥有等错误隐藏掉。条目比较包含装备徽记和软图标路径，相同值不会反复通知。


```cpp
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
```

## 5. 自动化源码（一新 / 一旧）

### 5.1 新UI测试：完整文件

完整路径如下。**不要放到`Source/EmberTrial/Tests`**；否则Core不能反向找到UI头，破坏模块依赖。

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Tests\ETRelicViewModelsTests.cpp
```

粘贴：


```cpp
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
```

测试使用真实配置目录，但只投影捕获的值快照；`Captured.Equipped`的本地修改不是向真实身体装备。假图标路径只检查软引用复制，不加载资源，也不要求你创建该图片。

### 5.2 扩充旧Core事务测试

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicEquipmentTransactionTests.cpp`。只插入下面三处，原断言 / 夹具与后续清理全部保留；不要把新UI测试内容贴进来。

**新断言使用既有Native Tag**。找到以下原文：

```cpp
#include "Abilities/ETGA_LightAttack.h"
```

保留原文，在其后插入：

```cpp
#include "Abilities/ETGameplayTags.h"
```

**空槽只读预览断言**。找到以下原文：

```cpp
    const FActiveGameplayEffectHandle Handle = Equipment->OwnedEffect;
```

保留原文，在其后插入：

```cpp
    // ---- 预览反复执行没有属性 / GE / 库存副作用，失败与NoChange清空输出 ----
    const uint64 PreviewRevision = Inventory->GetSnapshot().Revision;
    const int32 EffectsBeforePreview = ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
    FETRelicLoadoutPlan Preview;
    for (int32 Index = 0; Index < 20; ++Index)
    {
        TestTrue(TEXT("Life preview succeeds"), Equipment->TryBuildPreview(
            EETRelicSlot::First, LifeId, PreviewRevision, Preview).Result == EResult::Success);
    }
    TestEqual(TEXT("Preview max120"), Preview.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Preview attack25"), Preview.AfterPreview.LightAttackBaseDamage, 25.0f);
    TestTrue(TEXT("Preview leaves slots and revision unchanged"),
        Inventory->GetSnapshot().Revision == PreviewRevision && Inventory->GetSnapshot().Equipped.SlotA.IsNone());
    TestEqual(TEXT("Preview leaves real max100"), ASC->GetNumericAttribute(Max), 100.0f);
    TestEqual(TEXT("Preview leaves real health100"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Preview adds no effects"), ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), EffectsBeforePreview);
    TestTrue(TEXT("Preview preserves owned handle"), Equipment->OwnedEffect == Handle);
    TestTrue(TEXT("Unowned preview rejected"), Equipment->TryBuildPreview(
        EETRelicSlot::First, FName(TEXT("missing")), PreviewRevision, Preview).Result == EResult::RuleRejected);
    TestTrue(TEXT("Rejected preview clears previous success"), Preview.After.SlotA.IsNone() &&
        Preview.AfterPreview.MaxHealth == 0.0f);
```

**已装备预览与读写资格区别**。找到以下原文：

```cpp
    TestEqual(TEXT("Max Base100"), ASC->GetNumericAttributeBase(Max), 100.0f);
```

保留原文，在其后插入：

```cpp
    TestTrue(TEXT("Preview existing life plus edge"), Equipment->TryBuildPreview(
        EETRelicSlot::Second, EdgeId, Inventory->GetSnapshot().Revision, Preview).Result == EResult::Success);
    TestEqual(TEXT("Existing equipment not counted twice"), Preview.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Edge preview30"), Preview.AfterPreview.LightAttackBaseDamage, 30.0f);
    TestTrue(TEXT("Same item preview NoChange"), Equipment->TryBuildPreview(
        EETRelicSlot::First, LifeId, Inventory->GetSnapshot().Revision, Preview).Result == EResult::NoChange);
    TestEqual(TEXT("NoChange clears preview output"), Preview.AfterPreview.MaxHealth, 0.0f);
    ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestTrue(TEXT("Busy body may read preview"), Equipment->TryBuildPreview(
        EETRelicSlot::Second, EdgeId, Inventory->GetSnapshot().Revision, Preview).Result == EResult::Success);
    TestTrue(TEXT("Busy body cannot commit"), Equipment->TryEquip(
        EETRelicSlot::Second, EdgeId, Inventory->GetSnapshot().Revision).Result == EResult::NotReady);
    ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
```

新增段用同一身体的真实ASC比较预览前后的Max / Health、版本、Handle与效果数量；再验证已装备生命不重复计算、NoChange清空、活动攻击状态可读但不可提交。20次预览由测试循环执行，无需你手工点击20次。

## 6. 一次正常Build与索引

第3–5节完成并保存，UE保持关闭，PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本成功后CLion Reload CMake / Refresh。新模型与ViewModels测试属于UI目标；Core72 / UI25 / Editor3为预计索引数量，UBT结果为准。继续保持UE关闭：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两个Build成功后打开原uproject。无新蓝图 / BindWidget / 控件名需要设置，不给Controller新增组件。此处新增反射数据类型采用正常Build，不用Live Coding替代。若失败，发第一条error及文件 / 行号，不只发UBT exit6。

## 7. 两项自动化各一次

在非PIE状态的Output Log逐条执行，完成一项再下一项：

```text
Automation RunTests EmberTrial.UI.Relics.ViewModels
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
```

两项应Success。第一项覆盖真实目录投影、排序 / 搜索 / 筛选、捕获快照徽记、无匹配空列表、损坏数据拒绝、Item去重与解绑；第二项保留C2-02原事务 / 恢复断言，并追加只读预览隔离。上一章四项Success沿用，本批不再跑30项全组或重复Ownership / Session / Combo.Flow。

失败先保留日志，把Expected、实际值与文件行号给我；不要通过删断言 / 放宽模块依赖掩盖问题。测试通过后再做下节短查。

## 8. 一分钟真实短查

1. 打开正式`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，Play。HUD仍原100、体力100、治疗3与F就绪；本章尚无遗物面板，画面沿用原状态。
2. 在**游戏控制台**依次执行`ET_Relic 1 life_ember`、`ET_Relic 2 keen_edge`：仍100/120、Bonus5。左键对存活训练目标一次仍30；不要求手动调用新的原生预览函数或搭蓝图。
3. P暂停 / 一种继续方式、2 / E与移动转向跳跃做一次短查。Stop，Save All；逻辑装备只在本GI，Stop即结束，不修改Blueprint默认值。
4. 刚验收过的Retry / Continue / NewRun真实流程沿用C2-02反馈，不在本批重跑。临时节点、输入资源或场景设置均无需新增。

## 9. 简短反馈

```text
M3-03A 完成 / 卡在第__节第__步
三新UI / 三旧Core Source；Editor Build / 三索引 / Game Build：
UI.Relics.ViewModels / Inventory.EquipmentTransactions各一次：
原100 / 真实装备100/120与Bonus5、左键30：
2-E-P与移动转向跳跃、Stop清理 / 无新面板符合预期：
Source / 必要资产Save All；失败或不理解处（无填无）：
完成时分（完整时分）/ 手动代码 / 阅读 / 检查 / 构建等待：
```

成功日志由我读取；报错优先文字，UI实际表现不符再补截图。本章候选仅静态核对，真正Build / Success以你实际执行为准。

## 10. 本批 Commit / Push（固定放在最后）

本地HEAD `a3ae8ca`已含C2-02源码，审查起点工作区干净；未重新核验远端。当前助手只更新以下13份文档：


```text
Docs/Reports/M3_02C2_02_Verification.md
Docs/M3_03A_RelicViewModels.md
Docs/M3_02C2_02_EquipmentTransactions.md
Docs/M3_02C2_01_EquipmentEffectOwnership.md
Docs/M3_Roadmap.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Docs/M2_Roadmap.md
README.md
Introduction.md
```

- 当前文档建议英文：`docs: verify relic transactions and prepare inventory view models`
- 中文译文：`docs: 记录遗物换装事务验收并准备库存展示模型`

你实施本章后，Stage下面三新UI / 三旧Core Source和当时最新文档：


```text
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp
EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicViewModels.cpp
EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicViewModels.h
EmberTrial/Source/EmberTrialUI/Tests/ETRelicViewModelsTests.cpp
```

本章没有新资产 / Config / Build.cs；仅在UE确实产生合法重存差异时纳入实际变化的旧资产，由我同步相关来源SHA。当前77来源无需重写。

- 实现建议英文：`feat: add relic list models and side effect free equipment previews`
- 中文译文：`feat: 新增遗物列表数据模型与无副作用装备预览`

CLion刷新Changes→Stage最新修改（Stage后又改要再次Stage）→Review Diff→Commit / Push由你操作。Raw候选 / 日志 / 缓存 / Trace / 包留本地，不Force Add；助手本轮未改运行Source / Config / Content / AssetManifest / Tools / Git索引、未执行Build / 测试。
