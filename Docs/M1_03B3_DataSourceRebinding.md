# M1-03B3：真实换身体与旧数据源隔离

日期：2026-10-04；创建路线修正与收尾：2026-10-05。前置：[B2已验收](Reports/M1_03B2_Verification.md)。**主体与Waiting局部收尾均已通过，结果见[B3验收](Reports/M1_03B3_Verification.md)；无需重做本文五函数与整套测试。**本文保留初始实施指南，原预计手动90–150分钟，实际主体70 / 收尾10分钟。纯蓝图，无新增C++ / Build / 索引要求。

## 1. 验证目标

把“同一身体失去 / 恢复控制”推进到“真正不同的属性源”。保留原角色，生成隔离的原生Character子蓝图：原角色100/100，新身体40/80，未就绪身体100/60配置被拒绝。再复用已有裸Pawn验证UnsupportedPawn。

需要证明：

- 新身体有自己的ASC / 属性集 / Ready状态；Presenter按当前身体重绑，不带入旧值。
- 旧身体仍存活时应用真实GE，旧属性确实改变，当前HUD不受影响；销毁旧身体也不清空当前UI。
- 销毁当前身体进入NoPawn，再生成新实例回到Ready；同一旧身体恢复保持其修改值，新实例使用自己的初值。
- 角色已Ready后才Possess，Presenter仍立即读到快照；正常出生的“View先订阅、角色后Ready”与B2已有直接日志共同验证两种顺序。
- Unsupported / Waiting / NoPawn下重建Widget，先解绑旧View再读当前状态；停止PIE无旧数据残留。

这是数据流与生命周期回归，不实现正式死亡 / 重生、异步加载或完整新角色玩法。新测试Character没有M0交互 / 动作 / 暂停蓝图；测试结束回原角色验收原功能。不要复制BP_ThirdPersonCharacter，避免复制其UI与输入业务。

## 2. 准备与两个隔离资产

### 2.1 基线

