# M3-06B3-03 第五件体力恢复遗物与比较

2026-11-08下发，UE 5.5.4。预计手动50–80分钟，构建等待另计。范围为五份既有Source、一处启动目录配置与一件原生Data Asset。四份候选cpp已通过编译预检，实际UHT、链接与运行按第7–10节执行。[B3-02验收报告](Reports/M3_06B3_02_Verification.md)

## 1. 目标与显示口径

凝息护符给装备聚合增加每游戏秒5点体力恢复。默认基础速率20，装备后25，移除后回20。比较第四行显示两槽装备贡献，例如0 → 5，已装备显示5 → 5。实际速率仍由恢复组件将基础速率与ASC聚合加成相加。

装卸时由原装备事务复核计时资格。体力消费后仍遵守原延迟、忙碌阻断、世界暂停和满值停表规则。

## 2. 创建原生数据资产

1. Stop PIE。在Content Browser打开 **/Game/EmberTrial/Items/Relics**。
2. 右键空白处，选择 **Miscellaneous → Data Asset → ETRelicDefinition**，命名 **DA_ET_Relic_BreathCharm**。
3. 打开资产，按下表配置并Save。

| 字段 | 值 |
| --- | --- |
| Relic Id | `breath_charm` |
| Display Name | 凝息护符 |
| Description | 每秒体力恢复增加5点。消耗后的恢复延迟照常生效。 |
| Icon | None |
| Sort Order | 50 |
| Allowed Slots | First / Slot A 与 Second / Slot B，各一项 |
| Modifiers | 1项，Kind选 **Stamina Recovery Bonus**，Additive Value填 **5.0** |

4. 选中资产，右键 **Asset Actions → Validate Assets**。确认原生Valid并重新打开核对参数。Save All后关闭UE。

## 3. 启动拥有配置

**文件** `E:/UE_Project/Demo/EmberTrial/Config/DefaultGame.ini`

查找下列完整片段并替换。

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
+StartingRelics=ETRelic:blood_pact
+StartingRelics=ETRelic:renewal_charm
```

替换为

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
+StartingRelics=ETRelic:blood_pact
+StartingRelics=ETRelic:renewal_charm
+StartingRelics=ETRelic:breath_charm
```

这五项为新会话的拥有目录。新会话的两个槽位初始为空。

## 4. Presenter与快照

### 4.1 ETRelicPresenter.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h`

**修改1　添加两槽装备恢复加成的比较字段**

查找下列完整片段并替换。

```cpp
    float AfterHealingBonus = 0.0f;
    bool operator==
```

替换为

```cpp
    float AfterHealingBonus = 0.0f;
    // 两槽装备贡献，单位为体力点 / 游戏秒。
    float BeforeStaminaRecoveryBonus = 0.0f;
    float AfterStaminaRecoveryBonus = 0.0f;
    bool operator==
```

### 4.2 ETRelicPresenter.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp`

**修改1　让恢复加成变化参与快照去重**

查找下列完整片段并替换。

```cpp
               AfterHealingBonus == Other.AfterHealingBonus &&
               bHasCommands
```

替换为

```cpp
               AfterHealingBonus == Other.AfterHealingBonus &&
               BeforeStaminaRecoveryBonus == Other.BeforeStaminaRecoveryBonus &&
               AfterStaminaRecoveryBonus == Other.AfterStaminaRecoveryBonus &&
               bHasCommands
```

**修改2　直接投影核心方案的两槽贡献**

查找下列完整片段并替换。

```cpp
        Data.AfterHealingBonus = Preview.AfterModifiers.HealingBonus;
```

替换为

```cpp
        Data.AfterHealingBonus = Preview.AfterModifiers.HealingBonus;
        Data.BeforeStaminaRecoveryBonus = Preview.BeforeModifiers.StaminaRecoveryBonus;
        Data.AfterStaminaRecoveryBonus = Preview.AfterModifiers.StaminaRecoveryBonus;
```

**修改3　已装备分支读自有句柄对应的贡献**

查找下列完整片段并替换。

```cpp
            Data.AfterHealingBonus = Applied.HealingBonus;
```

替换为

```cpp
            Data.AfterHealingBonus = Applied.HealingBonus;
            Data.BeforeStaminaRecoveryBonus = Applied.StaminaRecoveryBonus;
            Data.AfterStaminaRecoveryBonus = Applied.StaminaRecoveryBonus;
```

## 5. 比较文字

### 5.1 ETRelicPanelWidget.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp`

**修改1　在同一个PreviewText增加带单位的第四行**

查找下列完整片段并替换。

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

替换为

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

## 6. 两份既有专项的增量

### 6.1 ETRelicDefinitionsTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp`

**修改1　检查磁盘上注册的第五件实物**

查找下列完整片段并替换。

```cpp
                 HealingModifiers[0].AdditiveValue == 10.0f);
    }
    return !HasAnyErrors();
```

