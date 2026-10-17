# M2-04B1：治疗动画素材与Montage

2026-10-17，固定 **UE5.5.4**。前置[A资源事务已验收](Reports/M2_04A_Verification.md)，[治疗路线](M2_04_HealingRoadmap.md)。本批已下发，**待用户手动制作**：三份新素材，完整五帧 / 七控制器数值如下。实际仍105Source / 56cpp / 来源58；三素材落地后预计来源61，当前不预填已完成资产。累计用户手动2451 / 已明确等待240分钟保持。

预计手动 **45–75分钟**：制作 / 烘焙30–45，Montage5–10，相关检查5–10，保存记录5–10；加载 / 烘焙等待按实际另记。这些是预算，不计为已投入时间。收到完成反馈后先保存验收报告，**无待补项时同一轮直接下发B2完整指南**，不再等待“进入下一章”；发现问题只处理具体缺口。C++手动 / Git用户操作的约定继续执行。

## 1. 本批结果与视觉标准

动作采用**原地聚气式治疗占位**：从原战斗准备姿势，右手回收到右腰肋外侧，左臂略收、保持护身；停留一小段，再收回准备姿势。没有药瓶 / 握持接触要求，也不做迈步或前挥命中动作。这样后续可在明确停留段提交一次治疗。

| 时点 | 含义 | 本批应观察到的结果 |
| --- | --- | --- |
| 0帧 | 准备 | 与原占位动作相同的双臂准备，左上臂Z=25 |
| 8帧，约.267秒 | 收拢完成 | 右手相对0帧明显向右腰肋回收，左肘略收；不抬到头后 |
| 8–18帧 | 稳定停留 | 七控制器8 / 18帧完全相同，期间姿势不额外弹动 |
| **15帧，.500秒** | **后续治疗生效点** | 位于停留段内部；本批只预留时点，B2才加原生Notify |
| 22帧，约.733秒 | 收回 | 右臂从回收姿势回到身体前方，连续衔接 |
| 30帧，1.000秒 | 准备 | 与0帧七控制器数值完全相同 |
| 排他End=31，约1.033秒 | 播放范围结束 | 最后姿势位于30帧，烘焙沿用原工程范围语义 |

新组合与时序**尚未在UE求值或查看蒙皮**。表中各个旋转向量复用已完成M0 / Combo数值，不等于这份新动作已验证。先做0与8帧并看一眼全身，符合上述方向再补三帧；若朝向明显相反 / 穿过身体，停在该帧按第8节给材料，不自行改骨架、Rig Graph或左右镜像Scale。

本批结束只在LS / AS / Montage中独立预览；正式Q仍未接入，治疗次数仍通过A的原生开发命令可用。**不把新AM赋给Attack / Dodge / PracticeAction，不新建GA / IA / Notify / Widget，不改原连段与敌人资源。**生效前后取消、真实回血与一次扣数由B2接入后验证。

## 2. 准备、路径与建立空白Sequence

### 2.1 进入原动画试场

1. Stop PIE → Save All；不需要关闭UE、运行C++ Build或刷新CLion索引。
2. 打开已有`/Game/EmberTrial/Maps/L_M0_AnimationLab`，使用原**M0_AnimationDummy（SKM_Manny）**。制作网格 / Rig与M0相同；后面再用Quinn_Simple独立预览烘焙AS。不要用正式玩家角色制作，不改默认地图 / GameMode。
3. 保存并关闭其他Sequencer标签，停止它们的播放。只让当前新Sequence驱动这个人偶。
4. 在Content Browser建立以下新目录；`Sequence`使用单数，与原工程习惯一致。没有Healing目录是正常的，不移动原Practice / Combo。

```text
/Game/EmberTrial/Animations/Healing/Sequence
/Game/EmberTrial/Animations/Healing/Clips
/Game/EmberTrial/Animations/Healing/Montages
```

最终只新增以下三份资产：

| 类型 | 最终资源路径 |
| --- | --- |
| 可编辑制作源 | `/Game/EmberTrial/Animations/Healing/Sequence/LS_ET_Heal_01` |
| 烘焙动画 | `/Game/EmberTrial/Animations/Healing/Clips/AS_ET_Heal_01` |
| Montage | `/Game/EmberTrial/Animations/Healing/Montages/AM_ET_Heal_01` |

