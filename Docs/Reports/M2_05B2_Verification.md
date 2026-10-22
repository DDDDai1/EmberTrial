# M2-05B2：韧性耗尽、恢复与占位反应验收

2026-10-22，UE5.5.4。用户11:19反馈全项完成，无失败或占位表现问题；代码 / 配置70分钟、检查20分钟、构建等待5分钟。**B2验收通过，闪退补修关闭，无待补项；直接进入[C1施放动画素材](../M2_05C1_BreakPulseAnimation.md)。**本轮只读核对源码、构建 / 索引、资产与日志，更新文档；未修改实际Source / UE资产，未代Build / 自动化 / PIE / Cook或操作Git索引。

## 1. 源码与构建

三新 / 九旧共12份Source均已落地。十份与下发候选有效Token一致，其中测试使用10月22日补修版；Character.h只把同一个friend声明移到friend列表末尾，Validator只有相邻字符串换行 / 拼接差异，格式化后候选一致，两者语义保持。相对B1，其余119份既有Source未改，没有额外玩法改动。

实际**131 Source / 70cpp**，CodeModel **Core50 / UI17 / Editor3**，16项自动化。最新测试11:12:53保存，Core DLL11:13:23、Editor receipt11:13:24；Game exe / receipt11:15:08 / 09。Editor模块的最终Validator在10:30前后修改、DLL10:31:58已覆盖，UI未改，不要求它与后改的Core测试一起重编。双目标结果结合用户反馈接受，不重复构建。

组件基于当前身体 / Ready订阅Poise与自身Spec结束，归零一次性派发，出生0 / 晚观察同样走快照；GAS拥有反应状态，结束存活才通过Instant GE回填当前MaxPoise。表现复用旧HitResult Montage，状态仍由1.2游戏秒Timer决定。无常驻Tick、无失败轮询；未测量新性能，不声称FPS提升。

## 2. 配置与资产

四项项目校验均为[M1-07C2] Valid / Issues0，记录位于本日崩溃前备份日志：GA最终11:04:32，玩家11:04:38、训练目标11:04:43、敌人11:04:48。后续修复只改测试，配置及生产实现未再变化，因此沿用这些结构校验，不要求重复四次Validate。

本批唯一UE资产字节变化为GA_ET_Stagger；原三个身体、HitResult AS / Montage、地图、Rig / Skeleton和Config保持B1哈希。用户确认GA新引用AM_M0_HitResult_01_Montage、Duration1.2与原Tags；StaggerAbility又实际断言正式时长和配置。未独立读取全部UE Details；来源仍65项，仅更新GA记录和旧Montage的新增用途，不把未来C1三资产提前计入。

## 3. 闪退补修及当次测试

之前11:06:50闪退发生于EndQueued：Body->Destroy后推进世界，测试还通过旧ASC调用GetNumericAttributeBase，崩溃栈落在ETPoiseReactionTests.cpp的126 / 174行。错误来自助手原测试的生命周期设计。修正版保留两个真实Destroy场景，销毁前查Current / Base，销毁后只查按值持有的原生事件记录与独立Timer句柄；不再访问旧Body / Component / ASC，也不留下引用栈变量的委托。

| 本地时刻 | 修复后当次最终结果 |
| --- | --- |
| 11:16:37.406 | EmberTrial.M2.Combat.PoiseReaction：Success |
| 11:16:55.349 | EmberTrial.M2.Combat.StaggerAbility：Success |
| 11:17:14.955 | EmberTrial.M2.Combat.PoiseFoundation：Success |

PoiseReaction覆盖11场景：正常循环与当前Max80回填、出生0、晚观察、暂停、存活取消、同帧归零死亡、活动死亡、排队 / 活动Destroy、外部Stunned解除及旧观察代次。B1状态专项保留四旧动作接管、Tag计数和DurationTimer / Spec清理；基础专项保留夹限与初始化。测试未放宽关键断言，未把未重跑的其余13项写成当次Success。

## 4. 实机证据与范围

当前正式世界11:17:32初始化；11:18:10玩家真实GE100→40，11:18:30 / 45查询仍40，支持部分韧性不自动补满。11:19:17敌人100→40，11:19:20归零，只一次Depleted Accepted1 / Stagger begin，11:19:21.563结束、.580 Restore Applied1 / Max100。11:19:31世界退出，三身体Ready撤销、UI与观察关系清理。

用户确认玩家归零 / 结束100 / 下轮独立、Quinn占位动作、敌人追击中断 / 恢复、生命与其他资源隔离、Stop再开与短基线。保留日志没有逐步记录玩家后半段归零或全部键鼠操作，这些按用户实机反馈接受，并以真实GE自动化补充逻辑证据；不虚构玩家Depleted日志或第二轮PIE时间。

修复后当前日志未见Fatal / 玩法Error / Accessed None；有启动图标文件缺失与EOS权限Warning，属于环境记录，不写成全日志零警告。此次检查20分钟高于普通目标，包含失败后的核对需求；用户未单独拆分排错时间，不另行追加工时。

B2完成被动归零反应与恢复，**F范围施放、范围 / 阵营 / 遮挡、冷却及施放素材尚未实现**。没有新Cook / Insights；独立包仍Cleanup/Attempt01，不包含后来新增的次数HUD、Poise或破势功能。

## 5. 工时与下一批

本批手动90 / 等待5分钟；M2-05累计240 / 15，M2累计805 / 79。项目累计**2816分钟（46小时56分）/ 明确等待264分钟（4小时24分）**。只计用户报告，不补算助手设计或未报告学习。

下一批[C1](../M2_05C1_BreakPulseAnimation.md)为独立施放LS / AS / AM，五帧完整L/R/S、12帧预留提交点；不改Source / 既有受害反应，不重复Build与旧测试。预计手动30–60、人工检查5–10分钟，实际制作后再核对资源、保存来源并下发C2。

## 6. 本次 Commit / Push（固定放在最后）

核对期间用户新建HEAD **98be3e0**，其父提交为1c1ab9d，内容只新增三份**0字节Source**。完整B2代码仍在工作区，工作区运行验收通过不代表Git已包含本批实现。当前应Stage全部十二Source、GA_ET_Stagger和待提交B1 / B2报告、指南、C1新指南 / JSON、来源 / 状态文档，补交一次，无需回滚；路径与CLion步骤见[B2末尾](../M2_05B2_PoiseReaction.md#12-本批-commit--push固定放在最后)。

```text
feat: complete poise reaction and recovery
```

C1三份UE资产当前尚不存在；不要提前提交空资源。Raw / 崩溃转储 / 日志 / 生成文件 / 包体留本地。用户自行Stage / Commit / Push，助手未核验远端或操作索引。
