# M1-06D：暂停迁移与菜单流程闭环

2026-10-18清理同步：保留历史接入、Designer、排障与分层验收经验；完整类以当前Source入口为准，不覆盖已完成工程。本轮清理由助手按用户授权直接修改，Git仍由用户操作。M1阶段包与后续功能已完成，当前状态见[Progress](Progress.md)，本批检查与提交清单见[Cleanup](Cleanup.md)。

下发：2026-10-12；验收更新：2026-10-13；UE **5.5.4**。**本批功能已验收通过，无待补项**，见[D报告](Reports/M1_06D_Verification.md)与[M1-06总验收](Reports/M1_06_Verification.md)。实际两新 / 六旧Source、69Source / 34cpp / 来源清单46，四旧资产已调整；双Build / 索引与三组实机反馈核对。用户13:01完成，手动105分钟（代码35 / 资产60 / 检查10）、等待5；累计1836分钟（30小时36分钟）/ 等待140。下文保留实施指南；下一步[M1-07A阶段包](M1_07A_StagePackaging.md)，当时尚未生成新包，后续阶段包已完成；当前独立包版本见Progress。

原D的C++、Blueprint、Build和检查由你手动完成；当前清理按本轮授权由助手直接修改并验证。普通功能开发保留原协作约定，反馈优先给完整文字日志与所在步骤。按[方案B](AcceptanceWorkflow_Options.md)复用正式按钮、P、既有4损伤入口与检查点，只查本批影响的边界，不搭临时Controller测试图。

## 当前暂停 / 失败呈现契约

[Flow声明](../EmberTrial/Source/EmberTrialUI/Public/Flow/ETFailureFlowComponent.h) / [实现](../EmberTrial/Source/EmberTrialUI/Private/Flow/ETFailureFlowComponent.cpp)仍是Controller上的单个协调组件，保留历史类名与蓝图配置。两个Widget只呈现状态并发意图；按钮名、中文、DisableNativeTick、P释放与背景只聚焦均保持。

bPresentationQueued、bCommandPending、bOwnsPause分别表示延迟呈现、会话命令和暂停所有权，不能因都像busy而删除。普通死亡沿NextTick呈现；已由本组件暂停时立即换失败页，保留暂停所有权，不等待停止推进的Timer。外部已经暂停时不擅自取得所有权。

层级150 / 200改为内部命名常量，数值保持。恢复输入只释放自己拥有的暂停，保留GameOnly首击、Flush与视口焦点；EndPlay先封口、清Timer和解绑，不再恢复结束中世界的输入。显式None地图仍由原预检拒绝，不改会话 / 检查点 / 资源规则。本批静态等价与资产Valid不等同于重跑键鼠焦点或切图实机矩阵。

## 1. 本批交付与实施顺序

本批把旧角色蓝图中的暂停Widget创建、暂停 / 输入模式、继续事件与结束清理迁到**现有Controller的单个FailureFlow组件**，并给暂停页添加“返回主菜单”。保留原生类名`UETFailureFlowComponent`和BP组件名`FailureFlow`，让已保存的失败页 / 地图配置继续有效；它现在负责游戏内暂停 / 失败这两种互斥界面的呈现与命令。源文件注释和职责表明确这个历史命名，暂不为命名引入CoreRedirect / 组件迁移。HUD仍归UIBootstrap，会话与检查点仍归RunSession，不把它们塞进菜单组件。

```text
游戏状态：Controller松开P → 单个FailureFlow → Pause Widget
Pause：鼠标继续 / Enter / Space / 松开P → 原世界继续
Pause：返回主菜单 → RunSession → Main Menu
Main Menu：继续本次试炼 → 已确认检查点 → 新世界 / 新身体100 / 新HUD
玩家死亡：Life.DeathSealed → 同一FailureFlow → Failure Widget
Failure：重试 / 返回菜单 → 既有RunSession；P不能叠开Pause
```

时间预算**80–140分钟手动投入**：代码30–55、旧蓝图整理与Designer35–60、三组相关检查10–15、记录约5–10；构建 / 索引等待另记。原路线60–100是粗估，本批将旧图清理逐项列出后采用更保守预算，可分两次完成。美术费用0元，复用已有中文字体链 / 全屏背景 / BTN_Resume，不接入Blender或手柄。

依次完成：保存并退出PIE → 手动写第3–6节代码 → 关闭UE正常双目标Build / 刷新索引 → 第8节清旧图和配置资产（中间不Play）→ 三组检查 → 提交反馈 → 按最后清单Stage / Commit / Push。本批编译新增反射类和属性时关闭UE，不依赖Live Coding；配置蓝图时再打开UE。

## 2. 文件、归属与关键规则

所有路径以`E:\UE_Project\Demo`为根。表中“替换”只替换指定文件全文，**不删除类或旧文件**。

| 操作 | 仓库相对路径 | 职责 |
| --- | --- | --- |
| 新建 | `EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h` | 暂停布局契约与继续 / 菜单意图 |
| 新建 | `EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp` | 原生按钮、P松开、背景焦点 |
| 替换 | `EmberTrial/Source/EmberTrialUI/EmberTrialUI.Build.cs` | 显式InputCore / SlateCore依赖 |
| 替换 | `EmberTrial/Source/EmberTrial/Public/Framework/ETPlayerController.h` | 核心输入意图、Blueprint呈现通知 |
| 替换 | `EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp` | P释放绑定；保留既有输入初始化 / 日志 |
| 替换 | `EmberTrial/Source/EmberTrialUI/Public/Flow/ETFailureFlowComponent.h` | 单个Controller菜单协调器 |
| 替换 | `EmberTrial/Source/EmberTrialUI/Private/Flow/ETFailureFlowComponent.cpp` | 互斥界面、暂停 / 输入、会话请求与清理 |
| 替换 | `EmberTrial/Source/EmberTrialUI/Private/Widgets/ETFailureWidget.cpp` | 修正C后已过期的“入口”文案 |

