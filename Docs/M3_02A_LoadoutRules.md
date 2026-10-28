# M3-02A：两槽装备方案与纯数值预览

2026-10-28，固定UE **5.5.4**。[M3-01已验收](Reports/M3_01_Verification.md)，当前进入M3-02第一批。**本页代码由你手动实现，助手当前只写文档 / 候选；本章Build、测试与新Source尚未执行 / 落地。**完成后先有可复用的规则，B再接GAS属性 / 效果，C把库存、身体与会话串起来，页面留03。

预计代码35–55分钟，检查3–5分钟，Build / 索引等待另计。本章没有蓝图节点、资产配置或临时按键，不要求搭Controller验收Graph。

## 1. 新增行为与本批边界

已有两件遗物的定义，但“放进哪个槽、是否拥有 / 重复、替换后应是多少”还需要统一规则。本批用一套**不改输入、不加载资源、不应用GE**的计算供后续装备服务与比较面板共用。

- 三新Source：一个公开反射头、一个规则cpp、一个专项测试；只在原Definition头增加一条测试friend。
- 两槽快照只保存RelicId；计算接受调用方提供的已拥有集合与借用目录，生成Before / After及两组属性预览。
- 拒绝无效槽、不拥有、缺定义、非法配置、不兼容、跨槽重复及非法最终数值。重复装备同物同槽返回NoChange，不产生新方案。
- 本章没有库存提交、GAS效果、组件 / Subsystem、UI / 输入 / 会话变化；Play仍100生命、原25伤害。
- 当前161 Source / 88cpp、Core62 / UI23 / Editor3、24测试 / 来源77；**实施后预计164 / 90、Core64 / UI23 / Editor3、25测试，资产不变**，不能把预计记作已成功。

## 2. 用几组例子理解规则

基值必须是**未叠加装备的基础最大生命100、基础轻击25**。不能把已有装备后的120再传入当基值，否则重复计算。实际基础值接入B / C时来自真实配置与消费者，本批100 / 25只是独立测试夹具。

| 输入 | 请求 | 结果 |
| --- | --- | --- |
| 两槽空 | A=生息余烬 | 生成方案，预览100/25→120/25，输入仍两槽空 |
| A=余烬 / B空 | B=砺锋 | 预览120/25→120/30 |
| A=余烬 / B=砺锋 | B=余烬 | DuplicateRelic；不抢占A、不自动交换 |
| A=余烬 / B=砺锋 | B=砺锋 | NoChange；不是一次新的成功提交 |
| A空 / B=砺锋 | B=余烬 | 重新汇总后120/25，旧攻击+5不会残留 |
| A=余烬 / B=砺锋 | A=None | 卸下方案100/30；此函数不修改当前生命 |

预览只支持现有两种**加法**：MaxHealth与LightAttackBonus。轻击显示基础伤害，实际命中还要乘段倍率 / 按合法目标结算；动态增益、敌人防御等尚未建模时不能给伪精确预览。纯规则不判断祭坛距离、暂停、存活或ASC能否应用，这些仍由后续真正的装备入口预检。

## 3. 新建三个文件

保存并关闭UE；CLion保持整个EmberTrial工作区。创建：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Inventory\ETRelicLoadout.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Inventory\ETRelicLoadout.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicLoadoutTests.cpp
```

每段是对应文件的完整内容，不覆盖原Types / Definition。新反射头`.generated.h`仍是最后一个include，不自己创建生成文件。

## 4. 三份完整代码

### 4.1 ETRelicLoadout.h：ID快照、结构化结果与方案

路径：`Source/EmberTrial/Public/Inventory/ETRelicLoadout.h`。Loadout可供后续库存用UPROPERTY持有；Plan只是一次计算结果，不含UObject地址、授权 / 版本号或已提交状态。调用方只能在Success时使用输出；NoChange与失败均清空输出，防止复用旧成功方案。


```cpp
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ETRelicTypes.h"
#include "ETRelicLoadout.generated.h"

