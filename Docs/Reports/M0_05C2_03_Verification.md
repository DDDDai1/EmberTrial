# M0-05C2-03：闪避参数与位移接入记录

更新：2026-09-29。**C2-03A 已通过；C2-03B 已实现，用户正常场景与旧 Lifecycle 通过，实际约 50 分钟。源码缺少两项准入保护，120 上限样本待复核，因此 B 暂记待补修，不标总任务完成。**

## 1. A：用户已完成

用户约 **10 分钟**：在 ETPracticeActionComponent.h 的 private 区域加入两个参数、原代码保留；关闭 UE 后正常 Build 成功；PracticeAction 可见 Dodge Distance / Duration，最终 150 cm / 0.30 s；原 Montage / Auto Activate 正确，蓝图编译通过；1/2/3 原地播放、受击打断与恢复、E/P 和基础输入正常；源码 / 资产已保存。

助手直接核对：

- 源码只增加两组 UPROPERTY 与 float，位置在 AttackMaxHorizontalSpeed 后、CurrentState 前，位于 private 区域。
- 距离 EditDefaultsOnly、范围 1–300、Units=cm、默认 150；持续时间 EditDefaultsOnly、范围 0.05–0.50、Units=s、默认 0.30，与指南一致。
- 最新 UBT 日志末尾为 `Target is up to date`、总耗时 1.57 秒。这是最新增量检查，不将它单独描述为一次全量重编译；正常 Build 成功以用户结果为据。
- 当前 EmberTrial.log 未保留本批的 Component ready / Begin 等 PIE 片段，未匹配到相关蓝图运行错误；不把“无日志片段”当成实机通过证据。具体功能结果采用用户反馈。
- 本轮 Git 中玩家蓝图未新增二进制差异，只有头文件和原有 DefaultEditor.ini 改动；继承新 C++ 默认值不必然产生蓝图磁盘差异，不因此否定用户的保存结果。

## 2. B：2026-09-28 交接时的实现边界

用户最新要求：**后续所有 C++ 编写 / 修改均输出到文档，由其手动操作，直到明确要求助手直接操作。** 本约定已写入 Introduction v1.2、Progress 与 IssueTemplate。

已下发 [C2-03B 手动接入指南](../M0_05C2_03B_DodgeMovement.md)：四个私有辅助函数、位移源所有权、请求校验、统一退出清理和 Montage 淡出中断。保持 A 的两个配置，目标为自身右侧 150 cm / 0.30 秒，位移窗口与 Montage 状态时长分开。

本轮只读取了本机 5.5.4 的相关引擎声明 / 实现，核对 Apply / Remove / GetByID、部分结束帧、Finished 标记、ClampVelocity 与淡出委托签名；**助手没有修改工程 C++ 或 UE 资产，没有构建新代码，没有运行新位移测试。**

## 3. B：2026-09-29 用户反馈与日志复核

用户约 **50 分钟**：正常 Build 与蓝图 Compile 通过，150 cm / 0.30 s 和资源 / Auto Activate 正确；自身右侧、方向固定、先停位移后解锁、无漂移、输入门控、五次受击打断、暂停 / 无补播 / 无卡键、墙壁 / 斜墙 / 上下坡、三次 PIE、原功能全部通过，已保存并恢复设置。Lifecycle 报告 Success。

日志证据分两份：

- `EmberTrial/Saved/Logs/EmberTrial-backup-2026.09.28-12.49.15.log` 包含 150.0 / 150.1 cm、13.7–143.0 cm 等位移样本、66.7 cm 时的 HitInterrupt，以及 UTC 2026-09-28 12:48:49.553 的 `Test Completed. Result={Success}`，路径为 `EmberTrial.M0.PracticeAction.Lifecycle`。短距离的具体碰撞场景和暂停行为仍以用户反馈为据，日志本身不证明画面。
- `EmberTrial/Saved/Logs/EmberTrial.log` 的 2026-09-29 片段支持如下距离；完整操作次数、坡面外观和输入行为按用户实机反馈记录，不用日志扩大结论。

| 设置 / 样本 | Travel2D | Elapsed | 帧编号 | 可支持的结论 |
|---|---|---|---|---|
| 普通平地 | 150.0 cm | 0.288 s | 437→454 | 完整距离，位移先结束、动画后 Completed |
| 上限 30 | 150.1 cm | 0.267 s | 816→824 | 与约 30 次更新 / 秒相符，距离在目标范围 |
| 上限 60 | 150.1 cm | 0.283 s | 411→428 | 与约 60 次更新 / 秒相符，距离在目标范围 |
| 上限 120 | 150.0 cm | 0.253 s | 993→998 | 距离通过，但仅约 5 个帧间隔，不能据此确认位移期间稳定 120 FPS |

用户观察约 120 FPS 与该次日志样本不一致；原因尚未确定。保留用户反馈，同时要求只补一组位移期间的实际 FPS 与完整日志。不能把上限设置当成实际帧率。

## 4. 源码审查与计时说明

按原指南逐函数对比，忽略空白 / 注释：ReleaseAction、TickComponent、StartDodgeMovement、StopDodgeMovement、UpdateDodgeMovement、HandleMontageBlendingOut 六个函数一致。RequestPracticeAction 缺少整个 `RequestedState == Dodging` 校验块，未出现 `InvalidDodgeConfig` / `ExternalRootMotion`。

风险是异常参数会进入距离 / 时长除法，已有 Root Motion Source 时也可能叠加新闪避。正常参数下的试玩与原 Lifecycle 不覆盖这两项，因此它们通过与源码遗漏并不矛盾。

本机源码依据：`LevelTick.cpp` 在世界 Tick 前部累加 TimeSeconds；`RootMotionSource.cpp` 按移动更新的 SimulationTime 推进位移源时间，并对最后不足一帧的 SimulationTime 做处理。由此推断 Elapsed 的世界时间采样差可能少计起步帧，与 30 / 60 样本相符；它不是源的累计模拟时间，也不能单独用来判定提前结束。

已在文档提供额外 `SourceTime = DodgeSource->GetTime()` 日志，待用户写入后直接确认；不改变 0.30 秒配置，不预先宣称诊断已实测。

## 5. 当前补修与后续

按 [保护补修与针对性回归](../M0_05C2_03B_GuardCorrection.md) 完成：补回保护块、增加 SourceTime、扩展原 Lifecycle 的八组非法配置及外部位移源用例、正常 Build / 自动回归、一次正常 / 打断 / 暂停检查和 120 上限短复测。旧实机矩阵保留，不要求整套重做。

本轮助手只读源码和日志、更新文档，未修改 C++ / UE 资产、未编译或执行新增测试。用户源文件保存为补修前检查点；补修后再复核 B，实际移动专项自动测试 / C3 / 打包继续后续安排。
