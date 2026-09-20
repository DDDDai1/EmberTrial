# M0-04C：暂停菜单与输入恢复

更新：2026-09-20。引擎：UE 5.5.4。前置：M0-04A / B 已通过。状态：**已完成；用户确认全部验收项通过，M0-04 总任务完成。下一批见 [M0-05A](M0_05A_AnimationSource.md)**。

原预计 60–120 分钟，用户实际约 **80 分钟**。本批只改蓝图、UMG 和输入资产，无 C++ 改动。以下保留操作步骤供复盘；证据见 [M0-04C 与总任务验收记录](Reports/M0_04C_Verification.md)。

## 1. 本批范围与操作约定

本批实现一个全屏暂停遮罩、标题和“继续游戏”按钮：

- 游戏中**按下并松开 P**，显示菜单，世界暂停，鼠标光标出现，交互提示隐藏。
- 菜单打开时，WASD、鼠标转向、跳跃和 E 不操作角色。
- 点击“继续游戏”，或在按钮获得焦点时按 Enter / Space，恢复游戏。
- 菜单内再次**按下并松开 P**也恢复游戏。
- 关闭后鼠标光标隐藏，视口恢复输入，不必额外点击视口；目标状态保持，提示重新同步。
- 反复开关只复用一个菜单 Widget，恢复后没有卡键或重复交互。

本批 P 的动作发生在**松开时**，这是明确的原型操作约定。游戏侧使用 IA_Pause.Completed，菜单侧使用 UMG OnKeyUp。暂不绑定 Esc，以免与 PIE 的停止快捷键冲突，也不改你的编辑器快捷键。暂不添加设置页、退出游戏或手柄专项导航。

| 对象 | 职责 |
|---|---|
| IA_Pause / 已有 IMC_Default | 游戏中收到 P 松开时的开菜单请求 |
| BP_ThirdPersonCharacter | 暂存菜单实例与打开状态，协调暂停 / 输入模式 / 提示恢复 |
| PlayerController 0 | 接收输入模式、鼠标光标和焦点设置 |
| WBP_PauseMenu | 展示菜单、发送“请求继续”通知，处理菜单自己的键盘事件 |

M0 继续沿用角色管理 UI 的现有结构；正式阶段统一迁到 PlayerController / LocalPlayer 层。Widget 不 Cast 到角色，不直接设置游戏暂停。菜单通过 Event Dispatcher 通知创建者，避免把界面写死到具体角色类。

暂停世界、把输入交给 UI、设置焦点、显示光标是不同操作，必须分别处理。UI Only 时，游戏侧 Input Action 不能承担关菜单职责；本批用 Widget 的按键事件 / 按钮通知退出。参考 [Epic：Set Input Mode UI Only（5.5）](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/Blueprint/UWidgetBlueprintLibrary/SetInputMode_UIOnlyEx?application_version=5.5)。

## 2. 创建 WBP_PauseMenu

### 2.1 Designer 布局

在已有 `/Game/EmberTrial/ui` 下新建 **Menus** 文件夹，新建 User Widget 蓝图 **WBP_PauseMenu**。目录使用工程已有的 `ui`，不另建大小写不同的 UI 文件夹。

验收时实际保存路径是 `/Game/EmberTrial/ui/Menu/WBP_PauseMenu`（Menu 为单数）；后续直接使用这个已验证资产，不为目录差异重新创建或迁移。

推荐层级：

```text
Canvas Panel
  Border（全屏深色半透明背景）
    Vertical Box（水平、垂直居中）
      Text Block：游戏已暂停
      Button：BTN_Resume
        Text Block：继续游戏
      Text Block：松开 P 或选择继续游戏返回
```

1. Border 的 Canvas Slot 锚点设为全屏拉伸：Min=(0,0)，Max=(1,1)，四个 Offset 全为 0；Brush Color 设为深色，Alpha 约 0.8。
2. Vertical Box 在 Border Slot 中水平 / 垂直居中；按钮上下留出约 20 的间隔，内容 Padding 约 (48,16)。标题字号约 32，按钮文字约 24。
3. BTN_Resume 勾选 **Is Variable** 和 **Is Focusable**。Button 的 Click Method / Press Method 保持 **Down And Up**，在一次按下并松开后触发点击。
4. WBP_PauseMenu 的 **Class Defaults → Is Focusable 勾选**；Widget 与全屏 Border 的 Visibility 保持 **Visible**，允许菜单接收输入。
5. Compile / Save。菜单的可点击性与交互提示不同，不要把整个菜单设成 Not Hit-Testable。

