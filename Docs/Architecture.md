# EmberTrial：架构与生命周期

更新2026-11-04。M3装备闭环、展示查询与[04C1上下文 / 事务](Reports/M3_04C1_Verification.md)已验收；[C2页面确认](M3_04C2_RelicConfirmationUI.md)仅准备指南。实际证据与工时见[Progress](Progress.md)。

## 1. 模块与构建边界

~~~mermaid
flowchart LR
    Assembly["Blueprint / 资源配置"] --> Core["EmberTrial · Runtime"]
    Assembly --> UI["EmberTrialUI · Runtime"]
    UI -->|查询 / 命令 / 事件| Core
    Editor["EmberTrialEditor · Editor"] -->|只读配置校验| Core
~~~

| 模块 | 职责 | 约束 |
| --- | --- | --- |
| EmberTrial | 身体 / ASC / 属性、能力与战斗、AI、会话 / 出生、交互与兼容Practice | 不依赖UI、UMG或Editor |
| EmberTrialUI | 本地玩家观察、Presenter、Widget装配、菜单 / 暂停 / 失败流程 | 私有依赖Core；通过公开接口取数，不拥有玩法数值 |
| EmberTrialEditor | 玩法配置、BT键绑定、UMG必需控件等有限静态规则 | 只进入Editor目标，不向Runtime反向传递依赖 |

[Game Target](../EmberTrial/Source/EmberTrial.Target.cs)只含Core / UI，[Editor Target](../EmberTrial/Source/EmberTrialEditor.Target.cs)包含三模块，DataValidation插件限Editor。模块StartupModule是进程级加载，不代表World、Pawn或GameplayReady。

[三个Build.cs](../EmberTrial/Source)保留既有Public / Private依赖；类型在公开头出现才决定暴露需求，不能为“减少依赖数量”盲目移动。CLion每个CodeModel使用自己的Definitions / include路径；真实构建由UBT / UHT完成，生成CMake不作为另一套游戏构建系统。

## 2. 归属与可信状态来源

| 拥有者 | 持有内容 | 不负责的内容 |
| --- | --- | --- |
| AEmberTrialCharacter | 原生ASC、Health / Stamina / Poise / Combat属性集、Recovery / HealingResource、自己授予的能力Handle | 不持有Widget或跨世界存档 |
| CombatTrace / CombatLife | 当前检测窗口与命中去重 / 本身体生命观察、受击Handle和死亡封锁 | 不由HUD扣血，不共用敌人运行状态 |
| 各GameplayAbility / Task | 本次激活的Tag、Montage实例、计时 / 位移源、回调与代次 | 不撤销别人的Handle或复用另一激活的运行数据 |
| AIController / BT Task | 本敌人的目标、距离决策、移动 / 攻击请求及结束观察 | 不复制伤害规则，不持有UI |
| UILocalPlayerSubsystem | 当前本地Controller / Pawn观察 | 不把LocalPlayer寿命当成旧World Actor寿命 |
| UIBootstrap | 当前Presenter与HUD View的强引用和创建 / 移除 | 不控制暂停或写玩法属性 |
| Presenter / Widget | 弱数据源订阅与展示快照 / 只读渲染和按钮意图 | 不用Text Binding或Tick轮询玩法 |
| FailureFlow / MainMenuController | 当前互斥面板、输入 / 焦点、自己的暂停及切图意图 | 不把暂停状态复制到Pawn布尔变量 |
| RunSessionSubsystem | 本进程已确认地图 / 检查点ID、待定切图请求 / 成功确认事件 | 不持有旧Pawn / ASC，不保存交互Actor状态或库存域 |
| RelicInventorySubsystem | 主资产配置 / 定义GC引用、拥有ID / 两槽、版本方案 / 逻辑提交与切图语义 | 不持有World / ASC / Widget或GE句柄，私有同步写入器连接身体事务 |
| RelicEquipmentComponent | 当前身体 / 会话弱引用、精确GE句柄、同步版本 / 槽位与初次恢复标记 | 不拥有另一个库存或Widget、不按GE类清整张ASC；提交前拒绝，提交后观察世界变化 |

共享Character沿用模板原生类名，避免复制移动实现，取舍见[ADR-001](ADR/ADR_001_FrameworkReuse.md)。ASC放在Character，Owner=Avatar=Character；控制权变化刷新ActorInfo，身体换成新对象则资源重新初始化，取舍见[ADR-002](ADR/ADR_002_ASCOnCharacter.md)。

