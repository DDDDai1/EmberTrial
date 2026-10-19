# M1-04A：正式 GAS 单段攻击

2026-10-19第18批清理同步：Character门控、Ready / ActorInfo与结束流程以当前Source为准，勿用早期完整函数覆盖已有连段、闪避或治疗。本轮清理由助手按授权直接执行；普通功能开发保留手动实践约定。当前检查与提交清单见[Cleanup](Cleanup.md)。

日期：2026-10-06；UE 5.5.4。前置M1-03通过。**状态：已验收。用户手动50分钟 / 构建等待2分钟，源码、双目标receipt、索引与能力日志核对通过，见[A验收](Reports/M1_04A_Verification.md)。**下方实施步骤保留供复查，无需重做；下一步[B1命中窗口与扫掠](M1_04B1_AttackTrace.md)。整体分批见[M1-04路线](M1_04_CombatRoadmap.md)。

## 1. 本批成果、范围与时间

地面站定时，鼠标左键或数字1启动正式GAS攻击，播放现有攻击Montage。活动期间移动、跳跃、E和练习闪避拒绝，镜头与P可用；完成后恢复。数字3先取消正式能力，再播放原练习受击。长按不重复开始，忙碌请求不缓存；菜单中不执行、不补播。

目前没有命中 / 伤害、体力消耗、冷却、连击和正式死亡。HUD仍100/100是正确结果。攻击仍要求水平速度≤3 cm/s且在地面；新规则没有放宽移动攻击。

预计手动85–130分钟：源码约50–80、资产 / 蓝图约25–35、检查10–15分钟；正常构建等待另计，排障另计。必要检查见第8节，沿用基线见[M1-03C](M1_03C_DataFlowReview.md)。

### 1.1 文件与资源清单

| 操作 | 完整路径 / 资源 |
| --- | --- |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Abilities\ETGameplayTags.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Abilities\ETGameplayTags.cpp` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Abilities\ETGA_LightAttack.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Abilities\ETGA_LightAttack.cpp` |
| 修改 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrial.Build.cs` |
| 局部修改 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h` |
| 局部修改 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp` |
| 单处修改 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.cpp`：外部活动Montage检查 |
| 后续创建 | `/Game/EmberTrial/Abilities/GA_ET_LightAttack` |
| 后续创建 | `/Game/EmberTrial/Input/Actions/IA_ET_LightAttack` |
| 配置 / 接线 | 既有IMC_Default、BP_ThirdPersonCharacter |

Practice组件只修改第5.7节的一处外部Montage条件，其他状态 / 位移 / 生命周期代码保留；不修改AttributeSet或UI模块。新源码完成后预计Source文件由33变37；这不是本批已经落盘的声明。

## 2. 准备与类创建

1. 停止PIE、保存全部资产并关闭UE。CLion可保持打开。
2. 在现有根工程 `E:\UE_Project\Demo\EmberTrial` 中创建文件；不要打开只有两份源码的新工程窗口。
3. 在Public / Private分别新建 `Abilities` 文件夹，再按表创建四份文件。可以在CLion创建空文件后输入第3–4节；Tags不是UObject类，不用UE创建C++类向导。
4. 若想用UE向导创建攻击类：All Classes中选GameplayAbility，名称填 `ETGA_LightAttack`，最终仍确认.h在Public/Abilities、.cpp在Private/Abilities，工程中不得留下同名副本。输出内容以第4节为准。操作后再保存并关闭UE进行正常构建。
5. `.h`不能被CLion自动变成`.hpp`；攻击头文件的generated.h必须为最后一个include，Tags头文件不含generated.h。

## 3. 原生Gameplay Tags与依赖

### 3.1 ETGameplayTags.h：完整内容

```cpp
#pragma once

#include "NativeGameplayTags.h"

namespace ETGameplayTags
{
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Light);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);
}
```

### 3.2 ETGameplayTags.cpp：完整内容

```cpp
#include "Abilities/ETGameplayTags.h"

namespace ETGameplayTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Light,
        "Ability.Attack.Light", "Identifies the single light attack ability.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking,
        "State.Attacking", "Owned by GAS while a formal attack is active.");
}
```

Tag由原生代码注册。不要再创建同名DataTable条目或在DefaultGameplayTags.ini手动重复定义。

