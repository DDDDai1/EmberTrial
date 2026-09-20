# M0-04B：按键交互与结果刷新

更新：2026-09-20。引擎：UE 5.5.4。前置：M0-04A 已通过。状态：**已完成；用户确认全部验收项通过。下一批见 [M0-04C](M0_04C_PauseMenu.md)**。

原预计 45–90 分钟，用户实际约 **40 分钟**。本批只改蓝图和输入资产，无 C++ 改动。以下保留实施步骤供复盘，证据见 [M0-04B 验收记录](Reports/M0_04B_Verification.md)。

## 1. 本批完成后是什么效果

走近 A，提示“按 E 激活：Target_Changed”。按一次 E，提示立即变为“已激活：Target_Changed”。离开后隐藏，再回来仍显示“已激活”。B 有自己的状态。已激活的物体再次收到请求，会拒绝重复执行。重新 Play 后，两者重新从未激活开始。

本批用文字变化和成功日志验证交互，暂不添加奖励、存档、动画或材质效果。范围重叠仍按 M0-04A 的候选顺序选择目标；已激活目标仍留在候选中，可以查看其状态。要操作另一个目标，移动使其成为当前目标。

```text
IA_Interact.Started（一次按下）
  → 角色刷新并选出 CurrentTarget
  → 向目标发送 TryInteract 接口消息
  → 目标检查玩家、当前重叠、是否已激活
  → 成功才修改目标自己的状态
  → 角色重新读取目标提示，更新已有 Widget
```

| 对象 | 本批职责 |
|---|---|
| IA_Interact / IMC_Default | 定义交互动作及 E 键映射 |
| BP_ThirdPersonCharacter | 接收输入、选择目标、发请求、刷新显示 |
| BPI_PracticeInteractable | 约定交互请求与提示读取的函数签名 |
| BP_ETPracticeActor | 检查交互条件、保存自身状态、生成提示文本 |
| WBP_InteractionPrompt | 接收整段 FText，设置文字和可见性 |
| C++ AETPracticeActor | 继续提供 GetPracticeLabel；本批不修改 |

两个接口方向不同：已有 BPI_PracticeInteractionReceiver 是“物体通知角色登记 / 移除”；新增 BPI_PracticeInteractable 是“角色请求物体交互 / 查询提示”。不要把物体的激活状态存到 Widget 或角色中。

这是 M0 单机原型。候选数组仍使用现有 ETPracticeActor 引用；正式阶段再迁到计划中的 C++ 交互接口 / 组件。本批不宣称已经实现通用交互框架。

## 2. 新增接口

在 `/Game/EmberTrial/Blueprints/Interfaces` 新建 Blueprint Interface，命名 **BPI_PracticeInteractable**，添加：

| 函数名 | Inputs | Outputs |
|---|---|---|
| TryInteract | Interactor：**Character Object Reference** | Succeeded：**Boolean** |
| GetInteractionPrompt | 无 | PromptText：**Text** |

选择对应函数，在 Details 中分别通过 Inputs / Outputs 的 `+` 添加参数。Interactor 不选 Class Reference；PromptText 不选 String。删除不用的初始函数，Compile / Save。

这两个函数有输出，因此在实现接口的蓝图中以**函数图表**实现，并通过 Return Node 返回结果；不是创建无输出的 Custom Event。接口本身不保存状态、不写实现。参考 [Epic Blueprint Interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprint-interface-in-unreal-engine?application_version=5.5)。

## 3. 物体实现交互规则

### 3.1 接口和状态

1. 打开 **BP_ETPracticeActor → Class Settings → Implemented Interfaces → Add**，加入 BPI_PracticeInteractable，Compile。
2. 添加 Boolean 变量 **bActivated**，默认 **False**，Instance Editable / Expose on Spawn 不勾选。
3. 在 My Blueprint 的 Interfaces 区找到 **TryInteract** 和 **GetInteractionPrompt**，双击进入对应函数图表。不要再创建同名普通函数。
4. 保留已通过的 BeginPlay 初始同步、BeginOverlap、EndOverlap 和 EndPlay。

### 3.2 TryInteract：先检查，再改状态

进入 TryInteract 函数后，应看到入口上的 Interactor 输出，以及 Return Node 上的 Succeeded 输入。**每个失败分支都返回 False，只有实际首次激活返回 True。**

按以下顺序搭建：