## 3. 初始化、Ready与清理

[Character](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)先校验生命 / 体力 / 韧性的六个出生配置与四组依赖，再初始化资源及Combat轻击加成基础0，最后发布Ready；非法配置不能只初始化一半。重复Ensure或同身体重新Possess不会补满资源或重置活动装备加成。

ActorInfo有效、GameplayReady、受控和存活是不同条件。Ready只表示当前四组属性基础成功初始化；死亡时仍可Ready且Health为0，UI不应把它显示成Waiting。各能力还有自己的配置 / Spec / 状态检查，不能仅看Ready；EquipmentReady已另行表达装备同步 / 当前控制与Pending状态，受控玩家输入据此门控。

观察者先解绑旧源，再绑定新源，并立即读取当前快照，兼顾“先Ready后订阅”和“先订阅后Ready”。异步装备 / 能力就绪尚未接入，不能把现有同步测试扩写成已覆盖全部未来时序。

取消、GE应用、Tag变化与委托可能同步重入。运行代码保留激活 / 观察代次、对象 / Montage实例身份与结束标记；先让旧身份失效，再解绑、清Timer / Task / RMS、停止自己拥有的资源。EndPlay先封口，防取消 / UnPossess迟到回调重新建立ActorInfo或订阅。

句柄按拥有者精确释放；解绑和结束操作需幂等。已从父级移除但尚未GC的Widget也必须立即停订阅，不能把GC当成业务解绑。身体结束撤销Ready、清理自己授予的能力与ActorInfo；LocalPlayer / GameInstance仍存活也不能保留旧身体。

## 4. 战斗能力与资源

[LightAttack](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp)共用玩家 / 敌人原生能力，蓝图分别配置玩家三段 / 25与敌人单段 / 10。Started输入进入单槽缓冲，窗口与激活 / 段代次决定是否续段；每段独立Trace Serial，取消后不能续播旧链。

[CombatTrace](../EmberTrial/Source/EmberTrial/Private/Combat/ETCombatTraceComponent.cpp)仅在命中窗口采样，骨骼更新后Sweep，校验准确Montage实例、去重与遮挡，向能力提供候选。能力再检查阵营、存活 / Ready、无敌及GE契约，以攻击者ASC生成独立Spec并结算。Notify只路由，不持有身体运行状态；Trace和Widget都不直接修改Health。

[Dodge](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Dodge.cpp)预检成功才提交一次体力成本并接管允许取消的攻击；无效请求保留原攻击。位移方向在开始时固定，RMS位移、Montage解锁与短无敌分别计时 / 清理。取消不退款，只撤销本次源 / GE Handle；移动组件处理碰撞 / 坡面，不用逐帧SetActorLocation绕开碰撞。

[Heal](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Heal.cpp)在开始时校验资源与互斥状态，在原生通知点重新核对当前Mesh / Montage实例 / 代次后至多提交一次。[HealingResource](../EmberTrial/Source/EmberTrial/Private/Resources/ETHealingResourceComponent.cpp)拥有离散次数，通过真实Instant GE成功应用才消费；满血、耗尽或提交前取消不扣数。它不管理动画 / 输入 / HUD，失活后再激活不补满，当前身体消失才结束。

[StaminaRecovery](../EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp)观察资源与正式动作Tag，缺额且允许恢复时才设世界Timer，按实际世界时间积分，满值即停；暂停不积累整段补量。Health / Stamina Base与Current夹限由各属性集维护，UI只读取结果。

[CombatLife](../EmberTrial/Source/EmberTrial/Private/Combat/ETCombatLifeComponent.cpp)先提交一次死亡锁与自己的一份Dead Tag，再关Trace、取消能力 / 位移并封锁输入；受击不能覆盖死亡。死亡Health保持0，治疗不能复活；当前死亡表现仍为冻结占位，新身体由Retry重建。

**治疗次数HUD已实现并验收：**[HealingPresenter](../EmberTrial/Source/EmberTrialUI/Private/Presenters/ETHealingPresenter.cpp)绑定LocalPlayer / Pawn Ready / HealingResource原生事件，弱来源 / 句柄 / 观察代次隔离旧回调；0次仍Ready，失活时也保留事件订阅。Widget仅显示整数，Bootstrap持有对象并区分重建View与重启观察；无Tick / Timer / Text Binding。晚加入、失活重启 / 恢复与世界重建见[C报告](Reports/M2_04C_Verification.md)。