### 2.2 发出“继续”通知

1. 切换 Graph，在 My Blueprint 的 **Event Dispatchers** 点击 `+`，命名 **OnResumeRequested**，不添加参数。
2. 回 Designer，选 BTN_Resume，在 Details → Events 的 **OnClicked** 旁点 `+`。
3. 在生成的 OnClicked(BTN_Resume) 事件后，把 OnResumeRequested 拖入图表，选择 **Call**。
4. 白线：**OnClicked(BTN_Resume) → Call OnResumeRequested**。不在这里调用 Set Game Paused。

Event Dispatcher 的 Call 是“发出通知”，不是调用角色函数。角色会在第 3 节绑定一个接收事件。

### 2.3 让键盘默认操作“继续游戏”按钮

新增普通函数 **FocusResumeButton**，无 Inputs / Outputs，Pure 不勾选：

1. Get BTN_Resume，从其输出拖线创建 **Set User Focus**，Target=BTN_Resume。
2. 添加 **Get Owning Player**（当前 Widget 的拥有者），返回值接 Set User Focus.Player Controller。
3. 白线：**FocusResumeButton 入口 → Set User Focus**。

一定从 BTN_Resume 的引用拖线，选择 Widget 的 Set User Focus；不要选需要 Event Reply 的同名函数。这个函数只会在菜单已经 AddToViewport 之后调用。

### 2.4 菜单内松开 P 请求关闭

1. 在 Graph 的 My Blueprint → Functions 旁找到 **Override / 重写** 下拉，选择 **OnKeyUp**，不是 OnKeyDown，也不是创建同名普通函数。
2. 从入口 **In Key Event** 拖线，创建 **Get Key**；从 Key 输出拖线创建 **Equal (Key) / == (Key)**，另一个 Key 选择键盘 **P**。
3. 入口白线接 Branch，Condition 接这个相等比较。
4. Branch.True → **Call OnResumeRequested** → Return Node；Return Value 接 **Handled** 节点的 Event Reply。
5. Branch.False → 另一个 Return Node；Return Value 接 **Unhandled** 节点的 Event Reply。

```text
OnKeyUp(InKeyEvent)
  → Branch(Get Key(InKeyEvent) == P)
      True  → Call OnResumeRequested → Return Handled
      False → Return Unhandled
```

Handled / Unhandled 是返回给 Slate 的事件处理结果，不是 bool；它们通常只有数据引脚。处理了 P 就返回 Handled，其他键保留按钮默认行为。焦点在按钮上时，未被按钮处理的 P 松开事件会沿 Widget 路径传递给菜单。

本机 5.5.4 的 UserWidget.h、KismetInputLibrary.h 已核对 OnKeyUp / GetKey 签名。UI 的按键和按钮事件在世界暂停时仍可处理，不需要给角色启用 Tick While Paused。

Compile / Save WBP_PauseMenu。

### 2.5 点击菜单背景后恢复按钮焦点

为避免点击空白背景后键盘焦点丢失，在 WBP_PauseMenu 再 Override **OnMouseButtonDown**：

1. 入口白线 → **FocusResumeButton（Target=Self）** → Return Node。
2. Return Value 接 **Handled** 的 Event Reply。

这处理传到菜单的背景点击；按钮自身正常处理点击，不在这里发送 OnResumeRequested。点击背景只恢复焦点，不继续游戏。不要用每帧 Set Focus 持续争抢焦点。Compile / Save。

## 3. 角色只创建一次菜单、只绑定一次通知

打开已有 **BP_ThirdPersonCharacter**，添加：

| 变量 | 类型 | 默认值 |
|---|---|---|
| PauseWidget | WBP_PauseMenu **Object Reference** | None |
| bPauseMenuOpen | Boolean | False |

两者都不勾选 Instance Editable / Expose on Spawn。再创建两个普通函数 **OpenPauseMenu**、**ClosePauseMenu**，无输入 / 输出，Pure 不勾选；先保留空入口，后面填内容。