Stop，Save All，打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`。确认原角色最终Initial Health / Initial Max Health为100/100；Controller仅一个UIBootstrap，两Widget Class保持B2配置。

B2临时4–9/J/K、函数和变量应已删除且正常保存。新键含义见第5节，不沿用旧键图表。新测试资产不放地图，不改GameMode / Default Pawn Class。

### 2.2 就绪身体 BP_ETHealthProbeCharacter

Content Browser进入 `/Game/EmberTrial/Tests/Blueprints`，右键Blueprint Class → All Classes，选择原生 **EmberTrialCharacter（AEmberTrialCharacter）**，命名 **BP_ETHealthProbeCharacter**。Class Settings确认父类是原生EmberTrialCharacter，不是BP_ThirdPersonCharacter或普通Character。

继承组件已有Capsule、Mesh、CameraBoom、FollowCamera、AbilitySystem；**不另加ASC、UIBootstrap、PracticeAction或属性集**。这就是已实现Character所有权的真实第二个实例。

| 位置 | 设置 |
| --- | --- |
| Class Defaults → EmberTrial / Startup | Initial Health=40；Initial Max Health=80 |
| Class Defaults → Pawn | Auto Possess Player=Disabled；Auto Possess AI=Disabled |
| Class Defaults → Input | Default Mapping Context=`/Game/ThirdPerson/Input/IMC_Default`；Jump Action=`/Game/ThirdPerson/Input/Actions/IA_Jump`；Move Action=IA_Move；Look Action=IA_Look（后两者同在Actions目录） |
| CapsuleComponent → Collision | Collision Presets=NoCollision；用于隔离属性测试，不测试这个身体的墙壁 / 地面碰撞 |
| CharacterMovement | Gravity Scale=0；Max Walk Speed=0；Jump Z Velocity=0，避免测试身体移动 / 落下 |
| CameraBoom | Do Collision Test=False，避免附近原角色影响测试镜头 |
| 继承Mesh | Skeletal Mesh / Anim Class保持None，本测试用下方Cube占位 |

四项Input资源**都要赋值**：原生SetupPlayerInputComponent仍会绑定输入，即使本测试不要求移动，也不留空这些指针。测试身体不具备1/2/3、E/P的蓝图业务；固定Cube和镜头是预期表现。

Components Add → Static Mesh，命名 **HealthProbeMesh**，挂Capsule下：Static Mesh=`/Engine/BasicShapes/Cube`；Location / Rotation全0；Scale=(0.35,0.35,0.35)；Collision Presets=NoCollision；Simulate Physics=False。无需新材质或美术资源。

Event Graph只加一条销毁证据：

1. 右键添加 **Event EndPlay**，白线接Print String，Print to Log=True。
2. 添加Get Object Name，Object=Self；添加Append，A=`[M1-03B3] Health test EndPlay Pawn=`，B=对象名；Return Value接Print.In String。
3. 不加BeginPlay初始化 / Delay / Tick / UI创建。原生BeginPlay负责属性与Ready；这条EndPlay日志不替代原生清理。

Compile / Save。

### 2.3 未就绪身体 BP_ETHealthWaitingCharacter

**2026-10-05修正：不要Duplicate带原生AttributeSet的Character蓝图。**本机5.5.4实际复制得到的Waiting默认HealthAttributes指针为None，与[Epic UE-273844](https://issues.unrealengine.com/issue/UE-273844)一致。应在Content Browser右键Blueprint Class → All Classes → 原生EmberTrialCharacter，新建直接子蓝图，按第2.2配置组件 / 输入 / 无碰撞设置。

第一次实施可命名BP_ETHealthWaitingCharacter；当前用户已完成主体且旧同名资产存在，按[收尾指南](M1_03B3_Closure.md)新建BP_ETHealthWaitingCharacter_Rebuilt，旧资产暂留但停用，不恢复整套临时图表。

Class Defaults只改 **Initial Health=100、Initial Max Health=60**，Compile / Save。它是保留在Tests的非法配置样例，生成时出现预期Invalid startup health Warning。不要把它设成默认玩家；本批结束也不把这个样例改成合法值。原玩家与合法Probe分别保持100/100、40/80。

### 2.4 裸Pawn

复用 `/Game/EmberTrial/Tests/Blueprints/BP_ETLifecycleProbePawn`，保持Pawn父类、NoCollision、Auto Possess Disabled和已有Camera / EndPlay日志，不加入ASC或其他玩法。新建的只有上面两个Character蓝图。

## 3. Controller临时变量与共用约定

全部临时函数 / 按键在 **BP_ETPlayerController**。`Self`是Controller；角色方法Target接Cast成功对象；UI方法Target接组件；Effect方法Target接ASC。原有1/2/3、E/P不修改。

创建五个变量，全部Object Reference、默认None、Instance Editable=False：

| 名称 | 类型 | 用途 |
| --- | --- | --- |
| ETB3OriginalPawn | Pawn Object Reference | 仅缓存原BP_ThirdPersonCharacter，之后不覆盖 |
| ETB3ReadyPawn | Pawn Object Reference | 合法Probe，40/80 |
| ETB3WaitingPawn | Pawn Object Reference | 未就绪测试身体 |
| ETB3BarePawn | Pawn Object Reference | 裸Pawn |
| ETB3HeldWidget | ETHealthHUDWidget Object Reference | 保留最新移除的旧Widget，验证存活时解绑 |

这是临时测试的强引用；不写进Presenter / Subsystem /正式Widget，不用于正式重生。Compile一次确认类型与None默认值。

本章Is Valid均为**带白线**版本。每个无效出口只Print然后Return / 结束，不连到成功业务。Print String全部Print to Log=True、Print to Screen=True、Duration=2。函数都为普通非Pure；Return Node可复制成多个，分别给出明确返回值。

## 4. 五个临时函数：执行线与数据线

### 4.1 ETB3_CacheOriginalPawn

无输入；Boolean输出 **Success**。只第一次缓存原角色。

**白线：**Entry → Is Valid（已缓存原Pawn）；有效 → Return Success=True；无效 → Cast To BP_ThirdPersonCharacter；成功 → Set ETB3OriginalPawn → Return True；Cast Failed → Print `[M1-03B3] Original Pawn unavailable; restart PIE` → Return False。

**数据：**Is Valid.Input Object=Get ETB3OriginalPawn；Cast.Object=Get Controlled Pawn.Return Value（Target=Self）；Cast.As BP Third Person Character=Set ETB3OriginalPawn的值。各Return.Success分别勾True / False。

不把Probe或None覆盖到原引用；无Possess / Spawn / Ensure。第一次4 / J / K会调用它，U也先缓存，其他测试键不会猜哪个对象是原角色。

### 4.2 ETB3_GetOrSpawnTestPawn

输入 **ExistingPawn（Pawn Object Reference）**、**PawnClass（Pawn Class Reference，紫色）**、**Offset（Vector）**；输出 **TestPawn（Pawn Object Reference）**。函数Local Variables新建 **SpawnedPawn（Pawn Object Reference）**，默认None；不增加第六个Controller变量。

先放下列执行节点：原引用Is Valid、ExistingPawn Is Valid、Spawn Actor from Class、Set本地SpawnedPawn、SpawnedPawn Is Valid、Cast To EmberTrialCharacter、两份成功Print与Return，以及拒绝Print与None Return。

**白线完整路径：**

```text
Entry → Is Valid（Input Object=Get ETB3OriginalPawn）
  Is Not Valid → Print“[M1-03B3] Spawn rejected: no original Pawn” → Return TestPawn=None
  Is Valid → Is Valid（Input Object=Entry.ExistingPawn）
    Is Valid → Return TestPawn=Entry.ExistingPawn
    Is Not Valid → Spawn Actor from Class → Set本地SpawnedPawn
      → Is Valid（Input Object=Get本地SpawnedPawn）
          Is Not Valid → Print“[M1-03B3] Spawn failed” → Return None
          Is Valid → Cast To EmberTrialCharacter
            成功 → Print“Spawn finished … GameplayReady=…” → Return TestPawn=Get本地SpawnedPawn
            Cast Failed → Print“Spawn finished … No gameplay interface” → Return同一本地SpawnedPawn
