# M4-05 试炼出口与胜利结算

已完成正式工程验收。[M4-05 验收报告](Reports/M4_05_Verification.md) · [M4-06 集成包](M4_06_TrialIntegratedPackage.md)

击败守关者后，玩家继续穿过最终出口。出口确认三场遭遇都已清场，显示「试炼完成」和清场数，并提供「再玩一次」与「返回主菜单」。预计手动 **45–75 分钟**，包含两次完成结算和一次死亡重试，构建等待另计。

## 1. 应用源码并构建

保存并关闭 UE 编辑器，在 PowerShell 逐条执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
git apply --check './Docs/Patches/M4_05_TrialVictory.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留错误输出' }
git apply './Docs/Patches/M4_05_TrialVictory.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留错误输出' }
$project = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$build = 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat'
& $build EmberTrialEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误' }
& './Tools/Refresh-CLion.ps1' -SkipBuild
& $build EmberTrial Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Game 构建失败，请保留首条错误' }
```

在 CLion 重新加载现有 CMake 项目，确认 Core 中可导航到 `ETTrialExit.h`，UI 中可导航到 `ETFailureWidget.h`，Editor 模块仍在索引中。补丁新增出口 Actor 的两份源码和一份专项测试，更新七份既有源码。隔离工程已通过双构建和 57 项自动化，正式工程仍需下面的资产装配与实机检查。[源码补丁](Patches/M4_05_TrialVictory.patch)

### 流程与职责

- `AETTrialExit` 在 Pawn 进入查询盒时读取关卡中配置的遭遇。检查点恢复的清场计入统计，引用缺失、重复或生成失败后放开的门均不能形成胜利。
- `UETRunSessionSubsystem` 核对当前世界、受控存活角色与切图状态，提交一次结算并发布事件。完成后主菜单的「继续本次试炼」禁用。「再玩一次」复用 `NewRun`，在新世界确认后重置到 Entry、空装备槽和初始资源。
- 既有 `ETFailureFlowComponent` 消费会话事件，负责结算页、暂停与输入焦点。胜利和失败各自使用一个 Widget Class，两页共用已有原生按钮绑定。关卡资产配置引用，Widget Designer 配置标题和按钮文字。

## 2. 在正式地图放置出口

1. 打开 `/Game/EmberTrial/Maps/L_ET_TrialRoute`。在 Place Actors 搜索 **ET Trial Exit** 或 **ETTrialExit**，将原生类拖入地图。Outliner 中命名 **TrialExit_A**。这一章只放一个实例。
2. 选 **TrialExit_A**，在 **Details → Transform** 填 **Location X=2820、Y=0、Z=120**，Rotation 三轴全 0，Scale 三轴全 1。出口盒位于守关者门的后方，现有 `TRIAL EXIT` 文字下方。
3. 仍选 **TrialExit_A**，在 Details 搜索 **Required Encounters**。按三次 **+** 得到索引 0、1、2。每项点击吸管，在 Outliner 依次选择下表中的**地图实例**。三个实例会引用同一个蓝图类，核对显示的实例名。

| 索引 | 实例 |
| --- | --- |
| 0 | `IntroEncounter_A` |
| 1 | `MixedEncounter_A` |
| 2 | `GuardianEncounter_Route` |

4. 保持原生查询盒默认半径 **X=90、Y=160、Z=130**（Details 中名为 **Box Extent**，表示半尺寸）。盒子覆盖最后一个门洞方向上的通行线。沿路线中心从门走向 `TRIAL EXIT` 即可触发。保存地图。

## 3. 制作胜利页并配置 Controller

1. 在内容浏览器进入 `/Game/EmberTrial/ui/Flow`。选中原 `WBP_ETFailure`，用 **Ctrl+D** 复制为 **WBP_ETVictory**。打开新资产。工具栏 **Class Settings → Parent Class** 应为 **ETFailureWidget**。
2. 切到 **Designer**。在 **Hierarchy** 展开 `Canvas_Root → Border_Background → VBox_Failure`。分别选中 `Text_Title`、`Text_Subtitle` 和 `RetryButton → Text_Retry`，在 **Details → Content → Text** 填入 **试炼完成**、**已抵达试炼出口** 和 **再玩一次**。保留 MainMenuButton 的「返回主菜单」。清空 `StatusText` 的设计时失败提示，清场数由 C++ 写入。
3. 在 Hierarchy 逐个核对下表。控件名称在选中后可从 Details 顶部查看，保留勾选 **Is Variable**。按钮的子文本名字不限。保持两个按钮 **Is Focusable**，文字和按钮均沿用原页的无 Binding 设置。选 **Class Defaults** 搜索 **Tick Frequency**，保持 **Never**。**Compile → Save**。

| 必需名称 | Designer 控件类型 | 运行时用途 |
| --- | --- | --- |
| `RetryButton` | Button | 再玩一次，打开同图新试炼 |
| `MainMenuButton` | Button | 返回主菜单 |
| `StatusText` | Text Block | 显示「已清场遭遇：3」或请求状态 |

4. 打开 `/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`。在左侧 **Components** 选中已有 **FailureFlow**。Details 搜索 **Victory Widget Class**，选择 **WBP_ETVictory**。清空搜索，确认 **Failure Widget Class=WBP_ETFailure**、暂停 Widget 与 **Main Menu Map** 仍按原值配置。**Compile → Save**，最后 **Save All**。

## 4. 分组验收

### 4.1 自动化

打开 Editor 的 **Output Log**，将下列命令逐条执行，等上一条输出 **Success** 后再执行下一条。这三项覆盖本章出口与检查点路径。

```text
Automation RunTests EmberTrial.M4.Trial.Victory
Automation RunTests EmberTrial.M4.Encounter.CheckpointResume
Automation RunTests EmberTrial.M4.Encounter.Mixed
```

`Victory` 使用真实触发盒重叠，覆盖等待中的守关者、生成失败、重复遭遇引用、重复进入去重，以及确认新试炼后清空结算。它使用测试世界，不验证 WBP 外观、焦点或编辑器里的地图引用。

### 4.2 实机两轮

1. 从正式主菜单开始，完成前两场，至少在祭坛装备一件遗物，再激活 B。到达守关者后先输一次，检查仍是原失败页。选「重新试炼」，确认从 B 出生、前两门已开、装备保留、新守关者 180 生命。
2. 击败守关者后先停在门内，确认门打开且画面尚未结算。再从门洞沿中心线走到 `TRIAL EXIT` 文字下方。应只出现一份「试炼完成」，清场数 **3**。检查鼠标可用、Tab 能在两个按钮间移动，按 P 或移动键不会回到战斗。用 **Enter** 激活已聚焦的「再玩一次」。确认新世界从 Entry 开始，槽位为空，第一场重新可触发，输入恢复。
3. 在新一轮通关后再到出口，用鼠标点击「返回主菜单」。检查「继续本次试炼」禁用。「开始新试炼」仍可用，进入后出生在 Entry。停止 PIE，确认界面和鼠标焦点清理。

| 现象 | 检查位置 |
| --- | --- |
| 门已开但穿过出口无结果 | 先确认角色到达 X≈2820、Y≈0；在 Outliner 检查出口的三份引用、原生盒大小，以及角色胶囊的 Generate Overlap Events。日志没有 `Trial completed` 时优先查出口与遭遇状态。 |
| 有 `Trial completed` 但未出现页面 | 在正式 Controller 的 FailureFlow 检查 Victory Widget Class；编译新 WBP 并确认三项绑定。成功应同时看到 `Victory shown ... ClearedEncounters=3 Paused=1`。 |
| 再玩一次回到 B 或装备未清空 | 查 `Travel requested`、`Snapshot committed` 是否为 `NewRun` 和 `Entry`。保留当前轮日志并记录触发按钮。 |

请回传双构建 / 三索引、出口三个引用与胜利 WBP / Controller 保存、三项自动化、失败 Retry_B、清场计数与 P 门控、键盘再玩一次、鼠标返回菜单与 Continue 禁用，以及完成时分和手动 / 等待分钟。日志有异常时附首条错误或对应段落。

## 5. Commit / Push

M4-04 的源码与资产已在本地提交。可先提交路线验收报告、本章指南与补丁及相关文档更新，再在正式工程实机通过后提交新功能。

| 批次 | 暂存内容 | 建议 Commit | 中文翻译 |
| --- | --- | --- | --- |
| 文档与补丁 | `Docs/Reports/M4_04_Verification.md`、`Docs/M4_05_TrialVictory.md`、`Docs/Patches/M4_05_TrialVictory.patch`，以及本批修改的 README、Introduction、M4-04 指南、Progress、Backlog、Architecture、AcceptanceWorkflow_Options、VersionControl 和 AssetManifest | `docs: verify trial route and guide victory flow setup` | `docs: 验收试炼路线并补充胜利结算装配指南` |
| 功能与资产 | 补丁对应的十份 Source、`EmberTrial/Content/EmberTrial/Maps/L_ET_TrialRoute.umap`、`EmberTrial/Content/EmberTrial/ui/Flow/WBP_ETVictory.uasset`、`EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset` | `feat(flow): add trial victory and replay` | `feat(flow): 新增试炼胜利结算与再开一局` |

在 CLion 检查暂存区中的地图和 WBP 走 Git LFS，随后 Commit / Push。构建、日志和隔离工程继续留在本机。
