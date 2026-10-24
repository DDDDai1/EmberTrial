# 当前开发进度

更新2026-10-24。**[M2-06A目标锁定已验收](Reports/M2_06A_Verification.md)，无待补项。**下一批[B1锁定标记](M2_06B1_TargetMarker.md)已下发、待用户手动实施；镜头跟随留B2。

## 当前工程与功能基线

- UE5.5.4 / Windows Standalone / 键鼠，CLion编辑、UBT / UHT正常构建。
- 实际147 Source / 80cpp，Core57 / UI20 / Editor3；20项自动化（Core17 / UI3），来源72项 / 本地291份UE资产。
- 三段轻攻击、体力方向闪避 / 短无敌、Q治疗 / 次数、F范围破势 / 冷却HUD、近战AI、菜单 / 暂停 / Failure / 本次进程检查点均保持。
- Tab Started获取 / 解除唯一Ready存活Enemy；按需Overlap、角度 / 距离量化评分 / Visibility筛选，1200获取 / 1500脱锁、.10秒维持与.35秒遮挡宽限。无锁无Timer，死亡 / 换控 / 结束清理，不自动抢换目标。
- 当前只有锁定身份与日志，**还没有标记或自动转镜头**。A真实请求 / 敌人及玩家死亡清理、Selection专项已通过。
- 当前包Cleanup/Attempt01仍是旧版本，不含后来次数HUD / Poise / F / 冷却HUD / 锁定；本轮无Cook / Insights。

## 当前证据与下一开发入口

用户12:44反馈A全部完成，代码 / 配置80、检查10、构建等待5。六Source与候选一致，双构建覆盖最终12:31:34源码，Selection于12:42:20.733 Success；真实Tab / 主动解除 / NoCandidate、敌人及玩家死亡 / PawnChanged清理有日志。新IA实际名IA_ET_LockTarget，引用与功能正确，指南同步实物名、不重命名。详见[A报告](Reports/M2_06A_Verification.md)。

[B1指南](M2_06B1_TargetMarker.md)新增六UI文件 / 零旧Source、新WBP与Controller一份UI标记组件，读取Core同一目标并按需投影；Widget无Tick / Binding，只有锁定有效且根可见时UI组件更新。预计80–120分钟 / 人工8–12，未来才153 Source / 83cpp、UI23、21测试与来源73。当前实际仍147 / 80 / 20 / 72，未计预计工时。

[锁定路线](M2_06_TargetLockRoadmap.md)原B拆为B1标记 / B2镜头；增加的60–90分钟粗估使用M2既有缓冲，不叠加全项目预算。A已用90 + 后续220–350，当前模块估计310–440分钟；仅90为实耗。

本次只读审查、写报告 / 指南 / 状态文档；未改实际Source / UE资产，未Build / 自动化 / PIE / Cook或操作Git。冻结M1-03B1保持，旧有效测试保留。

## 工期与协作

- A手动90 / 等待5；M2-06当前90 / 5，M2累计1160 / 109。
- 项目累计3171分钟（52小时51分）/ 明确等待294分钟（4小时54分）；M2-05已收口505 / 40，只计用户报告。
- 每周30小时、百元预算、键鼠、暂无硬期限；Blender与最终美术按中后期安排。
- 普通实现由我给完整代码 / 配置 / 短查，你手动操作；直接改工程须明确授权。方案B不重搭Controller验收图，日志由我关联时间。
- 每份新操作文档末尾给出实际提交范围、英文建议Commit与专业中文译文；不改历史提交标题。

## 证据与入口

[当前B1指南](M2_06B1_TargetMarker.md)、[锁定路线](M2_06_TargetLockRoadmap.md)、[A报告](Reports/M2_06A_Verification.md)、[M2-05总验收](Reports/M2_05_Verification.md)、[M2路线](M2_Roadmap.md)、[架构](Architecture.md)、[来源](AssetManifest.csv)、[任务板](Backlog.md)、[方案B](AcceptanceWorkflow_Options.md)、[版本管理](VersionControl.md)。

## 当前批次 Commit / Push（固定放在最后）

当前实现已由用户提交`5bd5354`；实际Source / UE资产与本地HEAD一致，助手未改索引或联网核验GitHub。本轮Stage A验收报告、来源72、M2-06路线 / B1新指南、A指南和本轮状态文档，不重复Stage无差异Source / 资产。建议Commit（英文）：`docs: verify target locking and prepare the lock marker`。中文译文：`docs: 记录目标锁定验收并准备锁定标记开发`。B1实物完成后按其末尾清单提交六新UI源码、新WBP / 原Controller实际差异及报告；Raw / 日志 / 生成物 / 包体留本地，Git由用户Stage最新内容 / Commit / Push。