在现有 **Event BeginPlay** 中，保留交互提示创建、AddToViewport、HideTarget、RefreshCandidate 等已通过的链路，在末尾追加：

1. **Create Widget**：Class=WBP_PauseMenu，Owning Player=**Get Player Controller 0**。
2. Return Value → **Set PauseWidget**。
3. 带执行引脚的 **Is Valid(PauseWidget)**，有效分支接下一步。无效分支 Print String：`[M0-04C] Create PauseWidget failed`，Print to Log 勾选，结束该分支。
4. 从 PauseWidget 输出拖线，搜索 **Bind Event to OnResumeRequested**，Target 自动接 PauseWidget。
5. 从 Bind 节点的红色 **Event** 引脚拖线，选择 **Add Custom Event**，命名 **EV_ResumeRequested**。
6. EV_ResumeRequested 的白色执行输出先接 Print String：`[M0-04C] Resume requested`，Print to Log 勾选，再接 **ClosePauseMenu（Target=Self）**。这一条日志用来检查通知是否重复绑定。

```text
已有 BeginPlay 初始化
  → Create Widget(WBP_PauseMenu, Owning Player = PC0)
  → Set PauseWidget
  → Is Valid(PauseWidget)
      Is Valid → Bind Event to OnResumeRequested
                    Target = PauseWidget
                    Event  = EV_ResumeRequested（红色委托线）

EV_ResumeRequested → 打印 Resume requested → ClosePauseMenu(Self)
```

**Bind 的执行线和 EV_ResumeRequested 的执行线是两条独立流程。** BeginPlay 只完成绑定，不应立刻执行 ClosePauseMenu；只有 Widget Call 通知时才执行接收事件。

这一步只创建、持有实例并绑定，不 AddToViewport。不要每次打开时 Create Widget，也不要在 Widget 的 Construct 中反复绑定角色事件。

## 4. OpenPauseMenu：暂停、显示、移交输入

进入角色的 OpenPauseMenu，按顺序连线。普通函数可以添加多个无输出 Return Node，作为提前结束：

### 4.1 前置检查与暂停

1. 入口 → Branch(bPauseMenuOpen)。True 直接 Return，防止重复打开；False 继续。
2. **Is Valid(PauseWidget)**。无效分支打印 `[M0-04C] PauseWidget invalid` 后 Return；有效继续。
3. **Get Player Controller 0 → Is Valid**。无效直接 Return，有效继续。
4. 调用 **Set Game Paused**，Paused=True；执行输出接 Branch，Condition 接它的 Return Value。
5. False 打印 `[M0-04C] Pause failed` 后 Return；True 继续显示菜单。该节点的 bool 表示是否成功，见 [Epic：SetGamePaused（5.5）](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Kismet/UGameplayStatics/SetGamePaused?application_version=5.5)。

### 4.2 成功后显示和聚焦

1. **Set bPauseMenuOpen=True**。
2. **Stop Jumping（Target=Self）**，结束可能仍保持的跳跃按键状态；不用 Stop Movement Immediately 清除角色运动速度。
3. 调用已有 **RefreshCandidate（Target=Self）**；第 6 节会让它在菜单打开时隐藏交互提示。
4. **Add to Viewport（Target=PauseWidget，ZOrder=100）**。
5. 从 Get Player Controller 0 拖线，搜索 **Set Show Mouse Cursor**，Target=PC0，值=True。
6. **Set Input Mode UI Only**：Player Controller=PC0，In Widget to Focus=PauseWidget，Mouse Lock Mode=Do Not Lock，**Flush Input=True**（若隐藏，展开高级引脚）。
7. **FocusResumeButton（Target=PauseWidget）**。这是第 2.3 节的函数；Target 不能留空为角色 Self。
8. 临时保留一条成功日志：Print String `[M0-04C] Pause opened`，Print to Log 勾选。

```text
通过检查 → Set Game Paused(True) 成功
  → bPauseMenuOpen=True → Stop Jumping → RefreshCandidate
  → PauseWidget.AddToViewport(ZOrder=100)
  → PC0.ShowMouseCursor=True
  → Set Input Mode UI Only(PC0, PauseWidget, Flush Input=True)
  → PauseWidget.FocusResumeButton → 打印 Pause opened
```