两个新文件手动创建到上述UI模块目录，不从UE默认向核心模块创建UserWidget类。`.generated.h`保持最后一条include；反射回调参数保持`CurrentPawn / SourceLife`，不要改成Controller继承作用域中已有的`Character`。本批不改Core Build.cs、Target、.uproject、GAS / Combat / AI / CheckpointStart / RunSession / GameMode / Presenter / UIBootstrap。

| 规则 | 实现与边界 |
| --- | --- |
| 单个菜单所有者 | 同一FailureFlow持有Pause / Failure；Pawn不再持有菜单，不再切换输入模式；MainMenu有独立Controller |
| 低耦合 | Core Controller只广播无UMG类型的原生意图；UI模块依赖Core；Widget只发意图；会话处理地图 / 稳定ID |
| P在两个输入环境 | Game Only由Controller在`IE_Released`请求打开；UI Only由Widget的`NativeOnKeyUp`继续；不加Esc |
| 暂停不中断战斗动作 | Pause只暂停世界并StopJumping，不取消能力、位移或Trace；Resume沿同一世界继续；死亡沿既有封锁顺序 |
| 输入恢复 | UI进入Flush；离开明确Game Only、隐藏鼠标、Flush、SetFocusToGameViewport；按钮Down And Up |
| 提示同步 | Core Controller的Blueprint事件仅桥接当前Pawn.RefreshCandidate；该函数读取真实IsGamePaused，无重复缓存 |
| 失败优先 | 死亡 / 排队失败拒绝新暂停；已有Pause收到DeathSealed则直接换Failure，保持暂停所有权，避免暂停世界等待NextTick |
| 切图请求 | 按钮pending拒绝重复；调用已验收RunSession，成功切图由旧对象EndPlay清理；预检拒绝恢复原菜单可操作状态 |
| 结束清理 | 解绑输入意图、Pawn / Life / Session事件、取消一次性计时、移除Widget；结束世界不恢复输入 / 焦点 / 暂停 |
| 性能 | Widget / 组件无常驻Tick、无属性Binding、无Delay；按需创建一个页面，关闭解绑并释放引用；无大世界 / 多人扩展 |

`InputComponent::BindKey(FKey, EInputEvent, Object, Function)`、EnhancedPlayerInput调用原输入栈、UserWidget的两个Native输入回调、FReply.SetUserFocus、UMG的Flush / Viewport Focus均按本机5.5源码核对。明确列模块依赖，避免因Unity / 间接include碰巧构建成功。**这属于静态核对，不等于本批Build或PIE通过。**

## 3. 手动添加原生暂停Widget

### 3.1 新建ETPauseWidget.h

保存完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETPauseWidget.h`，内容如下。保留旧按钮名`BTN_Resume`，使Designer可直接复用；两个新委托故意叫Intent，避免与旧Blueprint的OnResumeRequested重名。

当前源码：[ETPauseWidget.h](../EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h)。


### 3.2 新建ETPauseWidget.cpp

保存完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETPauseWidget.cpp`。背景点击只聚焦继续，不触发继续；按钮自身处理点击，不会被背景处理截断。

当前源码：[ETPauseWidget.cpp](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp)。


### 3.3 替换UI Build.cs

完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\EmberTrialUI.Build.cs`。`InputCore`提供按键定义，`SlateCore`提供输入事件 / FReply；Core模块已有InputCore无需重复修改。

```csharp
using UnrealBuildTool;

public class EmberTrialUI : ModuleRules
{
    public EmberTrialUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EmberTrial",
            "GameplayAbilities",
            "InputCore",
            "SlateCore"
        });
    }
}
```

## 4. Controller只发送暂停意图

### 4.1 替换ETPlayerController.h

完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\ETPlayerController.h`。OnGameMenuStateChanged为无参数Blueprint事件，后面只连当前角色的RefreshCandidate。

当前源码：[ETPlayerController.h](../EmberTrial/Source/EmberTrial/Public/Framework/ETPlayerController.h)。


### 4.2 替换ETPlayerController.cpp

完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETPlayerController.cpp`。仅新增input include、SetupInputComponent与释放回调；原BeginPlay / Possess / EndPlay实现保留。Controller不CreateWidget、不依赖UI模块，不持有角色菜单变量。

当前源码：[ETPlayerController.cpp](../EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp)。


## 5. 现有FailureFlow统一暂停与失败页

### 5.1 替换ETFailureFlowComponent.h

完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Flow\ETFailureFlowComponent.h`。既有FailureWidgetClass / MainMenuMap属性名保持，新增PauseWidgetClass；仍只有原来的一个组件。

当前源码：[ETFailureFlowComponent.h](../EmberTrial/Source/EmberTrialUI/Public/Flow/ETFailureFlowComponent.h)。


### 5.2 替换ETFailureFlowComponent.cpp

完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Flow\ETFailureFlowComponent.cpp`。全文替换，避免同名旧函数留在文件底部。`bOwnsPause`只在本组件成功取得暂停时记录，正常Resume恢复同一世界；切图交给Session，EndPlay只清理对象。新前缀统一为`[M1-06D]`，旧B日志前缀改变不表示B业务被删。

当前源码：[ETFailureFlowComponent.cpp](../EmberTrial/Source/EmberTrialUI/Private/Flow/ETFailureFlowComponent.cpp)。


## 6. 修正失败页检查点文案

完整路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETFailureWidget.cpp`。行为与B保持，仅ShowReady的一条文字改变；以下全文用于避免你查找行号。C已支持检查点，不能继续向用户承诺始终从入口重试。

当前源码：[ETFailureWidget.cpp](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETFailureWidget.cpp)。


## 7. 正常Build与CLion索引

1. 确认以上八文件保存，两份新增文件实际位于UI Public / Private的Widgets目录，无重复副本。
2. 保存UE资产并关闭UE编辑器。CLion可保持打开，关闭Live Coding构建流程。
3. PowerShell先执行既有脚本，完成Editor Build与两个模块的CodeModel刷新；这个脚本**不执行Game Build**：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

