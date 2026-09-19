# M0-04A：靠近显示交互提示

日期：2026-09-19。引擎：UE 5.5.4。状态：**待用户实现，尚未编译或运行验收**。

前置：M0-03 已通过。本批预计 60–120 分钟，以实际记录为准。基础代码和蓝图由你手动完成；复杂问题我读取工程接手。本文只有任务和参考代码，没有代改 Source 或蓝图资产。

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

**ShowTarget**：输入 InLabel，类型 Text（FText）。

```text
ShowTarget(InLabel)
    → TXT_Prompt.SetText
    → Self.SetVisibility：Not Hit-Testable (Self & All Children)

SetText 的 Text 输入：
Format Text，格式“可交互：{TargetName}”，TargetName 接 InLabel
```

Format Text 是数据节点，没有执行线；白色执行线连接 SetText → SetVisibility。设置非命中测试覆盖 Self & All Children，防止提示挡鼠标。

**HideTarget**：无输入，调用 Self.SetVisibility，选择 **Collapsed**。

编译保存。Widget 不获取玩家 / Actor，不选择目标。事件驱动更新的依据见 [Epic UMG 优化指南（5.5）](https://dev.epicgames.com/documentation/en-us/unreal-engine/optimization-guidelines-for-umg-in-unreal-engine?application_version=5.5)。

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
| 已离开但还显示 | EndOverlap 的胶囊过滤和 RemoveItem 输入 |
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

有一张实际 HUD 截图会方便核对布局。通过 A 后再下发 B 的按键交互；完整 M0-04 的暂停菜单等要求仍保留在 C，不算本批已完成。当前无需提前实现后续系统。
