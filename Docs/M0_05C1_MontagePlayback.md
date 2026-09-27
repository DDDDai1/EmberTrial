# M0-05C1：将三段动画接到玩家角色

日期：2026-09-24。引擎：UE 5.5.4。预计手动 **45–70 分钟**，含开头最后一处 B 补修；无需 C++ Build。

**2026-09-27 状态更新：**用户已完成播放逻辑并修复速度 Branch 的 True / False 接反问题，确认播放恢复；只读复核保存连线正确，B 最后一个关键帧也已通过。以下 §0 是历史步骤，无需重做。实际 Montage 名称是 `AM_M0_Attack_01_Montage` / `AM_M0_Dodge_01_Montage` / `AM_M0_HitResult_01_Montage`，引用有效，不要求去掉后缀。下一步见 [C2-01 组件准备](M0_05C2_01_ActionComponent.md)，完整边界验收仍以 §6 为准。

本批由你创建 Montage / Input Action、连接基础蓝图，我负责审查与复杂排障。按顺序做，每个小节都有可以看到的结果；卡住约 15–20 分钟就反馈。

## 0. 先收尾闪避的一个关键帧

你报告补修 20 分钟，我已复核：两段播放范围、时长和受击三个改值均正确。**保存后的 LS_M0_Dodge_01 第 0 帧 `upperarm_l_fk_ctrl.Rotation.Z` 仍为 15，末帧为 25**；烘焙动画的左手起止位置仍差约 5.27 cm。因此这最后一项尚未验收通过。

本次先完成以下操作，然后可以直接做 §1，不必单独等待一轮回复：

1. 停止 PIE，打开 `L_M0_AnimationLab` 和 **LS_M0_Dodge_01**，确认 Sequencer 顶部名称。
2. 停在 **第 0 帧**，选中左上臂 **upperarm_l_fk_ctrl**。
3. 在 Anim Details 填最终值：**Location=(0,0,0)，Rotation=(0,-20,25)，Scale=(1,1,1)**。Enter 后保持该控制器选中，把鼠标焦点放回动画视口，按 **S** 更新原有第 0 帧 Transform key（也可用该通道 Add Key 按钮），保持 Linear。只改 Details 数字而没有更新 key，切换时间后会恢复旧值。
4. 切到第 4 帧，再切回 0。确认 Z 仍为 **25**，再保存 Sequence。
5. **关闭这个 Sequencer 标签，再重新打开同一个 LS_M0_Dodge_01**，回到第 0 帧核对 Z=25。若变回 15，说明改值没有进入正确关键帧：此时反馈截图，我接手定位，先不要用它新建 Montage。
6. End 继续保持 **21**；重新 Bake Animation Sequence，更新 **AS_M0_Dodge_01**。不接受同名时可生成 AS_M0_Dodge_02 并反馈，后文闪避 Montage 改用你刚导出的版本。
7. 单独打开新烘焙动画确认首尾左臂一致、时长约 **0.700 秒**，保存。原有受击和攻击不用再次烘焙。

最新路径已经变化：**受击真正制作源现在是 LS_M0_HitResult_01，旧 _02 路径加载后指向 _01**。它本轮已正确，不再按旧补修指南打开 _02 进行改值。AS 仍为 **AS_M0_HitResult_01**。

只读核实的元数据如下。你填的“21 / 19”对应帧间隔数量，换算后的秒数已补齐，无需再为单位单独补交一次反馈。

| 动作 | AS 资产 | 采样率 | 帧间隔 / 采样点 | 时长 |
|---|---|---|---|---|
| 攻击 | AS_M0_Attack_01 | 30 fps | 31 / 32 | 1.033 秒 |
| 闪避 | AS_M0_Dodge_01 | 30 fps | 21 / 22 | 0.700 秒 |
| 受击 | AS_M0_HitResult_01 | 30 fps | 19 / 20 | 0.633 秒 |

## 1. 本批要做出的效果

回到 ThirdPersonMap，角色落地站定后：键盘上排 **1** 播放攻击、**2** 播放闪避姿势、**3** 播放受击。各播放一次，自动回到模板待机；长按不循环，动作期间新请求被忽略，暂停恢复后可以继续操作。

本批规定只能在**地面基本静止**时发起这三个调试动作。这三键暂时用于试验；“3”是手动预览受击，并未接入伤害系统。闪避片段为原地姿势，**按 2 后没有实际平移是本批预期**。

C 的安排：