### 2.2 新Sequence与单条Control Rig

1. 在Healing/Sequence文件夹空白处右键 → **Cinematics / 过场动画 → Level Sequence**，命名`LS_ET_Heal_01`，双击打开。创建全新空白资产，不Duplicate旧攻击 / 闪避Sequence；不在关卡新增自动播放LevelSequence Actor。
2. 在关卡Outliner选中M0_AnimationDummy。Sequencer左侧 **+ Track → Actor to Sequencer → 当前选中人偶**；也可从Outliner把该人偶拖到Sequencer轨道列表。
3. 在这个人偶绑定行 **+ Track → Control Rig → Control Rig Classes → CR_Mannequin_Body**。如果已经自动出现一条同名Rig轨道，复用它；同一人偶只留这一条Rig驱动，不叠加旧AS / 另一Rig，不切Layered。
4. Sequencer工具栏Display Rate设**30fps**，时间显示设**Frames**。Playback Options如有Playback Range Locked，先取消；设 **Start=0 / End=31**，Enter提交，再打开核对。Control Rig section覆盖全范围。
5. End=31是排他端点；最后一个姿势key在30。缩放底部可视范围不会修改播放范围，也不要把End设30。人偶Actor保持原位置、Rotation0 / Scale1，不给Actor Transform录位移。
6. 关闭Auto Key，保存新Sequence；核对顶部标题是LS_ET_Heal_01，再开始填值。

添加Control Rig轨道与选中控制器按S打Transform key的方式已核对[Epic UE5.5 Control Rig说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animating-with-control-rig-in-unreal-engine?application_version=5.5)，具体面板沿用本工程已操作的M0 / M2-03A。缺项时截当前菜单与所选对象，不用相似菜单替代。

## 3. 数值填在哪里、怎样记录一帧

### 3.1 固定条件

| 对象 / 设置 | 整段要求 |
| --- | --- |
| `arm_r_fk_ik_switch` | False，右臂FK |
| `arm_l_fk_ik_switch` | False，左臂FK |
| `spine_fk_ik_switch` | False，使用spine_01_ctrl；下表躯干旋转始终0 |
| 七控制器Location | 每帧(0,0,0)，相对控制器Offset的局部值 |
| 七控制器Scale | 每帧(1,1,1)，不填-1做镜像 |
| 控制器Space / 父级 | 模板默认，不做空间切换 |
| global / root / body、头、锁骨、腿 / 脚 / 手指等 | 保持模板默认，不额外打位移 / 姿势key |
| 插值 | 七控制器全部姿势key使用Linear |

三个FK/IK开关是布尔控制器，不填写Location / Rotation / Scale。在0帧选择开关并设False，使用相应通道旁Add Key菱形记录；若已有开关通道，整段不能出现True。不要把开关False与勾选“使用IK”混淆。

### 3.2 七行数值的填写方式

1. 停止播放，先把时间指针放到准确帧号，例如0。
2. 在右侧**Anim Outliner**搜索并选择完整控制器名，例如`upperarm_r_fk_ctrl`。在**Anim Details**确认标题也是这个控制器；不是普通Actor Details、Skeleton Tree骨骼或Rig的Initial / Offset Transform。
3. 表中括号均按**X / Y / Z**。Rotation单位度，X=Roll、Y=Pitch、Z=Yaw；若界面按Pitch / Yaw / Roll显示，按字段名对应，不按显示排列抄。全部填最终值，不叠加旧值。
4. 填该控制器Location / Rotation / Scale，每组Enter确认。保持控制器选中，鼠标焦点放动画视口，按**S**打本帧Transform key；也可用Transform通道Add Key菱形。
5. 确认位置 / 旋转 / 缩放都记录，再做这一帧其余六行。即使数值没变也在规定帧记录，尤其8 / 18的停留与0 / 30的首尾。
6. 切到别帧再回来，数值仍在才算打key成功。先完成0与8帧的七行，观察全身方向，再录18 / 22 / 30。
7. 五帧完成后，框选本次七控制器的姿势key点 → 右键 **Linear**。不要把布尔开关当Transform曲线处理；无需给Rig换插值算法或调整Offset。

`-0` / 约1e-6可看作浮点显示误差。Location0仅限表中七个控制器，**不代表把全身骨骼Location清零**；骨长与父子偏移由模板Rig处理。

