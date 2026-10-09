# M1-02A：Runtime 模块骨架与 CLion 索引

生成 / 验收日期：2026-10-02。前置：[M1-01 已验收](Reports/M1_01_Verification.md)。**本批已通过**，手动 30 min / 等待 5 min；导航在光标移入文件名后恢复。源码 / 配置、构建与索引 / 加载证据见 [验收报告](Reports/M1_02A_Verification.md)。下方原步骤保留供复查，无需重做。下一批：[M1-02B](M1_02B_FrameworkObjects.md)。所有文件由你手动创建或修改，我负责方案、代码说明和后续审查。

## 1. 本批目标

建立第二个运行时模块 **EmberTrialUI**，保持 Gameplay → UI 的事件出口清晰，并让 CLion 分别使用两个模块的真实编译上下文。预计手动 **35–55 分钟**，UBT 构建 / IDE 索引等待另计。

本批仅创建模块入口，不创建 Widget 或替换现有角色。现有蓝图、练习组件、地图、Montage 和测试保留原路径。M1-02 后续再分批建立正式对象、UI Bootstrap 与生命周期；M1-03 才接入 GAS / Health / HUD。

~~~mermaid
flowchart LR
    BP["后续 BP_ETPlayerController 装配"] --> Core["EmberTrial：玩法、状态、公开契约"]
    BP --> UI["EmberTrialUI：UI Bootstrap、Presenter、Widget"]
    UI -->|依赖公开契约| Core
~~~

核心模块不包含 UI 模块或 UMG；UI 模块可以查询核心状态和订阅核心事件。Blueprint 负责把两边装配到 Controller 上，避免核心 C++ 引用 UI 类而造成循环依赖。UBT 的模块配置与 IDE 文件列表是不同层次。[Epic：UE 5.5 模块](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules?application_version=5.5)

### 1.1 后续对象归属，先明确边界

| 对象 | 归属 / 职责 | 生命周期原则 |
|---|---|---|
| 正式 GameMode / Character / PlayerController | EmberTrial；本局规则、身体与玩法组件、输入 / Possess | GameMode 不创建 Widget；Character 的 ASC 在身体重建时重新初始化 |
| UI Bootstrap 组件 | EmberTrialUI；装配在 Controller，只服务本地玩家 | Pawn 更换先解绑旧数据，再绑定已就绪的新 Pawn |
| UILocalPlayerSubsystem | EmberTrialUI；UI 根节点、页面 / 输入策略 | 切图必须清理旧世界订阅，不能因 LocalPlayer 存活而保留旧 Actor |
| Presenter / Widget | EmberTrialUI；展示适配 / 视觉布局 | 状态来自玩法，Widget 不维护第二套生命 / 动作规则 |

本表是后续方向，**本批不创建这些类型**。保持 Introduction 中“Character 持有 ASC”的单机取舍；不新建 PlayerState 来重复保存实时战斗状态。

## 2. 操作前准备

1. UE 执行 **Save All**，退出 PIE，关闭 UE 编辑器。此次增加模块 / 修改 Target，需正常构建并加载新的 DLL；不使用 Live Coding。
2. CLion 保存全部文件，可以保持打开。切换 Project 文件视图；文件也可在资源管理器创建。开启资源管理器 **View → Show → File name extensions**，避免保存成 `.cpp.txt`。
3. 下面路径均为本机绝对路径。只修改第 3–4 节列出的文件。原 `Source\EmberTrial` 不移动到 Public / Private，避免影响当前测试的相对 include。

## 3. 新建 UI 模块文件

在 `E:\UE_Project\Demo\EmberTrial\Source` 下创建以下目录和两个文件：

~~~text
Source/
└─ EmberTrialUI/
   ├─ EmberTrialUI.Build.cs
   ├─ Public/                 本批可为空
   └─ Private/
      └─ EmberTrialUIModule.cpp
~~~

