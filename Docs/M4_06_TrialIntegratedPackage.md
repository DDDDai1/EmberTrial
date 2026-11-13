# M4-06 完整试炼 Win64 Development 集成包

把当前完整路线打成独立包，验证 Cook 后的主菜单、三场遭遇、两处检查点、祭坛、失败与胜利结算。本章预计手动 **35–60 分钟**，构建和 Cook 等待另记。

## 1. 打包前检查

1. 工作区应包含已验收的 M4-05 源码和三份正式资产，对应提交 `7d0096a`。本章沿用正式地图和配置。
2. Stop PIE、Save All，关闭 UE 编辑器和其他 EmberTrial 游戏窗口。保留主菜单默认图 `L_ET_MainMenu`、主菜单 Controller 的 Gameplay Map `L_ET_TrialRoute` 与该图的 Cook 条目。
3. 从仓库根目录执行 `& './Tools/Validate-ProjectConfig.ps1'`，应为 PASS，Errors=0、Warnings=0。若在此后改了正式源码、地图、资产或共享配置，先完成受影响部分的复验。

已用正式工程的 Editor 二进制运行 `Automation RunTests EmberTrial`，**57 项成功，0 失败**。当前正式源码和资产与测试基线一致时，沿用这次结果。报告在 `Artifacts/M4_06/Preflight/AutomationReport/index.json`。

## 2. 构建、Cook 和归档

在 PowerShell 整段执行。已有 Attempt01 时改为 Attempt02 等新编号，后续两轮都用同一个实际目录。

```powershell
Set-Location 'E:/UE_Project/Demo'
$trialAttempt = 'E:/UE_Project/Demo/Artifacts/Builds/M4_06/Attempt01'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,EmberTrial -ErrorAction SilentlyContinue) {
    throw '请先关闭 UE 编辑器和 EmberTrial 游戏窗口。'
}
if (Test-Path -LiteralPath $trialAttempt) {
    throw 'Attempt 已存在。请使用新编号。'
}
New-Item -ItemType Directory -Path $trialAttempt | Out-Null
$trialProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$trialUatArgs = @(
    'BuildCookRun'
    "-project=$trialProject"
    '-target=EmberTrial'
    '-platform=Win64'
    '-clientconfig=Development'
    '-unrealexe=UnrealEditor-Cmd.exe'
    '-installed'
    '-nop4'
    '-unattended'
    '-utf8output'
    '-nocompileeditor'
    '-build'
    '-cook'
    '-stage'
    '-pak'
    '-iostore'
    '-compressed'
    '-package'
    '-prereqs'
    '-nodebuginfo'
    '-archive'
    "-archivedirectory=$trialAttempt"
)
$trialUatLog = Join-Path $trialAttempt 'UAT_Console.log'
& 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/RunUAT.bat' @trialUatArgs |
    Tee-Object -FilePath $trialUatLog
if ($LASTEXITCODE -ne 0) {
    throw "UAT 失败，保留 $trialUatLog 并反馈第一条错误。"
}
foreach ($trialPath in @('Windows/EmberTrial.exe', 'Windows/EmberTrial/Binaries/Win64/EmberTrial.exe', 'Windows/EmberTrial/Content/Paks')) {
    if (!(Test-Path -LiteralPath (Join-Path $trialAttempt $trialPath))) {
        throw "归档缺少 $trialPath。"
    }
}
Write-Host "归档目录 $trialAttempt"
```

确认 UAT 正常退出并保留完整 `Windows` 目录。以下命令使用包外层 exe 启动，脚本会输出本轮日志和启动 JSON 的完整路径。

## 3. Run01 主路线、B 点恢复与首次胜利

```powershell
Set-Location 'E:/UE_Project/Demo'
& './Tools/Run-StageBuild.ps1' -BuildDirectory 'E:/UE_Project/Demo/Artifacts/Builds/M4_06/Attempt01' -EvidenceStage M4_06 -RunName Run01
```

