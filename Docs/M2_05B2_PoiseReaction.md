# M2-05B2：韧性耗尽、恢复与占位反应

2026-10-22，UE5.5.4。**[B2已验收](Reports/M2_05B2_Verification.md)，三项测试最终Success，闪退补修已关闭。**本章保留交付规则与配置 / 检查流程，重复源码改为实际文件入口；无需重做步骤。下一批[C1施放素材](M2_05C1_BreakPulseAnimation.md)已下发。

## 1. 目标、规则与用时

本批把Poise100→40→0真正接入已验收的破势状态：0事件只排一次处理，GAS取消旧动作并持有Stunned，播放既有受击Montage作占位，持续1.2游戏秒。正常结束或存活时外部取消后，延后用Instant GE恢复到**结束时的MaxPoise**；不硬填100。Poise只管韧性，本批不新增生命伤害、UI、按键、范围查询或冷却。

| 情况 | 规则 |
| --- | --- |
| Poise仍大于0 | 只改变数值，不破势、不自动恢复 |
| 首次归零 / 出生0 / 恢复观察时已为0 | 读取快照，排一次世界Timer派发；不在GE属性回调内激活能力 |
| 已破势再写0 / 再请求 | 不重启、不延长；正常入口拒绝忙碌时的开发扣减 |
| 占位动画先播放完 | 状态仍按1.2秒结束；短暂回待机但不能行动是当前占位表现 |
| 自然结束 / 存活取消 | 下一次派发回填当前MaxPoise一次；回填事件不再次触发破势 |
| 死亡 / EndPlay / Deactivate | 不回填；死亡最高优先级，旧Timer / 委托不影响新身体 |
| Deactivate后Activate(false) | 不重置数值；若仍0，根据快照重新进入一次完整反应 |
| Pause | 世界Timer与Montage冻结，继续后完成剩余过程；开发修改拒绝暂停请求 |
| 外部持有Stunned | 当前归零尝试可拒绝；外部状态撤销时补判一次，无常驻失败轮询 |
| 其他配置 / 激活失败 | 保留实际数值，不凭空回填；修正配置并重开PIE，日志定位，不无限重试 |

原预计手动90–140分钟，实际代码 / 配置70、检查20、等待5；未单独拆分排错，不追加工时。当前131 Source / 70cpp、Core50 / UI17 / Editor3、16测试，来源65项。本批复用素材，最终Blender动画替换不改变数值和状态归属。

## 2. 先保存关闭UE，再按清单操作

以下是已完成的12份Source清单。学习实现见第3–6节链接；不要按旧步骤再次创建类或回贴旧版本覆盖后续源码。历史构建 / 配置流程保留供查，不用Live Coding。

1. Stop PIE → Save All → 关闭UE；保存CLion当前文件。
2. 新建下表三文件，补全第3 / 6节。`Effects`必须位于`Abilities`内，公共 / 私有目录不能串位。
3. 第4节是**两个已有Component文件完整替换**，不是在末尾追加第二套类 / 函数；类名、模块宏、默认子对象名保持。
4. 第5节其余七旧文件按“原文→替换后”逐处操作，每处应只匹配一次。保留其余代码，不重新创建B1五文件。

所有路径均相对于仓库根`E:\UE_Project\Demo`；新建空文件后先写完整代码，再统一Build。保持UTF-8，生成头仍为最后一个include。

| 操作 | 路径 |
| --- | --- |
| 新建 | `EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETPoiseDeltaEffect.h` |
| 新建 | `EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETPoiseDeltaEffect.cpp` |
| 新建 | `EmberTrial/Source/EmberTrial/Tests/ETPoiseReactionTests.cpp` |
| 完整替换既有文件 | `EmberTrial/Source/EmberTrial/Public/Combat/ETCombatPoiseComponent.h` |
| 完整替换既有文件 | `EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp` |
| 局部修改既有文件 | `EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h` |
| 局部修改既有文件 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp` |
| 局部修改既有文件 | `EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Stagger.h` |
| 局部修改既有文件 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Stagger.cpp` |
| 局部修改既有文件 | `EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h` |
| 局部修改既有文件 | `EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp` |
| 局部修改既有文件 | `EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp` |

共**三新 / 九旧Source**。不改Build.cs / Target / uproject / UI模块 / 输入资源 / 地图；现有GAS依赖足够。

## 3. 新建原生Poise增量GE

