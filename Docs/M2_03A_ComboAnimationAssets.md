# M2-03A：第2、3段攻击动画与Montage素材

2026-10-15，固定 **UE5.5.4**。前置[M2-02已验收](Reports/M2_02_Verification.md)，[本章路线](M2_03_LightComboRoadmap.md)。**本批按用户约22:37反馈与文件 / 日志核对验收通过，无待补项。**六新素材已保存，当前96Source / 50cpp / 来源57项；实际手动50 / 等待10，详见[A报告](Reports/M2_03A_Verification.md)。下方步骤保留供复查，无需重复执行；B代码待下发。

原下发预算（非当前待办）：手动 **80–140分钟**；录入 / 烘焙55–95、两Montage10–20、分组检查5–10、保存记录5–10。实际以用户报告50 / 等待10为准，未拆检查时间，不要求做满预算。

## 1. 做完后应得到什么

| 段落 | 本批素材与视觉 | 时长 / 命中通知 |
| --- | --- | --- |
| 1，已有 | 原M0前挥；只复用，不重烘焙 | 原约1.033秒；8–14帧 |
| 2，新建 | 右手前伸后回抽到右肋，与第1段运动顺序不同 | 约1.033秒；8–14帧 |
| 3，新建 | 较短的蓄力 → 前击 → 收回，节奏更紧 | 约.700秒；6–12帧 |

本批结束可以独立预览三段素材，**正式左键仍只播放原第1段，这是预期**。连段 / 缓冲 / 取消由后续B / C代码实现；不在角色蓝图加临时按键、Delay或自动连播。玩家 / 敌人GA、旧Montage、ABP_Manny / Quinn、输入映射保持原状。

数值沿用[M0已完成的FK姿势](M0_05A_AttackPoseValues.md)，改变第2段顺序与第3段时序，并显式固定躯干。新序列与烘焙结果尚未在UE求值或看过蒙皮，仍需本批预览；这是上肢占位，不是最终武器 / 全身战斗动画。重要模型后续仍可接Blender。

## 2. 准备与建立空白 Sequence

### 2.1 保存并进入动画试场

1. Stop PIE → Save All。打开已有`/Game/EmberTrial/Maps/L_M0_AnimationLab`，继续使用原 **M0_AnimationDummy（SKM_Manny）**。不改默认地图 / GameMode。
2. 保存并关闭已有Sequencer标签；一次只开当前新动作。不要用BP_ThirdPersonCharacter作为制作人偶。
3. 在Content Browser建立以下三个目录。这里的新`Sequence`仍用单数，原Practice目录无需移动 / 改名。

```text
/Game/EmberTrial/Animations/Combo/Sequence
/Game/EmberTrial/Animations/Combo/Clips
/Game/EmberTrial/Animations/Combo/Montages
```

### 2.2 创建第2段

1. 在新`Combo/Sequence`文件夹空白处右键 → **Cinematics / 过场动画 → Level Sequence**，命名`LS_ET_Light_02`，双击打开。由Content Browser创建资产即可，不需要给关卡额外放自动播放的LevelSequence Actor。
2. 在关卡Outliner选中已有M0_AnimationDummy，回到Sequencer **+ Track → Actor to Sequencer → 当前选中的人偶**。也可把Outliner中的同一个人偶拖到Sequencer左侧列表。
3. 在人偶绑定行 **+ Track → Control Rig → Control Rig Classes → CR_Mannequin_Body**。若自动已有一条同名Control Rig轨道，使用它；只保留一条驱动该人偶的Rig，不叠加旧AS或另一Rig。
4. Display Rate设 **30fps**；时间轴显示 **Frames**。Playback Options中如勾选Playback Range Locked，先取消；设置 **Start=0、End=31**，Enter提交，再打开核对。Control Rig section覆盖整个范围。
5. End是排他端点，最后一个姿势在30帧。不要把End改30，也不要只缩放底部可视范围。
6. 关闭Auto Key，按第3 / 4节明确打关键帧。先保存这个空白新Sequence，核对顶部标题确为`LS_ET_Light_02`。

第3段待第2段烘焙后再按同样步骤另建`LS_ET_Light_03`，**从空白Sequence建立**，End改21；不Duplicate旧制作源，避免继承试改key、错误绑定和旧导出关联。

