# M0-06：首次 Windows Development 打包

生成日期：2026-10-01。前置：[M0-05 动画路线已验收](Reports/M0_05C3_Verification.md)。**本批已通过，下面保留操作步骤供复盘。**用户手动 30 分钟 / 等待约 26 分钟，UAT ExitCode=0、Game receipt 和一轮包内日志已核对；两次启动及完整交互以用户反馈登记，第一轮日志未单独留存。详见 [M0-06 验收](Reports/M0_06_Verification.md)。

本批由你手动完成 UE 配置、打包、离开编辑器运行与反馈。我读取本地配置、UAT / UBT 和游戏日志；出现问题先定位并给出修正步骤，只有你明确要求直接操作时才改 C++。

## 1. 目标、工期与编译方式

得到一个 **Win64 / Development / Game** 包，关闭 UE 编辑器后仍可操作 ThirdPersonMap，现有 1 / 2 / 3、E、P、移动 / 转向 / 跳跃保持正常，中文 UI 可读，重新启动后临时状态重置。

预计手动操作 **15–25 分钟**，首次编译 / Cook 另留 **10–40 分钟或更久**，取决于缓存、着色器和磁盘。无需重复 C2 的完整测试矩阵，也无需再录组合视频；有动画时序问题再录短片。

| 项目 | 本次使用 |
|---|---|
| 工程 | `E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject` |
| 引擎 | UE 5.5.4，`D:\UE5\UE_5.5` |
| 打包目标 | `EmberTrial`，已有 TargetType.Game |
| 输出 | `E:\UE_Project\Demo\Artifacts\Builds\M0_06\Attempt01`，目录已准备 |
| 独立运行日志 | `E:\UE_Project\Demo\Artifacts\M0_06\EmberTrial_Standalone.log` |

CLion 继续编辑代码，现有 CLion 的 EmberTrialEditor 目标用于编辑器构建 / 调试。**此次打包由 UE 的 UAT 调用 UBT 构建 Game 目标**，会沿用已安装的 MSVC / Windows SDK；无需切回 VS。之前 Editor Build 成功不能代替本次 Game Build。

打包依次涉及 Build（代码）、Cook（转换资源）、Stage（收集运行文件）与 Package / Archive（打包和输出）。独立运行才验证默认地图、输入、UMG、动画依赖及包内资源。参见 [Epic 的 UE 5.5 打包说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project?application_version=5.5)。

## 2. 打包前保存与地图检查

1. 打开当前 EmberTrial 工程，确认版本为 5.5.4，回到 **ThirdPersonMap**；退出 PIE。
2. 对 BP_ThirdPersonCharacter、ABP_Manny、ABP_Quinn 和已有的提示 / 暂停 Widget 点 Compile，确认没有编译错误；只检查现有资产。
3. 使用 **File → Save All** 保存。打包期间暂停编辑，CLion 可以保留打开，但不要同时触发另一个 Build 或 Live Coding。
4. 打开 **Edit → Project Settings → Project → Maps & Modes**。
5. **Game Default Map** 确认为 ThirdPersonMap；Editor Startup Map 也保留 ThirdPersonMap。当前 DefaultEngine.ini 已有正确配置，如果一致不必修改。

地图对应资产：`/Game/ThirdPerson/Maps/ThirdPersonMap`。**Game Default Map 决定包内启动地图**，仅在编辑器打开这张地图不能替代该配置。

当前全局 GameMode 为 EmberTrialGameMode，其 C++ 默认 Pawn 指向 BP_ThirdPersonCharacter；保持已有正确配置。地图自身如有 GameMode Override，也保持当前已通过 PIE 的设置。

## 3. Packaging 配置

在 Project Settings 左侧选择 **Project → Packaging**。搜索框每次只搜一个字段，检查后清空再搜下一个；高级字段可展开对应类别的 Advanced。以下为本次明确配置：

| 英文字段 / 搜索词 | 本次值 | 目的 |
|---|---|---|
| Build | Always | 此次明确执行 Game 构建 |
| Build Configuration | Development | 保留调试 / 日志能力 |
| Full Rebuild | 勾选 | 首次包采用完整项目重编译；后续迭代再考虑关闭 |
| For Distribution | 不勾选 | 本次按 Development 运行验收 |
| List of maps to include in a packaged build | 一项 ThirdPersonMap | 明确 Cook 入口地图 |

设置地图列表的具体步骤：

1. 搜索 `List of maps`，找到完整字段 **List of maps to include in a packaged build**。
2. 点击数组右侧 **+**，使列表有一项；展开 Index [0]。
3. 在 File Path 输入：

   ```text
   /Game/ThirdPerson/Maps/ThirdPersonMap
   ```

