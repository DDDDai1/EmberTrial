# M1-05C2：停步转向与行为树自动近战攻击

2026-10-19清理同步：保留历史接入、资产配置、排障和验收经验；完整类 / 旧完整方法改为当前Source入口，不按旧片段覆盖已完成工程。当前敌人已能感知、追击、转向并真实攻击；90 / 100 / 100厘米与0.5秒是现有配置，150 / 200属于早期阶段。普通功能开发保留手动实践约定，本轮清理按用户授权由助手直接修改；验证和本批提交清单见[Cleanup](Cleanup.md)。

下发：2026-10-09；复核：2026-10-10；引擎固定 **UE 5.5.4**。**C2已实施并验收通过，无待补项。**用户15:20全项完成，手动35 / 等待5分钟，源码与双构建 / 26cpp索引、真实自动交战 / 有限重试 / 清理核对，见[验收报告](Reports/M1_05C2_Verification.md)。以下保留实施步骤供复盘，已通过项无需重做。历史主体提交9cdbc75；后续[D复盘](M1_05D_EnemyCombatReview.md)已完成。当前清理范围与新验证记录见Cleanup，不重复实施本章。

原开发按C++手动实践约定完成；当前清理按本轮授权由助手直接修改和验证。本章不新增Controller测试函数 / 按键。最后一节列出本章Commit / Push内容；以后各开发章也采用这一形式。

## 1. 完成后看到什么、需要多久

进入正式地图，敌人发现玩家后追击；靠近后停止，转到玩家方向，再使用已有Enemy轻攻击。攻击过程中朝向固定，玩家能侧移躲开拳轨迹；真实命中扣10、受击与HUD同步。技能结束后有收招间隔，再判断追击或下一次攻击。敌人受击、目标失效、死亡、停止PIE均能退出当前任务并清理自己启动的攻击。

本章预计手动 **60–100分钟**：理解 / 写代码约35–60、资产配置约15–25、四组相关检查约10–15；构建等待另计。这是预算，实际时间按你完成反馈记录，不预先加入累计工时。

| 层 | 本章职责 |
| --- | --- |
| Character | 只公开已授予轻攻击的Spec Handle供原生任务识别；原Request入口与伤害实现保留 |
| AIController | 感知 / 身体生命周期继续归它；增加近战资格、距离参数与失败重试时间 |
| 原生BT Task | 一次停步 → 转向 → 请求 → 等待自身Spec结束；取消 / 解绑自己拥有的请求 |
| BB / BT资产 | 保存决策结果、组织攻击优先级和收招 / 等待；不直接扣血 |
| 既有GA / Trace / GE / Life / HUD | 继续处理动作、真实拳轨迹命中、阵营、伤害、受击 / 死亡、显示 |

这批不增加新动画、模型、输入资源或美术预算。Task只在运行期间Tick转向 / 检查上下文，Controller的决策仍复用0.2秒Service和已有事件，不增加全局Tick。正常日志按状态变化 / 一次任务记录，不逐帧打印。

## 2. 参数与树结构先看清楚

### 2.1 距离与时间

| 参数 | 本章最终值 | 含义 |
| --- | --- | --- |
| Pursuit Stop Distance | **90 cm** | 两身体中心水平距离；到这里进入保持距离状态 |
| Pursuit Resume Distance | **100 cm** | 达到这个距离解除保持，允许再次追击 |
| Melee Attack Distance | **100 cm** | 允许开始准备攻击的中心水平距离；不是伤害半径 |
| Melee Retry Delay | **0.5 s** | 一次攻击尝试结束 / 失败后，最早下一次尝试时间；使用世界游戏时间 |
| Task Turn Rate | **360 deg/s** | 只在准备阶段转向 |
| Task Facing Tolerance | **5 deg** | 朝向误差不超过此值才请求能力 |
| Task Preparation Timeout | **1 s** | 转向准备异常时退出，避免永久卡任务 |
| Task Ability Watchdog | **5 s** | 异常安全上限，超过才取消；正常结束依赖能力结束事件 |
| Attack Recovery Wait | **0.6 s** | 正常完成后的树内收招间隔 |
| Idle Or Retry Wait | **0.5 s** | 沿用B2失败 / 无意图等待 |