Control Rig添加轨道与S键打Transform key的操作见[Epic 5.5说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animating-with-control-rig-in-unreal-engine?application_version=5.5)。本指南沿用本工程已完成的界面路径；若菜单缺项，截图菜单与选中人偶即可，不修改Rig Graph。

## 3. 数值填法与固定条件

### 3.1 填写面板

1. 停止Sequencer播放，在右侧 **Anim Outliner** 搜索完整控制器名并选中，例如`upperarm_r_fk_ctrl`。
2. 在 **Anim Details** 中填写控制器当前 **Location / Rotation / Scale**；不是场景Actor Details、骨骼Skeleton Tree，也不是Rig Initial / Offset Transform。
3. 表格括号都是 **X / Y / Z**，Rotation单位度：X=Roll、Y=Pitch、Z=Yaw。若界面按Pitch / Yaw / Roll显示，按字段名对应，不能照显示顺序抄。
4. 全部是最终数值，不在旧值上累加。`-0`或约1e-6可按浮点误差看待。人偶本身的Rotation保持原零值、Scale1；只做原地动画。

### 3.2 开关与其他控制器

| 项目 | 本批所有关键帧 |
| --- | --- |
| arm_r_fk_ik_switch / arm_l_fk_ik_switch | False，两臂FK |
| spine_fk_ik_switch | False，使用spine_01_ctrl |
| 下文七控制器Location | (0,0,0) |
| 下文七控制器Scale | (1,1,1) |
| 父级 / Space | 模板默认，不切换 |
| global_ctrl / root_ctrl / body_ctrl、腿 / 脚 / 手指等 | 保持模板默认，不对它们打位移key |
| 人偶Actor Transform | 原静态位置，Rotation0 / Scale1，无动画Transform轨道 |
| 插值 | Linear，先防止曲线过冲 |

三个开关为布尔值，不填写Transform。若有开关动画通道，在0帧用其Add Key菱形记录False，整段不得出现True关键帧。**七控制器Location=0不代表把全身骨骼位置清零**；不要改骨长或Root Lock来掩盖漂移。

### 3.3 一帧的完整操作

1. 时间指针放到表内帧号，例如0。
2. 选中一个控制器，按表填Location / Rotation / Scale，分别Enter提交。
3. 保持控制器选中，鼠标焦点放到动画视口，按 **S** 记录本帧Transform key。也可用相应Transform通道的Add Key菱形；确认记录了Location / Rotation / Scale，不能只改值不打key。
4. 依次处理本帧七个控制器，即使手腕 / 左臂数值没变也记录。
5. 切到别帧再回来，确认数值保留。每段先做0与主姿势帧，方向正常后补另三帧，不等两段全做完才看效果。
6. 五帧齐全后选本次七控制器的关键帧点，右键 → **Linear**；不要选择Actor或把Rig改Layered。

## 4. 第2段：回抽

与原第1段的顺序不同：第8帧先前伸，第12帧再回抽；这是复用同一组已知FK姿势改变运动顺序。不要照原攻击把8帧填成蓄力、12帧填成前挥。先做0与12帧，再补8 / 22 / 30。

### 4.1 第0帧：准备

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -20, 15) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.2 第8帧：右手前伸

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 75) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, -20) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.3 第12帧：向右肋回抽

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 10, -55) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 80) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.4 第22帧：收回

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -15, 25) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 60) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.5 第30帧：回到准备

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -20, 15) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

**完成本段后：**在0与30帧来回切换，七控制器应完全相同；两臂始终FK、躯干不移动，右手能看出本段顺序。保存、关闭并重开该Sequence一次，核对最后一帧与左上臂Z=25后再烘焙。

## 5. 第3段：短促前击

先保存并关闭第2段Sequencer，再按第2节建立全新的LS_ET_Light_03。范围End=21，五帧改为0 / 6 / 10 / 15 / 20；不要把第2段31帧范围带过来。先做0与10帧，再补6 / 15 / 20。它使用已知前挥姿势，但蓄力与收招更短，用于辨认第3段节奏。

### 5.1 第0帧：准备

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -20, 15) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 5.2 第6帧：短蓄力

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 10, -55) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 80) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 5.3 第10帧：前击

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 75) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, -20) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 5.4 第15帧：收回

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -15, 25) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 60) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 5.5 第20帧：回到准备

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -20, 15) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