4. 脚本成功后，在CLion重新加载主工程的CMake，确认两个CodeModel / 源码目标刷新；实际cpp预计34。再保持UE关闭，在同一PowerShell单独执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

5. 两次Build都成功才打开UE；任何一步失败先停止，不把`exit code 6`当根因：复制前面的第一条UHT / C++错误及相邻10–20行给我，写明第几节 / 哪文件。不是选CodeModel对象目标代替真正UBT构建。
6. 用UE5.5打开原`EmberTrial.uproject`，进入正式地图。**先完成下面所有迁移，再Play；编译成功并不代表旧BP暂停已移走。**

## 8. 逐项迁移旧蓝图并配置暂停页

资产均是既有文件，不创建第二份暂停菜单，不改目录大小写。以下中间态可以Compile检查，但直到8.5完成前不要Play；删除旧变量前先改其引用，避免一屏未知变量错误。

### 8.1 BP_ThirdPersonCharacter先替换所有旧暂停条件

打开`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`。先备查现有OpenPauseMenu / ClosePauseMenu / EV_ResumeRequested、PauseWidget / bPauseMenuOpen，留在图里直到8.2删除。按下面三组替换数据条件，不重写候选、伤害或动作门控。除了交互函数，M1-04保留的数字3 / 4门控与旧C1函数也引用这个变量，必须一起处理。

**A. RefreshCandidate：**

1. My Blueprint → Functions双击`RefreshCandidate`。
2. 找到PromptWidget有效后、原`bPauseMenuOpen`作为Condition的Branch。原True通往`HideTarget`，False通往CurrentTarget有效性与原提示显示；保持这两条白线。
3. 空白处右键添加`Is Game Paused`（Gameplay Statics，纯bool节点），把Return Value接该Branch.Condition，替换Get bPauseMenuOpen的数据线。
4. 只删除失去用途的Get bPauseMenuOpen节点。当前候选选择、PromptWidget / CurrentTarget的Is Valid、文本与接口节点全部保留。

```text
原候选更新 → Is Valid(PromptWidget) → Branch(Is Game Paused)
    True  → HideTarget(Target = PromptWidget)
    False → 原CurrentTarget检查 → 原显示 / 隐藏提示
```

**B. TryInteractWithCurrentTarget：**

1. 双击`TryInteractWithCurrentTarget`，找原入口`bPauseMenuOpen OR Is Game Paused`的Branch。
2. 取已有`Is Game Paused.Return Value`，直接接同一Branch.Condition，删除那一个Boolean OR与Get bPauseMenuOpen。
3. 保持True→Return（拒绝），False→原后续链路。后面PracticeAction Idle、正式CombatLife Dead / 能力状态、目标有效性等门控全部保留。
4. 这两个函数已不再依赖旧菜单bool。不要交换Branch的True / False白线，继续下面C再Compile。

**C. 数字3 / 4与保留的旧函数：**

1. Event Graph找到`IA_PracticeHit.Started`（数字3）后面的暂停Branch。把已有`Is Game Paused.Return Value`直接接该Branch.Condition，删该处Boolean OR与Get bPauseMenuOpen。
2. 白线保持**Started → 暂停Branch；True不接；False → Request Hit React(Target=Self)**。不补FormalIdle检查，不接回旧Practice受击路线。
3. 找到顶部`Keyboard 4.Pressed`后面的暂停Branch，同样把`Is Game Paused.Return Value`直接接Condition，删该处OR / Get bPauseMenuOpen。
4. 白线保持**Pressed → 暂停Branch；True不接；False → 原ASC Is Valid / Ready → 原Make Effect Context / Apply Gameplay Effect to Self**。保留Input Object=玩家ASC、Apply.Target=同一ASC、Damage25、Dead保护、Execute When Paused=False；只换暂停Condition。
5. My Blueprint若仍保留旧`TryPlayPracticeMontage`（C1 Legacy - Disconnected），打开它，将入口`bPauseMenuOpen OR Is Game Paused`也改为**Is Game Paused单独接原Branch.Condition**。True→原Return，False→原Montage有效性 / 地面 / 速度 / 忙碌检查，保留已断开的调用，绝不接回1 / 2 / 3输入。
6. 对变量`bPauseMenuOpen`右键Find References逐个看剩余Get。属于OpenPauseMenu / ClosePauseMenu / 旧菜单清理的引用留给8.2整段删除；其他仍保留功能中的暂停查询，也按**Is Game Paused直接接原Condition，保持True / False白线**替换，删该处Get / OR。旧函数虽然不执行，仍会参与Blueprint编译，不能留未知变量节点。
7. **必须再检查左键事件`IA_ET_LightAttack.Started`和数字1事件`IA_PracticeAttack.Started`。** 两者各自后面的暂停Branch，Condition都要接`Is Game Paused.Return Value`；白线保持Started→Branch、True留空、False→`Request Light Attack(Target=Self)`。删除旧OR后不能让Condition悬空并保留默认True，否则Compile仍成功，但该入口永远不攻击。数字2如有同类暂停Branch也同样换Condition，保留其原动作调用。Compile后进入8.2，不新增验收专用图。

**2026-10-13窄排查记录：**用户反馈左键失效、数字1仍有动画与伤害。助手通过computer-use只读核对：IMC_Default的IA_ET_LightAttack仍映射Left Mouse Button，Mapping Triggers / Modifiers均为空；角色蓝图该事件Started与Branch白线存在，False也接Request Light Attack，但Condition红色输入未连接、默认True已勾选。这是左键门控断线，此前8.2检查漏查此项。随后用户按本步骤接回Is Game Paused并于13:01:09.879正常保存；最终实机反馈通过。本轮助手只读打开资产，确认该红色数据线已接Condition、False仍接Request Light Attack，Compile绿色 / All Saved；13:01:12有正式攻击Begin、停止PIE时取消 / 清标签。该补项已关闭，不需重做整套暂停检查。

### 8.2 BP_ThirdPersonCharacter删除旧菜单业务

