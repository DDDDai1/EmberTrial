[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$BuildDirectory = '',
    [ValidatePattern('^[A-Za-z0-9_-]{1,48}$')]
    [string]$RunName = 'Run01',
    [ValidateRange(640, 7680)]
    [int]$Width = 1280,
    [ValidateRange(480, 4320)]
    [int]$Height = 720,
    [switch]$Trace
)

$ErrorActionPreference = 'Stop'
$stageRepo = Split-Path $PSScriptRoot -Parent
if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $BuildDirectory = Join-Path $stageRepo 'Artifacts\Builds\M1_07A\Attempt01'
}
$stageBuild = (Resolve-Path -LiteralPath $BuildDirectory).Path
$stageExe = Join-Path $stageBuild 'Windows\EmberTrial.exe'
if (!(Test-Path -LiteralPath $stageExe -PathType Leaf)) {
    throw "Expected packaged executable was not found: $stageExe. Pass the folder containing Windows as -BuildDirectory."
}
if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
    throw 'Save and close Unreal Editor before checking the standalone package.'
}
$stageOutput = Join-Path $stageRepo 'Artifacts\M1_07A\Runtime'
$stageStamp = Get-Date -Format 'yyyyMMdd_HHmmss_fff'
$stageBase = "${RunName}_${stageStamp}"
$stageLog = Join-Path $stageOutput ($stageBase + '.log')
$stageMeta = Join-Path $stageOutput ($stageBase + '.json')
$stageTrace = Join-Path $stageOutput ($stageBase + '.utrace')
$stageArguments = @('-windowed', "-ResX=$Width", "-ResY=$Height", '-log', ('"-abslog=' + $stageLog + '"'))
if ($Trace) {
    $stageArguments += @('-trace=cpu,frame,gpu,bookmark,loadtime', '-statnamedevents', ('"-tracefile=' + $stageTrace + '"'))
}
Write-Host "Executable: $stageExe"
Write-Host "Log: $stageLog"
if ($Trace) { Write-Host "Trace: $stageTrace" }
if (!$PSCmdlet.ShouldProcess($stageExe, 'Start packaged EmberTrial with a distinct runtime log')) {
    return
}
New-Item -ItemType Directory -Path $stageOutput -Force | Out-Null
if ((Test-Path -LiteralPath $stageLog) -or (Test-Path -LiteralPath $stageMeta)) {
    throw 'The generated evidence filename already exists; run again to get a fresh timestamp.'
}
$stageProcess = Start-Process -FilePath $stageExe -WorkingDirectory (Split-Path $stageExe -Parent) -ArgumentList $stageArguments -PassThru
[ordered]@{
    RunName = $RunName
    StartedAt = [DateTimeOffset]::Now.ToString('o')
    Executable = $stageExe
    ExecutableSHA256 = (Get-FileHash -LiteralPath $stageExe -Algorithm SHA256).Hash
    BuildDirectory = $stageBuild
    Resolution = "$Width x $Height"
    TraceEnabled = [bool]$Trace
    ProcessId = $stageProcess.Id
    LogPath = $stageLog
    TracePath = $(if ($Trace) { $stageTrace } else { $null })
    Arguments = $stageArguments
    Purpose = 'Launch and evidence locations only; gameplay acceptance remains a separate observation.'
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $stageMeta -Encoding UTF8
Write-Host "Metadata: $stageMeta"
Write-Host 'Use the game menu to quit. Keep the complete Windows package folder together.'
