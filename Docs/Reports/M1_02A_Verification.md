# M1-02A：Runtime 模块与 CLion 索引验收

日期：2026-10-02。结论：**M1-02A 通过**。UI 模块入口、双模块构建 / 索引与已有玩法冒烟可用；整个 M1-02 的正式对象与 UI 生命周期仍在后续分批实现。

## 用户实机结果

用户约 20:55 完成，手动 **30 min**、构建 / 索引等待 **5 min**，确认：

- 两个新模块文件、两个 Target、.uproject、刷新脚本均已按文档保存。
- Editor Build / 索引刷新、Game Development Build 成功；CLion 两个 CodeModel、新文件与头文件导航通过。
- 本次编辑器启动一条 EmberTrialUI module ready；新地图两次 Play 的移动、1/2/3、2→3、E/P 通过。
- 源码与资产已保存。

导航问题在光标移入 include 文件名后解决；这是用户已确认的操作定位问题，未修改模块代码、引擎搜索路径或清除缓存。**不需要另发导航反馈报告。**

## 直接核对的证据

| 检查 | 结果 |
|---|---|
| UI 源码 | Build.cs / EmberTrialUIModule.cpp 正确；.cpp 忽略空白后与指南一致 |
| 依赖与注册 | UI 私有依赖 Core / EmberTrial；核心不依赖 UI；两个 Target 与 .uproject 均包含两个 Runtime 模块 |
| Editor receipt | EmberTrialEditor.target，20:36:50；UE 5.5.4 / Win64 / Development / Editor，包含 UI DLL / PDB |
| Game receipt | EmberTrial.target，20:42:08；UE 5.5.4 / Win64 / Development / Game |
| Game 构建日志 | 三项动作：编译 UI 模块、链接 EmberTrial.exe、写 receipt；UBT 共 33.18 s |
| 最新 Editor 日志 | Target is up to date，0.50 s；不记为第二次全量编译 |
| CLion 模型 | 两个 CodeModel；compile_commands.json 共 8 个源文件入口，其中 UI .cpp 1 个 |
| UI 编译上下文 | 包含正确 Core/Public 和本模块 Definitions 头文件 |

UE 运行日志最新仍是 **20:45:11** 保存的文件；20:55 是用户完成反馈 / 导航复验的大致时间，不冒充该时刻另有一轮 PIE。

| 本机 UTC+8 时间 | 现存日志事件 |
|---|---|
| 20:43:02 | EmberTrialUI module ready，一条 |
| 20:44:09–10 | 新地图 PIE；PracticeAction 初始化为 Idle |
| 20:44:10–16 | 攻击 / 闪避 / 受击开始与结束；三次位移分别 150.1 / 150.0 / 150.1 cm，SourceTime=0.300 s；一次 HitInterrupt |
| 20:44:19 / 21 | M1_Target_A / B 激活 |
| 20:44:24 | Pause opened，然后停止 PIE |
| 20:45:08 | EmberTrialUI module shutdown |

这一样本的 HitInterrupt 发生在位移窗口结束后、Montage 仍在播放时，不把它记成“位移中打断”。当前日志直接保留一次 PIE，且暂停后直接 Stop，没有 Resume 记录；两次 Play、恢复输入与具体视觉结果按用户反馈验收，不要求补跑。

未匹配到 Blueprint Runtime Error、Accessed None、LogBlueprint / LogScript Error、Fatal error 或 Assertion failed。其他编辑器 / 在线服务警告不等同于本批玩法错误。

原始片段：`E:\UE_Project\Demo\Docs\Reports\Raw\M1_02A_Acceptance_2026-10-02.log`。构建源：本机 UBT 的 Log.txt 与现存 Log-backup-2024.11.10-07.57.41.txt；备份文件名不是实际构建日期，内容 / 最后写入对应 2026-10-02。

## 工时与交接

- 已登记手动：693 + 30 = **723 min（12 h 03 min）**。
- 已明确报告的等待：26 + 5 = **31 min**；未报告投入不自行补算。
- 下一批：[M1-02B：Framework 对象与公开边界](../M1_02B_FrameworkObjects.md)。代码只写在文档，由用户手动实现。
