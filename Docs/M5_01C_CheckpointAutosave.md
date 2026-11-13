# M5-01C 检查点自动存档

正式路线首次激活 A 或 B 时，游戏线程采集当前关卡、检查点、已清场遭遇、遗物装配和剩余治疗次数。双槽存储确认写后读回才打印 `Checkpoint saved`。本章预计手动 25–40 分钟，构建另计。

| 场景 | 本批行为 |
| --- | --- |
| 新试炼从 Entry 出生 | Entry 是出生点。只有启用自动存档的检查点激活时才写盘 |
| 首次激活 A | 记录第一场清场与当时的遗物、治疗快照 |
| 激活 B | 记录前两场清场和更新后的快照。保存进行中再次激活检查点时，保留最新待写快照 |
| 重叠当前已激活的检查点 | 沿用会话去重规则 |
| PIE 验收 | 写入 `EmberTrial_Run_PIE_A/B`。独立游戏使用 `EmberTrial_Run_A/B` |

跨进程加载与主菜单 Continue 将在后续接入。当前阶段从主菜单测试，手动配置仅涉及正式路线实例。

## 1. 应用源码与构建

保存并关闭 UE 编辑器。在 PowerShell 逐条执行。[源码补丁](Patches/M5_01C_CheckpointAutosave.patch)

```powershell
Set-Location 'E:/UE_Project/Demo'
git apply --check './Docs/Patches/M5_01C_CheckpointAutosave.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留首条错误。' }
git apply './Docs/Patches/M5_01C_CheckpointAutosave.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留首条错误。' }
$saveProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$saveBuild = 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat'
& $saveBuild EmberTrialEditor Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误。' }
& './Tools/Refresh-CLion.ps1' -SkipBuild
& $saveBuild EmberTrial Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Game 构建失败，请保留首条错误。' }
```

CLion 重新加载 CMake 项目。确认三模块 CodeModel 正常，Core 能在 `ETRunSessionSubsystem.cpp` 导航到存档和资源组件。补丁更新七份现有 Core 文件，已通过隔离工程 Editor、Game 双构建和全组 60 项自动化。正式地图的实例配置与 A、B 触发仍需按下文实机验证。

## 2. 正式地图设置

1. 打开 `/Game/EmberTrial/Maps/L_ET_TrialRoute`。在 **World Outliner** 按下表逐个选择**地图里的 Actor 实例**，在 **Details** 搜索 **Encounter ID**，输入指定值并按 Enter。

| Actor 实例 | Encounter ID |
| --- | --- |
| `IntroEncounter_A` | `intro` |
| `MixedEncounter_A` | `mixed` |
| `GuardianEncounter_Route` | `guardian` |

2. 在 Outliner 选 `Start_Checkpoint_A`，打开 Details 搜索 **Autosave On Activation**，勾选。对 `Start_Checkpoint_B` 重复一次。核对两者的 **Player Start Tag** 仍为 `Checkpoint_A`、`Checkpoint_B`。
3. **Save All**。三个 ID 必须各不相同。若输出 `InvalidEncounterId Actor=...`，根据日志中的 Actor 核对实例 ID。

## 3. 验收

1. 在 Output Log 逐条运行，核对四个 Success。Slots 增加独立 PIE 槽的真实写入与回读。

```text
Automation RunTests EmberTrial.M5.Save
Automation RunTests EmberTrial.M4.Encounter.CheckpointResume
```

2. 从正式主菜单开一次单人 PIE。Entry 出生后先检查 100/100。击败第一名近战，激活 A。Output Log 应有一条 `Checkpoint saved Id=Checkpoint_A`，其中 `Cleared=1`。离开并再次进入 A，确认该轮没有第二条 A 保存成功。
3. 在祭坛装备生息余烬到槽 A、裂势印到槽 B。进入混合房，清完两名敌人后激活 B。日志应出现 `Checkpoint saved Id=Checkpoint_B`，其中 `Cleared=2 SlotA=life_ember SlotB=shatter_seal`，序号比本轮 A 大 1。如途中用过 Q，`Healing` 应与当时 HUD 一致。
4. 等 B 的保存成功行出现后再 Stop。对应文件位于 `E:/UE_Project/Demo/EmberTrial/Saved/SaveGames`，检查 `EmberTrial_Run_PIE_A.sav` 与 `EmberTrial_Run_PIE_B.sav`。若已有先前测试槽，按本轮序号和文件更新时间核对。

```text
M5-01C 完成
七旧源码 / Editor 与 Game Build / 三模块索引：
三遭遇 ID / A-B Autosave 配置 / 正式地图保存：
Slots / Validation / RoundTrip / CheckpointResume：
Entry 与100 / A清场1与重入不重复：
B清场2 / 遗物两槽 / Healing实际值 / 两次序号：
PIE双槽与Stop / 失败或不理解处（无填无）：
完成时分 / 手动分钟 / 构建等待分钟：
```

## 4. Commit / Push

文档批暂存本指南、补丁、M5-01B 验收报告，以及本批更新的 M5-01B 指南、README、Progress、Backlog、Architecture。功能批在实机通过后暂存补丁对应的七份源码和 `EmberTrial/Content/EmberTrial/Maps/L_ET_TrialRoute.umap`。本地 `Saved/SaveGames` 保留验收用存档。

| 批次 | 英文建议 | 中文建议 |
| --- | --- | --- |
| 文档与补丁 | `docs: verify dual-slot storage and guide checkpoint autosave` | `docs: 验收双槽存档并补充检查点自动存档指南` |
| 源码与地图 | `feat(save): autosave trial progress at checkpoints` | `feat(save): 在检查点自动保存试炼进度` |
