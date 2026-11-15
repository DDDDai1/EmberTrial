# M6-06 Windows 音频缓冲配置与独立包复验

2026-11-15 已由助手完成配置、打包与实机复验，实际为 `Attempt02`。两次双路探针以会话峰值与游戏日志验证，人工听感未采。当前等待用户提交。[验收报告](Reports/M6_06_Verification.md)

将 M6-04 的 512 帧候选写入 Windows 项目配置，再验证新包默认采用 512 帧、切图收益复现，以及换图前后和交战时的听感。预计手动 15–25 分钟，另计 Build / Cook 等待。[M6-04 对照](Reports/M6_04_Verification.md) · [M6-05 加载归因](Reports/M6_05_Verification.md)

## 1. 修改配置并打包

1. 保存并关闭 UE 编辑器，退出所有 EmberTrial 游戏窗口。
2. 在 [DefaultEngine.ini](../EmberTrial/Config/DefaultEngine.ini) 搜索 `AudioCallbackBufferFrameSize`。把 `[/Script/WindowsTargetPlatform.WindowsTargetSettings]` 下的 `AudioCallbackBufferFrameSize=1024` 改为 `AudioCallbackBufferFrameSize=512`，确认此节仅有一条该属性并保存。
3. 在 PowerShell 整段执行。若 `Attempt01` 已存在，换新的编号，并在第 2 节使用同一编号。

```powershell
Set-Location 'E:/UE_Project/Demo'
$audioPackageAttempt = 'E:/UE_Project/Demo/Artifacts/Builds/M6_06/Attempt01'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,EmberTrial -ErrorAction SilentlyContinue) {
    throw '请先关闭 UE 编辑器和 EmberTrial 游戏窗口。'
}
if (Test-Path -LiteralPath $audioPackageAttempt) {
    throw 'Attempt 已存在，请选新编号。'
}
New-Item -ItemType Directory -Path $audioPackageAttempt | Out-Null
$audioPackageProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$audioPackageArgs = @(
    'BuildCookRun', "-project=$audioPackageProject", '-target=EmberTrial', '-platform=Win64',
    '-clientconfig=Development', '-unrealexe=UnrealEditor-Cmd.exe', '-installed',
    '-nop4', '-unattended', '-utf8output', '-nocompileeditor',
    '-build', '-cook', '-stage', '-pak', '-iostore', '-compressed',
    '-package', '-prereqs', '-nodebuginfo', '-archive', "-archivedirectory=$audioPackageAttempt"
)
$audioPackageUatLog = Join-Path $audioPackageAttempt 'UAT_Console.log'
& 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/RunUAT.bat' @audioPackageArgs | Tee-Object -FilePath $audioPackageUatLog
if ($LASTEXITCODE -ne 0) {
    throw "UAT 失败，保留 $audioPackageUatLog 并反馈第一条错误。"
}
foreach ($audioPackageOutput in @(
    'Windows/EmberTrial.exe',
    'Windows/EmberTrial/Binaries/Win64/EmberTrial.exe',
    'Windows/EmberTrial/Content/Paks/EmberTrial-Windows.utoc'
)) {
    if (!(Test-Path -LiteralPath (Join-Path $audioPackageAttempt $audioPackageOutput) -PathType Leaf)) {
        throw "归档缺少 $audioPackageOutput。"
    }
}
```

预期显示 `BUILD SUCCESSFUL`。保留完整 Windows 目录和 UAT 日志。

## 2. 启动新包并采集

使用 M6-04 的同一副扬声器和 A 点存档，保持接电、1920×1080 窗口与中档。将下方整段复制进 PowerShell。本次直接读取打包配置，所以启动参数中应无 `AudioCallbackBufferFrameSize` 覆盖。

```powershell
Set-Location 'E:/UE_Project/Demo'
$audioPackageAttempt = 'E:/UE_Project/Demo/Artifacts/Builds/M6_06/Attempt01'
$audioPackageSeed = 'E:/UE_Project/Demo/Artifacts/M6_02/Runtime/Attempt01/User/After01_20261114_215142_739/Saved/SaveGames/EmberTrial_Run_A.sav'
& './Tools/Run-StageBuild.ps1' -BuildDirectory $audioPackageAttempt -EvidenceStage M6_06 `
    -RunName Package512 -Width 1920 -Height 1080 -Trace -PerformanceBaseline `
    -SeedCheckpointSave $audioPackageSeed
```

