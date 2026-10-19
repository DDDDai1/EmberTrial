# EmberTrial：架构与生命周期

更新2026-10-19。本文描述M2-04B2后的当前实现，不再逐阶段复制开发流水。规模与最新验证在[Progress](Progress.md)，逐批清理在[Cleanup](Cleanup.md)；完整历史证据仍在原指南与Reports。

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
| AEmberTrialCharacter | 原生ASC、Health / Stamina属性集、Recovery / HealingResource、自己授予的能力Handle | 不持有Widget或跨世界存档 |
| CombatTrace / CombatLife | 当前检测窗口与命中去重 / 本身体生命观察、受击Handle和死亡封锁 | 不由HUD扣血，不共用敌人运行状态 |
| 各GameplayAbility / Task | 本次激活的Tag、Montage实例、计时 / 位移源、回调与代次 | 不撤销别人的Handle或复用另一激活的运行数据 |
| AIController / BT Task | 本敌人的目标、距离决策、移动 / 攻击请求及结束观察 | 不复制伤害规则，不持有UI |
| UILocalPlayerSubsystem | 当前本地Controller / Pawn观察 | 不把LocalPlayer寿命当成旧World Actor寿命 |
| UIBootstrap | 当前Presenter与HUD View的强引用和创建 / 移除 | 不控制暂停或写玩法属性 |
| Presenter / Widget | 弱数据源订阅与展示快照 / 只读渲染和按钮意图 | 不用Text Binding或Tick轮询玩法 |
| FailureFlow / MainMenuController | 当前互斥面板、输入 / 焦点、自己的暂停及切图意图 | 不把暂停状态复制到Pawn布尔变量 |
| RunSessionSubsystem | 本进程已确认地图 / 检查点ID、待定切图请求 | 不持有旧Pawn / ASC，不保存交互Actor状态 |

共享Character沿用模板原生类名，避免复制移动实现，取舍见[ADR-001](ADR/ADR_001_FrameworkReuse.md)。ASC放在Character，Owner=Avatar=Character；控制权变化刷新ActorInfo，身体换成新对象则资源重新初始化，取舍见[ADR-002](ADR/ADR_002_ASCOnCharacter.md)。

## 3. 初始化、Ready与清理

[Character](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)先校验生命 / 体力的四个出生配置，再一次性写入属性并发布Ready；非法配置不能只初始化一半。重复Ensure或同身体重新Possess不会补满资源。

ActorInfo有效、GameplayReady、受控和存活是不同条件。Ready只表示当前两组属性链成功初始化；死亡时仍可Ready且Health为0，UI不应把它显示成Waiting。各能力还有自己的配置 / Spec / 状态检查，不能仅看Ready。

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

## 5. 敌人与行为树

[AI黑板契约](AI_BlackboardContract.md)集中定义四个稳定键。感知选择有效玩家，Controller维护每敌人独立状态，Service调度决策，追击Task复用引擎MoveTo，近战Task只请求 / 观察自己拥有的Spec。

停步 / 恢复使用迟滞，MoveTo读取黑板真实绑定，不能用相等的默认数值代替绑定。近战准备时转向，接受攻击后锁定方向；受击 / 死亡 / 目标失效需要关闭资格、取消相应路径或Task，再重判。无路径或请求拒绝走有限频率等待，不逐帧无限重试。距离与等待值以Controller / BT资产和共享契约页为准。

AI提出意图、Ability判断接受、Trace确定候选、GE结算伤害；不在BT或敌人蓝图复制一套扣血 / 无敌逻辑。目前仅离线Standalone，ServerOnly策略不等于已实现网络复制。

## 6. UI数据流与输入

LocalPlayer观察Controller / Pawn → Presenter观察当前身体Ready / ASC属性 → DTO快照 → Widget事件更新。Health与Stamina各自独立Presenter / View，Bootstrap持有它们；数据源变化先停旧源，发布前提交缓存，数值不变不重复广播。

Widget先订阅再取即时快照，晚创建可显示已有值。重建只换View，不重置身体属性；重新观察不换Presenter身份。移除时先StopObserving，再RemoveFromParent；结束先移除View，再Shutdown Presenter。DTO不持有Actor / ASC，布局蓝图不直接写资源。

正式Widget使用Never + DisableNativeTick，不设Text Binding。共享[比例计算](../EmberTrial/Source/EmberTrialUI/Private/Presenters/ETResourceViewMath.h)与[文本格式](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETResourceHUDText.h)只处理纯展示规则，不把业务状态合并为UI缓存。

