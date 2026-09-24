# M0-05B：原地闪避／受击占位动画

日期：2026-09-21。引擎：UE 5.5.4。前置：M0-05A 已验收通过，用户手动用时约 80 分钟。

**本批由你手动制作两段素材，我处理 Rig、烘焙等复杂问题。** 预计合计 60–100 分钟；可以先完成闪避，再做受击，分别记录用时。同一问题卡住约 15–20 分钟就反馈。无需 C++ Build。

**2026-09-24 状态更新：你已完成制作与播放回归，闪避 / 受击各约 40 分钟。** 只读检查发现两个播放范围和四个关键帧数值需要对齐，当前执行 [少量补修清单](M0_05B_Corrections.md)，无需重做本指南。详细结果见 [首次验收记录](Reports/M0_05B_Verification.md)。下文保留原始制作步骤与目标参数供复现。

实际受击动画沿用 **AS_M0_HitResult_01**，真正制作源为 **LS_M0_HitResult_02**，对应下文计划名 HitReact。不要为统一拼写重命名，也不要把加载后指向闪避的 LS_M0_HitResult_01 当成受击制作源。

## 1. 目标与输出

| 动作 | 视觉目标 | Level Sequence | Animation Sequence |
|---|---|---|---|
| 闪避 | 双手收紧保护身体，上身迅速向角色右侧倾斜，然后恢复 | LS_M0_Dodge_01 | AS_M0_Dodge_01 |
| 受击 | 上身快速后仰，双臂略展开，随后小幅回弹并恢复 | LS_M0_HitReact_01 | AS_M0_HitReact_01 |

制作仍用 **SKM_Manny + CR_Mannequin_Body**。玩家继续使用 SKM_Quinn_Simple；已核实玩家与攻击动画都引用同一个 SK_Mannequin，ABP_Quinn 继承 ABP_Manny。本批沿用相同骨架。

这两段是 **M0 功能验证素材**。闪避片段只表现避让姿态，当前双脚保持原位；真正的角色位移、输入、中断、无敌窗口等留到后续 Gameplay 阶段。M0-05C 会先验证 Montage / Slot 播放与 CharacterMovement 位移衔接。正式美术与更完整的身体重心、步伐以后迭代。

沿用已存在的目录，注意是单数 `Sequence`：

```text
/Game/EmberTrial/Animations/Practice/Sequence/LS_M0_Dodge_01
/Game/EmberTrial/Animations/Practice/Sequence/LS_M0_HitReact_01
/Game/EmberTrial/Animations/Practice/Clips/AS_M0_Dodge_01
/Game/EmberTrial/Animations/Practice/Clips/AS_M0_HitReact_01
```

## 2. 新建制作 Sequence

先做闪避。它通过预览和烘焙后，再重复本节建立受击。

1. 停止 PIE，保存已有资产；打开 `/Game/EmberTrial/Maps/L_M0_AnimationLab`。
2. 若 LS_M0_Attack_01 仍打开，停止播放、保存，再关闭它的 Sequencer 标签。
3. 在关卡工具栏的场记板菜单 **Cinematics → Add Level Sequence** 新建并保存 `LS_M0_Dodge_01`。受击阶段使用 `LS_M0_HitReact_01`。从空白 Sequence 建立，避免继承攻击的额外 key 或导出关联。
4. 选中已有的 **M0_AnimationDummy**；新 Sequence 中通过 **+ Track → Actor to Sequencer** 添加它。继续使用原人偶，无须再拖入一个角色。
5. 在人偶绑定行 **+ Track → Control Rig → Control Rig Classes → CR_Mannequin_Body**。若已经自动带一条同名轨道，直接使用。
6. 确认只编辑当前新 Sequence，播放轨道只有本次 Control Rig；不添加攻击动画轨道。
7. 设置下表的范围，Control Rig section 覆盖整个范围；Display Rate=30 fps，关闭 Auto Key。End 是排他端点，最后一个姿势帧放在 End 之前。

| 动作 | Playback Start | Playback End | 五个姿势帧 | 本批预期时长 |
|---|---:|---:|---|---|
| 闪避 | 0 | 21 | 0、4、8、14、20 | 约 0.7 秒 |
| 受击 | 0 | 19 | 0、3、7、12、18 | 约 0.63 秒 |