**Poise基础已实现：**[PoiseAttributeSet](../EmberTrial/Source/EmberTrial/Private/Attributes/ETPoiseAttributeSet.cpp)维护Base / Current夹限与Max变化，Character一次初始化第三组、公开只读快照；0有效且不等同死亡。自动耗尽 / 回填已由下述B2接入，基础约束见[A报告](Reports/M2_05A_Verification.md)。

**B1状态已实现：**[Stagger](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Stagger.cpp)拥有Stunned、能力取消 / 阻断及一个世界Timer / 激活代次；原四项在C2-01增加BreakPulse后为五项。[CombatPoise](../EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp)拥有当前身体的Spec / Ready观察。公共Idle、攻击缓冲、普通受击与体力恢复已接统一状态，死亡优先、结束只释放自己的一份Tag和Timer。

**B2联动已实现：**[CombatPoise](../EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp)订阅Poise、Stunned撤销和自身Spec结束，以弱来源 / 观察代次与一个一次性Timer合并归零 / 回填事务。出生0 / 晚观察不遗漏，反应期间不重启；存活结束才经[原生Instant Poise GE](../EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETPoiseDeltaEffect.cpp)回填当前Max。死亡 / Deactivate / EndPlay不回填，退出先撤销身份和订阅再取消自己的Spec。Stagger复用旧HitResult Montage，动画完成不提前解除1.2秒状态；无常驻Tick / 失败轮询。

[C1](M2_05C1_BreakPulseAnimation.md)已完成独立施放素材，受害反应仍旧HitResult。C2-01的BreakPulse GA拥有Casting / Task / Montage实例与一次真实Duration GE；GetCooldownTags按活动GE查询剩余时间，不另建Tick计时。F只Started，施放阻断攻击 / 闪避 / 治疗与共享Idle，受击 / Stagger / Death可以取消，已提交冷却不退。C2-02已接无状态Notify，准确Montage实例 / Token与一次提交标志隔离旧事件；只在生效点Overlap，按身体去重、中心距离 / 阵营 / 状态 / Visibility过滤，逐目标创建原生PoiseDelta GE。范围300 / Poise60为GA配置，通知时间由资产读取；同步取消停止剩余目标，已落地效果不回滚。

## 5. 敌人与行为树

[AI黑板契约](AI_BlackboardContract.md)集中定义四个稳定键。感知选择有效玩家，Controller维护每敌人独立状态，Service调度决策，追击Task复用引擎MoveTo，近战Task只请求 / 观察自己拥有的Spec。

停步 / 恢复使用迟滞，MoveTo读取黑板真实绑定，不能用相等的默认数值代替绑定。近战准备时转向，接受攻击后锁定方向；受击 / 死亡 / 目标失效需要关闭资格、取消相应路径或Task，再重判。无路径或请求拒绝走有限频率等待，不逐帧无限重试。距离与等待值以Controller / BT资产和共享契约页为准。

AI提出意图、Ability判断接受、Trace确定候选、GE结算伤害；不在BT或敌人蓝图复制一套扣血 / 无敌逻辑。目前仅离线Standalone，ServerOnly策略不等于已实现网络复制。

## 6. UI数据流与输入

LocalPlayer观察Controller / Pawn → Presenter观察当前身体Ready / ASC属性 → DTO快照 → Widget事件更新。Health与Stamina各自独立Presenter / View，Bootstrap持有它们；数据源变化先停旧源，发布前提交缓存，数值不变不重复广播。

Widget先订阅再取即时快照，晚创建可显示已有值。重建只换View，不重置身体属性；重新观察不换Presenter身份。移除时先StopObserving，再RemoveFromParent；结束先移除View，再Shutdown Presenter。DTO不持有Actor / ASC，布局蓝图不直接写资源。

正式Widget使用Never + DisableNativeTick，不设Text Binding。共享[比例计算](../EmberTrial/Source/EmberTrialUI/Private/Presenters/ETResourceViewMath.h)与[文本格式](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETResourceHUDText.h)只处理纯展示规则，不把业务状态合并为UI缓存。

Core Controller发布P意图，既有[FailureFlow](../EmberTrial/Source/EmberTrialUI/Private/Flow/ETFailureFlowComponent.cpp)统一协调Pause / Failure。Widget发继续 / Retry / 菜单意图；Flow维护焦点、输入刷新与自己取得的暂停。Failure优先且两页互斥；普通继续恢复同一世界，切图 / EndPlay只清自身资源，不擅自恢复另一个世界的输入。

