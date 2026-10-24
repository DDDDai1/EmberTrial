# 当前开发进度

更新2026-10-24。**[M2-06B1锁定标记已验收](Reports/M2_06B1_Verification.md)，无阻塞补项。**下一批[B2镜头跟随](M2_06B2_LockCamera.md)已下发、待用户手动实施。

## 当前工程与功能基线

- UE5.5.4 / Windows / 离线单人键鼠；CLion编辑，UBT / UHT正常构建。
- 实际153 Source / 83cpp，Core57 / UI23 / Editor3；21项自动化（Core17 / UI4），来源73项 / 本地292份UE资产。
- 三段轻攻击、体力方向闪避 / 短无敌、Q治疗 / 次数、F范围破势 / 冷却HUD、近战AI、菜单 / Pause / Failure / 本次进程检查点保持。
- Tab Started获取 / 解除唯一Ready存活Enemy，按需候选、距离迟滞 / 遮挡宽限及死亡 / 换控清理；不自动抢换目标。
- B1已接单菱形标记、UMG投影 / 离屏隐藏再现、独立View重建；Widget无Tick / Binding，表现组件锁定且根可见时在相机更新后投影。**自动镜头仍未实现**。
- 旧包Cleanup/Attempt01不含后来次数HUD / Poise / F / 冷却HUD / 锁定，本轮无Cook / Insights。

## 当前证据与下一开发入口

用户13:39反馈B1全项完成；六新UI与候选一致、147旧Source / Config不变，只有新WBP与旧Controller资产变化。最后Source13:20:12，Editor13:24:59 / Game13:27:38覆盖；MarkerRules13:36:47.580 Success，正式WBP两次项目Valid。早期绑定错误由后续保存 / Valid / 运行覆盖；13:38两次View重建不换Core目标，Pause / 返回菜单与Stop清理有日志。窗口缩放等画面按用户反馈接受，PIE方式未注明，不宣传完整DPI矩阵。详见[B1报告](Reports/M2_06B1_Verification.md)。

[B2指南](M2_06B2_LockCamera.md)三新Core / 两旧Controller，通过既有UpdateRotation提供平滑Yaw / Pitch，不另建Tick / Timer或目标身份，不改Actor / 动作方向；锁定跟随接管，解锁从当前角度归还鼠标。预算80–130 / 人工8–12；未来才156 Source / 85cpp、Core59 / UI23 / Editor3、22测试，来源仍73。当前不计B2完成或工时。

[锁定路线](M2_06_TargetLockRoadmap.md)A / B1已用180 + B2 / C剩余140–230，模块估计320–410分钟（5小时20分–6小时50分）；只有180为实耗。助手本轮未改实际Source / UE资产、未Build / 自动化 / PIE / Cook或操作索引；冻结M1-03B1不变。

## 工期与协作

- B1手动90 / 等待5，M2-06累计180 / 10，M2累计1250 / 114。
- 项目累计3261分钟（54小时21分）/ 明确等待299分钟（4小时59分）；M2-05已收口505 / 40，仅计用户报告。
- 每周30小时、百元预算、键鼠、暂无硬期限；Blender / 最终美术按中后期安排。
- 我提供完整代码 / 清楚配置 / 短查，你手动实施；直接改工程须明确授权。方案B不重搭Controller验收图，日志由我关联时间。
- 每份新操作文档末尾有实际提交范围、英文建议Commit与专业中文译文；不改历史提交标题。

## 证据与入口

[当前B2指南](M2_06B2_LockCamera.md)、[锁定路线](M2_06_TargetLockRoadmap.md)、[B1报告](Reports/M2_06B1_Verification.md)、[A报告](Reports/M2_06A_Verification.md)、[M2路线](M2_Roadmap.md)、[架构](Architecture.md)、[来源](AssetManifest.csv)、[任务板](Backlog.md)、[方案B](AcceptanceWorkflow_Options.md)、[版本管理](VersionControl.md)。

## 当前批次 Commit / Push（固定放在最后）

B1完整实现已由用户提交`c902dcc`；正确UI测试路径、六新UI与两资产均已纳入，实际Source / 资产与HEAD一致，无需重复Stage，旧误放 / AM暂存问题已关闭。本轮Stage新B1报告、指南收尾、来源73、状态 / 架构 / 路线及B2指南最新修订。建议Commit（英文）：`docs: verify the target marker and prepare camera following`。中文译文：`docs: 记录锁定标记验收并准备镜头跟随`。B2三新Core / 两旧Controller尚未实施，完成后再按[B2清单](M2_06B2_LockCamera.md#9-本章-commit--push固定放在最后)提交。Raw / 日志 / 生成物 / 包体留本地，Git由用户操作；助手未改索引、Commit、Push或联网核验远端。