顺序是先把菜单放到视口，再把焦点交给菜单 / 按钮。不使用 Delay 猜测焦点时序，也不添加 Event Tick。

## 5. ClosePauseMenu：解除暂停并恢复游戏输入

进入角色 ClosePauseMenu：

1. 入口 → Branch(bPauseMenuOpen)。False 直接 Return，防止重复关闭；True 继续。
2. Is Valid(Get Player Controller 0)，无效直接 Return；有效继续。
3. **Set Game Paused，Paused=False**。
4. 后接 Branch，Condition 来自 **Is Game Paused**：True 表示世界仍暂停，打印 `[M0-04C] Resume failed` 后 Return，菜单保持可见；False 才执行清理。
5. 加一个 **Sequence**，用于让 Widget 无效时也能恢复其他状态：
   - Then 0 → Is Valid(PauseWidget) → 有效时 **Remove from Parent（Target=PauseWidget）**。
   - Then 1 → 按下方顺序恢复输入。
6. Then 1 → **Set Show Mouse Cursor=False（Target=PC0）**。
7. → **Set Input Mode Game Only**，Player Controller=PC0，**Flush Input=True**。
8. → **Set Focus to Game Viewport**。
9. → **Set bPauseMenuOpen=False** → **RefreshCandidate（Target=Self）**。
10. → Print String `[M0-04C] Pause closed`，Print to Log 勾选。

```text
确认菜单打开 → Set Game Paused(False)
  → Branch(Is Game Paused)
      True → 打印 Resume failed → Return
      False → Sequence
          Then 0 → PauseWidget 有效时 RemoveFromParent
          Then 1 → PC0.ShowMouseCursor=False
                 → Set Input Mode Game Only(PC0, Flush Input=True)
                 → Set Focus to Game Viewport
                 → bPauseMenuOpen=False → RefreshCandidate
                 → 打印 Pause closed
```

Sequence 的 Then 0 没有潜在执行节点，执行完后自然进入 Then 1；不需要从 Then 0 的两个分支各复制整套输入恢复。

**正常关闭时不清空 PauseWidget，也不解绑通知。** 它仍由角色持有，下次复用；只有角色结束生命周期时才清理引用。开 / 关两侧显式 Flush Input，避免沿用进入菜单前的按键状态；Set Focus to Game Viewport 用来恢复视口焦点。相关节点在 [Epic UWidgetBlueprintLibrary（5.5）](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/Blueprint/UWidgetBlueprintLibrary?application_version=5.5)，并已核对本机 WidgetBlueprintLibrary.cpp。

## 6. 与已有交互逻辑衔接

### 6.1 菜单打开时隐藏交互提示

在现有 **RefreshCandidate** 中，保留遍历、Set CurrentTarget 和 Is Valid(PromptWidget)。只在 **PromptWidget 有效分支**增加 Branch(bPauseMenuOpen)：

- True → **HideTarget（Target=PromptWidget）**，结束此分支。
- False → 接回现有 **Is Valid(CurrentTarget)** 以及接口取文本 / ShowPrompt 的逻辑。

```text
更新 CurrentTarget → Is Valid(PromptWidget)
  → Branch(bPauseMenuOpen)
      True  → PromptWidget.HideTarget
      False → 原有 CurrentTarget 有效性检查和提示显示 / 隐藏
```

候选仍按原逻辑维护，关闭菜单后重新读取目标真实状态，不手动缓存一份可能过期的提示文字。

### 6.2 交互入口明确拒绝暂停状态

在 **TryInteractWithCurrentTarget** 入口加 Branch，Condition 使用 **Boolean OR**：`bPauseMenuOpen OR Is Game Paused`。

- True → Return，不发交互请求。
- False → 接回 B 阶段原来的 RefreshCandidate 和后续接口调用。

IA_Interact 的 Trigger When Paused 继续关闭。暂停状态不应激活 A / B，也不应在恢复后一口气补执行菜单期间按过的 E。

### 6.3 EndPlay 清理

#### 6.3.1 先明确要打开哪个蓝图

这里指的是 **BP_ThirdPersonCharacter 的 Event Graph（事件图表）**，不是 BP_ETPracticeActor，也不是 WBP_PauseMenu。