烘焙后记录实际采样数与时长，不把时间轴范围直接当成采样点数。作为已验证例子：A 的范围为 0–31，实际动画为 31 个帧间隔、32 个采样点、1.033 秒，这种末尾采样差异不需要回头修正 A。

## 3. 本批控制器与固定设置

数值填写方法与 A 相同：在 **Anim Outliner** 搜索控制器，选中后填右侧 **Anim Details** 的局部数值。所有括号按 **(X, Y, Z)**；Rotation 单位为度，X=Roll、Y=Pitch、Z=Yaw。表内是最终值，不是增量。

本批在原有六个手臂控制器之外增加 **spine_01_ctrl**，用于带动上身倾斜：

| 设置 | 值 |
|---|---|
| arm_r_fk_ik_switch | False／不勾选 |
| arm_l_fk_ik_switch | False／不勾选 |
| spine_fk_ik_switch | **False／不勾选**，让 spine_01_ctrl 驱动躯干 |
| 下方七个控制器的 Location | 所有关键帧均为 (0,0,0) |
| 下方七个控制器的 Scale | 所有关键帧均为 (1,1,1) |
| 控制器父级／Space | 模板默认，不做空间切换 |
| 插值 | Linear |
| global_ctrl、root_ctrl、body_ctrl、其他控制器、人偶 Actor | 保持模板默认姿态或原静态摆放，不另外打位移 key |

`spine_01_ctrl` 是控制器，不能选 Skeleton Tree 中的 `spine_01` 骨骼代替。当前模板的脊柱在 FK 模式，`chest_ctrl` 不能替代它驱动这套姿势。开关如已有动画通道，让整个片段保持 False。

每一帧都给七个控制器记录 Transform key，包括数值没变化的手腕。七个控制器 Location=0 仅适用于本表，其他骨骼和脚部控制器保持原样。

## 4. 制作原地右侧闪避

先做 **0 和 4 帧**，预览上身向角色自身右侧倾斜、双手收紧；角色正对镜头时，自身右侧通常在屏幕左边。方向正常后补 8、14、20 帧。

下面为完整数值。它不是侧翻或翻滚动作，头部在最明显一帧向右偏约 17 cm、向前约 8 cm，仅作为侧闪姿态占位。

### 4.1 第 0 帧：准备

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -20, 15) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 25) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 4.2 第 4 帧：快速侧闪

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (8, 18, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -25, 10) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 85) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 30) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 85) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 4.3 第 8 帧：保持侧闪

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (6, 14, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -25, 12) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 80) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 28) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 80) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 4.4 第 14 帧：恢复

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (2, 5, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -22, 15) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 75) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 26) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 75) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 4.5 第 20 帧：回到准备

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -20, 15) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 25) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

## 5. 制作正面受击

闪避烘焙完成后，按第 2 节另建空白 **LS_M0_HitReact_01**。先做 **0 和 3 帧**，从侧面确认上身后仰，再补 7、12、18 帧。

这是一次轻至中等受击反馈：快速后仰 → 衰减 → 小幅前倾回弹 → 准备。不是倒地动作，双脚和 root 保持固定。

### 5.1 第 0 帧：准备

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -20, 15) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 25) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 5.2 第 3 帧：受击后仰

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (-12, 0, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, 5, 35) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 30) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, 5, 35) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 30) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 5.3 第 7 帧：衰减

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (-7, 0, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -5, 25) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 50) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -5, 30) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 50) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 5.4 第 12 帧：小幅回弹

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (3, 0, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -20, 15) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 75) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 25) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 75) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

### 5.5 第 18 帧：回到准备