1. 从函数入口白线接一个带白色执行引脚的 **Is Valid**，Input Object 接入口 **Interactor**。
2. Is Not Valid 接一个 **Return Node**，Succeeded 不勾选（False）。可复制原有 Return Node，分别接失败分支。
3. 从 Interactor 拖线创建 **Get Capsule Component**。从 Components 拖入 **InteractionRange**，从其引用创建 **Is Overlapping Component**。
4. 重叠检查的 **Target = InteractionRange**，**Other Comp = Interactor 的 CapsuleComponent**。Is Valid 的有效执行输出接第一个 Branch；重叠检查 Return Value 接 Condition。
5. 第一个 Branch 的 False 接 Return Node，Succeeded=False；True 接第二个 Branch。
6. 第二个 Branch.Condition 接 **Get bActivated**。True 表示已经激活，接 Return Node，Succeeded=False。
7. 第二个 Branch 的 False 接 **Set bActivated=True**，然后接 Return Node，Succeeded=True。

```text
TryInteract(Interactor)
  → Is Valid(Interactor)
      Is Not Valid → Return False
      Is Valid → Branch(InteractionRange 与 Interactor.Capsule 重叠)
          False → Return False
          True → Branch(bActivated)
              True → Return False
              False → Set bActivated = True → Return True
```

这是**按键发生时再次检查范围**。显示提示时的检查不能替代交互时的检查。重复请求会返回 False，不再次改变状态。这里不设置寿命、不销毁物体、不操作 UI、不清空角色候选、不添加 Delay。

### 3.3 GetInteractionPrompt：返回当前真实状态对应的文字

1. 打开 GetInteractionPrompt 的函数图表，入口白线接 Branch，Condition 接 bActivated。
2. 添加两个 **Format Text** 节点，Format 分别填写：

```text
未激活（False）：按 E 激活：{TargetName}
已激活（True）：已激活：{TargetName}
```

只填写冒号后面的提示文本，不把上面的“未激活（False）”说明写进去。

3. 调用已有 **Get Practice Label（Target=Self）**，其 Text 输出分别接两个 Format Text 的 TargetName。
4. Branch.False 接一个 Return Node，PromptText 接未激活文本的 Result；Branch.True 接另一个 Return Node，PromptText 接已激活文本的 Result。
5. 两个 Return Node 可以通过复制得到；Format Text 是纯数据节点，不接白线。Compile / Save。

函数只读取标签和 bActivated，不修改状态。这里的 E 是本批固定键位提示；以后支持改键 / 手柄时再从输入映射提供键位文字。

## 4. 让 Widget 接收完整提示文本

在已有 **WBP_InteractionPrompt** 中新增普通函数 **ShowPrompt**：

- Inputs：**InPrompt，Text**；无 Outputs；Pure 不勾选。
- Get TXT_Prompt → **SetText (Text)**，Target=TXT_Prompt，In Text=InPrompt。
- Self → Widget 的 **Set Visibility**，Target=Self，In Visibility=**Not Hit-Testable (Self & All Children)**。
- 白线：**ShowPrompt 入口 → SetText → Set Visibility**。

这次 InPrompt 已是完整文本，不再在 Widget 中拼“可交互”前缀。现有 HideTarget 保持 Collapsed。原 ShowTarget 暂保留，下一节把角色调用切换到 ShowPrompt；不要在两个显示函数之间同时连执行线。Compile / Save。

实际资产目前位于 `/Game/EmberTrial/ui/Practice/WBP_InteractionPrompt`，使用这个已有资产，不因文件夹大小写另建副本。

## 5. 角色按目标状态刷新提示

打开 **BP_ThirdPersonCharacter → RefreshCandidate**。候选遍历、NewTarget 清空和 CurrentTarget 赋值保持现有正确逻辑。只修改最后的显示部分：

1. 在 **Is Valid(CurrentTarget)** 的有效分支后，接一个 Branch。
2. 添加 **Does Object Implement Interface**，Test Object=CurrentTarget，Interface=BPI_PracticeInteractable；布尔结果接这个 Branch.Condition。
3. Branch.True 接 **Get Interaction Prompt (Message)**，选择带信封的接口消息，Target=CurrentTarget。
4. 这个消息调用有白色执行引脚和 PromptText 输出。它的执行输出接 **ShowPrompt**；ShowPrompt.Target=PromptWidget，InPrompt=接口返回的 PromptText。
5. Branch.False 接 PromptWidget.HideTarget，避免意外的不支持接口目标留下旧提示。
6. **Is Valid(CurrentTarget) 的 Is Not Valid → HideTarget** 保持正确；PromptWidget 的无效分支不调用任何 Widget 函数。

