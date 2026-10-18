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

四文件有效Token、预处理指令保持一致；检查generated头顺序、其他Source / 资产 / Config未改与文档链接。未运行新UE Build / 测试，不把历史Success写成本批新结果。纯排版 / 注释按方案B静态收尾，逻辑 / 反射批执行构建与相关测试，最终统一自动化 / 实机 / 包短查。

首批结果：四文件有效Token / 预处理指令一致，106份其他Source与63项资产 / Config字节保持；40处受影响文档链接检查通过，clang-format格式检查通过。两个指南共删除557行重复源码，Progress从267行精简至42行；旧内容与原未提交文档均有本地快照。

额度快照：开工5小时已用30% / 周已用5%，首批复核时38% / 6%。这些是时点数据，不能线性换算剩余文件成本；本批只有可读性清理，暂维持整体20–35小时估计。

## 第2批：共享AI黑板键与旧入口审计

将Controller、追击Task、近战Task、Editor校验器中的8份局部FName定义集中为4个共享键，字符串 / 逻辑 / 配置不变；详见[AI黑板契约](AI_BlackboardContract.md)。新增1头 / 修改4cpp，实际111份Source / 59cpp，未增加新玩法。

Practice与诊断入口仍有正式互斥 / 清理、测试或复用检查依赖，本批保留，不凭名字删除。资产路径校验与同名反射属性不是黑板键，保留各自契约。

本批UE5.5.4正常Editor / Game Development Build均通过，三模块CLion元数据与CMake刷新成功，新头已进入Core CodeModel。实际DataValidation中正式行为树与隔离Probe均有本项目Valid标记，汇总0 Error / 0 Warning。11项现有EmberTrial自动化全部Success，0 Fail / 0 NotRun，覆盖首批治疗与旧Practice、连段、闪避、体力。

静态等价核对通过，106份其他Source、70项受保护文件（63项来源清单资产及Config / uproject）哈希保持；冻结B1指南未改。没有重新进行交互式AI实机或Cook / 打包；本批收益是减少重复名称定义与维护位置，不宣称帧率收益。日志 / 完整测试结果留在Raw/Cleanup/2026-10-18_Batch02。

额度快照：本批开始5小时已用41% / 周已用6%，收尾检查49% / 8%；保持小批次收口，未使用额度重置。助手构建 / 测试时间不计入用户手动实耗。

## 后续队列

1. 共享标识符、旧Practice / 诊断入口引用审计，确认后移除无用路径。
2. 动作 / 生命周期清理，保持取消、输入和迟到回调隔离。
3. AI与UI重复逻辑按真实职责整理。
4. 架构 / 工具 / 使用文档精简，保留有学习 / 面试价值的复盘。
5. 统一构建 / 自动化、链路与包短查，再汇总收益。

当前前两批已收口，后续队列未完成，不重建临时Controller验收图。下一批优先选择动作 / 生命周期中的一组可独立验证改动。

## 本批 Commit / Push（固定放在最后）

基于已提交的首批f0b4fd7，本批共11项文件：

- 新增EmberTrial/Source/EmberTrial/Public/AI/ETBlackboardKeys.h、Docs/AI_BlackboardContract.md。
- 修改Core模块Private/AI下ETAIController.cpp、ETBTTask_PursuitMove.cpp、ETBTTask_MeleeAttack.cpp，以及Editor模块Private/Validation/ETProjectAssetValidator.cpp。
- 同步Docs/Cleanup.md、Docs/Progress.md、Docs/Architecture.md、Docs/Backlog.md、Introduction.md。

在CLion查看Diff后由用户Stage / Commit / Push，可用提交说明：Consolidate AI blackboard key contract。Content / Config无变化，不Force Add Raw、构建 / 日志 / CMake生成文件。助手未操作Git索引或远程。
