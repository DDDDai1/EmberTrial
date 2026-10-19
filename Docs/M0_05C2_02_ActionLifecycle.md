# M0-05C2-02：动作状态、输入限制与受击打断

2026-10-19第19批清理同步：以下配置、接入及验收属于M0历史，当前左键 / 1、2、Q已走正式GAS；Practice只保留兼容及原回归入口。完整代码以Source链接为准，本轮按用户授权由助手直接清理；实际新检查和提交清单见[Cleanup](Cleanup.md)。

> 2026-09-28 验收更新：用户已完成本批全部实机测试并保存，实际约 **15 分钟**，C2-02 验收通过。下文保留实现和复现步骤，不需重做。现在执行 [C2-03A 手动新增闪避配置](M0_05C2_03A_DodgeConfiguration.md)。

原交接估时：手动 35–55 分钟；实际约 15 分钟。引擎：UE 5.5.4。正常 Build、1 项综合自动测试及用户蓝图 / PIE 验收已通过，覆盖范围见 [本批检查记录](Reports/M0_05C2_Verification.md)。以下操作保留供复现。

## 1. 本批要做出的效果

1 播放攻击，2 播放闪避姿势，3 播放受击；动作期间普通移动、跳跃和 E 交互被阻止，镜头与 P 仍可用。3 可以打断攻击或闪避，受击自己不能被连按重启。动作结束后恢复正常操作。

**按 2 仍不产生实际闪避位移**；这是 C2-03 的内容。C2-01 基础装配已由你用约 40 分钟完成，本批不用重新创建组件或重抄旧代码。

与 C1 的两处规则变化：

- C1 三种动作都只允许站定；现在攻击仍要求地面且水平速度 <=3 cm/s，闪避 / 受击允许在地面移动时发起，接受后会停止当前普通移动。
- C1 忙碌时三键都忽略；现在受击具有更高优先级，可以打断攻击和闪避。

| 当前状态 | 按 1 | 按 2 | 按 3 |
|---|---|---|---|
| Idle，地面站定 | 攻击 | 闪避姿势 | 受击 |
| Idle，地面移动 | 拒绝 | 停止普通移动，播放闪避姿势 | 停止普通移动，播放受击 |
| Attacking | 拒绝 | 拒绝 | 打断后进入 HitReact |
| Dodging | 拒绝 | 拒绝 | 打断后进入 HitReact |
| HitReact | 拒绝 | 拒绝 | 拒绝 |
| 空中 / 暂停 | 拒绝 | 拒绝 | 拒绝 |

本轮没有取消窗口、输入缓存、受击伤害或无敌帧；3 仍是调试输入。

### 当前兼容层的清理边界

[声明](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.h) / [实现](../EmberTrial/Source/EmberTrial/ETPracticeActionComponent.cpp)保留同一组反射入口和单一CurrentState。Ended与BlendingOut共用私有IsCurrentActionCallback，按结束标记 → Serial → Montage → 非Idle核对；旧回调不能关闭新动作。

ReleaseAction先推进Serial，再释放自有RMS、清动作状态 / 引用 / Tick；按精确动画实例解绑自身委托后才Stop。位移到期不提前开放输入，动画结束才回Idle。取消淡出0.05秒与RMS优先级500仅在cpp内部命名，数值保持。

这不是正式GAS的第二份状态：新能力继续拥有自己的Tag / Task，Practice只参与既有Idle / Cancel兼容。以下数字键接线描述是当时开发步骤，当前工程不回接旧动作输入。

## 2. C++ 已承担的工作

| 文件 / 入口 | 职责 |
|---|---|
| ETPracticeActionComponent.h / .cpp | 私有运行时状态、三个可配置 Montage、请求校验、受击打断、结束 / 取消 / EndPlay 清理 |
| RequestPracticeAction(RequestedState) | 返回 True 表示请求成功开始；False 表示拒绝，不排队 |
| GetPracticeActionState / IsPracticeActionIdle | 保留 C2-01 的只读查询 |
| EmberTrialCharacter.cpp | Move 和跳跃 Started 入口读取组件状态，忙碌时拒绝；跳跃 Completed 仍执行 StopJumping |
| Tests/ETPracticeActionTests.cpp | 临时测试世界中的实际 Montage 生命周期与失效路径测试，不改玩家资产 |

