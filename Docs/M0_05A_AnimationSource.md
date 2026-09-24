# M0-05A：零成本动画路线与第一段攻击占位

日期：2026-09-20；验收更新：2026-09-21。引擎：UE 5.5.4。前置：M0-04 已完成。状态：**M0-05A 已完成，手动用时约 80 分钟。验收见 [报告](Reports/M0_05A_Verification.md)，下一步进入 [M0-05B](M0_05B_DefensiveAnimations.md)。以下步骤保留供复盘。**

用户已确认没有现成攻击 / 闪避 / 受击动画，采用模板与 Control Rig 的零成本占位路线。本批预计 60–120 分钟，首次接触 Sequencer / Control Rig 时以实际用时为准；同一处操作卡住约 15–20 分钟就反馈，由我接管复杂部分。

## 1. 本批目标及 M0-05 拆分

本批先做一段约 1 秒的**原地右手挥击**：准备 → 蓄力 → 前挥 → 收回。复用现有 UE5 Manny 的骨架和 Control Rig，烘焙成 Animation Sequence。先让制作、保存、重开播放的完整流程跑通。

| 子批 | 内容 | 完成条件 |
|---|---|---|
| M0-05A，本批 | 核对当前角色绑定；在独立场景制作第一段攻击占位并烘焙 | AS_M0_Attack_01 可独立播放，骨架明确，root 不漂移 |
| M0-05B，后续 | 按同一路线补原地闪避姿态和受击占位 | 三段动作有辨识度，能判断零成本制作路线是否够用 |
| M0-05C，后续 | Montage / Slot 接入角色，移动 → 攻击 → 闪避 → 受击的 30 秒实机试验 | 动作退出 / 中断不锁死，暂停回归正常，位移方案有实测结论 |

本批烘焙成功只代表 A 完成，不等于攻击系统或 M0-05 已完成。真正的闪避位移由后续 CharacterMovement 方案驱动，本批不靠挪动 Mesh 或场景 Actor 假装实现位移。正式武器和关键模型仍按中后期 Blender 路线安排。

## 2. 已核实的工程资产，以及你要补充的绑定信息

磁盘已有以下资产，不必下载资源包：

```text
/Game/Characters/Mannequins/Meshes/SKM_Manny
/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple
/Game/Characters/Mannequins/Meshes/SKM_Quinn
/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple
/Game/Characters/Mannequins/Meshes/SK_Mannequin
/Game/Characters/Mannequins/Animations/ABP_Manny
/Game/Characters/Mannequins/Animations/ABP_Quinn
/Game/Characters/Mannequins/Rigs/CR_Mannequin_Body
```

现有 Animations 目录包含移动、待机、跳跃 / 落地；本轮文件名检索没有发现攻击 / 闪避 / 受击或 Montage 资源。文件名不能证明资产全部用途，因此不声称已逐个打开审查。

**记录当前玩家实际绑定的方法（本工程已完成核对，不必重做）：**

1. 打开 BP_ThirdPersonCharacter，Components 中选 **Mesh (CharacterMesh0)**。
2. 在 Details 记录 **Skeletal Mesh Asset、Animation Mode、Anim Class** 的实际值。
3. 用 Skeletal Mesh 旁的浏览按钮定位资产，打开其编辑器，在 Asset Details 查 **Skeleton**，记录资产名 / 路径。
4. 用 Anim Class 对应资产的浏览按钮打开当前 Anim Blueprint，查看 **Class Settings → Parent Class**，记录名称。ABP_Quinn 可能是子动画蓝图；不要为了找不到 AnimGraph 随意改父类。

2026-09-21 已通过用户反馈与只读 UE API 核实：玩家 Mesh=SKM_Quinn_Simple，Skeleton=SK_Mannequin，Animation Mode=Animation Blueprint，Anim Class=ABP_Quinn_C，ABP_Quinn 的父类为 ABP_Manny_C。外观和 `_C` 类名本身不能代替 Skeleton / ParentClass 检查。