class UETRelicDefinition;

/** 逻辑槽位快照；只含稳定ID，不含定义对象、ASC或GE句柄。 */
USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETRelicLoadout
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Relic")
    FName SlotA = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Relic")
    FName SlotB = NAME_None;

    bool operator==(const FETRelicLoadout& Other) const
    {
        return SlotA == Other.SlotA && SlotB == Other.SlotB;
    }
};

// 原生计算结果；当前只处理定义支持的两种加法，不模拟任意动态GAS效果。
struct EMBERTRIAL_API FETRelicModifierTotals
{
    float MaxHealthAdditive = 0.0f;
    float LightAttackBonus = 0.0f;
};

struct EMBERTRIAL_API FETRelicLoadoutPreview
{
    float MaxHealth = 0.0f;
    float LightAttackBaseDamage = 0.0f;
};

struct EMBERTRIAL_API FETRelicLoadoutPlan
{
    FETRelicLoadout Before;
    FETRelicLoadout After;
    FETRelicModifierTotals BeforeModifiers;
    FETRelicModifierTotals AfterModifiers;
    FETRelicLoadoutPreview BeforePreview;
    FETRelicLoadoutPreview AfterPreview;
};

enum class EETRelicLoadoutResult : uint8
{
    Success,
    NoChange,
    InvalidSlot,
    NotOwned,
    MissingDefinition,
    InvalidDefinition,
    SlotIncompatible,
    DuplicateRelic,
    NumericOverflow,
    InvalidBaseValues,
    InvalidFinalValues
};

namespace ETRelicLoadoutRules
{
// 调用期间借用目录；函数不加载资产、不持有这些对象，也不修改输入。
using FDefinitionMap = TMap<FName, const UETRelicDefinition*>;

EMBERTRIAL_API EETRelicLoadoutResult Evaluate(
    const FETRelicLoadout& Loadout, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, FETRelicModifierTotals& OutTotals);

// Base参数必须不包含装备加成，否则会重复计算。失败输出清空。
EMBERTRIAL_API EETRelicLoadoutResult MakePreview(
    float BaseMaxHealth, float BaseLightAttackDamage,
    const FETRelicModifierTotals& Totals, FETRelicLoadoutPreview& OutPreview);

// RequestedId=None明确表示卸下该槽；只生成方案，不提交库存或应用GE。
// 只有Success携带可用Plan；NoChange / 失败输出均清空，调用方必须检查结果。
EMBERTRIAL_API EETRelicLoadoutResult BuildReplacementPlan(
    const FETRelicLoadout& Current, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, EETRelicSlot RequestedSlot, FName RequestedId,
    float BaseMaxHealth, float BaseLightAttackDamage, FETRelicLoadoutPlan& OutPlan);
}
```

### 4.2 ETRelicLoadout.cpp：资格、汇总、预览与候选方案

路径：`Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp`。先复制输入再清输出，允许连续用上一份Plan.After生成下一份；不把旧槽位加成重复累计。

```cpp
#include "Inventory/ETRelicLoadout.h"
#include "Inventory/ETRelicDefinition.h"
#include "Math/NumericLimits.h"

namespace ETRelicLoadoutPrivate
{
bool FitsFloat(const double Value)
{
    return FMath::IsFinite(Value) && FMath::Abs(Value) <= TNumericLimits<float>::Max();
}
}

