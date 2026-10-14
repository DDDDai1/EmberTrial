# EmberTrial：架构与生命周期

版本1.0；更新2026-10-14。**B最终短函数已落地、Editor成功，功能正常按用户反馈；C1编辑器校验已下发待实施。**69Source / 34cpp / 清单46，仅Task.cpp相对A变化；Game最终构建 / include清理合并C1。计划Editor→Core / DataValidation / AIModule / UMGEditor，Runtime不引用Editor；代码仅指南，实际仍Core / UI两个模块。

## 1. 模块边界

~~~mermaid
flowchart LR
    Assembly["Blueprint / 配置装配"] --> Core["EmberTrial：核心 Runtime"]
    Assembly --> UI["EmberTrialUI：UI Runtime"]
    UI -->|公开查询 / 命令 / 事件| Core
    Editor["后期 EmberTrialEditor"] -.依赖运行时数据定义.-> Core
~~~

| 模块 | 当前状态 | 规则 |
|---|---|---|
| EmberTrial | 已有角色、练习动作 / 位移、交互Actor、ASC / 属性集 / 就绪、正式轻攻击Ability / Tag、命中组件 / Notify与原有自动测试 | 不依赖UMG / EmberTrialUI / Editor模块 |
| EmberTrialUI | 入口、LocalPlayerSubsystem、双Widget / Bootstrap与Presenter已实现 / 验收 | 公开UMG / 私有核心和GameplayAbilities依赖；核心仍不依赖UI |
| EmberTrialEditor | 尚未创建 | 真正需要编辑器校验器时再建，禁止进入 Game 目标 |

CLion CodeModel 用于索引；真实编译由 UBT / UHT 执行。每个模块使用自己的 Definitions 和搜索路径，避免混合导入 / 导出宏。

## 2. 对象归属与实施状态