### 3.1 EmberTrialUI.Build.cs

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\EmberTrialUI.Build.cs`。完整内容：

~~~csharp
using UnrealBuildTool;

public class EmberTrialUI : ModuleRules
{
    public EmberTrialUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "EmberTrial"
        });
    }
}
~~~

这里先只有模块入口，没有 UObject / Widget。Core 提供模块与日志 API；EmberTrial 是未来公开玩法契约的依赖。首次添加 Widget 基类时再加入所需 CoreUObject / Engine / UMG / Slate 依赖，并按 Public 头文件实际暴露的类型区分 Public / Private 依赖。不要把 UnrealEd 或 EmberTrialUI 加入核心模块的 Build.cs。

### 3.2 EmberTrialUIModule.cpp

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\EmberTrialUIModule.cpp`。完整内容：

~~~cpp
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUIModule, Log, All);

class FEmberTrialUIModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogETUIModule, Display, TEXT("[M1-02A] EmberTrialUI module ready"));
    }

    virtual void ShutdownModule() override
    {
        UE_LOG(LogETUIModule, Display, TEXT("[M1-02A] EmberTrialUI module shutdown"));
    }
};

IMPLEMENT_MODULE(FEmberTrialUIModule, EmberTrialUI)
~~~

`IMPLEMENT_MODULE` 注册普通模块。原 EmberTrial 的 `IMPLEMENT_PRIMARY_GAME_MODULE` 保持不动，项目仍只有一个主游戏模块。

启动日志是“编辑器进程加载模块”的日志，**不会每次 Play 重打**；关闭整个编辑器时才有 shutdown。StartupModule 不代表角色 / 世界已准备好，不能在这里创建 HUD 或查找玩家。

## 4. 注册模块与升级索引

### 4.1 修改 Game Target

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial.Target.cs`。

只把原来的：

~~~csharp
ExtraModuleNames.Add("EmberTrial");
~~~

替换为：

~~~csharp
ExtraModuleNames.AddRange(new string[] { "EmberTrial", "EmberTrialUI" });
~~~

### 4.2 修改 Editor Target

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor.Target.cs`。

做与 4.1 **相同的一处替换**，不要修改 TargetType、BuildSettingsVersion 或 IncludeOrderVersion。

Target 列出两模块，表示该目标要构建它们；这不会让 EmberTrial 的 C++ 反向依赖 EmberTrialUI。

### 4.3 修改 .uproject

文件：`E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`。添加第二个 Modules 对象。按目前文件核对后的完整内容如下，可完整替换：

