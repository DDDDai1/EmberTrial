[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$BuildDirectory = '',
    [ValidatePattern('^[A-Za-z0-9_-]{1,48}$')]
    [string]$RunName = 'Run01',
    [ValidateRange(640, 7680)]
    [int]$Width = 1280,
    [ValidateRange(480, 4320)]
    [int]$Height = 720,
    [ValidateSet('M1_07A', 'M1_07C3', 'Cleanup', 'M2_07A', 'M2_07B', 'M3_06A', 'M3_06C3', 'M4_06', 'M5_04', 'M6_01', 'M6_02', 'M6_03', 'M6_04', 'M6_06', 'M7_07')]
    [string]$EvidenceStage = 'M1_07A',
    [switch]$Trace,
    [switch]$PerformanceBaseline,
    [switch]$NoSound,
    [string]$SeedCheckpointSave = '',
    [ValidateSet(512, 1024)]
    [int]$AudioCallbackFrames
)

$ErrorActionPreference = 'Stop'
## 阶段集成包使用显式目录和独立的个人设置文件。
$stageRepo = Split-Path $PSScriptRoot -Parent
$stageUsesIsolatedRun = $EvidenceStage -in @('M6_01', 'M6_02', 'M6_03', 'M6_04', 'M6_06')
$stageShipping = $EvidenceStage -eq 'M7_07'
$stageSharedUserProfile = $EvidenceStage -in @('M5_04', 'M7_07')
$stageUsesSeparateSettings = $stageUsesIsolatedRun -or $stageSharedUserProfile -or $EvidenceStage -in @('M3_06A', 'M3_06C3', 'M4_06')
$stageHasAudioCallbackOverride = $PSBoundParameters.ContainsKey('AudioCallbackFrames')
if ($stageHasAudioCallbackOverride -ne ($EvidenceStage -eq 'M6_04')) {
    throw 'Specify -AudioCallbackFrames 512 or 1024 for EvidenceStage M6_04 only.'
}
if ($NoSound -and $EvidenceStage -ne 'M6_03') {
    throw '-NoSound is only supported in EvidenceStage M6_03.'
}
$stageSeedSavePath = $null
if ($SeedCheckpointSave) {
    if ($EvidenceStage -notin @('M6_03', 'M6_04', 'M6_06')) {
        throw '-SeedCheckpointSave requires EvidenceStage M6_03, M6_04 or M6_06.'
    }
    $stageSeedSavePath = (Resolve-Path -LiteralPath $SeedCheckpointSave).Path
    if ((Split-Path $stageSeedSavePath -Leaf) -cne 'EmberTrial_Run_A.sav' -or
        !(Test-Path -LiteralPath $stageSeedSavePath -PathType Leaf)) {
        throw 'Pass a saved EmberTrial_Run_A.sav file as -SeedCheckpointSave.'
    }
}
if ($stageUsesSeparateSettings -and [string]::IsNullOrWhiteSpace($BuildDirectory)) {
    throw "$EvidenceStage requires an explicit -BuildDirectory for the new package."
}
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $stageRepo 'Artifacts\Builds\M1_07A\Attempt01'
}
$stageBuild = (Resolve-Path -LiteralPath $BuildDirectory).Path
if ($stageUsesIsolatedRun -or $stageSharedUserProfile -or $EvidenceStage -in @('M3_06C3', 'M4_06')) {
    $stagePackageStages = switch ($EvidenceStage) {
        'M6_01' { 'M5_04' }
        'M6_02' { 'M5_04'; 'M6_02' }
        'M6_03' { 'M6_02' }
        'M6_04' { 'M6_02' }
        default { $EvidenceStage }
    }
    $stageExpectedRoots = $stagePackageStages | ForEach-Object {
        Join-Path $stageRepo "Artifacts\Builds\$_"
    }
    if ((Split-Path $stageBuild -Parent) -notin $stageExpectedRoots -or
        (Split-Path $stageBuild -Leaf) -notmatch '^Attempt[0-9]+$') {
        throw "$EvidenceStage requires an Attempt directory under $($stageExpectedRoots -join ' or ')."
    }
}
$stageExe = Join-Path $stageBuild 'Windows\EmberTrial.exe'
if (!(Test-Path -LiteralPath $stageExe -PathType Leaf)) {
    throw "Expected packaged executable was not found: $stageExe. Pass the folder containing Windows as -BuildDirectory."
}
if (!$WhatIfPreference -and (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue)) {
    throw 'Save and close Unreal Editor before checking the standalone package.'
}
if ($stageShipping -and !$WhatIfPreference -and
    (Get-Process -Name 'EmberTrial-Win64-Shipping' -ErrorAction SilentlyContinue)) {
    throw 'Quit the running EmberTrial Shipping instance before reusing the QA save profile.'
}
## 日志 / Trace / 元数据按同一唯一前缀关联；记录不是Gameplay验收结果。
## 历史性能命令仍归档到M1；显式M2_07B使用自己的证据目录。
$stageEvidenceStage =
if ($PerformanceBaseline -and !$stageUsesIsolatedRun -and $EvidenceStage -notin @('M2_07B', 'M4_06', 'M5_04')) { 'M1_07B' } else { $EvidenceStage }
$stageOutput = Join-Path $stageRepo ("Artifacts\$stageEvidenceStage\Runtime")
$stageStamp = Get-Date -Format 'yyyyMMdd_HHmmss_fff'
$stageBase = "${RunName}_${stageStamp}"
$stageLog = if (!$stageShipping) { Join-Path $stageOutput ($stageBase + '.log') }
$stageMeta = Join-Path $stageOutput ($stageBase + '.json')
$stageTrace = Join-Path $stageOutput ($stageBase + '.utrace')
$stageArguments = @('-windowed', "-ResX=$Width", "-ResY=$Height")
if ($stageLog) {
    $stageArguments += @('-log', ('"-abslog=' + $stageLog + '"'))
}
if ($NoSound) {
    $stageArguments += '-nosound'
}
if ($stageHasAudioCallbackOverride) {
    # 默认缓冲和小缓冲都通过相同的启动覆盖传入，实际采用值从 Mixer 日志核对。
    $stageArguments += "-ini:Engine:[/Script/WindowsTargetPlatform.WindowsTargetSettings]:AudioCallbackBufferFrameSize=$AudioCallbackFrames"
}
$stageSettingsIni = $null
$stageUserDir = $null
if ($stageUsesSeparateSettings) {
    ## 集成验收可跨进程复验保存，性能测试则为每次运行分配独立设置。
    $stageSettingsName = switch ($EvidenceStage) {
        'M4_06' { 'M4PackageUser.ini' }
        'M5_04' { 'M5PackageUser.ini' }
        'M6_01' { 'M6PackageUser.ini' }
        'M6_02' { $stageBase + '_GameUserSettings.ini' }
        'M6_03' { $stageBase + '_GameUserSettings.ini' }
        'M6_04' { $stageBase + '_GameUserSettings.ini' }
        'M6_06' { $stageBase + '_GameUserSettings.ini' }
        'M7_07' { (Split-Path $stageBuild -Leaf) + '_GameUserSettings.ini' }
        default { 'M3PackageUser.ini' }
    }
    $stageSettingsIni = Join-Path $stageOutput $stageSettingsName
    $stageArguments += ('"-GameUserSettingsINI=' + $stageSettingsIni + '"')
}
if ($stageUsesIsolatedRun -or $stageSharedUserProfile) {
    $stageUserDir = Join-Path $stageOutput (Join-Path (Split-Path $stageBuild -Leaf) 'User')
    if ($stageUsesIsolatedRun) {
        $stageUserDir = Join-Path $stageUserDir $stageBase
    }
    $stageArguments += ('"-UserDir=' + $stageUserDir + '"')
}
$stageProfileCommands = $null
if ($PerformanceBaseline) {
    ## M2 与 M6 使用中等画质口径，历史 M1 继续使用高画质。
    $stageQualityLevel = if ($stageUsesIsolatedRun -or $EvidenceStage -eq 'M2_07B') { 1 } else { 2 }
    $stageProfileCommands = ('t.MaxFPS 0,r.VSync 0,sg.ViewDistanceQuality {0},sg.AntiAliasingQuality {0},sg.ShadowQuality {0},sg.GlobalIlluminationQuality {0},sg.ReflectionQuality {0},sg.PostProcessQuality {0},sg.TextureQuality {0},sg.EffectsQuality {0},sg.FoliageQuality {0},sg.ShadingQuality {0},r.DynamicRes.OperationMode 0,r.ScreenPercentage 100' -f $stageQualityLevel)
    $stageArguments += ('-ExecCmds="' + $stageProfileCommands + '"')
}
if ($Trace) {
    $stageTraceChannels = 'cpu,frame,gpu,bookmark,region,loadtime,log'
    if ($EvidenceStage -in @('M6_04', 'M6_06')) {
        # 保留同一组 Trace 通道，便于 M6 切图样本对照。
        $stageTraceChannels += ',file'
    }
    $stageArguments += @("-trace=$stageTraceChannels", '-statnamedevents', ('"-tracefile=' + $stageTrace + '"'))
}
Write-Host "Executable: $stageExe"
if ($stageLog) {
    Write-Host "Log: $stageLog"
}
if ($stageUserDir) {
    Write-Host "User directory: $stageUserDir"
}
if ($stageHasAudioCallbackOverride) {
    Write-Host "Requested audio callback frames: $AudioCallbackFrames"
}
if ($PerformanceBaseline) {
    Write-Host "Runtime profiling commands: $stageProfileCommands"
}
if ($Trace) {
    Write-Host "Trace: $stageTrace"
}
## WhatIf必须在任何目录 / 证据写入和进程启动之前退出。
if (!$PSCmdlet.ShouldProcess($stageExe, 'Start packaged EmberTrial and record launch metadata')) {
    return
}
New-Item -ItemType Directory -Path $stageOutput -Force | Out-Null
if (($stageLog -and (Test-Path -LiteralPath $stageLog)) -or (Test-Path -LiteralPath $stageMeta) -or
    ($stageSeedSavePath -and (Test-Path -LiteralPath $stageUserDir))) {
    throw 'The generated evidence filename already exists; run again to get a fresh timestamp.'
}
$stageGameExeHash = $null
$stageGameContentTocHash = $null
$stageGameExe = $null
if ($stageUsesIsolatedRun -or $stageShipping) {
    $stageBinaryName = if ($stageShipping) { 'EmberTrial-Win64-Shipping.exe' } else { 'EmberTrial.exe' }
    $stageGameExe = Join-Path $stageBuild "Windows\EmberTrial\Binaries\Win64\$stageBinaryName"
    $stageGameExeHash = (Get-FileHash -LiteralPath $stageGameExe -Algorithm SHA256).Hash
}
if ($EvidenceStage -in @('M6_02', 'M6_03', 'M6_04', 'M6_06', 'M7_07')) {
    # 地图修改仅影响 Cook 内容，运行二进制可能完全相同。
    $stageContentToc = Join-Path $stageBuild 'Windows\EmberTrial\Content\Paks\EmberTrial-Windows.utoc'
    $stageGameContentTocHash = (Get-FileHash -LiteralPath $stageContentToc -Algorithm SHA256).Hash
}
$stageSeedSaveHash = $null
if ($stageSeedSavePath) {
    # 两组从同一份已验收快照出发，游戏只读写本轮用户目录。
    $stageSaveDir = Join-Path $stageUserDir 'Saved\SaveGames'
    New-Item -ItemType Directory -Path $stageSaveDir -Force | Out-Null
    $stageSeedCopy = Join-Path $stageSaveDir 'EmberTrial_Run_A.sav'
    Copy-Item -LiteralPath $stageSeedSavePath -Destination $stageSeedCopy
    $stageSeedSaveHash = (Get-FileHash -LiteralPath $stageSeedCopy -Algorithm SHA256).Hash
}
## 保留可见窗口供采集者实际游玩。
$stageProcess = Start-Process -FilePath $stageExe -WorkingDirectory (Split-Path $stageExe -Parent) -ArgumentList $stageArguments -PassThru
[ordered]@{
    RunName = $RunName
    StartedAt = [DateTimeOffset]::Now.ToString('o')
    Executable = $stageExe
    ExecutableSHA256 = (Get-FileHash -LiteralPath $stageExe -Algorithm SHA256).Hash
    GameExecutable = $stageGameExe
    GameExecutableSHA256 = $stageGameExeHash
    GameContentTocSHA256 = $stageGameContentTocHash
    SeedCheckpointSaveSHA256 = $stageSeedSaveHash
    BuildDirectory = $stageBuild
    Resolution = "$Width x $Height"
    TraceEnabled = [bool]$Trace
    EvidenceStage = $stageEvidenceStage
    PerformanceBaseline = [bool]$PerformanceBaseline
    NoSound = [bool]$NoSound
    RequestedAudioCallbackFrames = $(if ($stageHasAudioCallbackOverride) { $AudioCallbackFrames } else { $null })
    RuntimeProfileCommands = $stageProfileCommands
    ProcessId = $stageProcess.Id
    LogPath = $stageLog
    TracePath = $(if ($Trace) { $stageTrace } else { $null })
    Arguments = $stageArguments
    UserSettingsIni = $stageSettingsIni
    UserDirectory = $stageUserDir
    Purpose = 'Launch and evidence locations only; gameplay acceptance remains a separate observation.'
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $stageMeta -Encoding UTF8
Write-Host "Metadata: $stageMeta"
Write-Host 'Use the game menu to quit. Keep the complete Windows package folder together.'