仍在同一角色蓝图，用Graph的查找（Ctrl+F）定位节点；对变量 / 函数右键`Find References`辅助核对。只移走下表菜单部分，**既有交互提示、候选、Overlap、PracticeAction、能力输入和开发损伤4保留**。

| 位置 | 删除内容 | 接回方式 |
| --- | --- | --- |
| Event BeginPlay尾部 / Sequence某支 | Create Widget(WBP_PauseMenu) → Set PauseWidget → Is Valid → Bind Event to OnResumeRequested和该失败Print | 原交互提示初始化 / HideTarget / RefreshCandidate链路是独立保留部分；若这些菜单节点插在原链中间，将菜单前执行输出直接接菜单后原非菜单节点；若本来就是末尾，断开即可 |
| Event Graph | `EV_ResumeRequested`与其Print / ClosePauseMenu调用 | 删整条旧菜单自定义事件链，原生组件负责订阅 |
| Event Graph | 旧`IA_Pause`事件及Completed → OpenPauseMenu（B的Dead拒绝如在这条入口也一并删） | 删除这条独立旧输入链，不动其他IA |
| Functions | `OpenPauseMenu`、`ClosePauseMenu` | 先确保上面旧调用已删，再在My Blueprint右键两个函数Delete；函数中的死亡Branch随旧函数一起移走，由新组件原生判断 |
| Event EndPlay | PauseWidget有效检查 → Unbind All OnResumeRequested → RemoveFromParent，以及清PauseWidget / bPauseMenuOpen部分 | Sequence只断开菜单相关Then支；PromptWidget解绑 / 清候选与其他结束业务的Then支保留，可留空支，不删整个Event EndPlay或Sequence |
| Variables | `PauseWidget`、`bPauseMenuOpen` | 对两变量Find References，应没有活节点引用后再Delete；不删PromptWidget / CurrentTarget / 候选数组 |

具体EndPlay定位：Graph空白Ctrl+F搜`Event EndPlay`；若已存在就在查找结果双击，右上放大到看清Sequence。若没有该事件，搜`PauseWidget` / `Unbind All Events from OnResumeRequested`定位旧清理。**无需为了这次迁移新建EndPlay。**删除菜单变量前必须排除仍连着的Get / Set，否则不要直接Play；出现未知变量时先回本表找到遗漏的菜单支。

完成后Compile / Save。查找旧OpenPauseMenu、ClosePauseMenu、EV_ResumeRequested、bPauseMenuOpen应无活业务节点；角色BeginPlay不得再CreateWidget(WBP_PauseMenu)。Graph搜索`Set Game Paused`、`Set Input Mode UI Only`、`Set Input Mode Game Only`的旧暂停调用应已移除；角色创建交互提示Widget仍是正式玩法，保留。

**本地操作记录（2026-10-12）：第8.2节已由助手通过Computer Use完成。** 已清理旧暂停页创建 / 继续事件 / IA_Pause输入链、OpenPauseMenu / ClosePauseMenu、EndPlay暂停清理分支，以及无剩余引用的PauseWidget / bPauseMenuOpen。保留交互提示创建、候选注册 / 注销、交互与动作输入、数字4损伤入口；EndPlay的PromptWidget移除、PromptWidget / CurrentTarget清空及Candidates清理保持连接，原Sequence菜单Then支留空。

角色蓝图Compile结果为`successful! [in 356 ms]`，Compiler Results无错误 / Warning；工具栏Save后显示All Saved，磁盘正常保存时间为14:21。暂停关键词查找仅剩六处Is Game Paused查询（EventGraph三处、RefreshCandidate / TryInteractWithCurrentTarget / 保留的TryPlayPracticeMontage各一处）。本记录只确认第8.2节的编辑、编译与保存；**接着做8.3–8.5，完成8.5前不Play，不代表整章运行验收已通过。** 操作前资产备份位于`Docs/Reports/Raw/M1_06D_8_2_Before/`，属于本地恢复材料，无需Push。

### 8.3 IMC_Default移除旧P映射行

打开`/Game/ThirdPerson/Input/IMC_Default`，展开Mappings，找到**Input Action=IA_Pause、Key=P**的那一行，删除该行并Save。若同一Action下有多键，只删P键对应项，不删其他Action。鼠标左键正式攻击、1 / 2 / 3 / 4、E、移动 / 跳跃映射保持。

`IA_Pause`资产本身保留为M0过程资源，不删除磁盘文件或更改其他测试资产；当前正式P由Controller BindKey接收。不是给新组件再加一份Enhanced Input P映射，也不是去项目Input Settings另绑P。

### 8.4 原WBP_PauseMenu清Graph、改Parent、补布局

打开实际资产`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`，本节操作对象始终是**这个Widget**。先清旧Graph，再Reparent，避免重复响应。

**A. 清旧Blueprint菜单代码（Designer保留）：**

1. Graph删`OnClicked(BTN_Resume) → Call OnResumeRequested`旧链；若旧Event Construct / Initialized额外连了相同继续绑定，只删相关链。
2. My Blueprint → Functions删旧`FocusResumeButton`，以及Override的`OnKeyUp`、`OnMouseButtonDown`整张旧函数图；右键对应条目Delete，删的是Widget的蓝图Override，不删按钮或根背景。
3. Event Dispatchers删旧`OnResumeRequested`。8.2已删角色绑定，所以这里不再有有效依赖。
4. 这个Widget的Event Graph不再有继续业务、Set Game Paused、Set Input Mode、Cast角色、CreateWidget或OpenLevel；只有布局，原生代码接输入 / 意图。若查找仍有上述活链，先清对应旧菜单代码。
5. File → Reparent Blueprint（也可Class Settings → Parent Class选择）改为新原生**ETPauseWidget**，不是ETFailureWidget；Compile一次。此时MainMenuButton / StatusText还未补时出现required widget binding诊断是中间状态，继续B配置后消除再Save。

**B. Designer逐个确认名称、层级和属性：**

