# M3-06B1：第三件遗物与数据驱动的代价组合

2026-11-06下发，UE **5.5.4**。[M3-06A独立包已验收](Reports/M3_06A_Verification.md)。2026-11-07实施中：四份测试、Config与新资产已由用户写入；助手已修正新资产ID拼写和重复启动配置，四相关专项通过，详见第5.3节。第6节实机尚待完成，本章未整体验收。

## 1. 这一章做出什么

新增第三件遗物**逆焰刻印**：最大生命 **-20**，每段轻击基础伤害 **+10**。它复用两种已有Modifier、同一个装备GE和祭坛事务，不新增GA / Attribute / Widget，也不为blood_pact这个ID写Gameplay分支。最大生命降低代表承伤代价，攻击增加代表收益，名称和数值均为当前Demo设计。

| 两槽内容 | 最终生命上限 | 第1段轻击基础伤害 |
| --- | ---: | ---: |
| 空槽 | 100 | 25 |
| 仅逆焰刻印 | 80 | 35 |
| 逆焰刻印 + 砺锋残片 | 80 | 40 |
| 逆焰刻印 + 生息余烬 | 100 | 35 |

存活换装仍不治疗，只在上限下降时夹当前生命；新身体仍按最终上限补满。倍率沿用玩家三段的1，敌人保持10伤害。本批先做已有消费者能支持的代价型遗物；后续接体力恢复、破势效率、治疗提升，完整目标仍6件。

四份**既有测试**要调整：会话 / Presenter不再把真实库存数量固定成2；ViewModels明确区分真实目录投影与两项纯值样本；Definitions补查第三件实物。数量断言保留为“每个配置ID都有数据”，没有把2简单换成3。仍42项自动化。

预计手动55–80分钟（代码25–40、资产15–20、阅读与检查15–20），构建 / 重启另记。实际规模仍233 Source / 133cpp；完成后预计Content309→310、来源92→93，验收时核对实物再登记。当前Attempt01只代表已验收的两件版本。

## 2. 在UE中创建原生Data Asset

1. Stop PIE，Content Browser进入 **/Game/EmberTrial/Items/Relics**。
2. 空白处右键 → **Miscellaneous → Data Asset**。Pick Data Asset Class搜索 **ETRelicDefinition**，选原生类 → Select，命名 **DA_ET_Relic_BloodPact**。不要复制带旧ID的资产或创建Blueprint类。
3. 双击新资产，在Details逐项填写：

| 字段 | 值 |
| --- | --- |
| Relic Id | `blood_pact`，直接复制；末尾为pact，小写下划线，无ETRelic:前缀 |
| Display Name | 逆焰刻印 |
| Description | 最大生命 -20；每段轻攻击的基础伤害 +10，再按本段倍率结算。以承伤余量换取输出。 |
| Icon | None，沿用现有占位 |
| Sort Order | 30；原生息10 / 砺锋20保持，默认选择仍生息 |
| Allowed Slots | 2项，Index0=First / Slot A，Index1=Second / Slot B，各一次 |
| Modifiers | **2项**，见下表 |

| 数组项 | Kind下拉选项 | Additive Value |
| --- | --- | ---: |
| Index[0] | Max Health / MaxHealth | **-20.0** |
| Index[1] | Light Attack Bonus / LightAttackBonus | **+10.0** |

4. 若数组已有默认项，先核对数量再添加；两行Kind不同，不能把两个值填在同一行，也不是-0.2 / 0.1百分比。
5. Save。选中新资产 → 右键 **Asset Actions → Validate Assets**（或同名Validate入口），应原生数据校验Valid，无ET-RELIC错误。Data Asset没有Blueprint Compile按钮，不要求Widget校验器的M1标记。
6. 关闭再打开资产，确认ID、两个带正负号的值与两槽均保留，Save All；原两件保持。

## 3. 加入启动拥有项

保存并关闭UE，在CLion打开 **E:/UE_Project/Demo/EmberTrial/Config/DefaultGame.ini**，找到唯一的`[/Script/EmberTrial.ETRelicInventorySubsystem]`段，保留原两行，新增第三行，最终为：

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
+StartingRelics=ETRelic:blood_pact
```

不新增第二个同名段。原AssetManager扫描目录 / AlwaysCook保持；新资产在原目录内，原规则负责注册与后续Cook。本章不打包，不改个人ini。启动拥有不等于自动装备，新会话仍空槽。

## 4. 修改四份既有测试

下面每项给出准确文件、查找片段与替换片段，**只替换标明位置，其他代码保留**。不创建新文件。两个UI测试仍在EmberTrialUI/Tests，两个库存测试仍在EmberTrial/Tests。

下发前助手已复用UE / MSVC参数对四份本地候选做仅编译检查；用户随后写入正式工程。2026-11-07的四项实际运行结果见第5.3节，不再停留于候选编译结论。

### EmberTrial/Source/EmberTrial/Tests/ETRelicInventorySessionTests.cpp

**修改1：** 查找下列完整片段，只替换此处。

原代码：

```cpp
    TestEqual(TEXT("Two starting owned IDs"), Snapshot.OwnedIds.Num(), 2);