| 批次 | 目标 |
|---|---|
| **C1，本次** | Montage、已有 Slot、输入、统一播放入口、播放结束与暂停基础回归 |
| C2，后续 | 动作状态与优先级、受击打断、CharacterMovement 驱动实际闪避位移、生命周期清理 |
| C3，后续 | 四动作组合回归与约 30 秒实机记录，汇总 M0-05 结论 |

C1 不增加移动 / 跳跃锁，也不添加用固定 Delay 解锁的状态。如果播放中开始移动或跳跃，模板输入仍响应，原地全身占位动画可能暂时显得滑步；本批站定测试，移动限制和打断在 C2 一起处理。这样不会提前制造需要在多条退出路径恢复的锁定状态。

## 2. 创建三个 Montage

### 2.1 创建位置和来源

关闭 Sequencer，回到 ThirdPersonMap。Content Browser 中创建文件夹：

```text
/Game/EmberTrial/Animations/Practice/Montages
```

在已有 `Practice/Clips` 文件夹里，对下表每个 Animation Sequence 分别右键，选择 **Create → Create AnimMontage**（中文界面可能显示“创建 → 动画蒙太奇”）。UE 通常在原目录生成新资产；命名后，通过 Content Browser 的 **Move Here** 移到 Montages，使用编辑器移动，不在资源管理器拖 `.uasset`。

| 原始 Animation Sequence | 新 Montage 名称 |
|---|---|
| AS_M0_Attack_01 | **AM_M0_Attack_01** |
| AS_M0_Dodge_01（或 §0 刚烘焙的 _02） | **AM_M0_Dodge_01** |
| AS_M0_HitResult_01 | **AM_M0_HitResult_01** |

创建后双击检查资产类型为 **Anim Montage**，Skeleton 为 **SK_Mannequin**。源 AS 继续保留，Montage 引用它。

### 2.2 每个 Montage 的设置

1. 时间轴中应只有本动作的一段 Animation Segment。
2. Slot 轨道使用 **DefaultGroup.DefaultSlot**。选择既有 Slot，不新建同名组或插槽。
3. 点击动画片段，在该片段 Details 核对：Animation Reference 是正确 AS，Start Position=0，Anim Start Time=0，Anim End Time 为该 AS 全长，Anim Play Rate=1，**Looping Count=1**。
4. 保留默认的单个 **Default** Section；不把该 Section 的下一段连接回自己。如果 Montage Sections 面板里已经存在自循环，清除循环关联。
5. 打开 **Asset Details**（找不到可在 Window 菜单开启），设置下表。搜索属性名可以缩小列表；`Blend In` / `Blend Out` 需展开找到 **Blend Time**。

| 资产级属性 | 值 |
|---|---|
| Blend In → Blend Time | **0.05 秒** |
| Blend Out → Blend Time | **0.10 秒** |
| Blend Mode In / Out | Standard（若显示） |
| Blend Out Trigger Time | **-1** |
| Enable Auto Blend Out | **True** |
| Rate Scale | **1.0** |

三个 Montage 使用相同参数，便于这一轮比较。AS 的 Enable Root Motion 保持 False；本批没有 Root Motion 位移。

6. Preview Mesh 选 **SKM_Quinn_Simple**，点预览播放。动画应完整，时长约为上表对应值。预览工具栏自己的“循环预览”按钮可以关闭；它和 Segment 的 Looping Count 是两个设置。
7. 保存三个 Montage。

**本节结果：**三个 AM 可以独立预览，各自引用正确的 AS，没有把闪避或受击错误引用为攻击。角色内能否播放，要继续完成后面的 Slot 与输入。

## 3. 复用实际生效的 DefaultSlot

当前玩家 Mesh 是 **SKM_Quinn_Simple**，Anim Class 是 **ABP_Quinn_C**，其父类为 **ABP_Manny_C**。只读检查确认 **ABP_Manny 的 AnimGraph 已有一个 Slot，名称是 DefaultSlot，Always Update Source Pose=False**。ABP_Quinn 没有自己的 AnimGraph。

所以本批编辑的是父类的现有节点：

