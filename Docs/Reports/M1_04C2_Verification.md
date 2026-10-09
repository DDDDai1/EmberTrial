# M1-04C2：死亡与行动封锁核查

实施日期：2026-10-06；收尾：2026-10-07。**M1-04C2验收完成。**用户明确授权助手测试剩余时序项，助手使用Computer Use控制真实PIE，在窗口开启后触发致命伤害，日志直接证明主动关窗 / 取消。已恢复时间速度、玩家100/100与Debug False，Compile→Save并再次PIE确认。没有修改C++、执行Build或新增测试框架；主体其他组沿用既有已通过结果。

## 1. 源码、构建与保存

- Source仍45份，恰好修改指南列出的11份，其余34份与C1基线SHA-256一致；27个C++代码块中24个新代码片段全部匹配实际实现，另外3个是供定位的旧条件，不作为新实现。
- 已核对死亡锁先于取消、独立Dead Tag所有权、Trace主动关闭、归零快照、受击 / 攻击封锁、属性Base / Current钩子的死亡夹限与结束清理。两个CanActivateAbility均有原生IsCombatDead保护，不能只依赖可配置的Blocked Tags。
- Editor receipt21:43:13、Game receipt21:49:01，均UE5.5.4 / Win64 / Development；CLion索引21:47:57更新，22个独立cpp。构建成功采用用户反馈与产物记录，本轮未重编译。
- 清单仍35项，无新增资产；仅玩家Blueprint相对C1改变，其余34项哈希不变。22:10恢复反馈当时未有直接证据；10月7日GUI确认InitialHealth25/100且Draw Debug=True，已在Class Defaults恢复100/100、关闭绘制、编译成功并于19:56:22正常保存。19:56:37新PIE出生100/100、HUD正确，正常攻击窗口0.20秒。3 / 4图未改，4沿用既有Damage25入口。

保存[已接受C2基线](Raw/M1_04C2_Baseline.json)，待查项为空。M1-03B1旧指南保持不变，M0包未更新，Game Build不等于新Cook / 独立包。

## 2. 现有直接证据

实机片段21:52–21:56见[原始记录](Raw/M1_04C2_Acceptance_2026-10-06.log)。用户完成时间填写“30”，未给完整时刻，工时与日志时段分别记录。

- 九轮玩家初始化，18组两身体观察 / 清理。七次Death committed / sealed / tag released：四次HealthZero（目标一次、玩家三次），三次ReadySnapshot。按PIE轮次和身体划分，每个生命周期只提交一次；不能把复用的_C_0实例名跨PIE当作同一个身体。
- 七条Death sealed均TraceTick0 / PracticeIdle1 / MoveMode0 / AnimPaused1；七条Tag释放Remaining0。13组攻击Arm / Disarm最终Tick0，七组Hit begin / end清理完整。
- 目标四次真实25伤害归零，随后四次TargetDead拒绝；玩家不受目标死亡影响据用户实机反馈。
- 玩家致命伤害取消当前受击，HUD保持Ready0/100；后续输入拒绝、P与镜头可用据用户反馈，暂停开关有日志样本。
- 21:53:58闪避被致命伤害取消，Travel2D141.7 / Elapsed0.283 / SourceTime0.283，早于0.300结束，支持真实位移中取消。无残留漂移据用户观察。
- 存活治疗25→65直接进入HUD；三次0/100出生各有一次ReadySnapshot死亡、受击Granted0。死亡治疗Base / Current保持0据用户报告与夹限实现核对，没有画面读取的独立日志。
- 控制台只记录ClearDebugAttributes与`AbilitySystem.DebugAttributes`，未记录指南的`AbilitySystem.DebugAttribute Health`。不把这条不同的命令称为成功显示Current / Base的证明，也不因此要求重跑治疗组；正确命令已在收尾说明提醒。
- 当前日志未匹配到Blueprint Error / Warning、Accessed None、Blueprint Runtime Error、LogScript Error / Warning或Fatal error。冻结姿势无明显失败据用户反馈，仍是占位，不能称为倒地动画完成。

## 3. 开启窗口内死亡：历史尝试与最终关闭

现存攻击死亡样本的本地时点：

| 时间 | 事件 |
| --- | --- |
| 21:53:40.099 | 攻击Begin / Serial2 Arm |
| 21:53:40.348 | WindowOpen |
| 21:53:40.415 | 命中并结算目标100→75 |
| 21:53:40.548 | WindowClose，Reason=NotifyEnd |
| 21:53:40.581 | 玩家Death committed；Disarm、能力Cancelled1、TraceTick0 |

