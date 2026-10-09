# M1-03B3 数据源重绑验收与 Waiting 收尾

日期：2026-10-05。主体约21:40完成、手动70分钟；局部收尾约22:17完成、手动10分钟，无新增构建等待。**M1-03B3验收通过，Waiting补项关闭；B1 / B2 / B3组成的M1-03B已完成。M1-03总任务仍待C阶段汇总结论。**以下保留原差异与定位经过，最终收尾结果见第5节。

后续状态：用户授权继续后，C已复用本报告和A / B证据完成[阶段汇总](../M1_03C_DataFlowReview.md)，[M1-03总验收](M1_03_Verification.md)通过。以下不改写B3收尾时的工时与执行范围。

## 1. 用户实机结果

用户报告两个Character父类 / 单ASC / 四输入资源 / 无M0业务、合法40/80 / 非法100/60 / 裸Pawn配置正确；第6.1–6.6全部通过，包含独立身体与属性、旧源修改 / 销毁隔离、新实例恢复、Unsupported / Waiting / NoPawn、HUD重建、两种就绪顺序、往返与三次PIE。

用户确认已删除Controller临时按键 / 五函数 / 五变量，Compile并正常Save；原角色100/100、原1/2/3、2→3、E/P与基础输入正常；测试资产不在正式地图，也不是默认Pawn。

这些配置、画面、完整次数与临时节点清理按用户反馈登记。本轮没有通过UI或UE反射重新读取全部蓝图内部配置，不用文件大小 / 哈希冒充这些检查。

## 2. 本地直接证据

33份Source文件与[B2源码基线](Raw/M1_03B2_SourceHashes.json)逐份哈希一致，无新增源码。本批纯蓝图，无需重复Build / 索引；本轮未执行新构建、自动测试、PIE或编辑UE资产。[当前源码哈希](Raw/M1_03B3_SourceHashes.json)已保留。

[相关日志片段](Raw/M1_03B3_Observed_2026-10-05.log)保留546条。日志原始时间13:34–13:39为UTC，下列时间按Asia/Shanghai换算：

| 检查 | 直接结果 |
| --- | --- |
| LocalPlayer初始化 / 清理，即可区分的PIE轮次 | 5 / 5 |
| Health Widget创建 / 移除 | 10 / 10 |
| View订阅 / 解绑 | 10 / 10 |
| 旧View仍有效记录 | 5条 |
| 同轮View解绑后继续ViewDataChanged | 0条 |
| Blueprint Runtime Error / Accessed None / LogBlueprint Error / LogScript Error / Fatal / Assertion指定模式 | 0条 |
| 业务依赖Error | 1条，Waiting对象Missing gameplay dependencies，不能忽略 |

21:34原身体扣至75/100，新身体初始化40/80、扣至15/80。21:35–21:36实际修改原身体至50/100、测试身体至0/80；按对象身份读取，旧身体变化没有相应当前Presenter属性事件。21:36:32合法测试身体新实例名由C_0变C_1，并重新初始化40/80。

21:36:49与21:37:10两次裸Pawn实例出现UnsupportedPawn；重建快照保留Unsupported / NoPawn，之后新合法身体恢复40/80。21:38:23直接观察Health initialized / Ready在Spawn finished之前、Possess / Presenter快照之后补齐40/80，支持先就绪再观察的路径。清理顺序按各轮对象判定，没有解绑后的旧View变化。

当前日志没有逐项记录用户反馈的所有往返 / 重复次数、全部画面或第6.5精确操作顺序；这些结果以实机反馈为据，不要求为了扩充日志重跑全部用例。

## 3. Waiting用例的差异

21:37:45（原日志13:37:45）出现：

```text
LogETAbilityFoundation: Error: [M1-03A2-01] Missing gameplay dependencies Character=BP_ETHealthWaitingCharacter_C_0
```

随后Spawn finished GameplayReady=false，Presenter / HUD进入WaitingForGameplayReady，两个Effect请求Rejected not ready。这支持“未就绪拒绝与展示”，但未证明走过预期的非法100/60配置校验。

当前源码EnsureGameplayInitialized先检查ASC、ActorInfo归属、HealthAttributes及ASC属性集一致性，失败输出上述Error；后面才验证InitialHealth / InitialMaxHealth，失败应为Warning `Invalid startup health ... Health=100.0 MaxHealth=60.0`。此次样本没有该Warning。因此不能把依赖异常当作预期的非法参数结果。四输入资源不属于这里的依赖检查，不能据此断定是输入配置缺失。

2026-10-05收尾排查已定位：独立UE5.5.4 Python commandlet只读加载三个已保存蓝图，原玩家 / 合法Probe默认对象的HealthAttributes引用有效且Outer属于各自默认对象；旧Waiting的InitialHealth / MaxHealth确为100/60、ASC与四Input资源都有值，但HealthAttributes成员为None。其CDO下仍能枚举到名为HealthAttributes的子对象，所以这里是成员引用失效，不能靠组件面板看起来正常来排除。