B2的150 / 200是追击占位参数，不能原样用于当前拳击。C1的真实命中在近处成立；本章改90 / 100，并让攻击范围覆盖迟滞区间。两胶囊半径各42，贴近中心距离约84；此处不扩大25cm Trace、不移动8–14帧窗口。Controller拒绝Melee Attack Distance小于Resume Distance的配置，避免“停止追击却无法攻击”的距离空档。

### 2.2 最终父连接与顺序

```text
ROOT
└─ Selector CombatOrPursuitOrWait
   │  Service: ET Pursuit Decision（原有单份，0.2秒）
   ├─ Sequence MeleeAttackAndRecover（最左，优先级最高）
   │  │  Blackboard: CanMeleeAttack Is Set
   │  │  Notify Observer: On Value Change
   │  │  Observer Aborts: Lower Priority
   │  ├─ ET Melee Attack
   │  └─ Wait AttackRecovery（0.6秒）
   ├─ Sequence PursuePlayer（中间）
   │  │  Blackboard: CanPursue Is Set
   │  │  Observer Aborts: Both（沿用）
   │  ├─ ET Pursuit Move To（Radius读取PursuitStopDistance）
   │  └─ Wait ArrivalSettle（沿用0.25秒）
   └─ Wait IdleOrRetry（最右，沿用0.5秒，直接接Selector）
```

**攻击Decorator必须Lower Priority，不能Both或Self。**能力开始后身体带Attacking Tag，Controller会把CanMeleeAttack改False；Lower Priority能抢占右侧追击 / 等待，但不会因为自己变忙而中止正在运行的攻击或收招。任务另外检查自己的身体 / 目标上下文，受击由既有GA取消结束事件驱动。

## 3. 准备与文件范围

1. UE中Save All，Stop PIE，**关闭UE编辑器**。这次新增UCLASS与反射属性，用正常UHT / UBT构建，不通过Live Coding补反射。
2. CLion打开原工程工作区。新文件直接在指定Private/AI目录创建普通C++文件，不通过UE创建新的BT Blueprint，也不建立另一个CLion工作区。
3. 修改四个既有文件、新建两个文件，均在核心EmberTrial模块。**Build.cs、.uproject、旧Service和旧Move Task不改。**

| 操作 | 完整路径 |
| --- | --- |
| 局部新增 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h` |
| 局部新增 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp` |
| 本章用全文替换头文件 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\AI\ETAIController.h` |
| 局部替换 / 新增函数 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETAIController.cpp` |
| 新建全文（实际路径） | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\AI\ETBTTask_MeleeAttack.h` |
| 新建全文 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_MeleeAttack.cpp` |

实际Task头文件在**Public/AI**，cpp在Private/AI，第一行include `"AI/ETBTTask_MeleeAttack.h"`；本指南现按已验收布局统一。原下发建议两者均Private/AI，但用户创建在Public，6.1记录当时的一行路径补修；无需为目录差异搬迁或创建同名副本。`.generated.h`由UHT生成，不手工创建。

## 4. 当前Character与Controller职责

已完成工程直接阅读Source，不再重复粘贴早期整类 / 完整方法；第7节资产参数仍适用。

| 源码入口 | 阅读重点 |
| --- | --- |
| [Character声明](../EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h)与[实现](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp) | GetLightAttackSpecHandle只返回当前身体授予的句柄；请求 / 取消仍走既有能力入口 |
| [Controller声明](../EmberTrial/Source/EmberTrial/Public/AI/ETAIController.h)与[实现](../EmberTrial/Source/EmberTrial/Private/AI/ETAIController.cpp) | 感知、当前身体订阅、距离 / 重试资格、黑板更新和停树 |
| [共享黑板键](../EmberTrial/Source/EmberTrial/Public/AI/ETBlackboardKeys.h) | 四键名称与资产类型的唯一C++定义，见[黑板契约](AI_BlackboardContract.md) |
| [AI存活检查](../EmberTrial/Source/EmberTrial/Private/AI/ETAILiveBody.h) | Controller与近战Task共用有效对象、非销毁 / 死亡、Ready及有限正生命快照检查 |