### 破势冷却HUD（D已实现）

Character只读接口核对自有Spec / ASC并查询真实GE；[BreakPulsePresenter](../EmberTrial/Source/EmberTrialUI/Private/Presenters/ETBreakPulsePresenter.cpp)绑定本地Pawn / Ready与三个Tag，NextTick合并GAS中间态和Ready早于授予。可见且冷却活动时每.10游戏秒查询，隐藏 / 死亡 / 失去身体 / 结束即停。Widget仅渲染快照与报告根可见性，无Tick / Binding；Bootstrap换View不重置GAS，同Presenter重启隔离旧句柄 / 代次。冷却就绪只描述冷却，不复制CanActivate规则。新UI生命周期专项和真实画面边界见[D报告](Reports/M2_05D_Verification.md)。

### 目标锁定（A已实现）

[TargetLock](../EmberTrial/Source/EmberTrial/Private/TargetLock/ETTargetLockComponent.cpp)由游戏Controller创建唯一原生组件，Tab Started按请求Overlap并去重，Enemy / Ready / 存活 / 中心距离与Visibility精筛，角度 / 距离归一分数量化后按对象路径决胜。已有目标仅限频维持、距离迟滞 / 遮挡宽限；不自动抢换，无目标无Timer。弱引用、死亡封口 / Ready / EndPlay / Possess和代次确保释放；新身体不继承，不更改镜头 / 动作方向或伤害。实物IA_ET_LockTarget，代码不硬引用内容路径；见[A报告](Reports/M2_06A_Verification.md)。

### 锁定标记（B1已实现）

[B1报告](Reports/M2_06B1_Verification.md)：六新UI与Controller独立表现组件读取Core同一目标，不修改Bootstrap / Core身份。Widget只渲染直接Canvas子级Border，根可见且目标合法时组件在PostUpdateWork投影；暂停不Tick、解锁即停，离屏只隐藏子控件以便转回重新出现。UMG投影 / 玩家界面几何负责DPI，不重复缩放；重建移除旧View / 引擎委托，不改变Core锁。MarkerRules为第21项纯规则专项，不替代真实Viewport。

### 锁定镜头（B2右肩已实现）

Controller原UpdateRotation调用唯一原生Camera组件，读取同一Core目标；合法锁定帧替换RotationInput再走Super / CameraManager，不另建Tick / Timer。弱SpringArm保存取得时SocketOffset，以原值+(0,75,55)cm为肩位目标，解析补偿右移 / 抬高后的视线、注视目标中心上方45cm；PitchOffset=-18°、肩位偏航补偿上限12°，允许贴身目标略离中心以保持后侧俯视；不是反复累加偏移，也不依赖上帧相机坐标。

主动解除立即归还鼠标，原更新流程平滑回正；Deactivate / PawnChanged / EndPlay精确归还并清弱引用，Pawn观察成对解绑。Pause冻结跟随及回正；不改Actor旋转 / 伤害 / GA。CameraRules包含原角度规则及真实测试World中的肩位所有权 / 换控清理；Selected Viewport画面另外验证近景可读性，不把NullRHI当作图像。当前值与完整代码见[B2](M2_06B2_LockCamera.md)。

### 目标切换（C已验收）

[C指南](M2_06C_TargetSwitching.md)只扩Core的目标身份：GatherCandidates复用A资格 / Visibility，左右按同一视点的水平角相对当前目标线性选优；.20游戏秒共享门限在查询 / 广播前写入，失败也限频且不排队，空侧保持原目标。内部ResetTargetBindings解除旧目标 / 废弃旧Serial，Set装配新目标后只发布一次Previous→Current，不让标记 / 肩位看到中间None。

Controller只绑定两个独立Boolean IA的Started；Z / C在原IMC配置。B1 / B2读取原单一目标，不改UI / Actor旋转 / GA；相机近景另由Camera层修正，原Selection扩真实切换 / 旧来源隔离 / Pause；无新测试文件 / 常驻扫描 / Tick / Timer。C实际代码已由用户写入；Selection无地板夹具改为World启动后冻结，避免PostInitializeComponents恢复运动后重力污染范围断言，新增位置断言。用户已报告章节检查完成；近景 / 出生补修双Build与四专项通过，真实采样边界见[C报告](Reports/M2_06C_Verification.md)。首次PIE创建等待需求关闭；M2-07A已验证新包集成，B已完成同包性能分析，未发现需立即改架构的持续瓶颈；C汇总实际设计与证据。

