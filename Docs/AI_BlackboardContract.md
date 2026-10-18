# AI黑板名称契约

更新2026-10-18。清理第2批将8份局部名称定义合并为4个共享键；资产内容和决策行为保持。

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

## 本批验证状态

2026-10-18在UE5.5.4完成本批检查：

- 四个消费者除包含共享头、移除局部定义、替换键符号外有效Token保持；8份旧定义的字符串与新4键逐项一致。
- 正常Editor与Game Development构建均通过；CLion三模块元数据刷新、CMake重新生成成功，新共享头已进入Core CodeModel。
- DataValidation实际加载正式BT_ETMeleeEnemy与隔离BT_ETValidation_Probe，两者均有本项目Valid标记；commandlet汇总0 Error / 0 Warning。
- 全部11项现有EmberTrial自动化均Success，0 Fail / 0 NotRun，覆盖旧Practice、连段、闪避、体力与治疗；首批治疗代码也在本次构建与测试中验证。
- 106份其他Source、70项受保护文件（63项来源清单资产及Config / uproject）哈希保持；冻结B1指南未改。

这些证据覆盖编译、现有测试与资产结构，本批未重新进行交互式追击实机或新版Cook / 打包。没有新增专门AI运行时测试，不把资产Valid等同于完整AI实机回归。

本地日志与机器可读结果位于被忽略的Docs/Reports/Raw/Cleanup/2026-10-18_Batch02。

## 本批 Commit / Push（固定放在最后）

新增共享键头，四份消费者.cpp及本契约 / Cleanup / Progress / Architecture / Backlog / Introduction当前入口，按实际差异提交。Content / Config无计划变化；Raw基线 / 日志与生成索引本地。助手不Stage / Commit / Push。
