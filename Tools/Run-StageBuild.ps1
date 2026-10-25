[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$BuildDirectory = '',
    [ValidatePattern('^[A-Za-z0-9_-]{1,48}$')]
    [string]$RunName = 'Run01',
    [ValidateRange(640, 7680)]
    [int]$Width = 1280,
    [ValidateRange(480, 4320)]
    [int]$Height = 720,
    [ValidateSet('M1_07A', 'M1_07C3', 'Cleanup', 'M2_07A')]
    [string]$EvidenceStage = 'M1_07A',
    [switch]$Trace,
    [switch]$PerformanceBaseline
)

$ErrorActionPreference = 'Stop'
## 保留M1默认包；Cleanup / M2新包显式指定目录与阶段，避免混入历史证据。
$stageRepo = Split-Path $PSScriptRoot -Parent
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $stageRepo 'Artifacts\Builds\M1_07A\Attempt01'
}
$stageBuild = (Resolve-Path -LiteralPath $BuildDirectory).Path
$stageExe = Join-Path $stageBuild 'Windows\EmberTrial.exe'
if (!(Test-Path -LiteralPath $stageExe -PathType Leaf)) {
    throw "Expected packaged executable was not found: $stageExe. Pass the folder containing Windows as -BuildDirectory."
}
if (!$WhatIfPreference -and (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue)) {
    throw 'Save and close Unreal Editor before checking the standalone package.'
}
## 日志 / Trace / 元数据按同一唯一前缀关联；记录不是Gameplay验收结果。
$stageEvidenceStage = ## 固定性能采集CVar与顺序属于可复现基线，本批不改采样口径。
if ($PerformanceBaseline) { 'M1_07B' } else { $EvidenceStage }
$stageOutput = Join-Path $stageRepo ("Artifacts\$stageEvidenceStage\Runtime")
$stageStamp = Get-Date -Format 'yyyyMMdd_HHmmss_fff'
$stageBase = "${RunName}_${stageStamp}"
$stageLog = Join-Path $stageOutput ($stageBase + '.log')
$stageMeta = Join-Path $stageOutput ($stageBase + '.json')
$stageTrace = Join-Path $stageOutput ($stageBase + '.utrace')
$stageArguments = @('-windowed', "-ResX=$Width", "-ResY=$Height", '-log', ('"-abslog=' + $stageLog + '"'))
$stageProfileCommands = $null
if ($PerformanceBaseline) {
    $stageProfileCommands = 't.MaxFPS 0,r.VSync 0,sg.ViewDistanceQuality 2,sg.AntiAliasingQuality 2,sg.ShadowQuality 2,sg.GlobalIlluminationQuality 2,sg.ReflectionQuality 2,sg.PostProcessQuality 2,sg.TextureQuality 2,sg.EffectsQuality 2,sg.FoliageQuality 2,sg.ShadingQuality 2,r.DynamicRes.OperationMode 0,r.ScreenPercentage 100'
    $stageArguments += ('-ExecCmds="' + $stageProfileCommands + '"')
}
if ($Trace) {
    $stageArguments += @('-trace=cpu,frame,gpu,bookmark,region,loadtime,log', '-statnamedevents', ('"-tracefile=' + $stageTrace + '"'))
}
Write-Host "Executable: $stageExe"
Write-Host "Log: $stageLog"
if ($PerformanceBaseline) {
    Write-Host "Runtime profiling commands: $stageProfileCommands"
}
if ($Trace) {
    Write-Host "Trace: $stageTrace"
}
## WhatIf必须在任何目录 / 证据写入和进程启动之前退出。
if (!$PSCmdlet.ShouldProcess($stageExe, 'Start packaged EmberTrial with a distinct runtime log')) {
    return
}
New-Item -ItemType Directory -Path $stageOutput -Force | Out-Null
if ((Test-Path -LiteralPath $stageLog) -or (Test-Path -LiteralPath $stageMeta)) {
    throw 'The generated evidence filename already exists; run again to get a fresh timestamp.'
}
## 这是供用户操作的游戏窗口；不隐藏它，也不把启动成功记成玩法通过。
$stageProcess = Start-Process -FilePath $stageExe -WorkingDirectory (Split-Path $stageExe -Parent) -ArgumentList $stageArguments -PassThru
[ordered]@{
    RunName = $RunName
    StartedAt = [DateTimeOffset]::Now.ToString('o')
    Executable = $stageExe
    ExecutableSHA256 = (Get-FileHash -LiteralPath $stageExe -Algorithm SHA256).Hash
    BuildDirectory = $stageBuild
    Resolution = "$Width x $Height"
    TraceEnabled = [bool]$Trace
    EvidenceStage = $stageEvidenceStage
    PerformanceBaseline = [bool]$PerformanceBaseline
    RuntimeProfileCommands = $stageProfileCommands
    ProcessId = $stageProcess.Id
    LogPath = $stageLog
    TracePath = $(if ($Trace) { $stageTrace } else { $null })
    Arguments = $stageArguments
    Purpose = 'Launch and evidence locations only; gameplay acceptance remains a separate observation.'
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $stageMeta -Encoding UTF8
Write-Host "Metadata: $stageMeta"
Write-Host 'Use the game menu to quit. Keep the complete Windows package folder together.'