| 对象 | 当前状态 | 所有权 / 后续职责 |
|---|---|---|
| AEmberTrialCharacter / BP_ThirdPersonCharacter | 现有唯一玩家身体，A1 / A2及原功能回归已通过 | 持有原生ASC / 属性子对象；一次初始化 / 就绪契约与真实Effect边界已验证 |
| AEmberTrialGameMode | 项目默认 / ThirdPersonMap 的模板类，回归通过 | M0 对照入口保留 |
| AETGameMode / BP_ETGameMode | 新地图已实际使用，M1-02B 通过 | M1 本局规则与默认类选择，之后接重试 / 通关 |
| AETPlayerController / BP_ETPlayerController | Blueprint 装配、控制 / 结束日志和原功能已通过 | 控制 / Possess 与本地 UI 装配；身体更换时不能保留旧数据订阅 |
| UETUILocalPlayerSubsystem | C1 已实现并验收 | 原生 PlayerControllerChanged / Pawn 委托、立即快照、弱引用和 EndPlay 清理 |
| UETUIBootstrapComponent | 双Widget与唯一Presenter所有权已通过；两重建入口 / 重订阅结束保护已实现 | Controller上创建 / 移除Widget；UPROPERTY持有Presenter与当前Widget；移除前主动解绑 |
| UETLifecycleStatusWidget | C2-01 已验收；Never + DisableNativeTick | 订阅 C1、立即快照、事件 SetText、移除前主动解绑；不缓存 Pawn / 修改玩法 |
| BP_ETLifecycleProbePawn | C2-02 已创建 / 验收，保留 Tests 资产 | 隔离的裸 Pawn，不设默认 / 不放场景；Controller 测试入口已按用户反馈删除 |
| UAbilitySystemComponent / IAbilitySystemInterface | A1 已接入 / 验收 | Character 原生子对象，Owner=Avatar=Character；关联刷新与结束保护，见 ADR-002 |
| UETHealthAttributeSet / GameplayReady | A2已实施 / 验收；C2死亡夹限已接入 | ASC管理Health / MaxHealth；一次初始化 / Ready；State.Dead时Base / Current保持0，死亡不撤销Ready |
| UETGA_LightAttack / GA_ET_LightAttack / GA_ET_EnemyLightAttack | M1-04战斗与M1-05C1共享AI攻击已验收 | 单人ServerOnly / Standalone；校验本身体 / Controller / 阵营 / 动作；玩家25与敌人10独立资源，身体关系查询在Apply前过滤；Montage / Trace / GE生命周期共享，BT自动请求在C2已验收 |
| UETCombatTraceComponent | B1 / C2已验收 | 原生单组件，窗口内Tick / 骨骼后采样、实例身份与去重；CancelActiveAttack立即清序号 / 窗口，开窗死亡已实测Tick0；不依赖GAS / UI |
| UETANS_AttackWindow | M1-04B1已实现 / 验收 | 资产通知只路由Mesh / Montage实例ID / 通知身份，身体运行状态留Trace |
| UETCombatLifeComponent | C1 / C2已验收 | 单身体Health与Ready快照；受击Handle所有权；死亡锁 / 一份Loose Dead Tag先于取消，结束释放；无Tick |
| UETGA_HitReact / GA_ET_HitReact | C1已验收；C2死亡封锁已接入 | ServerOnly离线权威执行，无Controller目标可用；拥有受击Tag，取消正式攻击 / 练习位移；死亡时不能激活 / 重启 |
| BP_ETCombatTrainingTarget | C1已配置 / 验收；C2目标死亡通过 | 独立ASC / 100生命，胶囊Overlap与Quinn；真实损伤自动受击，归零冻结、再次候选TargetDead拒绝 |
| GE_ET_LightAttackDamage | M1-04B2已创建 / 验收 | Instant单Health Add(Base)，由能力SetByCaller.HealthDelta传值，沿用Current / Base夹限 |
| GE_ET_Test_* | A2-02五个Instant测试资产已创建 / 验收 | Tests/Effects保留样例；C1已复用Damage25建立玩家4简单开发损伤入口，留到C2 / D |
| UETHealthPresenter / FETHealthViewData | B1已实现，功能验收通过 | 弱引用数据源、native属性委托句柄、订阅后快照 / Ready状态 / 变化去重 / 清理 |
| UETHealthHUDWidget / WBP_ETHealthHUD | B2已创建 / 验收；Never + DisableNativeTick | 仅订阅Presenter并立即读取快照，事件更新 / 主动解绑，不缓存ASC或业务属性 |
| BP_ETHealthProbeCharacter / BP_ETHealthWaitingCharacter_Rebuilt | 已创建 / B3验收通过 | 原生EmberTrialCharacter子蓝图；40/80与非法100/60，隔离测试 / 不设默认玩家；旧Duplicate Waiting样例停用 |
| AETAIController / BP_ETMeleeEnemy | M1-05A已实现 / 验收；Public AIModule / UHT / 双Build通过 | 管理受控身体 / Ready、弱引用与解绑；敌人直接继承共享原生Character、独立生命 / 正式受击死亡；BodyCharacter避免继承成员遮蔽 |
| 原生EnemyPerception / BB_ETMeleeEnemy / BP_ETMeleeAIController | B1 / B2 / C2已验收，四键BB / 双Build / 26cpp | 感知唯一存活玩家，非同步目标 / 追击 / 停止距离 / 近战资格；Controller90停 / 100恢复 / 100攻击、0.5世界时间重试；BP只配置资源 |
| UETBTService_Pursuit / UETBTTask_PursuitMove / BT_ETMeleeEnemy | M1-05B2已验收，等待父连接补修关闭 | Service每0.2秒请求Controller距离决策；Task继承引擎Move To并保留NodeMemory / 取消 / 清理；BT根Service、左门控Both / Selector右侧Wait0.5秒；已保存运行结构确认父连接正确，失败请求间隔约0.550秒；配置与每敌人运行状态分开 |