## 4. 五帧完整Location / Rotation / Scale

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

### 4.2 第8帧：收拢完成

右手向右腰肋回收，左肘略收。先与0帧对比方向；不要求双手贴合或接触不存在的药瓶。

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 10, -55) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 80) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 30) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 85) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.3 第18帧：同姿势停留结束

**完整重复8帧数值**，形成8–18的停留。后续15帧生效点位于两帧中间，本批不额外打15帧姿势key、不创建生效通知。

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 10, -55) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 80) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 30) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 85) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.4 第22帧：收回

右臂离开回收姿势，向原准备位置过渡；此处沿用原占位动作的收回值。

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

与0帧七行完全相同；左上臂Z保持25。

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -20, 15) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

**保存前一次小检查：**0↔30应一致，8↔18也一致；把指针放15帧，应与8 / 18同姿势。连续播放一遍，应是收拢 → 停留 → 收回，腿 / 脚与人偶整体位置保持。保存、关闭并重开LS一次，确认8 / 18与左上臂0帧Z=25仍保留，再烘焙。

数值数据另存[本批JSON](Data/M2_04B1_HealPoseValues.json)，供查阅与后续核对，**不需要导入UE或运行Python**。数据与本表来自同一份姿势定义；新动作尚无Rig求值 / 蒙皮通过结论，不能用JSON替代预览。

## 5. 烘焙AS_ET_Heal_01并独立预览

### 5.1 正确输出

1. 停止播放，保存LS_ET_Heal_01；核对Start0 / End31 / Display Rate30与Control Rig section范围。
2. 清空Sequencer左侧控制器搜索，**右键最上层M0_AnimationDummy的Actor绑定行 → Bake Animation Sequence**。不要右键手臂控制器或Rig section，也不是Bake To Control Rig。
3. 输出目录选`/Game/EmberTrial/Animations/Healing/Clips`，名称`AS_ET_Heal_01`。沿用前几批的骨骼变换导出 / 当前播放范围 / 30fps；如提供Export in World Space，设False。不覆盖M0 / Combo动画。
4. 完成后关闭Sequencer，避免它继续驱动试场人偶干扰判断；从Content Browser**单独打开AS_ET_Heal_01**。
5. 在AS中确认Skeleton为**SK_Mannequin**、Rate Scale=1、Enable Root Motion=False。不要为掩盖漂移去开启Root Motion或修改Force Root Lock。
6. Preview Mesh选择**SKM_Quinn_Simple**，单独播放，固定镜头 / 地面参照下检查root / 双脚、不变的站立位置与手臂首尾。不要仅凭跟随镜头判断无漂移。

按本工程既有Bake路径，预期输出约**31帧间隔 / 32采样点 / 1.033333秒，30fps**。播放范围End31不是31秒，也不是最后姿势帧；第15帧对应.500秒。**实际AS信息优先**：记录看到的采样率、帧数或采样点数、秒数。界面没找到某项可填“未找到”，不要凭End猜数字，我在验收时核对；若明显多出空白、远长于约1秒或全程静止，先停在当前资产排查。

### 5.2 修改与重新烘焙

LS修改不会自动更新已经烘焙的AS。若修姿势，保存LS后重做5.1的Bake，更新**同一目标AS_ET_Heal_01**。若导出器不支持同名覆盖，临时用_v2并报告完整路径；先保留旧资产，不直接在资源管理器覆盖 / 删除uasset。Montage随后必须明确引用最终那份AS，不能让预览看新AS、AM仍放旧AS。

保存AS，关闭重开一次确认最新动作。若SKM_Manny正常而Quinn_Simple明显异常，发两者相同帧的预览和AS Skeleton，暂不做重定向、改骨架或修改原AnimBP。

## 6. 独立Montage与后续生效点

### 6.1 创建与Segment

1. 关闭Sequencer，对AS_ET_Heal_01右键 → **Create → Create AnimMontage**。这是从动画创建的新Montage，不是Create Child Montage，也不Duplicate原攻击 / 闪避AM。
2. 最终命名**AM_ET_Heal_01**，目录`/Game/EmberTrial/Animations/Healing/Montages`。若UE自动在Clips生成带_Montage后缀的文件，在Content Browser内重命名、拖到Montages → **Move Here**。不在资源管理器移动uasset。
3. 只对新Healing目录必要时右键Fix Up Redirectors in Folder，不对整个Content做整理。打开新AM，点击Slot中的动画长条**Segment**，Details核对：

