# M0-04A：靠近显示交互提示

更新：2026-09-20。引擎：UE 5.5.4。状态：**已完成；用户确认第 6.4 节及相关回归全部通过。下一批见 [M0-04B](M0_04B_Interaction.md)**。

前置：M0-03 已通过。本批原估计 60–120 分钟；用户报告首次实现约 60 分钟，第 6.4 节补修约 10 分钟，已报告手动用时合计约 70 分钟，未包含此前未计时的协助排障。基础代码和蓝图由你手动完成，复杂问题由我接手。验收证据见 [M0-04A 验收记录](Reports/M0_04A_Verification.md)。以下保留实现与排障步骤，供复盘查阅。

## 1. 目标与分工

走近练习方块，屏幕底部显示“可交互：Target_Changed”或“可交互：Target_B”，离开隐藏。范围重叠时，**最后进入范围的有效目标优先**；离开当前目标，回退到仍在范围内的另一个目标。

这是事件先后优先级，不是空间距离优先级。出生时已同时处于两个范围，事件顺序可能不同，显示任一有效目标都可接受。本批没有按键交互，提示暂不写“按 E”。M0-04B 再接 Enhanced Input，M0-04C 再做暂停菜单和输入恢复。

```text
物体 Sphere Begin / End Overlap
    → Blueprint Interface 注册 / 移除候选目标
角色蓝图保存候选数组、选择当前目标
    → 读取 C++ GetPracticeLabel
    → 唯一一个 Widget 的 ShowTarget / HideTarget
UMG 只处理文本和可见性
```

| 对象 | 职责 |
|---|---|
| C++ AETPracticeActor | 保留已有实现，新增标签读取函数 |
| BP_ETPracticeActor | 范围检测和报告进出，不创建 Widget |
| BPI_PracticeInteractionReceiver | 定义注册 / 移除函数签名，不保存状态 |
| BP_ThirdPersonCharacter | 管理候选目标、当前目标和一个 Widget |
| WBP_InteractionPrompt | 接收 FText 并展示，不查询场景或选择目标 |

本批是单机 M0 原型，提示管理暂放角色蓝图，便于检查完整数据流；正式阶段迁到 PlayerController / LocalPlayer 层与 Presenter。暂不建立通用框架，不给 C++ Gameplay 模块新增 UMG 依赖。

新增资产：

```text
/Game/EmberTrial/Blueprints/Interfaces/BPI_PracticeInteractionReceiver
/Game/EmberTrial/UI/Practice/WBP_InteractionPrompt
```

修改现有 BP_ETPracticeActor、`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` 和 ThirdPersonMap 中的练习实例。不要另建一个没有被 GameMode 使用的角色蓝图。

## 2. 手动给 C++ 增加 getter

保存并关闭 UE 编辑器。在 `EmberTrial/Source/EmberTrial/ETPracticeActor.h` 的 **public** 区域、PrintPracticeInfo 附近增加：

```cpp
UFUNCTION(BlueprintPure, Category = "Practice")
FText GetPracticeLabel() const;
```

在对应 `.cpp` 增加：

```cpp
FText AETPracticeActor::GetPracticeLabel() const
{
    return PracticeLabel;
}
```

保留已有组件、构造函数和日志函数。getter 无写状态或输出日志的副作用，使用 BlueprintPure，为其他蓝图提供明确的读取入口。Pure 不代表每帧自动执行，调用频率取决于图表使用方式。

VS 选择 **Development Editor / Win64**，Build EmberTrial，成功后重开工程。本次新增反射函数，继续使用关闭编辑器后的正常编译。

## 3. 创建 Blueprint Interface

在指定 Interfaces 文件夹，新建 **Blueprints → Blueprint Interface**，命名 BPI_PracticeInteractionReceiver。定义两个函数：

| 函数 | 输入 | 输出 |
|---|---|---|
| RegisterCandidate | Candidate：ETPracticeActor **Object Reference** | 无 |
| UnregisterCandidate | Candidate：ETPracticeActor **Object Reference** | 无 |