本批制作使用 **SKM_Manny + CR_Mannequin_Body**，当前玩家保持原样。已确认输出攻击动画与玩家共用同一 SK_Mannequin，可在 C 阶段据此验证 Montage / Slot；不同 Skeleton 资产不能只因骨骼名称相似就直接互换。

## 3. 建立独立动画试场

### 3.1 保存地图与目录

1. 停止 PIE，先 Save All 保存已有 M0-04 成果。
2. File → New Level，选择 **Basic / 基础**，保存为 `/Game/EmberTrial/Maps/L_M0_AnimationLab`。如果 Basic 模板不可用，先反馈菜单截图，不另建大型 Open World。
3. 在内容浏览器建立：

```text
/Game/EmberTrial/Animations/Practice/Sequences
/Game/EmberTrial/Animations/Practice/Clips
```

不更改 Project Settings 的默认地图、GameMode，也不替换玩家 Mesh / Anim Class。这个地图只用于制作素材，本批不需要在其中 Play。

### 3.2 加入制作人偶与 Level Sequence

1. 将 **SKM_Manny** 从内容浏览器拖入新地图，得到一个 Skeletal Mesh Actor，重命名为 **M0_AnimationDummy**。不要把 BP_ThirdPersonCharacter 拖入来做动画。
2. 把人偶摆在地面上，Scale=(1,1,1)，Rotation=(0,0,0)。设置一次静态位置即可，后面不对 Actor Transform 打动画关键帧。
3. 使用关卡工具栏 **Cinematics / 场记板菜单 → Add Level Sequence**，保存为 `/Game/EmberTrial/Animations/Practice/Sequences/LS_M0_Attack_01`。
4. Sequencer 打开后，选中场景中的 M0_AnimationDummy，在 Sequencer **+ Track / 添加轨道 → Actor to Sequencer** 中添加当前选中 Actor。
5. 在该 Actor 的轨道行点 **+ Track → Control Rig → Control Rig Classes → CR_Mannequin_Body**。
6. 若该人偶已经自动带有一条同名 Control Rig 轨道，直接使用，不重复加第二条。不要同时叠加另一个动画播放轨道。