**完成本段后：**在0与20帧来回切换，七控制器应完全相同；两臂始终FK、躯干不移动，右手能看出本段顺序。保存、关闭并重开该Sequence一次，核对最后一帧与左上臂Z=25后再烘焙。

## 6. 分别烘焙两段 Animation Sequence

第2段做完就烘焙 / 预览一次，再开始第3段；不要等两段都做完才检查是否烘焙到了正确资产。

| 制作源 | Start / End / FPS | 输出Animation Sequence | 按本工程既有导出方式预计 |
| --- | --- | --- | --- |
| LS_ET_Light_02 | 0 / 31 / 30 | `/Game/EmberTrial/Animations/Combo/Clips/AS_ET_Light_02` | 31帧间隔 / 32采样点 / 1.033333秒 |
| LS_ET_Light_03 | 0 / 21 / 30 | `/Game/EmberTrial/Animations/Combo/Clips/AS_ET_Light_03` | 21帧间隔 / 22采样点 / .700000秒 |

1. 停止播放、保存当前新Sequence；检查顶部名称与上表一致。
2. 清空Sequencer控制器搜索。右键最上层 **M0_AnimationDummy的Actor绑定行** → **Bake Animation Sequence**；不是右键某个手臂控制器或Control Rig section。
3. 按上表目录 / 名称输出，使用当前播放范围、30fps；保留骨骼变换导出，关闭 **Export in World Space**（如果提供）。不要覆盖AS_M0_Attack_01或导出到Practice。
4. 如果要修正本批新动画，修改LS后必须重新Bake对应的新AS；仅保存LS不会自动更新已烘焙AS。导出不支持同名覆盖时可临时用`_v2`并告诉我实际路径，不先删旧资产。
5. 关闭Sequencer，再从Content Browser**单独打开刚导出的AS**。确认Skeleton=`SK_Mannequin`、Rate Scale=1、Enable Root Motion=False；以界面实际采样率 / 采样点 / 秒数记录，不能把End数值当秒数。
6. 在新AS的Preview Mesh选择SKM_Quinn_Simple并播放。使用固定镜头 / 地面参照，查看root / 双脚位置与首尾手臂；不靠跟随镜头或Root Motion提取隐藏漂移。右手运动与LS应一致。
7. 保存新AS，关闭重开其中一段确认仍是最新动作。保留两份LS作为可编辑源；本批不改CR_Mannequin_Body资产。

遇到T/A Pose、全程静止、长度多出空白或骨架不符时，先停在当前段，给AS路径、实际时长与导出面板截图。root整体漂移需定位导出空间 / Actor轨道，不靠Force Root Lock强行盖住。

## 7. 新建两个独立 Montage

### 7.1 资产位置与动画引用

关闭Sequencer。对新AS分别右键 → **Create → Create AnimMontage**。若出现保存对话框直接选目标目录；若UE自动在Clips生成资产，先重命名为下表名称，再从Content Browser拖到Montages目录并选择 **Move Here**。不要选择Create Child Montage，也不要Duplicate旧Practice Montage。

| 新Montage最终路径 | 唯一动画Segment引用 |
| --- | --- |
| `/Game/EmberTrial/Animations/Combo/Montages/AM_ET_Light_02` | AS_ET_Light_02 |
| `/Game/EmberTrial/Animations/Combo/Montages/AM_ET_Light_03` | AS_ET_Light_03 |

**若自动生成的名字含`_Montage`后缀，创建当下就按表重命名**，不沿用到后续代码。只在UE Content Browser中重命名 / 移动；完成后新Combo目录内如产生Redirector，在该新目录右键 **Fix Up Redirectors in Folder**。不对整个Content做无关整理。

这是从AS创建Montage的标准路径，参考[Epic Montage说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-montage-in-unreal-engine?application_version=5.5)。网页版本可能跳到新版，具体字段按本机5.5.4头文件与已完成M0教程核对。

打开每个新Montage，单击 **Slot轨道里的动画长条Segment**，在Details核对：

| 字段 | 第2段 | 第3段 |
| --- | ---: | ---: |
| Animation Reference | AS_ET_Light_02 | AS_ET_Light_03 |
| Start Time | 0 | 0 |
| End Time | 实际AS全长，约1.033333 | 实际AS全长，约.700000 |
| Play Rate | 1 | 1 |
| Loop Count | 1 | 1 |

