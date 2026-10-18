# M1-04B1：动画命中窗口、扫掠与目标去重

2026-10-18清理同步：本页保留历史配置、接入差异和验收依据，重复整文件以当前Source链接为准。Attack Window的Begin / End共用私有Route，窗口归Trace管理；Combo Window仍转发给当前攻击Ability，两种职责不混合。不要把历史步骤整批重做；本次范围、验证与提交见[Cleanup](Cleanup.md)。

日期：2026-10-06；UE5.5.4。前置：[M1-04A已验收](Reports/M1_04A_Verification.md)。**状态：用户已手动完成并验收，手动40 / 等待5分钟，见[B1结果](Reports/M1_04B1_Verification.md)。后续执行[B2真实伤害](M1_04B2_GameplayEffectDamage.md)。本文保留原实施步骤；验收本轮助手未修改Source、编译或运行新功能。**

## 当前采样与候选职责（清理同步）

TickComponent只负责有效采样点、身体到拳遮挡、连续扫掠与调试绘制；ProcessHitResults按原OutHits顺序去重 / 广播候选，再检查Blocking Hit。回调后立即确认Serial、窗口与Owner，防止同步取消 / 新攻击 / 销毁后继续处理旧结果。

攻击登记清理SeenTargets，单个窗口关闭不清集合；每段攻击各自登记，串行隔离旧通知。CancelActiveAttack额外关窗是无有效登记时的停Tick兜底，不能把幂等保护按“重复”直接删除。Trace不判断阵营、无敌或GE成败，这些由当前Ability / Effect链路负责。

半径、骨骼、偏移与采样距离仍是原蓝图配置；Debug绘制数值不改变碰撞。原生Tag的当前[声明](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h)与[定义](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp)保留名字 / 注册顺序 / 说明字符串，不添加同名ini或运行时字符串别名。本批结果见[Cleanup](Cleanup.md)。

## 1. 本批成果和时间

沿用左键 / 数字1正式攻击。在攻击Montage第8–14帧开启检测，沿右手骨骼连续位置做球形扫掠；一次攻击中每个Pawn只发出一次命中候选。同一目标下一次攻击可以再次命中。只有窗口开启时组件Tick，取消 / 撞墙 / 结束 / 停止PIE关闭检测。

原B1阶段只验证候选，不扣血。当前已接入真实GE、受击 / 死亡与玩家连段，旧版仅打印日志的完整Ability函数不再复制；候选与伤害职责仍分离。下方验收步骤保留为历史经验，不按原阶段数值覆盖当前配置。

预计手动约90–150分钟：代码45–75、窗口 / 目标配置30–50、新行为检查15–25；构建等待和失败排查另计。这是本批预算，不要求按该时长完成，也不把A只用50分钟外推到首次碰撞接入。

无需重建Controller测试函数、不需要新输入、不需要购买动画。保留一份训练目标Blueprint和正式地图中的训练目标实例；它们是后续伤害的开发入口。临时遮挡墙、第二目标和帧率上限检查后恢复。

## 2. 职责与判定规则

| 对象 | 本批职责 |
| --- | --- |
| UETGA_LightAttack | 启动Montage后登记其准确实例ID；订阅候选；结束时解除订阅并撤销本次检测 |
| UETCombatTraceComponent | 每个身体独立保存AttackSerial、窗口、前一采样点和目标集合；在骨骼更新后查询碰撞 |
| UETANS_AttackWindow | 只把本次通知的Mesh、Montage实例ID和通知身份传给组件；不保存任何角色运行状态 |
| Montage资产 | 描述何时可命中；仍用已有动画 / DefaultSlot |
| BP_ETCombatTrainingTarget | 原生Character直接新建的训练身体，有独立ASC / 属性；胶囊参与查询，模型不再参与查询 |
| 现有HUD | 只展示玩家身体，训练目标命中日志不会把玩家生命改成目标生命 |

NotifyState对象属于动画资产，不能在其中存“当前角色”“已命中集合”或运行计时。相同动画可以被多个身体使用，状态必须归各自组件。

