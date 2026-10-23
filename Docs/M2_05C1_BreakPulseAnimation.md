# M2-05C1：破势施放动画素材与Montage

2026-10-23收尾，固定UE5.5.4。**[本章已验收](Reports/M2_05C1_Verification.md)**：用户13:49确认完成，手动45 / 等待5分钟；三素材保存与哈希已核对。以下保留完整制作步骤供学习 / 复现。实际Montage目录为单数，已同步，不需要搬资源。**只做三份独立动画资产，不改C++或输入，不运行Build / 自动测试。**范围破势、F键与冷却在后续C2接入，当前不会因为新建Montage就能施放技能。

预计手动30–60分钟，含录入 / 预览20–40、烘焙 / Montage配置5–10、分组检查与记录5–10；加载 / 烘焙等待另记，不要求做满预算。收尾当前131 Source / 70cpp、Core50 / UI17 / Editor3、16项自动化、来源68；三素材已存在，Source保持。

## 1. 施放动作、视觉标准与当前边界

玩家先把右掌收到右腰肋外侧，随后向前推出，短暂停留，再回准备姿势；左臂保持护身。动画本身没有震波粒子、范围伤害、位移或武器接触。本批是原地上肢占位，后续Blender / 最终美术可以替换素材，动作提交时点和GAS事务独立维护。

| 帧 / 秒 | 意义 | 应观察的结果 |
| --- | --- | --- |
| 0 / 0 | 准备 | 继承既有占位准备，两肘弯曲，左上臂Z=25 |
| 8 / .266667 | 蓄势 | 右肘后拉，右掌回到右腰肋外侧，左肘略收护身 |
| **12 / .400000** | **前推完成；预留唯一生效点** | 右掌明显前伸，肘部不反折；此时未来C2才进行一次范围结算 |
| 12–20 / .400–.667 | 稳定停留 | 七控制器两帧完全一致，前推姿势保持，不多次挥击 |
| 30 / 1.000000 | 收回准备 | 七控制器与0完全相同，连续衔接回待机 |
| End31 / 约1.033333 | 排他结束 | 最后姿势key仍在30，不能把End31当秒数 |

这组旋转向量复用M0挥击和治疗中已用的数值，但**制作前这组新组合 / 时序尚未在Rig求值、烘焙或检查蒙皮；现已按用户预览反馈验收**。先做0与12帧，看正面 / 侧面后再补其余三帧；方向明显不符时按第8节反馈，不通过修改Skeleton、Rig Offset或负Scale掩盖问题。

| 资源 / 状态 | 本批处理 |
| --- | --- |
| 新AM_ET_BreakPulse_01 | 施放者的前推动作，只独立预览，暂不赋给正式GA |
| 既有GA_ET_Stagger与AM_M0_HitResult_01_Montage | 受害者归零后的被动反应，保持B2配置 |
| 左键三段、2闪避、Q治疗、E、P与AI | 保持原绑定和资源；不添加临时播放按键 |
| 新动画的Notifies | 保持空；C2提供原生提交Notify后才加一次 |

## 2. 新建空白Sequence与独立目录

### 2.1 回到动画试场

1. Stop PIE → Save All；保持UE打开即可。保存并关闭其他Sequencer标签，停止它们的播放。
2. 打开已有`/Game/EmberTrial/Maps/L_M0_AnimationLab`，沿用原**M0_AnimationDummy（SKM_Manny）**。不把正式玩家 / 敌人拖来制作，不改默认地图或GameMode。
3. 在内容浏览器建立三个新目录，`Sequence`用单数：

```text
/Game/EmberTrial/Animations/BreakPulse/Sequence
/Game/EmberTrial/Animations/BreakPulse/Clips
/Game/EmberTrial/Animations/BreakPulse/Montage
```

最终只新增下表三份资产，不移动 / 覆盖Practice、Combo或Healing：

| 类型 | 完整UE资源路径 |
| --- | --- |
| 制作源 | `/Game/EmberTrial/Animations/BreakPulse/Sequence/LS_ET_BreakPulse_01` |
| 烘焙动画 | `/Game/EmberTrial/Animations/BreakPulse/Clips/AS_ET_BreakPulse_01` |
| Montage | `/Game/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01` |