本地结果与[Epic UE-273844](https://issues.unrealengine.com/issue/UE-273844)一致，官方明确说明复制含原生AttributeSet默认子对象的Actor蓝图会使复制资产的属性集引用失效，影响5.4 / 5.5、目标修复5.6。结合原指南第2.3要求Duplicate与本机5.5.4，确定收尾路线为从原生类新建干净样例；已修正原指南，不修改C++保护，也不升级引擎。

[检查脚本](../../Scripts/Editor/inspect_m1_b3_dependencies.py)、[默认对象报告](Raw/M1_03B3_DependencyInspection.json)与[命令行日志](Raw/M1_03B3_DependencyInspection.log)已保留。首次检查脚本读取Blueprint.parent_class不兼容，修正后独立进程返回0，最后摘要0 errors / 0 warnings。ASC内部SpawnedAttributes未向当前Python接口暴露，该字段报告inspection_error，不假称完整运行时属性注册验证；定位依据是可读取的成员指针、默认子对象身份、配置与现有运行Error。

检查脚本没有调用资产修改 / 编译 / 保存接口，没有运行Gameplay测试；运行后原玩家、合法Probe与旧Waiting三份资产哈希均与现存基线一致。用户随后按[局部收尾指南](../M1_03B3_Closure.md)完成新样例非法100/60与临时合法40/80复验，见第5节；其余已确认结果沿用，没有重建五函数或重跑6.1 / 6.2 / B2。

## 4. 资产与进度

| 资产 | 正常保存时间 | 大小 / SHA-256 |
| --- | --- | --- |
| BP_ETPlayerController | 2026-10-05 21:39:17，日志与磁盘一致 | 22278 bytes；`BA06EC2DAC6424E0FC3328DD23410524CA8B2578DD6129E30DA50DEB5431FD03` |
| BP_ETHealthProbeCharacter | 2026-10-04 21:46:59 | 54039 bytes；`04AB799A67FD5BA18523757B51FB98B3052068198C9766DD2C9DBE21EBECAC04` |
| BP_ETHealthWaitingCharacter | 2026-10-04 21:47:31 | 52730 bytes；`77542C2FE481C6659669E6B58270473D416E3CD0216D9CB9AD6B1BAA44A66689` |

主体阶段原玩家与生命HUD的哈希保持B2基线；两份测试资产已登记。旧Waiting现标为停用的失效样例，收尾新建Rebuilt替代，不再把旧资产作为合法测试入口。

主体完成时累计手动1188分钟；加本次收尾10分钟后，共 **1198分钟（19小时58分钟）**，已报告等待仍为73分钟。主体70分钟与收尾10分钟均未细分准备 / 执行时间，不作为纯验收耗时。

用户已选择[B方案：分层验收与复用入口](../AcceptanceWorkflow_Options.md)。本次读取证据、更新文档与清单，不推进C阶段、不执行新构建 / Gameplay测试或代改C++ / UE资产。

## 5. 22:17局部收尾验收

用户确认新Rebuilt从原生EmberTrialCharacter直接创建，非法100/60时Invalid startup health / HUD Waiting / 无依赖Error；临时40/80时Health initialized / Ready / HUD40/80与50%。新样例已恢复100/60；Level Blueprint临时节点和场景实例已删除，Compile / Save完成；正式原身体100/100、单HUD、移动转向 / P短基线正常。手动约10分钟。

[收尾日志](Raw/M1_03B3_Closure_2026-10-05.log)直接支持：

- 22:15:18新样例ActorInfo归属正确，精确非法100/60 Warning；Presenter / HUD为WaitingForGameplayReady。
- 22:16:13同一蓝图在下一轮的新实例正常初始化40/80、Ready=1，Presenter / HUD快照40/80、Fraction=0.500。
- 两轮View创建 / 移除、订阅 / 解绑各2次，LocalPlayer初始化 / 清理各2次；按世界分别核对，解绑后ViewDataChanged为0。两轮指定蓝图运行错误及Missing gameplay dependencies均为0。
- 新样例22:16:51再次正常保存；Controller哈希与B3主体清理后的基线一致，33份Source与收尾前基线一致。最终地图保存记录与用户清理确认共同记录。

当前样本中的两轮是非法 / 合法配置测试；正式短基线和最终100/60配置恢复以用户反馈及保存记录登记，不把它们计成额外直接观察的PIE。没有重复要求用户再跑一轮。实际界面50%、单HUD、Level Blueprint临时图表清理和场景实例删除由用户确认，本轮未用UE反射重新读取全部内部配置。

新样例 `BP_ETHealthWaitingCharacter_Rebuilt`：35330 bytes，22:16:51保存，SHA-256 `2E588FE38B5F05A2DFDD28A817644FEAE3599B5A3AE291109C8E5F40F841FB5B`。已纳入资产清单，现29项；旧Waiting保留停用标记。B3主体70 + 收尾10 = 80分钟。

## 6. M1-03B结论

[B1 Presenter](M1_03B1_Verification.md)、[B2 HUD](M1_03B2_Verification.md)及本批B3共同完成：Character / ASC属性经Presenter事件与快照驱动只读HUD；真实不同身体切换与旧源隔离、Waiting / Unsupported / NoPawn、早晚订阅、View重建和结束解绑已验收。B1结束保护在B2关闭，B3复制资产问题以原生新建样例关闭，当前B阶段没有未关闭补项。

本结论覆盖已实现的同步数据流，不声称已经完成正式死亡重生、异步资产加载或正式战斗能力。C仍待汇总阶段结论，后续按方案B复用本次和既有证据，不重新设计相同验收入口。
