# M3-06B2-03：第四件治疗遗物与装备比较

2026-11-07下发，UE **5.5.4**。[B2-02已验收](Reports/M3_06B2_02_Verification.md)，无待补。本章由你手动完成，**五份既有源码 + 一处Config、一件新Data Asset、原Panel高度调整**；当前只有Artifacts候选，正式文件 / 资产尚未修改。

## 1. 本章效果与边界

新增第四件**回生护符**，稳定ID为`renewal_charm`：正式Q固定治疗量**+10**，基础40不变，所以通常一次50；仍消耗1次，次数上限3不变。

| 两槽内容 | 最大生命 | 首段轻击 | 装备治疗加成 | 默认Q请求治疗量 |
| --- | ---: | ---: | ---: | ---: |
| 空槽 | 100 | 25 | 0 | 40 |
| 仅回生 | 100 | 25 | 10 | 50 |
| 回生 + 生息 | 120 | 25 | 10 | 50 |
| 回生 + 砺锋 | 100 | 30 | 10 | 50 |
| 回生 + 逆焰 | 80 | 35 | 10 | 50 |

实际回血仍夹到当前上限；装备不立即回血，也不增加次数。UI第三行叫**“装备治疗加成”**，显示0→10或10→0；不显示40→50，因为这个字段只比较两槽贡献，不混入GA基础值或未来其他临时效果。Q的真实结算沿用01 / 02，UI不重算公式。

列表仍是同一UListView、两槽、原确认权限和15个绑定；复用PreviewText增加一行，不加Tick / Timer / 额外属性订阅。装备变化由现有OnEquipmentChanged刷新。

预计手动60–90分钟（代码与理解30–45、资产 / Designer15–20、检查记录10–20），构建等待另记。Source仍233 / 133cpp、42测试；完成后预计Content310→311、来源93→94，原Panel来源行更新实际指纹。现在不提前登记未创建的资产，不重打包。

## 2. 先在UE创建第四件原生Data Asset

### 2.1 创建与逐项配置

1. 当前02原生校验已经支持HealingBonus。Stop PIE，Content Browser进入**/Game/EmberTrial/Items/Relics**。
2. 空白右键 → **Miscellaneous → Data Asset**，类选择原生**ETRelicDefinition** → Select。命名**DA_ET_Relic_RenewalCharm**；不是Blueprint Class，也不复制旧遗物后遗留旧ID。
3. 双击新资产，在Details逐项填表。ID请复制，不加ETRelic:前缀：

| 字段 | 值 |
| --- | --- |
| Relic Id | `renewal_charm` |
| Display Name | 回生护符 |
| Description | 正式Q治疗固定增加10点；生效时消耗1次，次数上限不变。 |
| Icon | None，原占位路线 |
| Sort Order | 40；旧三件10 / 20 / 30保持 |
| Allowed Slots | 2项：First / Slot A、Second / Slot B，各一次 |
| Modifiers | **1项**，Kind=**Healing Bonus**，Additive Value=**10.0** |

4. 不把+10填到InitialHealth、HealAmount或HealingResource次数；这是一件遗物的固定数值，不是10%。

### 2.2 保存与校验

1. Save新资产，Content Browser选中 → 右键**Asset Actions → Validate Assets**。应原生Valid，无ET-RELIC错误；Data Asset没有Compile按钮，不要求Widget的本项目Valid标记。
2. 关闭资产页再打开，核对ID、单个Modifier10、两槽、Sort40保留。Save All。
3. 保存后**关闭UE**，再进行第3–6节源码 / Config修改。此时不Play，不提前检验四行；新拥有配置尚未加入。

## 3. 加入唯一第四条启动拥有配置

**文件：** `E:/UE_Project/Demo/EmberTrial/Config/DefaultGame.ini`。

查找以下唯一片段：

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
+StartingRelics=ETRelic:blood_pact
```

仅替换为：

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
+StartingRelics=ETRelic:blood_pact
+StartingRelics=ETRelic:renewal_charm
```

原AssetManager / Cook目录保持。不要增加第二个同名段或重复旧三行；启动库存拥有四件不等于自动装备，新GI仍空槽。保存Config。

## 4. 原Presenter增加装备治疗比较

### 4.1 ETRelicPresenter.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h`。

**修改1：快照明确保存装备贡献，不是总治疗量。**