先点右上角**Designer**。后续从左侧**Hierarchy**选控件；右侧Details最上方输入框是**控件名称**，`Content → Text`是**画面显示的文字**，二者分别设置。名称区分大小写，改名可在Hierarchy选中后按F2，输入名称并按Enter确认。找不到属性时，先清空Details搜索框，避免上一项筛选仍在。

| 必须绑定的控件名称 | 类型 | Hierarchy中的位置 | Is Variable |
| --- | --- | --- | --- |
| `BTN_Resume` | Button | Vertical Box的直接子项，原有继续按钮 | True |
| `MainMenuButton` | Button | Vertical Box的直接子项，位于BTN_Resume下方 | True |
| `StatusText` | Text Block | Vertical Box的直接子项，位于两个按钮下方 | True |

最终层级如下。**StatusText与两个Button同级，不放进MainMenuButton内部。** 原生ShowReady / ShowError会修改StatusText的内容，选错会把“返回主菜单”按钮文字覆盖掉，即使Compile成功也属于布局错误。

```text
Canvas Panel
  Border（全屏深色背景）
    Vertical Box（居中）
      Text Block（显示：游戏已暂停）
      BTN_Resume（Button）
        Text Block（显示：继续游戏）
      MainMenuButton（Button）
        MainMenuLabel（Text Block，显示：返回主菜单）
      StatusText（Text Block，显示：松开 P 或选择继续游戏返回）
```

1. Hierarchy选**BTN_Resume外层Button**，核对名称和Is Variable。展开它，选里面的Text Block，`Content → Text`保持“继续游戏”；内层文字不要命名为BTN_Resume或StatusText。
2. 若尚无MainMenuButton，从Palette拖一个Button到Vertical Box，放在BTN_Resume下方；若已有则直接选它。将**外层Button**命名为`MainMenuButton`，勾Is Variable。给它放一个Text Block，命名为`MainMenuLabel`，`Content → Text`填“返回主菜单”。MainMenuLabel不参与原生BindWidget，其Is Variable可不勾，勾选也不影响绑定。
3. Hierarchy选原底部“松开P…”Text Block，按F2改名为`StatusText`，勾Is Variable。若没有这个独立文本，从Palette拖Text Block到**Vertical Box**，放在MainMenuButton下面；不要拖到MainMenuButton里面。若按钮内的文字已经误叫StatusText，先将它改为MainMenuLabel，再给底部文本命名。
4. 分别选**BTN_Resume / MainMenuButton外层Button**，Details搜索`Interaction`，检查Is Focusable=True；展开Advanced，Click Method / Press Method均为**Down and Up**。本工程Touch Method也保持Down and Up。若看到Font / Text而没有Interaction，说明选中了内层Text Block，应重新点外层Button。
5. 清空搜索，分别选两个**Button内部的Text Block**，设置`Slot (Button Slot) → Padding`：Left=48、Top=16、Right=48、Bottom=16；Horizontal / Vertical Alignment均为Center。Font Size保持24和既有字体。**48 / 16属于这里的Button Slot内边距，不填到外层Button的Vertical Box Slot上。**
6. 分别选两个**外层Button**，设置`Slot (Vertical Box Slot)`：Size=Auto、Horizontal Alignment=Fill。继续按钮保持原外边距Left=0、Top=20、Right=0、Bottom=20；MainMenuButton外边距Left=0、Top=12、Right=0、Bottom=0。展开Padding后按四个方向逐项核对，两个按钮应等宽、等高。本节不点击OnClicked旁的“+”添加蓝图事件，原生已接收按钮意图。
7. 选**独立StatusText**，`Content → Text`填“松开 P 或选择继续游戏返回”作为Designer占位（句末可加句号）；Font Size=18，保留能显示中文的既有字体。搜索`Wrap`确认Auto Wrap Text=True；清空搜索，设置`Slot (Vertical Box Slot)`：Size=Auto、Horizontal Alignment=Fill、Padding Left=0 / Top=12 / Right=0 / Bottom=0。Fill让长提示使用菜单宽度并正常换行。Text右侧保持没有Binding；运行时ShowReady / ShowError更新文字。这里没有一个名叫Designer Text的控件。
8. **先切右上角Graph，再点顶部Class Defaults**，这才是Widget自身默认属性；Designer下选Button检查的是按钮属性。Details搜索`Focusable`确认Is Focusable=True；清空后搜索`Tick`确认Tick Frequency=**Never**。原生已有DisableNativeTick，最终Compile应无requires native tick警告。完成后可回Designer。
9. Hierarchy分别选Canvas Panel与Border，Details搜索`Visibility`，两者均为**Visible**。选Border，清空搜索，`Slot (Canvas Panel Slot) → Anchors`展开检查Minimum=(0,0)、Maximum=(1,1)，Offset Left / Top / Right / Bottom均为0；`Content`的Horizontal / Vertical Alignment均为Center，使整个Vertical Box居中。搜索`Brush Color`查看背景色，本工程保留深灰R=G=B≈0.085416、A=0.8；`Content Color and Opacity`保持白色(1,1,1,1)，避免将子控件文字染暗。不要把整个菜单设为Not Hit-Testable。
10. 放大Designer预览，确认标题、等宽的两按钮、独立提示依次排列，中文正常且没有重叠；Compile后应显示绿色成功状态，无三个BindWidget缺失或Native Tick诊断。点工具栏Save，资产标签无星号、编辑器显示All Saved。本节完成后接8.5装配，再进入运行检查。

**8.4 computer-use实际检查与修正记录（2026-10-13收尾）：**

检查实际资产`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`。父类ETPauseWidget正确；旧Functions / Event Dispatchers已清空，Event Graph仅有未接执行线的默认Pre Construct / Construct / Tick事件，没有旧继续、暂停或切图业务。Widget自身Is Focusable=True、Tick Frequency=Never；两个外层Button的Is Variable / Is Focusable以及Down and Up触发方式正确。Canvas / Border均Visible，全屏锚点和四Offset正确，背景A=0.8，子控件颜色为白色，中文预览正常。

