# M4-01D2 可实战的远程守卫

本章交付一个可在 `L_ET_VerticalSlice` 遭遇的远程敌人。它发现玩家后靠近，进入交战距离后停步、转向并用现有远程GA发射，玩家贴近时退开，拉远后恢复追击。预计手动70–105分钟，构建等待另记。[D1验收](Reports/M4_01D1_Verification.md)

## 1. 行为与配置边界

| 行为 | 首轮配置与预期 |
| --- | --- |
| 感知 | 沿用900 cm发现、1100 cm失去半径、70°视锥半角。Controller只接收有效的本地玩家 |
| 走位 | 平面距离650 cm内停步，超过780 cm恢复追击。到约251 cm时进入退让，达到450 cm后退出退让。当前退让目标沿远离玩家的方向，取与玩家相隔650 cm的点 |
| 射击 | 复用 `GA_ET_RangedShot` 的250–1200 cm三维射程、视线检测、发射通知与冷却。进入射击分支后面向玩家，单枚有效命中扣12生命 |
| 中断 | 动作占用时关闭走位与射击门控，目标丢失或死亡时清空黑板目标。发射通知再次校验视线、目标和射程 |

Controller写黑板，Behavior Tree选分支，GA管理冷却和蒙太奇，投射物提交伤害。远程参数只在远程Controller与GA资产填写。近战Controller沿用原资产与默认分支。[黑板协议](AI_BlackboardContract.md)

## 2. 应用源码与正常构建

先保存并关闭UE，CLion可保留。由你在工作区根目录的PowerShell运行以下命令。补丁包含已整理好的C++修改，方便在CLion的 **Git → Local Changes** 逐文件审阅。

```powershell
Set-Location 'E:\UE_Project\Demo'
git status --short
git apply --check --ignore-space-change .\Docs\Patches\M4_01D2_RangedGuardAI.patch
```

`--check`成功时无输出。检查通过后执行下面三行。检查失败时保留终端首条错误与 `git status --short`，由我定位当前文件和D1基线的差异。

```powershell
git apply --ignore-space-change .\Docs\Patches\M4_01D2_RangedGuardAI.patch
git diff --check
git status --short
```

预期7份既有源码变更、3份新源码。新增 `Public/AI/ETBTTask_RangedAttack.h`、`Private/AI/ETBTTask_RangedAttack.cpp` 和 `Tests/ETRangedGuardAITests.cpp`。既有改动集中在Controller、黑板键、射击射程与遮挡规则、GA只读配置入口，以及角色的测试friend。[源码补丁](Patches/M4_01D2_RangedGuardAI.patch)

```powershell
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

脚本的Editor Build成功后刷新CLion的CMake模型，确认三模块和两个新cpp可见。Game Development也通过后打开UE。

## 3. 配置黑板与行为树

### 3.1 黑板

1. 在 `/Game/EmberTrial/AI/Blackboards` 选中 `BB_ETMeleeEnemy`，右键 **Duplicate**，命名 `BB_ETRangedGuard`。打开新副本。
2. 在 **New Key** 下分别创建 **Bool** `CanRangedAttack` 和 **Vector** `RangedRetreatGoal`。逐个选中键，在Details里核对名称并取消 **Instance Synced**。
3. 核对其余四个业务键 `TargetActor`（Object，Base Class=Actor）、`CanPursue`（Bool）、`CanMeleeAttack`（Bool）、`PursuitStopDistance`（Float）。引擎自带的 `SelfActor` 保留。保存新黑板。

### 3.2 行为树接线

1. 在 `/Game/EmberTrial/AI/BehaviorTrees` 空白处右键，选择 **Artificial Intelligence → Behavior Tree**，命名 `BT_ETRangedGuard`。打开资产，选中图中 **ROOT**，在Details的 **Blackboard Asset** 选 `BB_ETRangedGuard`。
2. 从ROOT底部拉线创建 **Selector**。再从Selector底部分别向四个空白位置拉线，依次创建下图四个子节点。布置时从左向右排开，确认节点的执行编号同序。`ET Ranged Attack` 和 `ET Pursuit Move To` 直接从搜索结果选C++任务。
3. 右键Selector本体 → **Add Service → ET Pursuit Decision**。单击绿色附着条，设 **Interval=0.2**、**Random Deviation=0**，高级项 **Call Tick on Search Start=True**、**Restart Timer on Each Activation=False**。
4. 逐个右键前三个Task的本体 → **Add Decorator → Blackboard**。单击各自蓝色附着条按下表填写。最右Wait直接接Selector，设 **Wait Time=0.25**、**Random Deviation=0**。

```text
ROOT [BB_ETRangedGuard]
└── Selector [Service: ET Pursuit Decision]
    ├── Move To             [RangedRetreatGoal Is Set]
    ├── ET Ranged Attack     [CanRangedAttack Is Set]
    ├── ET Pursuit Move To   [CanPursue Is Set]
    └── Wait 0.25
