# M1-04：正式攻击、伤害、受击与死亡总验收

日期：2026-10-07。**M1-04 A / B / C / D全部完成。**阶段范围是单人离线、单段攻击与训练目标战斗链路；没有把AI、倒地动画、重试或新包记作已完成。

## 1. 接受的证据

| 批次 | 结果 | 证据与限制 |
| --- | --- | --- |
| A | 正式GAS攻击、Montage完成 / 取消、输入迁移与门控 | [A核查](M1_04A_Verification.md)：源码 / receipt / 13组开始结束及取消，Lifecycle Success；具体输入边界采用用户反馈 |
| B1 | Notify窗口、扫掠 / 遮挡、每次攻击去重 | [B1核查](M1_04B1_Verification.md)：19组登记清理与去重 / 取消 / 遮挡日志；双目标与限帧完整操作按用户通过反馈 |
| B2 | SetByCaller真实GE伤害、来源 / 目标 / GE契约 | [B2核查](M1_04B2_Verification.md)：10条成功伤害、拒绝 / 配置Warning、23组Trace清理；配置最终恢复按用户反馈 |
| C1 | 属性事件驱动正式受击、取消攻击 / 闪避、独立身体 | [C1核查](M1_04C1_Verification.md)：17组受击、真实玩家损伤 / HUD和中断样本；历史中途连线错误已解决 |
| C2 | 一次死亡、先关窗再取消、行动与治疗封锁、结束清理 | [C2核查](M1_04C2_Verification.md)：主体七次提交 / 封锁 / Tag释放，窗中死亡由助手10月7日补实测；Base / Current死亡治疗采用用户反馈与源码核对 |
| D | 链路说明、保留入口、专项触发与后续边界 | [D汇总](../M1_04D_CombatReview.md)：复用已接受证据，无新源码或测试平台，无新的用户检查表 |

唯一时序缺口已关闭：19:53:24.529 WindowOpen；19:53:39.113 Death committed → WindowClose(AttackEnd) → Disarm.Tick0 → End.Cancelled1 / AttackingTag0 → Death sealed；结束Dead Tag Remaining0。实际执行于真实PIE、临时slomo0.01，证明清理路径，不据此宣称帧率或正常速度性能达标。

原始日志见[助手实测](Raw/M1_04C2_ComputerUse_2026-10-07.log)，[日志顺序核对](Raw/M1_04C2_ComputerUse_Assertions.json)全通过；后者不等于新增UE Automation用例。其他没有直接画面 / 日志的部分保持各报告列明的用户反馈范围，不改写为助手全项亲测。

## 2. 最终工程状态

- Source45份、22个工程cpp，与C2实施基线一致，收尾未修改C++、Build.cs、Target或uproject，未重新Build。
- 资产来源清单35项，只有玩家Blueprint因恢复100/100、Debug False而更新，其余34项与此前基线一致；没有新增模型、动画或其他资产。
- 玩家蓝图GUI Compile成功，19:56:22正常保存；19:56:37新PIE Health100 / Max100与HUD正确。正常攻击窗口19:57:15.833–19:57:16.033，0.20秒，19:57:16.600正常完成 / Tick0。
- 已执行slomo1并停止PIE，编辑器留在L_ET_VerticalSlice。3 / 4开发入口保留；4既有损伤图未改，原E / P与移动基线沿用C2用户结果，未重跑全矩阵。
- 当前助手实测日志未匹配到Blueprint Error / Warning、Accessed None、Blueprint Runtime Error、LogScript Error / Warning或Fatal error。

[M1-04D基线](Raw/M1_04D_Baseline.json)记录当前来源与资产哈希，不替代实机测试。Editor / Game构建使用C2已有成功receipt；当前M0包没有重Cook，不能代表本章内容。

## 3. 工时与下一阶段

用户手动：A：50、B1：40、B2：35、C1：25、C2主体：25 / 窄复验：1，共176分钟；明确报告等待22分钟。全项目用户累计1374分钟（22小时54分钟），等待95分钟另计。助手接管PIE / 恢复与D文档工作不计为用户手动投入，D没有要求新操作。

下一阶段M1-05近战占位敌人尚未开始。当前完整作品、性能基线、正式死亡表现、重试与投递准备仍待后续完成；该总验收只关闭M1-04的约定范围。
