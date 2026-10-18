# 代码与文档清理

2026-10-18开始，基于M2-04B2已验收实现，不进入下一功能开发章。

## 范围与边界

清理前基线为110份Source约14121行（11份测试约2353行）；156份Markdown约42888行。预计20–35小时有效工作，等待另记，按首批实际校准。

用户授权助手直接清理。先查引用再删，保留功能、测试断言和有用途的开发入口；硬编码按配置 / 标识符 / 稳定契约 / 夹具判断。每批至少改善可读性 / 功能性 / 性能一项，不虚构性能收益。小批次收尾，额度紧的一项余量接近15–20%时停止开启新改动；无法预测单文件精确成本。Git由用户操作，原未提交文档有快照。

## 首批：治疗代码可读性

四文件ETGA_Heal.h / .cpp、ETHealingResourceComponent.h / .cpp，统一空格、运算符和控制流换行；分区说明配置、启动、单次Notify事务和所有权清理，注释补充成员 / 同步回调 / 取消边界。

有效代码、判断顺序、名字、默认值、反射声明参数、Tag与日志保持。收益为可读性与维护入口一致性，功能 / 性能行为保持。两治疗指南删除四份重复源码全文并链接当前Source；Progress删除重复流水，保留状态 / 工时 / 协作与证据入口。

本地基线、差异与检查在被忽略的Docs/Reports/Raw/Cleanup/2026-10-18_Batch01。

## 检查与覆盖边界

四文件有效Token、预处理指令保持一致；检查generated头顺序、其他Source / 资产 / Config未改与文档链接。首批当次未运行新UE Build / 测试；第2批已补正常构建与全11项自动化，不把历史Success冒充当次新结果。纯排版 / 注释按方案B静态收尾，逻辑 / 反射批执行构建与相关测试，最终统一自动化 / 实机 / 包短查。

首批结果：四文件有效Token / 预处理指令一致，106份其他Source与63项资产 / Config字节保持；40处受影响文档链接检查通过，clang-format格式检查通过。两个指南共删除557行重复源码，Progress从267行精简至42行；旧内容与原未提交文档均有本地快照。

额度快照：开工5小时已用30% / 周已用5%，首批复核时38% / 6%。这些是时点数据，不能线性换算剩余文件成本；本批只有可读性清理，暂维持整体20–35小时估计。

## 第2批：共享AI黑板键与旧入口审计

将Controller、追击Task、近战Task、Editor校验器中的8份局部FName定义集中为4个共享键，字符串 / 逻辑 / 配置不变；详见[AI黑板契约](AI_BlackboardContract.md)。新增1头 / 修改4cpp，实际111份Source / 59cpp，未增加新玩法。

Practice与诊断入口仍有正式互斥 / 清理、测试或复用检查依赖，本批保留，不凭名字删除。资产路径校验与同名反射属性不是黑板键，保留各自契约。

本批UE5.5.4正常Editor / Game Development Build均通过，三模块CLion元数据与CMake刷新成功，新头已进入Core CodeModel。实际DataValidation中正式行为树与隔离Probe均有本项目Valid标记，汇总0 Error / 0 Warning。11项现有EmberTrial自动化全部Success，0 Fail / 0 NotRun，覆盖首批治疗与旧Practice、连段、闪避、体力。

静态等价核对通过，106份其他Source、70项受保护文件（63项来源清单资产及Config / uproject）哈希保持；冻结B1指南未改。没有重新进行交互式AI实机或Cook / 打包；本批收益是减少重复名称定义与维护位置，不宣称帧率收益。日志 / 完整测试结果留在Raw/Cleanup/2026-10-18_Batch02。

额度快照：本批开始5小时已用41% / 周已用6%，收尾检查49% / 8%；保持小批次收口，未使用额度重置。助手构建 / 测试时间不计入用户手动实耗。

## 第3批：正式闪避生命周期可读性

范围为ETGA_Dodge.h / .cpp与ETAbilityTask_DodgeMove.h / .cpp。统一控制流、运算符与长条件换行，补充职责分区，解释同步回调序号保护、位移 / 动画 / 无敌独立结束、只释放自己RMS与GE Handle、幂等回调与不退款边界。行为、判断顺序、默认值、反射声明与日志文本保持；不为排版引入共用基类或新配置。