发现并修正：原`StatusText`误命名在返回按钮内，将该内层文字改为MainMenuLabel；把原底部TextBlock改为真正的StatusText并勾Is Variable。MainMenuButton的Vertical Box Slot原误填左右48 / 上下16，改为0 / 12 / 0 / 0；其内层文字Button Slot由4 / 2改为左右48 / 上下16，与继续按钮一致。底部提示外边距改为0 / 12 / 0 / 0，Horizontal Alignment改为Fill，避免居中窄宽度造成不必要换行。

最终Compile为绿色成功（Good to go）；本次Editor日志无required widget binding或Native Tick警告，工具栏Save后All Saved，磁盘正常保存时间为**2026-10-13 12:36:26**（北京时间）。本记录验收的是**8.4编辑、绑定、布局、编译与保存**；8.5 Controller装配以及第9节实际暂停 / 焦点 / 返回菜单行为仍需后续完成。操作前资产备份在`Docs/Reports/Raw/M1_06D_8_4_Before/`，属于本地恢复材料，无需Push；本次修正的正式WBP_PauseMenu与本文档纳入章末Push清单。

### 8.5 BP_ETPlayerController装配与唯一功能桥接

打开`/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`，Parent保持ETPlayerController。Components仍是**一个UIBootstrap + 一个既有FailureFlow**；不要再加第二个FailureFlow，也不新建GameFlow组件。

1. 选原FailureFlow，Details搜索`Pause Widget Class`填**WBP_PauseMenu**。若搜不到，正常Editor Build / 重开UE是否完成，组件是否确为ETFailureFlowComponent。
2. 原`Failure Widget Class`保持**WBP_ETFailure**，`Main Menu Map`保持**L_ET_MainMenu**，Auto Activate=True。具体旧失败资产名以已验收引用为准，不能误填WBP_ETMainMenu；新增Pause Class不替换Failure Class。
3. UIBootstrap两Widget Class / Presenter保持，菜单Controller蓝图不添加这些游戏组件。
4. Graph空白右键，搜索**Event On Game Menu State Changed**（对应原生OnGameMenuStateChanged），添加红色事件；若没有，My Blueprint → Functions → Override找`OnGameMenuStateChanged`，添加该事件。它是无参数的BlueprintImplementableEvent，**不手动新建同名Custom Event**。
5. 空白右键添加`Get Pawn`，Target=Self（当前ETPlayerController）；从Return Value蓝线拖出`Cast To BP_ThirdPersonCharacter`。
6. 从Cast的`As BP Third Person Character`蓝色输出拖线，搜索**RefreshCandidate**，添加已有角色函数节点，Target必须接这个Cast输出，不留Controller Self。若找不到，回角色蓝图选该函数，在Details确认Access Specifier=Public并Compile，再回Controller从Cast输出搜索；不要新建同名函数替代它。
7. **白线：Event On Game Menu State Changed → Cast To BP_ThirdPersonCharacter → RefreshCandidate。** Get Pawn是纯节点，只接其数据输出到Cast.Object；Cast Failed不接（空Pawn / 其他身体无需更新M0提示）。Get Pawn.Target=Self，不用Get Player Character 0缓存另一份角色引用。

```text
Event On Game Menu State Changed
    → Cast To BP_ThirdPersonCharacter
         Object = Get Pawn(Target = Self).Return Value
         Success → RefreshCandidate(Target = As BP_ThirdPersonCharacter)
         Failed  → 结束
```

8. Compile / Save Controller，再Compile / Save角色和暂停Widget；Save All。本事件只刷新角色已有交互候选 / 提示，不重复暂停、创建菜单或改输入模式。它在打开、继续、失败呈现 / 恢复时由同一个原生组件发出，提示从World.IsGamePaused恢复；无需新增变量、委托绑定或临时测试函数。

至此再Play。原P的IA节点和映射已删，新Controller事件不直接处理P；P输入绑定在C++SetupInputComponent。若P无反应先查第11节，不把旧IA_Pause链重新接回。

## 9. 三组相关检查（总计10–15分钟）

保留Output Log，正常日志文件`E:\UE_Project\Demo\EmberTrial\Saved\Logs\EmberTrial.log`即可；不要求逐条抄整段日志。记录结束完整时分，我来取文件核对。每组新边界通过即可，不重做M1-03身体矩阵、属性全边界或M1-05完整AI矩阵。

### 9.1 暂停 / 焦点 / 当前动作（约4–5分钟）

1. 正式地图Play，HUD100 / 单个HUD，初次一条`[M1-06D] Game flow ready`。靠近A或B让交互提示显示，**松开P**：只出现一个暂停页、鼠标可见、交互提示隐藏、敌人 / 当前动作停止推进；应有`Pause shown ... Paused=1`，不再有M0 Pause opened / Resume requested打印。
2. 用鼠标“继续游戏”恢复，**无需再点击视口**可移动 / 转向；提示按真实候选恢复；日志`Pause removed`→`Pause resumed ... Paused=0`。A / B已激活后的提示应保留真实状态，不缓存旧文案。
3. 再两次P打开，分别用默认聚焦的Enter、Space继续；菜单里这些按键不补跳或播放战斗动作。再P打开，点空白背景，然后松开P应继续；Tab切到返回菜单按钮再Shift+Tab回来也可操作。无需10次循环。
4. 按住W行走时P暂停，菜单中松W，同时按 / 松E、左键、2、4，然后P恢复：角色不自动继续走，菜单期间无交互、伤害或补播。恢复后新按W / E / 左键能正常执行。
5. 站定启动攻击或闪避后立即P，停顿约1秒再继续；原动作完成，没有因为Pause重播 / 取消或漂移。先后检查各一次即可；正常碰墙位移与伤害矩阵已在旧批通过，不重复搭挡板。

### 9.2 暂停返回菜单 / 检查点继续（约3–4分钟）

