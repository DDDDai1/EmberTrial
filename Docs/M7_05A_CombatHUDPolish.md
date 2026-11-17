# M7-05A 战斗 HUD 正式布局

把生命、体力、治疗次数和破势状态收拢到左下角。统一边距、色彩和字号，保留生命与体力的数值、Q 的次数、F 的就绪和冷却。默认不创建右上角的 Pawn 生命周期面板，开发时可从控制器蓝图打开。预计手动 **35–50 分钟**，另计构建等待。

这批沿用四个原生 Widget、Presenter、HUD 装配和各自的状态更新。主菜单、暂停、祭坛页和胜败界面归到后续的界面打磨。战斗中间仍留给锁定标记和 E 交互提示。

## 1. 让生命周期面板按需显示

1. 打开 `EmberTrial/Source/EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h`。在 `private` 区域、`StatusWidgetClass` 的 `UPROPERTY` **之前**插入下方属性。

   ```cpp
   // 开发诊断按需开启，默认不创建视图。
   UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI|Diagnostics")
   bool bShowLifecycleStatus = false;
   ```

2. 打开 `EmberTrial/Source/EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp`。在 `RebuildStatusWidget()` 内，保留函数开头的 `HasBegunPlay()` 和 `bUIShuttingDown` 检查。在 `RemoveStatusWidget();` 后、`ResolveLocalUIController()` 前插入下方代码。

   ```cpp
   if (!bShowLifecycleStatus)
   {
       return;
   }
   ```

   把检查放在这里，BeginPlay 和蓝图重建入口都走同一条件。启用时仍走现有的本地控制器、创建和解绑路径。
3. 保存源码，关闭 UE，在 PowerShell 执行下方双目标构建。等 Editor Build、CLion 刷新索引和 Game Development Build 都成功后再开 UE。

   ```powershell
   Set-Location 'E:/UE_Project/Demo'
   & './Tools/Refresh-CLion.ps1'
   & 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat' EmberTrial Win64 Development '-Project=E:/UE_Project/Demo/EmberTrial/EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
   ```

4. 打开 `/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`，选择继承的 **UIBootstrap**。搜索 **Show Lifecycle Status**，确认未勾选。原 Status Widget Class 保留，供临时诊断启用。四个资源 Widget Class 保留之前的引用。Compile → Save。

## 2. 布置四张 HUD 卡

### 2.1 先改位置，再改层级内样式

在 Content Browser 打开 `/Game/EmberTrial/ui/HUD`。四个 WBP 都已经有 Canvas 根节点和卡片 Border。打开 **Designer**，从左侧 **Hierarchy** 点卡片的 Border，在 **Details → Slot (Canvas Panel Slot)** 设置。选 **左下单点** Anchor 后再输坐标，Anchor Min 与 Max 都应为 **(0,1)**。

四张卡共用 **Alignment (0,1)**、**Position X 24**、**Size X 304**、**Auto Size 关闭**。负的 Y 是卡片底边到视口底边的偏移。按表从底部往上排，卡间距为 8 个 UMG 布局单位。

| WBP | Border | Position Y | Size Y |
| --- | --- | ---: | ---: |
| `WBP_ETBreakPulseHUD` | `BreakPulseBorder` | -24 | 80 |
| `WBP_ETHealingHUD` | `HealingBorder` | -112 | 60 |
| `WBP_ETStaminaHUD` | `StaminaBorder` | -180 | 80 |
| `WBP_ETHealthHUD` | `HealthBorder` | -268 | 80 |

选 Border，设 **Padding** 左右 **12**、上下 **6**，Horizontal / Vertical Alignment 都用 **Fill**。Brush Color 用 **Hex sRGB `171B22E0`**（灰黑、约 88% 不透明）。Render Transform 保持原值。四个 WBP 分开预览时只显示自己的卡片，四张卡间距用 PIE 核对。

### 2.2 缩短进度条并区分视觉优先级

在 Hierarchy 展开 Border。标题与内容容器是同级，标题应留在内容容器外。标题 Text Block 将 **Font Size** 改为 **15**、颜色 **`DCE0E6FF`**，在 Vertical Box Slot 设 **Size Auto、Horizontal Fill、Bottom Padding 2**。数值 Text Block 用颜色 **`F5F1E8FF`**，生命字号 **22**，体力和次数 **18**，破势的提示与秒数 **16**。字号在 **Appearance → Font** 下设置，沿用能显示中文的现有字体。