A / B / C三份闪避指南移除11份重复的整文件实现 / 测试代码，改为Source链接；保留局部接入差异、配置、测试断言意图与已解决的计时问题。路线页增加当前所有权 / 时序说明并删除过期“下一步待开发”与待提交描述。Architecture / Backlog入口不再复制当前批次编号。

本批UE5.5.4正常Editor / Game Development Build均通过，CLion三模块元数据刷新成功。全部11项现有EmberTrial自动化Success，0 Fail / 0 NotRun，包含成本 / 位移 / 短无敌、恢复闪避取消、治疗互斥、体力与旧Practice路径；并非新建简化测试替代原断言。

四文件有效Token / 预处理指令一致，clang-format格式检查通过；107份其他Source、290项Content / Config / uproject / 来源清单保护文件哈希保持，冻结B1指南未改。文档链接与代码围栏核对通过；11份重复代码共1825行已移除。收益为可读性、职责与时序可解释性、减少文档漂移；功能 / 性能行为保持，不宣称帧率收益。

Source仍111份 / 59cpp。本批未交互式实机、未重新DataValidation或Cook / 打包；资产Valid沿用第2批历史结果，不写成本批新执行。完整差异 / 日志 / 测试JSON留在Raw/Cleanup/2026-10-18_Batch03。

额度快照：开工5小时已用51% / 周已用8%，收尾检查57% / 9%；不使用重置，不把助手时间计入用户手动实耗。

## 第4批：攻击身份查询去重与受击链路整理

LightAttack两个玩家请求共用18项身份条件，提取为私有FindPlayerStageMontageInstance；原绑定取值与条件等价，各自保留“下一段”或“三段恢复取消”的区别，不缓存实时状态。攻击其他函数在格式化相邻字符串拼接归一化后有效Token保持，日志文字不变；HitReact只改格式 / 注释。两个能力共四文件分区说明输入缓冲、Notify身份、段推进、旧Task与整链收尾，数值、反射字段、日志与敌人单段语义保持。

现有Combo.Flow增加实例ID不匹配、玩家输入禁用、暂停时两个请求都拒绝且不留补执行的窄检查，原测试场景 / 断言全部保留。四份指南移除整文件实现 / 测试副本及旧恢复查询，改为Source入口，保留配置、接入差异与排障；连段路线页补共享与差异规则，清除过期待实施描述。

本批UE5.5.4正常Editor / Game Development Build均通过，CLion三模块元数据刷新成功。现有11项EmberTrial自动化全部Success，0 Fail / 0 NotRun；Combo.Flow日志确认新增三组边界断言已执行，原连段 / 真实伤害 / 旧回调 / 取消 / 暂停 / 死亡 / EndPlay场景保留，未以新简化测试替代。

共有18项条件及局部绑定逐项等价，调用方特有门控保留；其他攻击代码仅排版 / 注释 / 等值相邻字符串换行，日志文字保持。HitReact有效Token保持，原Combo.Flow的全部Token在新增检查之外保持；五份Source的clang-format检查通过。四份指南共移除1660行重复代码（8份整文件副本及1份旧查询），改为当前Source入口，保留有用解释和操作材料。

106份其他Source、290项Content / Config / uproject / 来源清单保护文件哈希保持，冻结B1指南未改；文档链接 / 围栏与Git差异检查通过。Source仍111份 / 59cpp。收益是减少重复身份判断与维护位置、提升可读性和边界可验证性；数值、伤害和性能行为保持，不宣称帧率提升。

本批未交互式实机、重新DataValidation或Cook / 打包。完整快照、差异、日志与测试JSON位于Raw/Cleanup/2026-10-18_Batch04。额度快照：开工5小时已用59% / 周已用9%，收尾检查67% / 11%；未使用重置，不把助手时间计入用户手动实耗。

## 第5批：生命观察、死亡封口与属性规则

整理CombatLife、HealthAttributeSet、StaminaAttributeSet共六文件的格式 / 注释，说明Ready撤销与死亡锁的区别、Tag计数所有权、真实伤害分类、同步回调后的弱引用重取、Base / Current夹限与上限调整。保留生命与体力各自语义，不引入通用基类；同步回调保护不删。

Life对旧Practice头的跨目录相对include改为模块内规范包含名；既有Build.cs已提供ModuleDirectory私有包含路径，无需修改依赖或移动类。除此之外，有效Token在等值相邻字符串拼接归一化后保持，数值、字段、API和日志文字不变。