GetLightAttackSpecHandle不使用动作Idle作为查询条件，否则自己的攻击开始后就会失去身份。Task还需核对当前Controller / Pawn、感知与BB目标、阵营、ASC与Spec；共享存活检查不能替代这些归属条件。Controller另行要求目标为双向控制关系正确的本地玩家，不能把训练目标当作追击对象。

Controller先提交自身决策状态，再通知黑板观察者；停止阶段保留重入保护与强制停树。类名、反射字段、原生子对象名与当前资源的引用保持。

## 5. 原生BT攻击任务：按生命周期阅读

### 5.1 ETBTTask_MeleeAttack.h当前声明

[打开头文件](../EmberTrial/Source/EmberTrial/Public/AI/ETBTTask_MeleeAttack.h)。配置仍为360 deg/s、5度、1秒准备与5秒异常上限；每个AI实例各有执行身份、阶段时钟和结束订阅，不把可变状态放进共享BT模板。

### 5.2 ETBTTask_MeleeAttack.cpp当前实现

[打开实现](../EmberTrial/Source/EmberTrial/Private/AI/ETBTTask_MeleeAttack.cpp)。与Controller共享的[存活谓词](../EmberTrial/Source/EmberTrial/Private/AI/ETAILiveBody.h)是Core模块Private头，实时读取快照，无缓存 / Tick / 反射字段；不需要修改Build.cs。

| 阶段 / 入口 | 行为与退出 |
| --- | --- |
| ExecuteTask | 释放旧请求，验证配置与资格，记录本轮弱引用和Spec，停移动；等待首次Tick才激活能力 |
| Preparing | 重新核对决策；逐步转向。超时或资格失效退出；朝向达标后先订阅结束事件、锁定Yaw，再发出请求 |
| WaitingForAbility | 固定发招朝向；有效目标侧移不会让攻击追着转弯。正常依赖自身Spec结束事件，Watchdog只处理异常 |
| HandleAbilityEnded / FinishAttempt | 只处理自身Spec；同步结束暂存至请求返回。通知Controller后复核执行序号，再释放与结束BT任务 |
| ReleaseRequest / AbortTask / OnTaskFinished | 先撤销本轮身份，再按自身Handle解绑 / 取消；兜底重复调用安全，保留Super结束语义 |

### 5.3 需要解释清楚的边界

- ExecutionSerial用于识别同步回调是否已经中止或替换本轮任务；它不是可删的重复检查。Request返回与Controller更新决策都可能发生重入。
- 暂停不累积游戏Delta；恢复后继续原阶段，不把暂停时间补到准备 / Watchdog。
- 能力拒绝时不取消别的已有能力；只有本任务取得取消权且句柄仍有效时才取消该Spec。结束订阅只移除自己的Handle。
- FinishLatentTask可能立即启动下一轮，同一调用栈中其后不得再写任务状态。
- 受击 / 死亡与目标身份失效继续沿原能力和上下文路径收尾；本次清理不改距离、重试、动画、伤害或BT资产。

## 6. 正常构建与CLion索引（你执行）

### 6.1 已遇到的头文件路径错误与一行补修（2026-10-09）

首次构建日志的具体错误是`ETBTTask_MeleeAttack.cpp(1): fatal error C1083`，找不到`ETBTTask_MeleeAttack.h`。实际头文件在`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\AI\ETBTTask_MeleeAttack.h`，cpp在Private/AI，二者不同目录；原第一行的同目录写法与当前布局不匹配。UHT已通过，日志中的`UBT failed with exit code 6`只是脚本汇总，不是根因。

**本次只需手动修改cpp第一行**，保留现有Public头文件：

```cpp
#include "AI/ETBTTask_MeleeAttack.h"
```

其他include与代码保留。不要移动 / 复制文件、修改Build.cs或删除缓存。保存后保持UE关闭，重新执行下面的Refresh-CLion脚本；Editor成功后再继续索引与Game Build。后续Commit / Push使用实际Public/AI头文件路径，不再增加Private同名副本。本次只是读日志并提供补修，尚未替用户修改源码或重编译。