原文“在已有角色 Event EndPlay……”的说法预设了 M0-04A 第 5.4 节已经添加角色的清理事件，表达不够准确。**EndPlay 是引擎提供的生命周期事件，但图表中不一定预先存在它的节点。没添加过，就需要手动添加。** 物体蓝图里用于销毁清理的 EndPlay 和这里的角色 EndPlay 是两个不同对象上的事件。

EndPlay 在该 Actor 结束参与世界时由引擎调用，例如停止 PIE 或 Actor 被移除；不是每次关掉暂停菜单都会触发。节点定义已核对本机 UE 5.5.4 的 Actor.h / ReceiveEndPlay；可对照 [Epic AActor 中的 ReceiveEndPlay](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AActor)。

#### 6.3.2 查找现有事件

1. 回内容浏览器，打开 **Content → ThirdPerson → Blueprints → BP_ThirdPersonCharacter**。
2. 在蓝图编辑器左侧 **My Blueprint / 我的蓝图 → Graphs / 图表** 中，双击 **EventGraph / 事件图表**。若没有左侧面板，通过 Window / 窗口菜单打开 My Blueprint。
3. 确认当前画布页签是 EventGraph，不是 OpenPauseMenu、ClosePauseMenu 或其他函数图表。
4. 按 **Ctrl+F**，在当前蓝图内搜索 `EndPlay`；没有结果时再搜索带空格的 `End Play`，中文界面也可搜索 `结束运行`。
5. 如果有 **Event EndPlay / Event End Play** 事件节点的结果，双击结果跳转到节点，继续第 6.3.4 节。搜索结果若只是注释文字，不代表已有事件。

#### 6.3.3 如果没有，就添加引擎提供的事件

1. 仍在 **BP_ThirdPersonCharacter 的 EventGraph**，在画布空白处右键，保持 **Context Sensitive / 情境关联** 勾选。
2. 搜索 **End Play** 或 **EndPlay**，选择红色事件类别的 **Event EndPlay / Event End Play**。
3. 添加后应看到一个**红色事件节点**，有白色执行输出，以及 **End Play Reason** 输出。End Play Reason 本批可以不连接。
4. 不要创建一个 Custom Event 再把名字改成 EndPlay；自定义同名事件不会因此自动接入引擎生命周期。也不要在 Functions 的 `+` 中创建 EndPlay 函数。

同一个角色蓝图只实现一个这样的事件入口。如果已有节点就使用原节点。若在事件图表空白处右键仍搜索不到，发当前蓝图名称、EventGraph 页签和搜索菜单同框截图，我再定位。

#### 6.3.4 已有旧清理链时，追加暂停菜单清理

若 EndPlay 已连着 PromptWidget / Candidates 的清理逻辑，保留它：

1. 空白处右键创建 **Sequence**。原来若 EndPlay 已接 Sequence，直接使用原 Sequence，跳过插入新节点。
2. 若需插入：把 **Event EndPlay 的白色输出 → Sequence 输入**；把旧清理链第一个节点的执行输入接到 **Sequence.Then 0**。插入时调整原执行线，不删除旧节点。
3. 在 Sequence 底部点击 **Add pin / 添加引脚**，准备两个额外的执行输出。新建的 Sequence 默认有 Then 0 / Then 1，只需再加一个 Then 2。
4. 旧清理链用 Then 0，两个额外输出分别接下面两条链。如果复用原 Sequence，使用它后面新增的输出，不挤占原来已有分支。

```text
Event EndPlay → Sequence
  Then 0 → 原有交互提示 / 候选清理链
  Then 1 → Is Valid(PauseWidget)
               Is Valid → Unbind All Events from OnResumeRequested
                              Target = PauseWidget
                        → Remove from Parent
                              Target = PauseWidget
               Is Not Valid → 不连接
  Then 2 → Set PauseWidget = None → Set bPauseMenuOpen = False
```

从 PauseWidget 的 Get 引用输出拖线，创建 **Unbind All Events from OnResumeRequested** 和 Widget 的 **Remove from Parent**，确保两个 Target 都接 PauseWidget，不能留为角色 Self。

