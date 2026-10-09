# M1-02C2-02：真正的新 Pawn 替换与 UI 重绑

生成 / 验收日期：2026-10-03。前置：[C2-01 已验收](Reports/M1_02C2_01_Verification.md)。**本批及 M1-02 总任务已通过，用户约 19:45 完成，手动 30 分钟。** 证据见 [总验收报告](Reports/M1_02_Verification.md)，下一步按 [M1-03A1 ASC 基础接入](M1_03A1_ASCFoundation.md) 操作。下文保留测试步骤供复查，不用重做已通过项。

原估计手动 45–75 分钟，实际由用户 30 分钟完成；没有新增 C++ 或 Build / 索引等待。助手读取日志与资产复核，没有修改 Blueprint 或代执行测试。

## 1. 本批验证什么

C2-01 的 4→5 是同一角色失去 / 恢复控制。这次创建一个独立的测试 Pawn，验证：

1. 原角色 → 新测试 Pawn → 原角色，状态面板跟随当前对象。
2. 换回原角色后销毁旧测试 Pawn，当前 UI 不被清空。
3. 销毁正在控制的测试 Pawn，UI 显示 None；再次生成新实例后立即恢复。
4. 测试 Pawn / 空 Pawn 下重建 Widget，快照正确；被移除的旧 Widget 不再接收事件。

这验证的是 Controller / Pawn / UI 生命周期。测试 Pawn 没有动作、交互和暂停逻辑；正式角色重生、ASC / GameplayReady 在后续阶段实现。

不复制 BP_ThirdPersonCharacter：它已有 M0 BeginPlay / 输入 / UI，复制一套会给生命周期测试加入额外干扰。原角色保持存活，按 5 返回后再验收原玩法。

## 2. 准备与资产

