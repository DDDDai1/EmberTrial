# M5-03 存档 v1 → v2 迁移与回退

读取 v1 时，存储层将旧关闭标记 `bRetired` 转成 v2 的 `bRunClosed`，保存原检查点、序号、遗物、治疗次数和清场记录。读取过程在内存中完成迁移。下一次检查点写入使用 v2。预计手动 20–30 分钟，构建等待另计。

| 场景 | 预期 |
| --- | --- |
| v1 进度档 | 读入后可继续，序号和进度保持 |
| v1 结束档 | 迁移后保持关闭，菜单 Continue 禁用 |
| 较新槽损坏 | 回退到有效旧槽，若旧槽为 v1，读取时升级 |
| 未知未来版本 | 保留原档并返回版本不支持 |
| 新存档 | 写出 v2，沿用原双槽序号排序 |

## 1. 应用补丁与构建

关闭 UE 编辑器后，在 PowerShell 逐条执行。[源码补丁](Patches/M5_03_SaveMigration.patch)

```powershell
Set-Location 'E:/UE_Project/Demo'
git apply --check './Docs/Patches/M5_03_SaveMigration.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留首条错误。' }
git apply './Docs/Patches/M5_03_SaveMigration.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留首条错误。' }
$saveProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$saveBuild = 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat'
& $saveBuild EmberTrialEditor Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误。' }
& './Tools/Refresh-CLion.ps1' -SkipBuild
& $saveBuild EmberTrial Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Game 构建失败，请保留首条错误。' }
```

重新加载 CLion，检查 Core、UI、Editor CodeModel。补丁更新六份 Core 源码，已通过隔离工程双构建和 60 项自动化。保留上轮写入的 PIE 双槽，用于正式工程实机复验。

## 2. 分组验收

1. 在 **Output Log** 运行 `Automation RunTests EmberTrial.M5.Save`。等待 `Validation`、`RoundTrip`、`Slots` 全部 Success。Slots 在独立测试槽写入 v1，损坏较新的槽后从 v1 回退并迁移。Validation 检查 v1 结束标记和无效旧档，RoundTrip 核对 v2 结束标记。
2. 从 `L_ET_MainMenu` 开一轮单人 PIE。沿用 M5-02B 结束后留下的双槽，首屏应为 `Disk continue menu ready Continue=0`。有此结果即可验证旧结束档仍禁用。无需手动修改文件。
3. 开新试炼，从 Entry 出生，击败第一名近战并激活 A。等待 `Checkpoint saved Id=Checkpoint_A ... Schema=2`。若原档为序号 5，本次应为 **6**。等写入完成后 Stop。
4. 从主菜单再开一轮 PIE。应能看到 `Continue save found Id=Checkpoint_A Sequence=6 Fallback=0 Schema=2`（序号以实际值为准）。继续后确认从 A 出生、第一道门开放，再 Stop。

```text
M5-03 完成
六旧源码 / Editor与Game Build / 三模块索引：
Slots / Validation / RoundTrip：
旧v1结束档首屏Continue禁用：
新试炼Entry / A写入Schema与序号：
重开继续A / 第一门恢复 / Stop：
Source已保存 / 失败或不理解处（无填无）：
完成时分 / 手动分钟 / 构建等待分钟：
```

## 3. Commit / Push

文档批暂存 M5-02B 验收报告、本指南和补丁，以及同步的 M5-02B 指南、README、Progress、Backlog 与 Architecture。正式工程通过后暂存补丁对应的六份源码。存档及运行日志留在本地。

| 批次 | 英文建议 | 中文建议 |
| --- | --- | --- |
| 文档与补丁 | `docs: verify save retirement and guide schema migration` | `docs: 验收通关存档处理并补充存档版本迁移指南` |
| 源码 | `feat(save): migrate version 1 trial snapshots` | `feat(save): 迁移 v1 试炼存档快照` |