日志、JSON、Trace 和独立用户目录在 `Artifacts/M6_06/Runtime`。首屏点 **继续本次试炼**，确认 A 点、第一场已清、双 100 与治疗 3/3。打开游戏控制台，查询 `sg.GlobalIlluminationQuality`（1）、`r.SkyLight.RealTimeReflectionCapture`（0）和 `Trace.Status`（LoadTime、File 已启用）。如方便，在运行日志中搜 `Callback Buffer Frame Size To Use` 与 `Output buffers initialized`，两处应为 512。

## 3. 三轮 Continue 与两次试听

1. 在本进程首次到 A 点后，依次执行 `et.AudioProbe music` 和 `et.AudioProbe effects`，等第一段播完再播放第二段。记录两路有无声音、爆音、断续或掉音。等音效播完后站定约 15 秒，再开始计时。
2. 每轮在 A 点输入对应 Begin 并回车，收起控制台，按 **P → 返回主菜单**，待菜单就绪约 5 秒后点 **继续本次试炼**。回到 A 点再等约 5 秒，输入对应 End。确认本轮 End 后再开始下一轮。

   | 轮次 | Begin | End |
   | --- | --- | --- |
   | 1 | `Trace.RegionBegin M6_06_Continue01` | `Trace.RegionEnd M6_06_Continue01` |
   | 2 | `Trace.RegionBegin M6_06_Continue02` | `Trace.RegionEnd M6_06_Continue02` |
   | 3 | `Trace.RegionBegin M6_06_Continue03` | `Trace.RegionEnd M6_06_Continue03` |

3. 第三轮 End 后输入 `Trace.Stop`。留在 A 点再执行一次两路探针，按同样方式听完，确认经过切图仍有声。随后进入第二场做约 30 秒短查，观察敌人实际追击或命中，按一次左键或 2，留意交战时是否有爆音、断续或掉音。
4. 从 P 菜单返回主菜单并退出游戏。保留最新一组 `Package512_*`。如误操作导致某个 Region 没闭合，反馈实际采集轮数。

## 4. 验收与回传

正式包通过条件为 Mixer 实际采用 512、两次两路探针均有声、切图和交战听感正常、三轮 A 点恢复及退出正常。性能复验与 M6-04 的 1024 帧组比较，目标为切图帧中位数低于 77.33 ms，并接近 512 候选的 62.66 ms。我会同时检查最慢一轮、后一帧和音频 Flush。若出现可复现的音频劣化或 Mixer 未采用 512，保留该包证据，并把项目配置恢复 1024 后反馈。单轮尖峰用 Trace 定位后再判断是否需要补采。

```text
M6-06 完成
DefaultEngine.ini 512 / Windows Development Build-Cook-Stage-Package：
实际 Attempt / exe / UAT 日志（沿用可填沿用）：
实际缓冲两条日志（没查可填由日志核对）：
首屏 A 点 / 中档 1920×1080 / 同一设备与接电：
首次 A 点试听（音乐 / 音效实际听感）：
三轮 A Continue / 三组 Begin-End / Trace.Stop：
第三轮后试听（音乐 / 音效实际听感）：
第二场实际交战与听感 / 主菜单正常退出：
Runtime 目录（沿用可填沿用）/ 失败或明显问题（无填无）：
完成时分 / 手动分钟 / Build-Cook 等待分钟：
```

## 5. Commit / Push

在 CLion 的 Commit 窗口暂存以下文件。本地 `Artifacts` 保留构建、日志、Trace、峰值和存档。

- `EmberTrial/Config/DefaultEngine.ini`、`Tools/Run-StageBuild.ps1`
- `Docs/Progress.md`、`Docs/Backlog.md`、`Introduction.md`
- M6-04、M6-05、M6-06 三份操作指南与 `Docs/Reports` 下对应三份验收报告

- 英文 `perf(audio): use a 512-frame Windows mixer callback buffer`
- 中文 `perf(audio): 将 Windows 混音器回调缓冲调整为 512 帧`