四份指南合并13处属性 / 生命组件整文件、完整方法及旧Foundation测试副本，共移除912行代码，改为Source入口；保留历史接入、资产配置、排障与验收经验。死亡指南补当前所有权规则，HealthReady删除过期的尚未编码描述。

本批UE5.5.4正常Editor / Game Development Build均通过，CLion三模块元数据刷新成功。全部11项现有EmberTrial自动化Success，0 Fail / 0 NotRun；原测试未改，继续覆盖属性初始化 / 夹限、真实伤害、受击 / 死亡取消、闪避 / 治疗 / 连段及结束清理。没有为纯排版新建镜像测试或要求用户重建蓝图验收入口。

六文件在规范include与相邻字符串拼接归一化后有效Token / 预处理指令保持，clang-format检查通过。105份其他Source、290项Content / Config / uproject / 来源清单保护文件哈希一致，冻结B1指南未改；文档链接 / 围栏与Git差异检查通过。收益是可读性、所有权 / 时序解释与降低包含路径耦合，不宣称帧率提升。

Source仍111份 / 59cpp。本批未重新交互式实机、DataValidation或Cook / 打包，不能把旧失败页实机结果写成本批新验收。完整快照 / 差异 / 日志 / 测试JSON位于Raw/Cleanup/2026-10-18_Batch05。额度快照：开工5小时已用69% / 周已用11%，收尾检查75% / 12%；未使用重置，不计入用户手动实耗。

## 第6批：动画窗口通知小批次

开工5小时已用78% / 剩余22%，周已用12%，因此只选四份小型Notify Source。Attack Window的Begin / End重复路由提取为私有Route，两个入口仍分别传true / false；旧路由主体逐Token一致。Combo Window只整理格式 / 注释。两种通知不缓存身体状态，Trace / Ability仍分别拥有命中 / 连段窗口，不引入共用基类或新状态。

两份指南移除四份重复通知代码共100行，保留资产配置、时间数值与历史排障。路由主体与开关逐Token比对等价，Combo通知有效Token保持，预处理 / 反射声明与所有权规则未变，四文件clang-format检查通过。

本批UE5.5.4正常Editor / Game Development Build、CLion三模块元数据刷新成功；全部11项现有EmberTrial自动化Success，0 Fail / 0 NotRun，包含真实Montage通知、命中、连段及旧事件隔离。原测试不变，没有用镜像测试替代运行链路。

107份其他Source、290项Content / Config / uproject / 来源清单保护文件哈希一致，冻结B1指南未改；31处本地文档链接（含文件行号目标）与代码围栏、Git差异检查通过。Source仍111份 / 59cpp。收益是减少重复路由与提高可读性，不宣称帧率提升。未重新实机、DataValidation或Cook / 打包。完整证据位于Raw/Cleanup/2026-10-18_Batch06。

收尾额度快照：5小时已用82% / 剩余18%，周已用13%。已到约定15–20%区间，本批完成后停止开启新改动；额度回升后先复查，再继续后续队列。未使用重置，不计入用户手动实耗。

## 第7批：资源HUD数值格式去重与本地化测试

额度已回升，开工5小时已用1% / 周已用13%，第6批已提交。生命 / 体力HUD共四文件整理格式与订阅分区；原本重复的FNumberFormattingOptions / FText::Format收敛为UI私有无状态头文件，保留各自LOCTEXT命名空间 / Key、标题、状态、BindWidget和日志级别，不引入通用Widget基类或改变Presenter。

新增UI模块原生格式化测试，检查三种locale、整数显示 / 分组、调用方Pattern、保留FText的本地化历史，以及测试后语言 / locale / asset-group完整恢复。不调用角色、不修改属性或配置，不把文本测试当成HUD实机回归。

两份HUD指南移除四份重复整文件，共376行代码，改为Source入口；保留布局 / 生命周期 / 排障经验，修正过时的当前文件计数并补显示职责边界。

本批UE5.5.4正常Editor / Game Development Build均通过；三模块CLion元数据与CMake刷新成功，新私有头与测试已进入UI CodeModel。全部12项EmberTrial自动化Success，0 Fail / 0 NotRun / 0测试警告，原11项测试未改；新增格式化用例确认三种locale、取整 / 分组、FText历史与完整文化状态恢复。

实际DataValidation按WidgetBlueprint类型执行，正式HealthHUD与StaminaHUD均有本项目Valid标记，命令汇总0 Error / 0 Warning。107份其他Source、290项Content / Config / uproject / 来源清单保护文件哈希保持；冻结B1指南未改。六份Source格式检查、文档链接 / 围栏和Git差异检查通过。Source为113份 / 60cpp。