```

**Spawn与Cast引脚：**

| 引脚 | 接法 |
| --- | --- |
| Spawn.Class | Entry.PawnClass |
| Spawn.Collision Handling Override | Always Spawn, Ignore Collisions |
| Spawn.Owner / Instigator | 留空 |
| Spawn.Return Value | Set本地SpawnedPawn的值 |
| SpawnedPawn Is Valid.Input Object | Get本地SpawnedPawn |
| Cast To EmberTrialCharacter.Object | Get本地SpawnedPawn |
| 两个成功Return.TestPawn | Get本地SpawnedPawn，不是Get Controlled Pawn |

**Spawn Transform逐节点：**Get Actor Location（Target=Get ETB3OriginalPawn）→ Vector + Vector，另一输入接Entry.Offset；结果接Make Transform.Location。Get Actor Rotation（Target=同一原Pawn）接Make Transform.Rotation；Scale=(1,1,1)。Make Transform.Return Value接Spawn.Spawn Transform。读取原对象变换只发生在原引用有效的成功路径。

先把 **Pawn Class Reference** 类型的Entry.PawnClass连到Spawn.Class，再连Return Value；返回引脚应为Pawn兼容对象，可直接存入SpawnedPawn，不需要Cast To Pawn。若仍显示Actor且不兼容，先核对函数输入是不是错建为Actor Class Reference，并重建Spawn节点。

**Character成功Print数据：**Get Object Name.Object=Get本地SpawnedPawn；Is Gameplay Ready.Target=Cast.As EmberTrialCharacter；Bool返回值经过To String(Boolean)，连接Format Text的Ready。格式：

```text
[M1-03B3] Spawn finished Pawn={Pawn} GameplayReady={Ready}
```

Pawn接对象名，Format.Result→Print.In String（自动Text→String）。裸Pawn成功Print格式为 `[M1-03B3] Spawn finished Pawn={Pawn} No gameplay interface`，Pawn仍接本地对象名。**Cast到EmberTrialCharacter失败在这里不是Spawn失败**：它是合法裸Pawn路径，照样返回这个Pawn。

这个函数完成Spawn / 原生BeginPlay后才返回，**不执行Possess**。合法新身体日志应GameplayReady=True，未就绪身体False；后一项不是Spawn失败。不调用Ensure、初始化或写属性。固定的ExistingPawn引用只配对应固定Class，接法见第5节，不混用三个缓存。

### 4.3 ETB3_PossessIfDifferent

Pawn Object Reference输入 **TargetPawn**，无输出。

**白线：**Entry → Is Valid；无效 → Print `[M1-03B3] Possess rejected: invalid target` → Return；有效 → Branch（当前==目标）；True → Print `[M1-03B3] Already possessed` → Return；False → Possess → Print当前对象名 → Return。

**数据：**

| 引脚 | 接法 |
| --- | --- |
| Is Valid.Input Object | Entry.TargetPawn |
| Equal(Object).A | Get Controlled Pawn.Return Value，Target=Self |
| Equal(Object).B | Entry.TargetPawn |
| Branch.Condition | Equal(Object).Return Value |
| Possess.Target | Self（Controller） |
| Possess.In Pawn | Entry.TargetPawn |
| 最后Print | Append A=`[M1-03B3] Possess complete Current=`；B=Get Object Name(Object=Get Controlled Pawn，Target=Self) |

True提前退出，False才Possess。不先手动UnPossess，不创建对象，不重建Widget。Possess绑定的目标是Pawn，[UE5.5 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/GameFramework/AController/Possess?application_version=5.5)。本批单玩家PIE有权限执行，不声称支持联机。

### 4.4 ETB3_ApplyEffectToPawn

输入 **TargetPawn（Pawn Object Reference）**、**EffectClass（GameplayEffect Class Reference）**、**CaseLabel（String）**；无输出。这个函数特意接受“指定对象”，不能把内部所有查询都改成Get Controlled Pawn，否则无法验证旧身体。

**执行节点 / 白线：**

| 顺序 | 成功出口 | 失败出口 |
| --- | --- | --- |
| Entry → Branch，Condition=Is Game Paused | False → Pawn Is Valid | True → Print Rejected paused → Return |
| Pawn Is Valid | 有效 → Cast To EmberTrialCharacter | 无效 → Print Rejected invalid target → Return |
| Cast To EmberTrialCharacter | 成功 → Ready Branch | Cast Failed → Print Rejected unsupported Pawn → Return |
| Ready Branch | True → ASC Is Valid | False → Print Rejected not ready → Return |
| ASC Is Valid | 有效 → Apply Gameplay Effect to Self | 无效 → Print Rejected missing ASC → Return |
| Apply | → Snapshot Branch | 以快照检查结果，不用Instant Handle.IsValid判断 |
| Snapshot Branch | True → Format / Print快照 → Return | False → Print Snapshot unavailable → Return |

失败Print文字统一加 `[M1-03B3] ` 前缀。**数据线逐条：**

| 从 | 到 |
| --- | --- |
| Entry.TargetPawn | Pawn Is Valid.Input Object、Cast.Object |
| Cast.As EmberTrialCharacter | Is Gameplay Ready.Target、Get Ability System Component.Actor、Get Gameplay Health Snapshot.Target |
| Is Gameplay Ready.Return Value | Ready Branch.Condition |
| Get Ability System Component.Return Value（ASC，Blueprint Library查询） | ASC Is Valid.Input Object、Make Effect Context.Target、Apply.Target |
| Entry.EffectClass | Apply.Gameplay Effect Class |
| Make Effect Context.Return Value | Apply.Effect Context |
| Apply.Level | 输入1.0 |
| Get Gameplay Health Snapshot.Return Value（Boolean） | Snapshot Branch.Condition |

Get Gameplay Health Snapshot为纯节点，输出Health / MaxHealth在Apply后的Snapshot Branch / Print中求值。Make Effect Context通常纯节点；若显示白线，放在ASC有效出口与Apply之间。选择ASC版Apply，不能选Gameplay Ability或Spec Handle版。所有ASC节点都来自**传入TargetPawn的Cast对象**，不是Controller Self。

成功Print用Format Text：

```text
[M1-03B3] Effect result Pawn={Pawn} Case={Case} Health={H}/{M}
```

Pawn=Get Object Name(Object=Entry.TargetPawn)；Case=Entry.CaseLabel；H / M=Get Gameplay Health Snapshot两个float输出；Result接Print.In String。Print后Return。

这条快照证明指定旧身体确实改了属性；屏幕HUD依然只跟随当前受控对象。函数内不读Presenter来代替这个旧身体快照，不SetText / SetPercent，不修改Initial值。

### 4.5 ETB3_DestroyTestPawn

输入 **TargetPawn（Pawn Object Reference）**、**CaseLabel（String）**；无输出。必须拒绝销毁缓存的原玩家，并清掉被销毁对象对应的测试缓存。

**白线：**Entry → Is Valid（原引用）；无效 → Print `[M1-03B3] Destroy rejected: cache original first` → Return；有效 → Is Valid（传入目标）；无效 → Print `[M1-03B3] Destroy rejected: invalid target` → Return；有效 → Branch（目标==原Pawn）；True → Print `[M1-03B3] Destroy rejected: original Pawn` → Return；False → Print销毁请求 → Destroy Actor。

**数据：**第一Is Valid.Input Object=Get ETB3OriginalPawn；第二=Entry.TargetPawn。Equal(Object).A=Entry.TargetPawn、B=Get ETB3OriginalPawn，返回值接Branch.Condition。Destroy Actor.Target=**Entry.TargetPawn**，不是Self。

请求Print用Format Text `[M1-03B3] Destroy requested Pawn={Pawn} Case={Case}`，Pawn=Get Object Name(Object=Entry.TargetPawn)，Case=Entry.CaseLabel。放在Destroy之前，便于对照EndPlay实例名。

Destroy白色输出后依次清三个可能匹配的缓存：

```text
Destroy Actor → Branch R（Condition=ETB3ReadyPawn == Entry.TargetPawn）
  True  → Set ETB3ReadyPawn=None → Branch W
  False → Branch W
