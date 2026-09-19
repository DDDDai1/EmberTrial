# M0：环境验证与第一批手动操作

更新：2026-09-19。用户已创建工程并确认构建 / 移动 / 转向 / 跳跃正常，检查发现实际为 UE 5.4.4。现已按用户选择完成 UE 5.5.4 配置切换和 C++ 编译，等待新版本交互复验。本文保留最初操作说明，不要求重复创建工程；当前进展以 Progress 与迁移记录为准。

## 1. 本轮分工

你来创建工程、进行首次构建和运行第三人称模板。我负责检查配置、读取日志、处理复杂故障、复核结果，并安排后续基础 C++ / 蓝图练习。

本轮只完成“创建 → 冷构建 → 运行”，不要求同时搭建 GAS、UI 架构或导入美术。遇到报错停在现场即可，不必先尝试大量修复。

## 2. 已核实环境

| 项目 | 本机信息 | 当前结论 |
|---|---|---|
| 引擎 | UE 5.5.4，`D:\UE5\UE_5.5` | Editor、Build.bat、Third Person C++ 模板存在 |
| IDE | Visual Studio Community 2022 17.14.20，`D:\visual studio 2022` | 配套工具链已用于本项目成功构建 |
| 编译器 | MSVC 安装目录 14.38.33130、14.44.35207 | 5.5.4 成功构建实际使用 14.38.33145，位于 14.38.33130 目录 |
| Windows SDK | 10.0.22621.0、10.0.26100.0 | 5.5.4 成功构建实际使用 10.0.22621.0 |
| UBT 用户配置 | 已读取 Roaming 目录下 BuildConfiguration.xml | 只有空 Configuration 节点，没有显式编译器覆盖；本轮未修改 |
| CPU | AMD Ryzen 7 7840H | 已核实型号 |
| GPU | NVIDIA GeForce RTX 4060 Laptop GPU；另有 AMD Radeon 780M | 已核实设备；UE 实际选择哪个 GPU 待项目日志确认 |
| RAM / 显存预算 | 运行日志报告物理内存总量约 15554 MB；显存预算待定 | 不从 GPU 型号推断功耗或实际性能 |
| 版本管理 | Git 2.49.0.windows.1、Git LFS 3.6.1 | 本地仓库已建立；迁移前检查点 3562f61；无远端 |
| 可用空间快照 | E 盘约 101 GB，C / D 盘约 60 GB | 当前可开始小型模板工程，后续持续观察缓存与构建占用 |

本地引擎的 `Engine/Config/Windows_SDK.json` 已核对：优先 MSVC 14.38.33130–14.38.99999。IDE 版本和 MSVC 工具链版本是两件事，暂不降级 VS 或卸载 14.44。

本表已有 5.5.4 构建证据；之前的运行和独显选择证据来自 5.4.4。5.5.4 运行仍待复验，打包尚未执行。

## 3. 工作区与工程路径

```text
E:\UE_Project\Demo\                      ← 工作区 / 后续 Git 仓库根
    Introduction.md
    Docs\
    EmberTrial\                          ← 本轮由你创建的 UE 工程
        EmberTrial.uproject
        Source\
        Config\
        Content\
```

项目向导会将“项目位置 + 项目名称”组合为工程文件夹。位置填 `E:\UE_Project\Demo`，名称填 `EmberTrial`，最终应得到：

`E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`

不要预先在“项目位置”中再次填 `EmberTrial`，以免多生成一层同名文件夹。如果创建后路径不同，告诉我实际 `.uproject` 路径，由我核对，不需要自行移动目录。

## 4. 你现在执行的步骤

### M0-01A：创建工程

1. 启动 **UE 5.5.4**。可从 Epic Games Launcher 选择 5.5，或手动运行 `D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe`。若自动进入其他工程，用“文件 / File → 新建项目 / New Project”打开向导。
2. 选择“游戏 / Games → 第三人称 / Third Person”。
3. 使用下面的设置。某项未显示时不用寻找隐藏选项，记录当前界面即可。

| 选项 | 本轮值 |
|---|---|
| 项目类型 / Implementation | **C++** |
| 目标平台 / Target Platform | Desktop / 桌面 |
| 质量预设 / Quality Preset | Scalable / 可缩放，先验证开发与运行流程 |
| Starter Content / 初学者内容包 | 关闭；第三人称模板角色与该选项不是一回事 |
| Ray Tracing / 光线追踪（如果显示） | 关闭 |
| 项目位置 / Location | `E:\UE_Project\Demo` |
| 项目名称 / Name | **EmberTrial** |