查找以下唯一片段：

```cpp
    float AfterAttack = 0.0f;
    bool operator==
```

仅替换为：

```cpp
    float AfterAttack = 0.0f;
    // 只比较两槽装备的固定治疗加成；不包含GA基础值或其他临时效果。
    float BeforeHealingBonus = 0.0f;
    float AfterHealingBonus = 0.0f;
    bool operator==
```

### 4.2 ETRelicPresenter.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp`。

**修改1：仅治疗加成变化同样触发新快照。**

查找以下唯一片段：

```cpp
               BeforeAttack == Other.BeforeAttack && AfterAttack == Other.AfterAttack &&
               bHasCommands
```

仅替换为：

```cpp
               BeforeAttack == Other.BeforeAttack && AfterAttack == Other.AfterAttack &&
               BeforeHealingBonus == Other.BeforeHealingBonus &&
               AfterHealingBonus == Other.AfterHealingBonus &&
               bHasCommands
```

**修改2：可用预览直接投影核心方案，不在UI重算治疗公式。**

查找以下唯一片段：

```cpp
        Data.AfterAttack = Preview.AfterPreview.LightAttackBaseDamage;
```

仅替换为：

```cpp
        Data.AfterAttack = Preview.AfterPreview.LightAttackBaseDamage;
        Data.BeforeHealingBonus = Preview.BeforeModifiers.HealingBonus;
        Data.AfterHealingBonus = Preview.AfterModifiers.HealingBonus;
```

**修改3：已装备分支读取当前身体的自有贡献。**

查找以下唯一片段：

```cpp
        float Bonus = 0.0f;
        const UETGA_LightAttack* Attack = Body->GetLightAttackConfiguration();
        if (Attack && Body->GetGameplayHealthSnapshot(Health, Data.BeforeMax) &&
```

仅替换为：

```cpp
        float Bonus = 0.0f;
        FETRelicModifierTotals Applied;
        const UETGA_LightAttack* Attack = Body->GetLightAttackConfiguration();
        if (Attack && Equipment->GetAppliedModifiers(Applied) &&
            Body->GetGameplayHealthSnapshot(Health, Data.BeforeMax) &&
```

**修改4：已装备前后值相同。**

查找以下唯一片段：

```cpp
            Data.AfterAttack = Data.BeforeAttack;
```

仅替换为：

```cpp
            Data.AfterAttack = Data.BeforeAttack;
            Data.BeforeHealingBonus = Applied.HealingBonus;
            Data.AfterHealingBonus = Applied.HealingBonus;
```

**核对：** 新字段只读Plan.BeforeModifiers / AfterModifiers及组件GetAppliedModifiers；不从ASC总HealingBonus冒充装备贡献。清空状态由结构体零默认值处理，不增加GetGameplayAttributeValueChangeDelegate订阅。

## 5. 原PreviewText显示第三行

### 5.1 ETRelicPanelWidget.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp`。

**修改1：用现有PreviewText显示第三行，原15绑定不变。**

查找以下唯一片段：

```cpp
        PreviewText->SetText(FText::Format(FText::FromString(TEXT("最大生命 {0} → {1}\n首段轻击 {2} → {3}{4}")),
            FText::AsNumber(Data.BeforeMax, &NumberFormat), FText::AsNumber(Data.AfterMax, &NumberFormat),
            FText::AsNumber(Data.BeforeAttack, &NumberFormat), FText::AsNumber(Data.AfterAttack, &NumberFormat),
            Data.PreviewState == EETRelicPreviewState::AlreadyEquipped
                ? FText::FromString(TEXT("\n该槽已装备此遗物")) : FText::GetEmpty()));
```

仅替换为：

```cpp
        PreviewText->SetText(FText::Format(
            NSLOCTEXT("ETRelics", "EquipmentStatsWithHealing",
                      "最大生命 {0} → {1}\n首段轻击 {2} → {3}\n装备治疗加成 {4} → {5}{6}"),
            FText::AsNumber(Data.BeforeMax, &NumberFormat),
            FText::AsNumber(Data.AfterMax, &NumberFormat),
            FText::AsNumber(Data.BeforeAttack, &NumberFormat),
            FText::AsNumber(Data.AfterAttack, &NumberFormat),
            FText::AsNumber(Data.BeforeHealingBonus, &NumberFormat),
            FText::AsNumber(Data.AfterHealingBonus, &NumberFormat),
            Data.PreviewState == EETRelicPreviewState::AlreadyEquipped
                ? FText::FromString(TEXT("\n该槽已装备此遗物")) : FText::GetEmpty()));
```