### 3.3 当前模块依赖入口

[EmberTrial.Build.cs](../EmberTrial/Source/EmberTrial/EmberTrial.Build.cs)保留当前公开GAS / Tags / AI依赖与内部Tasks / PhysicsCore依赖，不再复制A阶段旧全文。Core仍不引用UMG / UI模块，本次没有修改依赖。

## 4. 正式攻击能力

### 4.1 ETGA_LightAttack.h：完整内容

当前源码：[ETGA_LightAttack.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h)。

当前声明包含段配置、三层身份与私有玩家段上下文查询；资产字段与公开请求接口保持。

### 4.2 ETGA_LightAttack.cpp：完整内容

当前源码：[ETGA_LightAttack.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp)。

当前实现统一两个玩家请求的身份检查，各自保留连段资格和恢复窗口门控；伤害与资源清理仍按现有链路执行。

### 4.3 为什么这样分工

- `InstancedPerActor`让每个已授予的能力拥有自己的运行时Task；`bRetriggerInstancedAbility=false`拒绝忙碌时重启。独立角色持有独立ASC / Spec。
- `ActivationOwnedTags`由GAS添加 / 移除，Character只查询。此阶段不要手动Add / Remove Loose Gameplay Tag。
- `CommitAbility()`保留正式提交契约，但当前没有配置成本或冷却，所以不会扣体力。
- Montage使用Ability Task；只在完整Completed或取消 / 中断时结束，**不连接OnBlendOut来提前解锁**。最后参数True允许淡出阶段的中断也完成清理；能力结束时Task停止自己拥有的Montage。
- EndAbility沿用引擎重复结束和ScopeLock保护；不以自设布尔标志提前拦截引擎延后的结束调用。
- `ActivateAbility`由本类完整实现，不调用基类的Blueprint激活分发。能力蓝图只装配资源，不写Activate / End事件图。

概念参考[Epic Ability Tasks](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-tasks-in-unreal-engine)：任务承载跨帧能力行为并随能力结束清理。官网当前文档版本较新，本节签名、参数与生命周期依据本机5.5.4的GameplayAbility.h/.cpp、AbilitySystemComponent.h及AbilityTask_PlayMontageAndWait.h/.cpp核对。下发时仅作静态核对；现用户已完成正常Editor / Game Build与实机验收，结果见本文开头链接。

## 5. Character局部接入

不要整体覆盖角色类，保留全部相机 / 输入 / ASC / 属性 / GameplayReady代码。以下每一步有明确插入位置；没有要求修改的函数保持原样。

### 5.1 头文件include与声明

在 `EmberTrialCharacter.h` 的include列表加入：

```cpp
#include "GameplayAbilitySpecHandle.h"
```

放在 `#include "EmberTrialCharacter.generated.h"` **之前**。在已有forward declaration列表加入：

```cpp
class UETGA_LightAttack;
```

在现有 `bool bGameplayReady = false;` 后、`void SetGameplayReady(...)` 附近的私有成员区加入：

```cpp
UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Abilities")
TSubclassOf<UETGA_LightAttack> LightAttackAbilityClass;

FGameplayAbilitySpecHandle LightAttackAbilityHandle;

void GrantLightAttackAbility();
```

在public区域、`EnsureGameplayInitialized()` 声明之后、现有Ready委托属性之前加入：

```cpp
UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
bool RequestLightAttack();

UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
void CancelLightAttack();

UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
bool IsFormalActionIdle() const;
```

`LightAttackAbilityClass`是默认类配置；Handle只在本次身体运行时使用，不作为存档数据。当前只授予一个能力，暂不搭通用Loadout框架；后续能力实际增多再扩展。

### 5.2 cpp include

在角色.cpp现有include列表末尾加入：

```cpp
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "GameplayAbilitySpec.h"
```

现有 `#include "Abilities/GameplayAbilityTypes.h"` 保留；不要改回没有 `Abilities/` 前缀的旧错误路径。

### 5.3 BeginPlay：在属性初始化后尝试授予

现有末尾是：

```cpp
RefreshEmberAbilityActorInfo(TEXT("BeginPlay"));
EnsureGameplayInitialized();
```

保留这两行，在它们之后添加一行：