1. Content Browser 打开 `/Game/Characters/Mannequins/Animations/ABP_Manny`。
2. 在左侧 **My Blueprint → Animation Graphs → AnimGraph** 双击进入图表；不是 EventGraph，也不是 Control Rig 资产。
3. 找到已有 **Slot 'DefaultSlot'**。可以在该蓝图内 `Ctrl+F` 搜索 `DefaultSlot`，双击结果定位。
4. 选中这个节点，在 Details 中确认 **Slot Name=DefaultSlot**。
5. 勾选 **Always Update Source Pose=True**。
6. 保留它已有的姿态连线、状态机和 Control Rig。**不要再添加第二个 DefaultSlot**。若你实际看到的图没有该节点或它未连接，请给整条 AnimGraph 截图，我来核对。
7. Compile、Save ABP_Manny；再打开 **ABP_Quinn**，Compile、Save。
8. BP_ThirdPersonCharacter 的 Mesh 继续使用 **Use Animation Blueprint + ABP_Quinn**，不改成 Use Animation Asset。

这个设置让 Slot 播放占位动作时，上游移动状态仍保持更新，方便结束后回到正常姿态。Montage 轨道与 AnimGraph 的 Slot 名称需要一致；Slot 平时传递上游姿态，收到对应 Montage 后参与混合。[Epic UE 5.5 Slot 说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-slots-in-unreal-engine?application_version=5.5)

**本节结果：**此时尚未连接新输入，PIE 中原有移动 / 转向 / 跳跃应仍正常。先快速检查一次再继续。

## 4. 添加三个试验输入

在 `/Game/EmberTrial/Input/Actions` 创建三个 **Input Action**：

| Input Action | Value Type | Triggers / Modifiers | Trigger When Paused |
|---|---|---|---|
| IA_PracticeAttack | Digital (bool) | 空 | False |
| IA_PracticeDodge | Digital (bool) | 空 | False |
| IA_PracticeHit | Digital (bool) | 空 | False |

Consume Input 保持默认；本批没有同键竞争。保存。

打开当前角色实际使用的 **`/Game/ThirdPerson/Input/IMC_Default`**，增加三条映射：

| Action | Key |
|---|---|
| IA_PracticeAttack | 键盘上排 **1** |
| IA_PracticeDodge | 键盘上排 **2** |
| IA_PracticeHit | 键盘上排 **3** |

映射层的 Triggers / Modifiers 同样为空。保留 Jump / Move / Look / Interact / Pause。该 IMC 已由模板 C++ 添加到玩家，无需再写一个 Add Mapping Context 或第二套 BeginPlay 注册。

保存 IMC。这里完成后，按键尚未调用任何动画，继续 §5。

## 5. 在角色中建立统一播放入口

