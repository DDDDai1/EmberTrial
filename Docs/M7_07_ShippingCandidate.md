# M7-07 Win64 Shipping 发布候选验证

以 `26832f0` 的正式资产和源码验证首次 Shipping 候选。先检查构建、Cook 和启动，再用独立进程走完首屏、两处存档与通关。预计手动 **30–45 分钟**，构建与 Cook 另计。[M7-06A 验收](Reports/M7_06A_Verification.md)

## 1. 构建与证据

配置校验为 **0 错误、0 警告**。Editor 全量回归 **60 项通过、0 失败、0 未运行**。报告在 `Artifacts/M7_07/Preflight/AutomationReport/index.json`。

首个归档为 `Artifacts/Builds/M7_07/Attempt01`。UE 5.5 的 UAT 完成 Win64 Shipping Build、Cook、Stage、Package 与 Archive，退出码 **0**，用时约 **2 分 52 秒**。同目录 `UAT_Console.log` 保留完整结果。

- 启动入口 `Windows/EmberTrial.exe`，内层程序 `Windows/EmberTrial/Binaries/Win64/EmberTrial-Win64-Shipping.exe`。
- 完整 `Windows` 目录含 IoStore 容器、运行库安装器及思源宋体许可，共 **546,488,048 字节**。包内许可与工程许可的 SHA-256 相符。
- 内层程序 SHA-256 为 `3FEABCF3ABEE91FFF2F0046A90282FCE6AABFCAB7DF6544B299F6B9CE455E843`。
- 实机启动和分发均以完整 `Windows` 目录为单位。

独立冒烟使用专用测试档，已看到中文首屏、干净档 Continue 禁用、Entry 默认资源、P 暂停、六件遗物的只读页面和返回焦点。实机由主菜单正常退出。小窗口下暂停面板会覆盖背景生命 HUD 标题的一角，后续两种窗口尺寸一并复查。

## 2. 三轮实机

先关闭 UE 编辑器和其他 EmberTrial 游戏进程。在项目根目录执行一次准备命令。

```powershell
Set-Location 'E:/UE_Project/Demo'
$candidate = 'E:/UE_Project/Demo/Artifacts/Builds/M7_07/Attempt01'
```

启动脚本给同一 Attempt 的三轮使用同一独立存档和设置目录。每轮开始前先从游戏菜单退出上一轮。需要重做干净档时改用新的 Attempt 或新验收目录。Shipping 结果以界面、操作和存档实物记录，启动 JSON 会记录 exe 和 IoStore TOC 的 SHA-256。

### Run01 首屏到检查点 B

```powershell
& './Tools/Run-StageBuild.ps1' -EvidenceStage M7_07 -BuildDirectory $candidate -RunName Run01 -Width 1920 -Height 1080
```

1. 全新用户目录首屏应禁用「继续本次试炼」。确认中文版标题、正式路线背景、菜单焦点与四边留白。用游戏设置切一次显示模式再取消，确认画面恢复。
2. 开始新试炼，从 Entry 出生。确认走路、锁定、轻击命中声、闪避、治疗与破势。设置中音效为 0 时，命中仍结算。复查后恢复音量。
3. 完成第一场并激活 A。在祭坛给两个槽装备遗物，滚动到列表首尾，核对状态文字与焦点框。清完第二场后激活 B，记下两个槽位和治疗次数，再由暂停返回主菜单并正常退出。

### Run02 读档、重试与通关

```powershell
& './Tools/Run-StageBuild.ps1' -EvidenceStage M7_07 -BuildDirectory $candidate -RunName Run02 -Width 1280 -Height 720
```

1. 继续应恢复到 B。确认前两道门开放、双槽和治疗次数与 Run01 一致。检查小窗设置页边距与遗物两行文字，普通暂停遗物页应为只读。
2. 让守关者击败玩家，确认失败页和 Retry 可用。再完成守关者战，从最终出口进入胜利页。清场应为 3，返回主菜单后 Continue 禁用。
3. 正常退出，保留 B 快照与通关标记的文件状态。

### Run03 通关后再开

```powershell
& './Tools/Run-StageBuild.ps1' -EvidenceStage M7_07 -BuildDirectory $candidate -RunName Run03
```

重启后继续禁用。新试炼应从 Entry 开始，生命 100、治疗 3/3、双槽空置。正常退出。

启动记录在 `Artifacts/M7_07/Runtime`。Attempt01 的共享验收存档位于 `Runtime/Attempt01/User/Saved/SaveGames`。遇到崩溃或黑屏时，保留对应启动 JSON、屏幕截图与 Windows 错误信息。

## 3. 验收回传

```text
M7-07 完成
实际 Attempt / Shipping exe / UAT 结果：
Run01 大窗口／设置回滚／短刃听感和路由：
Run01 A／B、双槽与治疗次数：
Run02 Continue B／小窗与只读／Retry：
Run02 清场 3／胜利返回／Continue 禁用：
Run03 重启／新局 Entry 与默认状态：
最明显问题（没有填无）／手动分钟／等待分钟：
```

## 4. Commit / Push

本批提交 M7-06A 验收报告、M7-05B-2 补交记录、发布候选指南、进度与路线文档和 `Tools/Run-StageBuild.ps1`。实机记录与归档包留在本地 `Artifacts/`。

- 英文 `chore(release): verify hit audio and prepare Shipping QA`
- 中文 `chore(release): 验收命中音效并准备 Shipping 验证`