生命和体力的条继续使用现有 Size Box。选外层 Size Box，把 **Height Override** 设为 **8**，Vertical Box Slot 的 **Top Padding** 改为 **3**。破势的条若直接位于 `BreakPulseContent` 下，在 **Hierarchy** 右键进度条 → **Wrap With → Size Box**，把 Size Box 的 **Height Override** 设为 **8**，Slot 用 **Auto / Fill / Top Padding 3**。其余边距使用 0。确认 Bar 的 **Bar Fill Type = Left to Right**，保留原 Percent 更新，关闭 Is Marquee。

| 进度条 | Fill Color and Opacity（Hex sRGB） |
| --- | --- |
| `HealthBar` | `D57460FF`（赤陶色） |
| `StaminaBar` | `A6BE7EFF`（苔绿色） |
| `BreakPulseBar` | `D9AA63FF`（琥珀色） |

### 2.3 按原绑定编译四个 WBP

依次确认下表列出的对象名、类型与 **Is Variable**。标题、数值为 Text Block，条为 Progress Bar，内容容器为 Vertical Box。Designer 的文字和 Percent 只是预览值，运行时数据由原生 Widget 写入。保留现有 **无 Binding、Never Tick、不可聚焦、Self & All Children 不参与点击检测**。四个资产分别 Compile → Save，若 `Validate Assets` 报缺绑定，从报错中的控件名回查 Hierarchy。

| WBP | 已有 BindWidget 对象 |
| --- | --- |
| `WBP_ETHealthHUD` | `HealthStatusText`、`HealthValuesText`、`HealthBar`、`HealthContent` |
| `WBP_ETStaminaHUD` | `StaminaStatusText`、`StaminaValuesText`、`StaminaBar`、`StaminaContent` |
| `WBP_ETHealingHUD` | `HealingStatusText`、`HealingValuesText`、`HealingContent` |
| `WBP_ETBreakPulseHUD` | `BreakPulseStatusText`、`BreakPulseValuesText`、`BreakPulseBar`、`BreakPulseContent` |

## 3. 分组验收

| 组别 | 操作与通过条件 |
| --- | --- |
| 布局与小窗 | 在 `L_ET_TrialRoute` 正式图进 PIE，先用常用窗口，再用约 **1280 × 720** 的 New Editor Window。四卡各一份、左下对齐、中文和条无裁剪，右上角无诊断面板。接近祭坛时 E 提示可读，锁定敌人时 Tab 标记清楚。在同一常用分辨率选一张截图保存到 `Artifacts/M7_05A`。 |
| 动态状态 | 在第一场让敌人命中一次，看生命数值和长度变短。按 2 观察体力消费、恢复，脱离后按 Q 观察数值与次数，站定按 F 观察冷却数值和进度。P 暂停后恢复，确认控件没有抢焦点，并保留原本的冻结与恢复。 |
| 诊断开关与重开 | Stop。在 Controller 的 UIBootstrap 临时勾选 Show Lifecycle Status，Compile，开始一次 PIE，确认右上诊断面板出现且左下四卡正常。再次 Stop，恢复关、Compile → Save。再 Play 短查右上角恢复空白，Stop。 |
| 专项自动化 | Stop 后在 Output Log 依次执行两项指令，均应为 `Success`。省去其他阶段的临时验收蓝图。 |

```text
Automation RunTests EmberTrial.UI.ResourceHUD.Formatting
Automation RunTests EmberTrial.UI.BreakPulse.Lifecycle
```

```text
M7-05A 完成
两旧源码 / Editor 与 Game Build / 三模块索引：
四个 HUD WBP / 四组绑定 / Controller 默认关闭 / Compile Save：
常用窗口与小窗实际尺寸 / 一张截图路径 / 四卡有无遮盖：
生命 / 2体力 / Q次数 / F冷却与P恢复：
E提示 / Tab标记 / 临时诊断可打开、已恢复关闭与Stop再开：
ResourceHUD.Formatting / BreakPulse.Lifecycle：
最明显的布局或失败情况（无填无）：
完成时分 / 手动分钟 / 构建等待分钟：
```

## 4. Commit / Push

文档批次暂存本指南、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。若和 M7-04 验收文档一起提交，按两个文档清单合并暂存文件。

- 英文 `docs(ui): plan and verify the compact combat HUD layout`
- 中文 `docs(ui): 记录紧凑战斗 HUD 的布局与验收步骤`

实现并通过实机验收后，暂存 `ETUIBootstrapComponent.h/.cpp`、第 2 节的四个 HUD WBP 和实际改动的 `BP_ETPlayerController.uasset`。来源清单随资产实物哈希更新。截图和日志保留在本地。

- 英文 `feat(ui): polish combat resource HUD and gate diagnostics`
- 中文 `feat(ui): 打磨战斗资源 HUD 并按需显示诊断面板`