```

替换为：

```cpp
    // 启动拥有项由配置决定；未来增加遗物不应破坏会话 / NewRun契约。
    TSet<FName> ExpectedStartingIds;
    for (const FPrimaryAssetId& Id : Inventory->StartingRelics)
    {
        TestTrue(TEXT("Configured primary type"), Id.PrimaryAssetType == FPrimaryAssetType(TEXT("ETRelic")));
        TestFalse(TEXT("Starting IDs are unique"), ExpectedStartingIds.Contains(Id.PrimaryAssetName));
        ExpectedStartingIds.Add(Id.PrimaryAssetName);
        TestTrue(TEXT("Configured ID is owned"), Snapshot.OwnedIds.Contains(Id.PrimaryAssetName));
        TestNotNull(TEXT("Configured definition cached"), Inventory->FindDefinition(Id.PrimaryAssetName));
    }
    TestEqual(TEXT("All configured starting IDs owned"), Snapshot.OwnedIds.Num(), ExpectedStartingIds.Num());
```

**修改2：** 查找下列完整片段，只替换此处。

原代码：

```cpp
    TestTrue(TEXT("NewRun keeps starting ownership, clears slots"), Snapshot.OwnedIds.Num() == 2 &&
             Snapshot.Equipped.SlotA.IsNone() && Snapshot.Equipped.SlotB.IsNone());
```

替换为：

```cpp
    TestTrue(TEXT("NewRun keeps starting ownership, clears slots"),
             Snapshot.OwnedIds.Num() == ExpectedStartingIds.Num() &&
             Snapshot.Equipped.SlotA.IsNone() && Snapshot.Equipped.SlotB.IsNone());
    for (const FName Id : ExpectedStartingIds)
    {
        TestTrue(TEXT("NewRun preserves each configured ID"), Snapshot.OwnedIds.Contains(Id));
    }
```


### EmberTrial/Source/EmberTrialUI/Tests/ETRelicViewModelsTests.cpp

**修改1：** 查找下列完整片段，只替换此处。

原代码：

```cpp
    const FETRelicInventorySnapshot Original = Inventory->GetSnapshot();
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    FETRelicListQuery Query;
    TArray<FETRelicListRow> Rows;
```

替换为：

```cpp
    const FETRelicInventorySnapshot DomainBefore = Inventory->GetSnapshot();
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    FETRelicListQuery Query;
    TArray<FETRelicListRow> Rows;
    if (!TestTrue(TEXT("Entire configured inventory projects"),
                  ETRelicPresentation::BuildRows(DomainBefore, Inventory, Query, Rows) == EResult::Success) ||
        !TestEqual(TEXT("One row for every configured item"), Rows.Num(), DomainBefore.OwnedIds.Num()) ||
        !TestTrue(TEXT("Reference items remain owned"),
                  DomainBefore.OwnedIds.Contains(LifeId) && DomainBefore.OwnedIds.Contains(EdgeId)))
    {
        return false;
    }
    // 以下是独立的两项纯值样本，专测既有排序 / 搜索 / 过滤；不改真实库存。
    FETRelicInventorySnapshot Original = DomainBefore;
    Original.OwnedIds = {LifeId, EdgeId};
    Original.Equipped = {};
```

**修改2：** 查找下列完整片段，只替换此处。

原代码：

```cpp
TEXT("Two actual owned items")
```

替换为：

```cpp
TEXT("Two explicit projection sample items")
```

**修改3：** 查找下列完整片段，只替换此处。

原代码：

```cpp
Inventory->GetSnapshot().Revision == Original.Revision && Inventory->GetSnapshot().Equipped == Original.Equipped
```

替换为：

```cpp
Inventory->GetSnapshot().Revision == DomainBefore.Revision && Inventory->GetSnapshot().Equipped == DomainBefore.Equipped
```


### EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp

**修改1：** 查找下列完整片段，只替换此处。

原代码：

```cpp
    TestEqual(TEXT("Two stable items"), Presenter->GetItems().Num(), 2);
```

**删除上面这一整行，不填替换代码。** 下一项会统一验证实际拥有的全部条目。

**修改2：** 查找下列完整片段，只替换此处。

原代码：

```cpp
    int32 ViewEvents = 0;