**B2阶段历史（已验收，距离随后由C2调整）：**Controller订阅身体Attacking / HitReact / Dead，忙碌与目标失效关门控、取消路径并清速度；150停 / 200恢复采用迟滞，StopDistance由Controller写Float黑板键，Move To读取且不叠加胶囊半径。失去目标保留等待，自身死亡 / Ready撤销 / UnPossess / EndPlay停止整树并解绑。两个辅助头文件实际在Public/AI，cpp为Private/AI；指南建议Private收口，目录差异不改变当前行为，不为本轮增加功能检查。节点无自定义共享可变身体状态，Build.cs保持原依赖。运行已支持追击 / 停走与死亡 / 暂停清理，活动Move受击与有效目标无路径等待均通过；性能测量仍未进行。

M1-05路线采用AI Perception + Blackboard / Behavior Tree，A身体 / 控制权 / 导航、B1有效玩家、B2真实Move To、C1共享攻击 / 玩家受伤均已验收。生命 / 感知订阅各自归属，Ready不代表存活；目标暂停中保留、停止清空。轻攻击ServerOnly且限定离线Standalone，分别校验本地Player与AI控制的Enemy；玩家胶囊支持命中。核心Public AIModule已构建，依旧不依赖UI；C2自动朝向 / BT攻击已通过；D已完成交战链路复盘与证据收尾，没有新增运行改动。详细边界见[M1-05路线](M1_05_EnemyRoadmap.md)。

**C2已实施并验收：**原生Task头实际Public/AI、cpp Private/AI，include使用AI路径；每Controller独立实例 / 弱引用 / Own Spec结束句柄。准备停步转向、接受后锁Yaw；匹配OnAbilityEnded，取消先解绑再取消自己Handle，Serial保护同步重入，Abort与Forced Stop幂等清理。Controller四键 / 0.5世界时间重试、StopTree Guard；BT左攻击Lower Priority避免自身Busy中止，正常收招0.6，中追击Both与右Wait0.5保留。90停 / 100恢复 / 100攻击，真实Trace25cm / 8–14帧不改。53Source / 26cpp，自动伤害 / 取消 / 失败等待 / 生命与暂停清理通过；UI / 伤害模块无反向依赖。具体BT属性与画面按用户实测，未逐字段解析资产或做性能测量。

角色沿用 AEmberTrialCharacter 名称，取代最初计划中的 AETCharacter 占位名，不复制玩家移动实现。理由与迁移边界见 [ADR-001](ADR/ADR_001_FrameworkReuse.md)。

本批 Public 目录与 API 宏开放的是受控类型 / 接口，不开放私有状态。蓝图负责选择资源和组件装配；规则、数据所有权和清理责任需要继续落在明确的 C++ 类型中。

## 3. 初始化与清理原则

- 模块 StartupModule 属于进程级加载，不等于世界 / 角色 / ASC 就绪。
- GameMode 每局随 World 创建；Controller 可更换 Pawn，切图仍须重新装配。
- 本地 UI Subsystem 由 LocalPlayer 自动创建；使用引擎的 PlayerControllerChanged，核心 Controller 不包含 UI 类型。Initialize 补读已存在的 Controller，订阅后立即读取 Pawn；C1 已验收。
- C2-01 已实现：UI Bootstrap 只服务本地玩家，Widget 每次 Construct 先清旧订阅，再订阅 C1 并读当前快照；Widget 不缓存 Pawn。
- C2-01 已实现：Bootstrap 移除 / 重建面板时先调用 StopObserving，再 RemoveFromParent，不等待旧 Widget GC；NativeDestruct 幂等清理。面板不改变输入模式、焦点或暂停，只读 / 不命中鼠标。
- B1已实现：Pawn变化时先解绑旧订阅，再检查新对象；订阅OnGameplayReadyChanged并立即查询，读取快照和订阅增量。B3验证真正不同生命数据源。
- A2-01已实施：Character属性初始化只成功执行一次；ActorInfo与OnGameplayReadyChanged分开。M1-04A正式攻击由Character持有授予Handle，C1受击由CombatLife持有自己的Handle，各自幂等并清理。
- 切图 / EndPlay / UI 关闭各自移除已注册的订阅；LocalPlayer 存活不能成为保留旧 World Actor 的理由。