1. **从首屏进入。** 中文主菜单正常，「继续本次试炼」禁用。若要留一个跨进程读回样本，将输入卡灵敏度应用为 **1.25**，保持不反转。从「开始新试炼」进入，确认 Entry 和单份 HUD。
2. **推进到 B。** 第一场近战清场开门。A 点祭坛按 E，确认六件遗物，装备生息余烬到槽 A、裂势印到槽 B。活体生命上限变为 120，原当前生命只保留或夹限。继续打混合遭遇，击败第一名时门仍关，第二名倒下后开门，随后激活 B。
3. **走一次失败和继续。** 在守关者战斗中被敌人击败，从原失败页选「重新试炼」。确认 B 出生、前两门已开、两槽保留、玩家新身体生命 120/120，推进到守关者时新敌人 180。使用 P 返回主菜单，点「继续本次试炼」，再次确认 B 和两槽。若想缩短操作，Retry 后在 B 直接 P 返回即可。
4. **结算并再玩一次。** 击败守关者后在门内停一下，确认开门且仍能操作。过门走到 `TRIAL EXIT` 下方，确认只出现一份中文胜利页、清场数为 3、按 P 不回到战斗。用键盘确认「再玩一次」，回 Entry，装备空槽、默认资源恢复、第一场可重新触发。
5. **正常退出。** 回菜单，用菜单退出按钮结束 Run01。

## 4. Run02 新进程与胜利返回菜单

Run01 退出后从同一个 Attempt 启动，脚本会另存日志。两轮共用 `Artifacts/M4_06/Runtime/M4PackageUser.ini`。

```powershell
Set-Location 'E:/UE_Project/Demo'
& './Tools/Run-StageBuild.ps1' -BuildDirectory 'E:/UE_Project/Demo/Artifacts/Builds/M4_06/Attempt01' -EvidenceStage M4_06 -RunName Run02
```

1. **新进程首屏。** 继续禁用。若 Run01 已设置灵敏度，先只读核对 1.25。开始新试炼后确认 Entry、默认资源及两个空槽。
2. **通关并返回。** 完成 1／2／1 三场，穿过最终出口。确认胜利页清场数 3，用鼠标点击「返回主菜单」。Continue 应禁用。「开始新试炼」仍可用，确认从 Entry 出生，随后通过 P 回菜单并正常退出。
3. **保留证据。** 两个日志中查找 `Trial completed`、`Victory shown`。Run01 应有 `Intent=Retry`、`Intent=Continue` 的 B 点记录和一次同图 `Intent=NewRun` 的 Entry 记录。Run02 应有胜利返回后的 `Menu ready ... Continue=0`。若结果异常，保留当轮日志和截图。

## 5. 验收回传

```text
M4-06 完成
配置 PASS / 正式全组回归结果：
实际 Attempt / Build-Cook-Stage-Package / exe 与 UAT 日志：
Run01 首屏-Entry / 1-2-1 / 祭坛两槽：
Run01 Retry_B / Continue_B / 玩家120与守关者180：
Run01 胜利清场3 / P门控 / 键盘再玩一次 / Entry与空槽：
Run02 新进程Continue禁用 / 设置读回（未采可填未采）：
Run02 胜利鼠标返回 / Continue禁用 / 新试炼Entry：
两轮Runtime目录 / 正常退出：
失败或明显问题（无填无）：
完成时分 / 手动分钟 / 构建与Cook等待分钟：
```

## 6. Commit / Push

M4-05 功能源码和资产已提交。将本次验收报告、M4-06 指南、启动脚本以及同步的 README、Introduction、M4-05 指南、Progress、Backlog、Architecture、AcceptanceWorkflow_Options、VersionControl、AssetManifest 一起暂存。包、日志、启动 JSON 和专用 ini 保留在本地 `Artifacts`。本阶段若需要补修正式源码或资产，在对应复验通过后再提交补修。

- 英文 `chore(release): verify trial victory and prepare M4 packaging`
- 中文 `chore(release): 验收试炼胜利结算并准备 M4 集成打包`
