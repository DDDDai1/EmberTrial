# M0-05B 闪避／受击动画首次验收

日期：2026-09-24。结论：**用户已完成制作与播放回归；只读检查发现两处范围和四个关键帧数值差异，待小范围补修复验。M0-05B 暂不标为完全通过，M0-05C 尚未开始。**

## 用户报告

闪避、受击五帧预览与 AS 独立播放正常；固定参考下 root / 脚无漂移；Quinn_Simple 与原攻击预览正常；地图、两个 Sequence 和两段动画已保存；ThirdPersonMap 基础输入 / E / P 正常。闪避约 40 分钟、受击约 40 分钟，合计 **80 分钟**。

用户填写“闪避 20 帧、受击 18 帧”，Skeleton 字段留空。下方通过 UE API 补齐元数据，并区分关键帧位置与烘焙采样数。上述外观和 PIE 功能结果来自用户，助手没有在本轮重新进行交互试玩。

M0-04 已报告 190 分钟，M0-05A 80 分钟，M0-05B 首轮 80 分钟，这三批手动记录合计 350 分钟；不含助手排障和其他阶段，补修用时待反馈。

## 直接读取的资产

独立 UE 5.5.4 命令行编辑器运行 [inspect_m0_defensive_assets.py](../../Scripts/Editor/inspect_m0_defensive_assets.py)，加载保存后的资产，读取元数据、Control Rig 关键帧和骨骼姿势。脚本不修改或保存 UE 资产。原始结果在本机 `Docs/Reports/Raw/M0_05B_AssetInspection.json`，日志在同名 `.log`，均按仓库规则不跟踪。

| 项目 | 闪避 | 受击 |
|---|---|---|
| 实际 AS 名称 | AS_M0_Dodge_01 | **AS_M0_HitResult_01** |
| 制作 Sequence | LS_M0_Dodge_01 | **LS_M0_HitResult_02** |
| Skeleton | SK_Mannequin | SK_Mannequin |
| 源采样率 | 30/1 fps | 30/1 fps |
| GetNumFrames / GetNumKeys | **31 / 32** | **31 / 32** |
| 动画时长 | **1.0333333 秒** | **1.0333333 秒** |
| Sequence Display Rate | 30/1 fps | 30/1 fps |
| Sequence Playback Start / End | **0 / 31** | **0 / 31** |
| Enable Root Motion | False | False |
| 动画轨道数量 | 161 | 161 |

玩家仍为 `SKM_Quinn_Simple`，Anim Class 为 `ABP_Quinn_C`；两段动画、攻击和玩家 Mesh 引用同一 `/Game/Characters/Mannequins/Meshes/SK_Mannequin`。原攻击仍为 31 帧间隔 / 32 采样点 / 1.033 秒，本轮不修改 A。

`LS_M0_HitResult_01` 文件存在，但加载该路径返回 `LS_M0_Dodge_01`，不能把它当成独立受击源。真正受击关键帧读自 `LS_M0_HitResult_02`。本轮保留旧路径，没有删除、重命名或修复重定向。

## 姿势与参数检查

在 Quinn_Simple 的组件空间，分别请求 Raw / Compressed 求值，并按每段 32 个时间点采样：所有返回值有效且有限。两段 root、pelvis、foot_l、foot_r 的位置变化均为 0 cm，缩放变化为 0；root 旋转变化为 0，脚的微量旋转读数约 0.000002°，属浮点精度量级。两种求值模式下，本次选取骨骼的汇总结果一致；这不是全部平台压缩质量或蒙皮穿插测试。

闪避头部最大位移约 19.39 cm；受击约 11.86 cm，确认最终烘焙结果具有动作。两段左手起止位置均相差约 5.27 cm，与第 0 帧左上臂未对齐末帧的源数据一致。

对两个 Sequence 的七个控制器、五个姿势帧逐项比较 L/R/S 与 [原参数表](../Data/M0_05B_DefensivePoseValues.json)，630 个标量中发现以下四项不符；其他被比较值一致：

| Sequence | 帧 | 通道 | 保存值 | 原计划值 |
|---|---:|---|---:|---:|
| LS_M0_Dodge_01 | 0 | upperarm_l_fk_ctrl.Rotation.Z | 15 | **25** |
| LS_M0_HitResult_02 | 0 | upperarm_l_fk_ctrl.Rotation.Z | 15 | **25** |
| LS_M0_HitResult_02 | 7 | spine_01_ctrl.Rotation.X | -12 | **-7** |
| LS_M0_HitResult_02 | 12 | spine_01_ctrl.Rotation.X | -12 | **+3** |

闪避五帧实际位于 0/4/8/14/20，受击位于 0/3/7/12/18。当前 End=31 使完成姿势之后仍保留额外尾段；应分别调整到 **21、19** 再烘焙。受击 7/12 帧一直保持最大后仰，未实现原定的衰减和小幅前倾回弹。上述差异是节奏和首尾连续性的具体问题，不要求重做其余姿势。

本次没有逐项校验全部控制器、插值模式或 FK/IK 曲线，也没有完成角色运行时 Montage 测试。最终脚本提取成功不等于动作验收通过；`review_findings` 单独记录待修项。

## 编辑器日志与错误边界

本轮读取的编辑器日志包含 06:47:25 UTC 首次 Dodge 烘焙的 `Missing track key data, expected uniform data` 错误。后续有旧 Dodge 删除、重新烘焙与 07:02:58 UTC 保存的记录；受击最终资产于 07:02:56 UTC 保存。本轮检查的是后续落盘版本，能够加载并逐帧求值；未据早期错误断言最终资产损坏，也不把整份编辑器日志说成零错误。

独立只读检查进程成功返回 0，汇总为 0 errors / 0 warnings；其作用是读取数据，不能替代用户的视觉验收。

## 下一步

执行 [M0-05B 补修清单](../M0_05B_Corrections.md)，由用户完成基础改值、打 key、范围调整、重烘焙并反馈。若导出或 Rig 出错，由助手接手。

B 复验通过后再下发 C：先 Montage / Slot 与按键播放，再动作状态、CharacterMovement 闪避位移及中断，最后做暂停回归和约 30 秒四动作实机片段。C 的拆分用于控制每批操作量，本轮未实现或验证 C。