实际[ETPoiseDeltaEffect.h](../EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETPoiseDeltaEffect.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETPoiseDeltaEffect.cpp)：单Instant / Poise AddBase / SetByCaller.PoiseDelta，正负数统一通道，夹限在属性集。未来正式范围技能由攻击者ASC建Spec，不调用PIE命令。

## 4. Component：事件、一次性派发与清理

实际[ETCombatPoiseComponent.h](../EmberTrial/Source/EmberTrial/Public/Combat/ETCombatPoiseComponent.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp)：观察当前来源 / 代次、归零只尝试一次、Spec结束后按即时Max回填。Stop先撤销身份 / Timer / 委托，再取消自己的能力；无常驻Tick / 失败轮询。bZeroAttempted与bRestorePending属于事务记录，不替代GAS状态。

## 5. 七份既有文件接入索引

- [Tags.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp)：唯一原生Poise增量Tag。
- [Stagger.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Stagger.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Stagger.cpp)：Montage表现托管，成功接管标记供结束观察；Timer仍决定1.2秒，不把动画Completed绑定成提前解锁。
- [Character.h](../EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)：ET_Poise只在Editor / PIE接受合法查询和负增量，暂停 / 忙碌 / 无敌 / 死亡拒绝修改。
- [Validator](../EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp)：原ET-STAGGER规则增加Montage引用 / 时长要求，不执行能力。

完整运行源码由Source维护，历史版本在Git中；文档不复制第二份逐渐过时的函数实现。

## 6. 自动化：11场景真实GE与销毁

实际[ETPoiseReactionTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETPoiseReactionTests.cpp)保留11场景，修复后11:16:37最终Success；B1状态 / 基础专项分别11:16:55 / 11:17:14 Success。具体覆盖见[报告](Reports/M2_05B2_Verification.md)。

<a id="61-闪退补修2026-10-22"></a>

### 6.1 闪退补修：2026-10-22，已关闭

原测试Destroy身体、推进世界后还经旧ASC读取属性，11:06:50发生访问冲突，属于助手提供的测试生命周期错误。修正版销毁前查Current / Base，销毁后只查按值持有的原生事件记录和独立Timer句柄；两个真实Destroy场景保留，不延长身体生命周期，不留下捕获局部引用的委托。B1测试继续直接核对DurationTimer与Spec清理。

用户已手动应用、正常双目标构建并完成三专项Success，无需再次执行旧补修步骤。不要用裸指针非空判断或跳过销毁断言代替正确的生命周期观察。

## 7. 关闭UE后正常双目标Build与索引

保存12份Source，确认UE关闭。PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

第一条脚本包含Editor Build和索引生成；成功后CLion Reload CMake Project。应为Core50 / UI17 / Editor3，新GE和新测试属于Core；Game Build是第二个目标，不用CMake的默认Build代替UBT。

编译失败先停在首个C++ / UHT错误，贴文件、行号、首条错误与约20行上下文，不只发最后的exit code 6。不要删Intermediate / Saved或重新创建工程。

## 8. UE资源装配：只配置既有GA

本批没有新uasset，也不创建新的Stagger Ability、组件或Montage。使用已存在且名称含`HitResult`的资源，**不是猜成HitReact的新名字**。

1. 启动UE，打开内容浏览器`/Game/EmberTrial/Abilities/GA_ET_Stagger`，点击顶部**Class Defaults（类默认值）**，不是Class Settings。
2. Details搜索`Stagger Montage`，把引用设为：

```text
/Game/EmberTrial/Animations/Practice/Montages/AM_M0_HitResult_01_Montage
```

3. 若下拉找不到，先清除搜索，在内容浏览器定位`EmberTrial/Animations/Practice/Montages`，选中`AM_M0_HitResult_01_Montage`，回GA用引用旁“使用所选资产”箭头。不要修改该Montage本身、AS、Slot或Notify；它仍供旧普通受击使用。
4. **Stagger Duration=1.2**，父类ETGA_Stagger，InstancedPerActor / ServerOnly / ReplicateNo，Cost / Cooldown=None；原Tags和Cancel / Block各四项保持，不增减。
5. Event Graph保持空，不加播放动画、Delay、填Poise或清Tag节点。Compile → Save。
6. 三正式身体的继承CombatPoise仍各单份、Stagger Ability Class=GA_ET_Stagger、Auto Activate=True。只确认现有配置，**不Add Component**，不改CombatLife的HitReact Class。
7. 在内容浏览器依次选新配置后的GA、玩家`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`、训练`/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget`、敌人`/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy`，右键Asset Actions → Validate Assets（验证资产）。每个应有本项目[M1-07C2] Valid / Issues0。

