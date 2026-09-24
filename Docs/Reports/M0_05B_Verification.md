# M0-05B 闪避／受击动画验收记录

更新：2026-09-24。**最新结论：补修后六项差异已解决五项，仅闪避 0 帧左上臂 Z 仍需从 15 改为 25 并重新烘焙。已下发 C1 指南，开头先收尾此项，再直接继续 Montage；B 尚未完全通过，C1 尚未由用户实施。**

下文“首次验收”保留补修前证据；最新结果见末尾“补修复验”，不要把之前的 1.033 秒当作当前两段时长。

## 首次验收结论

用户完成制作与播放回归；只读检查发现两处范围和四个关键帧数值差异，随后下发局部补修指南。

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

## 首次验收时的下一步

执行 [M0-05B 补修清单](../M0_05B_Corrections.md)，由用户完成基础改值、打 key、范围调整、重烘焙并反馈。若导出或 Rig 出错，由助手接手。

B 复验通过后再下发 C：先 Montage / Slot 与按键播放，再动作状态、CharacterMovement 闪避位移及中断，最后做暂停回归和约 30 秒四动作实机片段。C 的拆分用于控制每批操作量，本轮未实现或验证 C。

## 补修复验（2026-09-24）

用户补修约 **20 分钟**，报告四处数值、两个 End、重新烘焙、首尾 / 回弹 / 无漂移、Quinn_Simple、保存与基础输入 / E / P 正常。AS 沿用 AS_M0_Dodge_01 / AS_M0_HitResult_01。用户在“显示时长”填 21 / 19，实际秒数由下方直接读取补齐。

本次重新运行相同只读脚本，进程返回 0，0 errors / 0 warnings；`review_findings` 从 6 项降为 **1 项**。Raw JSON / 日志为本次复验结果，首次结果的摘要保留在上文及补修前 Git 检查点 `30a8289`。

| 项目 | 当前闪避 | 当前受击 |
|---|---|---|
| 真正制作源 | LS_M0_Dodge_01 | **LS_M0_HitResult_01** |
| Sequence Start / End | 0 / **21** | 0 / **19** |
| AS 源采样率 | 30 fps | 30 fps |
| AS 帧间隔 / 采样点 | **21 / 22** | **19 / 20** |
| AS 秒数 | **0.69999999** | **0.63333333** |
| Skeleton / Root Motion | SK_Mannequin / False | SK_Mannequin / False |
| 0 帧左上臂 Rotation Z | **仍为 15，待修正为 25** | **25，已修正** |
| 7 / 12 帧脊柱 X | 对应闪避参数，保持正确 | **-7 / +3，已修正** |
| 左手首尾位置差 | **约 5.27 cm** | **0 cm** |
| root / 骨盆 / 双脚位置变化 | 0 cm | 0 cm |

受击源路径与首次检查相比已经变更：现在 `_01` 是真正 Sequence，`_02` 加载后指向 `_01`。本轮未重命名资产，记录的是磁盘现状。此前补修文档里的受击路径不再适用于当前状态。

两种姿势求值模式仍返回有效、有限结果。受击的三项参数与范围均已对齐，用户也确认回弹正确；闪避范围正确，但 0 帧左上臂的源 key 和烘焙结果均保留旧值。不能仅据用户勾选“是”标记最后一项通过，也不能将其根因直接认定为忘记保存；需要检查的是实际 key。

当前安排：执行 [M0-05C1 指南](../M0_05C1_MontagePlayback.md) §0，更新闪避 key、关闭重开确认和重烘焙后，直接继续 C1，不要求再单独等待一次 B 回复。下轮将同时核对 B 收尾和 C1。

用户 B 制作总用时现为 **80 + 20 = 100 分钟**。M0-04、M0-05A、M0-05B 三批已报告手动时间合计 **370 分钟**；后续闪避收尾与 C1 另计。

## C1 前置读取

[inspect_m0_montage_prereqs.py](../../Scripts/Editor/inspect_m0_montage_prereqs.py) 只读检查确认：当前玩家仍用 SKM_Quinn_Simple / ABP_Quinn_C，实际 IMC 为 `/Game/ThirdPerson/Input/IMC_Default`；ABP_Manny 的 AnimGraph 存在一个 DefaultSlot，Always Update Source Pose=False；ABP_Quinn 无独立 AnimGraph。原始 JSON / 日志在本机 `Docs/Reports/Raw/M0_05C1_Prerequisites.*`，检查进程返回 0，0 errors / 0 warnings。

该检查枚举图表节点，不声称已经验证图内所有引脚连线。C1 指南复用此 Slot，并要求用户 Compile 与实际播放验证；本轮未生成 Montage / Input Action，也未修改角色或 AnimBP。Montage Play、Is Any Montage Playing 及混合参数语义已结合本机 5.5.4 源码核对。