4. 若使用右侧文件选择器，选 `EmberTrial\Content\ThirdPerson\Maps\ThirdPersonMap.umap`，再确认显示为上面的 `/Game/...` 资源路径。手动输入时不带 `.umap`、磁盘盘符或 `Content` 前缀。
5. 当前第一次打包不需要把动画试场 L_M0_AnimationLab 也加入地图列表。

另确认以下现有默认值；没有改过时通常已一致：

- **Use Pak File**、**Use Io Store**：勾选；使用 Io Store 时出现 `.utoc / .ucas` 正常，不要求所有资源都装在一个 `.pak` 中。
- **Include Prerequisites Installer**：勾选。包含安装程序不代表已经在另一台电脑验证过依赖。
- **Cook everything in the project content directory**：不勾选。先从指定地图及其依赖 Cook，避免整个 Content 无差别进入包。
- **Exclude editor content when cooking**：本批保持不勾选。保留目前默认设置，后续有包内资源证据再精简。
- **Generate Chunks**、**Use Zen Server as cooked output store**：本批保持关闭。

当前角色 / Montage / UMG / Input 的引用链应随地图和 Pawn 一起 Cook；是否齐全最终以包内运行确认。中文显示还受字体资源影响，独立验收时必须看提示和菜单，不能只看打包成功。