| Segment字段 | 值 |
| --- | --- |
| Animation Reference | AS_ET_Heal_01，或明确报告的最终重烘焙版本 |
| Start Time | 0 |
| End Time | 实际AS全长，预计约1.033333秒 |
| Play Rate | 1 |
| Loop Count | 1 |

Segment从Montage时间0开始，只保留一个动画段。Slot用已有**DefaultGroup.DefaultSlot**，不要新建Slot；一个Default Section、不连接回自己循环。Starting Position如只读不强行拖动；若误把长条移离0，Undo恢复。

创建入口与Slot / Section含义可参照[Epic UE5.5 Montage说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-montage-in-unreal-engine?application_version=5.5)，本工程沿用此前已完成的DefaultSlot方案，不修改ABP_Manny / ABP_Quinn。

### 6.2 Asset Details与预览

切换到Montage的**Asset Details**（不是刚才Segment的Details），清空搜索；找不到面板时Window菜单启用Asset Details。

| Montage字段 | 值 |
| --- | --- |
| Blend In → Blend Time | .10秒 |
| Blend Out → Blend Time | .10秒 |
| Enable Auto Blend Out | True |
| Rate Scale | 1 |
| Preview Mesh | SKM_Quinn_Simple |

关闭预览工具栏Loop，播放一轮，确认收拢 / 停留 / 收回顺序与AS一致、没有多余空白或重复循环。**独立动画编辑器停在最后一帧是正常的**，此时并没有正式玩家待机状态机接管；实机动作结束自动回待机 / 解锁由B2验证，不能仅凭预览是否停住认定失败。

### 6.3 15帧只预留，不创建通知

1. 在新AM时间轴查看**.500秒 / 15帧**，姿势应落在8–18停留段内。这个时点是后续正式治疗唯一生效点，不是Montage播放结束。
2. **本批Notifies保持空。**不用Add Notify → New Notify创建名字，不改SK_Mannequin Skeleton；不添加攻击窗口、Combo State、Montage Notify Window、蓝图Delay或Print String。
3. B2实现原生治疗Notify / GA后，再按明确步骤在这份AM上加一次生效通知。本批独立预览不回血、不减次数、没有Healing applied日志，均为正常。
4. 保存AM。旧三段轻攻击 / 敌人AM、Dodge / HitReact、正式GA和输入都保持原配置。

## 7. 分组检查与回到正式地图（约5–10分钟）

### 7.1 三资产各检查一次，约4–7分钟

| 准备 / 操作 | 通过条件 |
| --- | --- |
| LS关闭重开，依次看0 / 8 / 18 / 22 / 30 | 三FK开关False，七控制器五帧key / Linear；0=30、8=18；15位于稳定停留，左上臂0帧Z=25 |
| 关闭Sequencer后AS独立播放 | 骨架SK_Mannequin、约1.033秒 / 30fps；Quinn_Simple动作方向符合第1节；固定参照下root / 双脚不漂移，无异常穿身 / 扭转 |
| 新AM独立预览并看配置 | 唯一正确AS / DefaultSlot / Segment0开始 / Loop1 / Blend .10与.10 / AutoBlendOutTrue；Notifies空，不循环Section |
| 保存重开AM / AS | 真实最终名称与目录正确，动作仍为最新版本；没有覆盖旧资产 |

这批只新增孤立素材，**不运行C++ Build / Refresh-CLion、Healing.Resource / Combo / Dodge / Lifecycle旧自动化，也不Cook / 采Insights**。现有Validator面向正式配置Profile，新LS / AS / AM不会单独要求本项目Valid打印；不为获得该打印新增校验器或测试蓝图。B2改动作与输入时再触发相关专项。

### 7.2 正式地图短基线，约1–2分钟