```cpp
GrantLightAttackAbility();
```

不要在Tick、每次Possess、每次按键或UI重建里GiveAbility。空能力配置允许测试身体只提供生命属性；本批只在正式玩家蓝图赋值。

### 5.4 当前授予、请求与取消

[Character.cpp](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)为当前入口。GrantLightAttackAbility保存本身体授予的句柄；RequestLightAttack在未活动时请求GAS，活动时仅向现有轻攻击实例提交连段输入；CancelLightAttack按保存的Spec Handle取消。

IsFormalActionIdle包含现有正式状态，不恢复为A阶段只看Attacking的版本。请求返回True表示接受激活 / 缓冲意图，不代表动画或伤害成功，Montage、Trace和GE继续各自验证。

### 5.5 移动与跳跃共用私有门控

Move与StartPracticeAwareJump统一调用CanAcceptLocomotionInput：先查询正式Idle，通过后才检查Practice；组件无效时保留原允许规则。公共IsFormalActionIdle与蓝图入口保持，不复制状态或删除旧组件。

Look仍可转镜头，跳跃Completed仍StopJumping；没有新增IgnoreMoveInput计数锁。门控只在[Character.cpp](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)维护。

### 5.6 当前EndPlay清理

[EndPlay实现](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)先封锁重入、撤销Ready，再取消本ASC能力，清除Character保存的轻攻击 / 闪避 / 治疗句柄，清Cue / ActorInfo并交还Super。不能按早期只清轻攻击的完整函数覆盖。

Spec Handle以默认构造赋值清空，不调用不存在的Reset；不移除整个ASC或重复创建AttributeSet。组件各自清理自己的订阅 / Task，身体按原顺序退出。

### 5.7 Practice组件：活动Montage检查只改这一处

打开 `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.cpp`，在 `RequestPracticeAction()` 中找到完整原条件：

```cpp
if (IsPracticeActionIdle() && AnimInstance->IsAnyMontagePlaying())
{
    return RejectRequest(RequestedState, TEXT("ExternalMontage"));
}
```

仅改成：

```cpp
if (IsPracticeActionIdle() && AnimInstance->GetCurrentActiveMontage() != nullptr)
{
    return RejectRequest(RequestedState, TEXT("ExternalMontage"));
}
```

原方法在本机5.5.4检查Montage实例数组非空，已停止但仍淡出的实例也会被算入。改为检查尚未停止的活动实例：正式能力取消后允许受击平滑接入，真正活动的外部Montage仍拒绝。第4节正式攻击采用相同边界；已经停止的视觉淡出不再是动作状态拥有者。

不修改组件的Attack / Dodge / HitReact规则、Montage结束序号、外部根运动保护或测试源码，也不Stop All Montages清空其他系统。因为此条件实际改变，本批第8.6复用旧Lifecycle入口运行一次相关自动回归。

## 6. 正常Build与索引

1. 保存所有源码，确认UE已关闭。不使用Live Coding / Hot Reload替代本次反射属性与新类构建。
2. PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

此脚本已包含Editor正常Build；成功后不用再重复Build同一Editor目标。CLion在既有根工程Reload CMake Project，确认两个新cpp属于 `EmberTrialCodeModel`，引擎GameplayAbility / NativeGameplayTags / AbilityTask头文件能够导航。

3. 执行一次Game目标Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

4. 两目标成功后重开 `E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`。

失败就停在首个错误，不继续装配基于旧类的能力。反馈首个error前后约20行文字和文件 / 行号；目录或索引问题再附截图。不需要为了本批修改Refresh-CLion脚本或删除缓存。

## 7. 资产与蓝图逐项配置

### 7.1 新建能力资源并配置正式玩家

1. Content Browser在 `/Game/EmberTrial` 新建 `Abilities` 文件夹。
2. Add / 右键 → Blueprint Class → All Classes，搜索 **ETGA_LightAttack**，选该原生父类，命名 **GA_ET_LightAttack**。不要选普通GameplayAbility作为直接父类。
3. 打开能力蓝图，Class Defaults搜索 `Attack`：
   - Attack Montage：`/Game/EmberTrial/Animations/Practice/Montages/AM_M0_Attack_01_Montage`。
   - Attack Max Horizontal Speed：`3.0`。
