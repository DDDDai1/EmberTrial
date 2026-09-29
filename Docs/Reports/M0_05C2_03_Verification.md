# M0-05C2-03：闪避参数与位移接入记录

更新：2026-09-29。**C2-03A / B 已验收通过。B 首轮实现 50 分钟、补修 30 分钟；源码保护、扩展后的 Lifecycle、150.0 cm / SourceTime=0.300 和约 119 FPS 样本已核实。C2-03C 真实移动专项测试仅下发手动指南，未实现 / 构建 / 执行，C2-03 总任务尚未完成。** 历史审查保留，最终结论见第 7 节。

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

## 6. 保护补修期间的编译排障

用户已写入两项保护、SourceTime 和测试用例，Build 报 State 不是类或命名空间、Dodging 未声明，以及 bStarted / TestFalse 的连带错误；用户明确授权直接解决当前问题，但保留第 5 节 Build 和后续验证自行执行。

首次读取确认新增测试位于第一次 Walking 设置后，使用 State 的位置早于局部别名声明。后续读取磁盘确认顺序已调整：第 68 行声明 State，第 82 行开始保护测试，位于空中拒绝循环后的第二次 Walking 之后。静态比对确认新增测试与指南一致，原有全部测试保留；组件保护块和 SourceTime 日志也与指南一致。助手未运行 Build / 自动回归，不据此声称编译成功。

当前从补修指南第 5 节继续，无须再次粘贴第 2–4 节；本次授权不改变后续 C++ 默认由用户手动编写的约定。

## 7. B 最终验收与 C 交接

用户补修约 **30 分钟**：确认正常 Build 成功，Lifecycle Success，8 条 InvalidDodgeConfig / 1 条 ExternalRootMotion，正常闪避、2→3 打断和位移中 P 暂停恢复正常，120 上限时实际约 119 FPS；源码已保存、设置已恢复。用户无需再手工整理完整运行日志，助手直接读取项目 Saved/Logs。

源码复核：保护位于 Montage_Play 前；SourceTime 在 Reset 前读取；新增保护测试位于 State 声明和空中拒绝测试之后，旧测试完整保留。新日志来自 `EmberTrial/Saved/Logs/EmberTrial.log`：

| 时间（日志 UTC，显示时区 +8 小时） | 直接证据 |
|---|---|
| 13:13:52.907 | 正好 8 条 InvalidDodgeConfig、1 条 ExternalRootMotion |
| 13:13:53.161 | `Test Completed. Result={Success}`，`EmberTrial.M0.PracticeAction.Lifecycle` |
| 13:15:13.311 | t.MaxFPS 设为 120 |
| 13:15:31.454，帧 548 | Begin：150.0 cm / 0.300 s，方向 Y=1 |
| 13:15:31.749，帧 583 | End：DistanceWindowEnded，Travel2D=150.0，Elapsed=0.295，SourceTime=0.300 |
| 13:17:04.868 | t.MaxFPS 恢复 0 |

35 个帧间隔 / 0.295 秒约为 **118.64 次 / 秒**，与用户约 119 FPS 的观察相符，补齐前次 120 上限样本的证据。它是本次动作窗口的平均更新节奏，不是长期性能或逐帧稳定性测量。SourceTime 直接确认位移源累计 0.300 秒；Elapsed 稍短不再被当成提前结束故障。

本次新日志的正常实机位移样本是一组；不额外虚构第二组普通平地、实机打断或暂停日志。后两项以用户确认正常为据。Lifecycle 中 Travel2D=0、SourceTime=0 的即时打断属于关闭移动 Tick 的测试环境，不能作为实机位移证据。正常 Build 成功按用户反馈记录；读取本机 UBT 日志受权限限制，未据此新增独立构建结论。

筛选后的原始证据已保存到忽略目录 `Docs/Reports/Raw/M0_05C2_03B_Acceptance_2026-09-29.log`。B 验收通过，50 + 30 = **80 分钟**；全部已登记手动用时 **610 分钟（10 小时 10 分钟）**。

下一步为 [C2-03C 位移专项自动回归](../M0_05C2_03C_DodgeAutomation.md)：临时平地、真实移动 Tick、三种固定步长、打断与暂停续行。只下发文档代码，助手没有新增工程测试文件、修改头文件、运行 Build 或测试。固定步长测试不替代真实帧率 / 墙壁 / 坡面 / UI 实机验证。C 完成后进入 C3 组合演示，再进入 M0-06 首次打包。