### 2.2 新Sequence与Control Rig

1. 在新Sequence目录空白处右键 → **Cinematics / 过场动画 → Level Sequence**，命名`LS_ET_BreakPulse_01`，双击打开。创建空白资产，不Duplicate旧LS；不在关卡放自动播放LevelSequence Actor。
2. 在关卡Outliner选中M0_AnimationDummy。回Sequencer **+ Track → Actor to Sequencer → 当前选中的人偶**；也可把Outliner同一人偶拖到Sequencer左侧轨道列表。
3. 在人偶绑定行 **+ Track → Control Rig → Control Rig Classes → CR_Mannequin_Body**。若已经自动出现该Rig，直接复用；同一人偶只有一条Rig驱动，不叠加旧AS / 第二条Rig，不切Layered。
4. Sequencer工具栏Display Rate设**30fps**，时间显示设**Frames**。Playback Options如锁定Playback Range先解锁，填**Start=0 / End=31**，Enter提交，再打开核对。
5. Control Rig section覆盖全播放范围；时间轴底部可视范围的缩放不等于Playback Range。人偶Actor保持原静态位置、Rotation0 / Scale1，不给Actor Transform录位移。
6. 关闭Auto Key，保存新LS，确认顶部标题是LS_ET_BreakPulse_01，再开始填值。

界面路径沿用本工程已完成的[M2-04B1素材步骤](M2_04B1_HealingAnimation.md)。菜单不同就截当前选中对象和菜单，不修改Rig Graph尝试补菜单。

## 3. 数值面板、开关与一帧的操作

### 3.1 固定条件

| 项目 | 本批全段要求 |
| --- | --- |
| arm_r_fk_ik_switch / arm_l_fk_ik_switch / spine_fk_ik_switch | 三个布尔值均False，0帧记录相应开关key，全段不得出现True |
| 下表七控制器Location | 每帧(0,0,0)，是相对控制器Offset的局部值 |
| 下表七控制器Scale | 每帧(1,1,1)，不填负数镜像 |
| 控制器Space / 父级 | 模板默认，不切空间 |
| global / root / body、锁骨 / 头 / 腿 / 双脚 / 手指 | 保持默认，不给它们新增姿势或位移key |
| 插值 | 七控制器本次姿势key全部Linear；布尔开关不按Transform曲线处理 |

三个FK/IK开关没有Location / Rotation / Scale。控制器Location0**只限下表七项**，不代表把所有骨骼位置清零；骨长与父子偏移保持模板值。

### 3.2 每帧逐行填法

1. 停止播放，把时间指针准确放到目标帧，例如0。
2. 在右侧**Anim Outliner**搜索完整控制器名并选择，例如`upperarm_r_fk_ctrl`；确认**Anim Details**标题也是它。
3. 填该控制器当前Location / Rotation / Scale。不是Actor Details、Skeleton Tree，也不是Rig的Initial / Offset Transform。
4. 括号按**X / Y / Z**，Rotation单位度，X=Roll / Y=Pitch / Z=Yaw。若界面按Pitch / Yaw / Roll排列，按字段名对应；每项是最终值，不在旧数值上累加。
5. 各组Enter提交，保持控制器选中，鼠标焦点放动画视口，按**S**打本帧Transform key；也可用Transform通道Add Key菱形。确认Location / Rotation / Scale均有记录。
6. 填同帧其他六项，即使值不变也记录。切到其他帧再回来，确认数值保留，才算完成该帧。
7. **先完成0与12，再完成8 / 20 / 30。**全部完成后框选七控制器本次姿势key点，右键Linear。保存、关闭重开LS一次核对。

`-0`和约1e-6可按浮点显示误差处理。不要在全身骨骼上批量清零，不改骨架缩放或Root Lock来修正手臂方向。

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

### 4.2 第8帧：右掌后收蓄势

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 10, -55) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 80) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 30) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 85) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.3 第12帧：前推完成，预留生效点

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 75) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, -20) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 30) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 85) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.4 第20帧：保持前推，完整重复12帧

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 75) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, -20) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 30) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 85) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

### 4.5 第30帧：回到准备，完整重复0帧