4. 不创建Event ActivateAbility / EndAbility节点，不添加Delay、Play Montage或手动Tag节点；不修改默认实例 / 网络策略 / Owned与Blocked Tags / Cost / Cooldown。Compile → Save。
5. 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`，选 **Class Defaults**，搜索 `Light Attack Ability Class`，赋值 **GA_ET_LightAttack**。这是Character Self默认属性，不是PracticeAction组件上的Montage属性。
6. PracticeAction仍单个，Auto Activate=True，三个原Montage与150 cm / 0.30 s配置保留；原攻击Montage引用可以保留但旧输入不再调用它。Initial Health / Max Health保持100/100。
7. Compile → Save。测试Probe / Waiting_Rebuilt不赋攻击类，不更改其配置；新地图继续默认玩家BP_ThirdPersonCharacter。

### 7.2 创建左键输入并加入现有映射

1. 在 `/Game/EmberTrial/Input/Actions` 创建Input Action **IA_ET_LightAttack**。
2. Value Type=Digital (bool)，Triggers / Modifiers为空，Trigger When Paused=False；Save。
3. 打开既有 `/Game/ThirdPerson/Input/IMC_Default`，添加一条Mapping：Action=IA_ET_LightAttack，Key=**Left Mouse Button**，该Mapping的Triggers / Modifiers为空；Save。
4. 既有数字1 / 2 / 3映射保留，不新增另一个IMC，不再添加Add Mapping Context。既有数字1对应IA_PracticeAttack仍保留，但调用将迁到正式能力。

### 7.3 左键Started：新事件，只调用Character命令

在 **BP_ThirdPersonCharacter → Event Graph** 中：

1. 空白右键搜索 **IA_ET_LightAttack**，添加Enhanced Input Action事件；使用 **Started**白色输出，Triggered / Completed / Canceled留空。
2. 拖出变量 **bPauseMenuOpen**，选择Get。右键搜索 **Is Game Paused**，放置纯查询节点。右键搜索 **OR Boolean**，两红色输入分别接这两个bool输出。
3. 放置Branch，Condition接OR输出；**Started → Branch执行输入**。
4. Branch **True留空**，表示菜单 / 暂停时结束这次请求；**False → Request Light Attack**执行输入。
5. Request Light Attack来自Self原生函数，**Target=Self**；Return Value可留空，本批用日志和实际行为判断。

```text
IA_ET_LightAttack.Started → Branch(bPauseMenuOpen OR Is Game Paused)
    True → 不再执行
    False → Request Light Attack(Target=Self)
```

没有Is Valid节点，因为Self不是外部可为空对象；不要把Request节点Target设成PracticeAction、ASC或GA Class。

### 7.4 数字1：迁移旧攻击入口

1. 找到既有 **IA_PracticeAttack**事件及其Started到 `Request Practice Action(Attacking)` 的白线。
2. 按住Alt点击旧调用的白色执行输入断开；旧节点保留在Comment `M0 Attack - Disconnected`，不再连任何执行线。既有C1旧函数也保持断开。
3. 按7.3同样创建暂停OR和Branch，输入数据仍明确为 `bPauseMenuOpen` / `Is Game Paused`。
4. **IA_PracticeAttack.Started → 该Branch；True留空；False → Request Light Attack(Target=Self)**。

左键与数字1是同一个正式能力的两个输入入口，不是两份GA。不要把数字1先接旧Practice攻击再接正式攻击。

### 7.5 数字2：只新增正式Idle门控

1. 找到 **IA_PracticeDodge.Started → Request Practice Action(Dodging)**旧链。
2. 增加 **Is Formal Action Idle**纯查询，**Target=Self**。
3. 增加Branch，Condition接查询的红色Return Value。
4. 把原Started白线断开，改成 **Started → 新Branch；True → 原Request Practice Action；False留空**。
5. 原Request的 **Target仍为PracticeAction组件Get引用**，Requested State仍为Dodging。原组件自己拒绝暂停 / 空中 / Practice忙碌，不删其规则。

```text
IA_PracticeDodge.Started → Branch(Is Formal Action Idle(Self))
    True → Request Practice Action(Target=PracticeAction, Dodging)
    False → 不执行