打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`。本批的输入与函数都放在这个角色蓝图，三个动作共用一个函数，避免复制三套检查逻辑。AnimBP 只负责姿态输出，不负责接收键盘输入。

### 5.1 三个 Montage 引用变量

添加以下变量，类型均为 **Anim Montage → Object Reference**，不是 Anim Sequence，也不是 Class Reference。Category 可以统一填 `M0|PracticeAnimation`。不勾选 Instance Editable / Expose on Spawn。

| 变量 | 默认值 |
|---|---|
| PracticeAttackMontage | AM_M0_Attack_01 |
| PracticeDodgeMontage | AM_M0_Dodge_01 |
| PracticeHitMontage | AM_M0_HitResult_01 |

先 Compile，再在各变量的 Default Value 选择资产，保存。

### 5.2 创建函数及检查链

在 My Blueprint 的 Functions 旁点 `+`，命名 **TryPlayPracticeMontage**。

- 添加一个输入 **InMontage**，类型 **Anim Montage → Object Reference**。
- 不添加输出，不勾选 Pure。
- 普通函数可以通过 **Return Node** 提前结束。下列“Return”都表示本次请求到此结束，未通过检查就不往下播放。

按照下列执行顺序连接白色线。先完成一条检查链，后面三个输入直接复用它。

| 顺序 | 节点和数据来源 | 执行分支 |
|---:|---|---|
| 1 | Branch，Condition=`bPauseMenuOpen OR Is Game Paused`，沿用已有变量 | True → Return；False → 2 |
| 2 | **Is Valid**，Input Object=函数输入 InMontage；选择有白色执行引脚的版本 | Is Not Valid → Print String `[M0-05C1] Missing Montage` → Return；Is Valid → 3 |
| 3 | 从角色的 **Mesh** 组件拖出 **Get Anim Instance**；结果接另一个有执行引脚的 Is Valid | Is Not Valid → 打印 `[M0-05C1] Missing AnimInstance` → Return；Is Valid → 4 |
| 4 | 从 **Character Movement** 组件拖出 **Is Moving on Ground**，结果接 Branch | False → Return；True → 5 |
| 5 | **Get Velocity(Target=Self) → Vector Length XY → <= 3.0**，接 Branch | False → Return；True → 6 |
| 6 | 从同一个 Get Anim Instance 的结果拖出 **Is Any Montage Playing**，结果接 Branch | True → Return；False → §5.3 |

`3.0` 是水平速度阈值，单位 cm/s，允许落地站定后的极小残余速度。`Vector Length XY` 有时显示为 `VSize XY`，只取水平速度，不直接把 Velocity 向量接进 Boolean。

**本工程曾出现的易错点：**“速度 <= 3”的 Branch 是 **True → 下一步忙碌检查，False → Return**。站定时 `0 <= 3` 为 True，必须允许继续；如果接反，三个输入都会安静地提前返回，编译不会报错。它与后面的“Is Any Montage Playing=True 就返回”方向不同，不能把两个 Branch 按相同方向接线。

第 6 步使用 **Is Any Montage Playing**，不要替换成 Montage Is Active。本机 5.5.4 实现中，前者会在 Montage 实例仍留存时返回 True，包含尚未完成清理的淡出阶段，适合本批避免连续重启；这里不额外维护 `bIsAttacking` 或定时器。

Is Valid 的纯布尔版本也能配 Branch，但本指南选用自带 Is Valid / Is Not Valid 执行输出的版本，便于对照连线。

### 5.3 播放和成功日志

从 **Get Anim Instance** 蓝色输出拖线，搜索 **Montage Play**，选择 **Target is Anim Instance**、有 Float Return Value 的普通函数节点。

不要选 `Play Animation`；也不要在这个普通函数中放有 On Completed 等回调引脚的异步 `Play Montage` 节点。它们的使用方式不同。本批只用下表对应的 **Montage Play**。

| Montage Play 引脚 | 连接 / 设置 |
|---|---|
| 白色执行输入 | 上一步“Is Any Montage Playing=False”分支 |
| Target | **Mesh → Get Anim Instance** 的返回值，不是 Self |
| Montage to Play | 函数输入 **InMontage** |
| In Play Rate | **1.0** |
| Return Value Type | **Montage Length** |
| In Time to Start Montage At | **0.0** |
| Stop All Montages | **True**；如折叠，展开高级引脚查看 |

从 Montage Play 白色输出接 Branch，Condition 使用 **Return Value > 0.0**：

- True：Print String，内容用 **Append** 将 `[M0-05C1] Play: ` 与 **Get Display Name(InMontage)** 拼起来，Print to Log=True，Duration=1.5；然后 Return。
- False：Print String `[M0-05C1] Montage Play failed`，Print to Log=True；然后 Return。

缺少引用和播放失败的打印也勾选 Print to Log，便于我查看日志。被暂停 / 空中 / 移动 / 正在播放检查挡住时，直接返回即可，不逐次刷日志。

完整白线顺序：

```text
TryPlayPracticeMontage(InMontage)
  → 未暂停
  → InMontage 有效
  → Mesh 的 AnimInstance 有效
  → 在地面
  → 水平速度 <= 3 cm/s
  → Is Any Montage Playing = False
  → Montage Play
  → Return Value > 0 才打印 Play 成功