```

替换为：

```cpp
    const FETRelicInventorySnapshot InitialInventory = Inventory->GetSnapshot();
    const int32 ExpectedItemCount = InitialInventory.OwnedIds.Num();
    TestEqual(TEXT("One stable item for every owned ID"), Presenter->GetItems().Num(), ExpectedItemCount);
    for (const FName Id : InitialInventory.OwnedIds)
    {
        TestNotNull(TEXT("Every configured item is cached"), Presenter->FindItem(Id));
    }
    int32 ViewEvents = 0;
```

**修改3：** 查找下列完整片段，只替换此处。

原代码：

```cpp
Presenter->GetItems().Num() == 2 && Presenter->GetSnapshot().SelectedId == LifeId
```

替换为：

```cpp
Presenter->GetItems().Num() == ExpectedItemCount && Presenter->GetSnapshot().SelectedId == LifeId
```


### EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp

**修改1：** 查找下列完整片段，只替换此处。

原代码：

```cpp
    TestTrue(TEXT("Keen Edge registered"), RegisteredIds.Contains(FName(TEXT("keen_edge"))));
    return !HasAnyErrors();
```

替换为：

```cpp
    TestTrue(TEXT("Keen Edge registered"), RegisteredIds.Contains(FName(TEXT("keen_edge"))));

    // 实物配置合同：一件遗物可组合两种已支持效果，不为ID增加专用玩法分支。
    const UETRelicDefinition* Tradeoff = nullptr;
    for (const UETRelicDefinition* Definition : Catalog)
    {
        if (Definition->GetRelicId() == FName(TEXT("blood_pact")))
        {
            Tradeoff = Definition;
            break;
        }
    }
    if (!TestNotNull(TEXT("Blood Pact saved definition registered"), Tradeoff))
    {
        return false;
    }
    TestTrue(TEXT("Tradeoff supports both slots"), Tradeoff->SupportsSlot(EETRelicSlot::First) &&
             Tradeoff->SupportsSlot(EETRelicSlot::Second));
    const TArray<FETRelicModifier>& Modifiers = Tradeoff->GetModifiers();
    TestEqual(TEXT("Tradeoff has exactly two effects"), Modifiers.Num(), 2);
    const FETRelicModifier* HealthCost = Modifiers.FindByPredicate([](const FETRelicModifier& Modifier)
    { return Modifier.Kind == EETRelicModifierKind::MaxHealth; });
    const FETRelicModifier* AttackGain = Modifiers.FindByPredicate([](const FETRelicModifier& Modifier)
    { return Modifier.Kind == EETRelicModifierKind::LightAttackBonus; });
    TestTrue(TEXT("Tradeoff health cost is minus20"), HealthCost && HealthCost->AdditiveValue == -20.0f);
    TestTrue(TEXT("Tradeoff attack gain is plus10"), AttackGain && AttackGain->AdditiveValue == 10.0f);
    return !HasAnyErrors();
```

## 5. 正常编译与四个相关专项

### 5.1 Editor Build一次

UE保持关闭，保存四份测试 / Config，在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrialEditor Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

无新增C++文件、反射声明或依赖，不需要重建CLion CodeModel / 清缓存；本次只改测试源码，不强制重复Game Build，未来UAT正常构建Game。编译错误发第一条error与行号，不继续实机。

### 5.2 四个专项各一次

**本轮已由助手补修后复跑，四项全部Success，无需再次执行。** 下列命令保留供查阅。

打开UE 5.5.4，非Play，Output Log底部Cmd依次执行，等队列结束：

```text
Automation RunTests EmberTrial.M3.Inventory.Definitions
Automation RunTests EmberTrial.M3.Inventory.Session
Automation RunTests EmberTrial.UI.Relics.ViewModels
Automation RunTests EmberTrial.UI.Relics.Presenter
```

均应Success。Definitions含第三件实物配置；另三项验证全部配置ID投影、NewRun保留拥有项、旧两项规则与Presenter解绑。失败只补相关项，不重跑全42项或搭临时蓝图。

### 5.3 逆焰刻印ID拼写补修（已直接完成）

2026-11-07，Definitions在原第145行提示“Blood Pact saved definition registered”为空。检查发现新资产虽然文件名正确，但**Relic Id实际保存为`blood_pack`，应为`blood_pact`**。测试按稳定ID查找，合法但拼错的ID无法匹配；改资源文件名不能解决这类问题。

助手已通过UE资产编辑器将Relic Id改为`blood_pact`，提交字段后点击资产Save按钮正常保存；两槽、Sort30、MaxHealth -20 / LightAttackBonus +10保持。同时清除DefaultGame.ini中重复的life_ember / keen_edge记录，最终只保留本章第3节的三条启动拥有项。重复配置不是这条空指针断言的原因，但书面配置也应保持唯一。

随后关闭编辑器，以全新进程从磁盘加载，复跑本章四个专项：**00:06完成，4 Success / 0 Fail / 0 Warning / 0 NotRun，退出码0**。Definitions验证第三件已注册、双效果数值与双槽；其余三项同时通过。未修改C++或断言预期，因此没有额外Editor Build；也未跑全42项或重新打包。

原资产 / 配置 / 失败日志备份及修复后证据位于：

```text
E:/UE_Project/Demo/Artifacts/M3_06B1/IdRepair/
  Before_Automation.log
  DA_ET_Relic_BloodPact.before.uasset
  DefaultGame.before.ini
  Automation.log
  AutomationReport/index.json
  Summary.json