这是复用已有 Rig 的控制实例，不是修改 CR_Mannequin_Body 的 Rig Graph。支持的流程可对照 [Epic 5.5：Animating with Control Rig](https://dev.epicgames.com/documentation/en-us/unreal-engine/animating-with-control-rig-in-unreal-engine?application_version=5.5)。如果 Rig 列表没有这个类，先确认选的是 SKM_Manny，再发轨道菜单和 Mesh 名称截图，不创建新骨架或重绑全身。

### 3.3 时间轴

1. Sequencer 的 Display Rate 设为 **30 fps**，时间显示选择帧。
2. Playback Range 设为 **Start=0，End=31**，让第 30 帧也落在可播放范围内；Control Rig section 覆盖整个范围。
3. 本批关键姿势放在 **0、8、12、22、30 帧**，动画长度约 1 秒；最终精确时长以烘焙资产显示为准。
4. 先关闭 Auto Key，使用明确打关键帧的方法。预览用 Sequencer 的播放按钮，不是关卡工具栏 Play。

## 4. 制作简单的挥击

### 4.1 先验证控制器能驱动手臂

本地 Rig 资产中可以找到这些名称；真正可编辑状态需在 Sequencer 中验证：

```text
upperarm_r_fk_ctrl / lowerarm_r_fk_ctrl / hand_r_fk_ctrl
upperarm_l_fk_ctrl / lowerarm_l_fk_ctrl / hand_l_fk_ctrl
hand_r_ik_ctrl / arm_r_fk_ik_switch
root_ctrl / global_ctrl
```

1. 时间指针移到第 0 帧。展开 Control Rig，在 Anim Outliner 或控制器列表搜索 **upperarm_r_fk_ctrl**，选中它。
2. 在视口工具栏切为 **Rotate / 旋转**，选择 Local / 本地坐标，用旋转环小幅调整，检查右上臂是否跟随。
3. 再选 **lowerarm_r_fk_ctrl**，验证能弯曲右肘。名字中的 r / l 以角色自身左右为准。
4. 如果 FK 控制器移动了但手臂不动，可能是当前手臂由 IK 控制。先停止试改，发控制器名称、Anim Details 和 Rig 轨道截图，我确认当前切换值；不要猜测 0 / 1、同时改 FK 与 IK 或修改 Rig Graph。

保持 **root_ctrl、global_ctrl、脚部控制器和 Actor Transform** 不变。本批只做上肢占位，不追求复杂重心、脚步或手指动画。

### 4.2 五个关键姿势

**若视觉描述不易操作，直接使用新增的 [五帧 Location / Rotation / Scale 数值表](M0_05A_AttackPoseValues.md)。其中包含六个手臂控制器在五帧的完整值、具体填写面板、FK 开关与关键帧操作；已在 UE 5.5.4 临时 Rig 中求值。先做 0 和 12 帧确认，再补另外三帧。本次数值版先用 Linear 插值。**

通过上面的控制测试后，用右上臂 / 前臂 / 手控制器调整姿势；左上臂和前臂保持身体前方的警戒位置。数值表填写的是 Anim Details 中的控制器局部值，不是世界坐标或骨骼自身的 Transform。

| 帧 | 姿势 | 视觉标准 |
|---:|---|---|
| 0 | 准备 | 双臂自然弯曲，右手在胸腹前，左手保持警戒；双脚保持原位置 |
| 8 | 蓄力 | 右手略向身体右后方收，右肘弯曲；不要穿过躯干 |
| 12 | 前挥 | 右手快速挥向身体前方，手臂接近伸直但肘部保留弯曲 |
| 22 | 收回 | 右手收回至准备位置附近 |
| 30 | 回到准备 | 恢复第 0 帧同一姿势，便于后续退出动作时混合 |

每到一个指定帧：

1. 先移动时间指针，再调整姿势。
2. 选中本次用到的手臂 / 手控制器，鼠标焦点放在动画视口，按 **S**，给所选控制器记录 Transform key；也可以使用轨道的 Add Key 菱形按钮。
3. 检查对应轨道在该帧确实出现关键帧标记。**摆出姿势但没有 key，拖动时间轴后姿势不会按计划保留。**
4. 为保持左手稳定，每个关键姿势也记录左臂当前值；一套控制器在五个时间点都打 key，便于排查。

恢复第 0 帧姿势：选中相关控制器轨道在第 0 帧的关键帧点，Ctrl+C，将时间指针移到第 30 帧后 Ctrl+V，确认粘贴到 30 帧。选择的是关键帧点，不是复制 Actor 或整段轨道。

按 Sequencer 播放检查蓄力较慢、挥出较快、收回清楚即可。不要求本批达到成品战斗动画质量；不要用跳跃动画改名冒充攻击。

若曲线插值导致明显反向甩手或穿模，可先将相关关键帧的插值改为 Linear 验证动作路径，再决定是否需要平滑；不能靠疯狂提高播放速度掩盖错误。

## 5. 烘焙成 Animation Sequence

本工程已完成本节。复现时保留 30 fps、播放范围 Start=0 / End=31，先停止 Sequencer 播放。已核对本次输出为 31 个帧间隔、32 个采样点、1.0333333 秒。

1. 保存 LS_M0_Attack_01 和动画试场地图。
2. 若 Sequencer 左侧仍有控制器名称搜索，先清空搜索。找到最上层人偶 **M0_AnimationDummy** 的 Actor 绑定行（其下面才是 CR_Mannequin_Body 和各控制器），右键这行，选择 **Bake Animation Sequence**。不要右键具体手臂控制器或紫色 Control Rig section 来找该菜单。
3. 保存为 `/Game/EmberTrial/Animations/Practice/Clips/AS_M0_Attack_01`。
4. 导出范围用本次播放范围；保留骨骼变换导出，关闭 **Export in World Space**（若面板提供），避免把场景中人偶的摆放位置烘焙为世界位移。其余导出设置先保持默认。
5. 记录导出面板与结果的采样率 / 帧数 / 时长，确认与约 1 秒的片段一致。如果出现多分钟空白片段，先修正播放范围再导出。

Sequencer 可以把 Control Rig 结果烘焙成可在游戏中使用的 Animation Sequence，参见 [Epic Animation Sequences：Sequencer](https://dev.epicgames.com/documentation/unreal-engine/animation-sequences-in-unreal-engine)。

双击烘焙后的 AS_M0_Attack_01：

- 预览 Mesh 选择 SKM_Manny，确认 Skeleton 资产，单独播放仍能看到挥击。
- 本批 **Enable Root Motion=False**。关闭 Root Motion 提取本身不能证明动画没有位移，仍要检查 root 是否保持原位。
- 若预览界面启用了“跟随角色”的镜头，切到固定参考视角观察地面；确认角色没有整个人向前漂移后瞬移回起点。
- 关闭 Sequencer 再单独重开 Animation Sequence 播放，避免把 Sequencer 的实时控制误当成烘焙结果。

保留 Level Sequence 作为可编辑源，不删除它。Animation Sequence 是本批输出，之后才创建 Montage 接入角色。若烘焙为 T / A Pose、姿势不动或 Rig 列表不匹配，提供截图，我直接排查，不要求你先分析骨架底层问题。

## 6. 本批验收与恢复工作场景

| 检查 | 预期 |
|---|---|
| 当前玩家绑定 | 已记录 Mesh / Skeleton / Anim Class / AnimBP Parent，未修改这些设置 |
| 来源 | 现有 UE 模板 Rig + 自己打的关键帧，无外部动画包 |
| Sequencer | 0 / 8 / 12 / 22 / 30 帧姿势有区别，准备 / 蓄力 / 挥出 / 收回清楚 |
| 烘焙 | AS_M0_Attack_01 能在关闭 Sequencer 后独立播放，不是静止姿势 |
| Root | 固定参考下没有整体漂移或循环回跳；双脚未因移动 root 离开原位置 |
| 保存 | 地图、Level Sequence、Animation Sequence 已保存，可重新打开 |
| 回到原场景 | 重新打开 `/Game/ThirdPerson/Maps/ThirdPersonMap`，玩家 Mesh / Anim Class 与默认地图设置保持原值 |

返回 ThirdPersonMap 后只做一次快速回归：移动 / 转向 / 跳跃、E 交互、P 暂停 / 恢复正常。本批只在独立素材试场工作，不应改变这些功能；发现变化时直接反馈。

```text
M0-05A 完成 / 卡在第 __ 节
当前玩家 Mesh：
当前玩家 Skeleton：
当前 Anim Class / AnimBP Parent：
Control Rig 控制手臂：正常 / 实际结果
LS_M0_Attack_01 关键帧预览：正常 / 实际结果
AS_M0_Attack_01 单独播放：正常 / 实际结果
动画 Skeleton / 帧数 / 时长：
固定参考下 root 漂移：无 / 具体情况
地图 / Sequence / 动画已保存：是 / 否
已回 ThirdPersonMap，基础输入 / E / P：正常 / 实际结果
实际用时：约 __ 分钟
```

动画问题优先提供约 5–10 秒预览视频；控制器或导出菜单找不到时给截图；编译 / 导出报错用文字。若只有姿态或朝向问题，一张第 0 帧与第 12 帧的截图也能帮助定位。

本批通过后，再依据真实制作成本下发闪避 / 受击占位和 Montage 接入步骤；M0-05 总任务保持进行中。
