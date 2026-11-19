# M7-05A 战斗 HUD 正式布局

把生命、体力、治疗次数和破势状态收拢到左下角。统一边距、色彩和字号，保留生命与体力的数值、Q 的次数、F 的就绪和冷却。默认不创建右上角的 Pawn 生命周期面板，开发时可从控制器蓝图打开。预计手动 **35–50 分钟**，另计构建等待。

2026-11-17 已验收，用户手动 **30 分钟**、构建等待 **5 分钟**。动态资源、P 恢复、诊断开关与两项专项通过。[验收报告](Reports/M7_05A_Verification.md)

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

**2026-11-17 核对通过。** 已读取四份存盘蓝图的 Designer 与编译后 WidgetTree。左下单点锚、对齐、上表坐标和尺寸、关闭 Auto Size、内边距、两轴 Fill、背景色及默认 Render Transform 均符合设置，未发现需修正项。调整前的读数保存在本地 `Artifacts/M7_05A/HUDContentsBefore.json`。

### 2.2 缩短进度条并区分视觉优先级

在 Hierarchy 展开 Border。标题与内容容器是同级，标题应留在内容容器外。标题 Text Block 将 **Font Size** 改为 **15**、颜色 **`DCE0E6FF`**，在 Vertical Box Slot 设 **Size Auto、Horizontal Fill、Bottom Padding 2**。数值 Text Block 用颜色 **`F5F1E8FF`**，生命字号 **22**，体力和次数 **18**，破势的提示与秒数 **16**。数值插槽边距设为 **0**，包括破势原有的底部 8 单位留白。字号在 **Appearance → Font** 下设置，沿用能显示中文的现有字体。

生命和体力的条继续使用现有 Size Box。选外层 Size Box，把 **Height Override** 设为 **8**，Vertical Box Slot 的 **Top Padding** 改为 **3**。破势的条若直接位于 `BreakPulseContent` 下，在 **Hierarchy** 右键进度条 → **Wrap With → Size Box**，把 Size Box 的 **Height Override** 设为 **8**，Slot 用 **Auto / Fill / Top Padding 3**。其余边距使用 0。确认 Bar 的 **Bar Fill Type = Left to Right**，保留原 Percent 更新，关闭 Is Marquee。

| 进度条 | Fill Color and Opacity（Hex sRGB） |
| --- | --- |
| `HealthBar` | `D57460FF`（赤陶色） |
| `StaminaBar` | `A6BE7EFF`（苔绿色） |
| `BreakPulseBar` | `D9AA63FF`（琥珀色） |

**2026-11-17 已配置并复验。** 生命复用 `BarSize`，体力复用 `StaminaBarSize`。破势新增 `BreakPulseBarSize` 包裹原 `BreakPulseBar`，它仍在 `BreakPulseContent` 内。三条均为高 **8**、顶边距 **3**、从左向右填充、Marquee 关闭。四份蓝图已编译保存，新 UE 进程重开后通过四份资产校验（**4 Valid、0 Invalid、0 Warnings**）。编译前后证据在本地 `Artifacts/M7_05A`，包括 `ApplyCombatHUDStyle.log` 和 `HUDReopened.json`。

在正式图的内嵌 PIE 中，四张卡各显示一次，左下对齐，中文与三条进度条均未裁剪。右上角没有诊断面板。截图是 **1280 × 720 的编辑器窗口**，保存在本地 `Artifacts/M7_05A/CombatHUD_PIE.png`。独立小窗、动态资源变化和诊断开关仍按第 3 节验收。

#### 2.2.1 修复旧编辑器会话内的 HUD 重叠

2026-11-17 用户反馈大视口中生命条压到体力标题、体力条压到治疗标题。截图仍为原红、绿、蓝进度条，原条高与旧字号叠在第 2.1 节缩小后的卡片上。核对时间线发现，用户编辑器于 **23:03:50** 启动，四份 HUD 于 **23:13–23:14** 加载并保存第 2.1 节，助手的第 2.2 节样式于 **23:23:15** 从另一个 UE 进程写入磁盘。发生重叠的两次 PIE 在 **23:27:01** 和 **23:27:36**，仍属于之前的编辑器会话。四份磁盘资产的 SHA-256 与新版样式的存盘及复验记录相符。

