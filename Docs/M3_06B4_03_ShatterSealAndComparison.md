# M3-06B4-03 第六件裂势印与破势比较

2026-11-08下发，UE 5.5.4。预计手动45–70分钟，构建等待另计。本节涉及五份既有源码、一处启动目录配置和一件原生Data Asset。四份候选cpp通过编译预检，正式UHT、链接与运行按第7–10节执行。[B4-02验收报告](Reports/M3_06B4_02_Verification.md)

## 1. 本节效果

裂势印给F提供固定20点架势伤害加成。基础配置60，单独装备后为80，卸下回60。命中满架势训练目标时，装备后首发从100扣到20。实际伤害仍按施放快照结算。

遗物比较第五行显示两槽装备贡献。空槽预览为0 → 20，已装备时为20 → 20。F最终架势伤害由核心能力结合基础值与ASC聚合值计算。

## 2. 创建裂势印数据资产

1. 在含B4-02实现的UE编辑器中Stop PIE。Content Browser打开 **/Game/EmberTrial/Items/Relics**。
2. 在空白处右键，选择 **Miscellaneous → Data Asset → ETRelicDefinition**，命名 **DA_ET_Relic_ShatterSeal**。
3. 打开新资产，按下表填写。

| 字段 | 值 |
| --- | --- |
| Relic Id | `shatter_seal` |
| Display Name | 裂势印 |
| Description | 破势技（F）造成的架势伤害增加20点。 |
| Icon | None |
| Sort Order | 60 |
| Allowed Slots | 两项，First / Slot A与Second / Slot B |
| Modifiers | 一项，Kind为 **Break Pulse Poise Bonus**，Additive Value为 **20.0** |

4. Save后在Content Browser选中新资产，右键 **Asset Actions → Validate Assets**。确认Valid，重开资产核对ID、两个槽、唯一Modifier及20.0。Save All并关闭UE。

## 3. 启动拥有配置

**文件** `E:/UE_Project/Demo/EmberTrial/Config/DefaultGame.ini`

找到以下片段。

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
+StartingRelics=ETRelic:blood_pact
+StartingRelics=ETRelic:renewal_charm
+StartingRelics=ETRelic:breath_charm
```

替换为

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
+StartingRelics=ETRelic:blood_pact
+StartingRelics=ETRelic:renewal_charm
+StartingRelics=ETRelic:breath_charm
+StartingRelics=ETRelic:shatter_seal
```

每次新会话拥有上述六件，双槽从空开始。

## 4. Presenter与快照

### 4.1 ETRelicPresenter.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h`

**修改1　增加第五行的两个快照字段**

找到以下片段。

```cpp
    float AfterStaminaRecoveryBonus = 0.0f;
    bool operator==
```

替换为

```cpp
    float AfterStaminaRecoveryBonus = 0.0f;
    // 两槽装备对F的固定架势伤害贡献，单位为架势点。
    float BeforeBreakPulsePoiseBonus = 0.0f;
    float AfterBreakPulsePoiseBonus = 0.0f;
    bool operator==
```

### 4.2 ETRelicPresenter.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp`

**修改1　将破势字段纳入快照去重**

找到以下片段。

```cpp
               AfterStaminaRecoveryBonus == Other.AfterStaminaRecoveryBonus &&
               bHasCommands
```

替换为

```cpp
               AfterStaminaRecoveryBonus == Other.AfterStaminaRecoveryBonus &&
               BeforeBreakPulsePoiseBonus == Other.BeforeBreakPulsePoiseBonus &&
               AfterBreakPulsePoiseBonus == Other.AfterBreakPulsePoiseBonus &&
               bHasCommands
```

**修改2　可用方案投影核心装备贡献**

找到以下片段。

```cpp
        Data.AfterStaminaRecoveryBonus = Preview.AfterModifiers.StaminaRecoveryBonus;
```

替换为

```cpp
        Data.AfterStaminaRecoveryBonus = Preview.AfterModifiers.StaminaRecoveryBonus;
        Data.BeforeBreakPulsePoiseBonus = Preview.BeforeModifiers.BreakPulsePoiseBonus;
        Data.AfterBreakPulsePoiseBonus = Preview.AfterModifiers.BreakPulsePoiseBonus;
```

**修改3　已装备状态读取同一份自有贡献**

找到以下片段。

```cpp
            Data.AfterStaminaRecoveryBonus = Applied.StaminaRecoveryBonus;
```

替换为

```cpp
            Data.AfterStaminaRecoveryBonus = Applied.StaminaRecoveryBonus;
            Data.BeforeBreakPulsePoiseBonus = Applied.BreakPulsePoiseBonus;
            Data.AfterBreakPulsePoiseBonus = Applied.BreakPulsePoiseBonus;
```