设置 None 的方法：拖入变量选择 **Set PauseWidget**，它的对象值输入保持未连接、默认 None。Set bPauseMenuOpen 的布尔值不勾选。**Then 2 要直接接 Sequence，不能接在 Is Valid 的有效分支后面**，这样 Widget 无效时也会清空状态。

#### 6.3.5 如果角色此前完全没有 EndPlay 清理

如果第 6.3.3 节才新建事件、没有旧清理链，就一次补齐四条分支，不需要先虚构一条“原有逻辑”：

1. EndPlay 白线接 Sequence，点击 Add pin，使其具有 Then 0 / Then 1 / Then 2 / Then 3。
2. 按下面连接。Is Valid 都使用带白色执行引脚的版本；对象引用的 Set 值不连接，默认 None。

```text
Event EndPlay → Sequence
  Then 0 → Is Valid(PromptWidget)
               Is Valid → Remove from Parent(Target = PromptWidget)
               Is Not Valid → 不连接
  Then 1 → Set PromptWidget = None
         → Set CurrentTarget = None
         → Clear(Candidates 数组)
  Then 2 → Is Valid(PauseWidget)
               Is Valid → Unbind All Events from OnResumeRequested(Target = PauseWidget)
                        → Remove from Parent(Target = PauseWidget)
               Is Not Valid → 不连接
  Then 3 → Set PauseWidget = None
         → Set bPauseMenuOpen = False
```

清空数组：将 **Candidates** 拖入图表选 Get，从数组输出拖线搜索 **Clear**，Target Array 接 Candidates，白线接在 Set CurrentTarget 后面。这里清空的是正在结束的角色的全部候选；与物体正常离开范围时只 RemoveItem 一个候选不同。

Sequence 按顺序执行各分支，不是并行执行。上述分支没有 Delay 等潜在节点；某个 Is Valid 的无效分支没有后续，不会阻止 Sequence 继续执行下一条 Then。

此处处理当前 M0 停止 PIE / 结束关卡的 Widget 生命周期；不调用 ClosePauseMenu 去刷新结束中的 HUD 或抢回视口焦点。本批没有角色重生、联机、跨关卡 UI 保留；正式 UI 迁到 PlayerController 时再处理这些生命周期。

Compile / Save 角色蓝图。Play 后停止一次，确认没有 Accessed None，再继续第 7 节；第 8 节还会验证菜单打开时停止 PIE 的情况。

## 7. 接入 P 输入

1. 在 `/Game/EmberTrial/Input/Actions` 创建 **IA_Pause**，Value Type=Digital (bool)，Triggers / Modifiers 为空，**Trigger When Paused=False**，保存。
2. 在实际使用的 `/Game/ThirdPerson/Input/IMC_Default` 增加 IA_Pause → 键盘 **P**；此映射 Triggers / Modifiers 也为空。保留现有 Jump / Move / Look / Interact。
3. 在 BP_ThirdPersonCharacter.Event Graph 添加 **Enhanced Input Action IA_Pause** 事件。
4. 展开节点，**Completed → OpenPauseMenu（Target=Self）**；Started / Triggered 等其余执行输出不连接。

无附加触发器的布尔动作，按下激活、松开从 Triggered 进入 None，产生 Completed；这不是所有触发器配置都成立的通用“松开事件”。本机 5.5.4 InputTriggers.h 已核对定义。这里让开菜单发生在松开时，菜单自己也在 P 松开时关闭，避免同一次按键的剩余松开事件被误当成关闭请求。

不要把开关菜单写成 FlipFlop。明确的 Open / Close 和 bPauseMenuOpen 能让按钮、键盘以及重复通知共用同一套状态。

Compile / Save All。第一次 Play 后点击游戏视口取得输入焦点，再按下并松开 P。

## 8. 逐步验收

先测前三项，任何一项失败就停在该层反馈，不继续同时改多处。