Controller / Pawn观察、Widget装配 / 重建、真正的新Pawn更换与ASC关联均已验收。Probe是裸Pawn，正式重生仍未实现。ASC取舍见 [ADR-002](ADR/ADR_002_ASCOnCharacter.md)；ActorInfo有效不等于GameplayReady。A2已实现一次设置初值 / 就绪通知 / EndPlay撤销，验证真实Effect的Current / Base范围、最大值策略与修改后Ensure不重置。Ready暂表示最小属性链完成，不表示受控制或存活；正式能力 / 装备 / 存档接入后要扩展就绪前置条件。

B1已实现：Bootstrap强持有唯一Presenter；Presenter先解绑旧Pawn / ASC，再绑定当前角色就绪事件并立即查询，Ready时订阅两属性native事件。DTO只保存展示状态 / 名字 / 数值 / 比例，不保留Actor对象；变化快照发布前提交缓存。Restart只刷新数据源订阅，保持Presenter身份 / View订阅且不写Gameplay；结束Shutdown，GC兜底不广播。日志直接验证Presenter早于Ready、修改后重订阅不重置和单次变化。B1漏写的旧重建入口结束标记已在B2补齐。

B2已实现：原生Health Widget只订阅Presenter DTO并立即快照，不Tick；NativeConstruct只渲染，显式StartObserving管理订阅，NativeDestruct幂等清理。Bootstrap移除前主动解绑，EndPlay先移除View再Shutdown；临时强持有旧Widget验证其存活时也不再收事件。Designer布局 / Class装配由蓝图负责。17轮PIE中41组创建清理，无解绑后View更新，早于Ready订阅和晚加入已有值快照均有直接证据。

B3已验收：原生Character子蓝图共用现有核心实现，各自ASC / 属性 / 初值独立；真实GE修改旧身体属性时当前HUD不变，旧 / 当前对象销毁、Unsupported / Waiting / None、新实例与Ready先于Possess的快照 / 清理通过。临时Controller与Level Blueprint入口已清理；没有生产Character测试开关或伪造Ready。新Rebuilt样例从原生类创建，最终保留非法100/60。测试只覆盖同步初始化的两种订阅顺序，未来异步能力 / 装备就绪须另测。

## 4. 过渡基线

**M1-05C1已进入运行基线：**共享身体EETCombatTeam / 只读关系查询，Player可伤害Enemy或Neutral训练身体、Enemy只伤害Player、Neutral不主动造成轻攻击伤害；不据Controller临时存在与否推断敌友。Ability依赖身体公开查询，不依赖具体ETAIController / UI；阵营分类尚未接GenericTeamID。独立Enemy GA10和玩家25共享原生实现，C1真实命中 / 同方拒绝与取消清理通过；[证据边界](Reports/M1_05C1_Verification.md)区分直接日志与用户反馈。C2已用同一查询接入自动攻击决策 / 任务循环；[D复盘](M1_05D_EnemyCombatReview.md)说明AI提出意图、能力决定接受、Trace决定候选和GE结算的边界。

PracticeAction、练习目标、原型提示与暂停菜单现在仍在使用。它们帮助验证模块和对象变化后的行为。正式能力 / UI 接入时逐步迁移：同一动作只有一个可信状态源，正式 GAS 不与练习动作状态机并行驱动同一角色。