```

### 7.6 数字3：先取消正式能力，再调原受击

1. 找到 **IA_PracticeHit.Started → Request Practice Action(HitReact)**旧链，断开Started原白线。
2. 建立暂停OR / Branch：Condition=`bPauseMenuOpen OR Is Game Paused`。数据连接同7.3，两个查询各接OR一个红色输入。
3. **Started → 暂停Branch；True留空；False → Cancel Light Attack(Target=Self)**。
4. Cancel的白色执行输出 → 第二个Branch；Condition=**Is Formal Action Idle(Target=Self)**的bool Return Value。
5. 第二Branch **True → 原Request Practice Action(Target=PracticeAction, HitReact)**；**False留空**。False不加Delay、重试或缓存。

```text
IA_PracticeHit.Started → Branch(bPauseMenuOpen OR Is Game Paused)
    True → 不执行
    False → Cancel Light Attack(Self)
        → Branch(Is Formal Action Idle(Self))
            True → Request Practice Action(PracticeAction, HitReact)
            False → 不执行
```

第二Branch保证清理没有完成时不会同时启动练习受击。一般取消同步完成；额外保护不能被删除。没有正式攻击时Cancel是无操作，随后原Practice组件继续处理2→3中断及受击不重启。

### 7.7 E函数：把原Idle条件改为两个Idle同时成立

打开角色函数 **TryInteractWithCurrentTarget**，找到之前插入的Practice Idle Branch：其True去目标有效检查，False去Return；更前面原暂停检查保留。

1. 保留 **Is Practice Action Idle**纯节点及其Target=PracticeAction。
2. 新建 **Is Formal Action Idle**纯节点，Target=Self。
3. 新建 **AND Boolean**，两个红色输入分别接上述两个Return Value。
4. 将现有Idle Branch的Condition由单个Practice bool改为AND输出。
5. **原执行线不改**：暂停允许后进入该Branch；True进入原目标检查与交互；False进入原Return。

目标检查中有Is Valid时，Input Object仍为原函数的当前目标引用；本批没有新增Is Valid，也不把两个bool塞进Input Object。

全部Compile → Save。既有暂停函数、Widget与DefaultSlot / Always Update Source Pose设置不改；不再制造新的BeginPlay重复装配链。

## 8. 最小相关验收：共享准备，约10–15分钟

准备一次：打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`，确认100/100原身体、当前能力Class和Montage已赋值，角色Compile / Save通过；Output Log搜索 `[M1-04A]`，**不清除磁盘日志**。以下8.1–8.4可在同一次PIE中连续完成，8.5只做一次重开；8.6为停止PIE后的一个已有自动入口。不要为每组重新搭功能。

### 8.1 正常播放与无重启，约2分钟

- **操作：**站定左键点一次，等待完整结束；数字1点一次；长按左键约2秒并松开；随后在一次攻击中连点左键 / 1。
- **预期：**每次成功启动播放同一个攻击Montage并回待机；单次长按只开始一次，连点在当前攻击活动时不重启、不排队。每次PIE本身体一条Granted；正常攻击有Begin和End `Cancelled=0 AttackingTag=0`。拒绝请求可以有Accepted=0或Busy日志；不要求请求日志与Begin顺序固定。
- **恢复：**松开全部按键，等当前动画结束，站定。HUD100/100不变，无新错误。

### 8.2 身体 / 交互门控与两系统互斥，约2–3分钟

- **操作：**站定发起攻击，期间按住W、按跳跃 / E / 2并转镜头；结束后保持W，确认恢复移动，松W。移动中点左键，站定再点；跳跃在空中点，落地站定再点。另做一次2，闪避中点左键。
- **预期：**攻击时不移动 / 跳跃 / 激活目标 / 启动闪避，镜头可转；结束后按住W恢复移动，松键停止。移动和空中攻击拒绝，站定后只响应新的按下；闪避中正式攻击拒绝，不覆盖旧Montage，也不补播。
- **恢复：**落地、松键、两种动作均结束。此组只检查新增门控，不重测墙壁 / 坡面 / 各FPS位移距离。

### 8.3 正式攻击取消与过渡受击，约2分钟