原来的 CurrentState 是 public 且没有 Transient，现已收回为 private + Transient + BlueprintReadOnly。蓝图读取方式不变，状态转换由组件负责。

正常结束依赖该次 Montage 实例的结束回调，**包含淡出完成**。每次动作有一个序号；旧动作的迟到回调不会清除新受击。组件只停止自己持有的 Montage 实例，不调用 Stop All Montages 清空其他系统。

仅在忙碌期间启用轻量 Tick，用于发现 AnimInstance 被换掉或 Montage 实例丢失；Idle 时关闭。Tick 不播放动画、不轮询键盘，也不用固定 Delay 解锁。暂停仍由已有菜单冻结世界；本组件没有 Set Ignore Move Input 的计数式锁，不会顺手重置菜单或其他系统的输入状态。

## 3. 打开工程并配置组件

正常 Build 已成功，现在重新打开 `E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`。本批不需要你再次编译 C++。不要再复制 C2-01 文档中的旧类内容覆盖新实现。

1. 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`。
2. 在 Components 列表点击已有 **PracticeAction**，不是点击角色 Self，也不再添加第二个。
3. Details 搜索 `Montage`，找到新组件的三个属性，按表赋值。搜索选项中确认类型是 Anim Montage。

| 组件属性 | 实际资产 |
|---|---|
| Attack Montage | AM_M0_Attack_01_Montage |
| Dodge Montage | AM_M0_Dodge_01_Montage |
| Hit React Montage | AM_M0_HitResult_01_Montage |

三个都在 `/Game/EmberTrial/Animations/Practice/Montages`。填写的是**组件属性**，不是角色之前的 PracticeAttackMontage 等变量。旧变量暂留，避免清理时误删引用。

4. 清空搜索，确认组件 **Auto Activate=True**；Attack Max Horizontal Speed 保持 **3.0**。不必手动打开组件 Tick。
5. Compile、Save。旧代码已创建的只读查询节点应仍可用。

如果属性没有出现，先确认重新打开的是 5.5.4 工程并查看编译结果。不要反复移除 / 添加组件或重建类；反馈组件 Details 截图，我检查。

## 4. 三条输入改为请求组件

在角色 Event Graph 找到 C1 的三条 IA 输入。继续使用 **Started**，不改 Input Action / IMC 的 Trigger 设置。

先完成其中一条，再复制相同方式到另两条：

1. 把 Components 里的 **PracticeAction** 拖入图中，得到它的 Get 引用。
2. 从该引用蓝色输出拖线，搜索 **Request Practice Action**。Target 必须是 PracticeAction，不是 Self。
3. 在 Request 节点的 **Requested State** 下拉框选择对应状态。
4. 将 IA 的 Started 白色输出改接新 Request 节点。可以从 Started 拖到新节点执行输入以替换原连接；若旧线仍保留，按住 Alt 单击旧调用的执行输入断开旧线。
5. Request 的 bool Return Value 可以暂时留空；C++ 已打印接受 / 拒绝原因，暂不额外每次 Print。

| 原事件 | 执行引脚 | 新节点 Target | Requested State |
|---|---|---|---|
| IA_PracticeAttack | Started | PracticeAction | **Attacking** |
| IA_PracticeDodge | Started | PracticeAction | **Dodging** |
| IA_PracticeHit | Started | PracticeAction | **HitReact** |

旧的三个 `TryPlayPracticeMontage` 调用不再连到输入白线，移到 Comment `C1 Legacy - Disconnected` 中保留供对照。旧函数本身可以保留；**不能让一个按键同时经过旧函数和新组件**，也不要先调旧函数再调组件。

当前不再把 InMontage 作为输入参数传递。动画从组件的三个属性读取；相同动作的规则和资源选择只有一个入口。

Compile、Save。做到这里后不要用“仍出现 C1 Play 日志”作为新组件接线正确的证据，新入口日志前缀是 `[M0-05C2]`。

## 5. 忙碌时阻止 E 交互

打开角色函数 **TryInteractWithCurrentTarget**。既有的暂停检查应为 `bPauseMenuOpen OR Is Game Paused`，True → Return，False → 原交互检查。

在这条 **False（允许继续）**白线后、原目标有效检查前，插入一个新的 Branch：

1. 放置 PracticeAction 的 Get 引用。
2. 从引用拖出 **Is Practice Action Idle**。
3. 它的红色 bool 输出接新 Branch 的 **Condition**。
4. 原暂停 Branch 的 False 接新 Branch 的执行输入。
5. 新 Branch 的 **True → 原有交互检查链**；**False → Return**。

```text
TryInteractWithCurrentTarget
  → 已有暂停检查
      True  → Return
      False → Branch(Is Practice Action Idle)
                  True  → 原来的目标有效性 / 接口交互链
                  False → Return