替换为

```cpp
                 HealingModifiers[0].AdditiveValue == 10.0f);
    }

    const UETRelicDefinition* Breath = nullptr;
    for (const UETRelicDefinition* Definition : Catalog)
    {
        if (Definition->GetRelicId() == FName(TEXT("breath_charm")))
        {
            Breath = Definition;
            break;
        }
    }
    if (!TestNotNull(TEXT("Breath Charm saved definition registered"), Breath))
    {
        return false;
    }
    TestTrue(TEXT("Breath Charm supports both slots"), Breath->SupportsSlot(EETRelicSlot::First) &&
             Breath->SupportsSlot(EETRelicSlot::Second));
    TestEqual(TEXT("Breath Charm sort order fifty"), Breath->GetSortOrder(), 50);
    const TArray<FETRelicModifier>& RecoveryModifiers = Breath->GetModifiers();
    if (TestEqual(TEXT("Breath Charm has exactly one effect"), RecoveryModifiers.Num(), 1))
    {
        TestTrue(TEXT("Breath Charm recovery bonus is plus five"),
                 RecoveryModifiers[0].Kind == EETRelicModifierKind::StaminaRecoveryBonus &&
                 RecoveryModifiers[0].AdditiveValue == 5.0f);
    }
    return !HasAnyErrors();
```

### 6.2 ETRelicPresenterTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp`

**修改1　引用实际体力属性和恢复组件**

查找下列完整片段并替换。

```cpp
#include "Attributes/ETHealthAttributeSet.h"
```

替换为

```cpp
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Resources/ETStaminaRecoveryComponent.h"
```

**修改2　NoPawn清空恢复比较字段**

查找下列完整片段并替换。

```cpp
        Presenter->GetSnapshot().AfterHealingBonus == 0.0f);
```

替换为

```cpp
        Presenter->GetSnapshot().AfterHealingBonus == 0.0f &&
        Presenter->GetSnapshot().BeforeStaminaRecoveryBonus == 0.0f &&
        Presenter->GetSnapshot().AfterStaminaRecoveryBonus == 0.0f);
```

**修改3　复用原暂停祭坛的命令桥验证真实第五件**

查找下列完整片段并替换。

```cpp
        Presenter->SetQuery({});
        Presenter->SelectId(LifeId);
        --Presenter->ObservedRevision;
```

替换为

```cpp
        Presenter->SetQuery({});
        const FName BreathId(TEXT("breath_charm"));
        UETStaminaRecoveryComponent* Recovery =
            OtherBody->FindComponentByClass<UETStaminaRecoveryComponent>();
        if (TestNotNull(TEXT("Paused altar recovery consumer"), Recovery))
        {
            const FGameplayAttribute Stamina = UETStaminaAttributeSet::GetStaminaAttribute();
            const FGameplayAttribute RecoveryBonus =
                UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute();
            OtherASC->SetNumericAttributeBase(Stamina, 50.0f);
            OtherASC->SetNumericAttributeBase(RecoveryBonus, 2.0f);
            const uint64 BeforePreview = Inventory->GetSnapshot().Revision;
            TestTrue(TEXT("Select saved recovery relic"), Presenter->SelectId(BreathId));
            const FETRelicPanelData Preview = Presenter->GetSnapshot();
            TestEqual(TEXT("Before equipment recovery zero"), Preview.BeforeStaminaRecoveryBonus, 0.0f);
            TestEqual(TEXT("Proposed equipment recovery five"), Preview.AfterStaminaRecoveryBonus, 5.0f);
            TestEqual(TEXT("Preview preserves external recovery"), OtherASC->GetNumericAttribute(RecoveryBonus), 2.0f);
            TestTrue(TEXT("Recovery preview keeps revision"), Inventory->GetSnapshot().Revision == BeforePreview);

            FETRelicPanelData Changed = Preview;
            Changed.BeforeStaminaRecoveryBonus += 1.0f;
            TestFalse(TEXT("Before recovery participates in equality"), Changed == Preview);
            Changed = Preview;
            Changed.AfterStaminaRecoveryBonus += 1.0f;
            TestFalse(TEXT("After recovery participates in equality"), Changed == Preview);

            TestTrue(TEXT("Paused altar confirms saved recovery relic"),
                     Presenter->ConfirmSelected().Result == EResult::Success);
            const FETRelicPanelData Equipped = Presenter->GetSnapshot();
            TestTrue(TEXT("Recovery already equipped state"),
                     Equipped.PreviewState == EETRelicPreviewState::AlreadyEquipped);
            TestEqual(TEXT("Equipped recovery before five"), Equipped.BeforeStaminaRecoveryBonus, 5.0f);
            TestEqual(TEXT("Equipped recovery after five"), Equipped.AfterStaminaRecoveryBonus, 5.0f);
            TestEqual(TEXT("Current recovery includes external two"), OtherASC->GetNumericAttribute(RecoveryBonus), 7.0f);
            TestEqual(TEXT("Equip does not immediately restore stamina"), OtherASC->GetNumericAttribute(Stamina), 50.0f);
            TestTrue(TEXT("Paused commit leaves timer scheduled"), Recovery->IsRecoveryScheduled());
            const double PausedTime = World->GetTimeSeconds();
            TestTrue(TEXT("Advance paused test world"), Fixture.TickTestWorld(1.0f));
            TestEqual(TEXT("Altar pause freezes world time"), World->GetTimeSeconds(), PausedTime);
            TestEqual(TEXT("Altar pause freezes stamina"), OtherASC->GetNumericAttribute(Stamina), 50.0f);
            TestTrue(TEXT("Paused altar unequips recovery"),
                     Presenter->UnequipPreviewSlot().Result == EResult::Success);
            TestEqual(TEXT("Unequip preserves external recovery"), OtherASC->GetNumericAttribute(RecoveryBonus), 2.0f);
            TestEqual(TEXT("Unequip resets equipped recovery"), Presenter->GetSnapshot().BeforeStaminaRecoveryBonus, 0.0f);
            TestTrue(TEXT("Paused unequip leaves timer scheduled"), Recovery->IsRecoveryScheduled());
            OtherASC->SetNumericAttributeBase(RecoveryBonus, 0.0f);
            OtherASC->SetNumericAttributeBase(Stamina, 100.0f);
        }
        Presenter->SelectId(LifeId);
        --Presenter->ObservedRevision;
```