## 6. 两份既有测试补充实物与比较

### 6.1 ETRelicDefinitionsTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp`。

**修改1：新增第四件实物断言，保持目录可扩展。**

查找以下唯一片段：

```cpp
    TestTrue(TEXT("Tradeoff attack gain is plus10"), AttackGain && AttackGain->AdditiveValue == 10.0f);
    return !HasAnyErrors();
```

仅替换为：

```cpp
    TestTrue(TEXT("Tradeoff attack gain is plus10"), AttackGain && AttackGain->AdditiveValue == 10.0f);

    const UETRelicDefinition* Renewal = nullptr;
    for (const UETRelicDefinition* Definition : Catalog)
    {
        if (Definition->GetRelicId() == FName(TEXT("renewal_charm")))
        {
            Renewal = Definition;
            break;
        }
    }
    if (!TestNotNull(TEXT("Renewal Charm saved definition registered"), Renewal))
    {
        return false;
    }
    TestTrue(TEXT("Renewal supports both slots"), Renewal->SupportsSlot(EETRelicSlot::First) &&
             Renewal->SupportsSlot(EETRelicSlot::Second));
    TestEqual(TEXT("Renewal sort order forty"), Renewal->GetSortOrder(), 40);
    const TArray<FETRelicModifier>& HealingModifiers = Renewal->GetModifiers();
    if (TestEqual(TEXT("Renewal has exactly one effect"), HealingModifiers.Num(), 1))
    {
        TestTrue(TEXT("Renewal healing bonus is plus ten"),
                 HealingModifiers[0].Kind == EETRelicModifierKind::HealingBonus &&
                 HealingModifiers[0].AdditiveValue == 10.0f);
    }
    return !HasAnyErrors();
```

### 6.2 ETRelicPresenterTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp`。

**修改1：测试夹具提供真实治疗配置与属性依赖。**

查找以下唯一片段：

```cpp
#include "Abilities/ETGA_LightAttack.h"
```

仅替换为：

```cpp
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Heal.h"
#include "Attributes/ETCombatAttributeSet.h"
```

**修改2：两具隔离身体均支持治疗加成预检，不播放新动作。**

查找以下唯一片段：

```cpp
            Body->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
```

仅替换为：

```cpp
            Body->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
            Body->HealAbilityClass = UETGA_Heal::StaticClass();
```

**修改3：独立第三具身体验证治疗装备恢复，不消耗旧换控用例的新身体前提。**

查找以下唯一片段：

```cpp
    AEmberTrialCharacter* OtherBody = SpawnBody(400);
```

仅替换为：

```cpp
    AEmberTrialCharacter* OtherBody = SpawnBody(400);
    AEmberTrialCharacter* HealingRestoreBody = SpawnBody(800);
```

**修改4：新增身体同样检查有效性。**

查找以下唯一片段：

```cpp
        !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("Engine"), GEngine))
```

仅替换为：

```cpp
        !TestNotNull(TEXT("Healing restore body"), HealingRestoreBody) ||
        !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("Engine"), GEngine))
```

**修改5：在原View事件计数之前加入第四件与比较断言。**

查找以下唯一片段：

```cpp
    const FDelegateHandle ViewHandle = Presenter->OnChanged.AddLambda([&](const FETRelicPanelData&) { ++ViewEvents; });
```

仅替换为：

