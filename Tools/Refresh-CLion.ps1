[CmdletBinding()]
param(
    [string]$EngineRoot = 'D:\UE5\UE_5.5',
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
## 路径与正常UBT构建：SkipBuild仅复用已存在元数据，不替代编译。
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

## 统一转义CMake路径；保留排序 / 去重规则，避免每个模块各写一套引用方式。
function Quote-CMake([string]$Value) {
    '"' + $Value.Replace('\', '/').Replace('"', '\"').Replace(';', '\;') + '"'
}

$projectDescriptor = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
$moduleNames = @($projectDescriptor.Modules | ForEach-Object { $_.Name } | Select-Object -Unique)
if ($moduleNames.Count -eq 0) {
    throw 'No project modules found in the .uproject file.'
}

## 单引号here-string保留CMake变量字面量；模板与当前模块无关，只构造一次。
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

$moduleBlocks = [System.Collections.Generic.List[string]]::new()
## 每个模块使用自己的rsp与强制Definitions头，禁止混用Core / UI / Editor导出宏。
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
    # 相对include基于Engine/Source解析；/D沿用UBT实际定义，不自行猜测宏。
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

    $block = $moduleTemplate.Replace(
        '@MODULE@', $moduleName
    ).Replace(
        '@INCLUDES@', $includeLines
    ).Replace(
        '@DEFINITIONS@', $defineLines
    ).Replace(
        '@FORCE_INCLUDE@', $forceInclude
    )
    $moduleBlocks.Add($block)
    Write-Host "Index metadata: $moduleName; includes: $($includePaths.Count)"
}

## 只有全部模块元数据验证通过才写最终文件；本脚本不驱动CodeModel目标编译游戏。
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
# 输出统一LF，使here-string不受PowerShell脚本本身行尾格式影响。
$cmake = $cmake.Replace("`r`n", "`n")
$cmakePath = Join-Path $projectRoot 'CMakeLists.txt'
[IO.File]::WriteAllText($cmakePath, $cmake, [Text.UTF8Encoding]::new($false))
Write-Host "CLion model refreshed: $cmakePath"
Write-Host "Modules: $($moduleNames -join ', '); real build target: EmberTrialEditor (Win64 Development)."