## 7. 会话、菜单与出生

[RunSession](../EmberTrial/Source/EmberTrial/Private/Flow/ETRunSessionSubsystem.cpp)由GameInstance拥有，区分已确认快照和待定请求。预检通过再解除暂停并OpenLevel；GameMode用请求ID选出生点、报告实际选择，PostLoad确认后提交，失败释放请求门控。

[CheckpointStart](../EmberTrial/Source/EmberTrial/Private/Flow/ETCheckpointStart.cpp)是带稳定ID的PlayerStart，仅允许当前本地、Ready且存活身体激活；出生重叠用一次NextTick补查，无常驻Tick。Entry必须唯一，缺失检查点ID回退Entry并修正下一次会话快照。

Continue / Retry重载地图、创建新身体与新HUD，保留最近检查点ID；NewRun确认后重置Entry。普通交互目标激活、敌人生命、玩家资源不跨世界保存。真正退出进程后Continue资格消失，版本化磁盘存档尚未实现。

正式AETGameMode负责出生 / 会话；生成前按实际Pawn胶囊向下Sweep一次，100cm内非穿透可走地面采用原生行走间隙，XY / 朝向不变；无地板 / 高空 / 非Character等保留原出生逻辑，避免首帧落差让FootIK追赶。没有逐帧吸地，也不改保存的PlayerStart / Mesh。菜单AETMenuGameMode不生成Pawn / Spectator / AHUD，只由蓝图装配菜单Controller。旧AEmberTrialGameMode仍被默认配置 / 类重定向引用，保留兼容模板出生；类 / 子对象 / 反射字段不能凭名称看旧就删除。

## 8. 蓝图、兼容入口与引用

C++负责规则、接口 / 状态归属、句柄与清理；Blueprint负责资源选择、Designer布局、Montage / Notify配置与少量既有交互桥接。角色暂停旧链已迁移，不能再给同一动作接两个状态来源。基础功能学习继续手动实践，本次清理依据用户授权直接修改。

Practice仍被正式预检、移动 / 跳跃、受击 / 死亡与两项M0自动化使用；练习Actor仍服务E提示。数字1是正式攻击备用入口，3请求正式受击，4为保留的开发损伤入口；ET_Stamina / ET_Healing修改限定Editor / PIE及合法本地身体；ET_Poise经指定身体的PIE命令查询 / 合法负增量GE，拒绝暂停 / 忙碌 / 无敌 / 死亡修改。开发命令不代表最终产品界面。

第28批AssetRegistry只读审计确认：正式地图引用Tests下训练目标，玩家引用GE_ET_Test_Damage25，均须保留；不能对整个Tests目录直接NeverCook / 删除。未被两张正式地图包闭包引用的Probe也不能仅据此删除，包级硬 / 软引用图不能证明蓝图白线、反射加载或实际运行可达性。

## 9. 可读性、性能与验证边界

清理将重复状态查询 / 名称与纯展示规则集中，注释说明所有权和时序；保持反射名、默认子对象名、资源路径、Tag、日志与关键判断顺序。可配置设计值保留配置，稳定标识符集中为契约，不把每个常数都变成新UPROPERTY。

运行开销通过窗口内Trace Tick、缺额才运行的恢复Timer、事件驱动HUD与BB值变化去重控制。[M2-07B](Reports/M2_07B_Verification.md)实测待机 / 交战内窗主循环间隔P95约5.915 / 5.714ms，完整交战仍有3次>16.7ms Tick、最大30.013ms，保留未唯一归因限制。PNG2048×1081与1920×1080请求不同，不作严格1080p认证；Slate整体不能分摊成单HUD专属成本，无持续瓶颈证据就不先重写架构。

[Editor校验器](../EmberTrial/Source/EmberTrialEditor/Private/Validation)只读CDO、BT结构与必需控件，不Spawn / 授予 / 播放。配置脚本只查共享Default.ini / 文件存在，不合并Saved / 平台配置，也不反序列化地图。

M2历史23项自动化（含新增SpawnPlacement生成前贴地 / 回退）覆盖Practice两项、资源两项、韧性基础 / 状态 / 耗尽各一项及破势施放 / 冷却与范围各一项、闪避三项、连段两项、治疗两项、UI纯规则两项及破势冷却生命周期一项及目标选择 / 生命周期一项、标记边界纯规则一项及镜头角度规则一项；真实Montage / Notify / Sweep / GE测试仍不替代键鼠、焦点 / 布局、导航交战和独立包加载。当次执行 / 沿用范围见各批验收报告；[M2-07A](Reports/M2_07A_Verification.md)已阶段全组23Success，历史清理见[Cleanup](Cleanup.md)。

