# AI黑板名称契约

更新2026-10-19。清理第2批将8份局部名称定义合并为4个共享键；资产内容和决策行为保持。

## 唯一声明与用途

[ETBlackboardKeys.h](../EmberTrial/Source/EmberTrial/Public/AI/ETBlackboardKeys.h)由Controller、追击Task、近战Task与Editor资产校验器共用。命名空间内inline变量避免重复定义；不使用会在Unity编译中合并的匿名命名空间同名常量。

| C++名称 | BB资产键与类型 | 用途 |
| --- | --- | --- |
| ETBlackboardKeys::TargetActor | TargetActor / Object-Actor | 感知唯一存活玩家与任务目标 |
| ETBlackboardKeys::CanPursue | CanPursue / Bool | Controller决策、树门控 |
| ETBlackboardKeys::CanMeleeAttack | CanMeleeAttack / Bool | 近战资格与树门控 |
| ETBlackboardKeys::PursuitStopDistance | PursuitStopDistance / Float | Controller写入距离、MoveTo读取与Editor绑定校验 |

这是现有资产的稳定协议，不把键字符串变成可随意改动的每敌人配置。反射属性PursuitStopDistance与同名黑板键不是同一个对象；Validator中的ReadNumber仍按属性名查询，不做全局文本替换。

## 保留的行为

- Controller继续90停止 / 100恢复 / 100近战与0.5秒失败重试，实例状态仍由各Controller / Task拥有。
- MoveTo先设置150默认回退值，再SetKey绑定；保持原生Super清理与NodeMemory，不重引入M1到达半径问题。
- 不改BB / BT / Controller蓝图、距离判断、感知、受击 / 死亡 / 暂停和日志文本。
- 收益是名称维护集中和降低拼写漂移风险，不宣称帧率提升。

## Practice与诊断入口审计结论

Practice组件仍被正式GA预检、Character移动 / 跳跃、受击 / 死亡清理引用，并由Lifecycle / DodgeMovement两份旧自动化使用，故本批不删除。只查到C++引用不能证明蓝图节点已全部退役；后续删除前仍需检查实际蓝图调用。

ET_Stamina / ET_Healing继续供复用检查使用，修改逻辑已限定WITH_EDITOR及PIE / 本地上下文；不因其名称带开发阶段编号就删除。未完成蓝图引用审计的3 / 4入口同样保留，不为清理牺牲已有检查能力。

## 当前决策写入与回调边界（第16批）

Controller的两项布尔决策共用私有UpdateBlackboardDecision，只写已有同类型键、保留相同值去重。调用方先提交bPursuitAllowed / bMeleeAttackAllowed，再写黑板；不可把它与后续StopMovement交换，否则同步观察者会看到旧状态。共享键头与资产不改，实际距离 / 冷却也不改。

Service仍0.2秒调度，Super维护下一次间隔；Move不自建路径或Timer，保留原生NodeMemory、先150默认值再SetKey、执行前目标复核和结束前快照。Controller强制停树时保留bStoppingTree重入保护；清目标先中止旧Move再清黑板。原生子对象名与反射字段属于已有资产契约。

第16批验证记录保留在[Cleanup](Cleanup.md)对应批次；当前提交清单以Cleanup末尾为准。下面是第2批历史证据。

## 当前存活规则与近战请求（第17批）

Controller与近战Task共用Core私有[ETAILiveBody.h](../EmberTrial/Source/EmberTrial/Private/AI/ETAILiveBody.h)，统一有效对象、非销毁 / 死亡、Ready、有限且范围合法的正生命快照。它不缓存状态、不增加Tick，也不把生命下限等稳定规则变为新资源配置。Controller仍单独验证本地玩家与双向控制关系；Task另验Controller / Pawn、BB目标、阵营及ASC / Spec身份。

[近战Task](../EmberTrial/Source/EmberTrial/Private/AI/ETBTTask_MeleeAttack.cpp)保持首次Tick发请求、准备阶段转向、发招后固定Yaw、只结束 / 取消自身Spec。Request与黑板通知可同步重入，序号保护、同步结束暂存、先撤销身份再解绑 / 取消均保留；FinishLatentTask后不再写成员。

本批只合并相同谓词、统一格式并补职责注释；短路判断、反射与参数、日志、树和能力资源保持。验证与当前提交清单统一见[Cleanup](Cleanup.md)，不把下面历史结果冒充新执行。

## 第2批历史验证状态

2026-10-18在UE5.5.4完成本批检查：

- 四个消费者除包含共享头、移除局部定义、替换键符号外有效Token保持；8份旧定义的字符串与新4键逐项一致。
- 正常Editor与Game Development构建均通过；CLion三模块元数据刷新、CMake重新生成成功，新共享头已进入Core CodeModel。
- DataValidation实际加载正式BT_ETMeleeEnemy与隔离BT_ETValidation_Probe，两者均有本项目Valid标记；commandlet汇总0 Error / 0 Warning。
- 全部11项现有EmberTrial自动化均Success，0 Fail / 0 NotRun，覆盖旧Practice、连段、闪避、体力与治疗；首批治疗代码也在本次构建与测试中验证。
- 106份其他Source、70项受保护文件（63项来源清单资产及Config / uproject）哈希保持；冻结B1指南未改。

这些证据覆盖编译、现有测试与资产结构，本批未重新进行交互式追击实机或新版Cook / 打包。没有新增专门AI运行时测试，不把资产Valid等同于完整AI实机回归。

本地日志与机器可读结果位于被忽略的Docs/Reports/Raw/Cleanup/2026-10-18_Batch02。

## 本批 Commit / Push（固定放在最后）

第17批同步本契约，新增一个Core私有头并整理Controller实现 / 近战Task两文件；共享黑板键、Editor校验器与资产保持。当前四份Source与四份Markdown共八项，统一见[Cleanup末尾](Cleanup.md)，由用户重新Stage最新Diff再Commit / Push。Raw / 构建 / 测试结果及生成索引留本地，助手不操作Git索引。