4. 点击“创建 / Create”，等待生成和首次编译。首次处理着色器耗时较长并不自动代表卡死；若输出持续更新，可继续等待。
5. 成功后核对 `.uproject` 路径和 `Source` 目录存在。不要手动重命名模板生成的模块或类，下一步我会读取实际结构。

如果提示“工程已创建但编译失败”，不要删除或反复新建工程；保留弹窗文字 / 日志，把实际路径给我。即使编辑器打不开，我也可以检查生成的代码和构建记录。

步骤依据为 [UE 5.5 创建项目文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-a-new-project-in-unreal-engine?application_version=5.5)，具体选项是本项目的 M0 基线。

### M0-01B：进行一次编辑器关闭状态的构建

1. 工程成功生成后，保存并关闭 Unreal Editor。这次用正常构建确认工具链，不依赖 Live Coding。
2. 打开工程生成的 `EmberTrial.sln`。若没有 `.sln`，先记录现象；也可尝试右键 `.uproject` → “Generate Visual Studio project files / 生成 Visual Studio 项目文件”。若没有此菜单，不要修改注册表，把情况交给我。
3. Visual Studio 工具栏选择 **Development Editor** 和 **Win64**。
4. 在解决方案资源管理器中找到游戏项目 **EmberTrial**，右键选择 **生成 / Build**。本轮不需要重建整个引擎或整个解决方案。
5. 打开“视图 / View → 输出 / Output”，将输出来源选为“生成 / Build”。确认最终构建是否成功。
6. 记录此次输出中的 `Using Visual Studio ... toolchain` / SDK 行，以及最终成功摘要。若出现 error，保留第一个实际错误及其上下文。

如果显示“已是最新”，说明这次没有重新编译所有文件；连同首次创建时的日志一起交给我核对，不把它单独当作工具链完整验证。不要为触发编译随意更改引擎文件。

构建目标含义参考 [UE 5.5 编译工程文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/compiling-game-projects-in-unreal-engine-using-cplusplus?application_version=5.5)。

### M0-01C：验证模板运行

1. 双击 `.uproject` 打开工程，等待资源和着色器加载完成。
2. 在默认第三人称地图点击工具栏 **Play / 运行**。
3. 点击视口使其获得输入焦点，验证 **WASD 移动、鼠标转向、空格跳跃**。
4. 停止运行，再次 Play，确认可重复进入，没有明显错误弹窗。
5. 保存后告诉我实际结果；不需要为了验收截图所有窗口。

本轮先不更换角色、不导入大量素材、不创建自定义 Gameplay 类。我检查基线后再给你下一张小任务卡。

## 5. 这一步怎样算完成

- [ ] `.uproject` 位于预期路径，工程确实为 C++ 模板。
- [ ] 首次实际 C++ 编译成功；记录使用的 MSVC 和 SDK。
- [ ] 关闭编辑器后可正常 Build，再打开工程。
- [ ] 模板角色可移动、转向、跳跃，第二次 Play 也正常。
- [ ] 我检查项目配置、生成代码和日志后，将证据补到 Progress。

**完成以上只是 M0 的第一批任务，不代表 M0 全部完成。**

## 6. 完成或报错时怎么回复

成功时直接发送：

```text
M0-01 完成
工程路径：E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject
编辑器关闭后 Build：成功 / 已是最新 / 其他
Play：移动、转向、跳跃正常 / 具体异常
实际用时：约 __ 分钟
```

发生问题时，文字日志优先。你只要发“卡在 M0-01B，Build 报错，工程已保存，请你直接检查”，并附发生时间 / 日志路径，就足以让我开始本地排查。日志文字可复制则附上，不要求你先判断原因。

需要截图或视频的场景与完整模板见 `E:\UE_Project\Demo\Docs\IssueTemplate.md`。

## 7. 本批之后的 M0 安排

1. 我复核工程、真正使用的编译器 / SDK、GPU 选择和目录结构；建立 Git / LFS 规则，再保存可恢复基线。
2. 安排你创建一个简单 C++ Actor 与它的蓝图子类，通过属性暴露、组件和日志练习完整工作流；具体名字、父类和代码在下一批任务中给出。
3. 用简单蓝图 / UMG 验证交互提示和菜单开关；复杂的焦点 / 生命周期问题由我接手。
4. 以模板 / 占位资源测试移动、攻击、闪避、受击，并完成首次 Windows Development 打包。
5. 记录实际工时、资源与风险，决定进入 M1。

待补充的信息包括每周可投入小时、投递时间、是否有手柄和 RAM 容量；它们不阻止你现在创建工程。重要模型的 Blender 设计留在中后期，M0 不需要现在安装或连接 Blender。