1. 走入Checkpoint_A，确认当前会话已激活；P暂停，选“返回主菜单”（鼠标即可）。只能发一份`Travel requested Intent=MainMenu`；随后旧Pause removed / Game flow end / Life unbound，菜单无游戏HUD，Continue启用。
2. 菜单用Enter选择“继续本次试炼”，应看到`Intent=Continue Checkpoint=Checkpoint_A`、Confirmed=1；出生在该点，新身体 / HUD100，鼠标隐藏，移动 / 转向无需额外点击，无旧暂停页或旧角色提示事件。
3. 新世界再P打开 / P继续一次，证明新Controller绑定正常。然后移动跳跃、E与一次左键 / 2做短基线；不再执行C缺失ID注入 / NewRun清空矩阵。

此处的“继续本次试炼”沿用C：只保留地图和检查点ID，重载整图、新身体100/100；敌人、训练目标、M1_Target_A / B的交互激活状态回到初始值。暂停页直接“继续游戏”则保留当前世界和已激活目标。不要将返回主菜单后的交互重置判作本批失败；若以后需要保留世界进度，应单独增加按稳定ID恢复的状态系统。

### 9.3 预检拒绝、失败页互斥与结束清理（约3–5分钟）

**A. None Map只测一次：**停止PIE，原Controller选FailureFlow将Main Menu Map临时设None，Compile / Save；新PIE→P→返回主菜单。期望原世界仍暂停、状态文字给拒绝原因、两按钮重新可用、`Travel rejected`与`Panel recovered ... Paused=1`，没有真正切图。点击“继续游戏”应恢复移动；停止PIE，**立即恢复L_ET_MainMenu，Compile / Save**。这个负例是请求预检拒绝，不能称作已经验证真正引擎TravelFailure。

**B. 失败页互斥：**原Map恢复后Play，先P开 / 关一次，再用保留的4=Damage25连续四次真实致死；不要临时改默认生命或伤害。HUD0、只一份Failure；失败页显示“从最近检查点开始…”新文案。此时按 / 松P不会出现Pause，不能恢复死者游戏输入。鼠标重试一次，应走原Retry / 真实新100与当前CP出生；新世界P仍可开 / 关。敌人致死整矩阵已在B / C通过，本批不重做。

**C. 暂停中Stop：**P打开后用编辑器Stop（Esc仍是PIE停止，不改绑定）；再Play一次应未暂停、无旧菜单、100、P正常，停止后日志结束清理。只需这一轮，不重做三轮全战斗演示。

本批没有可自然触发的“世界已经暂停时又发生死亡”操作：正式玩法在暂停期间不推进伤害。代码保留该事件到达时的直接接管保护，**不要求你亚秒盲试、不加临时蓝图去伪造DeathSealed，也不把这条保护写成已实测**。若真实使用中发生重叠或日后自动化需要覆盖，由我在有当次授权时做针对性时序测试；当前按正常死亡后P拒绝与静态所有权检查收口。

完成恢复：Main Menu Map=L_ET_MainMenu，默认100/100、玩家25 / 敌人10、4=Damage25、slomo1、Debug False，Checkpoint_A与Entry原配置保留，Save All，回正式地图。不要为本批重Cook / 打包；M1-07再测当前阶段独立包 / Insights。

## 10. 章末复盘与反馈

通过D后我们用A / B / C已存在的[报告](M1_06_FlowRoadmap.md)与本批日志做章末汇总，**不再建一套“复盘专用验收图”**。你先完成上面三组并发送以下简表，我再核查Source / 资产与日志、更新来源清单和本章报告。这里只是下发复盘提纲，不预先宣称整章验收。

建议你读代码时能说明四件事：

- 为什么GameInstance保存地图包名 / 稳定ID，World / Pawn / Widget需要重新生成；退出程序为何不能继续。
- 为什么Core只发意图，Widget不直接OpenLevel，Controller组件统一持有暂停与失败页，UIBootstrap专管HUD。
- 为什么P分别走Game输入释放与Slate KeyUp，Flush / Viewport焦点 / Down And Up解决的是哪些真实现象。
- 为什么EndPlay主动解绑、当前身体身份检查与死亡sealed顺序一起避免旧事件污染新世界；为什么暂停时不能依赖NextTick完成接管。

后续面试复盘会把这些理由与实际日志 / 代码对应起来；本批不用准备长答案，也不追做磁盘存档、真正加载故障、多人、手柄或美术升级。

```text
M1-06D 完成
两新 / 六旧Source；Editor Build / 索引34cpp / Game Build：
角色旧暂停链 / 两变量 / 两函数删除；IMC旧P行删除：
原暂停Widget父类 / 三BindWidget / 无BP输入业务 / Never：
单UIBootstrap + 单FailureFlow / 两Class与Map / 提示事件桥接：
9.1 P / 鼠标 / Enter / Space / 背景焦点与无补播不卡键：
9.1 攻击 / 闪避中暂停恢复、E真实提示：
9.2 暂停返回菜单 / Continue检查点 / 新100与新P：
9.3 None预检恢复继续 / Map已恢复：
9.3 真实死亡仅Failure / P拒绝 / Retry新100：
9.3 暂停中Stop / 下一轮正常 / 最后短基线：
最终配置恢复、Compile / Save All：
失败或特别说明：
大致完成时间（完整时分，例如21:18）：
实际用时：代码  / 资产  / 检查 ；构建等待：
```

报错先给文字：当前节号、实际文件 / Widget名、首条完整错误及附近10–20行；蓝图连线或Designer不一致时附该局部截图（包含节点引脚或Hierarchy / Details）。不需要只截最后的exit code 6。不能完整描述时说明期望与实际现象，再告诉我是否授权本次直接操作；默认仍由你手动实现C++。

## 11. 常见问题与窄排查