清理32批及最后真实键鼠复验已关闭。当前M2_07A/Attempt01已实际Cook并验证四能力 / 资源HUD、Poise / F、锁定 / 标记 / 镜头与两轮世界重建；Cleanup包仅代表历史阶段，最新B性能报告对应同一A包；其短时Development / Medium观察不等于最终Shipping / 完整内容预算达标。后续构筑、持久存档与最终美术按[完整计划](../Introduction.md)分批。现有测试均受WITH_DEV_AUTOMATION_TESTS保护，普通Play不执行测试体；默认Shipping排除，Development保留编译 / 注册 / 体积成本，不因阶段早而删除有效回归。Content/Tests资源仍按实际引用保留。

### M3：两件遗物的正式装备闭环

**静态定义与纯方案。** UETRelicDefinition保存稳定ID、FText / 可空软图标、两槽资格与两种加法参数，PrimaryAssetId为ETRelic:RelicId；只在Editor条件依赖AssetRegistry测试。无状态规则计算资格 / 汇总、替换 / 卸下与预览，Plan记录来源弱身份、观察版本和前态，不持有ASC / Widget。增加已有类型遗物改数据，新增效果机制仍需消费者 / 校验。

**GI库存与身体效果。** Inventory独立于RunSession，持有定义和拥有 / 两槽ID / Revision；只监听已确认切图，NewRun重置、Retry / Continue / MainMenu保留并推进版本。玩家身体唯一RelicEquipment在Ready后持有一份自己创建的Infinite GE，以SourceObject / 精确句柄辨认所有权；更新同一GE避免临时上限下降，结束只清自己效果，敌人 / Neutral休眠，无Tick / Timer。四组基础GameplayReady与EquipmentReady分开，不把所有未来初始化塞进同一Ready。

**真实提交与资源语义。** TryMakePlan预检最终值，所有拒绝在副作用前；库存提交ID / 版本后调用预检过的同步原生写入器，更新同句柄并在完整结果后发布领域事件。同步属性回调可读新ID，重入写Busy；外部同步死亡 / 结束不伪装成回滚。活体装卸只保留 / 夹限生命，不免费治疗；轻击整链缓存(基础+加成)，不是每段重新取装备。新身体等Pending解除后的最终库存，重建后回满最终上限，治疗次数 / 冷却回默认。磁盘进度留M5，不声称GAS天然原子。

**Presenter与复用View。** 原生值快照供ListView，稳定ID维护选择；查询 / 比较不Apply GE，搜索清失效选择、排序保留合法ID。Presenter弱观察LocalPlayer / GI / 身体 / ASC，去重并精确解绑；条目分配 / 释放解绑旧Item。ListEntry Auto兼容UE原生Tick，Panel / Pause Never，业务不加Tick。搜索消费P、Esc离开文本焦点、Enter不误触外层继续；原Flow仍是暂停与输入模式唯一所有者。500行 / 异步图标压力尚未验证。

**祭坛权限与确认。** WorldSubsystem弱登记祭坛，按请求核对Player / Ready / Idle、距离 / 高差与Visibility，无每帧扫描或跨World强引用。原唯一E入口有祭坛就消费，无候选回旧A/B；单FailureFlow沿原Pause页展示。来源结束、换身体 / 死亡 / 关闭 / 切图清理沿原恢复链。

C1的FETRelicAltarAccess不可复制，持有弱World / Controller / 身体 / 来源 / 会话身份。C2已由单Flow在未暂停时创建、取得暂停并成功呈现后发放；弱View委托携带身份，最终接收端重验当前页 / IsInViewport / 暂停归属与Core实时资格。Presenter只提交槽 / 稳定ID / 观察版本，沿同一事务完成；无搜索结果仍能卸装，普通P只读。移除时先Close / Reset Access和命令，再清View；旧观察结果 / 执行中解绑有代际与强局部保活保护，不用短暂解暂停换取写入权限。公开UI委托使用Core类型，EmberTrial是UI公开依赖，Core无UI反向依赖。

