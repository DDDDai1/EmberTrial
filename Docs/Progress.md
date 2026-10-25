# 当前开发进度

更新2026-10-25。**M2-06C用户已报告检查完成；随后出生首帧陷地 / 贴身镜头侧转两项已按明确授权直接补修并复核。首次冷PIE创建等待仍存在，不能写成卡顿彻底消除。**本轮不进入下一章节；详细证据与限制见[C10.1](M2_06C_TargetSwitching.md#101-出生首帧与贴身镜头补修已直接实施)。

## 当前实际工程

- 157 Source / 86cpp，Core60 / UI23 / Editor3，23项自动化；来源75 / 本地294份UE资产。两新IA已按实物和只读配置核对登记。
- 已实现四能力 / 五HUD、近战AI、菜单 / Pause / Failure / 进程内检查点、Tab选择、屏幕标记、右肩平滑镜头及Z / C方向切换；空候选保留当前锁、.20游戏秒限频，不绕回 / 排队。
- 当前镜头Shoulder(0,75,55)cm / Focus45 / Pitch-18° / Yaw0 / MaxShoulderYaw12°；原臂长400与碰撞保留。Camera / Core仍共用唯一目标，Pause / 解锁 / 换身体恢复协议不变。
- 正式GameMode生成Pawn前按实际胶囊向下探测一次可走地面；原Entry / Checkpoint_A Z120、Mesh和FootIK保持，无新增逐帧吸地。
- 旧Cleanup/Attempt01不含后来次数HUD / Poise / F / 冷却HUD / 锁定，不能代表当前功能；本轮无新包或帧率性能结论。

## 本轮证据与保留项

2026-10-24正常Editor185.25秒 / Game172.35秒、三模块索引完成；22:13:43北京时间CameraRules / Selection / MarkerRules / SpawnPlacement四Success，0警告 / 0失败，其余19项沿用。此前Selection无地板夹具补修保留，不删原断言。

实际前台两次Entry首可采样帧脚尖约+1.48 / +1.77cm，胶囊底+2.15cm；600 / 200 / 79cm取景成功，贴身约-21°俯角 / -12°偏航补偿。10/25短复核三距离再次完成并正常Stop。Content / Config / 索引相对本轮补修前不变，临时AI停止 / Teleport只在PIE；19Actor、单敌人 / 无Probe或临时挡板，UE为All Saved，CLion已打开。

首次PIE创建仍采到约1.55秒，紧接第二次约0.10秒；受冷加载 / 内存压力干扰，不能说整体卡顿已经解决。没有关闭IK、取消碰撞或改系统 / 画质。Raw保留中断采样和最终正常完成的独立记录，不把脚本错误隐去；本次未另跑Retry / Continue的完整实机矩阵。

## 工时与协作

- 已量化M2-06为270 / 等待15，M2为1340 / 119，项目3351分钟（55小时51分）/ 明确等待304分钟（5小时4分）；C实际分项工时未提供，不用估算补账，助手补修不计入用户实耗。
- 普通后续C++继续用户手动；本次按直接修改授权完成五旧一新Source。Stage / Commit / Push由用户操作，未改索引。
- 方案B继续只查受影响专项和少量真实画面，不重复旧HUD / 临时Controller验收图。下一开发步骤另行进入M2-07新版包 / 必要性能与复盘，当前不提前下发。

## 当前批次 Commit / Push（固定放在最后）

本轮C检查后出生 / 近景补修已完成，首次冷PIE创建等待仍保留；没有进入下一章。精确Source、两新 / 三旧资产与文档范围见[C末尾清单](M2_06C_TargetSwitching.md#11-本章-commit--push固定放在最后)。建议英文`feat: add target switching and refine close-combat camera and spawning`，中文译文`feat: 新增锁定目标切换并优化贴身战斗镜头与出生贴地`。用户自行重新Stage最新内容 / Review Diff / Commit / Push；Raw、日志、截图、采样脚本、索引和包体留本地，助手未操作Git索引或核验远端。