关闭 Project Settings 后再次 Save All。多数项目设置会直接写入 Config；我会在收到结果后核对实际文件。字段含义参见 [Epic Packaging 项目设置](https://dev.epicgames.com/documentation/en-us/unreal-engine/project-section-of-the-unreal-engine-project-settings?application_version=5.5)。

## 4. 手动执行打包

1. 在 UE 编辑器顶部工具栏找到 **Platforms（平台）**，展开 **Windows**。
2. 在 **Binary Configuration** 区域选择 **Development**。这样平台级设置也明确为 Development，避免被先前的 Shipping 选择覆盖。
3. 本工程只有一个 Game 目标，Windows 菜单可能不显示 Build Target；这是正常的。如显示多个目标，选择 **EmberTrial**。
4. 点击 **Platforms → Windows → Package Project**。
5. 选择已准备的输出目录：

   ```text
   E:\UE_Project\Demo\Artifacts\Builds\M0_06\Attempt01
   ```

6. 等待右下角任务完成；点击 **Show Log** 可查看过程，也可打开 **Window → Developer Tools → Output Log**。
7. 成功依据：编辑器报告 Packaging Complete，日志末尾有 `BUILD SUCCESSFUL` / `AutomationTool exiting with ExitCode=0` 等完成信息，且输出目录存在可执行文件。不要把出现中间编译成功或文件夹已生成单独当成整个打包成功。

本机 UE 5.5 的入口为顶部 Platforms；网上旧版本的 File → Package Project 路径可能不同。如 Windows 不可用、提示 SDK / 工具链问题或按钮为灰色，按第 7 节反馈，不先清缓存、重装工具链或换引擎。

首次 Cook 可能长时间处理着色器；日志仍在输出时继续等待。出现失败就保留日志和本次目录，修正后使用 Attempt02 等新目录，便于区分结果。

## 5. 离开编辑器运行

### 5.1 找到实际程序

1. 打包成功后 Save All，然后**关闭 EmberTrial 的 UE 编辑器**。
2. 在资源管理器打开 Attempt01。通常输出结构为：

   ```text
   Attempt01\Windows\EmberTrial.exe
   Attempt01\Windows\EmberTrial\Binaries\Win64\...
   Attempt01\Windows\EmberTrial\Content\Paks\...
   Attempt01\Windows\Engine\...
   ```

3. 找到输出根目录中的 **EmberTrial.exe**。实际结构如与示例不同，以你找到的程序为准，反馈准确路径。

试玩需要保留整个 Windows 输出目录及其相对结构，不能只拿一个 exe。此次只在本机验证；后续交付才安排另一台机器测试。

### 5.2 带独立日志启动

按 Win 键搜索 **PowerShell** 并打开，粘贴执行下面这条命令；路径如不同，只修改第一段 exe 路径：

```powershell
& 'E:\UE_Project\Demo\Artifacts\Builds\M0_06\Attempt01\Windows\EmberTrial.exe' -windowed -ResX=1280 -ResY=720 -log '-abslog=E:\UE_Project\Demo\Artifacts\M0_06\EmberTrial_Standalone.log'
```

这会启动 1280×720 的窗口游戏，并将日志写到确定位置。日志窗口出现正常，操作焦点放在游戏窗口。`-abslog` 已按本机引擎的绝对日志路径参数核对。

确认打开的是游戏窗口，**没有 UE 编辑器的工具栏和 Content Browser**。第一次着色器 / 加载预热可能较慢；黑屏不恢复、弹窗或闪退时保留现象并反馈，不反复重启覆盖证据。

## 6. 包内最小回归

按以下顺序做一次，约 5–10 分钟即可。1 / 2 / 3 仍为上排数字调试输入；1 要站定，空中拒绝三键仍是既定行为。

| 项目 | 操作 | 通过标准 |
|---|---|---|
| 首次启动 | 看地图与人物 | ThirdPersonMap 正确，角色、材质、模板场景正常，无资源缺失弹窗 |
| 基础输入 | WASD、镜头、Space 跳跃后落地 | 能移动 / 转向 / 跳跃，无持续卡键 |
| 完整动作 | 站定分别按 1、2、3，每次等结束 | 三段动画齐全，回到待机；2 向角色自身右侧位移，结束无漂移 |
| 实际打断 | 按 2 后约 0.1 秒按 3；没赶上可重试一次 | 已开始的闪避中断，受击后恢复控制 |
| 攻击中暂停 | 站定按 1 后轻按并松 P；等 2 秒，鼠标点继续 | 世界 / 动作暂停，恢复后接着结束，无卡键；关闭后无需点击视口即可操作 |
| 菜单键盘 / 门控 | 再打开 P 菜单，菜单内按 E / 1 / 2 / 3，再用 Enter 或 Space 继续；背景点击后也试一次 P 关闭 | 菜单期间不执行游戏动作，恢复后不补播 / 不补执行 E；焦点与 P 返回正常 |
| A / B 与中文 | 走近 A、按 E、离开，再对 B 重复 | 中文提示可读，首次激活立即更新，离开隐藏；暂停菜单中文也可读 |
| 最后恢复 | 移动、转向、跳跃，松键站定 | 所有控制恢复，无提示残留或漂移 |
| 退出和第二次启动 | 用 Alt+F4 退出游戏，编辑器保持关闭；用相同命令再开一次，检查 A / B 后退出 | 两次都正常启动；目标恢复未激活状态，动作从 Idle 开始 |

第一次退出后，先用资源管理器把 `Artifacts\M0_06\EmberTrial_Standalone.log` **复制**为 `EmberTrial_Standalone_First.log`，再启动第二次，保留两轮证据。当前还没有存档功能，重开时练习目标状态重置是预期结果。

完成后保存工程配置；本次地图 / 输入不需要临时修改。如果改过帧率或测试设置，恢复再保存。

## 7. 失败时怎么告诉我

仍采用**文字日志优先、界面 / 连线用截图、时序 / 动作用短视频**。

- **编译 / Cook / Stage 失败：**告诉我卡在哪一阶段和大致发生时间；Output Log 中第一条实际错误及其前后 20–40 行可直接复制。仅最后的 `Unknown Error` 通常不足以定位。若日志很长，给完整文件路径，我来读取，不需要你摘抄全文。
- **设置找不到 / 窗口弹错：**给对应窗口截图，附一行操作入口。
- **包内黑屏 / 闪退 / 无输入：**给 exe 完整路径、启动方式、发生时间及日志路径；有报错弹窗加截图。
- **UI / 字体 / 动画异常：**说明包内与 PIE 的具体差别；静态问题截图，动态问题给 5–10 秒视频及保存路径。

UE 编辑器日志在 `E:\UE_Project\Demo\EmberTrial\Saved\Logs`。UAT 常见日志目录为 `%APPDATA%\Unreal Engine\AutomationTool\Logs`，UBT 常见为 `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`；**优先使用 Output Log 中打印的实际 Log file 路径**。我不会在没有运行结果时把修改建议记成修复成功。

## 8. 最简反馈与退出条件

```text
M0-06 完成 / 卡在第 __ 节
Windows Development 打包：成功 / 失败阶段
实际 exe 完整路径：
UE 编辑器关闭后独立运行：是 / 异常
角色 / 场景 / 三段动画：正常 / 异常
1/2/3、闪避打断、E-A/B、中文 UI：通过 / 异常
P 暂停 / 鼠标和键盘继续 / 无补播不卡键：通过 / 异常
最后移动跳跃、退出与第二次启动 / 状态重置：通过 / 异常
两次运行日志已保留：是 / 实际日志路径
配置已保存，临时设置已恢复：是 / 本次没有临时设置
大致完成时间：今天 __:__，日志由你读取
实际手动用时：约 __ 分钟；构建 / Cook 等待：约 __ 分钟
```

**2026-10-01 收尾：**M0-06 已验收，[M0-07 校准](Reports/M0_07_Calibration.md) 也已完成：每周 30 小时、暂无硬期限、总资源预算约 100 元、键鼠交付。M0 整体完成，下一批是 [M1-01 正式纵向地图](M1_01_VerticalSliceMap.md)。以后重复启动请使用不同日志名，例如 `EmberTrial_Run01.log` / `EmberTrial_Run02.log`，每轮退出后再开下一轮，避免覆盖过程证据。