### 6.2 正常构建顺序

确认六个文件保存、UE已关闭。CLion Terminal / PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先正常构建EmberTrialEditor再生成索引；若失败，先停在第一条具体UHT / C++错误，不继续配置资产。成功后CLion **Reload CMake Project**，确认核心CodeModel包含新的Private/AI任务，两份文件不显示“不属于Target”，引擎BT / GAS头能导航。

然后执行正常Game Development构建：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

本章预计工程源码由51份变为**53份、26个cpp**；这是完成后的预期，不是下发时已经新增。CLion CMake用于代码模型，实际UE构建仍由UBT执行。保持既有模块依赖，不通过修改引擎 / 删除全部缓存解决include。

## 7. UE资产配置（逐项完成后再Play）

### 7.1 Blackboard增加第四个键

打开已有 **`/Game/EmberTrial/AI/Blackboards/BB_ETMeleeEnemy`**，进入Blackboard页。保持原TargetActor / CanPursue / PursuitStopDistance。

1. Blackboard面板点击 **New Key → Bool**。
2. 选中新增键，Entry Name输入 **`CanMeleeAttack`**，大小写与代码完全一致。
3. Key Type确认Bool，**Instance Synced不勾选**。不要把四个键设同步，避免以后多敌人共用意图。
4. 检查原TargetActor仍Object、Base Class=Actor；CanPursue=Bool；PursuitStopDistance=Float。不要新建第二份黑板。
5. Save。这一页没有Blueprint Compile按钮，保存即可。

### 7.2 Controller Blueprint四个距离 / 重试参数

打开 **`/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController`** → **Class Defaults** → 搜索`EmberTrial` / 参数名，配置：

| Details字段 | 赋值 |
| --- | --- |
| Enemy Blackboard Asset | 上一步原BB_ETMeleeEnemy |
| Enemy Behavior Tree | 原BT_ETMeleeEnemy |
| Pursuit Stop Distance | **90** |
| Pursuit Resume Distance | **100** |
| Melee Attack Distance | **100** |
| Melee Retry Delay | **0.5** |

**旧Blueprint可能保留150 / 200覆盖值，不能只改C++默认值。**四项都手动核对；如果UI显示单位，仍输入90 / 100 / 100 / 0.5。Compile → Save。Event Graph不新增任何节点，EnemyPerception仍原单份。

### 7.3 改原BT的Selector顺序，不重建整棵树

打开 **`/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy`**，确认Blackboard Asset是7.1的原BB。先看原B2树：ROOT下的Selector带单份ET Pursuit Decision；原追击Sequence和IdleOrRetry都直接接Selector。

**A，给原Selector改Node Name：**选Selector，Details的Node Name填`CombatOrPursuitOrWait`，只是便于辨认。原Service保留，Interval=0.2、Random Deviation=0；不在新Sequence上再加一份Service。

**B，从原Selector新增最左Sequence：**从Selector底部连接点拖一条新线到空白处，选择Composite **Sequence**，放在原追击Sequence左侧。选中新增Sequence，Node Name=`MeleeAttackAndRecover`。新线的上端必须来自Selector，不能来自原追击Sequence、ROOT或Move任务。

**C，在新Sequence上加Blackboard Decorator：**右键**新Sequence节点本体** → Add Decorator → Blackboard。选中贴在新Sequence上的Blackboard条目，Details按下表赋值：

| 字段 | 精确设置 |
| --- | --- |
| Blackboard Key | **CanMeleeAttack** |
| Key Query | **Is Set**（Bool为True） |
| Notify Observer | **On Value Change** |
| Observer Aborts | **Lower Priority** |

不要把Decorator放在Task上；不要选Both / Self。原追击Sequence的CanPursue Decorator保持B2的Is Set / **On Result Change** / **Both**。两者都是Bool，本章新增攻击分支使用On Value Change，原追击通知设置无需改动。

**D，新Sequence的第一个子Task：**从**新Sequence底部**拖线到空白处，在任务列表搜索`ET Melee Attack`（也可搜`MeleeAttack`），选择这次新编译的**原生Task**。放在新Sequence左下方，保持Node Name可辨认。选任务，Details搜索以下字段：