## 5. 原PreviewText的第五行

### 5.1 ETRelicPanelWidget.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp`

**修改1　用原PreviewText渲染第五行**

找到以下片段。

```cpp
        PreviewText->SetText(FText::Format(
            NSLOCTEXT("ETRelics", "EquipmentStatsWithRecovery",
                      "最大生命 {0} → {1}\n首段轻击 {2} → {3}\n装备治疗加成 {4} → {5}\n"
                      "装备体力恢复加成（点/秒） {6} → {7}{8}"),
            FText::AsNumber(Data.BeforeMax, &NumberFormat),
            FText::AsNumber(Data.AfterMax, &NumberFormat),
            FText::AsNumber(Data.BeforeAttack, &NumberFormat),
            FText::AsNumber(Data.AfterAttack, &NumberFormat),
            FText::AsNumber(Data.BeforeHealingBonus, &NumberFormat),
            FText::AsNumber(Data.AfterHealingBonus, &NumberFormat),
            FText::AsNumber(Data.BeforeStaminaRecoveryBonus, &NumberFormat),
            FText::AsNumber(Data.AfterStaminaRecoveryBonus, &NumberFormat),
            Data.PreviewState == EETRelicPreviewState::AlreadyEquipped
                ? FText::FromString(TEXT("\n该槽已装备此遗物")) : FText::GetEmpty()));
```

替换为

```cpp
        PreviewText->SetText(FText::Format(
            NSLOCTEXT("ETRelics", "EquipmentStatsWithBreakPulse",
                      "最大生命 {0} → {1}\n首段轻击 {2} → {3}\n装备治疗加成 {4} → {5}\n"
                      "装备体力恢复加成（点/秒） {6} → {7}\n装备破势加成（F） {8} → {9}{10}"),
            FText::AsNumber(Data.BeforeMax, &NumberFormat),
            FText::AsNumber(Data.AfterMax, &NumberFormat),
            FText::AsNumber(Data.BeforeAttack, &NumberFormat),
            FText::AsNumber(Data.AfterAttack, &NumberFormat),
            FText::AsNumber(Data.BeforeHealingBonus, &NumberFormat),
            FText::AsNumber(Data.AfterHealingBonus, &NumberFormat),
            FText::AsNumber(Data.BeforeStaminaRecoveryBonus, &NumberFormat),
            FText::AsNumber(Data.AfterStaminaRecoveryBonus, &NumberFormat),
            FText::AsNumber(Data.BeforeBreakPulsePoiseBonus, &NumberFormat),
            FText::AsNumber(Data.AfterBreakPulsePoiseBonus, &NumberFormat),
            Data.PreviewState == EETRelicPreviewState::AlreadyEquipped
                ? FText::FromString(TEXT("\n该槽已装备此遗物")) : FText::GetEmpty()));
```

## 6. 两份既有专项的增量

### 6.1 ETRelicDefinitionsTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp`

**修改1　校验磁盘上的裂势印**

找到以下片段。

```cpp
                 RecoveryModifiers[0].AdditiveValue == 5.0f);
    }
    return !HasAnyErrors();
```

替换为

```cpp
                 RecoveryModifiers[0].AdditiveValue == 5.0f);
    }

    const FName ShatterId(TEXT("shatter_seal"));
    const UETRelicDefinition* const* Shatter = Catalog.FindByPredicate(
        [ShatterId](const UETRelicDefinition* Definition)
        { return Definition->GetRelicId() == ShatterId; });
    if (!TestNotNull(TEXT("Shatter Seal saved definition registered"), Shatter))
    {
        return false;
    }
    const UETRelicDefinition* Definition = *Shatter;
    TestTrue(TEXT("Shatter Seal supports both slots"), Definition->SupportsSlot(EETRelicSlot::First) &&
             Definition->SupportsSlot(EETRelicSlot::Second));
    TestEqual(TEXT("Shatter Seal sort order sixty"), Definition->GetSortOrder(), 60);
    const TArray<FETRelicModifier>& PulseModifiers = Definition->GetModifiers();
    if (TestEqual(TEXT("Shatter Seal has exactly one effect"), PulseModifiers.Num(), 1))
    {
        TestTrue(TEXT("Shatter Seal Break Pulse bonus is plus twenty"),
                 PulseModifiers[0].Kind == EETRelicModifierKind::BreakPulsePoiseBonus &&
                 PulseModifiers[0].AdditiveValue == 20.0f);
    }
    return !HasAnyErrors();
```

### 6.2 ETRelicPresenterTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp`

**修改1　测试身体沿用正式F的原生数值入口**

找到以下片段。

```cpp
#include "Abilities/ETGA_Heal.h"
```

替换为

```cpp
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_BreakPulse.h"
```

**修改2　为现有夹具配置F类**

找到以下片段。