[M1-04A实现](M1_04A_LightAttackAbility.md)已验收：UETGA_LightAttack与原生Ability.Attack.Light / State.Attacking，Character在Ready后保存自己授予的Spec Handle；攻击由Montage Task完成 / 取消并让GAS清理Owned Tag。左键 / 1走正式能力，2暂留练习闪避且互斥；3在A阶段为过渡练习受击，C1已改正式命令。移动 / 跳跃与E同时查询两种状态；Practice外部Montage条件已改为仍活动的实例，停止后的淡出允许接续。B2真实伤害、C1正式受击与C2一次死亡均已接入并验收。

[B1实现](M1_04B1_AttackTrace.md)已验收：Character唯一CombatTrace持有身体Mesh、准确Montage实例ID、AttackSerial和弱目标集合，原生Notify只路由通知；Ability登记准确播放实例，结束先撤销检测 / 委托再Super停止Montage。窗口内Tick在Mesh更新后采样，Visibility扫掠以目标Overlap / 世界Block区分，身体至拳检查及遇阻关闭防隔墙命中。每次攻击去重、窗口开关不清集合；候选交Ability，组件不依赖GAS / UI，B1不扣血。

[B2实现](M1_04B2_GameplayEffectDamage.md)已验收：Ability校验单人来源、合法Ready非零血目标与GE契约；攻击者ASC为每个目标单独创建Spec / Hit Context，以SetByCaller.HealthDelta传负健康增量并应用到目标ASC，沿用属性Base / Current夹限。Instant成功使用WasSuccessfullyApplied，Apply后读取重新验证的弱引用；GE由资产配置、正伤害数值由GA配置，不新增Trace / Widget扣血逻辑。

[C1已实现](M1_04C1_HitReaction.md)：原生单CombatLife独立观察本身体Health，Ready后一次订阅 / 授予受击能力，撤销前清理自己拥有的句柄；只立即读取直接Health减量的GEModData，不保存临时回调指针。HitReact能力拥有State.HitReact、取消攻击与Practice位移，Montage完成 / 取消后GAS清理Tag；不操作生命或UI。ServerOnly用于离线权威身体，支持无Controller目标，未实现网络同步。3已改正式命令、4已复用简单损伤图；实机及清理见[C1结果](Reports/M1_04C1_Verification.md)。

[C2已验收](M1_04C2_DeathState.md)：CombatLife先提交bDead / 自己的一份Loose State.Dead Tag，再立即关闭Trace / 取消当前身体能力和练习位移；死亡封锁独立于攻击 / 受击完成。Health归零不受GEModData过滤限制，Ready快照0也需提交；AttributeSet按Dead Tag夹Health Base / Current为0，UI保持Ready0。结束仅移除本组件拥有的一份Tag，不重置死亡锁。主体九轮PIE / 七组一次死亡与Tag释放、真实闪避中取消等规则已核对；10月7日助手补实测WindowOpen后死亡，在同毫秒主动WindowClose(AttackEnd)、Tick0、取消能力并封锁，结束释放Tag。见[C2核查](Reports/M1_04C2_Verification.md)与[D链路汇总](M1_04D_CombatReview.md)。冻结姿势是占位，倒地动画 / 新身体重试后续接入。

M1-02B使用地图级GameMode Override；M1-06A已把Game Default Map切到菜单、Editor Startup Map切到正式地图，两张新地图进入Cook列表。真正Cook / 独立包运行在M1-07验证，当前旧M0包仍对应其原版本，不代表新菜单与源码已打包。

## 5. 文档与证据