Branch W（Condition=ETB3WaitingPawn == Entry.TargetPawn）
  True  → Set ETB3WaitingPawn=None → Branch B
  False → Branch B
Branch B（Condition=ETB3BarePawn == Entry.TargetPawn）
  True  → Set ETB3BarePawn=None → Return
  False → Return
```

每个比较均为Equal(Object)：A接对应Get变量，B接函数Entry.TargetPawn。Set值输入不接线、保持None；**只清相等的缓存**。每条True的Set出口与对应False出口都接下一Branch的执行输入，不添加Sequence或提前Return。Original变量永远不清空；其他仍活着的测试身体不受影响。

输入TargetPawn在函数调用时已取得，Destroy后仍用于身份比较；不重新Get Controlled Pawn来做比较，因为此时Controller可能已变None。无效引用不执行Get Object Name / Destroy。

五函数依次Compile，最后Controller Compile / Save。不要把这些函数转为Pure。

## 5. 临时按键：只接Released

Event Graph添加顶部数字4–9以及 **J、K、U、H、L、F、X** 的键事件。每个选中后Details：Consume Input=True、Execute When Paused=False；Pressed留空，只接Released。每次测试完整按下 / 松开。不要占用W/A/S/D、E/P、Space或原1/2/3。

| 键 | 含义 |
| --- | --- |
| 4 | 生成 / 复用合法Ready身体并控制 |
| 5 | 返回原身体 |
| 6 / 7 | 当前身体Damage25 / Heal40 |
| 8 | 保留旧Widget后重建生命HUD |
| 9 | **查询**Presenter快照，本批不是重启观察 |
| J | 生成 / 复用Waiting身体并控制 |
| K | 生成 / 复用裸Pawn并控制，本批不是Max120 |
| U | 缓存原角色后UnPossess |
| H | 给已经离开控制的Ready测试身体Damage25 |
| L | 给已经离开控制的原身体Damage25 |
| F | 销毁保存的Ready测试身体，可验证旧身体销毁 |
| X | 销毁当前测试身体，函数拒绝销毁原角色 |

### 5.1 三条生成链：4 / J / K

每条都完整建立以下白线，各自使用一份缓存Set：

```text
键.Released → ETB3_CacheOriginalPawn → Branch（Condition=Success）
  False → Print“[M1-03B3] Cannot start test: original Pawn missing” → 结束
  True → ETB3_GetOrSpawnTestPawn → Set对应缓存 → ETB3_PossessIfDifferent → 结束