Core Controller发布P意图，既有[FailureFlow](../EmberTrial/Source/EmberTrialUI/Private/Flow/ETFailureFlowComponent.cpp)统一协调Pause / Failure。Widget发继续 / Retry / 菜单意图；Flow维护焦点、输入刷新与自己取得的暂停。Failure优先且两页互斥；普通继续恢复同一世界，切图 / EndPlay只清自身资源，不擅自恢复另一个世界的输入。

## 7. 会话、菜单与出生

[RunSession](../EmberTrial/Source/EmberTrial/Private/Flow/ETRunSessionSubsystem.cpp)由GameInstance拥有，区分已确认快照和待定请求。预检通过再解除暂停并OpenLevel；GameMode用请求ID选出生点、报告实际选择，PostLoad确认后提交，失败释放请求门控。

[CheckpointStart](../EmberTrial/Source/EmberTrial/Private/Flow/ETCheckpointStart.cpp)是带稳定ID的PlayerStart，仅允许当前本地、Ready且存活身体激活；出生重叠用一次NextTick补查，无常驻Tick。Entry必须唯一，缺失检查点ID回退Entry并修正下一次会话快照。

Continue / Retry重载地图、创建新身体与新HUD，保留最近检查点ID；NewRun确认后重置Entry。普通交互目标激活、敌人生命、玩家资源不跨世界保存。真正退出进程后Continue资格消失，版本化磁盘存档尚未实现。

正式AETGameMode负责出生 / 会话；菜单AETMenuGameMode不生成Pawn / Spectator / AHUD，只由蓝图装配菜单Controller。旧AEmberTrialGameMode仍被默认配置 / 类重定向引用，保留兼容模板出生；类 / 子对象 / 反射字段不能凭名称看旧就删除。

## 8. 蓝图、兼容入口与引用

C++负责规则、接口 / 状态归属、句柄与清理；Blueprint负责资源选择、Designer布局、Montage / Notify配置与少量既有交互桥接。角色暂停旧链已迁移，不能再给同一动作接两个状态来源。基础功能学习继续手动实践，本次清理依据用户授权直接修改。

Practice仍被正式预检、移动 / 跳跃、受击 / 死亡与两项M0自动化使用；练习Actor仍服务E提示。数字1是正式攻击备用入口，3请求正式受击，4为保留的开发损伤入口；ET_Stamina / ET_Healing修改限定Editor / PIE及合法本地身体。开发命令不代表最终产品界面。

第28批AssetRegistry只读审计确认：正式地图引用Tests下训练目标，玩家引用GE_ET_Test_Damage25，均须保留；不能对整个Tests目录直接NeverCook / 删除。未被两张正式地图包闭包引用的Probe也不能仅据此删除，包级硬 / 软引用图不能证明蓝图白线、反射加载或实际运行可达性。

## 9. 可读性、性能与验证边界

清理将重复状态查询 / 名称与纯展示规则集中，注释说明所有权和时序；保持反射名、默认子对象名、资源路径、Tag、日志与关键判断顺序。可配置设计值保留配置，稳定标识符集中为契约，不把每个常数都变成新UPROPERTY。

运行开销通过窗口内Trace Tick、缺额才运行的恢复Timer、事件驱动HUD与BB值变化去重控制。没有新测量就不宣称FPS提升，历史Insights仅代表当时包 / 环境，不代表最新M2性能。

[Editor校验器](../EmberTrial/Source/EmberTrialEditor/Private/Validation)只读CDO、BT结构与必需控件，不Spawn / 授予 / 播放。配置脚本只查共享Default.ini / 文件存在，不合并Saved / 平台配置，也不反序列化地图。

现有13项自动化覆盖Practice两项、资源两项、闪避三项、连段两项、治疗两项和UI纯规则两项；真实Montage / Notify / Sweep / GE测试仍不替代键鼠、焦点 / 布局、导航交战和独立包加载。具体哪些当次执行、哪些沿用，统一见[Cleanup](Cleanup.md)。

清理最终仍需集中实机与当前M2新Cook / 包短查。旧M1 Attempt02不含M2；次数HUD、后续破势 / 锁定 / 构筑、持久存档及最终美术仍按[完整计划](../Introduction.md)逐批实现。当前不进入下一功能章。

## 23. 本批 Commit / Push（固定放在最后）

第27–29批一起提交，完整清单及建议Commit信息见[Cleanup末尾](Cleanup.md#本批-commit--push固定放在最后)。Raw / 日志 / 生成文件 / 构建产物留本地，助手不操作Git索引。
