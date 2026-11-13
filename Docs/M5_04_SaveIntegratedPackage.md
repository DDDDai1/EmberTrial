# M5-04 存档功能 Win64 Development 集成验收

用同一份独立包启动三个进程，验证正式路线 Cook、A／B 保存、关闭进程后从 B 恢复、通关结束旧档，以及新进程重开。预计手动 30–45 分钟，Build 和 Cook 等待另计。

## 1. 打包前检查

1. 正式工程的存档迁移源码基线为 `61182ea`。保存工作后退出编辑器和其他 EmberTrial 游戏窗口。
2. 在仓库根目录执行 `& './Tools/Validate-ProjectConfig.ps1'`。预期 PASS、0 错误、0 警告。
3. 本章使用新的 Attempt。三个进程共用同一 Attempt 的验收用户目录。首次执行使用 Attempt01。复跑若要验证全新首屏，使用新的 Attempt 编号并同步后续启动命令。

已用正式工程 Editor 二进制执行全组回归，60 项通过，0 失败。其中 1 项伴随 EOS 后台请求警告，三项存档专项均无警告。源码、资产与配置沿用本次基线即可使用该结果。报告保存在 `Artifacts/M5_04/Preflight/AutomationReport/index.json`。

## 2. 构建、Cook 与归档

在 PowerShell 整段执行，完成后保留完整 `Windows` 目录和 UAT 日志。

```powershell
Set-Location 'E:/UE_Project/Demo'
$saveAttempt = 'E:/UE_Project/Demo/Artifacts/Builds/M5_04/Attempt01'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,EmberTrial -ErrorAction SilentlyContinue) {
    throw '请先关闭 UE 编辑器和 EmberTrial 游戏窗口。'
}
if (Test-Path -LiteralPath $saveAttempt) {
    throw 'Attempt 已存在，请选新编号。'
}
New-Item -ItemType Directory -Path $saveAttempt | Out-Null
$saveProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$saveUatArgs = @(
    'BuildCookRun', "-project=$saveProject", '-target=EmberTrial', '-platform=Win64',
    '-clientconfig=Development', '-unrealexe=UnrealEditor-Cmd.exe', '-installed',
    '-nop4', '-unattended', '-utf8output', '-nocompileeditor',
    '-build', '-cook', '-stage', '-pak', '-iostore', '-compressed',
    '-package', '-prereqs', '-nodebuginfo', '-archive', "-archivedirectory=$saveAttempt"
)
$saveUatLog = Join-Path $saveAttempt 'UAT_Console.log'
& 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/RunUAT.bat' @saveUatArgs | Tee-Object -FilePath $saveUatLog
if ($LASTEXITCODE -ne 0) {
    throw "UAT 失败，保留 $saveUatLog 并反馈第一条错误。"
}
foreach ($saveOutput in @('Windows/EmberTrial.exe', 'Windows/EmberTrial/Binaries/Win64/EmberTrial.exe', 'Windows/EmberTrial/Content/Paks')) {
    if (!(Test-Path -LiteralPath (Join-Path $saveAttempt $saveOutput))) {
        throw "归档缺少 $saveOutput。"
    }
}
```

应出现 `BUILD SUCCESSFUL` 及 UAT 退出码 0。实际 exe 路径以启动脚本输出为准。

## 3. 三个独立进程

从仓库根目录执行下列准备命令，再分别运行各小节的启动命令。每轮先用游戏主菜单退出，等进程退出后再下一轮。

```powershell
Set-Location 'E:/UE_Project/Demo'
$package = 'E:/UE_Project/Demo/Artifacts/Builds/M5_04/Attempt01'
```

### Run01 从 Entry 走到 B 并退出

```powershell
& './Tools/Run-StageBuild.ps1' -BuildDirectory $package -EvidenceStage M5_04 -RunName Run01
```

1. 等首屏存档检查结束，Continue 应禁用。开始新试炼，从 Entry 出生，生命 100/100、治疗 3/3。
2. 完成第一场并踩到 A。等日志出现 `Checkpoint saved Id=Checkpoint_A ... Schema=2`。在祭坛给槽 A 装备生息余烬，给槽 B 装备裂势印。
3. 混合遭遇有两名敌人。受伤后在安全距离按 Q 一次，等生效并确认治疗为 2/3。清完第二场并激活 B。日志应记录清场 2、两个遗物 ID、当前治疗次数和 `Schema=2`。全新用户目录下 A 为序号 1，B 为序号 2。
4. P 返回主菜单并正常退出。保留启动元数据和用户目录中的两份存档。

### Run02 跨进程恢复、Retry B 与通关

```powershell
& './Tools/Run-StageBuild.ps1' -BuildDirectory $package -EvidenceStage M5_04 -RunName Run02
```

1. 首屏等待检查完成，应显示 B 点存档且 Continue 可用。进入后确认 B 出生、前两道门开放、两个遗物槽恢复、生命 120/120，治疗次数与 Run01 的 B 快照一致。
2. 让守关者击败你，选择重新试炼。确认本进程 Retry 仍从 B 出生，装备保留，新守关者 180/180。记录 Retry 后 HUD 的实际治疗次数。
3. 通关后穿过最终出口。胜利页应显示清场 3。等 `Save retired Sequence=…` 再返回主菜单，Continue 应禁用。正常退出。

### Run03 确认通关后的重启与新局

```powershell
& './Tools/Run-StageBuild.ps1' -BuildDirectory $package -EvidenceStage M5_04 -RunName Run03
```

等菜单存档检查结束，确认 Continue 禁用。开始新试炼，验证回到 Entry、生命 100/100、治疗 3/3、双空槽。P 返回主菜单，正常退出。

运行证据在 `Artifacts/M5_04/Runtime`。以 Attempt01 为例，存档在 `Artifacts/M5_04/Runtime/Attempt01/User/Saved/SaveGames`，文件为 `EmberTrial_Run_A.sav` 和 `EmberTrial_Run_B.sav`。三个启动 JSON 中应使用同一 exe SHA-256 和同一用户目录。

## 4. 验收回传

```text
M5-04 完成
配置结果 / 正式全组回归：
实际Attempt / Build-Cook-Stage-Package / exe与UAT日志：
Run01 首屏继续禁用 / Entry / A-B序号与Schema：
Run01 B点双槽 / 治疗实际值 / 正常退出：
Run02 首屏继续可用 / 恢复B、前两门、双槽、120与治疗：
Run02 死亡Retry_B / 180与治疗实际值 / 守关者通关：
Run02 清场3 / Save retired序号 / Continue禁用 / 正常退出：
Run03 重启Continue禁用 / Entry双空槽与默认资源 / 正常退出：
三轮Runtime目录 / 双槽目录 / 失败或明显问题（无填无）：
完成时分 / 手动分钟 / Build与Cook等待分钟：
```

## 5. Commit / Push

暂存 M5-03 验收报告、本指南、M5-03 指南、README、Progress、Backlog、Architecture 和 `Tools/Run-StageBuild.ps1`。本地包、运行日志和测试存档留在 Artifacts。

- 英文 `chore(release): verify save migration and prepare persistence packaging`
- 中文 `chore(release): 验收存档迁移并准备持久化集成包`