选择 C++ ETPracticeActor 的对象引用，不选 Class Reference。删除不用的初始函数，编译保存。接口只定义签名，实现放在接收方，参见 [Epic UE 5.5 Blueprint Interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprint-interface-in-unreal-engine?application_version=5.5)。

## 4. 创建 UMG 提示

新建 User Interface → Widget Blueprint，父类 UserWidget，命名 WBP_InteractionPrompt。

### 4.1 Designer

1. 一个 Canvas Panel 作根，其下放 Border，再在 Border 内放 Text Block。
2. Border 的 Canvas Slot：底部居中锚点（Min / Max 都是 0.5,1），Alignment=(0.5,1)，Position=(0,-80)，Size 约 (500,64)。最终应在屏幕底部居中。
3. Border 半透明深色；Text Block 水平 / 垂直居中、字号约 24，命名 **TXT_Prompt**，勾选 **Is Variable**。
4. Widget 的 Is Focusable 关闭。Text 和 Visibility 都不要创建 Bind 函数。
5. 中文若显示方框，先用 `Target: {TargetName}` 验证功能，并记录字体问题；本批不采购字体或美术。

### 4.2 两个函数

这两个函数都由你在 **WBP_InteractionPrompt 的 Graph / 图表** 中创建。本节完成“提示界面接到命令后怎样显示 / 隐藏”；第 5 节的角色蓝图再负责调用它们。

| 函数 | 接收什么 | 做什么 |
|---|---|---|
| ShowTarget | 一段目标名称，例如 Target_B | 将 TXT_Prompt 改成“可交互：Target_B”，再显示整个提示界面 |
| HideTarget | 无参数 | 隐藏整个提示界面，文字内容可以保留 |

#### 4.2.1 创建 ShowTarget 和输入参数

1. 双击打开 **WBP_InteractionPrompt**，右上角从 Designer 切到 **Graph**。
2. 在左侧 **My Blueprint / 我的蓝图** 面板，找到 **Functions / 函数**，点击旁边的 `+`，命名 **ShowTarget**。若未显示该面板，可在 Window / 窗口菜单中重新打开 My Blueprint。
3. 双击 ShowTarget 打开函数图表。图中会有一个同名的函数入口节点。
4. 选中入口节点，在右侧 **Details / 细节 → Inputs / 输入** 点击 `+`，参数名填 **InLabel**，类型选 **Text / 文本**（对应 FText）。不要选 String / 字符串。本函数不需要 Outputs。
5. 函数 **Access Specifier 保持 Public**，**Pure 不勾选**。它要修改界面，必须保留白色执行引脚。

现在入口节点上应有一个白色执行输出，以及名为 InLabel 的文本数据输出。InLabel 是本次函数调用接收到的参数，不需要在 Variables 里另建同名成员变量。它虽然是函数的“输入”，但在函数内部会从入口节点输出给后面的节点使用。

蓝图函数创建和参数面板位置可对照 [Epic UE 5.5 Functions 文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/functions-in-unreal-engine?application_version=5.5)。

#### 4.2.2 添加修改文字的 SetText 节点

1. 在左侧 My Blueprint 的 Variables 中找到 **TXT_Prompt**，拖到 ShowTarget 图表，选择 **Get TXT_Prompt**。
2. 如果没有 TXT_Prompt，回 Designer 选中那块 Text Block，检查名称以及 **Is Variable**，Compile 后再回 Graph。使用 Designer 创建的控件变量，不要另建一个未赋值的同名 Text Block 引用。
3. 从 Get TXT_Prompt 的对象输出引脚拖线，搜索 **Set Text**，选择 **SetText (Text)**。这是 Text Block 的函数，Target 应自动接上 TXT_Prompt。

这个节点的含义是“修改这一块文字的内容”。它不是变量赋值节点 Set TXT_Prompt；后者修改的是控件引用。

#### 4.2.3 用 Format Text 组合提示语

1. 在图表空白处右键，搜索并添加 **Format Text**。
2. 在节点的 **Format** 文本框填写下面的内容，然后回车或点击空白处确认。花括号使用英文半角 `{` 和 `}`：

```text
可交互：{TargetName}
```

