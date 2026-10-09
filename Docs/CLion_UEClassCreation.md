# CLion：UE 新建类后的工程关联、CMake 与索引处理

更新日期：2026-10-03。适用本机 UE 5.5.4 / CLion 2025.3.3。**M1-02B 已验收，用户确认 CLion 配置问题已解决、功能正常，无需另发配置反馈。** 当前 CMake 与两个模块索引已核对恢复；本页保留日后查阅。若 UE 新建类再次出现版本 / 目标错误，代码已完成时执行第 4.2 节，仍在编写时执行第 4.3 节。

## 1. 首次排查原因与已完成的恢复（历史记录）

UE 的“新建 C++ 类”和“打开整个代码工程”使用不同入口。检查本机插件发现：

- [OpenSourceFiles](D:/UE5/UE_5.5/Engine/Plugins/Developer/CLionSourceCodeAccess/Source/CLionSourceCodeAccess/Private/CLionSourceCodeAccessor.cpp:136) 只传入新建的 .h / .cpp 路径，没有传入 EmberTrial 工程根目录。
- [OpenFileAtLine](D:/UE5/UE_5.5/Engine/Plugins/Developer/CLionSourceCodeAccess/Source/CLionSourceCodeAccess/Private/CLionSourceCodeAccessor.cpp:78) 与 OpenSolution 会传入工程根目录。不能据此假设“新建类”也会打开同一工程。
- 首次排查时 Public / Private 的 Framework 文件夹分别出现了 `.idea`，其中保存了独立工程状态，说明 CLion 将这两个目录作为工程打开。`.idea` 是 IDE 元数据，不是 UE 模块或 C++ 类文件；当前这两处已不存在。