```

| 附着的Task | Blackboard Key | Key Query | Notify Observer | Observer Aborts |
| --- | --- | --- | --- | --- |
| 最左 Move To | `RangedRetreatGoal` | Is Set | On Result Change | Both |
| ET Ranged Attack | `CanRangedAttack` | Is Set | On Result Change | Lower Priority |
| ET Pursuit Move To | `CanPursue` | Is Set | On Result Change | Both |

射击接受请求后 `State.Attacking` 会同步关闭射击门控。`Lower Priority` 允许射击抢占等待，已接受的攻击由GA走完或按受击、死亡规则中断。

**本次操作记录。** 已在编辑器中检查现有 `BT_ETRangedGuard`，ROOT已选择 `BB_ETRangedGuard`，Selector的四个子任务按退让、射击、追击、等待顺序连接。已分别补上三条黑板条件并设置表中的键、中止方式与 `Is Set`。Service为0.2秒、随机偏移0，搜索开始时Tick，重新激活时不重启计时器。Wait为0.25秒、随机偏移0。节点已拉开并保存，编辑器显示All Saved。此处完成的是行为树配置，正式地图的行为仍以第5节实机结果为准。

### 3.3 两个移动任务的Details

分别点选Task本体。`Acceptable Radius` 行左侧为常量，右侧下拉选择黑板绑定，显示 `None` 才使用常量。找不到高级选项时清空Details搜索并展开 **Node** 区域。

| 属性 | 左侧普通 Move To | 右侧 ET Pursuit Move To |
| --- | --- | --- |
| Blackboard Key | `RangedRetreatGoal` | `TargetActor` |
| Acceptable Radius | 常量40，右侧None | 右侧绑定 `PursuitStopDistance` |
| Track Moving Goal | False | True |
| Project Goal Location | True | False |
| Require Navigable End Location | True | True |
| Allow Partial Path / Allow Strafe | 均False | 均False |
| Reach Test Includes Agent Radius / Goal Radius | 均False | 均False |

其余布尔项均用常量。点Save，检查Selector下面有四条执行线，两种Move使用各自的键和到达半径。原近战树可用来核对5.5.4的属性位置。[原生追击任务配置](M1_05B2_BehaviorTreePursuit.md#64-move-to全部配置与ue55属性位置)

## 4. 配置Controller、身体和地图

### 4.1 Controller与守卫身体

1. 在 `/Game/EmberTrial/AI/Controllers` 将 `BP_ETMeleeAIController` Duplicate 为 `BP_ETRangedAIController`。打开 **Class Defaults**，逐项搜索下表的字段，填好后 **Compile → Save**。

| Controller字段 | 最终值 |
| --- | --- |
| Enemy Blackboard Asset | `BB_ETRangedGuard` |
| Enemy Behavior Tree | `BT_ETRangedGuard` |
| Use Ranged Attack | True |
| Pursuit Stop Distance | 650 |
| Pursuit Resume Distance | 780 |
| Ranged Retreat Stop Distance | 450 |

2. 在 `/Game/EmberTrial/Characters/Enemies` 右键 **Blueprint Class → All Classes**，选择 `/Script/EmberTrial.EmberTrialCharacter`，新建 `BP_ETRangedGuard_Native`。打开 **Class Defaults**，按下表填写。UE 5.5.4 复制含原生 GAS 属性集的 Actor 蓝图会让属性集引用失效，此处必须直接从原生类创建。[Epic UE-273844](https://issues.unrealengine.com/issue/UE-273844)

| 身体字段 | 最终值 |
| --- | --- |
| AI Controller Class | `BP_ETRangedAIController` |
| Auto Possess AI / Auto Possess Player | Placed in World or Spawned / Disabled |
| Combat Team | Enemy |
| Ranged Shot Ability Class / Light Attack Ability Class | `GA_ET_RangedShot` / None |
| Initial Health / Initial Max Health | 100 / 100 |

3. 配置新身体的继承组件，再 **Compile → Save**。新Controller使用其继承的 `EnemyPerception`。

| 继承组件 | 配置 |
| --- | --- |
| Mesh | `SKM_Quinn_Simple`，Animation Mode=`Use Animation Blueprint`，Anim Class=`ABP_Quinn`。相对位置 X/Y/Z=`0/0/-90`，Yaw=`-90°`（等价于270°），Pitch和Roll均为0，Scale=`1/1/1`。Collision Presets=`NoCollision`，Can Ever Affect Navigation=False，Visibility Based Anim Tick Option=`Always Tick Pose and Refresh Bones` |
| Capsule | 半径42、高度半值96，Query and Physics，Pawn。Visibility=`Overlap`，WorldStatic / WorldDynamic / Pawn=`Block`，Camera=`Ignore`。其余碰撞项见原近战敌人的胶囊配置 |
| CharacterMovement | Max Walk Speed=220，Orient Rotation to Movement=True，Use Controller Desired Rotation=False |
| CombatLife / CombatPoise | Hit React Ability Class=`GA_ET_HitReact`，Stagger Ability Class=`GA_ET_Stagger` |
| CameraBoom / FollowCamera | Do Collision Test=False，Auto Activate=False |

属性集沿用原生类创建的 `HealthAttributes`、`StaminaAttributes`、`PoiseAttributes`、`CombatAttributes`。胶囊和Mesh的其余设置可对照原近战敌人章节。[近战敌人组件设置](M1_05A_EnemyFoundation.md#64-capsule挡住世界可被命中)

### 4.2 正式地图实例与导航

1. 打开 `L_ET_VerticalSlice`，拖入一份 `BP_ETRangedGuard_Native`，Outliner命名 `RangedGuard_A`。若关卡已有旧副本实例，先记下它的 Transform，再用新身体替换该实例。现有20 m灰盒布局下可从 **X=250、Y=-650、Z=120、Yaw=180** 试摆。该位置和原 `MeleeEnemy_A` 分处房间两侧。若现有地图物件占位，沿负Y侧找一条开阔通道，确保守卫正面有至少约900 cm连续路线可供观察。记录最终XYZ和Yaw。
2. 从Outliner选新实例核对其Class与Controller，留出守卫身后或侧向的导航空间。现有Entry、检查点、近战敌人、训练目标与祭坛继续使用原实例。
3. 关卡菜单 **Build → Build Paths**。在编辑器视口按 **P** 显示导航，核对守卫脚下、玩家接近路线和退让路线有连续绿区。再按P关闭导航显示，执行 **Save All**。

## 5. 验收

### 5.1 正式工程五项专项

隔离工程已通过UE 5.5.4 Editor Build与48项自动化。正式工程应用补丁后，在Editor的Output Log命令输入栏逐条执行，等当前组完成再运行下一条。

```text
Automation RunTests EmberTrial.M4.AI.RangedGuard
Automation RunTests EmberTrial.M4.Combat.RangedShot
Automation RunTests EmberTrial.M4.Combat.Projectile
```

第二条前缀匹配 `RangedShot` 和 `RangedShotAbility`，第三条匹配 `ProjectileDamage` 和 `ProjectileFlight`。核对五个测试的独立结果。新专项覆盖真实Controller、黑板、原生攻击Task、GA与伤害，使用临时世界验证距离门控和遮挡。地图上的感知与寻路在下一组实机验收。

### 5.2 三组实机检查

1. **首次交战。** 在正式地图PIE，从守卫正面进入视锥，在约650–900 cm停住。看守卫自行走近、停步转向并发射。静止让一发命中，观察HUD 100→88和日志 `[M4-01C1] Fired`、`[M4-01B] Damage`。后撤到约780 cm以外、1100 cm以内，应看到它重新追击。需要看分支时，按 **Shift+F1** 释放鼠标，打开 `BT_ETRangedGuard`，在 **Debug Filter** 选对应的 `BP_ETRangedAIController` 实例。
2. **贴近、遮挡与中断。** 等一发结束后，在有导航空间的一侧贴到约250 cm以内，观察先退开、分离后再进入停步／射击。利用现有挡板或临时Box做一次遮挡，确认掩体后没有新增命中，再走出观察恢复。接近时真实左键命中一次，若打断发生在发射通知之前，旧动作无新弹体。暂停时观察世界和弹体冻结，继续后按正常节奏运行。临时Box若用于测试，Stop后删除、Build Paths并Save All。
3. **死亡、重开与原近战。** 用原攻击击败守卫，观察其停止行动和发射。停止PIE再开，确认新守卫恢复100生命，原 `MeleeEnemy_A` 仍能追击并单次扣10。最后检查移动、E与P。首轮走位如有失败，记实际位置、动作、黑板值与相邻日志。

快速定位时，`Blackboard unavailable` 查Controller资产引用，`Ranged blackboard keys invalid` 查两个远程键类型，`Tree start requested ... Accepted=0` 查两份资产的绑定和距离顺序。已显示退让Vector但没有位移时，在行为树看左侧Move To是否执行，再核对Vector键、绿色导航与空间。已停止却无发射时看 `CanRangedAttack`、冷却、遮挡和角色的 `Ranged grant ... Granted=1`。

#### 2026-11-11 首次交战故障记录

- 22:24:38 的正式 PIE 日志显示 `BP_ETRangedAIController_C_0` 成功接管 `BP_ETRangedGuard_C_1`，并初始化 `BB_ETRangedGuard`。同一实例随后报 `Missing gameplay dependencies`，未到 `Ready=1`，因此行为树没有启动，远程能力也没有授予。
- 用 UE 5.5.4 命令行加载已保存的蓝图检查默认对象。Duplicate 得到的 `BP_ETRangedGuard` 的 ASC 有效，四个属性集成员引用均为 `None`。原近战蓝图和直接从 `EmberTrialCharacter` 新建的验证蓝图均持有四个有效引用。这与 UE-273844 的触发方式和表现一致。[Epic UE-273844](https://issues.unrealengine.com/issue/UE-273844)
- 隔离地图使用项目保存的黑板、行为树、远程Controller与GA。直接新建的身体在独立游戏中输出 `Ready=1`、`Accepted=1` 和 `Granted=1`，真实感知在843.8 cm获取玩家，寻路到648.5 cm成功停止，随后发射并命中，玩家生命100→88。默认对象检查和运行日志分别位于本地 `Artifacts/M4_01D2/GuardDependencyInspection.json` 与 `GuardRebuildRuntime.log`。
- 正式工程按第4.1、4.2节新建身体并替换旧实例，然后重测第5.2节第1点。先确认新实例日志有 `Ready=1`、`Accepted=1`、`Granted=1`，再检查追击与命中。上述独立游戏结果覆盖首次交战，正式地图复验以替换后的 PIE 日志为准。

```text
M4-01D2 完成
7旧3新源码 / 双Build / 三索引：
BB与BT / 两个BP / Save All：
RangedGuard / RangedShot / RangedShotAbility / ProjectileDamage / ProjectileFlight：
RangedGuard_A 最终XYZ与Yaw：
真实感知 / 停步转向射击 / HUD100→88 / 后撤重追：
近身退让 / 遮挡与恢复 / 受击或暂停实际结果：
守卫死亡 / Stop重开 / 近战10 / E-P与基础输入：
源码与资产已保存 / 失败或最明显问题：
完成时分 / 实际手动时间 / 构建等待：
```

## 6. Commit / Push

本批可分两次Commit，均由你在CLion按最新Diff重新Stage。

| 批次 | 提交内容 | 英文建议 | 中文建议 |
| --- | --- | --- | --- |
| 验收与操作资料 | D1验收报告、D2指南和源码补丁，以及本轮更新的进度、路线、架构、黑板契约与验收索引 | `docs: verify ranged ability grant and guide ranged guard integration` | `docs: 验收远程能力授予并补充远程守卫接入指南` |
| D2实现 | 第2节10份Core源码，第3–4节四份 `.uasset` 和 `L_ET_VerticalSlice.umap` | `feat(ai): add ranged guard spacing and projectile attacks` | `feat(ai): 实现远程守卫的距离控制与投射物攻击` |

资料提交清单为 `README.md`、`Introduction.md`、`Docs/Progress.md`、`Docs/Backlog.md`、`Docs/Architecture.md`、`Docs/AI_BlackboardContract.md`、`Docs/AcceptanceWorkflow_Options.md`、`Docs/VersionControl.md`、`Docs/M4_01D1_RangedGuardAbilityGrant.md`、`Docs/Reports/M4_01D1_Verification.md`、本指南与本章补丁。

功能提交的资产为 `BB_ETRangedGuard`、`BT_ETRangedGuard`、`BP_ETRangedAIController`、`BP_ETRangedGuard_Native` 与正式地图。旧 Duplicate 副本属于故障样本，留在本地。应用补丁前可用 `git apply --stat .\Docs\Patches\M4_01D2_RangedGuardAI.patch` 列出对应的10份源码。

日志、`Artifacts` 隔离副本、构建产物和CLion生成索引留在本地。原近战BB、BT与蓝图如果出现变更，先看是否误保存。