若ET-STAGGER-001 / 002，先检查新Montage引用是否保存；本批HasValidConfiguration增加了这个条件。若只继承新属性尚未赋值，旧GA即使蓝图Compile成功也不能视为玩法配置有效。

## 9. 分层检查：自动化三项，人工10–15分钟

### 9.1 先跑三项自动化

非PIE的Output Log逐条执行，上一条最终完成后再输下一条：

```text
Automation RunTests EmberTrial.M2.Combat.PoiseReaction
Automation RunTests EmberTrial.M2.Combat.StaggerAbility
Automation RunTests EmberTrial.M2.Combat.PoiseFoundation
```

三项各Success。新测试管归零 / 回填 / 取消 / 死亡 / 旧回调，B1原测试管四旧动作接管和Tag / Timer，Foundation管夹限 / 初始化；本次不要求重跑其余13项，也不另搭临时蓝图图。它们不替代下方真正输入与动画表现检查。

### 9.2 玩家真实归零、一次反应与恢复

1. 正式地图新Play，避开敌人，确认生命 / 体力100和次数3。PIE视口按`~`打开**游戏控制台**，不是PowerShell；每条Enter执行，可关控制台观察。
2. 按顺序输入，第一条只查当前值：

```text
ke BP_ThirdPersonCharacter_C ET_Poise 0
ke BP_ThirdPersonCharacter_C ET_Poise -60
```

3. Output Log应见`[M2-05B2] Poise dev`，分别100/100、40/100；第一次扣减不播放破势，不改生命 / 体力 / 次数。
4. 再输入一次同样的`ke BP_ThirdPersonCharacter_C ET_Poise -60`，关控制台。归零后应一次Depleted Accepted1、Stagger begin，并看到身体播放既有受击占位动作，逻辑结束后可行动。短动画早于1.2秒结束是预期；不要求手掐何时按完全部门控键。
5. 等约2秒后查询`ke BP_ThirdPersonCharacter_C ET_Poise 0`，应恢复100/100，且只一条Restore Applied1。再次两次-60可以开启独立下一轮，不能自动无限循环。
6. 控制台`Called succeeded`只表示函数已派发；请求是否生效看Applied、Depleted Accepted与后续状态日志。Delta0查询Applied0是正常的；暂停 / 死亡 / 已Stunned / 短无敌中的开发扣减会拒绝。

### 9.3 敌人表现与最小旧基线

1. 玩家保持安全距离；地图仍只有一名正式敌人。游戏控制台依次两次：

```text
ke BP_ETMeleeEnemy_C ET_Poise -60
ke BP_ETMeleeEnemy_C ET_Poise -60
```

2. 应分别40 / 0，敌人一次占位反应，活动追击停走，结束回填后重新判断追击 / 攻击；玩家HUD生命不因Poise GE下降。敌人真实攻击仍可造成10生命伤害，这与Poise测试独立。
3. 玩家左键真实25、正式2、4伤害后Q、E提示、P一种继续方式、移动转向跳跃短查即可。无需再重演全部死亡 / 三次暂停 / 多Pawn矩阵，精确死亡和EndPlay时序已由自动化覆盖。
4. Stop再Play一次，新身体Poise100/100、资源仍双100与3/3、单HUD，无旧Depleted / Restore继续写新身体；结束PIE，回正式地图Save All。

本批不要求实机凑“零值回调与死亡恰好同帧”，也不要求临时改出生0；这两项已在独立世界自动化中检查，避免更改三正式身体的最终配置。

### 9.4 最终配置

GA Duration1.2、Stagger Montage正确；三个身体100/100与原Class / AutoActivate保持，玩家伤害25 / 敌人10 / Healing3/3，slomo1 / Debug False，Source与资产保存。不保留临时资源、按键或关卡实例；本章没有新输入，**F还不能施放范围技能**。

## 10. 完成反馈