1. 保存并关闭Sequencer，打开`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，Play一次。
2. 玩家生命 / 体力100。空地单点左键应仍是原轻攻击首段（不要求本批重测三段矩阵），2仍为正式方向闪避 / 一次25成本。
3. E一次提示 / 交互，P打开并用一种原支持方式继续，移动转向跳跃正常即可；不搭新治疗按键 / Controller图或临时播放AM。
4. Stop → Save All，保持正式地图打开。不要给GA / PracticeAction赋新治疗Montage，不改原资源3/3 / 4=Damage25 / 默认双100，slomo1 / Debug False保持。A已通过的治疗命令不必再消耗一轮来证明素材。

## 8. 完成反馈与问题材料

```text
M2-04B1 完成 / 卡在第__节第__步
三新资产实际名称与目录；旧资源未覆盖：
LS五帧0/8/18/22/30、三FK开关 / Linear；0=30、8=18、15在停留段：
AS Skeleton / 实际采样率 / 帧数或采样点数 / 秒数（找不到填未找到）：
Quinn_Simple独立播放、root / 双脚、收拢停留收回与首尾：
AM唯一AS / DefaultSlot / Loop1 / Blend .10与.10 / AutoBlendOut；Notifies空：
正式左键 / 2 / E / P与移动转向跳跃短基线：
已回正式地图；Source / 原配置不改、三资产Save All：
失败或占位动作最明显的问题（无填无）：
完成时分（如21:18）/ 实际手动用时 / 烘焙或加载等待：
```

表中数字必须填**实际看到的值**，不要把“完成”代替Skeleton / 秒数等信息；当前帧数与采样点命名可能不同，说明界面字段名即可。不需要手摘整段日志，给大致时分 / 路径，我自行核对。

姿势问题给“当前帧 + 控制器完整名 + 实际L/R/S”，截图包含人偶全身、Anim Details和时间指针；跳变 / 穿身给5–10秒预览视频。找不到烘焙 / Segment / Asset Details时截当前选中对象与菜单，不猜同名字段。导出 / 保存错误用文字贴第一条Error与前后几行，不先删资源 / 清缓存。

收到反馈后我先核对文件 / 保存日志 / 参数证据并生成B1验收报告；**无待补项则同一轮直接生成M2-04B2：正式治疗能力、一次Notify提交与Q输入指南**。有问题只留具体待补项并给简化修复，不机械重做旧矩阵。

## 9. 本批 Commit / Push（固定放在最后）

**当前指南准备阶段：**Stage本指南、[数值JSON](Data/M2_04B1_HealPoseValues.json)、治疗路线 / Progress / Backlog / M2_Roadmap / Architecture / AcceptanceWorkflow_Options / VersionControl / Introduction的交接与新“验收通过后直接下一章”约定。A主体已提交685a0f3；[A报告](Reports/M2_04A_Verification.md) / 来源三个身体说明和收尾未提交的可合并。此时工程没有新B1资产或Source可提交。

**B1实际完成并验收后：**在CLion刷新Changes，Stage以下三份新LFS资产的最终真实路径（_v2如采用需明确，并以AM实际引用为准）：

```text
EmberTrial/Content/EmberTrial/Animations/Healing/Sequence/LS_ET_Heal_01.uasset
EmberTrial/Content/EmberTrial/Animations/Healing/Clips/AS_ET_Heal_01.uasset
EmberTrial/Content/EmberTrial/Animations/Healing/Montages/AM_ET_Heal_01.uasset
```

制作地图`EmberTrial/Content/EmberTrial/Maps/L_M0_AnimationLab.umap`只在合法人偶配置确有保存差异时一并提交；正式地图、原Skeleton / Rig / AnimBP / GA / 输入与旧动画无计划改动，不为提交人为重存。若出现这些额外变化，先告诉我最后操作，我核对实际范围；不要全部勾选掩盖误操作。

届时同时Stage B1报告、数值JSON / 本指南 / 路线 / 进度、核实后AssetManifest新增三行真实SHA（预计58→61）。无实物前不填来源行或写已通过；后续自动进入B2只生成指南，不代写Source或自动上传。已Stage后又修改的文档 / 资产需重新Stage最新差异，检查Staged Diff / LFS指针后Commit / Push。主体可用`feat: add healing animation placeholder assets`，仅指南可用`docs: prepare healing animation workflow`。

Raw证据 / 本地生成脚本、日志 / 视频、Binaries / Intermediate / Saved、生成CMake / IDE缓存和历史包留本地，不Force Add。C++ / Build / 资产录入仍由用户执行，Stage / Commit / Push仍由用户完成。