```cpp
    // ---- 正式第四件：比较无副作用、已装备显示、外部贡献隔离与关闭重建 ----
    {
        const FName HealingId(TEXT("renewal_charm"));
        UETRelicListItem* HealingItem = Presenter->FindItem(HealingId);
        if (!TestNotNull(TEXT("Configured healing item"), HealingItem))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        const FGameplayAttribute Healing = UETCombatAttributeSet::GetHealingBonusAttribute();
        const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
        const uint64 BeforeHealingPreview = Inventory->GetSnapshot().Revision;
        const int32 EffectsBefore = ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
        TestTrue(TEXT("Select saved healing relic"), Presenter->SelectId(HealingId));
        TestEqual(TEXT("Before equipment healing zero"), Presenter->GetSnapshot().BeforeHealingBonus, 0.0f);
        TestEqual(TEXT("Proposed equipment healing ten"), Presenter->GetSnapshot().AfterHealingBonus, 10.0f);
        TestEqual(TEXT("Preview leaves real healing zero"), ASC->GetNumericAttribute(Healing), 0.0f);
        TestTrue(TEXT("Healing preview keeps revision"), Inventory->GetSnapshot().Revision == BeforeHealingPreview);
        TestEqual(TEXT("Healing preview adds no effects"),
                  ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), EffectsBefore);

        // 去重合同也必须识别新增字段，不能吞掉只有治疗数值变化的快照。
        FETRelicPanelData Changed = Presenter->GetSnapshot();
        Changed.BeforeHealingBonus += 1.0f;
        TestFalse(TEXT("Before healing participates in equality"), Changed == Presenter->GetSnapshot());
        Changed = Presenter->GetSnapshot();
        Changed.AfterHealingBonus += 1.0f;
        TestFalse(TEXT("After healing participates in equality"), Changed == Presenter->GetSnapshot());

        ASC->SetNumericAttributeBase(Health, 60.0f);
        TestTrue(TEXT("Real saved healing relic equips"), Equipment->TryEquip(
            EETRelicSlot::First, HealingId, Inventory->GetSnapshot().Revision).Result ==
                EETRelicInventoryResult::Success);
        TestTrue(TEXT("Healing already equipped state"),
                 Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::AlreadyEquipped);
        TestEqual(TEXT("Equipped healing before ten"), Presenter->GetSnapshot().BeforeHealingBonus, 10.0f);
        TestEqual(TEXT("Equipped healing after ten"), Presenter->GetSnapshot().AfterHealingBonus, 10.0f);
        TestEqual(TEXT("Real healing Current ten"), ASC->GetNumericAttribute(Healing), 10.0f);
        TestEqual(TEXT("Equipping does not heal"), ASC->GetNumericAttribute(Health), 60.0f);

        // 额外基础加成不是装备贡献；重新打开观察也不能把13误标为装备加成。
        ASC->SetNumericAttributeBase(Healing, 3.0f);
        Presenter->StartObserving(Subsystem.Get());
        TestTrue(TEXT("Reopen keeps stable healing item"), Presenter->FindItem(HealingId) == HealingItem);
        TestEqual(TEXT("Total healing includes external source"), ASC->GetNumericAttribute(Healing), 13.0f);
        TestEqual(TEXT("Reopened equipment contribution remains ten"),
                  Presenter->GetSnapshot().BeforeHealingBonus, 10.0f);
        PC->Possess(HealingRestoreBody);
        UAbilitySystemComponent* RestoredASC = HealingRestoreBody->GetAbilitySystemComponent();
        TestTrue(TEXT("New healing body ready without manual refresh"),
                 Presenter->GetSnapshot().State == EETRelicPanelState::Ready);
        TestEqual(TEXT("New body restores committed healing ten"),
                  RestoredASC->GetNumericAttribute(Healing), 10.0f);
        TestEqual(TEXT("New body exposes healing comparison ten"),
                  Presenter->GetSnapshot().BeforeHealingBonus, 10.0f);
        PC->Possess(Body);
        TestEqual(TEXT("Return keeps original wounded health"),
                  ASC->GetNumericAttribute(Health), 60.0f);
        TestEqual(TEXT("Return retains independent external contribution"),
                  ASC->GetNumericAttribute(Healing), 13.0f);
        TestTrue(TEXT("Real healing unequip"), Equipment->TryEquip(
            EETRelicSlot::First, NAME_None, Inventory->GetSnapshot().Revision).Result ==
                EETRelicInventoryResult::Success);
        TestEqual(TEXT("Unequip preserves external healing"), ASC->GetNumericAttribute(Healing), 3.0f);
        TestEqual(TEXT("Unequip comparison returns zero"),
                  Presenter->GetSnapshot().BeforeHealingBonus, 0.0f);
        TestEqual(TEXT("Unequip does not heal"), ASC->GetNumericAttribute(Health), 60.0f);

        // 恢复原生命 / 攻击场景的前提，不删除换控 / 命令等既有断言。
        ASC->SetNumericAttributeBase(Healing, 0.0f);
        ASC->SetNumericAttributeBase(Health, 100.0f);
        Presenter->SelectId(LifeId);
    }
    const FDelegateHandle ViewHandle = Presenter->OnChanged.AddLambda([&](const FETRelicPanelData&) { ++ViewEvents; });
```