```cpp
            Body->HealAbilityClass = UETGA_Heal::StaticClass();
```

替换为

```cpp
            Body->HealAbilityClass = UETGA_Heal::StaticClass();
            Body->BreakPulseAbilityClass = UETGA_BreakPulse::StaticClass();
```

**修改3　无Pawn时清空第五行**

找到以下片段。

```cpp
        Presenter->GetSnapshot().AfterStaminaRecoveryBonus == 0.0f);
```

替换为

```cpp
        Presenter->GetSnapshot().AfterStaminaRecoveryBonus == 0.0f &&
        Presenter->GetSnapshot().BeforeBreakPulsePoiseBonus == 0.0f &&
        Presenter->GetSnapshot().AfterBreakPulsePoiseBonus == 0.0f);
```

**修改4　复用暂停祭坛入口验证比较、真实装卸与外部贡献隔离**

找到以下片段。

```cpp
        Presenter->SelectId(LifeId);
        --Presenter->ObservedRevision;
```

替换为

```cpp
        const FName ShatterId(TEXT("shatter_seal"));
        const FGameplayAttribute BreakPulseBonus =
            UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute();
        OtherASC->SetNumericAttributeBase(BreakPulseBonus, 2.0f);
        const uint64 BeforeShatterPreview = Inventory->GetSnapshot().Revision;
        if (!TestTrue(TEXT("Select saved Break Pulse relic"), Presenter->SelectId(ShatterId)))
        {
            return false;
        }
        const FETRelicPanelData ShatterPreview = Presenter->GetSnapshot();
        TestTrue(TEXT("Break Pulse comparison available"),
                 ShatterPreview.PreviewState == EETRelicPreviewState::Available);
        TestEqual(TEXT("Before equipment Break Pulse zero"), ShatterPreview.BeforeBreakPulsePoiseBonus, 0.0f);
        TestEqual(TEXT("Proposed equipment Break Pulse twenty"), ShatterPreview.AfterBreakPulsePoiseBonus, 20.0f);
        TestEqual(TEXT("Preview preserves independent Break Pulse contribution"),
                  OtherASC->GetNumericAttribute(BreakPulseBonus), 2.0f);
        TestTrue(TEXT("Break Pulse preview keeps revision"),
                 Inventory->GetSnapshot().Revision == BeforeShatterPreview);

        FETRelicPanelData ChangedPulse = ShatterPreview;
        ChangedPulse.BeforeBreakPulsePoiseBonus += 1.0f;
        TestFalse(TEXT("Before Break Pulse participates in equality"), ChangedPulse == ShatterPreview);
        ChangedPulse = ShatterPreview;
        ChangedPulse.AfterBreakPulsePoiseBonus += 1.0f;
        TestFalse(TEXT("After Break Pulse participates in equality"), ChangedPulse == ShatterPreview);

        TestTrue(TEXT("Paused altar confirms saved Break Pulse relic"),
                 Presenter->ConfirmSelected().Result == EResult::Success);
        const FETRelicPanelData EquippedPulse = Presenter->GetSnapshot();
        TestTrue(TEXT("Break Pulse already equipped state"),
                 EquippedPulse.PreviewState == EETRelicPreviewState::AlreadyEquipped);
        TestEqual(TEXT("Equipped Break Pulse before twenty"), EquippedPulse.BeforeBreakPulsePoiseBonus, 20.0f);
        TestEqual(TEXT("Equipped Break Pulse after twenty"), EquippedPulse.AfterBreakPulsePoiseBonus, 20.0f);
        TestEqual(TEXT("Current Break Pulse includes independent two"),
                  OtherASC->GetNumericAttribute(BreakPulseBonus), 22.0f);
        TestTrue(TEXT("Paused altar unequips Break Pulse"),
                 Presenter->UnequipPreviewSlot().Result == EResult::Success);
        TestEqual(TEXT("Unequip preserves independent Break Pulse contribution"),
                  OtherASC->GetNumericAttribute(BreakPulseBonus), 2.0f);
        TestEqual(TEXT("Unequip resets equipped Break Pulse"),
                  Presenter->GetSnapshot().BeforeBreakPulsePoiseBonus, 0.0f);
        OtherASC->SetNumericAttributeBase(BreakPulseBonus, 0.0f);

        Presenter->SelectId(LifeId);
        --Presenter->ObservedRevision;
```

## 7. 正常构建

保存五份源码和Config。关闭UE后，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本完成Editor Build与三模块索引。CLion有提示时Reload CMake，再执行Game Development Build。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 8. 祭坛详情布局修复

实机选中凝息护符、逆焰刻印或已装备的裂势印时，说明换行和额外提示会增加右栏所需高度。原先只有`PreviewText`使用 Fill 1，剩余高度缩小时，文字可以绘制到确认和卸下按钮上。