```text
完成候选遍历 → Set CurrentTarget
  → Is Valid(PromptWidget)
      Is Valid → Is Valid(CurrentTarget)
          Is Not Valid → HideTarget(Target = PromptWidget)
          Is Valid → Branch(当前目标实现 BPI_PracticeInteractable)
              False → HideTarget(Target = PromptWidget)
              True → Get Interaction Prompt (Message, Target = CurrentTarget)
                   → ShowPrompt(Target = PromptWidget, InPrompt = PromptText)
```

新消息节点的 Target 是**物体**；ShowPrompt 的 Target 是 **Widget**。二者不要接反。Compile / Save。

此时可先 Play 做一次中间检查：靠近未激活物体，应显示“按 E 激活：名称”；离开应隐藏。E 尚未接入，没有交互效果属于预期。

## 6. 添加 Enhanced Input 动作

### 6.1 创建 IA_Interact

1. 在 `/Game/EmberTrial` 下创建 **Input/Actions** 文件夹，在其中右键 **Input → Input Action**，命名 **IA_Interact**。
2. 打开资产，Value Type 设为 **Digital (bool)**。
3. 本批 Triggers / Modifiers 保持空，**Trigger When Paused 关闭**；其他值保持默认，保存。

本批使用布尔动作的 **Started** 响应一次按下，不把 Started 和 Triggered 同时接入交互。在这份无附加触发器的配置中，Triggered 会在持续输入期间重复触发；Started 适合这里的单次按下入口。若以后添加 Hold / Chord 等条件，应按触发条件重新选择事件，不能把 Started 理解为所有触发器的“成功完成”。依据见 [Epic Enhanced Input](https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine)，并已核对本机 5.5.4 的 InputTriggers.h。

### 6.2 加到已有 IMC_Default

1. 在 BP_ThirdPersonCharacter 的 **Class Defaults** 搜索 **Default Mapping Context**，确认实际指向已有 **IMC_Default**。使用旁边的浏览按钮可定位资产。
2. 预期路径是 `/Game/ThirdPerson/Input/IMC_Default`。若实际指向别的资产，先反馈该名称 / 路径，不直接覆盖已有绑定。
3. 打开该 IMC，在 Mappings 新增一条，Input Action 选新建的 **IA_Interact**，Key 选键盘 **E**。
4. 这条映射的 Triggers / Modifiers 也保持空，保留原 Jump / Move / Look 映射，保存。

当前 C++ 的 SetupPlayerInputComponent 已对本地玩家添加 DefaultMappingContext，本批复用它。不再额外 Add Mapping Context，不给练习物体 Enable Input，也不添加旧式键盘 E 事件。

### 6.3 先验证按下一次只进入一次

1. 打开 BP_ThirdPersonCharacter 的 Event Graph，空白处右键搜索 **IA_Interact**，添加 **Enhanced Input Action IA_Interact** 事件。
2. 展开节点，使用 **Started** 执行输出。其余输出本批不接。
3. 临时接一个 Print String，In String 填 **[M0-04B] Input Started**，Print to Log 勾选；只在这一步用它诊断输入。
4. Compile / Save，Play 后点击游戏视口获得焦点。按住 E 两秒，Output Log 应只新增一条这条日志；松开再按，应再新增一条。
5. 停止 Play，**移除临时 Print String**，按下一节连接正式逻辑。输入不通时先修这一层，不同时改目标规则。

## 7. 发起交互并刷新结果

在 **BP_ThirdPersonCharacter** 新建普通函数 **TryInteractWithCurrentTarget**，无输入 / 输出，Pure 不勾选。

1. 函数入口先调用 **RefreshCandidate（Target=Self）**，更新 / 清理当前选择。
2. 然后接带执行引脚的 **Is Valid(CurrentTarget)**。Is Not Valid 不接后续；范围外按 E 安全结束。
3. 有效分支接 Branch；Condition 来自 Does Object Implement Interface，Test Object=CurrentTarget，Interface=BPI_PracticeInteractable。
4. Branch.True 接 **Try Interact (Message)**，Target=CurrentTarget，**Interactor=Self（当前角色）**。False 不接后续。
5. Try Interact 的执行输出接新的 Branch，Condition 接它的 **Succeeded** 输出。
6. Succeeded=True：先用 Print String 输出一次成功记录，再调用 RefreshCandidate。成功记录用 Format Text 拼下面的文本，TargetName 接 **CurrentTarget.GetPracticeLabel()**，Result 经 Text To String（可自动转换）接 Print String.In String，Print to Log 勾选：

```text
[M0-04B] Activated {TargetName}
```