**修改6：NoPawn同样清空新增比较字段。**

查找以下唯一片段：

```cpp
        Presenter->GetSnapshot().AfterMax == 0.0f);
```

仅替换为：

```cpp
        Presenter->GetSnapshot().AfterMax == 0.0f &&
        Presenter->GetSnapshot().BeforeHealingBonus == 0.0f &&
        Presenter->GetSnapshot().AfterHealingBonus == 0.0f);
```

Presenter新增第三具隔离身体专测治疗装备恢复，保留原OtherBody第一次恢复上限120的前提；返回原身体仍受伤，外部基础加成3独立保留。所有旧查询 / 两槽 / 生命 / 攻击 / 命令 / 结束断言保留，仍同一个测试入口。测试不会写入正式资产或修改你的默认配置。

## 7. 正常Editor / Game Build与索引

五份源码与Config保存，UE已关闭。PowerShell整段复制：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先正常Editor Build，再刷新三模块索引。CLion提示时Reload CMake；Source仍Core89 / UI41 / Editor3共133cpp，无新增文件，不清缓存 / 重建工程。

然后同一窗口进行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两项成功再打开工程。下发前已对四份候选cpp完成仅编译预检；预检复用旧反射声明，不替代本节UHT / 链接或后面的运行验证。不编辑任何.generated.h。

## 8. 为第三行留出空间（一个既有Widget）

1. Content Browser进入**/Game/EmberTrial/ui/Inventory**，打开**WBP_ETRelicPanel** → **Designer**。
2. Hierarchy选最上面的根**SizeBox：PanelSize**。Details搜索**Height Override**，勾选并将原460改为**520**；若你当前已高于520，保留更大值并在反馈注明实际高度。Width Override保持620。修改的是根SizeBox，不是List Entry高度或PreviewText字体。
3. Hierarchy选**PreviewText**，核对Is Variable勾选、Font16 / 中文字体、Auto Wrap Text=True；它的Vertical Box Slot仍**Fill 1**、水平Fill、Top Padding12。无需新增第三个TextBlock、改名称、创建Binding或Graph节点。
4. Panel的原15个BindWidget / 父类 / Never与确认按钮保持。Compile → Save，再Validate本Panel，应有本项目Valid。
5. 打开现有宿主暂停Widget **/Game/EmberTrial/ui/Menu/WBP_PauseMenu**，只Compile检查现有RelicPanel实例引用，不创建新宿主或替换Flow；若宿主因此标脏则Save。运行时第三行、已装备第四行及确认按钮应完整，不裁切或重叠。本章布局调整只需要根PanelSize，宿主无需重新排版。

## 9. 四个相关专项（各一次）

保持非Play，在Output Log底部命令行依次执行，上一项Success再执行下一项：

```text
Automation RunTests EmberTrial.M3.Inventory.Definitions
Automation RunTests EmberTrial.M3.Inventory.Session
Automation RunTests EmberTrial.UI.Relics.ViewModels
Automation RunTests EmberTrial.UI.Relics.Presenter
```

Definitions确认第四件已被磁盘注册且单HealingBonus10 / 两槽 / Sort40；Session与ViewModels沿用动态真实目录规则；Presenter验证预览无GE / 版本副作用、已装备值、外部贡献隔离、新身体恢复 / 旧身体不治疗及原流程。

无需重复02六项、全42项 / Cook或新临时蓝图。若第四件注册为空，先核对ID正好为renewal_charm、资产正常Save、Config只四条并已重启；不要通过删断言或改成optional规避。报错发测试名与首条文字error，其余日志由我读取。

## 10. 一条短实机路线（约5–10分钟）

### 10.1 四行、第三比较行与无副作用

1. 正式地图新Play，生命100/100、治疗3/3、两槽空；到祭坛按E，真实列表四行，回生护符在第四。
2. 选择回生护符、比较槽A：最大生命100→100、首段轻击25→25、**装备治疗加成0→10**。确认前HUD仍100 / 3次，切槽B时仍是同样候选，不应用任何GE。
3. 回槽A并确认。应显示已装备；重新选此槽此物，第三行**10→10**，生命仍100、次数仍3。各行和确认 / 卸装按钮完整可见。