```

返回值大于 0 表示播放请求已建立，**不等于画面一定显示正确**；Slot 名称 / 路径还需要实机验证。

Compile、Save。若找不到节点，先确认是从 **Get Anim Instance 返回值**拖线搜索，并且 Context Sensitive 勾选。

### 5.4 三个输入调用同一个函数

回到角色 **Event Graph** 空白位置，右键搜索并添加对应的 **Enhanced Input Action** 事件，按下表连接：

| 事件执行输出 | 调用函数 | InMontage |
|---|---|---|
| IA_PracticeAttack → **Started** | TryPlayPracticeMontage，Target=Self | Get PracticeAttackMontage |
| IA_PracticeDodge → **Started** | TryPlayPracticeMontage，Target=Self | Get PracticeDodgeMontage |
| IA_PracticeHit → **Started** | TryPlayPracticeMontage，Target=Self | Get PracticeHitMontage |

只连接 **Started**，其余 Triggered / Completed / Canceled 留空。Triggered 在持续按住时可能逐帧到达，会造成动作结束后又立刻重播；Started 适合本批一次按下只请求一次的约定。

用 Comment 框圈住三条调用，标题可填 `M0-05C1 Practice Montage Input`。保留已有 E / P、BeginPlay 和 EndPlay。函数不创建 Widget、绑定委托或持有异步计时，因此本批不用新增 EndPlay 清理分支。

Compile、Save。

## 6. 分组验收

### 6.1 单次播放

ThirdPersonMap → Play，点击游戏视口取得焦点，等角色落地站定；开始测试时松开 WASD 和空格。

1. 按 1，看到攻击，并产生一条 `[M0-05C1] Play: AM_M0_Attack_01`。结束后回到正常待机。
2. 等动作完全结束，按 2，看到原地闪避姿势，约 0.7 秒后结束。
3. 再按 3，看到受击的后仰、回弹和恢复。
4. 三段结束后分别确认可以移动、转向、跳跃；没有卡在最后一帧或 T Pose。

### 6.2 重入与条件

| 操作 | 本批预期 |
|---|---|
| 持续按住 1 约 3 秒 | 只播放一次，只产生一条 Play 成功日志；松开再按才能再次请求 |
| 攻击刚开始，快速按 1/2/3 | 当前动作继续；忙碌期间的请求忽略，不排队、不切换 |
| 等完全回到待机，再按 2 | 新闪避正常播放 |
| 持续移动时按 1/2/3 | 不播放；站定后重新按键才播放 |
| 跳起后按 1/2/3 | 空中不播放，也不在落地时补执行；落地后重新按键可播放 |

要观察“动作期间不重启”，重点在动作前半段测试。动作已经结束后发生的新按下可以正常播放，不算重复触发错误。

### 6.3 暂停与已有功能

1. 按 1 开始攻击，马上按下并松开 P。菜单出现，世界和当前动作停止推进；继续后动作从暂停位置继续并正常结束。
2. 菜单里按下并松开 1/2/3，再关闭菜单。恢复后不会补播菜单期间的请求。
3. 按住 1 开始动作，打开暂停菜单；在菜单里松开 1，再继续。原动作可以继续，但不会自动重播第二次。随后重新按 1 应正常。
4. 快速复验 E：范围外不交互、A/B 提示与首次激活正常；P 的鼠标继续、键盘继续和恢复焦点正常。
5. 动画播放中直接停止 PIE，再 Play；连续三次，站定后 1/2/3 都能用，无 Accessed None。
6. 保存三个 AM、三个 IA、IMC_Default、ABP_Manny、ABP_Quinn 和 BP_ThirdPersonCharacter。

本批不要求录正式视频，也未验证实际闪避位移或受击打断。若当前动作在暂停后恢复异常，把“按了什么、在什么时候按、恢复后发生什么”说清楚即可，我来排查。

## 7. 反馈与排障材料

```text
M0-05C1 完成 / 卡在第 __ 节
闪避第 0 帧左上臂 Z=25，关闭重开确认并重新烘焙：是 / 实际结果
当前使用的闪避 AS：
三个 Montage 独立预览：正常 / 实际结果
ABP_Manny 复用 DefaultSlot，Always Update Source Pose=True：是 / 实际结果
ABP_Manny / ABP_Quinn / 角色蓝图 Compile：通过 / 报错
1/2/3 播放并自动回到待机：通过 / 实际结果
长按单次 / 播放中不重启 / 结束后可再触发：通过 / 实际结果
移动和空中拒绝，站定重新按键可触发：通过 / 实际结果
动作中暂停恢复 / 菜单期间按键不补播：通过 / 实际结果
E / P / 移动 / 转向 / 跳跃和三次 PIE：正常 / 实际结果
资产已保存：是 / 否
实际用时：约 __ 分钟
```

| 问题 | 优先提供 |
|---|---|
| Compile 失败 / Accessed None | 完整错误文字和所在资产 / 函数名；不要只说“报错了” |
| 按键完全没有成功日志 | IA 事件到函数的截图、IMC 新增三行截图，并说明是否站定且视口有焦点 |
| 有 Play 成功日志但角色不动 | ABP_Manny 完整 AnimGraph、Montage Slot 轨道、角色 Mesh 的 Anim Class 截图 |
| 找不到某个节点 / 引脚 | 当前图表及搜索结果截图，保留你从哪个对象拖出的线 |
| 重播、卡住、暂停恢复时序异常 | 5–10 秒短视频，附按键顺序和 Output Log 相关文字 |

你不用先判断根因；编译、引用关系、继承或复杂时序问题由我接手。当前函数是 M0 的集中试验入口，之后 C2 会结合明确的动作状态处理移动和中断，正式 Gameplay 阶段再迁入计划中的组件 / 能力架构。