[JetBrains 的命令行说明](https://www.jetbrains.com/help/clion/2025.3/opening-files-from-command-line.html) 区分文件和工程路径：打开文件不保证打开其所属完整工程。因此，这里应修正打开工程的入口，保留已有源码。

还有第二个问题：UE 日志在本地时间 **21:22–21:23** 记录了 Generating CLion project files 与 Hot Reload；主 `CMakeLists.txt` 被覆盖成 CMake 2.6 / C++14 / FakeTarget 的 UE 默认模板。此前的双模块 CodeModel 需要通过本项目脚本恢复。

助手首次排查时已完成：

1. 将 UE 生成的 CMake 与两处 Framework 的 `.idea` 复制备份到 `E:\UE_Project\Demo\Artifacts\IDERecovery\20261002_213429`。
2. 执行 `Tools\Refresh-CLion.ps1 -SkipBuild`，恢复主 CMake 的 C++20、EmberTrialCodeModel、EmberTrialUICodeModel 与 UBT 构建目标。
3. 保留主工程 `E:\UE_Project\Demo\EmberTrial\.idea` 与 `.run`；当时留下两处嵌套 `.idea` 供你关闭窗口后清理。后续读取目录确认它们已不存在。
4. 没有修改 C++，没有执行正常 Build，没有验证 CLion 界面已重新加载。此次 `-SkipBuild` 复用现有构建元数据，完成本批源码后仍需正常 Build / 刷新。

**后续更新：**用户已确认可以正常从 UE 创建文件。日志在本地时间 **21:44–21:45** 记录了新建 ETGameMode、Generating CLion project files，并确认“是否立即编辑代码”选择了 No。窗口问题已经处理，但 UE 生成 CMake 的行为仍然存在；第二次版本 / 索引问题的步骤见第 4 节。本轮只更新本文件，没有代执行刷新 / Build。

## 2. Framework 独立窗口出现时：恢复完整 CLion 工程

当前已确认不再打开错误窗口，且两处嵌套 `.idea` 已不存在，可以跳过本节。若以后再次遇到相同窗口问题，再按以下步骤处理。

### 2.1 保存并关闭错误窗口

在只包含 ETPlayerController 文件的 CLion 窗口中保存文件，然后选择 **File → Close Project**。如果 Public / Private 各打开了一个 Framework 工程，两处都关闭。保留或随后重新打开完整的 EmberTrial 工程。

这里关闭的是 IDE 工程窗口，不是删除源码。

### 2.2 清理两处独立工程元数据

在资源管理器中确认上面的备份目录存在后，只删除下面两处 `.idea` 文件夹：

~~~text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\.idea
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\.idea
~~~

保留同目录的 ETPlayerController.h / .cpp。**不要删除主工程的 `E:\UE_Project\Demo\EmberTrial\.idea`**；它保存现有运行配置、索引偏好与其他工程状态。若看不到点开头的文件夹，可将上述完整路径粘贴到资源管理器地址栏定位。

两处嵌套 `.idea` 已被 Git 忽略，本次读取 `git check-ignore` 已确认；不需要再修改忽略规则。

### 2.3 从正确根目录打开

在 CLion 使用 **File → Open**，选择：

~~~text
E:\UE_Project\Demo\EmberTrial\CMakeLists.txt
~~~

弹出选择时用 **Open as Project**。如果完整工程已经打开，直接切回该窗口。

随后按 **Ctrl+Shift+A**，搜索 **Reload CMake Project** 并执行，等待配置和索引完成。这是重新加载，不是 Reset Cache / Invalidate Caches。[JetBrains：Reload CMake](https://www.jetbrains.com/help/clion/2025.3/reloading-project.html)

不要把 Public\Framework 或 Private\Framework 作为一个新项目打开，也不需要给它们创建 CMakeLists.txt。

### 2.4 确认恢复

应看到：

- 工程根目录为 `E:\UE_Project\Demo\EmberTrial`，而不是 Framework。
- Project 树中同时有 Source\EmberTrial 与 Source\EmberTrialUI；在其中打开已创建的 ETPlayerController.h / .cpp。
- CMake 目标包含 EmberTrialCodeModel、EmberTrialUICodeModel 和 EmberTrialEditor；实际编译 / 运行仍选 **EmberTrialEditor**。
- 既有 EmberTrialEditor 的启动配置还在，Git root 仍是 `E:\UE_Project\Demo`。
- Framework 下不再出现两处独立工程的 `.idea`。

现阶段可继续完成 M1-02B 的代码。新反射类的最终索引和运行结果，留到本批正常 Build 后确认。

## 3. 后续如何创建类

### 3.1 本批有完整代码时：直接在完整 CLion 工程创建文件

对于 [M1-02B](M1_02B_FrameworkObjects.md) 中已经给出完整声明 / 实现的类，可以继续采用原指南：

1. 保存 UE 资产并关闭 UE。
2. 在完整 EmberTrial 工程的 Project 树，右键指定的 Public 或 Private 子目录，选择 **New → File**。
3. 分别输入含扩展名的文件名，如 ETGameMode.h / ETGameMode.cpp，粘贴指南中的代码。
4. 完成本批后正常 Build / 刷新索引。

使用普通 File 即可；不用另建 CMake Project，也不用让 CLion 自动向 CMake 添加一个可执行目标。UE 类由 UCLASS / GENERATED_BODY 等声明和 UHT 构建识别，不要求必须由 UE 向导创建。`*.generated.h` 由 UHT 生成，不手动创建。

**已经存在的 ETPlayerController.h / .cpp 不要再次创建。**

### 3.2 如果继续使用 UE 的“新建 C++ 类”

可以保留这个习惯，但需要关闭自动编译并拒绝即时打开文件。

**一次性设置：**

1. 在 UE 打开 **Edit → Editor Preferences → General → Miscellaneous**。
2. 在 **Hot Reload** 分类取消 **Automatically Compile Newly Added C++ Classes（自动编译新添加的 C++ 类）**。
3. 本机当前 Live Coding 为关闭状态，因此第 2 步对应本次实际触发的路径。若以后启用 Live Coding，再到 **General → Live Coding**，同时取消该页面上同名的自动编译选项；两者是不同设置。无需为了取消灰色选项而启用 Live Coding。[Epic：Live Coding 设置](https://dev.epicgames.com/documentation/unreal-engine/using-live-coding-to-recompile-unreal-engine-applications-at-runtime?lang=zh-CN)
4. 偏好窗口可搜索 `Automatically Compile` 或中文“自动编译”，确认取消的是 Miscellaneous / Hot Reload 里的选项。源码编辑器继续保持 CLion。

**每次新建：**

1. 在 UE 向导选正确模块、父类和 Public / Private 路径，创建类。
2. 创建成功后，UE 会提示需要编译，并询问 **Would you like to edit the code now?（是否现在编辑代码）**：选择 **No / 否**。
3. **新建类后先恢复被 UE 覆盖的主 CMake**：代码尚未写完时按第 4.3 节用 -SkipBuild 恢复索引配置，再 Reload CMake；已经完成这一批源码时，直接按第 4.2 节关闭 UE、正常 Build / 刷新。
4. 回到已打开的完整 EmberTrial 工程，从 Project 树打开两个文件并继续编辑。
5. 如果第 3 步使用了 -SkipBuild，完成代码后仍按第 4.2 节进行正常 Build / 刷新，生成新类的反射头文件。

本机 [创建类对话框源码](D:/UE5/UE_5.5/Engine/Source/Editor/GameProjectGeneration/Private/SNewClassDialog.cpp:1233) 已确认：没有自动重载时才会询问是否编辑；选择 No 不调用 OpenSourceFiles。**只取消自动编译、随后仍选择 Yes，会再次走原来的文件打开入口。**

UE 向导仍可能重新生成默认 CMake；关闭自动编译不会阻止这一步。因此，使用 UE 向导后应执行项目刷新脚本恢复适配配置。只把 CLion 的“新工程在同一窗口打开”改成同窗口，仍会打开错误工程根目录。

## 4. CMake 版本错误与 This file does not belong to any project target

### 4.1 两个提示的原因

这次实际读取的 `E:\UE_Project\Demo\EmberTrial\CMakeLists.txt` 已重新变成：

~~~cmake
cmake_minimum_required (VERSION 2.6)
project (Unreal)
set(CMAKE_CXX_STANDARD 14)
~~~

这是 UE 默认生成器的旧模板，覆盖了本项目刷新脚本生成的适配文件。本机 CLion 的 CMake 4.x 不再兼容小于 3.5 的策略版本，因此在第 4 行停止配置。[CMake 官方说明](https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html)

CMake 配置失败后，CLion 不能更新目标与源文件列表。现存 `compile_commands.json` 还只有 Controller 的编译记录，没有 ETGameMode.cpp；因此新文件提示 **This file does not belong to any project target**。这是当前 IDE 构建模型未更新的现象，不能单凭它判断 UE 类实现或 UBT 编译失败。

**本项目的修复是重新运行已有刷新脚本，恢复整个适配模型。** 单独把 2.6 改成 3.5，或添加错误信息建议的 `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`，只处理版本门槛；旧模板的 C++14、FakeTarget 和模块索引设置仍然存在。它们不作为本项目的修复步骤。

### 4.2 本次推荐：代码完成后，正常 Build / 刷新再 Reload

本次已在正确目录找到 ETGameMode.h / .cpp，两份内容与 M1-02B 提供的代码一致。若你已经完成该指南第 3–5 节，按以下顺序操作。

**第一步：保存并关闭 UE。** CLion 可以保持打开并保存全部文件；暂时忽略已有的 CMake 失败提示。

**第二步：在独立 PowerShell 或 CLion 的 Terminal 中执行：**

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

这次**不带 -SkipBuild**。脚本直接调用 UE 的 Build.bat / UBT，先正常构建 EmberTrialEditor、运行 UHT，再读取两个模块最新的元数据生成主 CMake。它不依赖 CLion 当前失败的 CMake；因此可以从 Terminal 执行，暂时不用 CLion 的锤子按钮。

等待命令结束。除了正常 UBT 成功结果，应看到类似：

~~~text
Index metadata: EmberTrial; includes: ...
Index metadata: EmberTrialUI; includes: ...
CLion model refreshed: E:\UE_Project\Demo\EmberTrial\CMakeLists.txt
Modules: EmberTrial, EmberTrialUI; real build target: EmberTrialEditor (Win64 Development).
~~~

**第三步：回到完整 EmberTrial 工程，按 Ctrl+Shift+A，搜索并执行 Reload CMake Project。** 等待 CMake 配置和代码分析结束，再检查新文件。不要在执行刷新脚本后又点 UE 的原生 Refresh CLion Project；那会重新覆盖主 CMake。[JetBrains：Reload CMake](https://www.jetbrains.com/help/clion/2025.3/reloading-project.html)

**第四步：确认下列结果：**

- 主 CMake 开头恢复为 `cmake_minimum_required(VERSION 3.20)`、`project(EmberTrial LANGUAGES CXX)` 与 `set(CMAKE_CXX_STANDARD 20)`。
- CMake 不再报旧版本兼容错误，配置 / 生成成功。
- 目标中有 **EmberTrialCodeModel / EmberTrialUICodeModel / EmberTrialEditor**。CodeModel 用于索引；游戏真实编译 / 启动仍使用 EmberTrialEditor。
- 从完整工程树打开 ETGameMode.h / .cpp，两个文件的“无项目目标”提示消失；include 导航等待分析完成后再检查。
- UHT 已生成 ETGameMode.generated.h；其缺失若在正常 Build 前出现，不要手动创建这个文件。

随后继续 M1-02B 的 Game Build、Blueprint 装配与实机回归。本次索引修复不替代整批验收。

### 4.3 如果代码还没写完：先恢复索引，稍后正常 Build

有时刚用 UE 创建了空模板，还需要编写代码。可以先执行：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1 -SkipBuild
~~~

随后在完整 CLion 工程执行 **Reload CMake Project**。此操作只用已经存在的模块构建元数据重建主 CMake，不编译游戏，因此可以先解决版本错误和文件的目标关联，再继续编辑。

本项目两个 Runtime 模块已有成功构建记录，所以这条临时路线具备前提。`-SkipBuild` 不能为从未构建过的新模块生成元数据，也不能生成新 UCLASS 的 `*.generated.h`；此阶段可能仍有生成头文件缺失或补全不完整。完成代码后必须保存并关闭 UE，再执行第 4.2 节的**不带 -SkipBuild**流程，使索引使用最新构建信息。

如果第 4.2 节的 UBT Build 失败，脚本会在覆盖 CMake 前停止。先复制第一条真实编译错误与上下文；需要先恢复编辑环境时可按本节重建索引配置，随后修正 C++ 并重新正常 Build。

### 4.4 为什么不用手动给新文件添加目标

刷新脚本生成的主 CMake 使用 `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`，递归收集每个模块下的 .h / .cpp，因此当前两份文件已在正确收集范围：

~~~text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\ETGameMode.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETGameMode.cpp
~~~

恢复适配 CMake 并成功 Reload 后，它们应归属 **EmberTrialCodeModel**。不需要点提示里的 **Add to CMake Project**，也不用手动新增 `add_executable` / `add_library`；手动改生成文件还会被下次刷新覆盖。头文件不要求在 compile_commands.json 中有独立编译记录。

如果执行完第 4.2 节后仍提示无目标，依次检查：

1. 终端是否真正出现 `CLion model refreshed`；若没有，先处理终端中的首个错误。
2. 正在打开的根目录是否为 `E:\UE_Project\Demo\EmberTrial`；是否执行了 **Reload CMake Project** 并等待完成。
3. 文件是否仍位于以上 Source / 模块路径，扩展名是否确为 .h / .cpp；CMake 的两个 CodeModel 是否存在。
4. 从完整 Project 树重新打开文件；必要时 File → Close Project，再打开主 CMakeLists.txt 作为工程并 Reload。

若仍异常，提供最新 CMake 完整错误输出与文件完整路径，再继续定位。当前这两种提示无需删除主 `.idea`、Binaries、Intermediate 或清除全局缓存。

### 4.5 固定日常顺序

| 所处阶段 | 操作 |
|---|---|
| UE 创建类后，还要继续编写代码 | 拒绝即时编辑 → -SkipBuild 刷新 → Reload CMake → 在完整工程编辑 |
| 这一批源码已完成 | 保存并关闭 UE → 不带 -SkipBuild 刷新（正常 UBT / UHT）→ Reload CMake → 运行验证 |
| 本模块普通编辑，没有新增反射声明 / 依赖 | 沿用正常 Build / Run；需要重新收集文件时 Reload CMake |

每次使用 UE 新建类向导都可能覆盖主 CMake。关闭新类自动编译解决的是 Hot Reload / 即时打开路径；**恢复适配 CMake 是另外一个步骤**，需要在每次向导操作后执行。

## 5. 反馈方式

先反馈 IDE 是否恢复，无需提前做 M1-02B 全部验收。当前重点是第 4 节的结果：

~~~text
CLion 工程恢复
完整 EmberTrial 工程 / 两个模块 / 原运行配置：正常或异常
Framework 两处 .idea 已清理：是 / 否
Miscellaneous → Hot Reload 自动编译已取消：是 / 否
是否仍会自动打开 Framework 独立窗口：
本次刷新是否带 -SkipBuild：
正常 Editor Build（若已执行）：成功 / 失败 / 未执行
终端是否出现 CLion model refreshed：是 / 否
Reload CMake：成功 / 失败
ETGameMode.h / .cpp 无项目目标提示：已消失 / 仍存在
include 导航 / 生成头文件（正常 Build 后）：正常 / 异常 / 未验证
~~~

若恢复失败：界面问题发一张包含工程树、窗口标题及 CMake 提示的截图；CMake / Build 问题复制第一条错误与上下文。C++ 仍由你按指南手动修改，明确要求直接操作后才由助手接管。
