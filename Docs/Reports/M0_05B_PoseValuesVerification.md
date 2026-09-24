# M0-05B 闪避／受击数值校验

日期：2026-09-21。状态：**已检查临时 Rig 中的数值结果；用户尚未制作本批 Sequence 或烘焙资源，B 未验收。**

## 方法与边界

使用项目原版 CR_Mannequin_Body，在 UE 5.5.4 独立命令行编辑器中创建临时 Rig；双臂与脊柱 FK/IK 开关显式设为 False。运行 Construction、Forwards Solve，输入 [参数 JSON](../Data/M0_05B_DefensivePoseValues.json)，按欧拉角 Linear 插值检查每个整数帧。

本批七个控制器为 spine_01_ctrl 与双臂六个 FK 控制器，默认局部 Location≈0、Scale≈1 已检查。轴向探测确认 spine_01_ctrl 的 X 正角使上身前倾、Y 正角使上身向角色右侧倾斜。当前默认 FK 模式下 chest_ctrl 不驱动相同的躯干变化，因此指南使用 spine_01_ctrl。

脚本：[inspect_m0_defensive_poses.py](../../Scripts/Editor/inspect_m0_defensive_poses.py)。脚本只读 UE 资产并在内存求值，未保存任何 UE 资产；实际进程返回 0，最终日志 0 errors / 0 warnings。原始报告位于本机 `Docs/Reports/Raw/M0_05B_RigInspection.json`，同目录有日志；按仓库规则不跟踪 Raw。

## 结果

| 检查 | 闪避 | 受击 |
|---|---:|---:|
| 采样范围与次数 | 0–20，21 个整数帧 | 0–18，19 个整数帧 |
| root / pelvis / foot_l / foot_r 位置最大漂移 | 0 cm | 0 cm |
| 最后一帧与第 0 帧所有骨骼位置最大差值 | 约 2.88e-14 cm | 约 2.88e-14 cm |
| 双肘弯曲范围（0° 为伸直） | 108.98°–123.98° | 68.98°–113.98° |
| 最明显姿势的头部相对起始位置变化 | 第 4 帧：(-17.35,+7.87,-2.93) cm | 第 3 帧：(约 0,-11.65,-1.78) cm |

Rig 空间中，角色右侧为 -X、前方为 +Y。因此闪避确实向角色右侧避让，受击确实后仰。该空间的数值用于校验，不填入 Anim Details。

## 尚待用户验收

Sequencer 的关键帧与实际插值、骨骼蒙皮外观、肩部与手掌穿插、连续播放、烘焙、独立重开、Quinn_Simple 预览和基础输入回归。几何采样通过不等于动画已完成，也不代表已经实现角色闪避位移、伤害或无敌窗口。