| Task字段 | 最终值 |
| --- | --- |
| Turn Rate | 360 |
| Facing Tolerance | 5 |
| Preparation Timeout | 1 |
| Ability Watchdog | 5 |

任务无Blackboard Key字段，目标由原生Controller / BB身份校验取得；不要额外生成BTTask Blueprint。

**E，新Sequence的第二个子Task：**再次从**同一个新Sequence底部**拖第二条线，创建引擎 **Wait**，放在ET Melee Attack右侧。Node Name=`AttackRecovery`，Wait Time=**0.6**，Random Deviation=**0**。若Details展开BB绑定选项，Wait Time的Blackboard Key保持None；使用常量0.6。

ET Melee Attack与AttackRecovery是**同一Sequence的两个子节点**，不是把Wait接在Task下面。成功时先执行左Task，再执行右Wait；失败不会执行本Sequence的收招Wait，Controller最早重试门控与Selector右侧等待负责退路。

**F，保留原追击与失败等待：**原Sequence放在Selector中间；其两个子任务仍ET Pursuit Move To → ArrivalSettle Wait0.25。Move To的Acceptable Radius继续绑定 **PursuitStopDistance**，而不是绑定CanMeleeAttack；绑定启用时灰色常量150不是真正运行值，不需要为它解除绑定。原IdleOrRetry Wait0.5放在Selector最右，**直接接Selector**；不放进任一Sequence，不改为AttackRecovery。

**最后从父节点沿线数：**Selector恰好**三个**子节点：左攻击Sequence、中追击Sequence、右IdleOrRetry；攻击Sequence恰好**两个**子Task；追击Sequence恰好**两个**子Task。UE执行优先级按从左到右的节点顺序，必要时拖动位置后确认角上的执行序号也先攻击、后追击、最后等待。保存BT。

### 7.4 敌人装配只核对旋转设置，保留C1资源

打开 **`/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy`**。

1. Class Defaults：AI Controller Class仍BP_ETMeleeAIController，Auto Possess AI保持B2配置；Light Attack Ability Class仍GA_ET_EnemyLightAttack，Combat Team=Enemy，Initial Health / Max Health=100 / 100。
2. Class Defaults搜索`Use Controller Rotation`：Pitch / Yaw / Roll均False。
3. 选CharacterMovement：Orient Rotation to Movement=True，Use Controller Desired Rotation=False；Rotation Rate的Yaw=360，Max Walk Speed仍220，地面制动保持既有2000。
4. Mesh仍Quinn / ABP_Quinn / 原DefaultSlot，Mesh NoCollision；胶囊Visibility Overlap、世界与Pawn阻挡保持既有配置。原单ASC / CombatTrace / CombatLife保留，没有Practice / 玩家业务。
5. CombatTrace：hand_r、25cm、Draw Debug=False。不要添加重复组件或修改旧Montage窗口。

若没有改值，保存无需制造额外资产变化；若调整了覆盖配置，Compile → Save并在最终Push清单中包含敌人BP。原GA_ET_EnemyLightAttack保持ServerOnly / Attack Damage10 / Damage Effect Class=GE_ET_LightAttackDamage / 原攻击Montage；玩家GA25和Neutral训练目标保持不动。

### 7.5 地图与首次运行前