M3当前36项含原M2的23项、遗物9项与输入偏好 / 草稿 / 视频规则 / 预览事务4项；C2只执行Presenter / RelicAltar / EquipmentTransactions三项，无屏幕夹具、用户真实键鼠、资产Valid和真实Retry日志分开记录。[C2报告](Reports/M3_04C2_Verification.md)已关闭，独立包仍M2_07A/Attempt01，不代表M3已打包。

**输入偏好已落地。** [A1](Reports/M3_05A1_Verification.md)由引擎持有的UETGameUserSettings保存合法个人偏好，Character原Look消费一次；默认1 / 不反转、运行拒绝非法值、重启持久化已验证。个人ini与GI配装 / 试炼资源分开，锁定相机继续拥有构图 / 跟随。SaveSettings只在明确确认调用，逻辑配置名与实际Saved物理路径分开记录；没有每帧读写ini。

**A2输入卡已落地。** [输入设置卡](Reports/M3_05A2_Verification.md)嵌入原Pause左列，Pause透传Flow已有busy，Flow源码未改。纯值Draft保存应用基线与候选，浏览 / 默认 / 取消不应用；确认重验外部当前值与实际Viewport宿主，成功才应用 / 保存。程序同步控件防重入，移除时封口 / 丢弃未应用草稿；原P、卡内Esc与键盘焦点通过。两项专项与实际UI分开记录，个人设置不要求Pawn / ASC。

**A3已验收。** [主菜单](Reports/M3_05A3_Verification.md)加入第五输入卡绑定与Pending透传，复用同一WBP与通用文案；Controller / Session仍拥有切图 / 初始焦点。卡片无需Pawn / ASC，未应用离开不提交，应用后游戏与Pause同值；无新增类 / 测试。

**B1已验收。** [视频模式规则](Reports/M3_05B1_Verification.md)以普通Core值对象分开请求 / 上次确认 / 桌面状态，候选依赖调用方平台目录，无边框正规化到桌面，失败清空。三源码 / 双Build / 专项与只读查询通过；原生配置2048×1152 / 无边框不冒充实际Viewport，未切屏 / 保存。

**B2-01已验收。** [事务内核](Reports/M3_05B2_01_Verification.md)只接纯值 / 绝对单调时间并返回命令意图，观察候选后确认，恢复也必须观察实际基线；期限、Serial、失败重试已由专项验证。单宿主顺序调用，无World / I/O，不将CommitCandidate许可当落盘成功。

**B2-02已验收。** [GI视频适配](Reports/M3_05B2_02_Verification.md)绑定弱Owner / World / Viewport / 原生Window；CoreTicker用平台时间，清理时取消。派生设置类保护UE自动Confirm / 保存造成的候选泄漏，显式确认才提交；真实取消 / 超时 / 暂停 / 切图恢复已验证。SlateCore私有、ApplicationCore公开，目录复用Engine，Core不依赖项目UI。

**B3-01已验收。** [显示草稿](Reports/M3_05B3_01_Verification.md)由UI持有纯值副本，从实际模式 / 平台目录生成选择；无边框固定桌面，外部实际 / 目录变化时拒绝过期候选，关闭重开丢弃旧状态。三源码 / 专项与短查通过。

**B3-02已验收。** [共享显示卡](M3_05B3_02_VideoSettingsUI.md)复用主菜单 / Pause；View拥有草稿 / 返回Serial，Begin同步广播暂时抑制后认领，Core拥有真实时间和窗口副作用。宿主CloseView先解绑再取消，Pending亦取消。首次捕获失败会临时用CoreTicker有限重试，成功 / 超时 / 关页结束，无Widget Tick或世界Timer。用户全新进程专项反馈已通过，本轮独立日志未另存；原Never / DisableNativeTick与PIE保护保持。

**B4-01已验收。** [非分辨率偏好](Reports/M3_05B4_01_Verification.md)由引擎同一GameUserSettings持有；独立设置十组画质保留ResolutionQuality，混合值-1保持当前组。PIE / 视频保护期间拒绝，ApplyNonResolutionSettings与显式Save不启动窗口预览。Graphics03证实最后合法0 / 30 / 0重启读取，无新服务或常驻轮询。

**B4-02已验收。** [共用画质卡](Reports/M3_05B4_02_Verification.md)由纯草稿维护编辑、应用前重验Core公开快照；默认只填本卡1 / 60 / 0。View订阅原视频服务事件以禁用 / 恢复，不在倒计时中覆盖草稿；宿主先解绑画质卡再取消视频，旧View不能确认。两个宿主第七绑定，右列滚动且焦点自动进入视野。无新设置服务 / Widget Tick / 世界Timer；真实应用 / 预览隔离与重启1-60-0已通过。