```

此问题已关闭，**重新打开UE后直接从第6节实机继续**，无需重跑第5节。资产与配置的修改已经保存在正式工程，随本章第8节实现清单一并Commit / Push；备份、日志与个人配置仍留本地。本次自动化成功不替代祭坛三行 / 真实40伤害 / 存活换装不治疗的实机检查。

## 6. 正式祭坛一条短检查（约5–10分钟）

### 6.1 三行与预览无副作用

1. 正式地图新Play，空槽生命100/100。到祭坛按E，列表应3行且中文可读，第三行逆焰刻印在原两件之后。Owned=3日志由助手提取。
2. 选逆焰刻印 → 比较槽A。预览最大生命100→80、轻击25→35；**点击确认前HUD仍100/100**。
3. 确认装备槽A，HUD应80/80，显示槽A已装备。这是上限夹限，不当作敌人真实命中。

### 6.2 组合与真实伤害

1. 选砺锋残片 → 比较槽B → 确认装备，当前轻击40，生命仍80/80。
2. 继续游戏，面向存活训练目标，左键单击一段。助手核对本次实际伤害40；只打一次，不搭临时血条或输入。
3. 回祭坛，将**槽B的砺锋残片换为生息余烬**。上限回100，当前生命仍80，即80/100；轻击35。不同遗物的修饰符正确求和，没有回血或旧攻击加成残留。
4. 普通P显示三件 / 两槽，保持只读；不重复整套搜索 / 焦点旧矩阵。

### 6.3 收尾与最短基线

1. 回祭坛依次卸槽A / B；最终两槽空，当前生命可保留80/100、轻击回25。中途卸掉代价项，上限可能先变120，仍不能免费治疗。
2. Stop → 新Play，空槽生命100/100、拥有三件。移动 / 转向、P继续，以及2或Q任选一项短观察，再Stop。
3. Save All，第三件资产与第三条StartingRelics作为正式配置保留。临时状态只在PIE，不修改身体默认100 / 敌人10 / Debug False。

本批不重跑Retry / Continue、完整战斗矩阵或新包；旧事务 / 新身体语义沿用刚通过的06A，出现直接相关问题再窄复验。

## 7. 完成反馈

```text
M3-06B1 完成
第三件原生Data Asset / ID与双Modifier / 两槽 / Sort30 / Valid-Save：
DefaultGame.ini唯一三条StartingRelics；四份既有测试已保存：
关闭UE后Editor Build；Definitions / Session / ViewModels / Presenter：
真实三行 / 预览无副作用 / 逆焰单件80与35：
逆焰+砺锋80与真实40；换生息后80/100与35、不治疗：
普通P只读 / 祭坛卸装回25 / Stop再开空槽100与三件：
Source-Config-资产Save All / 最短基线；失败或不理解处（无填无）：
完成时分 / 手动代码 / 资产配置 / 阅读 / 检查 / 构建等待：
```

助手复核实物、测试与日志后再登记来源与验收结论。Details / UI问题发截图，编译与自动化发首条文字error；不复制长成功日志。

## 8. Commit / Push（固定放在最后）

**当前只提交06A收尾与本章指南：**

```text
Docs/Reports/M3_06A_Verification.md
Docs/M3_06A_IntegratedPackage.md
Docs/M3_06B1_TradeoffRelic.md
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

- 英文：`docs: verify M3 package and prepare tradeoff relic`
- 专业中文：`docs: 完成M3开发包验收并准备代价型遗物`

**实施完成后**再Stage以下真实增量，不提前创建空资产 / 报告或提交候选：

```text
EmberTrial/Content/EmberTrial/Items/Relics/DA_ET_Relic_BloodPact.uasset
EmberTrial/Config/DefaultGame.ini
EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicInventorySessionTests.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETRelicViewModelsTests.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp
```

- 英文：`feat: add a data-driven tradeoff relic`
- 专业中文：`feat: 新增数据驱动的代价型遗物`

验收时再追加实际来源清单 / 报告 / 状态变化。c03948c三份受伤测试修复已在本地，不重复Stage。包 / 日志 / JSON / 个人ini及Artifacts候选留本地；Stage后再编辑要重新Stage → Review → Commit / Push，Git由你操作，助手未核验远端。