EETRelicLoadoutResult ETRelicLoadoutRules::Evaluate(
    const FETRelicLoadout& Loadout, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, FETRelicModifierTotals& OutTotals)
{
    OutTotals = {};
    if (!Loadout.SlotA.IsNone() && Loadout.SlotA == Loadout.SlotB)
    {
        return EETRelicLoadoutResult::DuplicateRelic;
    }

    // 双精度中间和避免float加法先溢出；最终仍输出项目使用的float。
    double HealthAdditive = 0.0;
    double AttackAdditive = 0.0;
    const FName Ids[] = {Loadout.SlotA, Loadout.SlotB};
    const EETRelicSlot Slots[] = {EETRelicSlot::First, EETRelicSlot::Second};
    for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Ids)); ++Index)
    {
        const FName Id = Ids[Index];
        if (Id.IsNone())
        {
            continue;
        }
        if (!OwnedIds.Contains(Id))
        {
            return EETRelicLoadoutResult::NotOwned;
        }
        const UETRelicDefinition* const* Found = Definitions.Find(Id);
        if (!Found || !IsValid(*Found))
        {
            return EETRelicLoadoutResult::MissingDefinition;
        }
        const UETRelicDefinition* Definition = *Found;
        TArray<FText> Errors;
        if (Definition->GetRelicId() != Id || !Definition->CollectValidationErrors(Errors))
        {
            return EETRelicLoadoutResult::InvalidDefinition;
        }
        if (!Definition->SupportsSlot(Slots[Index]))
        {
            return EETRelicLoadoutResult::SlotIncompatible;
        }
        for (const FETRelicModifier& Modifier : Definition->GetModifiers())
        {
            switch (Modifier.Kind)
            {
            case EETRelicModifierKind::MaxHealth:
                HealthAdditive += Modifier.AdditiveValue;
                break;
            case EETRelicModifierKind::LightAttackBonus:
                AttackAdditive += Modifier.AdditiveValue;
                break;
            default:
                return EETRelicLoadoutResult::InvalidDefinition;
            }
        }
    }
    if (!ETRelicLoadoutPrivate::FitsFloat(HealthAdditive) ||
        !ETRelicLoadoutPrivate::FitsFloat(AttackAdditive))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
    OutTotals.MaxHealthAdditive = static_cast<float>(HealthAdditive);
    OutTotals.LightAttackBonus = static_cast<float>(AttackAdditive);
    return EETRelicLoadoutResult::Success;
}

EETRelicLoadoutResult ETRelicLoadoutRules::MakePreview(
    const float BaseMaxHealth, const float BaseLightAttackDamage,
    const FETRelicModifierTotals& Totals, FETRelicLoadoutPreview& OutPreview)
{
    OutPreview = {};
    if (!FMath::IsFinite(BaseMaxHealth) || BaseMaxHealth < 1.0f ||
        !FMath::IsFinite(BaseLightAttackDamage) || BaseLightAttackDamage <= 0.0f)
    {
        return EETRelicLoadoutResult::InvalidBaseValues;
    }
    const double Health = static_cast<double>(BaseMaxHealth) + Totals.MaxHealthAdditive;
    const double Damage = static_cast<double>(BaseLightAttackDamage) + Totals.LightAttackBonus;
    if (!ETRelicLoadoutPrivate::FitsFloat(Health) || !ETRelicLoadoutPrivate::FitsFloat(Damage))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
    if (Health < 1.0 || Damage <= 0.0)
    {
        return EETRelicLoadoutResult::InvalidFinalValues;
    }
    OutPreview.MaxHealth = static_cast<float>(Health);
    OutPreview.LightAttackBaseDamage = static_cast<float>(Damage);
    return EETRelicLoadoutResult::Success;
}