**C1已验收。** [音量偏好](Reports/M3_05C1_Verification.md)由同一GameUserSettings保存三个独立0–1值，非法拒绝、缺失用默认，显式确认才保存；该批两轮只读恢复0.6 / 0.3 / 0.8通过，实际声音随后由C2接入。

**C2已验收。** [声音路由](Reports/M3_05C2_Verification.md)由原生游戏GI服务管理专用三类树 / 常驻Mix，世界结束精确释放本Mix与试听，PostLoadMap后应用保存值。Master覆盖递归一次、叶子原始值不递归；配置软引用与Cook规则分开，用户真实听感及切图 / 重启0.6-0.3-0.8已通过。数据Setter / store已迁移到ApplyAudioPreferences / apply。

**C3已验收。** [共用音量卡](Reports/M3_05C3_Verification.md)用纯值草稿管理三滑条，显式应用 / 取消 / 默认与关闭解绑；Audio原生就绪事件解决首次菜单时序，视频事件只刷新资格不覆盖草稿。宿主第八绑定、原右列ScrollBox，Audio先于Video关闭；没有Tick / Timer或第二个Mix。首屏与重启0.5-0.5-1已通过。

**06A集成已验收。** [新版包](M3_06A_IntegratedPackage.md)将当前两件遗物与四类设置实际Cook；启动工具为M3强制明确目录、两轮共用专用个人ini，试炼状态仍在GI。旧能力测试的受伤准备移至出生装备恢复之后，42项阶段全组Success；Attempt01实际包内确认祭坛 / 设置与两次新身体恢复，Run02偏好保留 / 试炼装备重置。正式装备 / 能力逻辑未改。

**06B1已验收。** 第三件逆焰通过既有MaxHealth -20 / LightAttackBonus +10形成代价收益，沿用原装备句柄 / 事务 / UI投影；四既有测试解除真实目录固定两件的前提。真实组合40伤害、换装80/100不治疗与Owned3通过，无新增Gameplay分支。

**06B2-01已验收。** 原CombatAttributeSet已有HealingBonus0，Q读取聚合值并在接受施放时捕获基础40+加成，通知只用快照。真实GE移除后本次50 / 下次40与默认Q50→90通过，资源组件仍负责治疗 / 次数；无新增Tick / Timer或重复属性集。

**06B2-02已验收。** 治疗Kind、双槽汇总 / 方案重验和原装备GE第三SetByCaller完成；自有句柄校验 / 更新 / 清理均含新贡献。装备预检复用Q纯数值计算并保留外部贡献，六相关专项与默认路径通过。

**06B2-03已验收。** 回生数据通过原生HealingBonus10进入原GE / 事务 / Q快照；UI第三行投影Plan / AppliedModifiers的装备贡献，普通P只读、原15绑定与事件保持。四专项、真实Q50 / 移除40、外部贡献隔离与新身体恢复成立，无新增Tick / Timer。

**06B3-01已验收。** StaminaRecoveryBonus初始0，原组件用基础速率+聚合Current按世界时间恢复；变化前未结算片段按合同舍弃，消费截止时间保持。真实30 / 移除20、非法停表 / 合法唤醒、原阻断与独立属性Handle配对清理通过。

**06B3-02已验收。** 装备管线写入第四幅度，并在退出组件写锁后先复核恢复计时资格，再发布装备事件。相同速率通知保留积分起点。六项专项覆盖四幅度、句柄身份、过期方案及恢复计时。

**06B3-03已验收。** 凝息护符提供每秒5点恢复。Presenter以方案和已应用装备总和生成第四项，View沿用PreviewText。暂停祭坛装卸与恢复速率25／20通过。

**06B4-01待实施。** Combat属性集提供破势加成，F从施法者ASC读取Current并在施放时捕获基础值与加成之和。生效通知将该快照交给原Poise Delta GE，仍由目标的架势系统管理归零和破势反应。

## 10. 本批 Commit / Push（固定放在最后）

B3-03功能已在本地提交`2a8d1dd`。本轮提交验收报告、资产来源、B4-01指南和进度文档。[12份文档及Push清单](M3_06B4_01_BreakPulsePoiseBonusFoundation.md#8-commit--push)

- 英文 `docs: verify stamina recovery relic and plan break pulse bonus`
- 中文 `docs: 完成体力恢复遗物验收并规划破势加成`