3. 确认后节点会出现名为 **TargetName** 的参数输入引脚。
4. 将 ShowTarget 入口的 **InLabel → Format Text 的 TargetName**。
5. 将 Format Text 的 **Result → SetText 的 In Text**（参数名 InText，界面通常显示 In Text）。

例如传入 InLabel=`Target_B`，Format Text 的 Result 就是 `可交互：Target_B`。TargetName 只是格式模板中的占位符，不需要另建变量，也不是在这里查找场景 Actor。

Format Text 只有数据引脚，没有白色执行引脚。执行 SetText 时，为了取得 In Text 的值，就会计算这段格式化文本；不要尝试给 Format Text 连接白色执行线。

#### 4.2.4 显示整个提示界面，接好执行线

1. 在空白处右键，搜索 **Self**，添加 **Get a reference to self / 获取自身引用** 节点。
2. 从 Self 的对象输出拖线，搜索 **Set Visibility**，添加 Widget 的 Set Visibility 节点，Target 接 Self。
3. 将 **In Visibility** 下拉框设为 **Not Hit-Testable (Self & All Children)**：整个提示可见，但它和子控件都不参与鼠标命中测试。
4. 接白色执行线：**ShowTarget 入口 → SetText → Set Visibility**。

这里 Self 是当前这个 **WBP_InteractionPrompt 实例**，所以能连同 Border 背景和 Text Block 一起显示。不要把这个 Set Visibility 的 Target 接到 TXT_Prompt，否则只改变文字子控件。

完成后逐条核对下表：

| 线的用途 | 从哪里接出 | 接到哪里 |
|---|---|---|
| 执行顺序 | ShowTarget 的白色执行输出 | SetText 的白色执行输入 |
| 执行顺序 | SetText 的白色执行输出 | Set Visibility 的白色执行输入 |
| 要修改的文字控件 | Get TXT_Prompt 的对象输出 | SetText.Target |
| 传入的目标名称 | ShowTarget.InLabel | Format Text.TargetName |
| 拼好的提示文字 | Format Text.Result | SetText.In Text |
| 要显示的整个界面 | Self 的对象输出 | Set Visibility.Target |

图表的简化示意如下；这是连线说明，不是要写入 C++ 的代码：

```text
执行线：
[ShowTarget 入口] ──→ [SetText] ──→ [Set Visibility]

数据线：
[Get TXT_Prompt] ────────────────→ SetText.Target
[入口的 InLabel] → [Format Text 的 TargetName 输入]
[Format Text 的 Result 输出] ───→ SetText.In Text
[Self] ─────────────────────────→ Set Visibility.Target

Set Visibility 的下拉值：Not Hit-Testable (Self & All Children)
```

#### 4.2.5 创建 HideTarget

1. 左侧 Functions 再点击 `+`，命名 **HideTarget**，双击进入该函数图表。
2. 不添加 Inputs 或 Outputs，Access Specifier 保持 Public，Pure 不勾选。
3. 和上一小节相同，创建 Self → Set Visibility，使 Target 指向 Self。
4. In Visibility 选择 **Collapsed**。
5. 把 HideTarget 入口的白色执行输出接到 Set Visibility 的执行输入。

```text
执行线：[HideTarget 入口] ──→ [Set Visibility]
数据线：[Self] ────────────→ Set Visibility.Target
下拉值：Collapsed
```

Collapsed 会隐藏这个 Widget，也不为它保留布局空间。这里不用清空文字，不用删除 Widget；下次 ShowTarget 会更新文字并重新显示。

#### 4.2.6 本节完成条件

- Functions 中有 ShowTarget 和 HideTarget 两个函数。
- ShowTarget 有一个 Text 输入 InLabel，执行线依次经过 SetText 和 Set Visibility。
- SetText 的 Target 是 TXT_Prompt；两个函数中 Set Visibility 的 Target 都是 Self。
- ShowTarget 使用 Not Hit-Testable (Self & All Children)，HideTarget 使用 Collapsed。
- 点击 **Compile / 编译** 无错误，然后 **Save / 保存**。