```text
M2-05B2 完成
三新 / 九旧Source；关闭UE后Editor Build / Core50-UI17-Editor3索引 / Game Build：
GA复用AM_M0_HitResult_01_Montage / Duration1.2 / 原Tags；四资产Compile-Save-Valid：
PoiseReaction / StaggerAbility / PoiseFoundation各一次：
玩家100→40→0一次反应 / 结束100 / 下轮独立；生命与其他资源隔离：
敌人归零占位动作 / 追击中断与恢复 / 玩家HUD隔离：
Stop再开新Poise100 / 单HUD与清理 / 原能力-E-P-基础输入短基线：
Source资产Save All、默认值 / slomo1 / Debug False恢复：
失败或最明显占位问题（无填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

我读取日志核对，不要求你逐条抄记录或手掐时间点。报错用文字首条错误与上下文；动画 / 资源问题给当前GA Details、Montage引用与角色截图，时序难描述时短视频。无待补项后保存B2报告并下发C1施放素材指南。

## 11. 复盘时能解释的设计

- 为什么不把归零逻辑塞进AttributeSet：夹限与行为所有权分开，属性集不依赖动画 / AI，真实GE时序也更清楚。
- 为什么需要延后：GE / GAS可能持有ScopeLock，属性回调内强行取消再激活容易和旧动作叠加；一次性派发合并同轮事件。
- 为什么结束再回填要看当前来源和存活：回调可能晚到、组件可能停用、身体可能死亡；不能靠“Actor尚未GC”证明业务仍有效。
- 为什么动画结束不决定破势时长：占位素材长度与玩法配置分离；后续替换资源不用改门控和数值恢复。
- 为什么不宣称性能提升：本批只明确无常驻Tick、无失败轮询 / 扫描，没有新Insights数据就不填FPS收益。

本地UE5.5.4核对了FAbilityEndedData.AbilitySpecHandle / AbilityThatEnded、ASC.OnAbilityEnded在Spec ActiveCount递减后广播、SetTimerForNextTick返回句柄与PlayMontageAndWait的任务托管 / 按拥有者停Montage。静态核对不等于本批已经编译或运行。

## 12. 本批 Commit / Push（固定放在最后）

B2工作区已验收。核对期间用户创建了`98be3e0`，但该提交只新增以下三个**0字节文件**，完整实现仍在工作区；其他九旧Source / GA / 文档也尚未提交。当前暂存区为空，不能只修改Commit信息再提交，须先Stage本节全部最新差异，无需回滚已有提交。

CLion启用Staging Area时：打开Commit面板 → 在**Unstaged**中选择本节文件 → 右键**Stage** → 确认移到Staged → 点开三个原空文件，看到完整代码 → Commit → Push。若使用普通Changes勾选模式，勾选全部应交最新差异并检查预览。

```text
EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETPoiseDeltaEffect.h
EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETPoiseDeltaEffect.cpp
EmberTrial/Source/EmberTrial/Tests/ETPoiseReactionTests.cpp
```

同次Stage这九旧Source：

```text
EmberTrial/Source/EmberTrial/Public/Combat/ETCombatPoiseComponent.h
EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Stagger.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Stagger.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp
```

本批唯一UE资产差异与当前全部待提交文档：

```text
EmberTrial/Content/EmberTrial/Abilities/GA_ET_Stagger.uasset
Docs/Reports/M2_05B1_Verification.md
Docs/Reports/M2_05B2_Verification.md
Docs/M2_05B1_StaggerState.md
Docs/M2_05B2_PoiseReaction.md
Docs/M2_05C1_BreakPulseAnimation.md
Docs/Data/M2_05C1_BreakPulsePoseValues.json
Docs/AssetManifest.csv
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Docs/M2_Roadmap.md
Docs/M2_05_BreakPulseRoadmap.md
README.md
Introduction.md
```

```text
feat: complete poise reaction and recovery
```

三身体、旧HitResult AS / AM、地图、Rig / Skeleton、UI / 输入 / Config本批无字节差异，不强制重存。C1三份UE素材尚未创建，制作并验收后按[C1末尾](M2_05C1_BreakPulseAnimation.md#9-本批-commit--push固定放在最后)另交，可先Push B2。

CLion刷新Changes → 重新Stage全部最新完整修改 → 检查Staged Diff与LFS → Commit → Push。Raw / 日志 / 崩溃转储 / 缓存 / 生成CodeModel / Saved / Binaries / Intermediate与包体留本地，不Force Add；用户操作Git，助手未核验远端或代改索引。