- [M1-02A 模块验收](Reports/M1_02A_Verification.md)：已通过。
- [M1-02B Framework 验收](Reports/M1_02B_Verification.md)：已通过；源码写法、直接日志与用户反馈的边界已记录。
- [M1-02C1 验收](Reports/M1_02C1_Verification.md)：已通过，直接日志与用户实机反馈区分记录。
- [M1-02C2-01 验收](Reports/M1_02C2_01_Verification.md)：已通过；五份文件、原生 Tick 契约、三轮 PIE / 旧 Widget 解绑与反馈边界已核对。
- [C2-02 / M1-02 总验收](Reports/M1_02_Verification.md)：已通过；真实新实例切换、旧 / 当前对象销毁、None 后恢复与清理边界已记录。
- [M1-03A1 验收](Reports/M1_03A1_Verification.md)：已通过；源码 / receipt / 刷新与清理日志及用户反馈边界已记录。
- [M1-03A2-01验收](Reports/M1_03A2_01_Verification.md)：属性 / 初始化 / GameplayReady已通过，日志实际配置差异与用户最终反馈分别记录。
- [M1-03A2总验收](Reports/M1_03A2_Verification.md)：真实Effect / Current与Base动态边界已通过，直接日志与最终用户恢复反馈区分记录。
- [M1-03B1验收](Reports/M1_03B1_Verification.md)：功能通过，直接日志 / 用户反馈与源码补项区分记录；原操作文档保留。
- [M1-03B2验收](Reports/M1_03B2_Verification.md)：血条View / 晚加入 / 存活旧Widget解绑与结束保护已通过，直接记录与用户反馈区分。
- [M1-03B3验收](Reports/M1_03B3_Verification.md)：主体与Waiting局部收尾通过，非法100/60 / 合法40/80路径和归属 / 数据流已核对，B阶段补项全部关闭。
- [M1-03C数据流汇总](M1_03C_DataFlowReview.md)与[总验收](Reports/M1_03_Verification.md)：当时33份源码 / 清单29项资产不变，复用A / B已接受证据；两条时序、引用归属、源码入口与专项触发清单已整理，无需重复搭旧入口。
- [M1-04A验收](Reports/M1_04A_Verification.md)：37份源码、双目标receipt / 18cpp索引、13组攻击开始结束 / 标签清理 / 取消 / 暂停与Lifecycle Success核对；用户实机全项通过。清单32项，实际新增GA / IA两份，另补登既有IMC；B1方案未写入源码。
- [M1-04B1验收](Reports/M1_04B1_Verification.md)：当时41份源码 / 双receipt / 20cpp索引，19组登记清理、去重 / 取消 / 遮挡日志与全项用户反馈通过，资产清单33项；B1只产生命中候选，后续B2才接伤害。验收没有代写工程Source。
- [M1-04B2验收](Reports/M1_04B2_Verification.md)：41份源码中四文件与指南一致，其余37份不变；双receipt / 20cpp、10条真实伤害与拒绝 / 配置 / 窗口内暂停、23组清理核对，全项反馈通过，清单34项；真实伤害完成、受击 / 死亡待C。
- UE5.5.4测试资产创建约定：含原生AttributeSet默认子对象的Character蓝图，从原生类创建新资产；本机Duplicate导致旧Waiting默认成员指针失效，与Epic UE-273844一致。保留原生子对象 / 归属保护，不能移除检查绕过问题；[局部收尾](M1_03B3_Closure.md)已实施通过，新样例替代旧失效资产。
- 后续在 Pawn 更换、GameplayReady 与 UI 数据流真实接入后补充时序和测试结果；图中计划节点不记作已完成功能。

## 8. M1-06流程（A / B / C / D均已验收）

A已将菜单世界与战斗世界分开：Core Menu GameMode不生成Pawn / Spectator / HUD，Controller Class由BP装配；UI模块Menu Controller持有唯一菜单Widget、UI Only / 焦点，Widget只发命令 / 渲染状态。地图使用软引用并加入Cook列表；游戏Controller BeginPlay恢复Game Only、隐藏鼠标、设置首次点击捕获策略并Flush。菜单→战斗、旧页清理 / 新HUD100与PIE退出有日志，具体键鼠操作据用户反馈。核心不依赖UI，现有LocalPlayer / Presenter重绑逻辑沿用。A发现的默认InitializeHUDForPlayer空AHUD Warning已在B用菜单专属空覆盖关闭，不调用Super，未改变战斗HUD流程。

