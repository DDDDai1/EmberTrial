# M5-02B 新试炼覆盖确认与通关存档处理

已完成正式工程验收。[验收报告](Reports/M5_02B_Verification.md) · [M5-03 存档迁移](M5_03_SaveMigration.md)

存在可继续进度时，主菜单第一次点击「开始新试炼」显示确认。确认并实际进入新试炼后，双槽写入结束标记。胜利结算也会写入结束标记，重开后已完成试炼的 Continue 禁用。预计手动 30–45 分钟，构建等待另计。

| 行为 | 结果 |
| --- | --- |
| 点击取消 | 恢复主菜单按钮，之前的 Continue 仍可使用 |
| 确认开始 | 切图成功后写结束标记，玩家从 Entry 以默认资源开始 |
| 胜利出口 | 屏幕显示原胜利页，存储依次写结束标记。两份读回确认后日志显示 `Save retired Sequence=…` |
| 退出后再打开 | 最新结束标记让 Continue 禁用。下一次有效检查点保存可重新提供 Continue |

标记通过原 A／B 槽写入并读回，保留序号排序。旧版本写出的有效 v1 快照仍可读取。通关后若立刻点再玩一次或返回菜单，界面可能提示正在结束旧存档。等成功行出现后再点即可。写入失败会留下日志，正式验收需先确认成功。

## 1. 应用补丁与构建

保存并关闭 UE 编辑器。在 PowerShell 逐条执行。[源码补丁](Patches/M5_02B_SaveRetirement.patch)

```powershell
Set-Location 'E:/UE_Project/Demo'
git apply --check './Docs/Patches/M5_02B_SaveRetirement.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留首条错误。' }
git apply './Docs/Patches/M5_02B_SaveRetirement.patch'
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留首条错误。' }
$saveProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$saveBuild = 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/Build.bat'
& $saveBuild EmberTrialEditor Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误。' }
& './Tools/Refresh-CLion.ps1' -SkipBuild
& $saveBuild EmberTrial Win64 Development "-Project=$saveProject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Game 构建失败，请保留首条错误。' }
```

CLion 重载后检查三个 CodeModel。补丁更新六份 Core 源码和三份 UI 源码。菜单沿用现有两个按钮及其直接子级文字。隔离工程 Editor、Game 双构建成功，60 项自动化均成功，其中 1 项同时记录了 EOS 后台请求警告。覆盖确认和通关后的按钮操作按第 3 节验证。

## 2. 自动化

在 Output Log 逐条运行，等前一条结束后再执行下一条。

```text
Automation RunTests EmberTrial.M5.Save
Automation RunTests EmberTrial.M4.Trial.Victory
```

Slots、Validation、RoundTrip 与 Victory 应报 Success。Slots 增加两份结束标记读回、丢失其中一份时继续禁用、以及后来新检查点覆盖结束标记三个场景。

## 3. 正式实机

以下操作会更新原 PIE 测试档。若希望留作历史样本，在操作前把 `EmberTrial_Run_PIE_A.sav`、`EmberTrial_Run_PIE_B.sav` 从 `EmberTrial/Saved/SaveGames` **复制**到本地 Artifacts。

1. **旧档兼容与取消。** 打开 `L_ET_MainMenu` 单人 PIE，等待首屏读档。应仍可继续 B。点一次「开始新试炼」，应看到「确认新试炼」和「取消」，焦点落在取消。按 Enter 取消，核对两按钮恢复、Continue 仍可用。
2. **通关处理。** 点 Continue，从 B 出生并击败守关者，穿过最终出口。胜利页显示清场 3。等日志出现 `Save retired Sequence=3`（若上轮仍是序号 2），再返回主菜单。确认 Continue 禁用。Stop，重新从主菜单开一轮 PIE，确认 Continue 仍禁用。
3. **重新生成可继续存档。** 开始新试炼，从 Entry 出生，击败第一名近战并激活 A。等 `Checkpoint saved Id=Checkpoint_A`，序号应比结束标记大 1。P 返回主菜单，确认 Continue 可用。
4. **确认覆盖。** 点击「开始新试炼」再点击「确认新试炼」。实际出生应为 Entry、两槽为空、资源默认。等 `Save retired` 后在抵达任何检查点之前 Stop。再开主菜单 PIE，Continue 应禁用。
5. 记录两次 `Save retired`、A 新存档序号和最后一轮的 `Disk continue menu ready Continue=0`。修改或保存仅涉及源码。

```text
M5-02B 完成
九旧源码 / Editor与Game Build / 三模块索引：
Slots / Validation / RoundTrip / Victory：
旧B可继续 / 点击开始与取消焦点 / 原档保留：
B通关 / 第一次Save retired序号 / 重开继续禁用：
新试炼A写入序号 / Continue恢复：
确认覆盖 / Entry默认资源空槽 / 第二次Save retired序号 / 重开继续禁用：
Source已保存 / 失败或不理解处（无填无）：
完成时分 / 手动分钟 / 构建等待分钟：
```

## 4. Commit / Push

文档组暂存 M5-02A 验收报告、本指南和补丁，以及更新的 M5-02A 指南、README、Progress、Backlog、Architecture。实机通过后提交补丁对应的九份源码。验收存档及日志留在本地。

| 批次 | 英文建议 | 中文建议 |
| --- | --- | --- |
| 文档与补丁 | `docs: verify disk resume and guide save retirement` | `docs: 验收磁盘读档并补充存档结束流程指南` |
| 源码 | `feat(save): confirm new trials and retire completed progress` | `feat(save): 新增新试炼覆盖确认与通关存档处理` |