死亡发生在正常关窗后**33毫秒**。这验证了Montage仍在播放时死亡的取消，但当时Trace已经停止，不能证明死亡主动关闭一个仍开启的窗口。用户报告该项完成与现存这轮时点不同，保留为单项待确认，而不是推断代码错误或重测整章。

### 22:10复验记录

用户反馈窗口内死亡通过，用时1分钟。直接日志见[本次原始记录](Raw/M1_04C2_ClosureAttempt_2026-10-06_2210.log)：

| 本地时间（2026-10-06） | 事件 |
| --- | --- |
| 22:10:15.064 | 攻击Begin / Serial1 Arm |
| 22:10:15.297 | 玩家Death committed，开始后0.233秒 |
| 22:10:15.298 | Disarm.Windows=0 / Sweeps=0 / Tick=0 |
| 22:10:15.298 | End.Cancelled=1 / AttackingTag=0；Death sealed.TraceTick=0 |
| 22:10:43.268 | 结束释放Dead Tag，Remaining=0 |

这一Serial从未WindowOpen，也没有WindowClose，因此是窗口前的正常取消，不能关闭“开启窗口内死亡”待查项。用户看到挥击与冻结，不一定等于已经进入Notify窗口；未发现新的代码失败或运行错误。此前按0.30秒估计的操作不够稳定，现已替换，避免反复凭手感试时间。

### 10月7日助手实测：通过

采用已安装的Computer Use技能，控制UE5.5.4编辑器 / `L_ET_VerticalSlice`真实PIE；使用当前玩家25/100、既有4损伤入口和正式左键攻击。为留出读取日志和界面操作的时间，临时执行`slomo 0.01`；观察到WindowOpen后才按4，而非靠人手估计0.30秒。

| 本地时间（2026-10-07） | 同一PIE、同一玩家、Serial2的事件 |
| --- | --- |
| 19:52:57.879 | 攻击Begin / Arm，MontageID4 |
| 19:53:24.529 | WindowOpen，Windows1 |
| 19:53:39.113 | Death committed，HealthZero / DeadTag1 |
| 19:53:39.113 | WindowClose，Reason=AttackEnd |
| 19:53:39.113 | Disarm，Windows1 / Sweeps499 / Unique0 / Tick0 |
| 19:53:39.113 | 能力End，Cancelled1 / AttackingTag0；Death sealed四项正确 |
| 19:54:05.667 | 执行slomo 1恢复正常时间 |
| 19:54:12.193 | EndPlay释放Dead Tag，Remaining0 |

死亡前没有该Serial的NotifyEnd关窗；死亡提交立即主动关窗，同毫秒 / 同帧234取消能力并封锁身体，直到结束没有重新开窗。499次扫掠是慢速窗口内的累计数，不是499次伤害；本次空地未命中目标，不用它宣称去重或性能达标。慢速样本只证明实际Notify / GE / 死亡路径的时序，普通速度结果沿用既有通过项。

恢复时直接观察Class Defaults的100/100与Trace.Draw Debug=False，蓝图Compile成功；19:56:22正常保存，19:56:37新PIE HUD100/100。正常速度攻击19:57:15.833开窗、19:57:16.033关窗，0.20秒，19:57:16.600正常完成 / Tick0。19:57:35停止PIE并留在正式地图。当前日志未匹配到上述蓝图 / 脚本运行错误。没有为此重跑输入 / 暂停 / 属性完整矩阵，移动基线沿用此前用户通过反馈。

证据：[GUI实测原始日志](Raw/M1_04C2_ComputerUse_2026-10-07.log)、[时序与恢复核对结果](Raw/M1_04C2_ComputerUse_Assertions.json)。JSON由已有日志提取事件顺序，不是新增UE Automation测试，也不宣称运行了原Lifecycle / DodgeMovement测试。

## 4. 工时与阶段状态

本批主体用户手动25分钟（代码10 / 资产10 / 检查5），等待5分钟；22:10用户窄复验1分钟，C2用户合计26分钟。累计用户手动**1374分钟，22小时54分钟**；已报告等待**95分钟**另计。M1-04用户手动176 / 等待22分钟。助手此次PIE、GUI恢复与记录整理另列，不虚构为用户新增手动工时。

C2与C已验收，D按方案B完成[阶段汇总](../M1_04D_CombatReview.md)，[M1-04总验收](M1_04_Verification.md)通过，不要求新的用户反馈表或旧矩阵。3 / 4诊断图继续保留，最终发布入口策略、倒地动画、重试 / 新身体与新阶段包后续完成；阶段逻辑完成不等于作品可投递。