B已实现：GameInstanceSubsystem保存地图包名 / 当前请求，一次请求校验后解除暂停再OpenLevel，所属GI的PostLoad确认 / 失败释放门控；仅当前请求使用OriginWorld弱引用，结束即释放。正式GameMode登记地图，支持直接PIE；菜单开始路由统一服务。Life在原Death sealed之后广播原生事件，Controller FailureFlow订阅当前Life并查死亡快照，一次NextTick让旧死亡 / 初始输入回调结束后再显示失败View。Widget只发意图，UI组件管理暂停 / 焦点、旧Pawn / 结束解绑与取消一次任务；旧P函数新增死亡拒绝，未把UI并入Core或改HUD数据流。[B报告](Reports/M1_06B_Verification.md)核对六新 / 七旧Source、双构建 / 32cpp、三次Retry和四次MainMenu到达、新100及旧事件清理；None预检恢复已测，真实引擎TravelFailure尚未实测。

C已按[指南](M1_06C_SessionCheckpoints.md)实施两新 / 八旧Source，稳定Entry / Checkpoint_A、GameMode出生选择与菜单继续已验收。RunSession区分已确认地图 / ID与待定意图；FindPlayerStart在生成Pawn前使用待定ID，绕过旧StartSpot / 无有效Start时WorldSettings原点兜底。唯一Entry是地图契约，缺失检查点ID回入口；StartPlay报告实际选择，PostLoad成功后确认快照。NewRun确认后重置Entry，Retry / Continue沿用最近ID，不保存Transform / Actor / ASC；真正引擎TravelFailure仍未实机专项测试。

原生CheckpointStart继承PlayerStart，Sphere只接受本地受控、存活且Ready身体，重复ID / 同ID不激活；一次NextTick补出生重叠，原生事件更新占位Label，结束清Timer / Overlap / Session委托，无常驻Tick。MainMenu Controller沿用一次请求 / UI输入边界；View只发继续意图和渲染资格。菜单BeginPlay早于PostLoad清待定请求，继续资格查询读取已确认快照，点击另受pending门控，避免到达菜单后按钮永久禁用。

失效ID检查复用仅编辑器PIE有效的GameMode开发命令，不搭Controller验收图或在PIE编译测试蓝图；真实死亡 / Retry验证Entry回退与下一次请求修正。三Retry / 两Continue / 三NewRun均确认到达，八组检查点 / HUD / Life清理闭合，详见[C报告](Reports/M1_06C_Verification.md)。重复ID / 缺失Entry保护未单独实测。检查点只在当前进程有效，版本化磁盘存档留M5；[D指南](M1_06D_PauseFlow.md)已实施 / 验收，将旧Pawn暂停业务迁到既有Controller FailureFlow，统一输入 / 暂停所有者，见[D报告](Reports/M1_06D_Verification.md)。

D已验收（[手动指南](M1_06D_PauseFlow.md) / [报告](Reports/M1_06D_Verification.md)）：保留原生FailureFlow类名与Controller单组件装配，统一持有互斥Pause / Failure，UIBootstrap仍专管HUD；Core ETPlayerController仅发布原生P释放意图和无UMG类型的Blueprint通知，UI模块显式依赖InputCore / SlateCore，Widget只发继续 / 菜单意图。角色删除旧菜单实例 / 布尔缓存 /输入切换 / 清理，两交互条件改读World暂停；Controller Blueprint通知仅桥接当前Pawn.RefreshCandidate。Pause KeyUp处理UI Only输入、按钮Down And Up、背景回焦，正常Resume恢复同一世界与输入；返回菜单 / Retry仍调用RunSession，切图EndPlay只清理。失败sealed优先、当前身体身份检查、一次NextTick与已暂停时直接接管保护分工明确；已暂停时直接接管仍只核对静态设计，未冒充运行测试。两新 / 六旧Source、四旧资产已实施，当前69Source / 34cpp / 清单46。D日志中12组Flow绑定 / 清理、15组Pause显示 / 移除、一次真实Failure及三次返回菜单闭合；具体键鼠操作按用户反馈。当前阶段独立包已在[M1-07A](Reports/M1_07A_Verification.md)通过，性能采样留[M1-07B](M1_07B_InsightsBaseline.md)完成。

