# M2-06C：方向切换与体验补修验收

2026-10-25，UE5.5.4。用户已报告章节检查完成；随后出生首帧陷地与贴身镜头侧转按明确授权直接补修，双Build、四项相关自动化与真实PIE取景已复核。**本章通过，进入[M2-07A新版包与集成检查](../M2_07A_IntegratedPackage.md)。**首次PIE创建等待按用户决定忽略，关闭该需求，不作为遗留Bug或后续性能采样任务。

## 1. 实现与实际范围

原切换修改五个既有Core文件：TargetLock组件h / cpp、PlayerController h / cpp、ETTargetLockTests.cpp。新增两个Boolean IA，原IMC唯一Z / C映射，Controller分别引用；按同一视点的水平角选择指定侧，空侧保留当前目标、不绕回 / 排队，两方向共享.20游戏秒，旧→新只广播一次事件。原Tab、标记、相机和能力各保留职责。

收尾补修五旧一新：Camera h / cpp / CameraRules、GameMode h / cpp、新ETSpawnPlacementTests.cpp。正式GameMode在生成前按实际Pawn胶囊向下探测一次可走地面，保留XY / 朝向与原生行走间隙，避免出生落差让FootIK首帧追赶。没有关闭IK或改玩家胶囊 / Mesh，原Entry / Checkpoint_A Z120保持。镜头保留Shoulder(0,75,55)cm / Focus45 / 臂长400与碰撞，新增偏航补偿上限12°、PitchOffset=-18°，近景保持玩家后侧并适度俯视。

实际157 Source / 86cpp、Core60 / UI23 / Editor3、23项自动化、来源75 / 本地294份UE资产。两新IA、三个旧资产（Controller / IMC / 地图）已登记SHA；用户完成配置并保存，本轮体验补修没有重存Content或Config。当前本地提交`2b62e6a`已包含这些实现；本地跟踪引用一致，不据此声称本轮已联网核验GitHub。

## 2. 构建、自动化与取景证据

| 项目 | 实际证据 / 范围 |
| --- | --- |
| Selection夹具故障 | 无地板测试目标掉落导致拆墙后超范围；已在World启动后冻结身体，保留原拆墙断言并新增位置断言，不扩大获取阈值 |
| 正常Editor / 索引 | 2026-10-24完成，185.25秒，正常UHT / 链接；Core60 / UI23 / Editor3 |
| 正常Game Development | 同日完成，172.35秒；没有Cook或Package |
| 四项相关自动化 | 北京时间2026-10-24 22:13:43，SpawnPlacement / CameraRules / Selection / MarkerRules全部Success，0警告 / 0失败 / 0未执行；其他19项沿用 |
| 用户原章节检查 | 用户报告“检查完成”，接受其真实键鼠检查反馈；本轮不补造逐项时刻、死亡路径或新的双敌人物理按键采样 |
| 出生样本 | 两次前台Entry首可采样帧脚尖约+1.48 / +1.77cm，胶囊底+2.15cm，未再见旧-16至-19cm首帧陷地 |
| 贴身镜头 | 600 / 200 / 79cm三距离有效取景，贴身Pitch约-21.01°、交战轴偏航补偿约-12.00°；2026-10-25短复核三距离完成并正常Stop |
| 环境恢复 | 19Actor / 单正式敌人，无Probe / 临时挡板，UE All Saved；AI停止 / Teleport只用于PIE，结束后销毁；实际Content / Config / Git索引相对该轮补修前不变 |

相机截图为引擎HighResShot，路径`Artifacts/M2_06C_CameraStartup/CloseCombat.png`；它不含UMG，不用该图证明HUD / 标记。单目标身份 / Pause / 换身体等精确规则由自动化验证，真实输入 / 观感由用户检查和有限画面样本支持；未重新做完整Retry / Continue / 所有墙角实机矩阵。

原始材料位于本地忽略目录`Docs/Reports/Raw/M2_06C_SelectionFix_2026-10-24`与`M2_06C_CameraStartup_2026-10-24`。前两份补采保留清理超时 / 相机对象中断错误；后续`CameraReview_2026-10-25.json`独立记录完成与正常Stop，不把早期脚本错误删掉或写成无错误全程采集。冻结M1-03B1文档未改。

## 3. 关闭项、工时与交付边界

首次PIE创建曾采到约1.55秒、随后约0.10秒；不能据有限样本保证完全消除。用户已明确“如果不可以，直接忽略”，因此本项需求关闭，不再追加优化 / 验收。该等待与独立包实际游戏帧耗时分开，未测得任何帧率提升，不作为M2-07B采样目标。

C实际手动与等待时间未报，不补造分钟数。已量化项目3351 / 明确等待304分钟，M2为1340 / 119、M2-06为270 / 15，均另加C未登记部分；助手补修 / 工具构建不记作用户实耗。无功能阻塞；最新独立包与M2阶段整体复盘尚未完成，交由M2-07分批处理。

## 4. Commit / Push（固定放在最后）

实现已在本地`2b62e6a`；本轮新增本报告，仅随[M2-07A末尾](../M2_07A_IntegratedPackage.md#9-本章-commit--push固定放在最后)的收尾文档 / 脚本提交，不重复Stage未变玩法文件。下发材料建议英文`docs: prepare M2 package integration checks`，中文译文`docs: 准备M2阶段包内集成检查`；A完成后使用该章完成态建议。Raw / 日志 / 截图和包体留本地，用户自行Stage / Commit / Push。