Segment从时间轴0开始，只保留这一个动画段。Starting Position通常只读，不为修改它随意移动动画长条；如明显不在0，Undo刚才的拖动恢复。Slot Name用已有 **DefaultGroup.DefaultSlot**，不新建Slot。保留一个Default Section，不连接回自身形成循环；预览工具栏的循环开关不等于运行时Loop Count。

### 7.2 混合与预览

在Montage的 **Asset Details**（不是选中Segment后的小节Details）清空搜索，展开 **Blend In → Blend Time**与**Blend Out → Blend Time**，各设 **0.10秒**。Enable Auto Blend Out=True，Rate Scale=1。若看不到Asset Details，在Window菜单启用该面板；找不到时发面板截图，不去GA找同名字段。

Preview Mesh使用SKM_Quinn_Simple。关闭预览循环，分别播放一次，确认第2段回抽、第3段更短的前击、结束后无卡最后一帧。保存新Montage；不修改原ABP Slot或玩家Anim Class。

### 7.3 在新 Montage 中加已有命中 NotifyState

必须放在**各自Montage的Notifies轨道**，不是LS、AS或AnimBP。两段各一条`ET Attack Window`，使用已编译的ETANS_AttackWindow，不新建原生类 / 蓝图。

1. Notifies轨道左侧菜单可新建一条Notify Track，命名`ET_HitWindow`。
2. 在该轨道空白处右键 → **Add Notify State → ET Attack Window**。不是普通Notify，也不是Montage Notify Window。
3. 取消多选，只选中刚建状态条；**右键状态条中间的本体**，在菜单中设置下表`Notify Begin Time`，输入后 **Enter提交**。
4. 再次右键同一条本体，设置`Anim Notify State Duration`，Enter提交。时间不在普通Details；如果菜单只有Add Notify，说明点到了轨道空白。

| Montage | Notify Begin Time，秒 | Duration，秒 | Notify Frame | Anim Notify State Frames | 结束帧 |
| --- | ---: | ---: | ---: | ---: | ---: |
| AM_ET_Light_02 | .266667 | .200000 | 8 | 6 | 14 |
| AM_ET_Light_03 | .200000 | .200000 | 6 | 6 | 12 |

`Anim Notify State Frames`是持续帧数，不填结束帧14 / 12；Duration不是End Time。只点击空白而不Enter不保证应用输入。30fps下以右键菜单的开始帧 / 持续帧复核即可，显示小数位少不需要反复改。

5. 左键选该NotifyState，在Details搜索 **Montage Tick Type**，设 **Branching Point**。原生类已强制Branching Point；若此项不显示，先确认条名和原生类，不另建替代通知。
6. 重新右键确认时间，保存Montage。每个AM只有一条命中状态；A不加连段 / 恢复取消通知，等后续原生类就绪再添加。
7. 编辑器预览不会产生Gameplay的Arm / Candidate / Damage日志，当前类关闭了编辑器触发，这是正常的。本批只核对姿势、窗口位置、资产保存，不把预览当真实命中证明。