## 9. M1-07A包基线与B采样

A已生成Win64 Development Attempt01，UAT Build / Cook / Stage / Package / Archive成功，三次独立启动保留日志；正式Menu、真实伤害、Checkpoint_A Retry / Continue、新进程Entry及UI清理闭合。游戏Source / 46项资产与D逐份哈希一致，B原Trace已完成CPU / GPU待机、实际暂停与Retry分析，Combat02真实交战已采集 / 分析，Move150 / Controller90绑定回归已手动补修，最终精简版Editor通过；旧包仍保留原回归，后续C3统一Attempt02。B工具只增加本次进程CVar / Trace、Region标记与本地证据目录，不污染Core / UI模块、不增加Blueprint测试图。UE5.5.4的RegionChannel独立于BookmarkChannel，工具显式开启两者；命令与帧 / Timer语义按本机代码和5.5官方说明核对。GameplayCueNotifyPaths未配置导致/Game回退扫描，功能正常，C再评估配置范围，不从Warning直接推断性能根因。

B测量口径：主循环相邻Tick开始时间与GPU场景Scope分别报告，不叠加Inclusive / 线程总时间；Scene GPU Scope不冒充完整GPU Frame。现有Slate约0.169ms / 调用，没有证据要求改HUD为轮询或重写Presenter。原Combat01只有玩家动作，Combat02死亡前19秒已补齐真实AI活动；详见[B报告](Reports/M1_07B_Verification.md)。

B新增发现：AcceptableRadius为FValueOrBBKey_Float，先SetKey后float赋值会复制None键；默认常量必须在绑定之前。BT节点应绑定PursuitStopDistance，实际Move应和Controller Stop90一致。这是配置 / 默认顺序回归，不是已证明的帧率瓶颈；8.2绑定修复已执行，实际Radius90与新自主攻击样本成立；C1将此键契约纳入有限校验。

2026-10-14函数收尾：00:26自主接近攻击样本成立。最终OnTaskFinished只用通用Controller / Pawn / Actor取目标和中心距离，清理前缓存数值 / 名称、无条件Super清理、清理后输出原始Result / 距离 / 半径，删除一次Nav / 地面 / GAS资格 / 网络模式快照。此版本已手动落地 / Editor构建，用户报告功能正常；Game构建转C1。旧残余原因未唯一定位，日志本身不声称修复玩法。

## 10. M1-07C1编辑器校验设计（尚未实施）

[指南](M1_07C1_EditorValidation.md)新增独立EmberTrialEditor模块与原生UEditorValidatorBase。Editor Target加载它，Game Target保持Core / UI；DataValidation插件限定Editor。Subsystem启动自动发现原生Validator，无重复手动注册。规则分别遍历BT任务、检查Float键 / 绑定 / 三个常量开关，及读取Widget设计树 / 父类树匹配必需BindWidget名称与类型。仅限正式BT / 项目Widget与专用Validation副本；不检查既有非法健康样例、不创建运行世界或修改资产。规则ID / 路径 / 修复提示经UE的Message Log输出，项目Valid标记证明入口运行。

C1未创建实际模块 / Probe或执行构建。校验不证明运行输入 / UI订阅 / 导航成功，不检查完整BT语义；资源引用、Controller距离关系 / 输入 / 地图规则留C2。专项负例只覆盖ET-AI-003断键→恢复，未宣称每条规则都有错误样例。C3出新包并复用短流程；工具成本按实际用户反馈登记。
