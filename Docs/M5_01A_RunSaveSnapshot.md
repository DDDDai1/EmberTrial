# M5-01A 版本化存档快照

本章把进度数据整理成一份可校验、可序列化的快照，并用 UE 的 `USaveGame` 内存接口验证往返。预计手动 15–25 分钟，构建等待另计。

| 字段 | 保存语义 |
| --- | --- |
| `SchemaVersion`、`Sequence`、`SavedAtUtc` | 格式版本、正整数序号、UTC 保存时间 |
| `MapPackage`、`CheckpointId` | 关卡长包名与检查点 ID |
| `ClearedEncounterIds`、`ClaimedRewardIds` | 已清场遭遇和已领取奖励的稳定 ID |
| `OwnedRelicIds`、`EquippedRelicA`、`EquippedRelicB` | 拥有和已装备的遗物 ID。装备必须在拥有列表内，同一遗物只占一槽 |
| `HealingCharges` | 保存时治疗剩余次数，允许为零 |

v1 校验返回有效、数据无效、版本不支持三种结果。关卡装配、检查点存在性、遗物定义与槽位兼容性由后续恢复环节按当前内容校验。本章只有值数据与内存往返，磁盘 A／B 槽、回退和迁移，以及游戏流程接入分批实现。编辑器内的 Continue 目前仍使用本进程会话。

## 1. 应用补丁并构建

保存并关闭 UE 编辑器。在 PowerShell 逐条执行。补丁新增 Core 中的存档头、实现及测试，共 3 个文件。[源码补丁](Patches/M5_01A_RunSaveSnapshot.patch)

```powershell
Set-Location 'E:/UE_Project/Demo'
git apply --check './Docs/Patches/M5_01A_RunSaveSnapshot.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留首条错误。' }
git apply './Docs/Patches/M5_01A_RunSaveSnapshot.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留首条错误。' }
$saveProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$saveBuild = 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat'
& $saveBuild EmberTrialEditor Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误。' }
& './Tools/Refresh-CLion.ps1' -SkipBuild
& $saveBuild EmberTrial Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Game 构建失败，请保留首条错误。' }
```

在 CLion 重新加载现有 CMake 项目。确认 Core 中能打开 `ETRunSaveGame.h`、`ETRunSaveGame.cpp` 和 `ETRunSaveSnapshotTests.cpp`，三模块 CodeModel 仍可见。本章直接从主菜单做最短检查，无需创建蓝图或改地图。补丁已通过 UE 5.5.4 隔离工程的 Editor、Game 双构建和全部 59 项自动化。正式工程按以下步骤验收。

## 2. 分组验收

重新打开 UE。在 **Output Log** 底部运行下面的命令，展开结果确认两个子项都报 Success。

```text
Automation RunTests EmberTrial.M5.Save
```

| 子项 | 预期 |
| --- | --- |
| `Validation` | 未来版本有独立拒绝结果。无效序号、空检查点、重复 ID、未拥有装备或装备重复均被拒绝。空槽与零次治疗有效。 |
| `RoundTrip` | 通过 UE SaveGame 序列化写入内存，反序列化回原生类后逐字段一致。测试不产生真实存档文件。 |

从主菜单开启一轮 PIE。首屏 Continue 禁用，新试炼从 Entry 进入，生命 100/100。按一次 P 打开暂停，再继续并 Stop。完整路线沿用 M4-06 已验收的包内结果。

```text
M5-01A 完成
三个新文件 / Editor 与 Game Build / 三模块索引：
Validation / RoundTrip：
首屏Continue禁用 / 新试炼Entry与100 / P继续 / Stop：
源码保存 / 失败或不理解处（无填无）：
完成时分 / 手动分钟 / 构建等待分钟：
```

## 3. Commit / Push

文档组先提交 M4-06 验收报告、本指南和补丁，以及本批修改的 README、Introduction、M4-06 指南、Progress、Backlog、Architecture、AcceptanceWorkflow_Options 和 VersionControl。源码组在你完成双构建与专项后提交补丁对应的三个文件。打包与日志仍在本地 `Artifacts`。

| 批次 | 英文建议 | 中文建议 |
| --- | --- | --- |
| 文档与补丁 | `docs: verify the M4 package and guide save snapshot setup` | `docs: 验收 M4 集成包并补充存档快照开发指南` |
| 源码 | `feat(save): add versioned trial snapshots` | `feat(save): 新增版本化试炼存档快照` |