~~~json
{
    "FileVersion": 3,
    "EngineAssociation": "5.5",
    "Category": "",
    "Description": "",
    "Modules": [
        {
            "Name": "EmberTrial",
            "Type": "Runtime",
            "LoadingPhase": "Default",
            "AdditionalDependencies": [
                "Engine"
            ]
        },
        {
            "Name": "EmberTrialUI",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ],
    "Plugins": [
        {
            "Name": "ModelingToolsEditorMode",
            "Enabled": true,
            "TargetAllowList": [
                "Editor"
            ]
        }
    ]
}
~~~

注意两个模块对象之间的逗号。实际 C++ 依赖写在 Build.cs；此次保留原 AdditionalDependencies 和 Modeling 插件设置。

### 4.4 升级 Refresh-CLion.ps1

文件：`E:\UE_Project\Demo\Tools\Refresh-CLion.ps1`，**完整替换**为下方代码。该操作也是你手动执行。

旧脚本只读取 EmberTrial 的编译信息。新脚本从 .uproject 取得模块名，分别读取每个模块的 Shared.rsp / Definitions 头文件，生成各自的 CodeModel，避免把两模块不同的导入 / 导出宏混到一起。真实构建和现有运行配置名称保持一致。

~~~powershell
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
~~~

不要自己手动维护生成的 CMakeLists.txt。新增 .cpp / .h 后 Reload CMake 通常可重新扫描文件；新增模块、模块依赖或反射头文件后，先正常 UBT Build，再刷新脚本和 Reload。不需要清理 Binaries、Intermediate、Saved 或 .idea。

## 5. 正常 Build 与 CLion 验证

### 5.1 保存全部文件，构建 Editor 并刷新索引

确认 UE 已关闭，在 CLion Terminal 或 PowerShell 执行：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

这一次**不要加 -SkipBuild**：新模块还没有 UBT 元数据。成功后应有：

~~~text
Index metadata: EmberTrial; includes: ...
Index metadata: EmberTrialUI; includes: ...
CLion model refreshed: E:\UE_Project\Demo\EmberTrial\CMakeLists.txt
Modules: EmberTrial, EmberTrialUI; real build target: EmberTrialEditor (Win64 Development).
~~~

然后在 CLion 执行 **Reload CMake Project**。保持原 MSVC / amd64 工具链和 EmberTrialEditor 运行配置。Project 视图可展开 Source/EmberTrialUI，确认两个新文件可见；在新 .cpp 中对 ModuleManager.h 使用 Ctrl+B，确认能导航到引擎头文件。

生成的 CMakeLists.txt 应包含 EmberTrialCodeModel 和 **EmberTrialUICodeModel**。这两个都是索引对象，**不要选它们 Build 游戏**。仍使用 EmberTrialEditor 的锤子 / Run / Debug 入口。Public 空目录可能不显示，属正常。

Ctrl+B 的光标应在 include 引号内的文件名上；本次问题已按此操作解决，见 [导航记录](CLion_HeaderNavigation.md)。不需要改依赖或 include。

### 5.2 检查非 Editor 的 Runtime 构建

保持 UE 关闭，执行一次 Game Development Build：

~~~powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
~~~

这一步确认 UI 模块没有误带入 Editor 依赖；它只编译 Game，**不会 Cook、打包或更新旧 M0 exe**。无需重新执行 M0-06 打包。

## 6. 加载与最小回归

1. 在 CLion 选择 **EmberTrialEditor → Run**，或正常打开 .uproject；不要同时开两份 UE。
2. 打开 **Window → Developer Tools → Output Log**，搜索 `M1-02A`。本次编辑器启动应有一条 `EmberTrialUI module ready`。
3. Content Browser 打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`。默认启动地图本批仍保持原值，所以若启动在 ThirdPersonMap，手动切到新地图即可。
4. Play：移动 / 转向 / 跳跃；站定分别 1 / 2 / 3，再做一次 2→3；靠近 A / B 各按 E；P 暂停后继续，确认控制恢复。只做这轮基本回归，不重跑坡面 / 帧率矩阵或已有自动测试。
5. Stop 再 Play，确认目标状态重置、三动作能正常结束、没有新增运行错误。停止并 Save All。

此模块没有 UCLASS，Content Browser 的 C++ Classes 中未显示 EmberTrialUI 不代表加载失败；以 Build、启动日志和源码文件为准。

## 7. 问题反馈与完成格式

Build 失败时先给**第一条真实错误及前后约 15 行文字**、执行命令和文件路径；我会读取本机日志核对。只有 CLion 红线时，补充是否 UBT 成功、出错文件和索引截图。不用清缓存重建，也不用自行反复启动 UE。

~~~text
M1-02A 完成 / 卡在第 __ 节
新模块两个文件、两个 Target 与 .uproject：已按文档保存
Refresh-CLion 脚本已手动替换：是
Editor Build / 索引刷新：成功
Game Development Build：成功
CLion 两个 CodeModel / 新文件可见 / 头文件导航：通过
本次启动一条 EmberTrialUI module ready：是
新地图两次 Play；移动、1/2/3、2→3、E/P：通过
源码与资产已保存：是
大致完成时间：今天 __:__，日志由你读取
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
~~~

本批已通过，现交接 [M1-02B 正式对象 / 公开边界](M1_02B_FrameworkObjects.md)，随后处理 UI 的 Pawn 重绑 / 清理；**A 通过不等于整个 M1-02 完成**。

下发前检查：本机引擎头文件路径、文档 JSON / PowerShell 语法通过；引用脚本使用现有 EmberTrial 元数据成功生成隔离的索引文件（302 条 include）。没有写入实际脚本 / CMake，没有执行新 UBT Build；第二模块的实际元数据与加载由本批手动验收确认。