| 控制器 | Location (X,Y,Z)，cm | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
| --- | --- | --- | --- |
| `spine_01_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_r_fk_ctrl` | (0, 0, 0) | (0, -20, 15) | (1, 1, 1) |
| `lowerarm_r_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_r_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |
| `upperarm_l_fk_ctrl` | (0, 0, 0) | (0, -20, 25) | (1, 1, 1) |
| `lowerarm_l_fk_ctrl` | (0, 0, 0) | (0, 0, 70) | (1, 1, 1) |
| `hand_l_fk_ctrl` | (0, 0, 0) | (0, 0, 0) | (1, 1, 1) |

完成后检查：**0=30、12=20**；指针放16帧应与12 / 20保持同姿势。连续播放应有“后收 → 较快前推 → 稳定保持 → 收回”，无瞬间翻转。用固定镜头 / 地面参照检查root和双脚，不能只靠跟随镜头判断。

参数另存[本批数值JSON](Data/M2_05C1_BreakPulsePoseValues.json)，与本表来自同一数据源。JSON用于查阅 / 核对，不需要导入UE或运行脚本，也不能代替实际预览。

## 5. 烘焙AS并用Quinn_Simple独立预览

1. 停止播放，保存LS；再次确认**Start0 / End31 / 30fps**及Control Rig section范围。
2. 清空控制器搜索，在Sequencer右键**最上层M0_AnimationDummy的Actor绑定行 → Bake Animation Sequence**。不是右键手臂控制器、Rig section，也不是Bake To Control Rig。
3. 输出到`/Game/EmberTrial/Animations/BreakPulse/Clips`，名称`AS_ET_BreakPulse_01`。沿用本工程的骨骼变换导出 / 当前播放范围 / 30fps；如有Export in World Space设False。不覆盖旧AS。
4. 完成后关闭Sequencer，从内容浏览器**单独打开AS_ET_BreakPulse_01**，避免制作LS仍在驱动人偶造成误判。
5. Asset Details确认Skeleton=**SK_Mannequin**、Rate Scale=1、Enable Root Motion=False。不要为遮漂移改Force Root Lock。
6. Preview Mesh选**SKM_Quinn_Simple**，单独播放一遍；正面 / 侧面观察前推、停留和首尾，固定参照下root / 双脚不漂移。
7. 记录AS实际显示的Skeleton、采样率、帧数 / 采样点数和秒数。预期约**31帧间隔 / 32采样点 / 1.033333秒，30fps**，以实际信息为准；看不到字段写“未找到”，不要用“完成”代替具体数字。

若只改LS并Save，已烘焙AS不会自动更新；必须重新Bake对应新AS。同名覆盖界面不支持时可暂用`_v2`并报告真实路径，不先删除旧资产。AS时长明显不符时先查Range与导出采样率，不用Rate Scale改速去迁就预期秒数。

## 6. 从新AS创建独立Montage

### 6.1 创建与配置

1. 关闭AS，在内容浏览器右键`AS_ET_BreakPulse_01` → **Create → Create AnimMontage**。
2. 将新AM命名为`AM_ET_BreakPulse_01`并移到`BreakPulse/Montages`。若移动产生Redirector，只对本次BreakPulse目录Fix Up Redirectors，不全工程整理 / 重存。
3. 双击新AM。动画轨道应只有一个Segment，引用**AS_ET_BreakPulse_01**，从0秒开始；单击该动画块后在Details确认Play Rate=1、Loop Count=1，使用完整AS范围。
4. Slot使用已存在的**DefaultGroup.DefaultSlot**；不要新增Slot或修改Skeleton / ABP。
5. 点击**Asset Details**确认Blend In Time=.10、Blend Out Time=.10、Enable Auto Blend Out=True。Asset Details中的Montage资产设置与“选中Segment后Details”是不同对象，不在Segment里找Blend。
6. Section使用一个Default即可，Next Section为None，不形成回指自身的循环。独立播放应自动退出，不在尾部重复推掌。

### 6.2 预留12帧提交，不添加Notify

1. 时间轴看**.400000秒 / 第12帧**，应正好到前推完成；12–20帧保持该姿势。这个时点供后续C2原生Notify使用，不是本批已接入的生效事件。
2. **AS和AM的Notifies均保持空。**不添加Skeleton New Notify、Attack Window、Combo State或Heal Commit；不新建蓝图事件去减Poise。
3. 保存并关闭重开AM，确认唯一AS、Slot、Blend、Loop1、AutoBlendOut与空Notifies仍正确。
4. GA_ET_Stagger继续引用原AM_M0_HitResult_01_Montage；本批新AM只属于将来的施放者。不要赋给Attack / Dodge / Heal / PracticeAction。

## 7. 分组检查与回到正式地图：约5–10分钟

### 7.1 新素材检查一轮

| 准备 / 操作 | 通过条件 |
| --- | --- |
| LS保存后关闭重开，看0 / 8 / 12 / 20 / 30 | 三FK开关False，七控制器五帧key / Linear；0=30、12=20，16帧保持前推 |
| 关闭Sequencer，AS独立播放 | SK_Mannequin，约1.033秒 / 30fps；Quinn_Simple方向正确，无明显穿身 / 扭转，root / 双脚固定 |
| AM独立播放与查看设置 | 唯一正确AS / DefaultSlot / Loop1 / Rate1 / Blend .10与.10 / AutoBlendOut；12帧前推、Notifies空，自动退出 |
| 三资源保存 / 重开 | 真实名称与BreakPulse目录正确，旧资源未覆盖，没有多余临时版本被误当最终资源 |

### 7.2 正式地图短基线，约1–2分钟

回`/Game/EmberTrial/Maps/L_ET_VerticalSlice`新Play，左键、2、4后Q、E / P一种继续方式、移动转向跳跃短查，原生命 / 体力 / 次数HUD正常即可。**本批F没有新行为是预期。**Stop，保持正式地图为当前编辑地图，Save All。

本批只是三个孤立素材，不重跑B2自动化、死亡 / 多Pawn / 重建矩阵；没有源码变更不Build。若旧资源或源文件意外出现差异，先核对原因，不把所有变化一并Commit。

## 8. 完成反馈与报错方式

```text
M2-05C1 完成
三个新资产真实名称 / 目录，旧资源未覆盖：
LS五帧0/8/12/20/30、三FK False / Linear；0=30、12=20、16在停留段：
AS Skeleton / 实际采样率 / 帧数或采样点数 / 秒数（找不到填未找到）：
Quinn_Simple独立播放、前推停留收回 / root与双脚 / 首尾：
AM唯一AS / DefaultSlot / Loop1 / Rate1 / Blend .10-.10 / 自动退出；AS与AM Notifies空：
GA_Stagger保持旧HitResult引用；正式左键-2-Q-E-P与基础输入短基线：
已回正式地图，三资产Save All；Source / 原配置保持：
失败或最明显占位问题（无填无）：
完成时分 / 手动制作与配置 / 检查 / 烘焙或加载等待：
```

姿势不符时给**帧号、控制器完整名、实际L/R/S**，截图同时含全身、Anim Details与时间指针，最好补侧面；时序用5–10秒预览视频。菜单找不到截当前对象和菜单；保存 / 导出报错用文字贴首条Error和上下文。不要求逐条手抄日志，给完成时分与实际路径，我读取保存记录核对。

C1反馈及保存核对已完成，无待补项；当前进入[C2-01施放 / 冷却](M2_05C2_01_BreakPulseAbility.md)。素材已完成，正式F仍待手动接入；C2-02再做通知 / 范围结算。

## 9. 本批 Commit / Push（固定放在最后）

三份动画已纳入本地`b3cbc24`，完整B2也已补交；先前空文件问题关闭。当前没有源码 / 资产待补交，不重复Stage没有差异的资源。

本次验收收尾包含本指南、`Docs/Data/M2_05C1_BreakPulsePoseValues.json`、`Docs/Reports/M2_05C1_Verification.md`、`Docs/AssetManifest.csv`、Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / 两份M2路线 / README / Introduction，及`Docs/M2_05C2_01_BreakPulseAbility.md`；B2指南仅同步收尾状态。

```text
docs: verify break pulse animation and prepare casting ability
```

CLion刷新Changes → Stage最新文档 → 检查Diff → Commit → Push。C2-01 Source / GA / IA没有实际生成，实施后按其末尾清单另交。Raw / 日志 / 缓存 / 生成文件 / 包体留本地；助手未改索引或核验远端。