```

三个函数调用Target均Self。GetOrSpawn.TestPawn接Set对应变量值；PossessIfDifferent.TargetPawn接**该缓存的Get**，不是Get Controlled Pawn。Set必须在Possess之前执行。

| 键 | GetOrSpawn.ExistingPawn | PawnClass固定选择 | Offset | Set / 后续TargetPawn |
| --- | --- | --- | --- | --- |
| 4 | Get ETB3ReadyPawn | BP_ETHealthProbeCharacter | (300,0,120) | ETB3ReadyPawn |
| J | Get ETB3WaitingPawn | BP_ETHealthWaitingCharacter | (600,0,120) | ETB3WaitingPawn |
| K | Get ETB3BarePawn | BP_ETLifecycleProbePawn | (900,0,120) | ETB3BarePawn |

原身体必须首次被正确缓存。Spawn失败返回None时PossessIfDifferent安全拒绝；不能用None去控制对象或提前销毁原身体。每条ExistingPawn与Class配套，不把三个变量接成同一个。

### 5.2 数字5：返回原身体

白线 **5.Released → ETB3_PossessIfDifferent → 结束**；Target=Self，TargetPawn=Get ETB3OriginalPawn。无Spawn / UnPossess / 初始化。重复5由函数的对象比较拒绝重复Possess。

### 5.3 数字6 / 7：当前身体Effect

各放ETB3_ApplyEffectToPawn调用，Target=Self。6.Released / 7.Released白线分别接调用，不接其他业务。

| 输入 | 6 | 7 |
| --- | --- | --- |
| TargetPawn | Get Controlled Pawn.Return Value，Target=Self | 同样显式接当前Pawn |
| EffectClass | `/Game/EmberTrial/Tests/Effects/GE_ET_Test_Damage25` | GE_ET_Test_Heal40（同目录） |
| CaseLabel | CurrentD25 | CurrentH40 |

Class下拉不选None或GameplayEffect基类。函数里Cast的是**原生EmberTrialCharacter**，支持两个新测试身体；不能继续只Cast To BP_ThirdPersonCharacter。

### 5.4 H / L：指定旧身体Effect

两条各放Is Valid、Equal(Object)、Branch和Effect调用。

**白线：**键.Released → Is Valid；无效 → Print `[M1-03B3] Old target unavailable`结束；有效 → Branch（保存对象==当前Pawn）；True → Print `[M1-03B3] Target still controlled; old source test rejected`结束；False → ETB3_ApplyEffectToPawn，结束。

| 引脚 | H（旧Ready身体） | L（旧原身体） |
| --- | --- | --- |
| Is Valid.Input Object | Get ETB3ReadyPawn | Get ETB3OriginalPawn |
| Equal(Object).A | 同一保存对象 | 同一保存对象 |
| Equal(Object).B | Get Controlled Pawn.Return Value，Target=Self | 同上 |
| Branch.Condition | Equal(Object).Return Value | 同上 |
| Apply调用.Target | Self | Self |
| Apply.TargetPawn | Get ETB3ReadyPawn | Get ETB3OriginalPawn |
| Apply.EffectClass | GE_ET_Test_Damage25 | GE_ET_Test_Damage25 |
| Apply.CaseLabel | OldReadyD25 | OldOriginalD25 |

Equal.True是仍在控制该对象，拒绝这次“旧源测试”；False才调用。这里的TargetPawn不是Get Controlled Pawn，函数内部ASC也不能改查当前Pawn。这两条测试入口不属于产品的远程伤害设计。

### 5.5 数字8：存活旧Widget解绑

Components拖UIBootstrap到Graph选Get。从输出创建Get Health Widget与Rebuild Health Widget，两者Target均接UIBootstrap。

白线：**8.Released → Set ETB3HeldWidget → Rebuild Health Widget → Is Valid**。

- Set.值=Get Health Widget.Return Value，在重建前保存旧实例。
- Is Valid.Input Object=**Get ETB3HeldWidget**，不是重建后的Get Health Widget。
- 有效出口→Print：Format Text `[M1-03B3] Held old View valid: {Name}`；Name=Get Object Name(Object=Get ETB3HeldWidget)，Result接In String；结束。
- 无效出口→Print `[M1-03B3] No held old View`；结束。

蓝图不再CreateWidget / RemoveFromParent / 手动解绑；原生Bootstrap负责。新View必须立即显示当前数据源状态，旧引用仅为测试保留。

### 5.6 数字9：查询当前Presenter，而非写HUD

从UIBootstrap Get输出拖出Get Health Presenter，Target接UIBootstrap；返回值接带白线Is Valid.Input Object。白线 **9.Released → Is Valid**；无效Print `[M1-03B3] No Presenter`后结束；有效接下面Print。

从Presenter返回值拖出 **Get Snapshot**，Target接这个Presenter；Return Value接 **Break ETHealthViewData**。放Format Text：

```text
[M1-03B3] Presenter query State={State} Pawn={Pawn} Health={H}/{M} Fraction={F}
```

| Format输入 | 来源 |
| --- | --- |
| State | Break.State → 从枚举引脚搜索Enum to String / To String转换 |
| Pawn | Break.PawnName |
| H / M / F | Break.Health / MaxHealth / HealthFraction |
| Print.In String | Format.Result，Text→String自动转换 |

这个Print只读缓存；不重启观察、不重建Widget、不改属性。Unavailable状态下的0字段是占位，不按0血量判死亡。

### 5.7 U：安全UnPossess

白线 **U.Released → ETB3_CacheOriginalPawn → Branch(Success)**。False只Print `[M1-03B3] UnPossess rejected: no original Pawn`结束；True → Is Valid（Input Object=Get Controlled Pawn.Return Value，Target=Self）。有效 → Un Possess（Target=Self），结束；无效 → Print `[M1-03B3] Already no Pawn`，结束。

不清空原引用，不销毁当前对象。重复U无额外业务；先缓存再失去控制，避免无法用5返回。

### 5.8 F / X：销毁旧或当前测试对象

两条白线各为 **键.Released → ETB3_DestroyTestPawn → 结束**，调用Target=Self：

| 参数 | F | X |
| --- | --- | --- |
| TargetPawn | Get ETB3ReadyPawn | Get Controlled Pawn.Return Value，Target=Self |
| CaseLabel | SavedReady | CurrentTestPawn |

函数内部有无效检查、原对象保护和缓存清理；不要额外在键事件上对Self调用Destroy Actor。按X时若正在控制原身体应被拒绝，原身体不能因测试丢失。当前对象传入函数后固定，销毁后比较仍使用Entry.TargetPawn。

### 5.9 开始前逐项自查

1. GetOrSpawn先验证原引用；返回的是新 / 保存测试对象，不是当前受控Pawn。
2. 4 / J / K的ExistingPawn、Class和Set变量一一对应。
3. 所有自定义函数调用Target=Self；UIBootstrap方法Target=组件。
4. Effect函数TargetPawn参数贯穿Is Valid / 原生Cast / ASC /快照；H/L传旧对象，6/7传当前对象。
5. 8先Set旧View再重建，9只查询；F/X不会销毁Original。
6. 全部使用Released / Execute When Paused=False；没有Tick / Delay / 新BeginPlay业务。

Controller Compile / Save。把键组加注释“B3临时测试”，避免最终清理漏删。

## 6. 分组验收

每组 **Stop → 新Play**，原角色100/100；用9看Presenter完整State / PawnName，右上状态面板也应跟随当前对象。所有效果按松开触发，等待画面更新再下一步。Output Log过滤 `[M1-03B`；必要时清过滤观察A2初始化 / EndPlay。

测试身体Cube、固定位置、无M0动作 / E/P是预期；用5回原角色再测原玩法。记录真实实例名，不把显示标签或资产名当对象身份。

### 6.1 新身体独立属性与同一对象恢复

**准备：**新PIE100/100。

| 顺序 | 操作 | 画面 / 数据预期 |
| ---: | --- | --- |
| 1 | 6一次 → 9 | 原角色75/100，0.750；记原对象名 |
| 2 | 4一次 → 9 | 新Character40/80，0.500；名字与原角色不同 |
| 3 | 4重复一次 | Already possessed；无新Spawn / Possess / 属性重置 |
| 4 | 6一次 → 9 | 新身体15/80，0.1875，日志约0.188 |
| 5 | 5一次 → 9 | 原身体仍75/100，不带入15/80 |
| 6 | 5重复一次 | Already possessed；状态不重复变化 |
| 7 | 4一次 → 9 | 复用刚才同一新身体15/80，无新Health initialized |

**日志预期：**首次合法Spawn finished的GameplayReady=True发生在Possess complete之前；Presenter.PawnChanged直接读Ready40/80。合法身体Owner / Avatar / ASC属于自己的Character，原身体UnPossess不重置属性。Presenter / 当前Widget身份保持，切身体不主动重建HUD。

**清理：**5回原角色，Stop。两个测试身体本轮随World清理。

### 6.2 旧ASC变化与旧身体销毁隔离

**准备：**新PIE；6一次把原身体变75/100；4生成Ready40/80。

| 顺序 | 操作 | 旧对象快照 | 当前HUD预期 |
| ---: | --- | --- | --- |
| 1 | L一次 | OldOriginalD25：原身体50/100 | 新身体仍40/80，不能闪到50/100 |
| 2 | 6一次 | CurrentD25：新身体15/80 | 15/80 |
| 3 | 5 → 9 | 返回原身体 | 50/100 |
| 4 | H一次 | OldReadyD25：新身体0/80 | 原身体仍50/100，不被变成0/80 |
| 5 | 4 → 9 | 控制仍存活的Ready对象 | 0/80，空条但State=Ready |
| 6 | 5 → F → 9 | 销毁已离开控制的Ready身体 | 原身体仍50/100，不能NoPawn / Waiting |
| 7 | F再一次 | invalid target拒绝 | 仍50/100 |
| 8 | 4 → 9 | 生成新的Ready实例，名字不同 | 40/80，不继承旧实例的0/80 |

**日志预期：**L/H必须出现带**旧对象名**的Effect result，证明旧属性实际变化。步骤1/4/6期间不能发布改变当前HUD数据的错误Snapshot；步骤6有旧身体EndPlay / Ready撤销 / ActorInfo清理，但不能清掉当前原身体。步骤8新实例恰好一次初始化；不把它记成同一个实例被重置。

本组验证可见行为隔离；移除旧ASC委托与旧源回调拒绝还要结合已审查的C++实现判断，不仅凭“屏幕没动”推断委托一定已移除。

**清理：**5回原角色，Stop。无需恢复属性资产配置。

### 6.3 UnsupportedPawn、销毁后NoPawn与恢复

**准备：**新PIE；4先缓存原角色 / 创建Ready身体。

**操作：**K → 9 → 6 / 7各一次 → 8 → X → 9 → 8 → K → 9 → 5 → U → U → 4 → 5。

**分步预期：**

1. K控制裸Pawn：State=UnsupportedPawn，标题“当前角色不提供生命信息”，数值 / 条隐藏；原状态面板显示裸Pawn。
2. 6 / 7拒绝unsupported，不创建ASC或改属性；8新View.SubscribeSnapshot仍Unsupported。
3. X销毁当前裸Pawn，旧Probe EndPlay与Controller清空后State=NoPawn，标题“暂无受控角色”。9读NoPawn，8重建也立即NoPawn。
4. 再K生成新裸Pawn，名字不同，UI恢复Unsupported，不沿用被销毁实例。
5. 5回原身体100/100；U变NoPawn，第二次U只Print Already no Pawn；4可从空Pawn控制已有Ready40/80；最后5回原身体100/100。

**日志预期：**不同状态的PawnName正确；旧View解绑后无后续事件；销毁当前对象能清空当前数据，而不是只按角色Ready决定。无Accessed None。

**清理：**确保最后已5恢复原角色，Stop。

### 6.4 Waiting下旧身体销毁与合法新源恢复

**准备：**新PIE；4创建Ready40/80；5回原身体。

**操作：**J → 9 → 6 / 7各一次 → 8 → F → 9 → X → 9 → 4 → 9 → 5。

**画面预期：**J生成未就绪Character，标题“角色准备中”，数值 / 条隐藏；两Effect拒绝，8立即Waiting。F销毁已经离开控制的合法Ready身体，当前Waiting对象 / UI不受影响。X销毁当前Waiting身体后NoPawn；4生成新的合法Ready身体恢复40/80；5回原角色100/100。

**日志预期：**J的Invalid startup health 100/60 Warning是样例预期；Spawn finished GameplayReady=False；9为WaitingForGameplayReady，不能冒充Ready0血。F的旧Ready撤销不能改变当前Waiting.PawnName；X后NoPawn；新合法对象先初始化 / Ready，再Possess后立即查询到40/80。

这里是**替换为合法新对象**，不是把同一个非法身体修复成Ready。不要在运行时反射修改Initial参数或广播伪造Ready。

**清理：**5返回后Stop。原玩家保持100/100、合法Probe40/80；Waiting资产保留100/60测试配置。

### 6.5 当前Ready身体销毁、新实例与View重建

**准备：**新PIE；6原身体75/100；4控制新Ready40/80；6变15/80。

**操作：**8 → X → 9 → 8 → 4 → 9 → 6 → 9 → 5。

**画面预期：**第一次8立即15/80；X销毁当前Ready身体后NoPawn，数值与条隐藏；第二次8立即NoPawn。4生成新实例40/80；再6只有一次15/80更新；5回原角色75/100。

**日志预期：**X前后身体名 / EndPlay / NoPawn对应；恢复实例名不同，恰好一次Health initialized。两次8移除旧View前解绑，Held old View valid存在；最后6只有当前View接变化，旧实例没有更新。

**清理：**5返回后Stop；不把原角色销毁作为正式重生模拟。

### 6.6 两种就绪顺序、三轮往返与PIE结束

**准备：**新PIE100/100，保存本轮出生的B1/B2相关日志。

**操作与检查：**

1. 出生若先出现Waiting / SubscribeSnapshot再ReadyTrue / ViewDataChanged，记录真实顺序；若首次直接Ready也合法。B2已保存真实“先订阅、后Ready”证据，**不为制造时序加Delay或改初始化**。
2. 第一次4生成新Ready身体，确认日志先Health initialized / Ready，再Spawn finished GameplayReady=True，再Possess / Presenter Ready快照。这证明通知先发生仍能通过查询补齐。
3. 回5，然后做 **三轮4→5→J→5→K→5**；各状态分别40/80、原100/100、Waiting、原100/100、Unsupported、原100/100。第二 / 三轮复用相同测试对象，不再初始化；重复4 / 5各一次不重复Possess。
4. K状态下8一次 → 9，重建快照仍Unsupported；5后J，**当前Waiting时Stop**，观察当前View / Presenter及所有身体清理。
5. 再Play / Stop两轮，合计三轮；后两轮原身体出生100/100，不继承前轮引用或状态。

**日志预期：**同一轮Presenter身份保持；离开控制的身体不会因再次Possess重置属性。所有View解绑后没有再ViewDataChanged，Presenter停止后无旧Ready。World结束可能先发NoPawn / Ready撤销，按真实对象与清理先后判断，不能要求所有角色固定同一销毁顺序。

本批验证现有同步初始化的两种订阅顺序，不声称实现 / 验证了未来异步加载等待数秒后就绪。后续有真实异步前置条件时再加入对应超时 / 取消测试。

**清理：**Stop后进入第7节；不要求重做B2完整九组或十次重建。

## 7. 删除临时图表与最终回归

Stop。在BP_ETPlayerController删除：

- **4–9、J、K、U、H、L、F、X**事件及所有本批临时调用 / Cast / Print / Get / Set组。
- 第4节五个 **ETB3_** 函数；函数内部Local SpawnedPawn随函数删除。
- 第3节五个 **ETB3** 对象变量，包含HeldWidget。若变量删除提示仍被引用，先定位遗漏临时节点。

保留唯一UIBootstrap、两Widget Class配置、正式Widget / Presenter实现；保留Tests中两个新Character和既有裸Pawn。不把测试资产加入地图、默认Pawn或正式初始化配置。原角色100/100、合法Probe40/80、Waiting100/60与四项Input资源分别保持预定值。

Controller **Compile → Save**，不只依赖自动保存；新测试资产也Compile / Save，Save All。新地图做一轮短回归：移动 / 转向 / 跳跃、1/2/3与2→3、E-A/B、P鼠标与键盘继续 / 无补播不卡键。左右面板一份、出生生命100/100。Stop后再Save All。

本批没有Source改动，不重复Build / 索引 / 打包。两个新测试资产完成后才加入来源清单；当前交接时仍不计为已创建。下一步M1-03C整理总回归与数据流结论，再进入正式能力开发。

## 8. 反馈与报错

```text
M1-03B3 完成 / 卡在第 __ 节
两个新Character父类 / 单个ASC / 输入四资源 / 无M0业务：正确
合法40/80 / Waiting100/60 / 裸Pawn配置，均在Tests：正确
五变量 / 五函数 / 临时按键，Controller Compile：通过
6.1 原75/100 ↔ 新40/80与15/80，身份 / 属性保持：通过
6.2 L/H确实改旧属性；当前HUD不变；旧身体销毁隔离：通过
6.2 销毁后再生成新名字 / 新40/80：通过
6.3 Unsupported / 当前裸Pawn销毁NoPawn / 新对象恢复：通过
6.4 Waiting / Effect拒绝 / 旧Ready销毁隔离 / 新合法身体恢复：通过
6.5 当前Ready销毁 / NoPawn重建 / 新实例 / 无旧View事件：通过
6.6 先Ready后Possess快照 / 三轮往返 / 三次PIE清理：通过
临时按键 / 五函数 / 五变量已删除；Compile并正常Save：是
原角色最终100/100，原1/2/3、2→3、E/P与基础输入：正常
两个测试资产未放地图 / 未设默认Pawn，资产已保存：是
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟
```

失败时发章节 / 操作顺序 / 预期与实际数值，以及相关日志文字。我按完成时间读取本地记录，无需整理全部日志。接线问题截图需包含整条白线与Target / Input Object；尤其提供H/L目标、GetOrSpawn返回、三个缓存与Class配对。

若新测试身体报输入资源缺失，先看第2.2四项配置；若Spawn失败提供Spawn Class / Transform / Collision Override截图；若旧源影响HUD，发同时包含旧对象Effect result和B1 Snapshot / B2 View的片段。未经明确要求，我先给修改方案，由你手动执行。
