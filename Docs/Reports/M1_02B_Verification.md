# M1-02B：Framework 对象与角色公开边界验收

日期：2026-10-02。结论：**M1-02B 通过**。角色迁移 / 导出、正式 GameMode / Controller、新地图装配、双目标构建与原功能回归已验收。M1-02 总任务继续进行，UI 生命周期、ASC 和正式 HUD 尚未实现。

## 用户实机结果

用户约 **22:00** 完成，手动 **50 min**、构建 / 索引等待 **15 min**，确认：

- 角色 .h / .cpp 移动到 Public / Private，原位置无副本，类名和既有功能保留；API、include 和私有搜索路径已调整。
- 新 GameMode / Controller 四个源码文件已保存；Editor Build / 索引刷新 / Game Build 成功。
- CLion 新文件与公开头文件导航通过；此前配置问题已解决、功能正常，**无需另发配置反馈报告**。
- 两个 Blueprint 父类 / Classes / 新地图 Override 正确；玩家蓝图 Compile，通过单个 PracticeAction 检查。
- 新地图三轮 Play、Framework 日志及原有功能通过；旧 ThirdPersonMap 回归通过，已回新地图保存。

## 源码与构建的直接证据

| 检查 | 结果 |
|---|---|
| 角色迁移 | Public/Characters/EmberTrialCharacter.h 与 Private/Characters/EmberTrialCharacter.cpp 存在；旧路径无重复文件 |
| 角色声明 | 保留 AEmberTrialCharacter，已加 EMBERTRIAL_API |
| 原行为 | 移动前 HEAD 与当前角色 .cpp 去除 include / 空白后内容相同；本批未另建第二套角色 |
| 核心依赖 | PrivateIncludePaths.Add(ModuleDirectory) 已加入；核心未依赖 EmberTrialUI / UMG |
| 正式对象 | 四个新类文件忽略空白后与 M1-02B 指南一致；日志覆盖 GameMode StartPlay、Controller BeginPlay / Possess / UnPossess / EndPlay |
| Editor receipt | EmberTrialEditor.target，21:54:13；UE 5.5.4 / Win64 / Development / Editor，两个 Runtime 模块 |
| Game receipt | EmberTrial.target，21:55:22；UE 5.5.4 / Win64 / Development / Game |
| Game 编译输出 | 19 项动作，包括 UI 模块、新对象及 UHT 生成文件、角色和原模块、链接 exe、写 receipt；UBT 共 50.37 s |
| 后续 Editor 输出 | 21:55:31，Generated code is up to date / Target is up to date，0.52 s；不记成另一轮完整编译 |
| CLion 模型 | 主 CMake 为 3.20 / C++20，保留双 CodeModel 与真实 UBT 目标 |
| 编译数据库 | 共 10 个 .cpp 入口；两个 Framework .cpp 和迁移后的 Character 属于 EmberTrialCodeModel，UI .cpp 属于 EmberTrialUICodeModel |
| 新类生成头文件 | Editor / Game 的 UHT 生成记录与编译产物支持新反射类已正常构建 |

当前角色 .cpp 的头文件使用 `../../Public/Characters/EmberTrialCharacter.h`，旧 GameMode 使用 `Public/Characters/EmberTrialCharacter.h`，练习组件使用相对路径；与指南推荐的模块内搜索路径写法不同。实际构建 / 导航通过，本次按实际源码记录；后续整理可以统一为 `Characters/EmberTrialCharacter.h` 和 `ETPracticeActionComponent.h`，不把已通过的本批回归判为失败。

## 运行日志与保存证据

现存 `EmberTrial.log` 最后写入 **22:02:05**。下面时间已从 UTC 换为本机 UTC+8。

| 本机时间 | 现存日志事件 |
|---|---|
| 21:56:01 | EmberTrialUI module ready，一条 |
| 21:59:32 | 新地图 PIE；Game class = BP_ETGameMode_C |
| 21:59:32 | Possess：BP_ETPlayerController_C_0 → BP_ThirdPersonCharacter_C_0 |
| 21:59:32 | Controller BeginPlay，Local=1；单个玩家 PracticeAction ready / Idle |
| 21:59:32 | Rules ready：GameMode / ControllerClass / PawnClass 均为本批指定 Blueprint 类 |
| 22:00:15 | UnPossess，CurrentPawn=None；Controller EndPlay / Reason=0，然后世界清理 |
| 22:00:28–31 | ThirdPersonMap PIE，沿用模板 EmberTrialGameMode，PracticeAction 初始化 |
| 22:00:33–34 | 第二轮旧地图 PIE / 清理 |
| 22:00:40 | 回到 L_ET_VerticalSlice |
| 22:02:02 | UI module shutdown |

最新及前三个备份日志中，直接找到 **一轮新地图的完整 Framework 记录**；其他两轮新地图和原功能的完整操作矩阵按用户实机反馈验收。当前样本未包含动作 / E / P 的详细事件，不由 Framework 日志推断这些操作；无需补跑或摘录正常日志。

未匹配到 Blueprint Runtime Error、Accessed None、LogBlueprint / LogScript Error、Fatal error 或 Assertion failed。此结论只描述现存样本。

| 资产 | 最后保存时间 | 核对方式 |
|---|---|---|
| BP_ETPlayerController | 21:57:16 | 磁盘文件 / hash；Blueprint 父类按用户反馈，运行实例支持实际使用 |
| BP_ETGameMode | 21:58:04 | 磁盘文件 / hash；运行 Classes 与用户配置一致 |
| BP_ThirdPersonCharacter | 21:58:58 | 磁盘文件 / hash；编译和单组件检查按用户反馈 |
| L_ET_VerticalSlice | 22:00:10 | 磁盘文件 / hash；新地图 Override 在实际运行中生效 |

保存时间早于最后一轮旧图回归，日志随后有返回新地图；最终 Save All 按用户反馈确认。资产清单更新至 **18 项**，新增两个原创 Framework Blueprint，刷新本批改变的地图 / 玩家蓝图 hash。

原始运行片段：`E:\UE_Project\Demo\Docs\Reports\Raw\M1_02B_Acceptance_2026-10-02.log`。Game 编译来源：本机 UBT 的 `Log-backup-2024.11.10-07.57.41.txt`，内容实际开始于 2026-10-02 21:54:32；备份名中的旧日期不作为构建日期。

本轮助手只读取源码 / 日志 / 构建产物并更新文档及清单，没有改 C++、资产、引擎 / IDE 配置，没有重跑 Build / PIE / 自动测试。

## 工时与交接

- 手动：723 + 50 = **773 min（12 h 53 min）**。
- 已报告等待：31 + 15 = **46 min**；未报告投入和助手排障不自行补算。
- 下一批：[M1-02C1：本地 UI 生命周期与 Pawn 订阅](../M1_02C1_LocalPlayerLifecycle.md)。代码在文档，用户手动实现；UI Widget 装配在 C2，GameplayReady / HUD 数据绑定在 M1-03。