### 10.2 正式Q真实治疗50

1. 继续游戏，站在安全位置，已有数字4造成两次25伤害到50；等受击结束并站定。
2. 单按Q，通知前不提前加血 / 扣次，生效后**100/100、2/3**。日志应Healing applied Amount=50.0，不要求你截精确0.5秒，由我读取。
3. 普通P打开应四件、第三比较行正常且仍只读；按原方式继续，输入恢复。

### 10.3 替换后恢复40、无免费治疗与新会话

1. 回祭坛，把槽A回生护符替换为**生息余烬**：预览治疗加成10→0；确认后生命应**100/120**，不能因换装免费升满。第三行0→0 / 新候选按当前装备计算。
2. 继续游戏，数字4两次到**50/120**，站定Q后应**90/120、1/3**，日志Amount=40.0，证明旧治疗加成已经移除。
3. 回祭坛卸下生息，最终空两槽，生命可保留90/100；Stop → 新Play，应新100 / 3次、空槽、拥有四件。移动 / 转向、P继续与左键或2任选一项短查，再Stop。
4. Source、Config、新Data Asset及Panel正常Save All；保留正式四条拥有项、回生+10、原GA基础40 / 次数3，所有其他默认保持。无帧率、导航或临时节点要恢复。

换身体的治疗加成恢复由Presenter专项负责，本批不重复手动Retry / Continue、所有取消点与设置矩阵。若敌人打断Q，这次取消按原规则正常处理，移到安全位置重做该步，不改角色默认。

## 11. 完成反馈

```text
M3-06B2-03 完成
五既有Source / Config四条 / 新原生回生资产；双Build / 三索引：
回生ID renewal_charm / 单HealingBonus10 / 两槽 / Sort40 / Valid-Save：
Panel实际高度 / PreviewText原配置 / Compile-Save-Valid：
Definitions / Session / ViewModels / Presenter各一次：
四行 / 预览无副作用 / 0→10与已装备10→10、布局：
正式Q50到100与2次；替换生息100/120不治疗 / Q40到90与1次：
普通P只读 / 卸装 / Stop新100-3次与空槽四件 / 最短输入：
Source-Config-资产Save All；失败或布局问题（无填无）：
完成时分 / 手动代码与理解 / 资产Designer / 检查记录 / 构建等待：
```

本章只创建一件数据资产；不因同机制复制GE / GA / Widget。第三行是装备贡献，实际治疗量仍由Gameplay权威计算。来源94与旧Panel新指纹在验收核对后登记，不预填已完成。

## 12. Commit / Push（固定放在最后）

本次B2-02收尾可提交以下**11份文档**。本地aa19954已有21份Core实现，无需重复Stage。03只有Artifacts候选，尚未写入正式源码 / Config / 资产。

```text
Docs/Reports/M3_06B2_02_Verification.md
Docs/M3_06B2_02_HealingEquipmentPipeline.md
Docs/M3_06B2_03_HealingRelicAndComparison.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文：`docs: verify healing equipment pipeline and plan relic UI`
- 专业中文：`docs: 完成治疗装备管线验收并规划遗物界面`

原日志 / JSON / 编译预检 / 候选、包与个人ini留Artifacts本地。Stage后继续编辑需重新Stage → Review → Commit / Push，由你操作；助手未操作索引、提交或远端。

**你实施并验收本小节后，再提交以下实际改动：**

```text
EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h
EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp
EmberTrial/Config/DefaultGame.ini
EmberTrial/Content/EmberTrial/Items/Relics/DA_ET_Relic_RenewalCharm.uasset
EmberTrial/Content/EmberTrial/ui/Inventory/WBP_ETRelicPanel.uasset
```

原暂停宿主若Compile后确实改变内容，再追加`EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset`；不为凑清单提交其他资产。验收时补真实来源 / 报告 / 状态变化。Artifacts候选、日志 / 个人ini / 包不上传。

- 英文：`feat: add healing relic and equipment bonus comparison`
- 专业中文：`feat: 新增治疗遗物并展示装备加成比较`