打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`，保留原单个敌人、训练目标、导航体和PlayerStart。导航覆盖双方，无临时墙或无路径修改。选地图敌人实例，若对旋转 / 速度有黄色覆盖值，核对与7.4一致；不要改玩家默认身体或在关卡蓝图加测试逻辑。Save All。

首次Play后先确认`Blackboard ready ... Stop=90.0 Melee=100.0`和原`Tree start requested ... Accepted=1 Stop=90.0 Resume=100.0`。如果出现Invalid decision keys / Tree config rejected，先停下来核对四键和Controller四项值，不能把“敌人不攻击”当成动画问题继续调Trace。

## 8. 四组分层检查（合计约10–15分钟）

使用现有敌人、控制台`ke / slomo`、原左键 / 3 / 4 / E / P。日志由我按你报告的**完整时分**读取；你无需手算间隔或捕捉亚秒窗口。本章不重搭UI / 换控 / 属性测试图，不重复三帧率 / 打包矩阵。

### 8.1 自动追击 → 转向 → 真实攻击（约3–4分钟）

**准备：**新Play，玩家 / 敌人100，slomo=1。走入敌人视线；不使用ke RequestLightAttack，确认攻击确实来自BT。

**操作：**玩家在敌人前方站定，观察敌人追来、停步并攻击。保持站定让它命中两次，玩家HUD100→90→80。随后侧移，观察收招后它重新准备朝向；不要把玩家走到视野背面丢失目标的一次取消当成“朝向固定打空”。

**预期：**一次流程中能找到如下日志，Body / Character应是敌人：

```text
[M1-05C2] Prepare Body=BP_ETMeleeEnemy... Target=BP_ThirdPersonCharacter... Handle=...
[M1-04A] Begin Character=BP_ETMeleeEnemy...
[M1-05C2] Requested Body=BP_ETMeleeEnemy... LockedYaw=...
[M1-04B2] Damage ... Requested=10.0 Applied=1 ... Before=100.0 After=90.0 ...
[M1-05C2] Finish Body=BP_ETMeleeEnemy... Result=Succeeded Reason=AbilityEnded
[M1-05C2] Melee retry ... Delay=0.50 ...
```

这是字段示意，完整输出以既有日志格式为准。Begin可以早于Requested；Finish前后可能穿插End / Tag / Trace日志，不要求固定行序。自己的Busy / CanMeleeAttack=False不应立刻Abort攻击；正常攻击的Finish来自AbilityEnded，不来自Watchdog。第二次命中变80，原玩家受击 / HUD事件各自处理。

**朝向 / 空挥一次：**控制台输入`slomo 0.1`，看到敌人进入攻击前摇后关闭控制台，向侧方移出拳路径；尽量仍保持敌人正面视野内。已接受的这一次拳方向保持，没Candidate则不扣血。若输出SightLost / ContextLost，这次证明目标失效取消，不能当作空挥；无需精确追逐窗口，告诉我实际现象即可。下一次准备会重新转向，不永久固定旧朝向。

**清理：**输入`slomo 1`，Stop PIE。新Play恢复100。

### 8.2 活动攻击被受击取消、恢复后再决策（约2–3分钟）

**准备：**新Play，走近让敌人自动攻击；控制台输入`slomo 0.1`。这一组只需攻击已开始，不要求在8–14帧的指定瞬间操作。

**操作：**看到敌人攻击前摇 / Begin后，在控制台执行一次：

```text
ke BP_ETMeleeEnemy_C RequestHitReact
```

**预期：**敌人原轻攻击Cancelled=1、Trace Disarm / Tick0、任务Finish Failed Reason=AbilityCancelled（同步结束时可为SynchronousCancelled），随后受击。不会因HitReact结束错把另一份能力结束当攻击成功；受击中不启动新攻击。等待受击结束、恢复`slomo 1`，敌人重新追击或自动攻击；没有旧订阅重复Finish。

如果命令下得太晚，已先输出正常AbilityEnded，只再观察下一次自动攻击调用一次即可；不为这项重建按键 / 函数。另在正常速度下，玩家左键对敌人真实命中一次，敌人受击、生命减25，自己的HUD不因敌人扣血变化，随后敌人能恢复决策。不是必须与敌人拳发生在同一帧。

**清理：**slomo1，Stop；下一组初始100。

### 8.3 能力拒绝时有限重试、恢复配置后正常（约2–3分钟）

**准备：**Stop PIE，打开`/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack` → Class Defaults。记住Damage Effect Class原值 **GE_ET_LightAttackDamage**。仅临时把这一字段设None，Compile → Save；Attack Damage10、Montage与其他字段不改。

**操作：**新Play，走近让敌人尝试攻击；站定观察约3–4秒现实时间，足够产生至少三次拒绝。然后Stop。

**预期：**Request Accepted=0 / Task Finish Failed Reason=AbilityRejected，无合法伤害；控制台不会每帧刷几十次请求。每次尝试结束会输出Melee retry Delay=0.50，下一次Prepare / Rejected至少隔约0.5秒游戏时间，实际会受Service和Wait调度变长。BT右侧等待 / 追击退路正常，没有卡死节点。**不用你计算时间差，我读取日志核对。**负例的拒绝 / Warning是本组预期。

**立即恢复：**Stop后把同一GA Damage Effect Class改回 **GE_ET_LightAttackDamage** → Compile → Save。新Play敌人自动攻击、真实扣10，再Stop。本章不保留None配置，不为负例创建第二份GA。若连Prepare都没有，先检查范围 / BB门控；不能把“从未尝试”算有限重试通过。

### 8.4 暂停、生命中止、结束与短基线（约3–5分钟）

**准备：**已经恢复GE、slomo1，新Play100。以下小流程可以用新的PIE轮次，不要求在同一轮承受全部伤害。

1. 敌人攻击前摇时P暂停，世界 / 动画冻结。鼠标继续恢复一次；再P，以Enter或Space继续一次。菜单期间的左键 / 3 / 4不补执行，恢复没有额外伤害 / 重复Requested，移动转向正常。暂停不消耗重试、转向准备或Watchdog游戏时间。
2. **目标死亡：**近处让敌人自动命中，配合原4的Damage25让玩家归零。看到TargetDead清目标，当前敌人Task退出，攻击关闭，无后续对尸体追击 / Requested。HUD0、P / 镜头仍可用。无需再做死亡治疗 / Base矩阵。
3. **自身死亡：**新Play，在近处用玩家左键四次合法25点命中杀死敌人；玩家可以移动避拳。敌人BodyDead停止树 / 任务，未结束的自身攻击清理，不再自动攻击。玩家生命与HUD仍属于自己。D再做完整组合交战证据，本项只查新Task不会悬挂。
4. **停止重开：**新Play一次敌人攻击运行中Stop PIE；下一次Play双方100，没有旧Task Finish、旧Montage / Trace残留。最后移到安全距离做E-A/B一次、P一次、移动 / 转向 / 跳跃短基线；不重新验十次HUD重建等未改入口。

生命失效可以走ContextLost，也可以因既有GA取消走AbilityCancelled，StopTree可走Abort；依据目标 / 身体与Trace清理判断，不能要求所有路径都打印同一个Reason。结束后不能再有旧对象的Requested / Finish重复。

**最终清理：**Stop PIE；slomo恢复1；玩家 / 敌人默认100/100、阵营Player / Enemy、伤害25 / 10；敌人GA的Damage Effect Class已恢复；Trace Debug=False；无新增测试图、临时实例 / 墙 / 输入，导航正常。修改过的Blueprint Compile → Save，BB / BT / 地图Save All。

## 9. 问题反馈与局部定位

| 现象 | 先核对 / 发给我的材料 |
| --- | --- |
| UHT / Build失败 | 第一条具体错误与前后5–10行文字；新头generated.h最后、只一份同名类；include正确是Abilities/GameplayAbilityTypes.h |
| ET Melee Attack找不到 | 新Task Private/AI两文件、正常Editor Build成功、UE重新打开；不要用旧UE会话 / 新建Blueprint掩盖原生类未加载 |
| Blackboard / Tree config rejected | 四键名称类型、Controller90 / 100 / 100 / 0.5、同一BB与原BT；发Defaults / BB截图 |
| 走到近处不攻击 | Melee gate日志Reason、Body Ready / Team / GA Class；旧Controller BP仍150 / 200会导致新配置校验失败 |
| Prepare后马上取消，反复开始 | 攻击Sequence的Observer Aborts必须Lower Priority；发整棵BT与Decorator Details，不只截图Task |
| Wait跳过 / 没恢复追击 | Selector三个孩子、两个Sequence各两个任务；IdleOrRetry直接Selector。截图必须看见所有父线与执行序号 |
| Requested后正常结束前卡住 | Begin / End / Finish日志片段、Montage引用 / Slot；5秒Watchdog触发是故障，不当正常完成验收 |
| 敌人朝向异常 | 敌人三个Use Controller Rotation False、Movement Orient=True / Desired=False，检查实例覆盖；提供10–20秒视频与Task参数 |
| Begin但不命中 | 先看距离 / 朝向与Candidate；确认90 / 100和碰撞原值。可临时敌人Draw Debug=True拍短视频后恢复，不擅自改窗口 / 半径 |
| 每帧请求 / 拒绝 | MeleeRetryDelay0.5、Notify与Task Finish / Abort实现、世界时间日志；发完整文本，不调大Damage绕过 |
| 停止后旧对象有回调 | Task Release是否先解绑再Cancel、Handle过滤 / Serial保护是否齐全；发Stop前后日志，明确最后操作 |

纯代码 / 日志报错优先文字；BB / BT连线与配置用截图；动画 / 时序问题用短视频。普通报错先由我读代码 / 日志定位并给局部修正；你明确要求接管后我再直接修改或操作。

## 10. 完成反馈与核查边界

```text
M1-05C2 完成
四个既有 / 两个新源码；Editor Build / 索引26cpp / Game Build：
BB四键非同步 / Controller90-100-100-0.5 / BT父线与两Decorator：
自动追击-转向-攻击，两次10伤害 / HUD100→90→80：
攻击期间方向固定 / 侧移空挥或实际取消现象：
活动攻击受击取消 / Trace清理 / 受击结束恢复：
玩家左键真实25与敌人受击，玩家HUD隔离：
None GE三次有限重试 / 原GE恢复后自动伤害：
暂停鼠标 / 键盘继续无补播 / 两种死亡停止：
动作中Stop重开 / E-P与基础输入短基线：
slomo1、100/100、25/10、GE恢复、Debug False、Save All：
失败或特别说明：
大致完成时间（完整时分，例如21:18）：
实际用时：代码__ / 资产__ / 检查__；构建等待__分钟
```

完成后先反馈，我读取最新源码 / Build receipt / 索引与日志，更新本章验收记录、资产哈希和Progress等；通过后你按最后一节Commit / Push。若遗漏某个专项只补该项，不重做整章或旧UI矩阵。当前仍51Source / 25cpp / 清单40，累计手动1486 / 等待115分钟，没有提前登记新的Task / 实耗。

本指南已按本机UE5.5.4接口静态核对：OnAbilityEnded传入FAbilityEndedData，包含AbilitySpecHandle / bWasCancelled；CancelAbilityHandle只取消指定Handle；BT Task的Execute / Tick / Abort / OnTaskFinished与GetTaskStatus / FinishLatentTask可用，Forced Stop走任务中止清理。这里是接口与逻辑审查，**尚未编译这份待手动实现的代码，也没有实机通过结论**。

### 10.1 历史实施收尾（2026-10-10）

本章已验收：用户15:20反馈无失败，手动35 / 等待5分钟。历史日志记录真实伤害、活动攻击取消后恢复、死亡 / 暂停清理，以及None GE有限重试；具体统计保留在[原验收报告](Reports/M1_05C2_Verification.md)，不当作当前清理重新执行的结果。

本轮第17批已整理近战Task、合并两处相同存活谓词，并核对提取前后的有效Token。构建与回归完成后统一记录在[Cleanup](Cleanup.md)；资产Valid验证结构，不能替代运行时交战矩阵。

## 11. 本章需要Commit / Push的内容（固定放在文档最后）

本章历史功能及资源已提交，不按旧清单重复制造差异。当前第17批实际变动为四份Source与本指南 / 黑板契约 / Cleanup / Progress，共八项；逐文件清单及最终验证见[Cleanup末尾](Cleanup.md)。

复盘时可定位既有BB_ETMeleeEnemy、BP_ETMeleeAIController与BT_ETMeleeEnemy，分别在Content/EmberTrial/AI/Blackboards、Controllers与BehaviorTrees。只有实际改变并恢复最终配置的可编辑资产才需提交，本批未改这些资源。

用户在CLion刷新差异，重新Stage最新修改与新增Private头文件，再Commit / Push现有远端；具体步骤见[版本管理](VersionControl.md)。Raw、构建日志、测试报告、CMake索引与本机设置继续留本地。助手不操作Git索引、Commit或Push。