本版碰撞规则：Visibility通道，训练目标胶囊Overlap，墙 / 地面Block；查询忽略自身。球沿上一帧右手位置到当前右手位置扫掠，首个有效采样也查询当前位置的重叠。**SweepMulti返回false不代表没有Overlap结果，必须遍历OutHits。**返回结果遇到Blocking Hit后停止。

还检查身体中心到当前拳位置的阻挡；任一世界阻挡会关闭窗口，并禁止本次攻击后续窗口再打开。这是一版保守近战规则：身体与拳头不能隔墙命中，碰到墙就结束本次命中机会。它尚不是完整刀锋 / 曲线的亚帧采样方案。前后采样相距超过200cm也关闭窗口，避免瞬移后扫过整个房间。

目标集合在**整次攻击**开始 / 结束时清理，窗口开关不清理；以后同一攻击加多个窗口，也不会重复命中同一身体。候选不等于伤害成功，B2再做合法目标 / GE判定。

## 3. 文件准备

先Save All并关闭UE；CLion保留打开。不要用Live Coding添加本批反射类 / 子对象。

以下四份是新文件，属于**EmberTrial核心模块**。可在CLion目录树对应目录右键创建目录 / 文件，再手动输入全文；本批不要求用UE类向导，避免默认路径 / 自动打开另一工作区。没有要求修改的代码保留。

| 文件 | 操作 |
| --- | --- |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Combat\ETCombatTraceComponent.h | 新建，输入第4.1节 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Combat\ETCombatTraceComponent.cpp | 新建，输入第4.2节 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Animation\ETANS_AttackWindow.h | 新建，输入第5.1节 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Animation\ETANS_AttackWindow.cpp | 新建，输入第5.2节 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrial.Build.cs | 第3.1节增加PhysicsCore私有依赖 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h | 第6节局部增加 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp | 第6节局部增加 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Abilities\ETGA_LightAttack.h | 第7.1节局部增加 |
| E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Abilities\ETGA_LightAttack.cpp | 第7.2–7.5节局部增加 / 替换 |

本批涉及四份新文件和五份既有文件。UI模块不变，Practice组件和旧自动测试不变。完成后源码应从37增至41份，工程cpp从18增至20份；不要把文档代码另存为重复编译的第二份cpp。

### 3.1 PhysicsCore私有依赖

本批cpp直接使用`CollisionShape.h`中的FCollisionShape，它属于PhysicsCore模块。打开核心`EmberTrial.Build.cs`，把原私有依赖中的：

```csharp
"GameplayTasks"
```

改为：

```csharp
"GameplayTasks", "PhysicsCore"
```

保留其他依赖、PrivateIncludePaths和完整文件结构。PhysicsCore只用于私有cpp，不加入UI模块、不硬编码引擎头文件路径。

## 4. 检测组件

### 4.1 ETCombatTraceComponent.h当前声明

当前源码：[ETCombatTraceComponent.h](../EmberTrial/Source/EmberTrial/Public/Combat/ETCombatTraceComponent.h)。

### 4.2 ETCombatTraceComponent.cpp当前实现

当前源码：[ETCombatTraceComponent.cpp](../EmberTrial/Source/EmberTrial/Private/Combat/ETCombatTraceComponent.cpp)。

`SeenTargets`用弱引用身份，不用数组索引或Actor名字去重。日志名字只供人读取；对象销毁后同名的新对象仍是另一个目标。`Disarm.Tick=0`表示组件已调用关闭Tick，不是整角色或引擎动画停止Tick。

## 5. 原生NotifyState

### 5.1 ETANS_AttackWindow.h当前声明

当前源码：[ETANS_AttackWindow.h](../EmberTrial/Source/EmberTrial/Public/Animation/ETANS_AttackWindow.h)。

通知保持原生Branching Point与禁止编辑器预览触发；仅路由本次Mesh、Sequence、Montage实例ID、Token和开关，不缓存身体状态。