| 检查 | 预期 |
|---|---|
| 首次松开 P | 一个暂停菜单出现，有光标；日志 Pause opened 一条 |
| 鼠标点击继续 | 菜单消失、光标隐藏；移动 / 鼠标转向立即可用，不用再点击视口；Pause closed 一条 |
| 键盘继续 | 重新打开后，Enter / Space 能操作默认获得焦点的继续按钮；再次打开后松开 P 也能关闭 |
| 暂停是否真实生效 | 跳起后快速松开 P，菜单期间角色保持在空中，关闭后继续下落；菜单只覆盖画面不算通过 |
| 暂停时阻断游戏操作 | 菜单期间尝试 WASD / 转向 / 空格 / E，角色不执行这些游戏动作；E 不生成 Activated 日志。注意按钮有焦点时空格用于点击继续，属于 UI 操作 |
| 按键释放恢复 | 按住 W 时用 P 打开菜单，在菜单内松开 W，再点继续；不再按 W 时不能持续自行移动，重新按 W 能正常响应。跳跃键做一次类似检查 |
| 不补执行 E | 靠近未激活目标，开菜单期间按下并松开 E，继续后目标仍未激活；重新按 E 才首次激活 |
| 交互提示恢复 | 在已激活 / 未激活 / 无目标三种状态分别开关菜单，暂停时隐藏提示，恢复后文字 / 隐藏状态正确 |
| 点击背景后的焦点 | 打开菜单后点击深色背景，再松开 P 应可关闭；如焦点丢失，按第 9 节反馈，不加 Tick 抢焦点 |
| 反复开关 10 次 | 每次一条 opened / Resume requested / closed，只有一个菜单，没有重复通知；每次输入和光标都恢复 |
| 连续 3 次 PIE | 其中一次在菜单打开时停止 PIE，再次 Play 不残留菜单 / 光标 / 暂停状态，无 Accessed None |
| A / B 回归 | 出生重叠、切换 / 回退、离开隐藏、独立激活与重复保护正常 |

Set Game Paused 会冻结普通世界计时，因此物体的 Life Span 暂停期间不流逝是预期，不能把它当作销毁清理失效。本批不需要重新添加寿命测试；A / B 的销毁回归已有记录。

上面“点击背景后 P”若失败，给我第 2.5 节和 FocusResumeButton 的图表，以及点击的具体位置，检查实际焦点路径。

本批已由用户实机验收通过。opened / Resume requested / closed 三种事件日志保留为开发证据，后续统一整理。不能仅凭 closed 一条就判断绑定只有一次，因为重复关闭会被 bPauseMenuOpen 的检查挡住；应一起检查 Resume requested 的次数。

## 9. 报错与反馈

| 现象 | 提供材料 |
|---|---|
| P 没反应 | IA_Pause / IMC 配置、Completed 连接截图；说明视口是否有焦点 |
| 菜单闪现后立刻关闭 | 输入事件和 OnKeyUp 截图，说明是否混用了 Started / Completed |
| 有菜单但按钮点不了 | Widget Visibility、输入模式参数和光标设置截图 |
| 鼠标可关闭，键盘不行 | Is Focusable、FocusResumeButton、OnKeyUp 完整连线截图，说明点击背景前后是否不同 |
| 点击继续多次调用 | BeginPlay 的 Create / Bind 和 OnClicked 截图，粘贴 opened / closed 日志 |
| 恢复后卡键 / 需点视口 | 说明暂停前按住什么键、何时松开；提供 Open / Close 的输入模式、Flush Input、焦点节点截图 |
| Accessed None / 编译错误 | 首条日志原文和上下文，加错误指向的节点截图 |

```text
M0-04C 完成 / 卡在第 __ 节
蓝图 Compile：通过 / 错误文字
P 松开打开 / 关闭：通过 / 实际结果
鼠标继续 / Enter 或 Space 继续：通过 / 实际结果
世界暂停与恢复：通过 / 实际结果
暂停期间无游戏交互、恢复后不补执行 E：通过 / 实际结果
按住移动键暂停、菜单中松键、恢复不卡键：通过 / 实际结果
关闭后无需点击视口即可移动 / 转向：通过 / 实际结果
交互提示恢复与 A / B 回归：通过 / 实际结果
点击背景后 P 关闭：通过 / 实际结果
连续 10 次开关 / 3 次 PIE：通过 / 实际结果
资产已保存：是 / 否
实际用时：约 __ 分钟
```

C 现已通过，M0-04 的 A / B / C 整体完成。下一批为 [M0-05A：零成本动画路线与第一段攻击占位](M0_05A_AnimationSource.md)。M0-06 打包尚未执行，不能把编辑器内通过当成最终试玩包通过。
