# M3-06C3 六件遗物版 Win64 Development 集成包

2026-11-09完成，UE 5.5.4。43项阶段回归、Win64 Development打包及两轮实机通过。以下保留打包与复验步骤。[C3验收报告](Reports/M3_06C3_Verification.md)

## 1. 本章验收范围

| 项目 | 本轮证据 |
| --- | --- |
| 阶段回归 | 43项全组Success，配置检查0错误、0警告 |
| 新包 | C3独立Attempt，完整Build／Cook／Stage／Package／Archive |
| Run01 | 六件Cook可达、长详情滚动、两槽装备、F80、检查点Retry与Continue |
| Run02 | 同包新进程，读回个人偏好，六件空槽、Entry出生、F60 |

已使用正式工程的Editor二进制执行全组，报告保存在`Artifacts/M3_06C3/Preflight/AutomationReport/index.json`。如果打包前只提交本批已验收测试、文档和启动工具，沿用这次43项结果。若修改了影响运行的源码、资源或共享配置，先按影响复验再打包。

已扩展`Tools/Run-StageBuild.ps1`的`M3_06C3`选项。它要求显式提供本阶段Attempt，并将日志、启动JSON和专用个人ini归档到`Artifacts/M3_06C3/Runtime`。启动器语法、C3目录保护、历史M3_06A路径和WhatIf已检查。

## 2. 打包准备

1. Stop PIE并Save All，保留正式地图、单敌人、祭坛、Entry与Checkpoint_A。关闭UE编辑器。
2. 工作区采用已验收的六件目录。打开PowerShell执行第3节。若已有本章Attempt01，换成Attempt02等新编号，并在第4、6节使用同一编号。
3. 保持C2测试源码为已验收的完整版本。该版本已包含在`115bdd0`。

## 3. UAT 完整构建与归档

在PowerShell整段执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
$m3C3Attempt = 'E:\UE_Project\Demo\Artifacts\Builds\M3_06C3\Attempt01'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue) {
    throw '请先保存并关闭UE编辑器。'
}
if (Test-Path -LiteralPath $m3C3Attempt) {
    throw 'Attempt已存在，请选新编号，并同步后两轮的启动路径。'
}
New-Item -ItemType Directory -Path $m3C3Attempt -Force | Out-Null
$m3C3UatArgs = @(
    'BuildCookRun'
    '-project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject'
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
    "-archivedirectory=$m3C3Attempt"
)
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\RunUAT.bat' @m3C3UatArgs |
    Tee-Object -FilePath (Join-Path $m3C3Attempt 'UAT_Console.log')
$m3C3Exit = $LASTEXITCODE
if ($m3C3Exit -ne 0) {
    throw "UAT失败，退出码$m3C3Exit。保留UAT_Console.log并反馈首条错误。"
}
$m3C3Required = @(
    'Windows\EmberTrial.exe'
    'Windows\EmberTrial\Binaries\Win64\EmberTrial.exe'
    'Windows\EmberTrial\Content\Paks'
)
foreach ($m3C3Relative in $m3C3Required) {
    if (!(Test-Path -LiteralPath (Join-Path $m3C3Attempt $m3C3Relative))) {
        throw "归档缺少 $m3C3Relative，请核对实际输出目录。"
    }
}
Write-Host "本轮包 $m3C3Attempt\Windows\EmberTrial.exe"
```

等到UAT退出码0、控制台出现`BUILD SUCCESSFUL`。保留整个`Windows`目录，后续按Game二进制和Cook容器核对。记录手动操作与构建等待时间。

## 4. Run01启动

UE保持关闭。若实际是Attempt02，改成实际路径。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M3_06C3\Attempt01' -EvidenceStage M3_06C3 -RunName Run01
```

默认请求1280×720。两轮共用`Artifacts/M3_06C3/Runtime/M3PackageUser.ini`，各自生成独立日志和JSON。保留脚本报告的实际启动路径。

## 5. Run01集成路线

1. **主菜单。** 新进程的Continue应禁用，设置区可读。输入卡把镜头灵敏度设为1.25并应用，保持Y轴不反转。如果专用ini已有1.25，选另一个容易识别的合法值并记录，给Run02留一个明确的读回样本。开始新试炼，从Entry出生，生命和体力100、治疗3、F就绪。
2. **祭坛六件。** 走到祭坛按E。核对六件、中文与第五比较行。选择逆焰刻印，滚动右侧详情，确认长说明和五行比较都能读完，确认／卸下区域固定。给槽A装备生息余烬，应为100/120。给槽B装备裂势印，确认第五行20→20和已装备提示。
3. **真实F与权限。** 继续后靠近满架势训练目标，站定按F一次。日志预期`Damage=80.0`、目标`Poise=100.0->20.0`。按P检查同一两件仍显示已装备，普通P页为只读。关闭后继续移动。
4. **检查点和新身体。** 进入Checkpoint_A并确认ACTIVE，再让敌人真实攻击至失败。Retry应从Checkpoint_A出生，生命120/120、体力100、治疗3、F就绪。用P查看生息与裂势仍在相应槽位。P返回主菜单后点Continue，再核对Checkpoint_A与两槽仍保留。
5. **退出。** 回菜单正常退出Run01。若某个操作未成功，只记录现象和对应日志，后续针对该段复验。

## 6. Run02新进程

Run01完全退出后，从同一Attempt启动。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M3_06C3\Attempt01' -EvidenceStage M3_06C3 -RunName Run02
```

1. 主菜单Continue应禁用。查看输入卡已读回Run01应用的值，先保持只读。可在游戏控制台输入`et.InputSettings`，记录日志中的实际灵敏度。
2. 开始新试炼，从Entry出生。P页应为六件、双空槽，生命和体力100、治疗3。关闭P，靠近满架势训练目标并站定按F一次，应结算60、目标架势100→40。
3. 正常回菜单退出。反馈实际Attempt和exe路径、两轮Runtime目录、六件与详情滚动、装配F80、Retry／Continue两槽、重开F60与设置读回、手动工时和构建等待。若有偏差，带上对应步骤和首条异常。

## 7. 验收结果

2026-11-09，实际Attempt01已通过Win64 Development打包与两轮独立验收。装配后F80，Retry和Continue恢复双槽，新进程从Entry空槽开始，F60且读回1.25倍灵敏度。六件与详情滚动由用户实机确认。手动30分钟，等待10分钟。[验收报告](Reports/M3_06C3_Verification.md)

## 8. Commit / Push

C2完整测试、启动工具与打包指南已在当前Git历史。本轮Stage验收报告、状态文档和下一章指南。包、UAT日志、Runtime和专用ini留在本地Artifacts。[两组提交清单](M4_01A_ProjectileFlightFoundation.md)

- 英文 `docs: verify six-relic package and plan projectile foundation`
- 中文 `docs: 完成六遗物集成包验收并规划投射物飞行基础`