| 现象 | 先查 |
| --- | --- |
| 新Widget Input / FReply链接或include失败 | UI Build.cs的InputCore / SlateCore、正常双构建；回调签名与generated.h位置，不临时开Unity规避 |
| P不打开、无Pause shown | 当前Controller是否BP_ETPlayerController且Parent正确；单FailureFlow / Pause Widget Class；角色是否Ready / Alive；IMC旧P行是否移除；Build后是否开了新Editor |
| P打开立即关闭 / 两份菜单 | 旧IA_Pause链、角色BeginPlay CreateWidget / EV_ResumeRequested、Widget旧OnKeyUp或OnClicked是否仍在；不要补Delay |
| required widget binding缺失 | WBP Parent、三个外层控件准确名字 / 类型 / Is Variable；编译最终资产，不把中间缺控件Warning当完成 |
| Native Tick警告 | ETPauseWidget有DisableNativeTick，WBP Parent确为它，Tick Never，旧动画 / Blueprint Tick是否残留 |
| 暂停提示不隐藏 / 继续不恢复 | Controller原生Event是否真的连Cast成功→RefreshCandidate；Get Pawn.Target=Self，RefreshCandidate.Target=Cast输出；角色Branch直接读IsGamePaused，True Hide / False原链 |
| 恢复要点击视口 / 卡移动 | 输入模式 / Flush / SetFocus代码是否完整；旧角色ClosePauseMenu是否残留；只查9.1，不重做全AI矩阵 |
| None Map后按钮一直disabled | HandleMainMenuRequested与RecoverActivePanel全文、ShowError启用两按钮；提供Travel rejected / Panel recovered日志 |
| 失败页能P开Pause | 角色旧输入没有删，或新HasCurrentBody(false) / FailureWidget / queued门控漏写；给死亡→按P片段 |
| Source替换后重定义 | 指定旧文件全文替换，不能追加第二份函数；UI两个新文件不复制进Core同名目录 |
| CLion文件无Target / 导航失败 | 正常Refresh-CLion完成并Reload正确主工程；见既有[CLion创建类说明](CLion_UEClassCreation.md)，不删除UE或IDE缓存当首选 |

切图pending到旧世界结束期间不会恢复Game Only；新Controller原BeginPlay负责新世界输入。真正TravelFailure回调有恢复路径，但本批负例仅None预检；性能只做事件驱动设计，未测量帧时。当前阶段完整打包仍在M1-07，不拿M0旧exe当本章结果。

**2026-10-13最终验收：**用户13:01全部完成，手动35 / 60 / 10、等待5。八源码块一致、61旧Source不变，69Source / 34cpp / 46资产；四实际资产哈希更新。日志15份Pause显示 / 移除、12组Flow / Life清理、11次继续Paused0、单None拒绝 / 恢复、三MainMenu / 两Continue / 一Retry到达全部确认，窗口无指定运行错误。D的Continue直接日志为Entry，Checkpoint_A规则沿用C不变实现与既有证据，不把本轮Entry写成新A点日志；细节见[D报告](Reports/M1_06D_Verification.md)。8.2 / 8.4当次接管、左键补线及用户配置 / 按键反馈已分清记录。A+B+C+D与本章总验收完成，直接进入[M1-07A](M1_07A_StagePackaging.md)，无新增验收图或重复Build。

## 12. 本批需要Commit / Push的内容（固定放在文档最后）

### 12.1 源码：两新 / 六旧

按第2节八份实际变化暂存：新ETPauseWidget.h / cpp，既有UI Build.cs、ETPlayerController.h / cpp、ETFailureFlowComponent.h / cpp、ETFailureWidget.cpp。新增公开头 / 私有实现位于UI模块。**本文代码块下发不代表这些Source已经变动，需你保存实际文件后再Stage。**不提交无变化的GAS / AI / Session / GameMode / Target / .uproject。

### 12.2 既有可编辑资产

| 变化 | 仓库相对路径 |
| --- | --- |
| 原生暂停父类 / 两按钮 / 状态、删除旧Widget业务 | `EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset` |
| 单FailureFlow的Pause Class / 功能事件桥接 | `EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset` |
| 移除旧角色菜单业务 / 两条件改World暂停 | `EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset` |
| 移除IA_Pause的P映射行 | `EmberTrial/Content/ThirdPerson/Input/IMC_Default.uasset` |

只有这四份资产预计需要变化，Content继续LFS。IA_Pause资产保留，无需删除提交；正式地图、Checkpoint BP、MainMenu / Failure Widget蓝图原配置保留，无实际变化不为清单重存。若实际必要调整了它们，检查Diff / 说明后加入。保持MainMenuMap恢复、没有把None负例配置作为最终提交。

### 12.3 文档、C收尾与提交步骤

提交本指南`Docs/M1_06D_PauseFlow.md`和实际更新的FlowRoadmap / Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / Introduction。本轮进入D时还有C收尾文档未提交：`Docs/Reports/M1_06C_Verification.md`、`Docs/AssetManifest.csv`、C指南，以及B指南 / B报告的交接文字；它们与上述状态文档的实际差异一并保留。**C主体十份Source / 三资产和B报告已在3351b53，不重Stage相同已提交内容；B报告如果本轮有文字差异，提交该差异即可。**

你完成后我根据反馈更新D / 章末验收摘要、来源清单四资产哈希与状态；这些实际生成的文档再加入Commit，当前不提前新增“已通过”报告或改资产来源记录。Raw静态核查 / 日志、Artifacts / 视频 / 包、Binaries / Intermediate / Saved / DDC、IDE / 生成CMake与compile_commands均只留本地，不Force Add。

CLion开启Staging Area时简单步骤：**保存全部文件 → Commit窗口刷新Changes → 将上述实际变化Move to Staged（或Unstage后重新Stage，已Stage又修改时须把Unstaged变化一并Stage）→ 检查Staged Diff → Commit → Push现有origin/main**。不确定某项是否该提交，把该路径 / Git状态给我；不要清缓存或重生成工程来更新Stage。[详细五步](M1_06B_FailureAndRetry.md#1411-clion重新stage最新修改简单步骤)保留供查阅。

建议正文提交标题`feat: unify pause and failure UI flow`；若先只保存C收尾与D指南，可先做`docs: record M1-06C acceptance and plan pause migration`，随后再提交真正D实现 / 验收。提交和Push由你执行，助手不代写Git索引、不代Push，不改历史；本轮只按本地HEAD / origin缓存描述3351b53，未联网核验远端或LFS上传。
