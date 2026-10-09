# M1-02C1：本地 UI 生命周期与 Pawn 订阅验收

日期：2026-10-03。结论：**M1-02C1 通过**。C2 拆成 Widget / Bootstrap 装配与重建（C2-01）和真正的新 Pawn 替换验证（C2-02）。M1-02 总任务仍进行中。

## 用户实机反馈

用户约 **13:34** 完成，手动 **30 min**、构建 / 索引等待 **4 min**：

- UI Build.cs 与两个 Subsystem 文件手动保存；Editor Build / 索引刷新 / Game Build 成功，CLion UI 目标与引擎导航通过。
- 初始 local ready / Controller / Pawn 正确，4/5 失去 / 恢复与重复按键保护通过；三轮恢复移动 / 转向 / 跳跃。
- 6 销毁清空、销毁后 5 不执行、没有 Accessed None；三次 PIE 初始化 / 清理、下一轮对象通过。
- 临时节点与 ETTempPawn 已删除、Compile / Save；原动作、打断、E/P 与基础输入正常。

## 直接核对的源码 / 构建证据

| 检查 | 结果 |
|---|---|
| 三份实施文件 | UI Build.cs、Subsystem .h / .cpp 去除空白后与指南完全一致 |
| 生命周期 | 原生 PlayerControllerChanged；订阅 Controller Pawn 变化 / EndPlay；初始快照、同值保护、Pawn EndPlay、弱引用及 Deinitialize 清理存在 |
| 依赖边界 | UI Public 依赖 Core / CoreUObject / Engine，私有依赖核心；核心未加入 UI / UMG |
| Editor receipt | 13:20:51，UE 5.5.4 / Win64 / Development / Editor |
| Game receipt | 13:21:31，UE 5.5.4 / Win64 / Development / Game |
| Game 构建日志 | 13:20:56 开始，6 项动作：新类 .gen.cpp / .cpp、UI init / 入口编译、exe 链接、receipt；共 34.64 s，执行器 27.75 s |
| 后续 Editor 日志 | 13:21:45，Target is up to date，0.47 s；不是另一轮完整编译 |
| CLion | CMake 3.20 / 两个 CodeModel 保留；cmake-build-debug/compile_commands.json 共 11 个 cpp，UI 2 个，其中包含新 Subsystem |
| BP_ETPlayerController 保存 | 13:34:13 保存记录与文件时间吻合；SHA256 02D1E52B4D5AAB06CC072CC3299FFB9DA76F248EE1E40A61FD476E454DCC1C95 |

本轮只审查 / 读日志和更新文档 / 清单，未代用户编译、运行、修改源码或 Blueprint。源码共 23 份文件（含 Build.cs / Target）；临时蓝图节点已删除以用户反馈为据，文件保存记录只证明保存发生，不作为图内节点删除的独立证明。

## 日志范围与边界

现存 `EmberTrial/Saved/Logs/EmberTrial.log` 最后写入 **13:34:18**。下表已将 UTC 转为本机 UTC+8。

| 本机时间 | 实际记录 |
|---|---|
| 13:22:39–13:23:13 | LocalPlayer_0：初始化、正确 Controller、出生 Pawn、结束清空、Controller 清理与 Deinitialize |
| 13:32:56–13:33:34 | LocalPlayer_1：初始化；13:32:58 失去控制、13:33:13 恢复、13:33:24 失去、13:33:26 恢复、13:33:29 再失去，随后结束清理 |
| 13:33:35–13:33:42 | LocalPlayer_2：出生与完整结束清理 |
| 13:34:13 | Controller 资产最终保存 |
| 13:34:15–13:34:18 | LocalPlayer_3：最终运行、正确出生与结束清理 |

合计 4 条 ready、4 条 Controller bound、12 条 Pawn changed、4 条 Controller cleared、4 条 deinitialized。没有扫描到 Accessed None、Blueprint Runtime Error、LogBlueprint / LogScript Error、Fatal error、Assertion failed。

**现存日志直接支持两次完整失去 / 恢复，随后一次失去；不覆盖用户报告的全部三轮恢复和按 6 销毁过程。**这些项目及完整原功能矩阵按用户实机反馈验收，无需重跑。当前源码的 PawnEndPlay / 清理路径已审查，但不把未留存的销毁操作写成直接日志证据。实例后缀可复用，区分 PIE 世界 / LocalPlayer，不能仅凭 C_0 判断跨世界残留。

关键原始片段已保留在 `Docs/Reports/Raw/M1_02C1_Acceptance_2026-10-03.log`，构建证据来自本机 UBT Log.txt 与上一轮备份。

## 工时与下一步

累计已报告手动 **803 min（13 h 23 min）**，等待 **50 min**。本次 30 / 4 min 加在先前 773 / 46 min 上，未补算助手工时或未报告等待。

资产来源清单维持 18 项，更新 Controller 的 C1 阶段 / 哈希；C1 没有新增美术资产。下一批按 [M1-02C2-01](../M1_02C2_01_WidgetBootstrap.md) 手动实现状态面板与 Bootstrap；该页代码尚未应用 / 编译。真正的新 Pawn 重绑在 C2-02 验证，ASC / 正式 HUD 在 M1-03。

