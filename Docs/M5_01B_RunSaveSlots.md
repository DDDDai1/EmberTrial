# M5-01B 双槽存档读写与回退

本章实现独立的游戏进度存储。两个槽轮流写入，写后从目标槽读回同一份数据才报告成功。读档选择最高序号的有效快照，另一槽损坏时可恢复前一份。本批预计手动 **20–30 分钟**，构建等待另计。

| 场景 | 预期 |
| --- | --- |
| 首次写入 | 写 A 槽，存储层分配序号 1 和 UTC 保存时间 |
| 再次写入 | 保留有效快照，写另一槽。读回校验通过后返回新快照 |
| 最新槽损坏 | 校验长度和 CRC，读取前一份有效快照并返回回退信息 |
| 两槽均无有效快照，且存在损坏文件 | 返回损坏状态，保留原文件 |
| 任一槽出现未知存档版本 | 返回版本不支持，保护该存档 |
| 读写请求重叠 | 存储层报告忙。调用方可在前一请求结束后重试 |

`UETSaveSubsystem` 使用 UE 平台异步存档接口，序列化和结果回调在游戏线程，单个请求串行读两个槽。CRC 用于检测意外损坏。后续接入会话触发点、存档域校验与界面反馈，本章通过正式接口在独立测试槽执行读写。

## 1. 应用补丁并构建

保存并关闭 UE 编辑器。在 PowerShell 逐条执行。[源码补丁](Patches/M5_01B_RunSaveSlots.patch)

```powershell
Set-Location 'E:/UE_Project/Demo'
git apply --check './Docs/Patches/M5_01B_RunSaveSlots.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留首条错误。' }
git apply './Docs/Patches/M5_01B_RunSaveSlots.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留首条错误。' }
$saveProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$saveBuild = 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat'
& $saveBuild EmberTrialEditor Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误。' }
& './Tools/Refresh-CLion.ps1' -SkipBuild
& $saveBuild EmberTrial Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Game 构建失败，请保留首条错误。' }
```

重新加载 CLion 项目。在 Core 的 `Public/Save`、`Private/Save` 和 `Tests` 中确认 `ETSaveSubsystem.h`、`ETSaveSubsystem.cpp`、`ETRunSaveSlotsTests.cpp`。同时核对 UI、Editor 两个 CodeModel。

UE 5.5.4 隔离工程的 Editor、Game 双构建及 60 项自动化均通过。新专项验证了 A／B 写入、最高序号读取、损坏回退、未知版本保护与忙碌拒绝。

## 2. 分组验收

1. 打开 UE，在 **Output Log** 运行 `Automation RunTests EmberTrial.M5.Save`。等 `Slots`、`Validation`、`RoundTrip` 都报 Success。新测试使用带 GUID 的 `EmberTrial_Auto_…` 槽，结束后清理。正常跑完无需手动创建或删除文件。
2. 只做一次主菜单短查。首屏 Continue 禁用，开始试炼从 Entry 出生，生命 100/100。P 暂停并继续，然后 Stop。

```text
M5-01B 完成
三新源码 / Editor 与 Game Build / 三模块索引：
Slots / Validation / RoundTrip：
首屏继续禁用 / Entry与100 / P继续 / Stop：
Source已保存 / 失败或不理解处（无填无）：
完成时分 / 手动分钟 / 构建等待分钟：
```

## 3. Commit / Push

文档组暂存 M5-01A 验收报告、本指南及补丁，以及本批更新的 M5-01A 指南、Progress、Backlog、Architecture 与 README。正式验收后提交补丁对应的三个源码文件。

| 批次 | 英文建议 | 中文建议 |
| --- | --- | --- |
| 文档与补丁 | `docs: verify save snapshots and guide dual-slot storage` | `docs: 验收存档快照并补充双槽存储开发指南` |
| 源码 | `feat(save): add asynchronous dual-slot storage and fallback` | `feat(save): 新增异步双槽存档与回退` |
