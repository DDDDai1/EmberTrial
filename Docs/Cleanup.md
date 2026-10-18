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

## 后续队列

1. 共享标识符、旧Practice / 诊断入口引用审计，确认后移除无用路径。
2. 动作 / 生命周期清理，保持取消、输入和迟到回调隔离。
3. AI与UI重复逻辑按真实职责整理。
4. 架构 / 工具 / 使用文档精简，保留有学习 / 面试价值的复盘。
5. 统一构建 / 自动化、链路与包短查，再汇总收益。

前四批已收口，其余队列未完成。后续按独立职责继续清理，不重建临时Controller验收图，不进入新玩法开发。

## 本批 Commit / Push（固定放在最后）

第3批已提交，开工HEAD为f4c3353。本批共12项，按CLion实际Diff由用户Stage / Commit / Push：

- EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h
- EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp
- EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_HitReact.h
- EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_HitReact.cpp
- EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp
- Docs/M1_04A_LightAttackAbility.md
- Docs/M1_04C1_HitReaction.md
- Docs/M2_03B_LightCombo.md
- Docs/M2_03C_RecoveryDodgeCancel.md
- Docs/M2_03_LightComboRoadmap.md
- Docs/Cleanup.md
- Docs/Progress.md

可用提交说明：Deduplicate player attack context and clarify combat lifecycle。

Content / Config不因清理重存；Raw证据、日志 / 测试报告、生成索引和构建产物留本地。助手不Stage / Commit / Push。
