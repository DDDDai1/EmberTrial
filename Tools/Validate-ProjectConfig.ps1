[CmdletBinding()]
param([string]$ProjectDirectory = '')

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectDirectory)) {
    $ProjectDirectory = Join-Path (Split-Path $PSScriptRoot -Parent) 'EmberTrial'
}
$configProject = [IO.Path]::GetFullPath($ProjectDirectory)
$configIssues = [System.Collections.Generic.List[string]]::new()
$configWarnings = [System.Collections.Generic.List[string]]::new()

function Get-SectionLines([string]$Path, [string]$Section) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Config file missing: $Path" }
    $selectedSection = ''
    foreach ($configLine in Get-Content -LiteralPath $Path) {
        $configTrimmed = $configLine.Trim()
        if ($configTrimmed -match '^\[(.+)\]$') { $selectedSection = $Matches[1]; continue }
        if ($selectedSection -eq $Section -and $configTrimmed -and !$configTrimmed.StartsWith(';')) {
            $configTrimmed
        }
    }
}

function Get-Scalar([string[]]$Lines, [string]$Key) {
    $configValue = ''
    foreach ($configLine in $Lines) {
        if ($configLine -match ('^' + [regex]::Escape($Key) + '=(.*)$')) { $configValue = $Matches[1].Trim().Trim('"') }
    }
    $configValue
}

function Get-Array([string[]]$Lines, [string]$Key) {
    $configValues = [System.Collections.Generic.List[string]]::new()
    foreach ($configLine in $Lines) {
        if ($configLine -match ('^([+.!-]?)' + [regex]::Escape($Key) + '=(.*)$')) {
            $configOperator = $Matches[1]
            $configElement = $Matches[2].Trim()
            switch ($configOperator) {
                '!' { $configValues.Clear() }
                '-' { [void]$configValues.Remove($configElement) }
                '+' { if (!$configValues.Contains($configElement)) { $configValues.Add($configElement) } }
                '.' { $configValues.Add($configElement) }
                default { $configValues.Clear(); $configValues.Add($configElement) }
            }
        }
    }
    $configValues.ToArray()
}

$configEngineFile = Join-Path $configProject 'Config\DefaultEngine.ini'
$configGameFile = Join-Path $configProject 'Config\DefaultGame.ini'
$configMapLines = @(Get-SectionLines $configEngineFile '/Script/EngineSettings.GameMapsSettings')
$configPackageLines = @(Get-SectionLines $configGameFile '/Script/UnrealEd.ProjectPackagingSettings')
$configCueLines = @(Get-SectionLines $configGameFile '/Script/GameplayAbilities.AbilitySystemGlobals')
$configMenu = '/Game/EmberTrial/Maps/L_ET_MainMenu'
$configTrial = '/Game/EmberTrial/Maps/L_ET_VerticalSlice'

foreach ($configMapRule in @(
    @{Key='GameDefaultMap'; Package=$configMenu},
    @{Key='EditorStartupMap'; Package=$configTrial}
)) {
    $configActual = Get-Scalar $configMapLines $configMapRule.Key
    $configExpected = $configMapRule.Package + '.' + ($configMapRule.Package.Split('/')[-1])
    if ($configActual -ne $configExpected -and $configActual -ne $configMapRule.Package) {
        $configIssues.Add("[ET-CONFIG-001] $configEngineFile : $($configMapRule.Key) must be $configExpected; actual=$configActual")
    }
}
$configCookMaps = @(Get-Array $configPackageLines 'MapsToCook' | ForEach-Object {
    if ($_ -match '^\(FilePath="([^"]+)"\)$') { $Matches[1] }
})
foreach ($configPackage in @($configMenu, $configTrial)) {
    if ($configCookMaps -notcontains $configPackage) {
        $configIssues.Add("[ET-CONFIG-002] $configGameFile : MapsToCook missing $configPackage")
    }
    $configRelative = $configPackage.Substring('/Game/'.Length).Replace('/', [IO.Path]::DirectorySeparatorChar)
    $configMapFile = Join-Path $configProject ('Content\' + $configRelative + '.umap')
    if (!(Test-Path -LiteralPath $configMapFile -PathType Leaf)) {
        $configIssues.Add("[ET-CONFIG-003] $configMapFile : required map file missing")
    }
}
$configCuePaths = @(Get-Array $configCueLines 'GameplayCueNotifyPaths' | ForEach-Object { $_.Trim('"') })
if ($configCuePaths.Count -eq 0) {
    $configWarnings.Add("[ET-CONFIG-004] $configGameFile : GameplayCueNotifyPaths absent; C2 should add /Game/EmberTrial/Abilities")
} else {
    foreach ($configCuePath in $configCuePaths) {
        if ($configCuePath -notmatch '^/Game/EmberTrial/Abilities(?:/[A-Za-z0-9_/]+)?$') {
            $configIssues.Add("[ET-CONFIG-004] $configGameFile : cue scan path outside current ability scope: $configCuePath")
            continue
        }
        $configCueDirectory = Join-Path $configProject ('Content\' + $configCuePath.Substring('/Game/'.Length).Replace('/', [IO.Path]::DirectorySeparatorChar))
        if (!(Test-Path -LiteralPath $configCueDirectory -PathType Container)) {
            $configIssues.Add("[ET-CONFIG-004] $configCueDirectory : cue scan directory missing")
        }
    }
}
foreach ($configMessage in $configWarnings) { Write-Warning $configMessage }
foreach ($configMessage in $configIssues) { Write-Host $configMessage -ForegroundColor Red }
if ($configIssues.Count -gt 0) {
    Write-Host "[M1-07C2] Config FAILED Errors=$($configIssues.Count) Warnings=$($configWarnings.Count)"
    exit 1
}
Write-Host "[M1-07C2] Config PASS Errors=0 Warnings=$($configWarnings.Count) Project=$configProject"
exit 0
