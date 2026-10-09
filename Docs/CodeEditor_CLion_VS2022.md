# EmberTrial：CLion 日常开发与恢复 VS2022

更新日期：2026-10-03。适用本机 UE 5.5.4、CLion 2025.3.3、VS2022 17.14.20。

## 1. 工程、编辑器与编译器的关系

工程始终位于 `E:\UE_Project\Demo\EmberTrial`，主文件是 `EmberTrial.uproject`。Git 仓库仍在上一层 `E:\UE_Project\Demo`，切换 IDE 不会迁移源码、蓝图、地图或 Git 历史。

CLion 用于 C++ 编辑、导航、Git 和运行/调试入口；UE 用于蓝图、资产和 PIE。真正编译仍由 UnrealBuildTool（UBT）和 UnrealHeaderTool（UHT）完成，使用现有 VS2022 的 MSVC 14.38 与 Windows SDK。**保留 VS2022 的 C++ 工具链和 Windows SDK**，不需要为了换 IDE 卸载它们。

本机路径：

| 用途 | 路径 |
|---|---|
| UE | `D:\UE5\UE_5.5` |
| CLion | `E:\Clion\CLion 2025.3.3\bin\clion64.exe` |
| VS2022 | `D:\visual studio 2022` |
| UE 构建工具 | `D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat` |
| 索引刷新脚本 | `E:\UE_Project\Demo\Tools\Refresh-CLion.ps1` |

## 2. 为什么有 CLion 工程适配

UE 5.5.4 自带 CLion Source Code Access 插件，可在 Windows 中从 UE 打开 CLion；但其 CMake 生成模板仍使用 CMake 2.6 和 C++14，并且生成包含大量引擎源码的 FakeTarget。当前 CLion 的 CMake 4.1.2 与该模板不匹配。

本项目用刷新脚本生成本机的 `EmberTrial/CMakeLists.txt`：

- 使用 CMake 3.20 以上、C++20。
- 从各模块自己的 Shared.rsp 读取真实 include 目录，并强制包含对应 Definitions 头文件，以解析 UE 宏与生成头文件。
- `EmberTrialCodeModel` 与 `EmberTrialUICodeModel` 分别提供模块索引，排除在默认构建之外；不要把它们当游戏构建目标。
- `EmberTrialEditor` 才是常用真实构建目标：调用 UBT 构建 Win64 Development Editor。
- `EmberTrialEditor_DebugGame` 调用 UBT 构建 Win64 DebugGame Editor，可在后续需要更方便的变量观察时使用。

本机 CMake 文件、`.idea` 和 `cmake-build-*` 是可重建的 IDE 数据，已加入忽略规则；刷新脚本、本说明及 `.run/EmberTrialEditor.run.xml` 属于应保留的工程文件。运行配置记录了本机引擎和工程路径，换机器后需要调整。脚本现在按 .uproject 中各项目模块分别生成索引上下文；新增模块后先正常 UBT Build，再刷新索引 / Reload CMake。

**2026-10-02 更新：**用户已按 [M1-02A](M1_02A_RuntimeModules.md) 替换脚本；实际 CMake 与编译数据库现已包含 EmberTrialCodeModel 和 EmberTrialUICodeModel，真实 Build / Run 入口仍为 EmberTrialEditor。include 导航在光标移入文件名后恢复，本批 [验收通过](Reports/M1_02A_Verification.md)；[排查记录](CLion_HeaderNavigation.md) 保留供查阅。

## 3. CLion 日常编写、编译与运行

打开 `E:\UE_Project\Demo\EmberTrial\CMakeLists.txt` 时选择 **Open as Project**。