### 5.2 ETANS_AttackWindow.cpp当前实现

当前源码：[ETANS_AttackWindow.cpp](../EmberTrial/Source/EmberTrial/Private/Animation/ETANS_AttackWindow.cpp)。

通知保持原生Branching Point与禁止编辑器预览触发；仅路由本次Mesh、Sequence、Montage实例ID、Token和开关，不缓存身体状态。

本类在Montage中使用Native Branching Point，获取5.5.4真实播放实例ID。它没有Notify Tick，不负责采样骨骼；组件在Mesh更新完成后采样。不要另建Blueprint NotifyState，也不要在AnimBP事件图加同名通知。

## 6. Character装配原生唯一组件

### 6.1 头文件两处增加

在`EmberTrialCharacter.h`已有forward declarations中增加：

```cpp
class UETCombatTraceComponent;
```

在现有`private`成员区域（类开头、public之前，与ASC / 属性 / 能力Class并列）加入：

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Combat",
    meta = (AllowPrivateAccess = "true"))
TObjectPtr<UETCombatTraceComponent> CombatTraceComponent;
```

不要移动.generated.h，不改变现有能力Handle / 属性字段，不需要公开一份Blueprint布尔窗口。

### 6.2 cpp两处增加

include列表加入：

```cpp
#include "Combat/ETCombatTraceComponent.h"
```

构造函数中，在创建`HealthAttributes`后、原胶囊配置前加入：

```cpp
CombatTraceComponent = CreateDefaultSubobject<UETCombatTraceComponent>(TEXT("CombatTrace"));
```

BeginPlay / EndPlay / 输入 / 授予 / 生命初始化不改。组件自行绑定身体Mesh与结束兜底，能力正常结束也主动撤销检测。以后所有本类子身体有一个继承CombatTrace；**不要在玩家或目标蓝图再Add Component创建第二个**。

## 7. Ability接入候选回调

### 7.1 ETGA_LightAttack.h局部增加

已有class声明中增加：

```cpp
class UETCombatTraceComponent;
struct FHitResult;
```

已有private区域中，在MontageTask之后加入（不是加在类外）：

```cpp
TWeakObjectPtr<UETCombatTraceComponent> ActiveTrace;
FDelegateHandle TraceHitHandle;
uint32 ActiveTraceSerial = 0;

void HandleTraceHit(uint32 Serial, const FHitResult& Hit);
```

`HandleTraceHit`是原生委托回调，不写UFUNCTION，也不做Blueprint事件。原两个Montage回调 / UFUNCTION保留。

### 7.2 cpp include与CanActivate局部修改

include列表加入：

```cpp
#include "Combat/ETCombatTraceComponent.h"
```

`CanActivateAbility`最后原本是：

```cpp
UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
return IsValid(AttackMontage) && IsValid(AnimInstance) &&
    AnimInstance->GetCurrentActiveMontage() == nullptr;
```

只把该尾部改成下方。之前的Health、Controller、地面 / 速度、Practice等条件**全部保留**：

```cpp
const UETCombatTraceComponent* Trace =
    Character->FindComponentByClass<UETCombatTraceComponent>();
UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
return IsValid(AttackMontage) && IsValid(AnimInstance) &&
    IsValid(Trace) && Trace->CanStartAttack() &&
    AnimInstance->GetCurrentActiveMontage() == nullptr;