```

这里 True 表示“没有动作占用”，必须继续交互。不要沿用暂停 Branch 的 True=返回方向。没有组件引用有效性问题时无需额外增加通用查找或每帧搜索。

Compile、Save。范围提示仍按 M0-04 显示，**动作中可以保留提示，但 E 不执行**；动作结束后再次按 E 才执行，不补执行之前的按键。提示样式的忙碌状态不在这批新增。

Move / Jump 的动作检查已在 C++ 完成，不再给蓝图增加 Set Ignore Move Input、Disable Movement、Set Movement Mode 或定时恢复。也不需要改角色 EndPlay：组件自行接收并处理它的 EndPlay。

## 6. 先读懂日志，再分组验收

Output Log 筛选 `[M0-05C2]`。常见日志如下；枚举前面可能带 `EETPracticeActionState::`：

```text
Component ready Owner=... State=Idle
Begin ...Attacking Montage=AM_M0_Attack_01_Montage Serial=...
Reject ...Dodging Reason=Busy
End ...Attacking -> Idle Reason=HitInterrupt
Begin ...HitReact Montage=AM_M0_HitResult_01_Montage Serial=...
End ...HitReact -> Idle Reason=Completed
```

受击打断时，旧动作的 End 和新受击 Begin 在同一请求内发生。中间的 Idle 日志用于记录释放旧动作，不能据此认为游戏插入了一帧可移动状态；最终仍以当前状态和实机行为为准。

| 拒绝原因 | 含义 / 处理 |
|---|---|
| Busy | 处于不允许被当前请求打断的动作中，属于规则拒绝 |
| Moving / NotGrounded | 攻击速度或地面条件不满足，不排队 |
| MissingMontage | 新组件的对应资源未赋值；不要只检查角色旧变量 |
| MissingAnimInstance | 当前 Mesh 没有 AnimInstance，反馈 Mesh 设置 |
| ExternalMontage | 组件空闲时仍有其他来源的 Montage；先查是否旧 C1 调用未断开 |
| NotReady | 组件未激活 / 未初始化 / 已结束；确认 Auto Activate 与组件装配 |
| PlaybackFailed | UE 未成功开始该 Montage，发完整日志与三个属性截图 |
| Paused | 世界暂停期间拒绝请求，恢复后需要重新按键 |
| InvalidAction | Requested State 错选成 Idle 等非动作值，核对三条输入 |
| MissingMontageInstance | 播放返回成功后无法找到实例，发完整日志，由助手排查 |

拒绝日志由一次 Started 请求触发，**拒绝本身不等于 Bug**。菜单使用 UI Only 时按键可能根本不送到组件，因此菜单内无 Reject 日志也正常。

### 6.1 普通播放与输入限制

1. 地面站定按 1，看到攻击与 Begin；结束产生 Completed，再可移动 / 跳跃。
2. 2、3 同样可以播放 / 结束，状态返回 Idle。
3. 按住 1 约 3 秒，只触发一次。动作中快速按 1/2 不重启，不在结束时补播。
4. 攻击中按住 W，应在动作结束前保持原地；**若一直不松 W，结束后继续移动是预期**。若在动作中已经松开，结束后不自行移动。
5. 动作中按下并松开空格，不跳；结束后不补跳，重新按空格正常。动作中持续按住空格直到结束，也不补跳；松开后重新按才跳。
6. 动作中移动鼠标，镜头仍可转动；按 P 可以暂停。
7. 移动时按 1 拒绝；移动时按 2/3 接受并停止普通移动。空中按三键均拒绝，落地后不补播。

### 6.2 受击打断与旧回调

1. 按 1 后约 0.2 秒按 3：攻击停止，受击立即开始，看到 HitInterrupt 与新的 HitReact Begin。
2. 受击期间连按 1/2/3，均不替换受击；受击结束后按 1 正常。
3. 按 2 后立即按 3，闪避姿势被打断；重复 5 次，过程中不提前恢复移动、不出现卡状态。
4. 测试同一帧附近的快速按键时，以先被处理的请求为准。先按 3 再按 1，结果应是受击；在动作已完全结束后按新键，正常接受不算重启。

### 6.3 E / P 与生命周期

1. 开一个新的 PIE，在尚未激活的 A 范围内站定。按 1 后立即按 E：A 不激活；攻击结束再按 E：正常首次激活。对 B 重复。
2. 攻击开始后马上松开 P，动画停住；继续后接着播完并恢复操作。
3. 菜单内按下 / 松开 1/2/3/E，继续后不补执行。按住 W 或 1 打开菜单、在菜单中松开、继续后不卡键 / 不自动重播。
4. 动作播放中停止 PIE，再 Play；连续三次，组件初始化、三键、移动、跳跃、交互都正常，无 Accessed None。
5. Compile / Save 所有改动的蓝图，恢复任何临时测试设置。

这批不要求你手动破坏资产去测异常路径。缺失资源、失败回退、外部停止、AnimInstance 更换、组件 Deactivate 与 Owner EndPlay 由助手自动测试覆盖；若实机出现这些问题仍直接反馈。

## 7. 反馈模板

```text
M0-05C2-02 完成 / 卡在第 __ 节
PracticeAction 三个 Montage 已赋值，Auto Activate=True：是 / 实际结果
三条 Started 只连新组件，旧 C1 调用已断开：是 / 实际结果
TryInteractWithCurrentTarget 已加入 Idle 检查：是 / 实际结果
角色蓝图 Compile：通过 / 错误原文
1/2/3 正常开始与结束，日志 Begin / Completed：通过 / 实际日志
长按单次 / 忙碌拒绝 / 无补播：通过 / 实际结果
动作期间移动 / 跳跃 / E 阻断，结束恢复：通过 / 实际结果
镜头仍可转动，按住 / 松开输入边界：通过 / 实际结果
地面移动时攻击拒绝、2/3 接受；空中三键拒绝：通过 / 实际结果
3 打断攻击 / 闪避，受击不能重启，连续五次：通过 / 实际结果
动作中暂停 / 菜单不补播 / 无卡键：通过 / 实际结果
动作中停止 PIE、连续三次和 A/B 回归：通过 / 实际结果
资产已保存、临时设置已恢复：是 / 否
实际用时：约 __ 分钟
```

编译 / 运行报错发完整文字；接线问题发截图，包含 IA 到组件、组件 Details 或交互函数入口；打断 / 暂停异常发按键顺序及 5–10 秒视频。若只看到旧 `[M0-05C1] Play`，先反馈输入链截图，不重做动画。

## 本次清理 Commit / Push（固定放在最后）

第19批四份Source与十份Markdown，共14项；当前检查、最新差异和逐文件清单见[Cleanup末尾](Cleanup.md)。历史资产 / 测试未改，不重复制造差异。用户重新Stage后Commit / Push；Raw、报告、索引留本地，助手不操作Git索引。