已在`/Game/EmberTrial/ui/Inventory/WBP_ETRelicPanel`编译并保存以下层级。根`PanelSize`沿用720×620，15个原生绑定沿用原名称和类型。

```text
DetailsColumn
├─ DetailsScroll（Scroll Box，Vertical Box Slot = Fill 1）
│  ├─ DetailNameText
│  ├─ DetailDescriptionText
│  ├─ SlotText
│  ├─ SlotButtons
│  └─ PreviewText
└─ CommandContent（Vertical Box Slot = Auto，顶部间距12）
   ├─ CommandButtons
   └─ CommandResultText
```

`DetailsScroll`启用 Clip to Bounds。说明与比较按内容高度排列，超过可视范围时在右侧详情区滚动。键盘焦点移到滚动区内的槽位按钮时会带动滚动。确认装备、卸下该槽和结果提示留在底部。

2026-11-08复验通过UE资产校验，0个错误、0个警告。无渲染PIE经真实祭坛入口依次选择凝息护符、逆焰刻印并装备裂势印，验证比较文字、已装备提示以及底部命令区的层级。画面与滚动观感按第10节检查。

第10节检查时依次选凝息护符和逆焰刻印，再装备裂势印。将鼠标放在详情区向下滚动，确认第五项与已装备提示可完整读到，底部按钮位置保持稳定。普通P页共用详情布局，命令区仍按只读规则收起。

## 9. 四项专项各一次

打开UE，在非Play状态下于Output Log底部逐条执行。每条等到`Result={Success}`再继续。

```text
Automation RunTests EmberTrial.M3.Inventory.Definitions
Automation RunTests EmberTrial.M3.Inventory.Session
Automation RunTests EmberTrial.UI.Relics.ViewModels
Automation RunTests EmberTrial.UI.Relics.Presenter
```

Definitions验证新实物及数值。Session与ViewModels核对配置驱动的目录与列表。Presenter通过现有暂停祭坛桥验证0→20、20→20、比较无副作用、实际装卸与独立加成保留。

## 10. 实机短查

1. Play正式地图`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。去祭坛按E，确认六件。若六件超出列表视口，向下滚动选最后一件裂势印。空槽A应显示`装备破势加成（F） 0 → 20`。确认装备后应显示20 → 20。检查第8节的文本和按钮。
2. 点继续，接近满架势训练目标到约300 cm内，松开移动键并站定，按F一次。Output Log筛`LogETBreakPulse`。同一次`Serial`的`Pulse`应有`Damage=80.0`，对应训练目标的`Target`应有`Applied=1 Poise=100.0->20.0`。
3. 返回祭坛，卸下槽A。继续后等F冷却就绪，在有效范围再按一次F，核对`Damage=60.0`与真实训练目标命中。目标剩余架势取决于刚才的命中，按日志实记。
4. Stop再Play，核对六件、空双槽、玩家`BreakPulsePoiseBonus=0.0`与F就绪，短查P和移动。Stop并Save All。

反馈时记录双Build／三索引、裂势印实际资产路径与Valid、四项专项、六件和第五行布局、F 80／60、Stop后新身体0与空槽、实际手动工时与构建等待。

## B4-03验收结果

2026-11-08通过。双Build、三索引、裂势印Valid、四项专项、六件与第五行布局均通过。日志确认F 80／60，Stop后六件、空双槽与新身体加成0。手动60分钟，构建等待5分钟。实现提交为`bce7dac`。[验收报告](Reports/M3_06B4_03_Verification.md) · [C1操作指南](M3_06C1_RelicProjectionStress.md)

## 11. Commit / Push

B4-02实现已在本地提交`06318c7`。在CLion Review并Stage本轮11份文档，随后Commit和Push。

```text
Docs/Reports/M3_06B4_02_Verification.md
Docs/M3_06B4_02_BreakPulseRelicEquipmentPipeline.md
Docs/M3_06B4_03_ShatterSealAndComparison.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文 `docs: verify break pulse equipment and plan sixth relic`
- 中文 `docs: 完成破势装备验收并规划第六件遗物`

B4-03操作通过后，功能提交包含以下源码、Config、资产与来源清单。

```text
EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h
EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp
EmberTrial/Config/DefaultGame.ini
EmberTrial/Content/EmberTrial/Items/Relics/DA_ET_Relic_ShatterSeal.uasset
EmberTrial/Content/EmberTrial/ui/Inventory/WBP_ETRelicPanel.uasset
Docs/AssetManifest.csv
```

第8节的面板修复随功能提交。裂势印在整节验收后登记资产来源。

- 英文 `feat: add Shatter Seal relic and Break Pulse comparison`
- 中文 `feat: 新增裂势印遗物与破势加成比较`