```

### 7.3 Ability启动的当前实现

这次在**实际GAS播放实例创建后**登记检测，不能把Montage资产指针当成播放实例身份。任务播放失败 / 同步取消时不会登记残余窗口。

当前源码：[ETGA_LightAttack.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp)。

### 7.4 EndAbility只增加清理块

保留原`IsEndAbilityValid`与`ScopeLockCount > 0`保护。找到这两项之后的：

```cpp
const FString CharacterName = GetNameSafe(ActorInfo->AvatarActor.Get());
```

在这一行**前面**插入下方；不放到ScopeLock分支之前，不整体重写父类结束逻辑：

```cpp
if (UETCombatTraceComponent* Trace = ActiveTrace.Get())
{
    Trace->OnTraceHit.Remove(TraceHitHandle);
    Trace->EndAttack(ActiveTraceSerial);
}
TraceHitHandle.Reset();
ActiveTraceSerial = 0;
ActiveTrace.Reset();
```

这样先撤销检测再由Super停止Montage；随后迟到的旧NotifyEnd因实例身份 / 无活动上下文而被忽略。原End日志、MontageTask置空、Super调用全部保留。

### 7.5 候选回调的当前实现

当前源码：[ETGA_LightAttack.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp)。

原B1回调只输出候选；当前Ability已接入真实Effect，完整策略以Source为准。Trace组件仍不需要Include ASC / 属性 / UI，也不承担伤害政策。

接口已按本机5.5.4的AnimNotifyState.h / AnimNotify.h、AnimMontage.h、AnimInstance.h、AbilitySystemComponent.h、World.h核对；**静态核对不代替用户正常Build和实机结果**。

## 8. Build和索引

1. 保存九份涉及的源码，UE保持关闭。
2. PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

3. Editor Build成功后，在原工程的CLion执行Reload CMake Project。确认EmberTrialCodeModel包含两份新cpp；Ctrl+B可导航到`AnimNotify.h`和`ETCombatTraceComponent.h`。不清缓存、不重新创建CLion工作区。
4. 同一PowerShell执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

5. 成功后打开原.uproject，确认新类可见。失败时先停在Build，把第一条具体编译错误及其上下各5–10行作为文字发来；不要在失败后继续调整动画 / 蓝图。

## 9. 玩家组件与Montage窗口

### 9.1 玩家检查

打开`BP_ThirdPersonCharacter`，Components应有**一个继承CombatTrace**，单个PracticeAction和ASC仍保留。选中CombatTrace，填：

| 项目 | 值 |
| --- | --- |
| Auto Activate | True |
| Trace Bone Name | hand_r（骨骼名，非hand_r_fk_ctrl控制器名） |
| Trace Local Offset | X=0 / Y=0 / Z=0 |
| Trace Radius | 25 cm |
| Max Sample Travel | 200 cm |
| Draw Debug | True，本批检查后改False |

选中玩家Mesh，在Optimization分类搜索`Visibility Based Anim Tick Option`，设为**Always Tick Pose and Refresh Bones**，确保拳位置随Montage更新；只修改玩家Mesh，不把全场所有目标设置为高频骨骼更新。Compile → Save。

GA_ET_LightAttack仍指向原Montage，速度仍3。攻击 / E / P / 2 / 3蓝图事件图**不增加任何节点**。

### 9.2 在Montage本身添加NotifyState

打开`/Game/EmberTrial/Animations/Practice/Montages/AM_M0_Attack_01_Montage`。必须在Montage编辑器操作，**不修改AS或Sequence**。

1. 时间轴找到Notifies轨道。轨道左侧菜单可增加一条Notify Track，命名`ET_HitWindow`，便于识别；不要增加Slot或新动画段。
2. 在该Notify轨道约0.27秒位置右键 → **Add Notify State** → `ET Attack Window`（搜索`ETANS_AttackWindow`也可）。这是第5节原生类，不选普通Notify、Montage Notify Window或Timed Particle。
3. **时间数值在状态条的右键菜单中，不在Details。**先单击空白取消其他选择，再左键单击刚创建的ET Attack Window状态条，确保只选中这一条、不要按住Ctrl。右键点击**状态条中间的本体**，找到菜单中的 **`Notify Begin Time`** 数值框，输入 **`0.266667`**（秒，8/30），然后**按Enter提交**；菜单会自动关闭。再右键同一状态条，找到 **`Anim Notify State Duration`**，输入 **`0.200000`**（持续秒数），再次**按Enter提交**。最终开始0.266667秒、持续0.200000秒、结束0.466667秒（14/30）；Duration不是结束时刻，不填0.466667。输入后只点击别处不会按该菜单的提交逻辑应用修改。
4. 左键选中同一状态条，在Details清空搜索过滤，找到`Montage Tick Type`并设为**Branching Point**。该类`bIsNativeBranchingPoint=true`在Montage中也会强制该路径；看不到此项时不另建蓝图代替，先确认选择的是该原生NotifyState。这个播放方式选项与第3步的时间菜单是两个入口。
5. 只保留**一条**ET Attack Window。不在AnimBP接通知，不手动调用SetAttackWindow，不在窗口内放Delay / Tick图表。
6. Save。预览时第8帧蓄力到第12帧前挥均在窗口中，第14帧结束。编辑器预览不会做Gameplay碰撞，也不会出现Arm / Candidate，这是正确的；要在PIE中观察。

**填写后立即复核：**再次右键同一状态条，确认`Notify Begin Time`约0.266667、`Anim Notify State Duration`为0.2。菜单还提供下表中的帧数项；本动画30fps时，应分别显示**8 / 6**。`Anim Notify State Frames`是持续帧数，**不是结束帧14**。数值框显示的小数位可能少于六位，不需要为了显示精度反复改值。

| UE5.5.4状态条右键菜单项目 | 含义 | 本批数值 |
| --- | --- | --- |
| Notify Begin Time | 开始时间，秒 | 0.266667 |
| Notify Frame | 开始帧 | 8 |
| Anim Notify State Duration | 持续时间，秒 | 0.200000 |
| Anim Notify State Frames | 持续帧数 | 6 |

鼠标悬停状态条时，提示会显示开始秒数、开始帧和持续秒数，可辅助检查。若右键菜单只出现`Add Notify / Add Notify State`，说明点到了**轨道空白**，请右键已有状态条本体；若数值项仍不出现，先取消多选，再只选一条。若有开始时间却没有两个State Duration / Frames项，检查是否误创建了普通Notify，而非第5节的NotifyState。无需更换引擎、清缓存或添加C++时间属性。

**版本核对说明（2026-10-06）：**已读取本机`D:\UE5\UE_5.5\Engine\Build\Build.version`，确认为5.5.4；[SAnimNotifyPanel.cpp](D:/UE5/UE_5.5/Engine/Source/Editor/Persona/Private/SAnimNotifyPanel.cpp:2813)明确在单个通知的右键菜单创建上述四个数值项，并仅在Enter时提交。[AnimTypes.h](D:/UE5/UE_5.5/Engine/Source/Runtime/Engine/Public/Animation/AnimTypes.h:318)的Duration没有EditAnywhere标记，不能按普通Details可编辑属性寻找。原文把时间输入入口写成Details不适用于本机版本，现已修正；本次只核对源码和更新文档，没有改动UE资产。

窗口覆盖既有8→12帧前挥及短暂接触，不在准备 / 长收招阶段保持命中。范围后续可以依据实际轨迹微调，本批先固定上述数值。没有播放窗口前Arm只是登记上下文，组件Tick仍关闭。

## 10. 可复用训练目标：只配置，不设计事件图

### 10.1 原生父类直接新建

在`/Game/EmberTrial/Tests/Blueprints`右键 → Blueprint Class → All Classes → 搜索并选择原生**EmberTrialCharacter**，命名`BP_ETCombatTrainingTarget`。**不要Duplicate玩家、Probe或Waiting**，避免已记录的UE5.5原生属性子对象引用问题。

此身体将一直保留作B2 / C的训练入口；现在不改正式GameMode默认Pawn，不增加AI。

### 10.2 Class Defaults

| 配置 | 数值 / 资源 |
| --- | --- |
| Initial Health / Initial Max Health | 100 / 100 |
| Light Attack Ability Class | None，训练目标不自动攻击 |
| Auto Possess Player | Disabled |
| Auto Possess AI | Disabled |
| Default Mapping Context | /Game/ThirdPerson/Input/IMC_Default |
| Jump Action / Move Action / Look Action | 沿用原模板IA_Jump / IA_Move / IA_Look |

选择继承CharacterMovement，设`Default Land Movement Mode=None`、`Default Water Movement Mode=None`、`Gravity Scale=0`、`Max Walk Speed=0`、`Run Physics with No Controller=False`。这让未被控制的训练身体固定摆放，不另加BeginPlay或Tick逻辑。

### 10.3 胶囊：唯一命中碰撞体

选择继承CapsuleComponent：

| 属性 | 值 |
| --- | --- |
| Capsule Radius / Half Height | 32 / 96 |
| Collision Presets | Custom |
| Collision Enabled | Query Only（No Physics Collision） |
| Object Type | Pawn |
| 所有通道初值 | Ignore |
| Visibility | **Overlap** |
| Camera / Pawn | Ignore |
| Generate Overlap Events | False，本次是主动查询，不靠BeginOverlap事件 |

先把响应表全部Ignore，再只将Visibility改Overlap，避免继承Pawn预设的Block混入。Generate Overlap Events=False不会禁止Sweep查询；Collision Enabled=No Collision才会禁止。Pawn Ignore允许玩家靠近目标而不推挤，也不会改变目标在Visibility扫掠中的Overlap响应。

选择继承Mesh：本目标保持Skeletal Mesh=None，Collision Presets=NoCollision。不需要AnimBP或动画；以后C再装配可受击身体。不要删除继承Mesh或ASC / CombatTrace，不添加PracticeAction。

### 10.4 可见占位模型

Components点击Add → Static Mesh，命名`TrainingBody`，**拖到继承CapsuleComponent下面**成为其子组件，不挂到Mesh / CameraBoom下面。选择TrainingBody填写：

| 属性 | 值 |
| --- | --- |
| Static Mesh | /Engine/BasicShapes/Cube |
| Relative Location / Rotation | 均0 / 0 / 0 |
| Relative Scale | 0.64 / 0.64 / 1.92 |
| Mobility | Movable |
| Collision Presets | NoCollision |
| Generate Overlap Events / Simulate Physics | False / False |

模型宽64 / 高192cm，和半高96cm胶囊的高度对应；查询始终只命中胶囊。Compile → Save。Event Graph保持空；本批没有蓝图白线、Target或IsValid需要补接。

### 10.5 正式地图放置

打开`L_ET_VerticalSlice`，放一个该Blueprint实例，Outliner命名`TrainingTarget_A`：Location **X=-490 / Y=25 / Z=96**，Rotation **0 / 0 / 0**，Scale **1 / 1 / 1**。沿用原PlayerStart **-600 / 0 / 120、零旋转**，默认出生面朝+X、落地中心Z约96；目标在出生点前方约110cm、右侧25cm。

保留原地面、四墙、交互A / B与一个PlayerStart。检查训练目标没有与交互方块重叠；如你的地图位置已改，以出生点前方110cm / 右侧25cm / 地面上96cm为相对位置换算，并在反馈写最终位置。不要为命中临时把TraceRadius扩大到几百cm。

地图Save All。首次PIE日志应有目标单独的Health initialized=100/100 / Ready=1；它没有Controller仍可有合法属性，玩家HUD依然订阅玩家。目标配置非法或出现Missing gameplay dependencies时先解决目标装配，不能把Ready=0候选当作通过。

## 11. 分层检查：共享准备一次，约15–25分钟

仅本批新空间 / 时序行为与直接相关的取消 / 暂停。没有修改的旧HUD重建、不同身体五函数、动态MaxHealth、Practice位移 / Lifecycle源码沿用已有结果；**这批不要求重跑Lifecycle**。本批尚无新自动测试，不能把旧Success当成新Trace的证明。

### 11.0 共享准备

- 新地图、Default Player Start、TrainingTarget_A已保存；玩家站定朝+X，组件Draw Debug=True。
- Output Log可筛选`M1-04B1`，需要时取消筛选查看M1-04A和暂停日志。不要清空日志文件；下一轮由助手读时间片段。
- PIE先点击视口捕获输入，等角色落地约1秒。每次等攻击完全结束再开启下一次，避免把合法的忙碌拒绝误当命中失败。
- 每次完整攻击应Arm→WindowOpen→Candidate / TraceHit（有目标时）→WindowClose→Disarm；取消时可没有WindowOpen。日志同一Serial来自同一次攻击，名字数字不作为去重依据。

### 11.1 首次命中、去重与下一次重置

**准备：**共享出生位与单目标，停止移动。

**操作：**左键一次，等结束；再左键一次；再用数字1一次。播放中可连按左键，确认不重启。

**预期：**每次一次Arm与Disarm，Serial递增；黄色轨迹仅第8–14帧附近出现，同一Serial / 同一目标最多一条Candidate与一条TraceHit，Unique=1、Sweeps>0。三次各能再次命中；TraceHit.Ready=1 / 100 / 100 / DamageApplied=0。HUD100/100正常。

若第一下空挥，先看是否WindowOpen / Sweeps>0以及黄色轨迹在哪里。只能按轨迹把目标少量移近 / 侧移，或角色走近后站定再打；报告最终位置，不同时改窗口、骨骼名和半径。没有轨迹先查Arm / 窗口，轨迹触及目标却无Candidate先查**胶囊Visibility=Overlap / QueryOnly**及模型NoCollision。

**清理：**保留最终可命中位置，停止操作。不增加临时打印函数。

### 11.2 范围外与窗口外

**准备：**同一轮PIE，目标不移动。

**操作：**玩家后退至少300cm，站定空挥一次；再靠近目标，站定正常挥一次。观察准备阶段和收招阶段是否持续画新的黄色球。

**预期：**范围外Unique=0，正常近处Unique=1。窗口外不新增查询轨迹；残留黄色线可能显示0.12秒、红色标记0.35秒，这只是调试绘制寿命，不是窗口未关闭。两个Disarm均Tick=0。

**清理：**站定回近处；无参数修改。

### 11.3 取消、立即下一次与无旧通知干扰

**准备：**近处单目标，同一轮PIE。

**操作：**攻击开始立即按3一次（窗口前）；再攻击，看到黄色轨迹时按3一次（窗口中）；两次均等受击结束后立刻再次攻击。各做一次即可。

**预期：**窗口前取消可以Windows=0 / Sweeps=0；窗口中取消立即WindowClose与Disarm，不继续Candidate。下一次新Serial正常开窗命中，旧NotifyEnd不会提前关闭它。攻击期间2仍拒绝，原2→3与最后移动跳跃可用。

已经发出的本次Candidate不会被取消“撤回”，这是正常时序；B2已结算伤害也不会因取消回滚。本组观察取消后不新增命中。

**清理：**等动作恢复Idle，不改输入和资源。

### 11.4 窗口内暂停与菜单鼠标

**准备：**近处目标；P流程沿用A。

**操作：**攻击接近第12帧时按P。若没赶上窗口，只重试这次；暂停中左键点继续按钮，恢复后让原攻击结束，再新按左键攻击一次。

**预期：**暂停时角色 / 轨迹停止新增，日志不持续增加Candidate。点击继续不补发新攻击；原窗口恢复可以继续查询，但同一Serial命中过的目标不再次Candidate。随后新Serial能命中。

**清理：**菜单关闭、等待Idle；不为精确时点加慢放代码 / 临时输入。如窗口前暂停也通过，但该轮不能代替窗口中暂停结果。

### 11.5 世界遮挡：防止后续帧穿墙

**准备：**Stop PIE，原训练位置已记下；本组只增加一个临时Cube挡板。

**操作：**放Cube，命名`SM_ET_TempTraceWall`，Location **-545 / 0 / 100**，Rotation全0，Scale **0.1 / 2 / 2**，Static / BlockAll / 不模拟物理。它位于玩家和目标之间。Play落地站定攻击一次；Stop删除该临时挡板，再Play攻击一次。

**预期：**有挡板时Blocked指向该墙、Disarm.Blocked=1且目标不Candidate；碰到阻挡后该Serial不再从墙后继续查目标。拆墙后新的攻击正常命中。若PlayerStart不是共享值，挡板相对放在玩家前方55cm、与目标之间，高200cm、宽200cm、厚10cm，避免穿入出生胶囊。

**清理：**删除临时挡板、Save地图；保留训练目标。不要通过把正式墙Visibility改Ignore来修“没有命中”。

### 11.6 两个目标与采样速率短检查

**准备：**Stop PIE，直接再拖一份**同一训练Blueprint**到地图，不Duplicate Blueprint资产，命名`TrainingTarget_B`；Location **-490 / 50 / 96**、零旋转 / 单位Scale。这里允许胶囊互相重叠，以证明候选遍历不只取第一个。

**操作：**Play，站定攻击一次；确认黄色球同时接触两个胶囊范围。控制台输入`t.MaxFPS 30`，站定再攻击一次；恢复`t.MaxFPS 60`再攻击一次。每次只确认对应两目标候选与Disarm，不复测旧闪避位移。

**预期：**同一Serial对A / B各一条Candidate，Unique=2；30 / 60上限均有Sweeps>0且成功去重。采样次数可不同，不要求相同；限帧不保证实际帧率恰好等于上限，也不代表已建立量化性能基线。如第二目标未触及拳轨迹，先少量调位置使两胶囊均进入范围，不扩大半径 / 扩窗。

**清理：**Stop，删除TrainingTarget_B实例，保留A；控制台恢复`t.MaxFPS 0`（原先有固定上限则恢复原值），保存地图。第二实例只作一次空间检查，Blueprint不删除。

### 11.7 停止PIE与短基线

**准备：**只有A、没有临时挡板，帧率已恢复。

**操作：**一次攻击中直接Stop，再Play正常攻击一次；等Idle后检查E / P、移动转向跳跃；Stop，玩家Draw Debug改False，玩家与目标Compile / Save，Save All。

**预期：**停止PIE关闭旧检测；新一轮独立Serial从1开始是正常，不能把不同PIE的Serial作为同一对象比较。新攻击正常，玩家HUD100/100，原基础输入正常；无Accessed None / 新Error。关闭Draw Debug只去绘制，命中仍工作。

**清理：**只保留一份正常训练目标A、Montage一条窗口；无新增临时事件图 / 按键，地图和资产正常保存。

## 12. 反馈与报错方式

不需要手动复制全部日志。完成时间用于助手读取本地日志；只需如下短反馈，失败只标失败组并补现象。先定位失败组，不重做已通过且无关的全部旧功能。

```text
M1-04B1 完成 / 卡在第__节
Editor Build / 索引 / Game Build：
单个继承CombatTrace / hand_r / 25cm / 原Montage8–14帧窗口：
原生训练目标 / 单胶囊查询 / Ready100/100：
单目标去重 / 下一次重置 / 范围与窗口外：
窗口前和中取消 / 立即下一次 / 暂停鼠标：
遮挡墙 / 双目标 / 30与60上限：
停止重开 / E / P / 移动跳跃 / HUD100/100：
临时挡板与B实例删除、帧率恢复、Draw Debug=False、已保存：
失败或特别说明（含最终目标位置，如有调整）：
大致完成时间：
实际用时：代码__ / 资产配置__ / 检查__；构建等待__分钟
```

编译报错发第一条具体错误及上下文文字；Arm有、WindowOpen无时发Montage通知条和Details截图；轨迹有但漏目标时发胶囊响应表和相对摆放截图；取消 / 暂停时序问题用5–15秒短视频加操作顺序。不清缓存，不删除Intermediate试错，不默认授权助手直接改C++。

原B1候选检查当时不代表伤害验收。当前B2 / C及后续连段已完成，长期自动化包含真实Montage通知 / 扫掠 / GE；清理回归结果见Cleanup，历史手动矩阵不要求每批重做。