收益是减少数值格式维护位置、提高阅读性并补本地化验证，不改变属性规则、不新增Tick，不宣称帧率提升。没有重新进行交互式HUD布局 / 输入实机或Cook / 打包。自动化启动仍出现既有EOS好友权限403环境警告，测试自身无警告，不属于本次改动。完整快照 / 差异 / 构建 / 校验 / 测试JSON位于Raw/Cleanup/2026-10-18_Batch07。

收尾额度快照：5小时已用10% / 剩余90%，周已用15% / 剩余85%。未使用重置，不计入用户手动实耗。

## 第8批：UI装配入口去重与状态面板整理

开工HEAD为386141e，第7批已提交、工作区干净；5小时已用11% / 周已用15%。本批整理UIBootstrap与LifecycleStatusWidget四份Source，不改Presenter / 属性 / 玩法 / 反射字段或蓝图资产。

将五处相同的Controller / LocalPlayer身份检查集中到私有ResolveLocalUIController，每次读取当前Owner，不缓存身份；调用方的HasBegunPlay / 关闭标记、旧View移除、失败清理顺序保持。HUD的3 / 4 / 5层级改为内部命名常量，实际值与蓝图Class配置不变，不混入暂停 / 失败的职责。补充所有权、重建与观察的区别、主动解绑和结束清理分区。

静态展开对比已确认提取后的原五处判断等价，其余Bootstrap逻辑与状态Widget有效Token保持。三份历史指南移除13处重复实现 / 完整接入片段，共655行代码，改为Source入口；保留布局、复盘和验证经验，修正“尚未编译 / 需要补标记”等过期表述。文件数量保持113份Source / 60cpp。

本批UE5.5.4正常Editor / Game Development Build均通过，三模块CLion元数据与CMake刷新成功。现有12项EmberTrial自动化全部Success，0 Fail / 0 NotRun / 0测试警告；原测试与断言未改，没有为代码提取新建镜像测试。

实际DataValidation按WidgetBlueprint类型执行，状态面板、生命HUD、体力HUD均有本项目Valid标记；本次共7个Widget项目Valid，命令汇总0 Error / 0 Warning。此检查验证资产结构，不等于重新操作焦点 / 暂停 / 重建实机矩阵。

109份其他Source、290项Content / Config / uproject / 来源清单保护文件哈希保持，冻结B1指南未改。四份Source格式检查、三种层级值 / 五处条件静态展开、文档链接 / 围栏与Git差异检查通过。收尾按现有.gitattributes统一本批文本LF与文件末尾换行，只改变行结束，不改变有效Token。

收益是减少本地入口条件维护位置、提高装配 / 生命周期可读性；数值和性能行为保持，不宣称帧率提升。本批没有重新交互式实机或Cook / 打包。自动化启动仍有既有EOS好友权限403环境警告，测试自身无警告。完整差异、快照、构建 / 校验 / 测试JSON位于Raw/Cleanup/2026-10-18_Batch08。

收尾检查额度：5小时已用17% / 剩余83%，周已用16% / 剩余84%。未使用重置，不计入用户手动实耗。

## 第9批：体力恢复订阅去重与时序可读性

开工HEAD为97b5b98，第8批已提交、工作区干净；5小时已用19% / 周已用16%。范围为Recovery两文件、通用StaminaDelta Effect两文件和原恢复测试，共五份Source；不改默认值、反射字段、角色或资产。

五项动作Tag从两份重复数组集中为cpp私有函数的只读列表，首次组件使用时构造，此后返回ArrayView；注册与解绑共用原顺序，仍由原组件独占自己的Handle。其余恢复逻辑在展开比对后有效Token保持，三个其他运行文件只改格式 / 注释。保留World Timer、真实GE、延迟 / 采样误差与同步消费修订保护，不引入Tick。

现有StaminaRecovery用例增加受击 / 治疗阻断和五项ASC订阅 / 解绑检查，原断言与流程保留；不是用新简化用例替换。B1指南移除五份重复完整源码，共569行，改为Source入口并修正过期“尚未实现 / 原型2”说明。

本批UE5.5.4正常Editor / Game Development Build均通过，三模块CLion元数据与CMake刷新成功。全部12项EmberTrial自动化Success，0 Fail / 0 NotRun / 0测试警告；日志确认新增四类动作阻断、五项订阅及配对解绑检查实际执行，原场景 / 断言保留。

