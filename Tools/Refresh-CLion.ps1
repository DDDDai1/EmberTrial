[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE5\UE_5.5',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$projectRoot = Join-Path $repoRoot 'EmberTrial'
$projectFile = Join-Path $projectRoot 'EmberTrial.uproject'
$buildScript = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$engineSource = Join-Path $EngineRoot 'Engine\Source'
$editorBuildRoot = Join-Path $projectRoot 'Intermediate\Build\Win64\x64\UnrealEditor\Development'

if (!(Test-Path -LiteralPath $buildScript)) {
    throw "UE Build.bat not found: $buildScript"
}

if (!$SkipBuild) {
    if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
        throw 'Save and close Unreal Editor before refreshing the CLion build metadata.'
    }
    & $buildScript EmberTrialEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) {
        throw "UBT failed with exit code $LASTEXITCODE"
    }
}

function Quote-CMake([string]$Value) {
    '"' + $Value.Replace('\', '/').Replace('"', '\"').Replace(';', '\;') + '"'
}

$projectDescriptor = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
$moduleNames = @($projectDescriptor.Modules | ForEach-Object { $_.Name } | Select-Object -Unique)
if ($moduleNames.Count -eq 0) {
    throw 'No project modules found in the .uproject file.'
}

$moduleBlocks = [System.Collections.Generic.List[string]]::new()
foreach ($moduleName in $moduleNames) {
    if ($moduleName -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
        throw "Unsupported module name: $moduleName"
    }
    $moduleRules = Join-Path $projectRoot "Source\$moduleName\$moduleName.Build.cs"
    $moduleBuild = Join-Path $editorBuildRoot $moduleName
    $sharedResponse = Join-Path $moduleBuild "$moduleName.Shared.rsp"
    $definitions = Join-Path $moduleBuild "Definitions.$moduleName.h"

    if (!(Test-Path -LiteralPath $moduleRules)) {
        throw "Module rules file is missing: $moduleRules"
    }
    if (!(Test-Path -LiteralPath $sharedResponse) -or !(Test-Path -LiteralPath $definitions)) {
        throw "Build metadata is missing for $moduleName. Run this script without -SkipBuild first."
    }

    $includePaths = [System.Collections.Generic.List[string]]::new()
    $compileDefinitions = [System.Collections.Generic.List[string]]::new()
    foreach ($line in Get-Content -LiteralPath $sharedResponse) {
        if ($line -match '^/(?:external:)?I\s*"([^"]+)"') {
            $includePath = $Matches[1]
            if (![IO.Path]::IsPathRooted($includePath)) {
                $includePath = Join-Path $engineSource $includePath
            }
            $includePaths.Add([IO.Path]::GetFullPath($includePath))
        } elseif ($line -match '^/D(.+)$') {
            $compileDefinitions.Add($Matches[1].Trim('"'))
        }
    }
    if ($includePaths.Count -eq 0) {
        throw "No include paths parsed from $sharedResponse"
    }
    $includeLines = ($includePaths | Sort-Object -Unique | ForEach-Object { '    ' + (Quote-CMake $_) }) -join "`n"
    $defineLines = ($compileDefinitions | Sort-Object -Unique | ForEach-Object { '    ' + (Quote-CMake $_) }) -join "`n"
    $forceInclude = Quote-CMake ('/FI' + $definitions)

    $moduleTemplate = @'
file(GLOB_RECURSE @MODULE@_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/@MODULE@/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/@MODULE@/*.h")
add_library(@MODULE@CodeModel OBJECT EXCLUDE_FROM_ALL ${@MODULE@_SOURCES})
target_include_directories(@MODULE@CodeModel PRIVATE
@INCLUDES@
)
target_compile_definitions(@MODULE@CodeModel PRIVATE
@DEFINITIONS@
)
target_compile_options(@MODULE@CodeModel PRIVATE @FORCE_INCLUDE@ /Zc:__cplusplus /Zc:preprocessor /utf-8)
'@
    $block = $moduleTemplate.Replace('@MODULE@', $moduleName).Replace('@INCLUDES@', $includeLines).Replace('@DEFINITIONS@', $defineLines).Replace('@FORCE_INCLUDE@', $forceInclude)
    $moduleBlocks.Add($block)
    Write-Host "Index metadata: $moduleName; includes: $($includePaths.Count)"
}

$header = @'
# Local CLion integration. Recreate with Tools/Refresh-CLion.ps1.
# All real UE builds go through UBT, including UnrealHeaderTool.
cmake_minimum_required(VERSION 3.20)
project(EmberTrial LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
'@

$footerTemplate = @'
add_custom_target(EmberTrialEditor ALL
    COMMAND @BUILD@ EmberTrialEditor Win64 Development @PROJECT@ -WaitMutex -NoHotReloadFromIDE
    USES_TERMINAL VERBATIM)
add_custom_target(EmberTrialEditor_DebugGame
    COMMAND @BUILD@ EmberTrialEditor Win64 DebugGame @PROJECT@ -WaitMutex -NoHotReloadFromIDE
    USES_TERMINAL VERBATIM)
# Refresh only the code model after a normal UBT build; avoid recursive builds.
add_custom_target(Refresh_CLion_Model
    COMMAND powershell.exe -NoProfile -File "${CMAKE_CURRENT_SOURCE_DIR}/../Tools/Refresh-CLion.ps1" -EngineRoot @ENGINE@ -SkipBuild
    USES_TERMINAL VERBATIM)
'@
$footer = $footerTemplate.Replace('@BUILD@', (Quote-CMake $buildScript)).Replace('@PROJECT@', (Quote-CMake ('-Project=' + $projectFile))).Replace('@ENGINE@', (Quote-CMake $EngineRoot))
$cmake = $header + "`n" + ($moduleBlocks -join "`n") + "`n" + $footer + "`n"
$cmakePath = Join-Path $projectRoot 'CMakeLists.txt'
[IO.File]::WriteAllText($cmakePath, $cmake, [Text.UTF8Encoding]::new($false))
Write-Host "CLion model refreshed: $cmakePath"
Write-Host "Modules: $($moduleNames -join ', '); real build target: EmberTrialEditor (Win64 Development)."