本机5.5.4 `SAnimNotifyPanel.cpp`中这四个时间输入项都在状态条右键菜单，且Enter才提交；详细故障对照可查[已修正的旧指南9.2](M1_04B1_AttackTrace.md#92-在montage本身添加notifystate)。不用再到Details寻找Notify起止数值。

## 8. 分组检查与回到正式地图（约5–10分钟）

### 8.1 素材检查，约4–7分钟

| 一组只查一次 | 通过条件 |
| --- | --- |
| 两份新LS | 三FK开关False，七控制器五帧齐、Linear；第2段0与30 / 第3段0与20一致，左上臂Z=25 |
| 关闭Sequencer后两AS独立播放 | Skeleton正确、30fps、时长约1.033 / .700秒、动作顺序可辨、固定参照下root / 脚不漂移 |
| 两新AM | 引用各自AS，DefaultSlot、Loop1、Blend .10 / .10、自动退出；各一条原生命中状态，开始8 / 6帧、均持续6帧 |
| 保存重开 | 新LS / AS / AM均在Combo目录，重开后动作 / 通知数值仍在；旧攻击资产不被覆盖 |

本批只新增孤立素材，**不跑C++ Build、Refresh-CLion、旧Dodge / Resources / Lifecycle测试或Cook**，也不搭临时角色图。现有项目Validator主要检查正式GA / 身体，不会对每个新Montage单独打印`M1-07C2 Valid`；本批不以该打印作为验收条件，不为此新增校验器。

### 8.2 正式地图短基线，约1–2分钟

1. 保存关闭Sequencer，打开`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，只Play一次。
2. 空地站定单击左键：仍是原单段攻击；按2仍150cm / .30秒并一次扣25，后续正常恢复。
3. 移动 / 转向 / 跳跃、E一次提示 / 交互、P打开后用一种已支持方式继续；不重复十次菜单或敌人死亡矩阵。
4. Stop，Save All，保持正式地图打开。新Montage不赋给GA / PracticeAction，不改输入和敌人配置；界面里的左键尚不能连段不算本批失败。

## 9. 完成反馈与问题反馈

```text
M2-03A 完成 / 卡在第__节
六新资产实际名称与目录；旧资产未覆盖：
第2段五帧 / 第3段五帧、FK开关 / Linear / 首尾：
两新AS独立播放；Skeleton / 实际采样率 / 秒数（按2、3顺序）：
Quinn_Simple预览 / root与双脚固定：
两新AM各自AS / DefaultSlot / Loop1 / Blend .10 / .10 / 自动退出：
两命中通知开始帧8 / 6，均持续6帧；各只有一条：
正式左键仍单段、2 / E / P与移动转向跳跃短基线：
已回正式地图；Source不改、Save All：
失败或占位动作最明显的问题（无填无）：
完成时分（如21:18）/ 实际手动用时 / 烘焙或加载等待：
```

姿势问题给当前帧、控制器名、实际Transform，并截图人偶全身 + Anim Details + 时间指针；动作顺序 / 跳变用5–10秒预览视频。菜单找不到就截菜单，不用猜相似字段。导出 / 保存报错贴第一条Error原文；普通运行问题说明输入 / 当时状态 / 实际现象，日志给路径或大致时分，我读取。不要为记录手摘几十行。

数值数据另存[本批JSON](Data/M2_03A_AttackPoseValues.json)，仅供查阅 / 校准，不需要导入UE或执行Python。完成后我核对真实资产，再下发B；当前没有连段实现或新测试Success。

## 10. 本批 Commit / Push（固定放在最后）

A实物已完成，按[A验收报告最后清单](Reports/M2_03A_Verification.md#4-本次-commit--push固定放在最后)Stage六新资产、来源清单57项与最新文档；下列为原范围，文件路径按真实保存结果。

- 六新资产：`Animations/Combo/Sequence/LS_ET_Light_02.uasset` / `LS_ET_Light_03.uasset`，`Combo/Clips/AS_ET_Light_02.uasset` / `AS_ET_Light_03.uasset`，`Combo/Montages/AM_ET_Light_02.uasset` / `AM_ET_Light_03.uasset`；均在`E:\UE_Project\Demo\EmberTrial\Content\EmberTrial`下。临时_v2如确实采用，报告最终路径再按真实引用处理。
- 制作地图`L_M0_AnimationLab.umap`若因合法人偶配置实际变化，一并保存 / 检查差异；正式地图 / 玩家GA / 输入 / AnimBP / 旧Practice动画 / Rig无计划修改，不为提交制造无关差异。新目录移动后只处理该目录Redirector。
- 本指南、分批路线、数值JSON、A验收报告、AssetManifest新增六行真实SHA，以及Progress / Backlog / M2路线 / Architecture / VersionControl / AcceptanceWorkflow / Introduction的最新状态；当前清单57项，实物已核对，不重复创建素材。
- 上一批C三新AM / 八旧Source与C / 整章验收收尾如尚未提交，按[C报告](Reports/M2_02C_Verification.md#5-本次-commit--push固定放在最后)重新Stage最新内容；这是前一批已有改动，**本批没有新C++**。Raw / 日志 / Binaries / Intermediate / Saved、生成CMake / IDE缓存 / 旧包仍留本地，不Force Add。

CLion刷新Changes → Stage本批真实新资产 / 最新文档（已Stage但又修改的文件重新Stage）→ 检查Staged Diff / LFS指针 → Commit → Push。可用`feat: add second and third light attack animation placeholders`；仅指南用`docs: prepare light combo animation assets`。Git仍由用户操作，本批预览通过不写成整个M2-03已完成。