配置脚本实际PASS，Errors=0 / Warnings=0。108份其他Source、290项Content / Config / uproject / 来源清单保护文件哈希保持，冻结B1指南未改；五份Source格式、LF / 末尾换行、文档链接 / 围栏和Git差异检查通过。Source保持113份 / 60cpp。

收益是减少Tag维护位置与重复临时列表、提高时序 / 所有权可读性并补解绑验证，不宣称帧率提升。本批未重跑交互式实机、DataValidation资产校验或Cook / 打包；第8批Widget Valid是历史结果，不作本批新执行。自动化启动仍有既有EOS好友权限403环境警告，测试自身无警告。

完整快照、差异、进度盘点、构建 / 配置 / 测试JSON位于Raw/Cleanup/2026-10-18_Batch09。收尾额度快照：5小时已用27% / 剩余73%，周已用17% / 剩余83%；未使用重置，不计入用户手动实耗。

## 整体进度快照（第9批后）

截至本批，累计9批涉及43 / 113份Source（文件覆盖率约38%）、25份Markdown；已记录移除6654行重复代码副本，不计其他说明精简。这是改动覆盖统计，**不是整体清理完成率**：AI / 校验器有些文件只做过黑板键归一，测试与新共享头也计入，不能视为整文件审计完成。

| 范围 | 目前状态 | 主要剩余 |
| --- | --- | --- |
| 正式能力 / 属性 / 资源 | 攻击、连段、闪避、受击、治疗、生命 / 体力、死亡与恢复主要链路已整理 | Character大类、CombatTrace、其他Effect / Tag / Notify及旧Practice引用审计 |
| UI | 两HUD、状态Widget、Bootstrap及数值格式已整理 | 两Presenter、LocalPlayer观察、菜单 / 暂停 / 失败流程 |
| AI | 黑板键统一；行为树与运行回归已有历史证据 | Controller / Service / Task完整职责、取消与失败重试审计 |
| 工程 / 学习材料 | 相关指南改为Source入口，12项自动化保留并扩展 | Editor校验器、工具脚本、测试夹具复用、剩余架构 / 操作文档 |
| 最终收尾 | 分批正常构建与对应验证 | 清理完成后的统一实机链路、Cook / 打包和收益汇总 |

目前处于逐模块清理阶段，尚未进入统一收尾。按每批约4–6份Source及同步文档的粒度，后续暂估约10–15批，再做统一实机 / 打包回归；这是工作量估计，不是承诺，审计确认无须改动的文件会保留，复杂依赖也可能增加批次。此前20–35小时是整体有效工作估计，不能从批次数反推剩余精确小时。

## 后续队列

1. 共享标识符、旧Practice / 诊断入口引用审计，确认后移除无用路径。
2. 动作 / 生命周期清理，保持取消、输入和迟到回调隔离。
3. AI与UI重复逻辑按真实职责整理。
4. 架构 / 工具 / 使用文档精简，保留有学习 / 面试价值的复盘。
5. 统一构建 / 自动化、链路与包短查，再汇总收益。

前九批已收口；其余队列未完成，不进入新玩法开发。

## 本批 Commit / Push（固定放在最后）

第8批已提交，本批基线HEAD为97b5b98。五份Source与B1指南 / Cleanup / Progress共8项，验证已收口；由你在CLion查看Diff、重新Stage最新修改后Commit / Push。

- [ETStaminaRecoveryComponent.h](../EmberTrial/Source/EmberTrial/Public/Resources/ETStaminaRecoveryComponent.h)
- [ETStaminaRecoveryComponent.cpp](../EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp)
- [ETStaminaDeltaEffect.h](../EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETStaminaDeltaEffect.h)
- [ETStaminaDeltaEffect.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETStaminaDeltaEffect.cpp)
- [ETStaminaRecoveryTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETStaminaRecoveryTests.cpp)
- [M2_01B1_StaminaRecovery.md](M2_01B1_StaminaRecovery.md)
- [Cleanup.md](Cleanup.md)
- [Progress.md](Progress.md)

建议提交说明：Refactor stamina recovery subscriptions and verify cleanup guards。

Content / Config / Build.cs / Target未改；Raw日志、快照、测试报告与生成文件留本地。助手未Stage / Commit / Push。