## 7. 正常构建

保存五份源码和Config。在UE关闭状态下，用PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本执行Editor Build和三模块索引。CLion出现提示时Reload CMake。然后执行Game Development Build。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 8. 核对现有布局

打开`/Game/EmberTrial/ui/Inventory/WBP_ETRelicPanel`的Designer。根`PanelSize`以当前720×620作为检查基线。`PreviewText`继续承担四行比较及已装备提示，核对Auto Wrap Text、Font 16和Vertical Box Slot的Fill 1。实机确认文字、第五个列表项与确认按钮完整。若裁切，请记录实际窗口尺寸和被裁切区域。

## 9. 四个专项各一次

在非Play状态下，逐条粘贴到Output Log底部命令行，每项完成后再执行下一项。

```text
Automation RunTests EmberTrial.M3.Inventory.Definitions
Automation RunTests EmberTrial.M3.Inventory.Session
Automation RunTests EmberTrial.UI.Relics.ViewModels
Automation RunTests EmberTrial.UI.Relics.Presenter
```

Definitions检查第五件实物。Session与ViewModels验证配置驱动的拥有目录和列表。Presenter复用原暂停祭坛命令桥，检查0→5、5→5、外部2点独立、装卸后计时资格和暂停冻结。最后恢复夹具状态，继续原过期命令与关闭检查。

## 10. 实机短查

1. 新Play，E打开祭坛，确认五件、双空槽。选凝息护符，比较槽A，第四行应为0→5。确认后变为5→5。关闭祭坛后按P打开普通暂停页，核对该页只读。
2. 恢复游戏，安全位置按2一次，体力75按原延迟恢复满值。装备事务日志应记录有效速率25。可以等体力回满再回祭坛。
3. 回祭坛卸下凝息。再按2一次，确认恢复正常。卸装日志应记录有效速率20。速率和计时点由日志与专项核对，实机观察体力条即可。
4. Stop再新Play，体力100、加成0、拥有五件且槽位为空。短查移动与P，随后Stop。Save All。

反馈时记录双Build／三索引、五件资产配置与Valid、四专项、第四行与布局、25／20恢复、Stop新100及实际手动工时与构建等待。来源清单在验收新资产后登记。

## 11. Commit / Push

B3-02的19份实现已在本地提交`eb9c1ad`。本次收尾提交以下11份文档。

```text
Docs/Reports/M3_06B3_02_Verification.md
Docs/M3_06B3_02_StaminaRecoveryEquipmentPipeline.md
Docs/M3_06B3_03_StaminaRecoveryRelicAndComparison.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文 `docs: verify recovery equipment and plan fifth relic`
- 中文 `docs: 完成体力恢复装备验收并规划第五件遗物`

在CLion Review后由你Stage、Commit和Push。预检候选与原始日志存放在本地Artifacts。

完成B3-03操作并验收后，功能提交的文件清单如下。

```text
EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h
EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicDefinitionsTests.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp
EmberTrial/Config/DefaultGame.ini
EmberTrial/Content/EmberTrial/Items/Relics/DA_ET_Relic_BreathCharm.uasset
```

若实机布局确实需要修改原Panel，把对应资产加入功能提交。验收后再Review当时的来源清单与报告。

- 英文 `feat: add stamina recovery relic and comparison`
- 中文 `feat: 新增体力恢复遗物与装备比较`
