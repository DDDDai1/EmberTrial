# M2-05C2-02：唯一通知与范围破势验收

2026-10-23，UE5.5.4。用户18:44反馈全项完成，无失败或明显占位问题；代码 / 配置70、检查10，共手动80分钟，构建等待10分钟。**C2-02验收通过，无阻断项。**本轮只读核对源码、资产、构建和日志并保存文档；未代改实际Source / UE资产，未Build / 自动化 / PIE / Cook，未改Git索引。下一开发批为路线D的冷却反馈与模块收口，本次同时回答用户关于保留自动化测试的疑问，不执行测试删除。

## 1. 实现与构建

三新 / 五旧共8份Source完整落地，格式化并忽略末尾换行后全部与下发候选一致；其余131旧Source保持C2-01。实际**139 Source / 75cpp，Core55 / UI17 / Editor3**，18项自动化（Core16、UI2）。最后Source18:29:58；Core / Editor / UI DLL18:31:44 / 50 / 57，Editor receipt18:31:57，Game exe / receipt18:34:32 / 33覆盖最终源码。用户确认索引刷新与双目标成功，不重复构建。

GA在当前Mesh / Montage实例 / Notify Token与有效时间下封锁一次提交；一次Overlap按身体去重、目标中心300cm距离与Visibility遮挡判定，逐个以独立Instant PoiseDelta GE扣60。来源取消时停止余下目标，已生效GE不回滚；原6秒冷却提交不重复、取消不退款。Notify只路由，不缓存角色 / 目标，不新增常驻Tick或逐帧扫描。

## 2. 资产与专项

本批唯一字节变化的UE资产为AM_ET_BreakPulse_01，实际目录BreakPulse/Montage单数，18:37:23保存；SHA256为`029C617E470B5231005921C4A2A862B2A502E7FF93E3BCD3B76D1119D416A15C`。新原生Notify已由用户设在12帧 / .4秒，AS无通知；GA300 / 60 / 6 / 3配置、原策略保持，18:38:00.597本项目Valid / Issues0。

GA自身字节保持C2-01，新默认值来自原生继承；Range专项实际读取保存的GA / AM并断言半径300、扣除60、正式通知点.4秒，不能仅凭无资产差异误判漏配。原LS / AS、Skeleton / Rig、正式身体 / 敌人 / 地图、Config / uproject保持；来源仍**70项**，本地289份UE资产。

| 本地时刻 | 本轮结果 |
| --- | --- |
| 18:38:24.497 | EmberTrial.M2.Combat.BreakPulseRange：Success |
| 18:38:44.835 | EmberTrial.M2.Combat.BreakPulseAbility：Success |
| 18:41:27.373 | EmberTrial.M2.Combat.PoiseReaction：Success |

Range的14场景覆盖真实通知 / 查询 / 两目标多形状去重、边缘中心距离、遮挡、阵营 / 死亡 / Waiting / 无敌 / Stunned、生效时进出范围、前取消、Pause、GE同步取消与EndPlay；重复 / 旧实例检查随相关场景执行。旧Ability保留冷却与施放事务，旧Reaction保留归零 / 恢复 / 生命周期。其余15项未重跑，不声称本轮全部18项Success。

## 3. 实机证据与范围

18:42:05.601首次真实范围命中训练身体，Poise100→40；18:42:34.103第二次40→0，.104一次Depleted / Stagger request Accepted1；18:42:35.321 Restore Applied1 / Max100。每个Serial仅一条该目标的真实应用；18:42:48.838有Candidates0的空放。

18:42:59.155施放，.388取消、PulseAttempted0，支持通知前取消无结算；18:43:07.337施放后打开Pause，18:43:10.198恢复后的唯一Pulse，再自然结束，支持活动施放暂停。具体输入、资源HUD隔离、F中Stop再开与最后短基线按用户全项反馈接受。敌人真实交战行仅填写“完成”、未描述具体反应，当前日志未找到敌人被F命中的样本；该项在指南为可选观察，不当作必查缺口，也不虚构已观察敌人破势，D交战收口再补可见样本。

现有日志未见Fatal、Blueprint Error、Accessed None或自动化Error；不将环境Warning算成玩法缺陷，也不声称全日志零警告。没有新Cook / Insights；Cleanup/Attempt01不包含后来的次数HUD / Poise / F范围等编辑器功能。

## 4. 测试文件与性能结论

用户询问的Core/Tests目前16份，另有UI/Private/Tests两份，共18项。每份测试主体与include均包在`#if WITH_DEV_AUTOMATION_TESTS`内；项目Game / Editor Target没有强制将开发自动化编入Shipping，也没有源码 / Config启动时执行RunTests的逻辑。

按本机UE5.5.4的UEBuildTarget.cs，默认Development启用开发测试，Shipping / Test关闭，除非显式强制打开。AutomationTest.h的宏创建并注册测试实例；注册不等于执行RunTest，普通Play不会自行生成这些测试世界或运行断言。

因此**无需删除**：保留它们不产生测试体的持续每帧运行；Development有额外编译 / 链接、代码 / 字符串体积、静态注册与少量常驻内存成本，不能说绝对零开销。默认Shipping排除这些测试主体，源码继续保留在仓库。未来只在功能删除、用例失效或确有重复覆盖时更新 / 合并；不因阶段编号早或文件数增加就删除仍有效的回归保护。本次未测量启动耗时 / 字节或FPS，不捏造量化收益，不改测试编译宏来隐藏验证。

本结论仅针对Source下自动化代码；Content/Tests中被正式玩法引用的训练身体 / GE等资源另按引用与Cook规则管理，不能整目录删除或NeverCook。

## 5. 工时与本次 Commit / Push（固定放在最后）

本批手动80 / 等待10；M2-05累计**435 / 35**，M2累计**1000 / 99**。项目累计**3011分钟（50小时11分）/ 明确等待284分钟（4小时44分）**。只计用户报告，未拆报学习 / 排错与助手设计不追加。D剩余60–90分钟，模块预计总计495–525分钟，构建等待另列。

初查时本地HEAD为0cce598，三新文件暂存0字节、工作区完整；核对期间用户已完成**36bb86906933b02aaa25c5cf9ecaf24d83bba849**，139Source与HEAD归一换行一致，AM LFS OID匹配，当前暂存区为空。该暂存问题已关闭，不需回滚或重复Stage源码 / 资产。助手未操作索引。

当前只Stage本报告、来源70项与状态 / 路线 / 最近指南的最新文档收尾，建议：

```text
docs: verify break pulse range and retain regression tests
```

Raw / 日志 / 生成物 / 包体留本地，用户自行Stage / Commit / Push；没有联网核验GitHub，不能以本地HEAD或缓存origin代替远端确认。