| 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| 躯干 `spine_01_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 右上臂 `upperarm_r_fk_ctrl` | (0,0,0) | (0, -20, 15) | (1,1,1) |
| 右前臂 `lowerarm_r_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 右手 `hand_r_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |
| 左上臂 `upperarm_l_fk_ctrl` | (0,0,0) | (0, -20, 25) | (1,1,1) |
| 左前臂 `lowerarm_l_fk_ctrl` | (0,0,0) | (0, 0, 70) | (1,1,1) |
| 左手 `hand_l_fk_ctrl` | (0,0,0) | (0, 0, 0) | (1,1,1) |

## 6. 打 key、预览与烘焙

### 6.1 每个姿势的操作

1. 停止播放，先把时间指针移到表中帧。
2. 逐个选控制器，输入对应 L / R / S，Enter 提交。
3. 选中的控制器保持选中，将鼠标焦点放入动画视口，按 **S** 记录 Transform key；检查轨道确实出现该帧关键帧标记。若 S 不生效，使用对应 Transform 通道 Add Key 按钮。
4. 七个控制器全部完成后，切走再切回来，确认数值没有丢失。
5. 完成五帧，将这七个控制器的本次关键帧插值设成 Linear。
6. 分别从正面、侧面连续预览。最后一帧与第 0 帧相同；不在整数帧之外额外插入试验 key。

### 6.2 预览检查

| 检查 | 闪避 | 受击 |
|---|---|---|
| 动作辨识 | 上身向角色右侧避让，双手收紧 | 上身后仰，手臂略展开，再回弹 |
| 节奏 | 0→4 帧快速进入，后面恢复 | 0→3 帧快速受击，之后逐渐恢复 |
| 根和脚 | 保持原位 | 保持原位 |
| 身体外观 | 肩肘自然，无明显穿胸或翻腕 | 无过度折腰、头部反折或肘部翻转 |
| 循环 | 回到准备，无明显瞬跳 | 回到准备，无明显瞬跳 |

若 spine_01_ctrl 数字在变但身体不动，先核对 **spine_fk_ik_switch=False** 以及控制器名称。若某帧不动，核对是否打 key。问题仍在就发同屏截图，我接管检查；无需修改 Rig Graph。

### 6.3 烘焙与独立播放

每完成一段就烘焙一段，沿用 A 已通过的流程：

1. 保存当前 Sequence 和地图。清空 Sequencer 左侧搜索。
2. 右键最上层 **M0_AnimationDummy** 绑定行，选择 **Bake Animation Sequence**。
3. 使用本段对应的新名字输出到 Clips，务必核对 Dodge / HitReact 名称，保留 AS_M0_Attack_01。
4. 使用当前播放范围，保留骨骼变换导出，Export in World Space=False（若提供）。
5. 输出后 **Enable Root Motion=False**，核对 Skeleton 为 **SK_Mannequin**，记录实际帧数／采样点数、采样率和时长。
6. 关闭 Sequencer，单独重开 Animation Sequence，Preview Mesh 选 SKM_Manny，确认动作仍会播放、root 不漂移。
7. 再将 **预览 Mesh** 切为 SKM_Quinn_Simple 看一次，检查用于当前玩家外观时没有明显异常。这里只改动画编辑器的预览 Mesh，不改玩家蓝图或 Skeleton。
8. 保存新动画及其 Level Sequence，保留可编辑源。

如烘焙为静止姿势、动作不全或时长明显过长，提供导出选项和时间轴截图；如果出现报错，复制日志文字。不要覆盖攻击资产来反复试错。

## 7. 本批验收与反馈

两段均完成后，独立重开已有 AS_M0_Attack_01 快速播放一次，确认 A 的动作仍正常。保存后回 ThirdPersonMap，做一次移动／转向／跳跃、E 交互、P 暂停恢复回归。

```text
M0-05B 完成 / 闪避完成 / 卡在第 __ 节
闪避五帧预览：正常 / 实际问题
受击五帧预览：正常 / 实际问题
AS_M0_Dodge_01 独立播放：正常 / 尚未完成 / 实际问题
AS_M0_HitReact_01 独立播放：正常 / 尚未完成 / 实际问题
两段动画 Skeleton：
闪避实际采样率 / 帧数或采样点数 / 时长：
受击实际采样率 / 帧数或采样点数 / 时长：
固定参考下 root 和脚漂移：无 / 实际问题
Quinn_Simple 预览：正常 / 实际问题
原攻击动画回归：正常 / 实际问题
地图 / 两个 Sequence / 两段动画已保存：是 / 否
回 ThirdPersonMap，基础输入 / E / P：正常 / 实际问题
实际用时：闪避约 __ 分钟，受击约 __ 分钟
```

姿势问题给关键帧截图（人偶、Anim Details、时间指针同屏），时序问题给 5–10 秒视频，报错用文字。先完成闪避即可反馈，不必等两段全做完才报告阻塞。

这两段通过后，进入 M0-05C 的 Montage / Slot、输入与四动作实机试验。B 完成不代表角色已经能闪避或受伤。

数值来源：[参数 JSON](Data/M0_05B_DefensivePoseValues.json)；校验边界：[数值校验报告](Reports/M0_05B_PoseValuesVerification.md)。数据用于复现与后续校准，你无需运行 Python 或把 JSON 导入 UE。
