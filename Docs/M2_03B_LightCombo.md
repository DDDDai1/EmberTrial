# M2-03B：三段轻攻击、单槽输入缓冲与独立命中

2026-10-18清理同步：本页保留历史配置、接入差异和验收依据，重复整文件以当前Source链接为准。Attack Window的Begin / End共用私有Route，窗口归Trace管理；Combo Window仍转发给当前攻击Ability，两种职责不混合。不要把历史步骤整批重做；本次范围、验证与提交见[Cleanup](Cleanup.md)。

2026-10-16，固定 **UE5.5.4**。**本批已按用户13:26反馈与源码 / 构建 / 日志核对验收通过，无待补项。**实际99Source / 52cpp / 来源58项，手动50 / 等待5；[B报告](Reports/M2_03B_Verification.md)记录两Success、正式三段各25与旧共用AM重存差异。下方原操作步骤保留供复查，无需重复执行；下一[C指南第2节](M2_03C_RecoveryDodgeCancel.md#2-准备与文件清单)。

原下发预算（非当前待办）：手动 **120–210分钟**：代码 / 理解80–145、资产装配20–35、相关检查10–15、保存记录5–10；构建等待另列，可分两次完成。预算不计实耗，不要求做满。

## 1. 本批结果与责任

| 输入 / 状态 | 本批结果 |
| --- | --- |
| 空闲、地面站定单击左键 | 第1段一次25，正常结束 |
| 第1 / 2段约.48秒再单击 | 输入最多存1条，有效.25世界秒；到.60秒连段窗消费，进入下一段 |
| 连段窗.60–.80秒内新输入 | 接受一次后延后至Timer调度点切段，避免在Notify调用栈里替换Montage |
| 太早按，例如开始后.05秒 | 到窗前过期，不推进 |
| 多次点击挤在同一段 | 只更新这一条输入，不储存两次未来攻击；不能靠第一段的一串点击自动排满三段 |
| 第3段再点 | 不排第4段，结束后重新按才从1开始 |
| 长按 | Started只发生一次，不自动连击 |
| 整条链 | 一个能力、Attacking持续；每段独立Trace Serial / SeenTargets，同目标每段最多一次 |
| 受击 / 死亡 / Stop | 清自己的Trace、Task、输入与待切段Timer；旧回调不能影响新动作 |
| P暂停 | 世界时间和缓冲有效期冻结；菜单输入拒绝；暂停前已合法缓存的输入可随原动作恢复后消费 |
| 攻击中按2 | **本批仍拒绝**；恢复阶段闪避取消留C，不提前删Dodge的Attacking门控 |

玩家每段仍25，三次正常命中100→75→50→25；不额外消耗体力。敌人仍原单段10，原GE / 无敌提交门控保留。数字1仍是现有攻击请求别名，不新增第二份能力或临时测试按键。

**素材隔离：**A只新增2 / 3段；B现在新建`AM_ET_Light_01`供玩家使用，让旧`AM_M0_Attack_01_Montage`继续服务敌人、Practice和Dodge真实命中测试。后续B的玩家1 / 2段添加连段NotifyState，第3段没有该窗口；不修改旧共用Montage。

**实现分工：**Character负责把活动能力的输入交给同一实例；Ability拥有当前段 / 代次、单条带到期时间的输入、Montage Task和唯一待切段Timer；NotifyState只转发带Mesh / Montage实例 / Token的事件，不在共享通知对象里保存某个角色状态。Trace既有BeginAttack / EndAttack已支持独立Serial，本批不改它。

Timer只为离开当前通知调用栈后切段，不按动画时长解锁。UE5.5默认SetTimerForNextTick可能在同一世界帧后面的Timer调度阶段执行，**不保证一定跨过一个渲染帧**；代码依赖延后调度和激活 / 段代次，而不是屏幕帧号。End先使旧身份失效、解绑旧Task的动态回调和属于该Task的原生Montage委托，再停止自己的旧Montage。Task执行EndTask完成正常解绑后，才清空这份已结束Task的Ability指针；即使动画队列保留旧原生委托的副本，也不能用它清掉新一轮相同Montage的ASC归属。停止动作可能同步回调，前后都复核激活 / 段代次；不修改引擎代码或仍活动Task的归属。

本机接口按Engine的AbilityTask_PlayMontageAndWait.cpp、TimerManager.cpp、AnimInstance.h核对，参考[Epic 5.5 Montage Task](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/Abilities/Tasks/UAbilityTask_PlayMontageAndWait?application_version=5.5)与[Notify说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine?application_version=5.5)。静态核对不代替本次实际编译 / 运行。

## 2. 准备与文件清单

Stop PIE → Save All → 关闭UE → CLion保存。先完成第3–6节全部九份Source，再第7节正常Build；不使用Live Coding处理反射结构变更。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Animation\ETANS_ComboWindow.h                 新
├─ EmberTrial\Private\Animation\ETANS_ComboWindow.cpp              新
├─ EmberTrial\Tests\ETLightComboFlowTests.cpp                       新
├─ EmberTrial\Public\Abilities\ETGA_LightAttack.h                  旧，完整替换
├─ EmberTrial\Private\Abilities\ETGA_LightAttack.cpp               旧，完整替换
├─ EmberTrial\Private\Characters\EmberTrialCharacter.cpp          旧，只替换RequestLightAttack
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h             旧，只加一行friend
├─ EmberTrial\Public\Combat\ETCombatLifeComponent.h               旧，只加一行friend
└─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp 旧，只插入一段
```

不改Build.cs / Target / uproject / Trace / Dodge / UI / GameMode或存档；新测试在既有Tests目录，不是UCLASS。`FETLightAttackStage`结构体放现有GA头文件，保留原AttackMontage / Damage等属性名称，避免已有蓝图引用失效。不创建新的GA类、Actor组件或DataAsset。

## 3. 新建无状态的连段通知（两完整文件）

### 3.1 ETANS_ComboWindow.h

路径见第2节Public/Animation，完整内容：

当前源码：[ETANS_ComboWindow.h](../EmberTrial/Source/EmberTrial/Public/Animation/ETANS_ComboWindow.h)。

通知保持原生Branching Point与禁止编辑器预览触发；仅路由本次Mesh、Sequence、Montage实例ID、Token和开关，不缓存身体状态。

### 3.2 ETANS_ComboWindow.cpp

路径见Private/Animation，完整内容：

当前源码：[ETANS_ComboWindow.cpp](../EmberTrial/Source/EmberTrial/Private/Animation/ETANS_ComboWindow.cpp)。

通知保持原生Branching Point与禁止编辑器预览触发；仅路由本次Mesh、Sequence、Montage实例ID、Token和开关，不缓存身体状态。

## 4. 完整替换 LightAttack 两文件

### 4.1 ETGA_LightAttack.h

完整覆盖该头文件。原类名 / API / 四个旧配置属性保留；两行friend是临时世界测试访问，不是蓝图装配节点。新增组合配置默认关闭，只有后面玩家GA显式启用。

当前源码：[ETGA_LightAttack.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h)。

当前声明包含段配置、三层身份与私有玩家段上下文查询；资产字段与公开请求接口保持。

### 4.2 ETGA_LightAttack.cpp

完整覆盖该cpp，不再另外粘贴旧Activate / End或第二份HandleTraceHit。伤害函数继续使用当前Health GE / 阵营 / Ready / 无敌提交保护；只把本段请求值改为当前段缓存伤害。

当前源码：[ETGA_LightAttack.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp)。

当前实现统一两个玩家请求的身份检查，各自保留连段资格和恢复窗口门控；伤害与资源清理仍按现有链路执行。

## 5. 其他四个既有文件：精确局部修改

### 5.1 Character.cpp：完整替换 RequestLightAttack

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp`。只替换从`bool AEmberTrialCharacter::RequestLightAttack()`到它对应结尾大括号；下一函数`GetLightAttackSpecHandle()`及其后保持。

```cpp
bool AEmberTrialCharacter::RequestLightAttack()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !LightAttackAbilityHandle.IsValid())
    {
        UE_LOG(LogETAbilityFoundation, Display,
            TEXT("[M1-04A] Request rejected Character=%s Reason=NotReadyOrNotGranted"),
            *GetNameSafe(this));
        return false;
    }
    const FGameplayAbilitySpec* Spec =
        AbilitySystemComponent->FindAbilitySpecFromHandle(LightAttackAbilityHandle);
    if (!Spec)
    {
        UE_LOG(LogETAbilityFoundation, Display,
            TEXT("[M1-04A] Request rejected Character=%s Reason=MissingSpecOrBusy"),
            *GetNameSafe(this));
        return false;
    }

    if (Spec->IsActive())
    {
        UETGA_LightAttack* Attack=Cast<UETGA_LightAttack>(Spec->GetPrimaryInstance());
        const bool bBuffered=IsValid(Attack) && Attack->TryBufferComboInput();
        UE_LOG(LogETAbilityFoundation,Display,TEXT("[M2-03B] Active attack input Character=%s Accepted=%d"),
            *GetNameSafe(this),bBuffered ? 1 : 0);
        return bBuffered;
    }

    const bool bAccepted = AbilitySystemComponent->TryActivateAbility(
        LightAttackAbilityHandle, false);
    UE_LOG(LogETAbilityFoundation, Display,
        TEXT("[M1-04A] Request Character=%s Accepted=%d"),
        *GetNameSafe(this), bAccepted ? 1 : 0);
    return bAccepted;
}
```

活动输入的Accepted=1表示已接受一条缓冲，不等于立即进入下一段或造成伤害；没有对活动Spec重复TryActivate。旧空闲入口 / Ready / 死亡 / 结束保护保留。

### 5.2 Character.h：只加一行friend

在该文件已有以下行后面：

```cpp
friend class FETDodgeInvulnerabilityTest;
```

紧接着新增：

```cpp
friend class FETLightComboFlowTest;
```

保留Stamina / Dodge等全部旧friend、属性和函数。

### 5.3 CombatLife.h：同样只加一行friend

在`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Combat\ETCombatLifeComponent.h`的private区，已有`friend class FETDodgeInvulnerabilityTest;`下一行新增同一个`friend class FETLightComboFlowTest;`。不改受击类属性、死亡逻辑或cpp；测试用它给临时身体选择正式受击类。

### 5.4 Editor Validator.cpp：插入连段规则

在`ValidateAbility`中找到**唯一**一行：

```cpp
    if (!bAttack) { return; }
```

用下面完整段落替换这一行；它后面的`const TOptional<double> Damage = ...`等旧数值 / GE校验继续保留，前面Dodge分支完全保留。

```cpp
if (!bAttack) { return; }
    const UETGA_LightAttack* Attack=CastChecked<UETGA_LightAttack>(Defaults);
    if (!Attack->HasValidComboConfiguration())
    {
        AddIssue(Blueprint,TEXT("ET-COMBO-001"),
            TEXT("Combo requires two distinct follow-ups, finite buffer .05...50, same skeleton / DefaultSlot / no root motion, one hit state each, one later combo state only on stages 1/2 before blend-out."),Issues);
    }
    const FString Package=Blueprint->GetOutermost()->GetName();
    if ((Package==TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack") && !Attack->IsComboEnabled()) ||
        (Package==TEXT("/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack") && Attack->IsComboEnabled()))
    {
        AddIssue(Blueprint,TEXT("ET-COMBO-002"),TEXT("Formal player requires Combo enabled; formal enemy must keep Combo disabled."),Issues);
    }
```

该cpp原本已include ETGA_LightAttack，不重复添加。ET-COMBO-001共用Runtime谓词，ET-COMBO-002明确玩家启用 / 敌人禁用；原Validation Probe路径不强制启用玩家连段，旧负例规则保留。

## 6. 新增长期真实连段自动化

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETLightComboFlowTests.cpp`。完整内容如下。

测试加载保存后的玩家 / 敌人GA与真实Montage，临时目标生命1000，不改变地图 / 正式默认值。每次Tick把目标放在当前右手附近以隔离几何距离变化，仍由真实Notify / Sweep / GE提交命中；这验证时序 / 去重 / 伤害链，不冒充敌人AI或完整手感。没有伪造FHitResult、直接调用伤害回调或手动广播成功通知；仅故意发送旧窗口结束 / 旧Serial清理作为负例。

精确时机由有上限的Until观察实际Montage位置 / 段索引安排，不要求你赶.25秒，也不依赖.16秒这类紧贴Timer边界的断言。暂停、待切段受击 / 死亡 / EndPlay、旧段清理和五次输入只存一条都在测试中驱动。另用强引用保留已结束Task，在新一轮使用同名Montage时投递旧回调，检查新ASC归属和Trace未被清空；这与旧窗口 / Serial清理同属负向注入，不伪造正向命中。代码不先写成Success，需完成第8节资产装配后实际运行。

当前源码：[ETLightComboFlowTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp)。

现有真实连段 / 伤害 / 旧回调隔离场景保留；新增实例身份、输入禁用与暂停对两个请求的一致拒绝检查。

## 7. 正常 Build、索引与 Game Build

1. 确认九份文件保存，UE保持关闭。PowerShell依次执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

2. 该脚本先Editor Build，再生成CLion索引。成功后在CLion Reload CMake Project，核对Core / UI / Editor三个CodeModel；预计 **36 + 13 + 3 = 52cpp**。新Notify cpp和测试cpp属于Core；UHT / 自动生成.cpp不计入这个手写源码数。
3. 若出现CMake版本 / 目标问题，按既有[CLion创建类指南](CLion_UEClassCreation.md)处理，不删全项目缓存。新头文件最后一个include必须是本文件.generated.h。
4. 再执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

5. 两目标成功再开原.uproject。此时玩家GA还未配置启用连段，不能据此判断新功能失败；继续第8节。失败时停在第一条真正C++ / UHT错误，文字贴文件 / 行号及上下5–10行，`UBT exit code6`本身不是根因。

## 8. 资产装配：先Montage，再GA

### 8.1 新建独立第1段 Montage

1. Content Browser进入`/Game/EmberTrial/Animations/Combo/Montages`，空白处右键 → **Animation → Animation Montage**；选择Skeleton **SK_Mannequin**，命名 **AM_ET_Light_01**。不要选Create Child Montage，也不覆盖旧AM_M0_Attack_01_Montage。
2. 打开新AM，在编辑器右侧Asset Browser搜索 **AS_M0_Attack_01**，核对来源`/Game/EmberTrial/Animations/Practice/Clips`；拖到新Montage已有的DefaultSlot轨道，放在时间0。只一个Segment，不把AS_ET_Light_02拖作第1段。
3. 单击Segment，在Details核对Animation Reference=AS_M0_Attack_01，Start Time=0，End Time=该AS全长约1.033333，Play Rate=1，Loop Count=1；Slot Name=DefaultGroup.DefaultSlot。保留一个Default Section，不接回自身形成循环。
4. Montage Asset Details：Rate Scale=1、Enable Auto Blend Out=True，Blend In → Blend Time=.10，Blend Out → Blend Time=.10。Preview Mesh选Quinn_Simple，预览原前挥正常；不更改旧AS的Root Motion或玩家AnimBP。
5. 新建ET_HitWindow通知轨道，在轨道空白右键 **Add Notify State → ET Attack Window**，只一条。右键状态条本体，`Notify Begin Time=.266667` Enter，再右键`Anim Notify State Duration=.200000` Enter；开始帧8 / 持续帧6 / 结束帧14。Montage Tick Type=Branching Point，Save。

### 8.2 玩家第1 / 2段各加一个连段窗口

对下表两份AM分别做同样步骤：

| Montage | 原命中状态 | 新连段状态 |
| --- | --- | --- |
| Combo/Montages/AM_ET_Light_01 | ET Attack Window：8–14帧，保留 | ET Combo Window：18–24帧 |
| Combo/Montages/AM_ET_Light_02 | A已配的8–14帧，保留 | ET Combo Window：18–24帧 |
| Combo/Montages/AM_ET_Light_03 | A已配的6–12帧，保留 | **没有连段窗口** |

1. 在AM的Notifies下增加另一条Notify Track，命名 **ET_ComboWindow**，避免与命中状态重叠看错。
2. 轨道空白约.60秒处右键 → **Add Notify State → ET Combo Window**。这是第3节新原生类，不选ET Attack Window或普通Notify；找不到类先查本次Build，不临时新建蓝图代替。
3. 单选新状态条，右键**中间本体**：`Notify Begin Time=.600000`，Enter；再右键`Anim Notify State Duration=.200000`，Enter。复核`Notify Frame=18`、`Anim Notify State Frames=6`；Duration不是End，不填.80。
4. 在Details把Montage Tick Type设为Branching Point（原生类也会强制）；时间在右键菜单，不在Details。菜单只有Add Notify表示点到空白；改值后必须Enter。
5. 每份AM应有**一条命中 + 一条连段**。Save后关闭重开，核对.60 / .20。第3段只核对已有命中，不加ET Combo Window，也不修改其.700秒长度。
6. 旧Practice的AM_M0_Attack_01_Montage不动，敌人继续用它。新Notify不在AnimBP接事件，不放到AS或LS，也不在通知对象里加角色变量。

### 8.3 玩家 GA Class Defaults：按字段装配

1. 打开`/Game/EmberTrial/Abilities/GA_ET_LightAttack`，**Class Defaults**，Parent仍ETGA_LightAttack。Event Graph保持空，不加事件业务。
2. EmberTrial | Attack：**Attack Montage换成刚建的AM_ET_Light_01**，必须确认是Combo/Montages路径；Attack Damage保持25，Attack Max Horizontal Speed=3，Damage Effect Class仍GE_ET_LightAttackDamage。不要换成GE_ET_DodgeCost。
3. EmberTrial | Combo：勾选 **Enable Combo**，**Combo Buffer Seconds=.25**。
4. 展开 **Combo Follow Ups**，点击数组右侧`+`两次，使数组只有 **2个元素**。展开每个元素，按下表填：

| 数组元素 | 代表段落 | Montage | Damage Multiplier |
| --- | --- | --- | ---: |
| Element 0 / Index 0 | 第2段 | AM_ET_Light_02（Combo/Montages） | 1.0 |
| Element 1 / Index 1 | 第3段 | AM_ET_Light_03（Combo/Montages） | 1.0 |

第1段使用上面的Attack Montage，不在数组再放一次。变量类型是Montage Object Reference，不是Class / AS；不把三份AM串到同一Montage里。两个数组元素不能重复。

5. Cost Gameplay Effect Class / Cooldown Class保持原None，本批不增加轻攻击成本。Compile → Save，Content Browser右键该GA → Asset Actions → Validate Assets，应有`[M1-07C2] Valid / Issues0`。
6. ET-COMBO-001说明段数组、时长 / Skeleton / Slot / Root Motion或通知契约有缺项；先按8.1–8.3逐项核对，不删除规则。ET-COMBO-002说明玩家Enable Combo未开或敌人误开。

### 8.4 敌人 GA 与角色

1. 打开`GA_ET_EnemyLightAttack`的Class Defaults：**Enable Combo=False**，Combo Follow Ups保持空；Attack Montage仍旧AM_M0_Attack_01_Montage，Attack Damage=10，原GE / 速度不变。Compile → Save → 项目Valid / Issues0。
2. BP_ThirdPersonCharacter的Light Attack Ability Class仍GA_ET_LightAttack；不重新挂组件或改父类。Compile / Save，单个ASC / Trace / Life / Recovery保持。
3. 玩家Mesh / Anim Class / Always Tick Pose等沿用既有配置，不重新编辑ABP_Manny / Quinn；GA_ET_Dodge仍.15 / 150 / .30 / Cost25。

## 9. 蓝图输入：沿用请求入口，不新接连段图

**正常情况下不需要新增蓝图节点。**左键既有IA_ET_LightAttack、数字1既有别名，都应使用Started，经过已有暂停 / UI允许门控，最终只到`Request Light Attack(Target=Self)`。Triggered / Completed / Canceled仍不连业务，Return Value可以留空。

```text
IA_ET_LightAttack.Started
    → 原来已验收的暂停 / UI门控的“允许游戏输入”白线
        → Request Light Attack（Target = Self）
```

本批不重建旧暂停变量或门控。**攻击请求前不要再用Is Formal Action Idle作额外Branch**：它在攻击期间会False，导致第二次输入到不了C++缓冲。若你现有图有这样一层，只把该层前面的白线直接连到原Request执行输入，保留更前面的暂停 / UI门控；原Branch断开即可。E / 移动 / 跳跃各自的Idle检查继续保留，不跟着删。

Request的Target是角色Self，不是PracticeAction / ASC / GA类。C++函数会区分“空闲激活”和“当前活动实例收输入”，不在蓝图中自行TryActivateAbility、Get Primary Instance、Delay或记连段索引。若自动化可连但实际左键只有单段，截图Started到Request的完整局部白线与Target即可，不先搭临时按键。

## 10. 分组检查（约10–15分钟）

### 10.1 两项自动化，约2–4分钟

不Play，Output Log先运行：

```text
Automation RunTests EmberTrial.M2.Combo
```

等 **Flow Success** 后再运行：

```text
Automation RunTests EmberTrial.M2.Dodge.Invulnerability
```

只复用这一项旧真实命中测试，因为本批改共享LightAttack；不重跑未变的CostContract / Movement / Resources / Practice全套。Flow检查新连段 / 缓冲，旧Invulnerability同时验证敌人单段10与无敌拒绝仍成立。

Flow应有`[M2-03B] Real combo 1->2->3: three unique traces, Health1000->925`。这组临时目标1000不代表正式地图配置变了。它还覆盖单次 / 过期 / 一槽五次输入只多打一段 / 无第4段、旧窗口 / Serial清理、已结束Task的迟到回调隔离、受击取消待调度、暂停前缓冲保留 / 菜单输入拒绝、非法配置 / 死亡 / EndPlay；不是性能基准。

失败时提供测试名、**第一项Expected失败**和大致时间 / 日志路径，我读取真实时序。不要删断言或人为加长缓冲来“凑成功”；也不要求你在手动测试中掐精确.25秒。

### 10.2 玩家请求 / 连击，约2–3分钟

- **操作：**正式地图Entry空地Play。只点一次让第1段结束；再长按一次；再按正常节奏逐次点击打1→2→3。数字1仍能作为同一请求入口。
- **预期：**单点 / 长按只一段；有节奏追加能三段，三段姿态 / 时长有区别；日志每段`Stage begin`，整链末尾只一次`End / AttackingTag0`，段间无移动 / E / 跳跃解锁。轻攻击不扣体力，镜头仍可转。
- **不易把握节奏时：**仅临时`slomo .25`放慢，观察日志Stage begin后在动作中后部追加；具体早过期 / 单槽契约已由自动化验证，不做十轮手动时序矩阵。本组末尾恢复`slomo 1`。

### 10.3 一次真实三段命中，约1–2分钟

- **操作：**靠近仍100血的训练目标，站定保持在右手范围内打完整三段，结束后不继续点。
- **预期：**三段各一次25，目标100→75→50→25；每段Trace Serial不同，同段多次Sweep没有重复扣。新段允许重新命中同目标；不要求本批再摆双目标 / 墙坡 / 帧率矩阵。
- **若没打满：**先看是否已有Stage2 / 3日志，再看各段Candidate与距离；占位动画轨迹不同，空挥不是强行改去重 / 提高范围的理由。给目标位置 / 当时状态和短视频，我定位；不要改正式25cm半径掩盖问题。

### 10.4 取消与P，约2–3分钟

- **操作A：**在任意段按3，等正式受击结束再点左键。再开一条链，在第1或2段按2。
- **预期A：**3取消当前链，Trace / 输入 / 待切段清空，受击结束新请求从1开始；2本批拒绝是预期，C后续才开放恢复取消。
- **操作B：**本次只单击一次左键、不要预缓存下一段，在动作中P暂停；菜单停留约2秒，点背景 / 按1，再用一种已支持方式继续。
- **预期B：**原动作恢复并结束，不把菜单输入补成下一段，不补跳 / 卡键。若暂停前已合法缓存，恢复后消费那一条是既定规则，自动化已单测；不要把它与菜单输入混淆。

### 10.5 敌人与结束短基线，约2分钟

- **操作：**让敌人正常接近挥击一次，观察普通10点扣血 / HUD与受击；再于空地开始攻击并Stop，重开一次。
- **预期：**敌人仍单段，原动作 / 无敌规则正常；新身体生命 / 体力100、单HUD，新攻击从1开始，无旧链续播。最后E一次、P一种继续方式、移动转向跳跃正常即可。
- **恢复：**slomo1、玩家25 / 敌人10、100/100、GA数组2元素 / 倍率1、Buffer.25、Dodge原配置、Debug False；无新增临时箱 / 测试Actor，Source / GA / Montage / 蓝图Save All。当前不Cook，不重做Retry / Continue或全部UI旧矩阵。

## 11. 完成反馈与报错

```text
M2-03B 完成 / 卡在第__节
三新 / 六旧Source；关闭UE后Editor Build / 索引52cpp / Game Build：
独立AM_ET_Light_01 / 原AS；旧共用AM未改：
玩家1/2命中与连段双状态、第三段只命中；.60/.20与8/6帧命中：
玩家Enable Combo / .25 / 两元素02-03与倍率1，25；敌人关闭 / 原AM / 10；Valid：
左键 / 数字1仍Started到角色Request，无额外攻击Idle拦截；Compile / Save：
Combo.Flow / Dodge.Invulnerability各一次Success：
单点 / 长按一段、正常1→2→3、每段独立Serial与真实各25：
3取消 / 结束再从1、2仍拒绝；P菜单输入无补播不卡键：
敌人单段10 / HUD；动作中Stop / 新身体与E-P-基础输入：
slomo1 / 原数值 / Debug False、Source资产Save All：
失败或占位动作最明显问题（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

编译报错用文字贴第一条真实错误；字段 / 通知菜单问题截图GA Class Defaults或状态条右键菜单，包含资产路径。输入问题截图Started到Request的白线，运行问题描述输入 / 当时段落与实际现象；日志给路径或大致时间，我自行提取，不要求手摘几十行。自动化可复现的窄窗口交由测试驱动，不让你重复手动赶时点。

## 12. 本批 Commit / Push（固定放在最后）

- **三新Source**：ETANS_ComboWindow.h / .cpp、ETLightComboFlowTests.cpp；**六旧**：ETGA_LightAttack.h / .cpp、Character.h / .cpp、ETCombatLifeComponent.h、ETGameplayAssetValidator.cpp。保留全部旧文件；Trace / Dodge / Build依赖 / UI无计划修改。
- **一新资产**：`EmberTrial/Content/EmberTrial/Animations/Combo/Montages/AM_ET_Light_01.uasset`；旧GA_ET_LightAttack与AM_ET_Light_02的真实配置差异。Enemy GA / BP_ThirdPersonCharacter只在确有合法保存差异时提交；没有新Input Action / IMC或地图。原Practice共用AM不修改。来源清单验收时新增实物 / 更新真实哈希，当前仍57，预计58不预填。
- 本指南、Progress / Backlog / M2与连击路线 / Architecture / VersionControl / AcceptanceWorkflow / Introduction，以及A报告 / 清单等未提交收尾。2026-10-16只读确认HEAD / 缓存origin/main均2202e74，旧C与A六素材已纳入提交，远端本轮未联网核验。当前仅本指南及同步文档；本批实现完成后重新Stage实际新代码 / 资产，避免已Stage早期骨架遗漏最新内容。
- .25 / 25 / 10 / 100、Dodge.15 / 150 / .30 / Cost25、slomo1 / Debug False恢复后Save All。Raw / 日志 / Binaries / Intermediate / Saved、生成CMake / IDE缓存与历史包仍本地，不Force Add，不修改全局Git设置。

CLion刷新Changes → 重新Stage本批完整Source / 新Montage / 真实资产差异 / 最新文档 → 检查Staged Diff → Commit → Push。可用`feat: add three-stage light combo with bounded input buffer`；仅指南用`docs: prepare light combo runtime guide`。仍由用户操作Git；B通过不代表C恢复闪避取消或整个M2-03已完成，当前仅指南下发。

验收后Git更新：B主体已纳入9a0cf14，HEAD / 缓存origin一致，远端未联网核验。当前提交B报告 / 来源58项 / 已验收状态与C指南；一新AM01及三新 / 六旧Source无需重复制造差异。具体收尾以[B报告末尾](Reports/M2_03B_Verification.md#5-本次-commit--push固定放在最后)为准。