EETRelicLoadoutResult ETRelicLoadoutRules::BuildReplacementPlan(
    const FETRelicLoadout& Current, const TSet<FName>& OwnedIds,
    const FDefinitionMap& Definitions, const EETRelicSlot RequestedSlot, const FName RequestedId,
    const float BaseMaxHealth, const float BaseLightAttackDamage, FETRelicLoadoutPlan& OutPlan)
{
    // 先复制，允许Current来自上一次OutPlan.After；清输出不会抹掉输入。
    const FETRelicLoadout Before = Current;
    OutPlan = {};
    if (RequestedSlot != EETRelicSlot::First && RequestedSlot != EETRelicSlot::Second)
    {
        return EETRelicLoadoutResult::InvalidSlot;
    }

    FETRelicLoadoutPlan Candidate;
    Candidate.Before = Before;
    EETRelicLoadoutResult Result = Evaluate(Before, OwnedIds, Definitions, Candidate.BeforeModifiers);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }
    Result = MakePreview(BaseMaxHealth, BaseLightAttackDamage,
                         Candidate.BeforeModifiers, Candidate.BeforePreview);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }

    Candidate.After = Before;
    FName& Destination = RequestedSlot == EETRelicSlot::First
                             ? Candidate.After.SlotA : Candidate.After.SlotB;
    Destination = RequestedId;
    if (Candidate.After == Before)
    {
        return EETRelicLoadoutResult::NoChange;
    }
    Result = Evaluate(Candidate.After, OwnedIds, Definitions, Candidate.AfterModifiers);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }
    Result = MakePreview(BaseMaxHealth, BaseLightAttackDamage,
                         Candidate.AfterModifiers, Candidate.AfterPreview);
    if (Result != EETRelicLoadoutResult::Success)
    {
        return Result;
    }
    OutPlan = Candidate;
    return EETRelicLoadoutResult::Success;
}
```

### 4.3 ETRelicLoadoutTests.cpp：规则与失败输出专项

路径：`Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp`。夹具用独立强引用对象，不改正式两资产，也不加载世界 / ASC。先完成第5节friend再编译。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicLoadout.h"
#include "Inventory/ETRelicDefinition.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicLoadoutTest, "EmberTrial.M3.Inventory.LoadoutRules",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETRelicLoadoutTest::RunTest(const FString& Parameters)
{
    using EResult = EETRelicLoadoutResult;
    TStrongObjectPtr<UETRelicDefinition> Life(NewObject<UETRelicDefinition>());
    TStrongObjectPtr<UETRelicDefinition> Edge(NewObject<UETRelicDefinition>());
    auto Configure = [](UETRelicDefinition* Definition, const FName Id,
                        const EETRelicModifierKind Kind, const float Value)
    {
        Definition->RelicId = Id;
        Definition->DisplayName = FText::FromString(TEXT("Fixture"));
        Definition->Description = FText::FromString(TEXT("Loadout rules fixture"));
        FETRelicModifier Modifier;
        Modifier.Kind = Kind;
        Modifier.AdditiveValue = Value;
        Definition->Modifiers = {Modifier};
    };
    const FName LifeId(TEXT("fixture_life"));
    const FName EdgeId(TEXT("fixture_edge"));
    Configure(Life.Get(), LifeId, EETRelicModifierKind::MaxHealth, 20.0f);
    Configure(Edge.Get(), EdgeId, EETRelicModifierKind::LightAttackBonus, 5.0f);
    const TSet<FName> Owned = {LifeId, EdgeId};
    ETRelicLoadoutRules::FDefinitionMap Catalog;
    Catalog.Add(LifeId, Life.Get());
    Catalog.Add(EdgeId, Edge.Get());
    const FETRelicLoadout Empty;
    FETRelicLoadoutPlan Plan;

    // ---- 装入 / 替换 / 卸下与输入不变；预览数值独立给出期望 ----
    auto Build = [&](const FETRelicLoadout& Current, const EETRelicSlot Slot, const FName Id)
    {
        return ETRelicLoadoutRules::BuildReplacementPlan(
            Current, Owned, Catalog, Slot, Id, 100.0f, 25.0f, Plan);
    };
    TestTrue(TEXT("Life plan accepted"), Build(Empty, EETRelicSlot::First, LifeId) == EResult::Success);
    TestTrue(TEXT("Input remains empty"), Empty.SlotA.IsNone() && Empty.SlotB.IsNone());
    TestEqual(TEXT("Before health"), Plan.BeforePreview.MaxHealth, 100.0f);
    TestEqual(TEXT("After health"), Plan.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Life does not increase damage"), Plan.AfterPreview.LightAttackBaseDamage, 25.0f);
    TestTrue(TEXT("Output alias is safe"),
             Build(Plan.After, EETRelicSlot::Second, EdgeId) == EResult::Success);
    const FETRelicLoadout Both = Plan.After;
    TestTrue(TEXT("Two independent slots retained"), Both.SlotA == LifeId && Both.SlotB == EdgeId);
    TestEqual(TEXT("Combined health"), Plan.AfterPreview.MaxHealth, 120.0f);
    TestEqual(TEXT("Combined damage"), Plan.AfterPreview.LightAttackBaseDamage, 30.0f);
    TestTrue(TEXT("Unequip life accepted"), Build(Both, EETRelicSlot::First, NAME_None) == EResult::Success);
    TestEqual(TEXT("Unequip health"), Plan.AfterPreview.MaxHealth, 100.0f);
    TestEqual(TEXT("Other slot damage retained"), Plan.AfterPreview.LightAttackBaseDamage, 30.0f);
    TestTrue(TEXT("Both input unchanged"), Both.SlotA == LifeId && Both.SlotB == EdgeId);
    TestTrue(TEXT("Replacement recomputes instead of adding old bonus"),
             Build(Plan.After, EETRelicSlot::Second, LifeId) == EResult::Success);
    TestEqual(TEXT("Replacement clears old damage bonus"), Plan.AfterPreview.LightAttackBaseDamage, 25.0f);
    TestEqual(TEXT("Replacement adds life once"), Plan.AfterPreview.MaxHealth, 120.0f);

    // ---- 拒绝不复用旧输出，NoChange不是可提交的新方案 ----
    TestTrue(TEXT("Duplicate in two slots rejected"),
             Build(Both, EETRelicSlot::Second, LifeId) == EResult::DuplicateRelic);
    TestTrue(TEXT("Rejected plan cleared"), Plan.After.SlotA.IsNone() && Plan.After.SlotB.IsNone());
    TestTrue(TEXT("Same item same slot is NoChange"),
             Build(Both, EETRelicSlot::Second, EdgeId) == EResult::NoChange);
    TestEqual(TEXT("NoChange output cleared"), Plan.AfterPreview.MaxHealth, 0.0f);
    TestTrue(TEXT("Unknown slot rejected"),
             Build(Empty, static_cast<EETRelicSlot>(255), LifeId) == EResult::InvalidSlot);
    TestTrue(TEXT("Unowned item rejected"),
             Build(Empty, EETRelicSlot::First, FName(TEXT("not_owned"))) == EResult::NotOwned);
    Catalog.Remove(LifeId);
    TestTrue(TEXT("Owned but missing definition rejected"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::MissingDefinition);
    Catalog.Add(LifeId, Life.Get());
    Life->AllowedSlots = {EETRelicSlot::First};
    TestTrue(TEXT("Incompatible slot rejected"),
             Build(Empty, EETRelicSlot::Second, LifeId) == EResult::SlotIncompatible);
    Life->AllowedSlots = {EETRelicSlot::First, EETRelicSlot::Second};
    Life->Modifiers[0].AdditiveValue = 0.0f;
    TestTrue(TEXT("Invalid definition rejected"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::InvalidDefinition);
    Life->Modifiers[0].AdditiveValue = 20.0f;
    FETRelicLoadout Broken;
    Broken.SlotA = LifeId;
    Broken.SlotB = LifeId;
    TestTrue(TEXT("Broken current state is not silently repaired"),
             Build(Broken, EETRelicSlot::Second, NAME_None) == EResult::DuplicateRelic);

    // ---- 合法有符号取舍、非法最终值、NaN基值与合计溢出 ----
    Life->Modifiers[0].AdditiveValue = -10.0f;
    TestTrue(TEXT("Legal tradeoff accepted"), Build(Empty, EETRelicSlot::First, LifeId) == EResult::Success);
    TestEqual(TEXT("Tradeoff preview"), Plan.AfterPreview.MaxHealth, 90.0f);
    Life->Modifiers[0].AdditiveValue = -110.0f;
    TestTrue(TEXT("Final nonpositive health rejected"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::InvalidFinalValues);
    Life->Modifiers[0].AdditiveValue = 20.0f;
    TestTrue(TEXT("NaN base rejected"), ETRelicLoadoutRules::BuildReplacementPlan(
        Empty, Owned, Catalog, EETRelicSlot::First, LifeId,
        std::numeric_limits<float>::quiet_NaN(), 25.0f, Plan) == EResult::InvalidBaseValues);
    Life->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    Edge->Modifiers[0].Kind = EETRelicModifierKind::MaxHealth;
    Edge->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    FETRelicModifierTotals Totals;
    TestTrue(TEXT("Finite inputs cannot overflow result silently"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::NumericOverflow);
    TestEqual(TEXT("Overflow output cleared"), Totals.MaxHealthAdditive, 0.0f);
    return !HasAnyErrors();
}

#endif
```