- **操作：**攻击中按3，受击中再次按3，结束后重新左键；再做一次2→3。可在攻击快收招时补一次3，检查淡出阶段取消。
- **预期：**先结束正式攻击（Cancelled=1、AttackingTag=0），再由旧Practice开始HitReact；受击不被3重复启动，受击结束后左键可重新开始。2→3仍正常。每次被取消的正式攻击只有一次有效End，不能在后续旧回调中解除正在播放的受击或留下攻击锁。
- **恢复：**等待受击完整结束、松键。无需额外临时输入或循环五次。

### 8.4 暂停与新增鼠标输入，约2–3分钟

- **操作：**攻击中按下松开P，菜单停留约2秒；菜单中点左键 / 按1和3，再用键盘继续，观察原动作恢复。另开菜单，用鼠标点击继续；还可在菜单中按住左键，关闭后再松开并重新点一次。
- **预期：**世界与攻击暂停；菜单输入不创建新攻击 / 不取消原攻击；恢复继续原动作并正常结束，不补播菜单按键。点击继续本身不顺带攻击；按住菜单中的左键不会在恢复后自动开始，重新松开按下才接受。恢复后移动不需再点视口。
- **恢复：**松开所有键 / 鼠标，关闭菜单。若出现补播，报告具体关闭方式与短视频；不要自行添加Delay或把Started换成Triggered。

### 8.5 一次结束清理与下一轮，约1–2分钟

- **操作：**正式攻击活动时停止PIE，再Play一次；站定点左键并让它结束，最后靠近目标做一次E、开关一次P，再移动 / 跳跃。
- **预期：**上一轮没有EndPlay之后再启动；新身体只授予一次新Spec、正常100/100 / 一份生命HUD，新攻击可完成；E / P与基础输入正常，无Accessed None / Ability Task异常。结束时正在活动的攻击取消；如果已结束，不要求额外End日志。
- **恢复：**停止PIE，Compile / Save角色、能力 / 输入资产和映射。不要保存运行时状态或删除正式输入；本批没有临时函数需要清理。

### 8.6 已有Lifecycle：运行一次，无新接线

- **准备：**停止PIE，在Output Log中清空搜索过滤，找到下方命令输入框。
- **操作：**输入并回车：

```text
Automation RunTests EmberTrial.M0.PracticeAction.Lifecycle
```

- **预期：**等待测试结束，该项Result=Success；如果显示没有找到测试或失败，保留首个相关错误并报告。它复核被修改的Practice组件，不能替代8.1–8.5的正式GAS能力实机检查。
- **恢复：**无需改地图 / 删除节点；保留日志，不要求重复第二次，也不运行未修改的DodgeMovement整套。

遇到失败只停止并说明该组，不重跑A2 / B3或未受影响的老矩阵。当前没有新增自动测试，不要把旧Lifecycle测试称为正式能力自动化验证。也不要求录制完整新视频，时序有问题时才录短片段。

## 9. 简短反馈与问题材料

```text
M1-04A 完成 / 卡在第__节
Editor Build / 索引 / Game Build：
GA父类 / Montage / Character能力Class / 左键映射；Compile / Save：
正常攻击、长按 / 连按、门控、3取消、暂停鼠标边界、重开：通过 / 失败项
Practice外部Montage条件已修改 / Lifecycle一次：成功或失败
最后E / P与移动跳跃、HUD100/100：
失败现象或最需要说明的地方：
大致完成时间：
实际用时：代码__ / 蓝图资产__ / 检查__分钟；构建等待__分钟
```

如果没有分开计时，填总手动用时即可，不为统计额外补测。日志由助手读取；编译错优先文字，节点Target / 引脚问题用截图，取消 / 淡出 / 暂停时序用短视频。不要手动复制全部Begin / End；给出发生时间和一次操作顺序即可。

助手收到后将核对改动范围、构建 / 保存与对应日志，再登记两份新资产及来源。未创建前不写入资产清单；未验收前不标A完成。后续B才接命中窗口与真实伤害。

## 本次清理 Commit / Push（固定放在最后）

本页历史功能已完成，不重做旧步骤。第18批为Character两份Source与八份Markdown，共十项；最新差异、检查结果与逐文件清单见[Cleanup末尾](Cleanup.md)。用户重新Stage后Commit / Push，Raw / 索引 / 日志留本地；助手不操作Git索引。