处理时先保存当前工作，退出持有旧 Widget 模板的编辑器，再从 `EmberTrial.uproject` 重新打开工程。开始下一次 Play 前，打开任意一份 HUD 核对标题字号 **15**，生命进度条 Size Box 高 **8**，确认已读取第 2.2 节。若仍显示旧字号，先停止测试并保留日志，检查启动的工程路径。

此前助手的截图来自独立新会话的小型内嵌视口，遗漏了用户已打开编辑器的内存状态。本地证据为 `Artifacts/M7_05A/StaleSession_2327.log` 与 `ReportedOverlap_2569x1389.png`。后续通过后台 UE 进程改资产时，交接步骤需包含用户会话的重载或重启确认，再开始 PIE。

**现场复验通过。** 用户于 **23:35** 保存并关闭旧会话，重新打开的 UE 5.5 会话于 **23:38:35** 建立。助手直接在该会话打开正式图，23:40:02 开始 PIE，核对普通内嵌视口和 **F11 沉浸大视口**。四卡均有间距，标题与数值可读，赤陶色、苔绿色、琥珀色细进度条均位于卡片内部，重叠消失。日志确认四个 HUD 各创建一次，摘录在 `Artifacts/M7_05A/UserSessionRecheck.log`。现场大视口图由 computer-use 返回，未另存独立截图文件。复验后已停止 PIE。其余战斗动态与独立小窗仍按第 3 节进行。

### 2.3 按原绑定编译四个 WBP

依次确认下表列出的对象名、类型与 **Is Variable**。标题、数值为 Text Block，条为 Progress Bar，内容容器为 Vertical Box。Designer 的文字和 Percent 只是预览值，运行时数据由原生 Widget 写入。保留现有 **无 Binding、Never Tick、不可聚焦、Self & All Children 不参与点击检测**。四个资产分别 Compile → Save，若 `Validate Assets` 报缺绑定，从报错中的控件名回查 Hierarchy。

| WBP | 已有 BindWidget 对象 |
| --- | --- |
| `WBP_ETHealthHUD` | `HealthStatusText`、`HealthValuesText`、`HealthBar`、`HealthContent` |
| `WBP_ETStaminaHUD` | `StaminaStatusText`、`StaminaValuesText`、`StaminaBar`、`StaminaContent` |
| `WBP_ETHealingHUD` | `HealingStatusText`、`HealingValuesText`、`HealingContent` |
| `WBP_ETBreakPulseHUD` | `BreakPulseStatusText`、`BreakPulseValuesText`、`BreakPulseBar`、`BreakPulseContent` |

## 3. 分组验收

本轮已通过，按用户最终反馈与日志记录结果。[实际证据及窗口尺寸记录边界](Reports/M7_05A_Verification.md)

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

源码、四份 HUD 和 Controller 已随 `2c55ea0` 提交。本轮补交[验收报告与来源清单](Reports/M7_05A_Verification.md#本次文档-commit--push)。下方保留原各次提交范围供查找。

文档批次暂存本指南、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。若和 M7-04 验收文档一起提交，按两个文档清单合并暂存文件。

- 英文 `docs(ui): plan and verify the compact combat HUD layout`
- 中文 `docs(ui): 记录紧凑战斗 HUD 的布局与验收步骤`

本次单独提交第 2.1–2.2 节核对记录时，暂存这份指南。

- 英文 `docs(ui): record combat HUD layout and styling checks`
- 中文 `docs(ui): 记录战斗 HUD 布局与样式复验`

本次重叠问题的记录可以随同一份指南提交。

- 英文 `docs(ui): record stale-session HUD overlap recovery`
- 中文 `docs(ui): 记录旧会话 HUD 重叠的定位与恢复`

实现并通过实机验收后，暂存 `ETUIBootstrapComponent.h/.cpp`、第 2 节的四个 HUD WBP 和实际改动的 `BP_ETPlayerController.uasset`。来源清单随资产实物哈希更新。截图和日志保留在本地。

- 英文 `feat(ui): polish combat resource HUD and gate diagnostics`
- 中文 `feat(ui): 打磨战斗资源 HUD 并按需显示诊断面板`