## 5. 原Definition头只新增一条friend

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Inventory\ETRelicDefinition.h`。

在原来的：

```cpp
private:
    friend class FETRelicDefinitionsTest;
```

后面紧接着补一行，变为：

```cpp
private:
    friend class FETRelicDefinitionsTest;
    friend class FETRelicLoadoutTest;
```

原friend、字段、方法全保留，不新增第二个private区域，不把测试代码写进Definition.cpp。friend让这一个测试类设置**临时对象夹具**，不会给Gameplay / Widget开放修改DataAsset的Setter；它也不会修改正式两份DataAsset。

本章不改Build.cs、Config、Character、GE / GA或两份遗物资源。

## 6. 正常Editor Build / 索引 / Game Build

1. 保存三新文件与Definition.h，确认UE已关闭。
2. PowerShell：

   ```powershell
   Set-Location 'E:\UE_Project\Demo'
   .\Tools\Refresh-CLion.ps1
   ```

3. 成功后CLion按已有流程Reload CMake / Refresh，确认公开Loadout头与两新cpp在Core目标、Definition头能导航。预计Core64 / UI23 / Editor3，仅作辅助计数，不清缓存制造索引。
4. 仍保持UE关闭，Game Development正常构建：

   ```powershell
   & 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
   ```

5. 成功后打开原uproject。没有新资产或Class配置要填写，也不用给Controller挂组件。

遇到C++错误给第一条真正错误 / 文件行号，不只报exit code6；`private member`优先核对第5节friend类名，`generated.h`优先核对完整头文件及正常UHT编译。不要改测量期望值来凑Success。

## 7. 最小验收

### 7.1 两项库存专项一次执行

UE打开、非PIE，在Output Log命令栏执行：

```text
Automation RunTests EmberTrial.M3.Inventory
```

本章预计只匹配两项：

```text
EmberTrial.M3.Inventory.Definitions
EmberTrial.M3.Inventory.LoadoutRules
```

两项都明确Success才继续。Definitions复用两件磁盘资源 / ID注册，LoadoutRules用临时对象验证成功 / 失败输出、替换 / 卸下、预览与数值边界；不创建世界、身体或假UI，不写正式DataAsset。无需重跑旧23项战斗 / HUD测试。

如果只找到旧Definitions，确认新Tests文件完整、文件保存且正常Editor Build后重开UE；如果LoadoutRules失败，贴首条Expected错误，不重写正式DataAsset数值来迎合夹具。

### 7.2 一次短基线（约1分钟）

1. 正式`L_ET_VerticalSlice` Play一次；原生命 / 体力100、治疗3、原HUD正常。
2. 左键、2一次，E / P一种继续方式与移动 / 镜头 / 跳跃正常后Stop。当前仍原25伤害，**没有装备菜单或120 / 30的实机变化**。
3. Save All。无新临时节点 / 按键 / 场景设置需要清理，两DataAsset保持01的合法配置。

不需要重做十次重建 / 全Pawn切换 / 死亡重试或Cook。预览规则没改运行路径，相关真实装备 / 新身体 / 事务失败在C接入后再检查。

## 8. 简短反馈

```text
M3-02A 完成 / 卡在第__节第__步
三新Source / Definition friend保留并新增；Editor Build / 三索引 / Game Build：
Inventory.Definitions / Inventory.LoadoutRules各一次：
正式地图原100 / 原25，左键-2-E-P与基础输入短基线：
Source与原资产Save All；失败或不理解处（无填无）：
完成时分 / 手动代码 / 检查 / 构建等待：
```

成功长日志不用复制；我从本机读取。报错提供第一条错误文字 / 文件行号，IDE找不到文件时给包含完整路径 / Project树的截图。

## 9. 这段代码后续怎样复用

计算输入是已拥有集合、定义目录与当前快照；输出是ID / 数值，不存UObject引用。目录由调用方在计算期间保证对象存活，真正库存域会用自己的UPROPERTY引用持有静态定义，不能将本章临时借用指针当成跨帧所有权。

Evaluate负责资格 / 汇总，MakePreview负责基值 / 最终值，BuildReplacementPlan负责一次槽位变更的前后关系。新槽位结构只支持既定两个槽；同一效果类型在**不同遗物**上可以相加，单件内部同Kind重复仍按01拒绝。先汇总成double并检查float范围，避免有限单项相加溢出；最终MaxHealth≥1、基础轻击>0。

输出成功不等于Gameplay提交成功：本章Plan没有版本 / 身体身份或应用句柄。C接入时必须重验当前库存版本、上下文和ASC资格，再按事务提交，过期方案不能直接施加；失败保留旧库存 / 身体结果。UI将通过业务组件提出请求，不能直接调用纯函数后自己宣布装备成功。

## 10. 下一批与分工

M3-02B先接轻击加成的真实属性 / 统一消费者和受约束装备GE；C再接GameInstance库存、当前身体效果所有权 / 失败处理、NewRun / Retry / Continue与受限PIE入口。C若超出单批手动规模再拆。03才做列表 / 详情与比较，04接祭坛 / 页面输入，不把后续能力提前记为本章完成。

普通C++仍由你手动，复杂排错按当次授权；无新模型 / 图标购买或Blender操作。本章预估不计用户实耗，完成反馈后只补真实时间，验收无问题直接下发下一批。

## 11. 本批 Commit / Push（固定放在最后）

本地81a32c7已含01源码 / Config / 两资产。本轮助手只新增：

```text
Docs/Reports/M3_01_Verification.md
Docs/M3_02A_LoadoutRules.md
```

同步修改：

```text
Docs/AssetManifest.csv
Docs/M3_01_RelicDefinitions.md
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

当前共13份实际文件变化。仅提交本轮文档 / 清单时，建议英文`docs: verify relic definitions and prepare loadout rules`。

中文译文：`docs: 记录遗物定义验收并准备装备方案规则`。

**你实施并验收后**，再提交三新 / 一旧Source与最新报告 / 文档：

```text
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicLoadout.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicDefinition.h
```

实现建议英文`feat: add relic loadout planning and stat previews`。

中文译文：`feat: 新增遗物装备方案与属性预览`。

没有新Content / Config / Build.cs变化，不为本章人为重存旧资产；01已提交实现不重复Stage。先CLion Refresh Changes → Stage最新完整内容（已Stage后又改需再次Stage）→ Review Diff → Commit / Push，由你操作。Raw候选 / 日志 / 生成缓存 / Trace / 包留本地，不Force Add；助手未改Git索引或核验远端。