7. Succeeded=False：直接调用 RefreshCandidate，不打印成功记录。可以放第二个 RefreshCandidate 调用节点分别接两条分支，每次只会走其中一条。
8. 回 Event Graph，把 **IA_Interact.Started → TryInteractWithCurrentTarget（Target=Self）**，Compile / Save。

```text
IA_Interact.Started → TryInteractWithCurrentTarget

函数内：
RefreshCandidate → Is Valid(CurrentTarget)
  → 检查目标实现接口
  → Try Interact (Message)
       Target = CurrentTarget（物体）
       Interactor = Self（角色）
  → Branch(Succeeded)
       True  → Print String 成功记录 → RefreshCandidate
       False → RefreshCandidate
```

输入层一次按下触发一次；目标层 bActivated 阻止重复执行，两者作用不同。失败也刷新是为了让 UI 继续反映实际目标状态。交互成功后不需要再次走出 / 走进范围才能更新文字。

本批 TryInteract 不销毁目标、没有潜在执行节点，因此成功分支可以立即读取目标标签。后续若变为拾取后销毁，需要调整引用校验 / 提前保存显示数据；不把本批这段调用链直接照搬到那种流程。

## 8. 实机验收

所有相关蓝图 Compile，通过后 Save All。确认临时输入测试日志已移除、临时 Set Life Span 已移除或为 0。使用 Play，点击视口取得输入焦点；开局仍使用 Default Player Start。

| 检查 | 预期 |
|---|---|
| 范围外按 E | 不激活任何物体，无成功日志、无 Accessed None |
| 靠近 A，按一次 E | 提示立即从“按 E 激活：Target_Changed”变为“已激活：Target_Changed”，成功日志一条 |
| 对 A 长按 / 连按 | 不再次出现 A 的成功日志，不产生重复结果 |
| 离开已激活 A，再回来 | 离开隐藏，回来仍显示已激活 |
| B 的独立状态 | 激活 A 不影响 B；B 仍可首次激活并产生自己的一条成功日志 |
| 重叠范围中按 E | 只激活当前提示指向的目标；切换 / 回退仍与 M0-04A 一致 |
| 出生就在范围内后按 E | 无需先移动，能正确显示提示并首次激活 |
| 重开 Play | A / B 恢复未激活，运行状态没有保存回编辑器资产 |
| 目标销毁回归 | 可临时设置 30 秒寿命，激活后销毁仍隐藏或回退；销毁后按 E 不调用无效对象；测试后恢复寿命设置 |
| 基础输入 / 连续 PIE | 移动、转向、跳跃正常；连续三次 Play 无重复 Widget 或 Accessed None |

日志中两个实例首次激活各一条是交互结果证据；第 6.3 节的临时日志才用于验证输入频率。不能仅凭“成功日志只有一次”证明输入没有每帧重复，因为目标层本身也会拒绝重复请求。

## 9. 失败时怎样反馈

| 现象 | 给我什么 |
|---|---|
| 接口 / 函数编译失败 | Compiler Results 首条错误文字，加对应函数入口 / Return Node 截图 |
| E 没反应 | IA_Interact 设置、IMC 的 E 映射、角色输入事件截图；说明第 6.3 节是否有日志 |
| 有输入但不激活 | TryInteractWithCurrentTarget 与目标 TryInteract 连线截图；说明是否在范围内 |
| 激活后文字不变 | GetInteractionPrompt、RefreshCandidate、ShowPrompt 连线截图 |
| 重复成功日志 | 输入事件使用哪个引脚、TryInteract 中 bActivated 分支与 Set 的截图 |
| Accessed None | Output Log 错误和调用链文字，加涉及节点截图 |

复制日志文字；节点和类型用截图。你不必先猜根因；如难以完整描述，我读取工程和日志并接管编辑器排查。

```text
M0-04B 完成 / 卡在第 __ 节
蓝图 Compile：通过 / 错误文字
输入单次触发（第 6.3 节）：通过 / 实际结果
范围外按 E：正常 / 实际结果
A / B 首次激活与提示立即更新：通过 / 实际结果
长按 / 连按不重复成功：通过 / 实际结果
离开重进 / 双范围 / 出生重叠：通过 / 实际结果
目标销毁回归：通过 / 未测 / 实际结果
重开 Play 状态重置：通过 / 实际结果
移动 / 转向 / 跳跃和连续三次 Play：正常 / 实际结果
临时测试设置已恢复，资产已保存：是 / 否
实际用时：约 __ 分钟
```

B 现已通过，下一批按 [M0-04C：暂停菜单与输入恢复](M0_04C_PauseMenu.md) 操作。本批没有实现暂停菜单，M0-04 总任务仍未完成。