1. 退出 PIE，Save All，打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`。
2. World Settings 保持 BP_ETGameMode；不要更改 GameMode 的 Default Pawn Class。
3. 打开 BP_ETPlayerController：一个 UIBootstrap，StatusWidgetClass 保持 WBP_ETLifecycleStatus。C2-01 的临时 4/5/6/7 应已删除；本批重新建立不同逻辑。
4. 新建目录 `/Game/EmberTrial/Tests/Blueprints`；创建 Blueprint Class，**父类选 Pawn**，命名 **BP_ETLifecycleProbePawn**。
5. **不要把这个 Pawn 放到地图里**，后面由 Spawn Actor 生成。它不替代正式玩家 Blueprint。

在 Probe Pawn 的 Components 中保留 DefaultSceneRoot，再添加：

| 组件 | 设置 |
|---|---|
| Static Mesh，命名 ProbeMesh | Static Mesh=/Engine/BasicShapes/Cube；Location=(0,0,0)，Rotation=(0,0,0)，Scale=(0.3,0.3,0.3)；Collision Presets=NoCollision，Simulate Physics=False |
| Camera，命名 ProbeCamera | 挂在 DefaultSceneRoot 下；Location=(-300,0,150)，Rotation=(0,-15,0)，Scale=(1,1,1)；Auto Activate=True；Use Pawn Control Rotation=False |

Class Defaults：Auto Possess Player=Disabled，Auto Possess AI=Disabled。不给 Probe 添加移动、Enhanced Input、UI 或 PracticeAction；Compile / Save。Probe 控制期间镜头固定、角色不能移动，是本次测试的预期表现。

### 2.1 加一条销毁证据日志

在 Probe 的 Event Graph 空白处右键，搜索 **Event EndPlay**；连接到 Print String。

- Print String 展开高级引脚，**Print to Log=True**，Print to Screen 可保持 True。
- In String 接 **Append**：A 填 `[M1-02C2-02] Probe EndPlay Pawn=`，B 接 **Get Object Name**。
- Get Object Name 的 Object 接 **Self**。这里记录实例名，使用 Get Object Name，避免 Actor Label 与 UI 中实例名不同。
- EndPlay 的执行线只接 Print String；本测试 Pawn 没有其他 EndPlay 逻辑。

Compile / Save。按 6 前的请求日志加上这条 EndPlay，能明确区分销毁与单纯 UnPossess；停止 PIE 也会产生 EndPlay，是正常的。

## 3. Controller 添加两个临时变量

在 BP_ETPlayerController 的 My Blueprint → Variables 添加：

| 名称 | 类型 | 默认值 / 设置 |
|---|---|---|
| ETTestOriginalPawn | **Pawn Object Reference** | None；Instance Editable=False，不选 Class Reference / Soft Reference |
| ETTestProbePawn | **Pawn Object Reference** | 同上 |

Compile 一次。这两个引用只存在于临时测试；不要写进 C1 Subsystem 或 Widget。

## 4. 创建三个普通蓝图函数

均在 BP_ETPlayerController → Functions 点击 + 创建，**Pure 不勾选**。后文的 Is Valid 指带白色执行引脚、输出 Is Valid / Is Not Valid 的版本。

Return Node 可以复制成多份；每个分支末端接对应 Return。输入 / 输出在选中函数入口或 My Blueprint 函数名称后的 Details 中添加。

### 4.1 ET_TestCacheOriginalPawn：只缓存原角色

无输入；添加一个 **Boolean 输出 Success**。

执行线：

~~~text
函数入口 → Is Valid（Input Object=Get ETTestOriginalPawn）
  Is Valid     → Return（Success=True）
  Is Not Valid → Cast To BP_ThirdPersonCharacter
                   成功 → Set ETTestOriginalPawn → Return（Success=True）
                   Cast Failed → Return（Success=False）
~~~

数据线：

- Cast 的 Object 接 **Get Controlled Pawn，Target=Self** 的 Return Value。
- Cast 的 As BP Third Person Character 接 Set ETTestOriginalPawn 的值。
- 各 Return 的 Success 分别勾 True / 取消勾 False，不接到其他变量。

该函数不调用 Possess / UnPossess。第一次在原角色上按 4 时保存原角色；之后即使控制 Probe 或当前 Pawn=None，也使用已经保存且有效的原角色，不覆盖它。

Compile。若找不到 Cast 类，确认本项目原玩家 Blueprint 仍为 BP_ThirdPersonCharacter。

### 4.2 ET_TestGetOrSpawnProbe：有效则复用，失效则创建

无输入；添加 **Pawn Object Reference 输出 ProbePawn**。

执行线：

~~~text
函数入口 → Is Valid（Input Object=Get ETTestOriginalPawn）
  Is Not Valid → Return（ProbePawn=None）
  Is Valid → Is Valid（Input Object=Get ETTestProbePawn）
    Is Valid → Return（ProbePawn=Get ETTestProbePawn）
    Is Not Valid → Spawn Actor from Class
                → Set ETTestProbePawn
                → Is Valid（Input Object=Get ETTestProbePawn）
                    Is Valid → Print String → Return（ProbePawn=Get ETTestProbePawn）
                    Is Not Valid → Print String → Return（ProbePawn=None）
~~~

配置 Spawn Actor from Class：

| 引脚 | 连接 / 配置 |
|---|---|
| Class | BP_ETLifecycleProbePawn |
| Spawn Transform | **Get Actor Transform** 返回值；该节点 Target 接 **Get ETTestOriginalPawn** |
| Collision Handling Override | **Always Spawn, Ignore Collisions**；ProbeMesh 已 NoCollision |
| Owner / Instigator | 留空 |
| Return Value | 接 Set ETTestProbePawn 的值；不要接 Get Controlled Pawn |

成功 Print String：Append A=`[M1-02C2-02] Probe spawned Pawn=`；B=Get Object Name（Object=Get ETTestProbePawn）。**Print to Log=True**。

失败 Print String：In String=`[M1-02C2-02] Probe spawn failed`，Print to Log=True；Return 的 ProbePawn 保持 None。

每个有效返回的数据线都是 **Get ETTestProbePawn → Return 的 ProbePawn**。无效返回留 None。只有有效原角色才进入 Get Actor Transform；不要把 Spawn 节点改成纯函数，不加 Delay。

Compile。该函数只找 / 创建对象，不执行 Possess。

### 4.3 ET_TestPossessIfDifferent：有效且不同才控制

添加 **Pawn Object Reference 输入 TargetPawn**；无输出。

~~~text
函数入口 → Is Valid（Input Object=入口的 TargetPawn）
  Is Not Valid → Return
  Is Valid → Branch（Condition=Get Controlled Pawn == TargetPawn）
    True  → Return
    False → Possess → Print String → Return
~~~

数据线：

- Get Controlled Pawn：**Target=Self**。
- 比较节点选择 **Equal (Object)**；A 接当前 Get Controlled Pawn，B 接函数输入 TargetPawn。
- Possess 的 **Target=Self（本 PlayerController）**，**In Pawn=函数输入 TargetPawn**。两者不能接反。
- Print String：Append A=`[M1-02C2-02] Possess complete Current=`；B=Get Object Name（Object=Get Controlled Pawn，Target=Self）；Print to Log=True。

**True 是当前已控制目标，直接 Return；False 才调用 Possess。** 这保留 C1 已验证的重复按键保护。无需先手动调用 UnPossess，引擎会处理不同 Pawn 的切换。

Compile / Save。

## 5. Event Graph 接临时数字键

使用键盘顶部 **4 / 5 / 6 / 7**，均连 **Released**；不接 Pressed，不配置新的 Input Mapping Context。键盘事件放在 **Controller**，Probe / 空 Pawn 时仍由 Controller 接收。

### 5.1 数字 4：生成 / 复用 Probe 并控制

~~~text
4 Released → ET_TestCacheOriginalPawn → Branch（Condition=输出 Success）
  False → Print String（见下）
  True  → ET_TestGetOrSpawnProbe → ET_TestPossessIfDifferent
~~~

- 最后一个调用的 TargetPawn 接 GetOrSpawnProbe 的输出 **ProbePawn**。
- 所有自定义函数调用的 Target 都是 Self。
- False Print String：`[M1-02C2-02] Original Pawn not cached; stop PIE and start from original character`，Print to Log=True。
- 不加 UnPossess，不改 Default Pawn，不主动写 UI 文本。

### 5.2 数字 5：返回原角色

`5 Released → ET_TestPossessIfDifferent`；TargetPawn 接 **Get ETTestOriginalPawn**。

无效缓存由函数拒绝；已经控制原角色时不重复 Possess。必须先按一次 4 保存原角色，再使用 5。

### 5.3 数字 6：只销毁测试 Pawn

~~~text
6 Released → Is Valid（Input Object=Get ETTestProbePawn）
  Is Not Valid → 不连接后续
  Is Valid → Print String → Destroy Actor → Set ETTestProbePawn=None
~~~

- Print String：Append A=`[M1-02C2-02] Probe destroy requested Pawn=`；B=Get Object Name（Object=Get ETTestProbePawn）；Print to Log=True。
- **Destroy Actor 的 Target=Get ETTestProbePawn**。
- Set ETTestProbePawn 的值不接线，默认 None。
- **不使用 Get Controlled Pawn 作为销毁目标。** 这样能分别验证“销毁旧 Probe”和“销毁当前 Probe”，并保留原角色供返回。
- 不调用 SetText / Rebuild / UnPossess，不清 ETTestOriginalPawn。

### 5.4 数字 7：重建现有状态面板

`7 Released → UIBootstrap 的 Rebuild Status Widget`。

从 Components 拖出已有 **UIBootstrap 的 Get**，接 Rebuild Status Widget 的 Target；不创建新组件，不额外 Create Widget / Add to Viewport。

Compile / Save。若出现警告 / 错误，先提供文字，不进入下节。

## 6. 分组实机验收

每次开始前退出暂停菜单、站定，建议在 A/B 交互范围外。仅在 Viewport 获得输入焦点后按键；Probe / None 阶段用 4/5/6/7 测试，返回原角色后再测试玩法。

### 6.1 原角色 → Probe → 原角色

1. Play：一份状态面板，Pawn 是 BP_ThirdPersonCharacter 实例。
2. 按 **4**：日志有 Probe spawned、Possess complete；PawnText 变为 **BP_ETLifecycleProbePawn 实例**，仍只有一份面板。固定镜头 / 不能移动为预期。
3. 再按两次 **4**：复用同一 Probe；没有新的 spawned / Possess complete，不出现额外 Pawn changed / PawnEvent。
4. Probe 上按 **7** 两次：每次新 Widget 的 ConstructSnapshot 都是当前 Probe；旧 Widget removed / unsubscribed，仍只有一份面板。
5. 按 **5**：返回原角色，面板显示原角色。短测移动 / 转向 / 跳跃；再次 5 不重复 Possess。

**正常切换日志允许出现 Original → None → Probe，返回时也可能 Probe → None → Original。** Possess 内部会先解绑再绑定，不要求一次按键只能有一条 Pawn changed。要求是每次实际状态变化只被当前 Widget 处理一次，最终 Pawn 与面板一致；ConstructSnapshot 不算重复 PawnEvent。

### 6.2 销毁已经离开控制的旧 Probe

承接 6.1，当前必须是原角色：

1. 按 **6**：出现 Probe destroy requested 与对应 Probe EndPlay。
2. **原角色仍受控制，面板继续显示原角色，不变成 None**；不应因为旧 Probe 的 EndPlay 新增清空当前 Pawn 的 C1 / Widget 事件。
3. 再按 6：无有效 Probe，不执行销毁，不报 Accessed None。
4. 按 **4**：再次出现 Probe spawned，控制新生成的 Probe，面板立即显示它。

对象名后缀可能因引擎命名规则变化，不规定必须是 C_1 / C_2。以“旧 Probe 已 EndPlay、缓存被清空、随后有新的 Spawn 日志”为新实例证据；不要仅凭跨 PIE 名称相同判定复用。

### 6.3 销毁当前 Probe → None → 新实例恢复

1. 承接上节，正在控制 Probe，按 **6**：请求 / EndPlay 日志，状态面板仍在，显示暂无 Pawn / None。
2. 再按 6：无动作、无错误。
3. 空 Pawn 时按 **7**：立即得到 ConstructSnapshot Pawn=None，一份面板。
4. **直接按 4**：原角色缓存仍有效，重新 Spawn Probe 并控制它；UI 立即显示这个实例，不需要先按 5 / 重建面板。
5. 在新 Probe 上再按 7，快照正确；按 **5** 返回原角色，重复 5 无重启。

最后做三轮 **4→5**，可复用当前有效 Probe，不要求每轮销毁 / 生成。每轮显示最终当前对象，旧 Widget 不再收到 PawnEvent。

### 6.4 PIE 生命周期与清理

1. 完成上述测试后 Stop；应有当前 Widget 的 removed / unsubscribed 和 C1 Controller / LocalPlayer 清理。Probe 若仍存活，Stop 时 EndPlay 正常。
2. 重新 Play，原角色 / 一份面板正常，缓存从 None 开始；做一次 **4→7→6→4→5**，然后 Stop。
3. 第三次 Play 做 **4→5→6** 后 Stop；无旧世界对象误用、无 Accessed None。

日志搜索 `M1-02C1`、`M1-02C2`、`M1-02C2-02`；面板只读，不应改变输入模式或抢焦点。出现问题按第 8 节反馈，不继续删除诊断节点。

## 7. 删除临时测试入口并回归

仅在第 6 节全部通过后：

1. Stop PIE；BP_ETPlayerController 删除本批 4/5/6/7 事件及调用 / Print String 节点。
2. 删除三个临时函数：ET_TestCacheOriginalPawn、ET_TestGetOrSpawnProbe、ET_TestPossessIfDifferent；删除两个 ETTest 变量。
3. **保留 UIBootstrap、StatusWidgetClass 与 WBP_ETLifecycleStatus，不改 C1 / C2 C++。** Compile / Save。
4. BP_ETLifecycleProbePawn 保留在 Tests/Blueprints，作为隔离测试资产；不放入地图、不设为默认 Pawn。其 EndPlay 日志可保留。
5. 在 L_ET_VerticalSlice Play：一份面板；原 1/2/3、2→3、E-A/B、P 鼠标 / Enter / Space / 背景关闭、无补播 / 卡键、移动 / 转向 / 跳跃正常。
6. Stop，Save All；如果菜单 / 原角色输入不正常，先反馈，不调整 Gameplay 实现去适配没有玩法的 Probe。

本批不需要重新 Cook、录制组合视频或重复 30/60/120 FPS 闪避矩阵。临时 Probe 也不是正式重生功能或新的玩家身体架构。

## 8. 报错与结果反馈

优先提供 **出错章节、触发按键 / 操作顺序、预期 / 实际结果和错误原文**。编译 / Output Log 复制文字；连线问题截相应函数或键盘分支，包含相关数据线、执行线和引脚；时序问题再用短视频。你不必先判断根因。

只需给大致完成时间，我会读取本机日志，不要求手动整理整套日志。测试失败时保留临时节点供排查；所有 C++ 修改继续默认写进文档，由你执行，除非你明确要求我直接修改。

~~~text
M1-02C2-02 完成 / 卡在第 __ 节
Probe 父类 Pawn / 不放地图 / NoCollision / AutoPossess Disabled：是
三个临时函数 / 两个变量 / Controller 4–7；Compile：通过
原角色→Probe→原角色；重复 4 / 5 无重复 Possess：通过
Probe 上 7 重建 / 旧 Widget 无后续 PawnEvent：通过
销毁已离开控制的旧 Probe，原角色与 UI 不受影响：通过
再次 4 新 Spawn / UI 显示新实例：通过
销毁当前 Probe→None；空 Pawn 的 7 快照：通过
空 Pawn 直接 4 新建并恢复 / 5 返回原角色：通过
三轮 4→5 / 三次 PIE 清理 / 无 Accessed None：通过
临时按键、三个函数和两个变量已删除；组件 / Widget 保留：是
Probe 留在 Tests、不设默认 Pawn；地图与资产保存：是
最终原 1/2/3、2→3、E/P、移动 / 转向 / 跳跃：正常
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟
~~~

本批已通过，源文件 / 资产 / 日志已复核并形成 M1-02 总验收；M1-03A1 方案已下发，后续逐批接入属性 / GameplayReady 与 HUD。