只完成本节时，Play 不一定出现提示：第 5 节尚需创建 Widget、AddToViewport 并调用这两个函数。函数不会仅因已经定义就自动执行，本节也不用把它们同时接到 Event Construct。

Widget 只接收文本和显示命令，不获取玩家 / Actor、不选择目标。事件驱动更新的依据见 [Epic UMG 优化指南（5.5）](https://dev.epicgames.com/documentation/en-us/unreal-engine/optimization-guidelines-for-umg-in-unreal-engine?application_version=5.5)。

## 5. 角色蓝图管理候选与 Widget

### 5.1 接口和变量

打开已有 BP_ThirdPersonCharacter，Class Settings → Implemented Interfaces → Add，加入 BPI_PracticeInteractionReceiver，编译。添加以下变量，不勾选 Instance Editable：

| 名称 | 类型 | 默认值 |
|---|---|---|
| Candidates | ETPracticeActor Object Reference 的 **Array** | 空 |
| CurrentTarget | ETPracticeActor Object Reference | None |
| PromptWidget | WBP_InteractionPrompt Object Reference | None |

### 5.2 函数 RefreshCandidate

新建无输入函数 RefreshCandidate，添加函数局部变量 **NewTarget**（ETPracticeActor Object Reference）。按顺序连线：

```text
函数入口
    → Set NewTarget = None（每次调用先清空）
    → ForEachLoop：Candidates
        Loop Body → IsValid(Array Element)
            Valid → Set NewTarget = Array Element
        Completed → Set CurrentTarget = NewTarget
            → IsValid(PromptWidget)
                Valid → IsValid(CurrentTarget)
                    Valid → PromptWidget.ShowTarget(CurrentTarget.GetPracticeLabel())
                    Not Valid → PromptWidget.HideTarget()
```

普通 ForEachLoop 即可，不要 Break。每遇到有效对象就覆盖 NewTarget，最终得到数组中最后一个有效对象。**Completed** 才更新 UI，不在循环体中反复刷新。数组末尾代表最后注册，不代表距离最近。

注意：HideTarget 接的是 **IsValid(CurrentTarget) 的 Is Not Valid**，此时 Widget 已经确认有效。不要接到 IsValid(PromptWidget) 的无效分支，否则没有目标时不会隐藏，还可能对无效 Widget 调用函数。此处之前已修复，不需要重复修改。

这里跳过无效 / 已销毁对象；正常退出时会删除对应元素。候选只有两个，不需要 Tick、距离排序或复杂容器。暂不加“新旧目标相同就返回”：候选可能早于 Widget 创建，创建后仍须同步一次 UI。

### 5.3 接口事件

在 Event Graph 实现两个无输出的接口事件：

```text
Event RegisterCandidate(Candidate)
    → IsValid(Candidate)
        Valid → Candidates.AddUnique(Candidate)
              → RefreshCandidate

Event UnregisterCandidate(Candidate)
    → Candidates.RemoveItem(Candidate)
    → RefreshCandidate
```

AddUnique 防重复。移除只用 RemoveItem，不 Clear 全数组，也不直接 HideTarget。**Unregister 不先做 Candidate 的 IsValid 检查**：即使目标进入销毁流程，也需要按引用把它移除。

### 5.4 创建和清理 Widget

```text
Event BeginPlay
    → Create Widget（Class=WBP_InteractionPrompt，Owning Player=Get Player Controller 0）
    → Set PromptWidget（接 Return Value）
    → PromptWidget.AddToViewport
    → PromptWidget.HideTarget
    → RefreshCandidate
```

这是单机 Player Controller 0。已有 BeginPlay 逻辑时用 Sequence 保留原链并追加初始化，不删模板输入逻辑。最后 RefreshCandidate 处理已经登记的候选。只创建一次 Widget，不放进 Overlap 或 Tick。

Event EndPlay：PromptWidget 有效时 RemoveFromParent；之后清空 PromptWidget / CurrentTarget 引用和 Candidates 数组。无有效 Widget 时也清空引用。保留原有清理逻辑。

本批不设置 InputMode、不显示鼠标光标，不启用 UI Only；提示无需改变输入归属。编译保存角色蓝图。

## 6. 物体报告范围事件

打开 BP_ETPracticeActor，保留 BeginPlay → PrintPracticeInfo。

### 6.1 Sphere

新增 Sphere Collision，命名 **InteractionRange**，挂在 SceneRoot 下，中心与物体对齐，Sphere Radius **220 cm**。两个 Actor 整体 Scale 暂保持 (1,1,1)，避免范围随整体缩放改变。

Collision Presets=Custom，Collision Enabled=Query Only，Object Type=WorldDynamic；所有通道 Ignore，再把 **Pawn=Overlap**；**Generate Overlap Events 开启**。Mesh 保持 NoCollision。

角色 CapsuleComponent 也开启 Generate Overlap Events，其他响应保留模板默认值。两端均需生成重叠事件，参见 [Epic OnComponentBeginOverlap（5.5）](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Components/UPrimitiveComponent/OnComponentBeginOverlap?application_version=5.5)。

### 6.2 BeginOverlap

新增变量 **OverlappingCharacter**，Character Object Reference，默认 None，用于物体销毁时通知接收方。本批只支持一个本地玩家。

选中 InteractionRange，在 Details → Events 添加 OnComponentBeginOverlap：

```text
OnComponentBeginOverlap
    → Cast OtherActor To Character
    → 比较 OtherComp == 该 Character.GetCapsuleComponent（Equal Object）
        True → Does Implement Interface（OtherActor，BPI_PracticeInteractionReceiver）
            True → Set OverlappingCharacter = 该 Character
                 → RegisterCandidate (Message)
                       Target = OtherActor
                       Candidate = Self
```

检查 OtherComp 是胶囊，避免角色 Mesh / 其他组件触发额外进出事件。接口调用选择带信封标识的 **Message**；Target 是接收方角色，Candidate 才是 Self。这里不依赖 GetPlayerCharacter 的 Possess 时序，只接受实现接口的 Character。

### 6.3 EndOverlap / EndPlay

添加 InteractionRange 的 OnComponentEndOverlap，使用与 BeginOverlap 相同的 Character / Capsule / Interface 过滤。通过后：

```text
UnregisterCandidate (Message)：Target=OtherActor，Candidate=Self
    → Set OverlappingCharacter = None
```

添加 Actor 的 Event EndPlay：OverlappingCharacter 有效时发送 UnregisterCandidate (Message)，Target=OverlappingCharacter，Candidate=Self，然后清空引用。物体销毁或离开关卡也有显式清理入口，不假设所有移除路径一定先触发 EndOverlap。

编译保存。物体不持有 Widget、不用 GetAllActorsOfClass 搜索角色、不用 Tick。

### 6.4 补修：出生已在范围内时，主动同步一次初始状态

**本节已由用户完成并复验通过，以下步骤保留供复盘。** 本次补修不需要改 C++、关闭编辑器或重新进行 C++ Build，只修改 BP_ETPracticeActor，Compile / Save 后 Play；保留已有离开隐藏、范围切换 / 回退和销毁清理逻辑。

#### 6.4.1 为什么补这一段

BeginOverlap 用于接收进入范围的通知；初始化时，还需要让候选数组反映“玩家现在已经在范围内”这个状态。原指南要求验收出生重叠，但没有明确安排一次初始范围同步，这里补齐。现有证据尚不能断言本次失败究竟发生在哪个事件时序环节，也不能据此认定 UE 不会触发初始 BeginOverlap。

做法是在物体 BeginPlay 后等待一个 Tick，再检查玩家胶囊是否与 InteractionRange 重叠；若重叠，就通过原有接口登记。等待仅执行一次，不启用 Event Tick，也不反复重试。

把这段放在**物体蓝图**，是因为除了登记候选，还要写入 **OverlappingCharacter**，这样出生时就在范围内的物体被销毁后，原来的 EndPlay 也能通知正确的角色。角色已有 AddUnique，正常 BeginOverlap 和本次同步即使都登记了同一个物体，数组也不会重复添加。

#### 6.4.2 创建函数，先接有效性检查

1. 打开 **BP_ETPracticeActor → Graph**，在 My Blueprint → Functions 点击 `+`，命名 **RegisterInitialOverlap**。
2. 本函数不添加 Inputs / Outputs，**Pure 不勾选**，保留白色执行引脚。
3. 空白处右键添加 **Get Player Character**，Player Index 保持 **0**。当前 M0 是单个本地玩家。
4. 添加带白色执行引脚、输出 **Is Valid / Is Not Valid** 的 **Is Valid** 节点。不要选只有布尔输出的纯函数版本。
5. 将函数入口的白色执行输出接到 Is Valid 的执行输入；Get Player Character 的 **Return Value → Is Valid.Input Object**。
6. Is Not Valid 暂不连接。若玩家尚不存在，本次函数安全退出；本节只覆盖当前模板的开局，不覆盖未来异步生成玩家或重生流程。

#### 6.4.3 检查球体和玩家胶囊是否已经重叠

1. 从左侧 Components 把 **InteractionRange** 拖入图表，得到它的 Get 引用。
2. 从这个引用输出拖线，搜索 **Is Overlapping Component**，创建节点。它的 **Target 必须接 InteractionRange**。
3. 从 Get Player Character 的 Return Value 拖线，搜索 **Get Capsule Component**，创建节点。它的 Target 是这个玩家角色。
4. 将 Get Capsule Component 的胶囊输出接到 **Is Overlapping Component.Other Comp**。
5. 添加 **Branch**，将 Is Valid 的 **Is Valid 执行输出 → Branch 执行输入**。
6. 将 Is Overlapping Component 的 **Return Value → Branch.Condition**。Branch 的 False 不连接；玩家不在这个物体范围内，就不登记。

这一步检查的是 InteractionRange 与玩家胶囊，和现有 BeginOverlap 的胶囊过滤一致。不要用 Mesh，也不要换成对整个 Actor 的模糊判断。节点含义可见 [Epic：Is Overlapping Component](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Collision/IsOverlappingComponent)；相关签名已同时核对本机 UE 5.5.4 的 PrimitiveComponent.h。

#### 6.4.4 保存接收者，再发送接口消息

1. 空白处右键，搜索 **Does Object Implement Interface**。Test Object 接 Get Player Character 的 Return Value；Interface 下拉选择 **BPI_PracticeInteractionReceiver**。
2. 添加第二个 Branch：第一个 Branch 的 **True → 第二个 Branch 的执行输入**；Does Object Implement Interface 的布尔输出接第二个 Branch.Condition。这个接口检查节点没有白色执行引脚，执行顺序通过 Branch 控制。
3. 从 Variables 把现有 **OverlappingCharacter** 拖入图表，选择 **Set**。第二个 Branch 的 **True → Set OverlappingCharacter 执行输入**；它的值接 **Get Player Character.Return Value**。
4. 从 Get Player Character 的 Return Value 拖线，搜索 **Register Candidate**，选择带信封图标的 **Register Candidate (Message)**。
5. **Set OverlappingCharacter 的执行输出 → Register Candidate (Message) 的执行输入**。
6. 消息节点的 **Target = Get Player Character.Return Value**，**Candidate = Self**。添加 Self 引用后把它接入 Candidate；Self 是这个练习物体，不是角色。
7. 第二个 Branch 的 False 不连接。不调用 HideTarget、不清空候选数组：另一个物体可能仍然有效。

完成后，函数的白线应只有这一条主链：

```text
RegisterInitialOverlap 入口
  → Is Valid（Input Object = 玩家角色）
      Is Valid → Branch（Condition = 球体与玩家胶囊已重叠）
          True → Branch（Condition = 玩家实现接收接口）
              True → Set OverlappingCharacter = 玩家角色
                   → Register Candidate (Message)
                       Target    = 玩家角色
                       Candidate = Self（练习物体）
```

关键数据线核对：

| 节点输入 | 连接来源 |
|---|---|
| Is Overlapping Component.Target | InteractionRange |
| Is Overlapping Component.Other Comp | 玩家角色的 Get Capsule Component |
| Does Object Implement Interface.Test Object | Get Player Character.Return Value |
| Set OverlappingCharacter 的值 | Get Player Character.Return Value |
| Register Candidate (Message).Target | Get Player Character.Return Value |
| Register Candidate (Message).Candidate | Self |

#### 6.4.5 从 Event Graph 调用一次

1. 回到 **BP_ETPracticeActor → Event Graph**，找到已有 Event BeginPlay；不要再创建第二个 BeginPlay。
2. 保留 PrintPracticeInfo 以及其他必要的现有开局逻辑，在这条开局执行链末尾添加 **Delay Until Next Tick**。
3. 从 My Blueprint 的 Functions 把 **RegisterInitialOverlap** 拖入 Event Graph，作为函数调用节点，Target 为 Self。
4. **Delay Until Next Tick 的 Completed → RegisterInitialOverlap 的执行输入**。

```text
Event BeginPlay
  → PrintPracticeInfo（以及已有必要的开局逻辑）
  → Delay Until Next Tick
  → RegisterInitialOverlap（Target = Self）
```

**Delay Until Next Tick 必须放在 Event Graph，不能放进普通函数 RegisterInitialOverlap。** 本机 5.5.4 的 KismetSystemLibrary.h 已确认存在这个延迟一个 Tick 的潜在执行节点。无需填写秒数，不使用循环 Timer 或 Event Tick。若搜索不到，先确认你已回到 Event Graph，再搜索英文完整名称。

如果图里还留有销毁测试的 **Set Life Span**，先把 In Lifespan 改为 **0**，取消这项测试计时，避免物体几秒后自动消失干扰出生测试；没有这个节点就跳过，不用新增。保留原有 BeginOverlap / EndOverlap / EndPlay，以及角色创建 Widget 后的 RefreshCandidate。

最后 Compile 并 Save BP_ETPracticeActor，保存测试关卡。

#### 6.4.6 复验与反馈

先退出 Play。在 Play 下拉选项确认 **Spawn Player At = Default Player Start**，不是 Current Camera Location；使用正常 Play，不用 Simulate。记录 PlayerStart 原位置，测试结束后恢复。

| 测试 | 操作与预期 |
|---|---|
| 出生只在 A 内 | PlayerStart 放在 A 范围明显靠内、且 B 范围外；避开方块本体，确保实际出生胶囊在范围内。Play 后先不按移动键，开局应显示 Target_Changed |
| 出生只在 B 内 | 同样方式只放进 B，静止开局应显示 Target_B |
| 出生在两者之外 | 不移动时无提示，之后走进 / 离开仍能正常显示 / 隐藏 |
| 出生在两者之内 | 开局显示任一有效目标均可；退出当前目标但留在另一范围，应回退；离开全部范围，应隐藏 |
| 出生重叠后目标销毁 | 可复用原销毁测试，临时 Set Life Span = 30 秒；开局就在范围内，销毁后提示回退或隐藏。测试后移除该临时节点或改回 0，编译保存 |

前三项每次重新开始 Play；测试时不要靠先走出再走进来取得“初始显示”的通过结果。最后确认正常移动 / 转向 / 跳跃，并恢复测试位置和临时寿命设置。

本次补修**已由用户实测通过**。若以后出现回归，不要反复增加 Delay 秒数；区分“函数未执行”“玩家无效”“胶囊实际未重叠”“注册后 UI 未刷新”再定位，必要时由我接管编辑器。本次通过不等于证明初始 BeginOverlap 从未触发，也不代替未来重生 / 异步生成流程的验证。

```text
M0-04A 第 6.4 节：成功 / 失败
蓝图 Compile：通过 / 错误原文
出生 A / 出生 B / 出生范围外：各自结果
出生双范围及切换 / 离开隐藏：结果
出生重叠后销毁清理：通过 / 未测 / 实际结果
移动 / 转向 / 跳跃：正常 / 异常
已恢复临时测试设置并保存：是 / 否
实际用时：约 __ 分钟
```

失败时优先发 **RegisterInitialOverlap 的完整函数连线截图**和 **BeginPlay 新增链路截图**；Output Log 如有错误，复制首条错误及上下文文字。再写清出生在哪个范围、静止是否显示、走出再走进后是否恢复。日志文字和蓝图截图各自承担不同信息，不需要先自行推断根因。

## 7. 手动验收

确保接口和三个蓝图编译通过，保存关卡。A 的现有标签 Target_Changed 保留即可；两个物体中心相距约 300 cm，放在方便绕行的地方，以制造范围重叠。

| 检查 | 预期 |
|---|---|
| 出生在范围外 | 无提示 |
| 只进入 A，再离开 | 显示 Target_Changed，离开隐藏 |
| 只进入 B，再离开 | 显示 Target_B，离开隐藏 |
| 从 A 进入 B，仍在两个范围内 | 切为 B |
| 当前 B，离开 A 但留在 B | 仍显示 B |
| 重新从 A 进入 B，再退出 B 但留在 A | 回退显示 A |
| 离开所有范围 | 提示隐藏 |
| 临时把 PlayerStart 放进 A 范围，再 Play | 显示 A；验证后恢复 PlayerStart 位置 |
| 连续三次开始 / 停止 Play | 每次只有一个提示，无残留或 Accessed None |
| 有提示和无提示时操作 | 移动、转向、跳跃正常 |

重叠范围的两条退出路线可分两次 Play 测试。在编辑状态选中 InteractionRange 可查看球体边界。

额外销毁测试：可临时在 BP_ETPracticeActor 的 PrintPracticeInfo 后接 **Set Life Span(Self, 30.0)**，Play 后走进范围并等待物体销毁，提示应回退或隐藏且无 Accessed None。完成后移除临时节点，恢复原执行线、编译保存，并确认物体正常保留。此项没做就写“未测”，不标为通过。

## 8. 报错方式

| 现象 | 提供的材料 / 重点检查 |
|---|---|
| Build 失败 | 首个错误和上下文文字；我直接读 h / cpp，不必抄整份代码 |
| 找不到 Getter | 正常 Build / 重启后搜 Get Practice Label，确认 public / UFUNCTION |
| 没有 Overlap | 两端 GenerateOverlapEvents、Pawn 响应、胶囊过滤；给组件 Details 和相关节点截图 |
| 有事件但无提示 | 接口是否在角色实现，Message 的 Target / Candidate、PromptWidget 创建链；给截图 |
| 离开 A 导致 B 提示消失 | 是否误用 Clear 或直接 Hide，是否接错 ForEachLoop Completed |
| 已离开但还显示 | EndOverlap 的胶囊过滤、RemoveItem 输入，以及 HideTarget 是否接在 CurrentTarget 无效分支 |
| 出生已在范围内但无提示 | 按第 6.4 节检查一次性初始同步；发新函数与 BeginPlay 链截图，并说明实际出生位置 |
| Accessed None | Output Log 错误及调用链文字，加相关节点截图 |
| 挡操作或提示叠加 | 是否改变 InputMode、重复 Create Widget、Visibility 选错 |

遵循 [IssueTemplate.md](IssueTemplate.md)：**日志用文字，节点 / 布局用截图，闪烁 / 时序用短视频**。写“卡在哪一节、做了什么、预期与实际”即可，无须先诊断根因。简单节点问题由你按说明修正，复杂问题我直接接手。

## 9. 回复与后续

```text
M0-04A 完成 / 卡在第 __ 节
C++ Build：成功 / 首个错误
蓝图编译：通过 / 具体错误
A / B 靠近显示、离开隐藏：通过 / 实际结果
范围重叠切换与回退：通过 / 实际结果
出生已在范围内：通过 / 实际结果
连续三次 Play：通过 / 实际结果
目标销毁清理：通过 / 未测 / 实际结果
移动 / 转向 / 跳跃：正常 / 具体异常
蓝图和场景：已保存
实际用时：约 __ 分钟
```

有一张实际 HUD 截图会方便核对布局。A 现已通过，下一批按 [M0-04B：按键交互与结果刷新](M0_04B_Interaction.md) 操作；完整 M0-04 的暂停菜单等要求仍保留在 C，不算本批已完成。