工具链使用 **Visual Studio / MSVC / amd64**。不要用 CLion 默认 MinGW 构建 UE 模块。[JetBrains：Windows 工具链](https://www.jetbrains.com/help/clion/quick-tutorial-on-configuring-clion-on-windows.html)

1. 在 `Source/EmberTrial` 中编辑并保存 C++；测试文件在其中的 `Tests` 文件夹。
2. 添加/修改 UCLASS、UPROPERTY、UFUNCTION 等反射声明后，保存所有资产并关闭 UE，进行正常 UBT Build。沿用当前项目的工作原则，避免依赖 Live Coding 完成结构性改动。
3. 在右上角选择 **EmberTrialEditor** 运行配置。点击锤子图标 **Build** 编译；点击绿色三角 **Run** 会先 Build，再启动本工程的 UE 编辑器。保持 UE 关闭后再进行正常 C++ Build，避免模块 DLL 被占用。不要选择 `EmberTrialCodeModel` 来编译游戏。
4. 在 UE 中 **Play** 运行游戏。修改蓝图、地图和资产仍在 UE 中进行。
5. 编译错误先复制 **Build 输出中的第一条真实错误及前后上下文**；运行问题提供复现步骤和 UE 日志；只有界面/连线问题优先附截图。无需手动摘录所有正常日志。

CMake profile 的名称或 Build type 与 UE 的目标配置不是同一层：真实 `EmberTrialEditor` 目标已经明确使用 Development；选择 CMake 的 Debug 并不自动把 UE 游戏变成 DebugGame。[JetBrains：CMake 工程](https://www.jetbrains.com/help/clion/quick-cmake-tutorial.html)

### 3.1 已配置好的运行入口

在 **Run → Edit Configurations** 中可以检查：

| 字段 | 当前值 |
|---|---|
| Name / Target | `EmberTrialEditor` |
| Executable | `D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe` |
| Program arguments | `"E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject"` |
| Working directory | `E:\UE_Project\Demo\EmberTrial` |
| Before launch | Build |

**调试：**保存并关闭 UE，在源码左侧行号旁点击设置断点，再点击右上角虫子图标 **Debug**。UE 打开后点击 Play，执行到该 C++ 位置时 CLion 会暂停；在 Debug 面板查看 Variables 和 Frames，点击 **Resume Program** 继续。不要为了“没反应”重复启动多个 UE。结束后停 PIE、正常关闭 UE，再开始下一次 Build。

本次在 `UETPracticeActionComponent::BeginPlay` 命中了断点，看到 GameThread 调用栈与组件变量；临时断点已移除。Development 优化下部分变量可能无法观察，需要时再单独配置和验证 DebugGame 的构建与运行入口；本次没有验证 DebugGame 路线。

### 3.2 代码检查的已知限制

当前保留 **CLion Nova** 语言引擎。此次观察到 `ETPracticeActionComponent.cpp` 中两处 `Movement->Velocity` 被 IDE 标成 **Member is inaccessible**，但真实 UBT 构建成功，Ctrl+B 能跳到引擎声明，调试也能运行。这是此配置下的索引 / 宏解析误报；不能据此修改 UE 引擎成员权限或删除游戏逻辑。

新增反射声明后先正常 Build，再按第 4 节刷新索引；仍有红线时，把 **IDE 提示与真实 Build 输出分开反馈**。真实编译成败以 UBT 输出为准。本次没有承诺 CLion 能完全理解 UE 的所有反射宏、蓝图引用或引擎专用检查。

## 4. 新文件与索引刷新

同一模块下新增 `.cpp/.h` 后，文件夹视图可直接显示，CMake 的 `CONFIGURE_DEPENDS` 会在加载/构建时更新源码清单。先保存并正常 Build，让 UHT 产生需要的生成头文件。

**从 UE 新建 C++ 类的例外：**本机 UE 5.5 插件只将两个新文件路径交给 CLion，可能把 Public / Private 子目录作为独立工程打开并生成嵌套 `.idea`；向导还可能覆盖主 CMake 为旧默认模板。2026-10-02 已发生并确认；用户在 M1-02B 验收时确认配置问题已解决、功能正常，现存 CMake / 编译数据库包含两个 Framework 新文件，不需要额外配置反馈。完整步骤见 [UE 新建类与 CLion 工程关联](CLion_UEClassCreation.md)：有完整代码时直接在完整工程 **New → File**；继续使用 UE 向导时取消新类自动编译、成功后拒绝即时编辑，再按编写阶段 -SkipBuild 或完成阶段正常 Build 刷新 / Reload。保留主工程 `.idea`。

更改 Build.cs 依赖、构建设置，或代码索引出现缺少 UE 头文件时：保存并关闭 UE，在 PowerShell 执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先正常 Build，再刷新本机 CMake 索引；随后在 CLion 执行 **Reload CMake Project**。如果刚完成正常 Build，可用 `-SkipBuild` 只更新索引：

```powershell
.\Tools\Refresh-CLion.ps1 -SkipBuild
```

不需要删除 Binaries、Intermediate、Saved 或 `.idea`。**不要只使用 UE 的原生 Refresh CLion Project 后直接打开旧模板**；若它覆盖了适配后的 CMakeLists.txt，重新执行本脚本即可恢复。

## 5. CLion 中管理 Git

Git root 应映射为 `E:\UE_Project\Demo`，不是新建一个 EmberTrial 子仓库。Git 可执行文件为 `C:\Program Files\Git\cmd\git.exe`。CLion 的 Git Log 应显示 main 与现有提交历史。

本地 Commit 保存历史，Push 上传到已配置的 GitHub 远端；切换 IDE 本身不建立远端或上传。查看待提交文件时，源码、Config、Content、Docs、Tools 均属于同一个仓库。IDE 索引、构建产物和 Saved 日志按 `.gitignore` 排除。

## 6. 日后恢复 VS2022：按此顺序操作

### 6.1 保存与关闭

1. 在 CLion 保存所有修改；在 UE 保存蓝图、地图与资产。
2. 关闭 UE 和当前 CLion 工程，避免构建/项目文件同步冲突。

### 6.2 修改工程的默认编辑器

打开：

```text
E:\UE_Project\Demo\EmberTrial\Config\DefaultEditorSettings.ini
```

将以下设置：

```ini
[/Script/SourceCodeAccess.SourceCodeAccessSettings]
PreferredAccessor=CLionSourceCodeAccessor
```

改为：

```ini
[/Script/SourceCodeAccess.SourceCodeAccessSettings]
PreferredAccessor=VisualStudio2022
```

这是项目默认值。**同时完成下面的界面步骤**，让本机个人偏好与默认值一致：打开 UE，进入 **Edit → Editor Preferences → General → Source Code → Source Code Editor**，选择 **Visual Studio 2022**，关闭偏好窗口，然后关闭并重新打开 UE。确认该下拉框仍显示 Visual Studio 2022，避免个人偏好覆盖项目默认值。

恢复 VS 不需要禁用或卸载 CLion 插件，也不需要卸载 CLion。

### 6.3 更新 VS 的项目清单

在资源管理器右键 `EmberTrial.uproject` → 必要时“显示更多选项” → **Generate Visual Studio project files**。也可用下面明确指定 VS2022 的命令；它不依赖当前 IDE 默认值：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' -ProjectFiles -Project='E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -Game -2022 -Automated
```

**不需要清理缓存或重新创建工程。** 原有 `.sln` 可继续使用；生成一次可同步新加入的文件。注意本机引擎在 D 盘、工程在 E 盘，优先沿用 `.sln` 路线。[Epic：生成 IDE 工程文件](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-generate-unreal-engine-project-files-for-your-ide?application_version=5.5)

### 6.4 在 VS 中编译与启动

1. 打开 `E:\UE_Project\Demo\EmberTrial\EmberTrial.sln`。
2. 配置选 **Development Editor**，平台选 **Win64**，启动项目选 **EmberTrial**。
3. 保持 UE 关闭，执行 Build，确认成功。
4. **Ctrl+F5** 正常启动 UE；需要调试时按 **F5**。也可直接打开 `.uproject`，在 UE 中 Play。
5. 在 UE 中双击 C++ 类，确认打开的是 VS2022；验证移动/转向/跳跃与当前 1/2/3、E/P 的基本运行。

原有源码、蓝图资产、Git 历史和 CLion 配置都可以保留。若以后再次使用 CLion，将第 6.2 节改回 `CLionSourceCodeAccessor`，执行 `Refresh-CLion.ps1`，再打开 CMakeLists.txt。

## 7. 本次操作与验证记录

| 检查 | 本次结果 |
|---|---|
| UE 默认代码编辑器 | 项目配置与 UE 偏好设为 CLion；重启 UE 后 Tools 菜单显示 Open CLion / Refresh CLion Project |
| 从 UE 打开代码编辑器 | 点击 Open CLion，打开本工程的 CLion 窗口 |
| CMake 工程加载 | 本机 MSVC 工具链成功配置，生成代码模型与 UBT 构建目标 |
| 真实构建入口 | 命令行适配验证、CLion 的 Build Before Run、恢复启动后的锤子 Build 均成功调用 UBT；最后一次 UBT 耗时 1.22 秒，Target is up to date，不记作全量重新编译 |
| 运行 | CLion Run 启动 UE 5.5.4 的 EmberTrial 工程，进入 ThirdPersonMap |
| 调试 | CLion Debug 启动 UE；PIE 命中组件 BeginPlay 断点，变量与调用栈可查看，恢复后 PIE 继续运行 |
| 代码导航 | 可打开游戏源码，Ctrl+B 从 Velocity 跳到引擎 MovementComponent.h 声明 |
| Git | CLion Git Log 显示父目录仓库的 main 与原有提交历史；未配置远端或上传 GitHub |
| 刷新脚本 | `-SkipBuild` 成功生成 CMake 文件，读取 302 条 include 路径 |

兼容性检查曾尝试 Classic 引擎；重启时发现旧 CLion 实例的 `.port` 端点残留导致系统目录无法锁定。现已恢复 Nova，并恢复正常启动。原有缓存、索引与 LocalHistory 保留在原路径；仅将失效的 `.port` 和旧 `.pid` 留在 `C:\Users\D1334\AppData\Local\JetBrains\CLion2025.3-stale-socket-20261001` 备份目录。这是此次启动故障的修复记录，不是日常刷新流程。

本次只验证编辑器切换与上述开发入口。没有修改 Gameplay 源码，没有重新执行 Lifecycle / DodgeMovement 自动测试，也没有代替 M0-05C3 组合演示验收。
