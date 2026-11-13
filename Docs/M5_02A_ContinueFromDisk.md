# M5-02A 主菜单读档与检查点恢复

已完成正式工程验收。[验收报告](Reports/M5_02A_Verification.md) · [M5-02B 覆盖确认与通关处理](M5_02B_SaveRetirement.md)

新会话从主菜单异步检查最新可用存档。点击「继续本次试炼」后，关卡到达并确认实际出生点，再恢复清场记录、遗物装配和剩余治疗次数。预计手动 20–35 分钟，构建等待另计。

| 场景 | 预期 |
| --- | --- |
| 有效 B 存档 | 首屏 Continue 可用。进入后在 B 出生，前两门开放，生息余烬和裂势印恢复，生命回到装配后上限，治疗次数按快照恢复 |
| A/B 中只有较早一槽可用 | 显示上一份有效存档及对应检查点。槽损坏回退由现有 `Slots` 专项覆盖 |
| 两槽均不存在或无法读取 | Continue 禁用，新试炼可用。无法读取时显示提示 |
| 点击开始新试炼 | 进入 Entry、清空当前会话装备并恢复默认资源。磁盘仍保留上一次成功写入的检查点，直到下一次存档。正式的覆盖确认、通关后的磁盘记录处理与迁移在后续批次接入 |

## 1. 应用补丁与构建

保存并关闭 UE 编辑器。在 PowerShell 逐条执行。[源码补丁](Patches/M5_02A_ContinueFromDisk.patch)

```powershell
Set-Location 'E:/UE_Project/Demo'
git apply --check './Docs/Patches/M5_02A_ContinueFromDisk.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留首条错误。' }
git apply './Docs/Patches/M5_02A_ContinueFromDisk.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留首条错误。' }
$saveProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$saveBuild = 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat'
& $saveBuild EmberTrialEditor Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误。' }
& './Tools/Refresh-CLion.ps1' -SkipBuild
& $saveBuild EmberTrial Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Game 构建失败，请保留首条错误。' }
```

重新加载 CLion 项目，检查三模块 CodeModel。补丁更新八份 Core 源码和三份 UI 源码，已通过隔离工程双构建和 60 项自动化。保留 M5-01C 生成的两份 PIE 存档，按第 3 节验证正式地图与主菜单。

## 2. 专项验收

逐条在 **Output Log** 运行。等待前一条运行完成再输入下一条。

```text
Automation RunTests EmberTrial.M5.Save
Automation RunTests EmberTrial.M4.Encounter.CheckpointResume
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
```

确认 Slots、Validation、RoundTrip、CheckpointResume、EquipmentTransactions 均为 Success。CheckpointResume 增加按磁盘清场 ID 恢复的样本，包含 B 点且清场列表为空时，按空列表处理。

## 3. 新会话读档实机

1. **打开主菜单。** 关闭上轮编辑器后再次打开 UE。打开 `L_ET_MainMenu` 并开启单人 PIE。等「正在检查试炼存档…」结束。「继续本次试炼」应可用，状态栏出现 `Checkpoint_B`。按上轮正式记录，此时预期 `Continue save found Id=Checkpoint_B Sequence=2 Fallback=0`。
2. **恢复 B。** 点击继续，核对从 B 出生。前两道门开放，前两队敌人无需再打，守关者仍待触发。生命应为 **120/120**，治疗 **2/3**。P 打开遗物面板，核对槽 A 生息余烬、槽 B 裂势印，再继续。日志应该有一条 `Disk continue restored Id=Checkpoint_B Sequence=2 Cleared=2 SlotA=life_ember SlotB=shatter_seal Healing=2`，以及 `Encounter restored from disk Id=intro`、`Id=mixed`。
3. **检查新试炼。** 此轮按 P 返回主菜单，选择开始新试炼。应从 Entry 出生、生命 **100/100**、治疗 **3/3**，P 查看两个空槽。验证第一场仍会生成后 Stop。
4. 若首屏提示存档不可用或实际出生、槽位、治疗次数与预期有差异，保留磁盘两份 PIE 存档，反馈对应日志。恢复阶段核对实际出生点、遗物目录、治疗容量及地图内的遭遇 ID，校验失败时会记录 `Disk continue used normal spawn defaults`。

```text
M5-02A 完成
十一旧源码 / Editor与Game Build / 三模块索引：
Slots / Validation / RoundTrip / CheckpointResume / EquipmentTransactions：
重开编辑器 / 首屏Continue可用 / B与序号：
B实际出生 / 前两门 / 守关者待触发：
生命120 / 治疗2 / 两槽与P继续：
新试炼Entry / 双空槽与默认资源 / 第一场与Stop：
Source已保存 / 失败或不理解处（无填无）：
完成时分 / 手动分钟 / 构建等待分钟：
```

## 4. Commit / Push

文档批暂存 M5-01C 验收报告、本指南和补丁，以及本批同步的 M5-01C 指南、README、Progress、Backlog、Architecture 与 AssetManifest。功能批在正式实机通过后暂存补丁对应的十一份源码。本地 PIE 存档和日志保留在 Saved。

| 批次 | 英文建议 | 中文建议 |
| --- | --- | --- |
| 文档与补丁 | `docs: verify checkpoint autosave and guide disk resume` | `docs: 验收检查点自动存档并补充磁盘读档指南` |
| 源码 | `feat(save): continue a trial from a disk checkpoint` | `feat(save): 从磁盘检查点继续试炼` |
