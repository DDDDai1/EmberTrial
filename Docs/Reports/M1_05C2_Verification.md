# M1-05C2：BT自动近战攻击与生命周期验收

复核：2026-10-10。**M1-05C2验收通过，无待补项；M1-05C总任务完成。**用户15:20报告全项完成、无失败；手动35分钟（代码10 / 资产10 / 检查15），构建等待5分钟。助手读取源码、构建 / 索引、资产与运行日志并更新文档，没有代改Source / UE资产、重新Build或操控编辑器。M1-05D尚未开始。

继续已选方案B：关键新路径用直接证据核对，具体画面 / 操作与未逐项记录的配置以用户实测反馈登记，不重搭旧测试图。C1缺少的第二次连续命中与玩家对敌人25，本轮自动交战已经获得直接日志。

## 1. 六份源码、编译与索引

- 相对[C1最终基线](Raw/M1_05C1_Baseline.json)，恰好修改Character与AIController各两份源码，新增原生Melee Task头 / cpp；其余47份Source的SHA-256不变。共**53份Source / 26个cpp**，Build.cs / .uproject与既有GA / Trace / Life / UI实现未修改。
- [C2指南](../M1_05C2_BehaviorTreeMeleeAttack.md)16个C++片段全部匹配实际源码，去除空白 / 注释后核对。完整Task代码比较采用已记录的Public头文件include修正，不误把目录差异当遗漏。
- 实际Task头位于`EmberTrial/Source/EmberTrial/Public/AI/ETBTTask_MeleeAttack.h`，cpp在Private/AI，第一行`#include "AI/ETBTTask_MeleeAttack.h"`。首次C1083原因是按同目录写法引用了Public头，已由用户手动修正；没有Private重复副本，不要求搬迁文件。
- 每Controller独立Task实例；准备转向，接受后锁Yaw；OnAbilityEnded过滤自身Spec Handle。请求中同步结束暂存结果，Serial防止旧回调重入；正常结束与Abort显式解绑，只取消自己启动的攻击。Controller保留感知 / 生命订阅，并增加停止Guard与世界时间重试。
- Editor receipt：**2026-10-09 23:05:35.658**；Game receipt：**23:08:58.225**，均Win64 / Development / UE5.5.4，TargetType分别Editor / Game。最后源码修改23:03:18，早于两份构建记录。用户确认两次Build成功；最新UBT日志另显示Editor Target is up to date，助手没有重新构建。
- `cmake-build-debug/compile_commands.json`于23:06:00.080更新，去重后**26个项目cpp**，包含新Task。CMakeLists是两个模块的GLOB代码模型，不能把两条glob误当只有两个cpp；此处按实际编译数据库核对。

## 2. 新行为的直接运行证据

[日志摘录](Raw/M1_05C2_Acceptance_2026-10-10.log)与[结构化证据](Raw/M1_05C2_Evidence_2026-10-10.json)保存本轮范围。以下时间均为**Asia/Shanghai**，原日志UTC加8；样本集中在2026-10-10 15:16–15:20。

| 路径 | 直接观察与结论 |
| --- | --- |
| 运行参数 / 追击接攻击 | 7轮Blackboard ready，Stop90 / Melee100；Tree start Accepted1、Resume100；实际Move Radius90。15:16:13.310在87.4cm Prepare，Requested后Busy撤销资格但攻击继续，不被自己的门控中止 |
| 连续真实伤害 / HUD | 15:16:13.679 Enemy→Player Requested10 / Applied1，100→90 / Base90；15:16:15.360下一次90→80 / Base80，Presenter / HUD比例随之更新。全轮14次敌人真实伤害 |
| 能力结束而非Delay | 10次Task Succeeded，均Reason=AbilityEnded；例如15:16:14.344完成后，14.993下一次Prepare，约0.649秒的正常收招 / 调度间隔。没有实际Watchdog / InactiveWithoutEndEvent故障输出 |
| 活动攻击受击取消 / 恢复 | 15:17:40.908执行原ke RequestHitReact；40.910 Task Failed / AbilityCancelled、敌人受击与Trace清理；47.225 Hit end，47.241新Prepare，之后再次合法命中。15:19:39.403玩家真实25命中也取消敌人攻击 |
| 失败重试与配置恢复 | None GE负例41次AbilityRejected，40个相邻间隔**0.549–0.633秒，平均0.5541秒**，没有逐帧重试。15:18:59.810敌人GA正常保存；15:19:14.368恢复后再次Requested10 / Applied1，100→90 |
| 玩家反击 / 独立生命 | 15:19:39.403、41.486、43.471、45.537四次玩家→敌人25，敌人100→75→50→25→0；玩家同期被敌人合法扣血，双方Source / Target和HUD归属分明 |
| 目标死亡取消当前任务 | 15:19:56.635敌人新攻击开始；56.868玩家死亡、TargetDead清黑板目标 / HUD0；56.869 Task Failed / ContextLost、Disarm Tick0、轻攻击Cancelled1 / AttackingTag0。之后该轮无新Requested |
| 自身死亡停树 | 15:19:45.537敌人归零，BodyDead清目标并停止树、死亡封锁TraceTick0；该轮之后无新Requested。本样本发生在收招间隔，不能将其写成“攻击窗口内自身死亡” |
| 暂停恢复 | 15:19:14.487–18.619、19.002–19.801两次暂停恢复发生在同一攻击期间，19.936才正常完成；22.500还有一次恢复。三次Pause opened / closed成对，无额外请求风暴 |
| 动作中Stop与下一轮 | 四次ControllerEndPlay中活动Task Abort并清理；例如15:16:18.745中止，15:16:39.318下一轮黑板重新初始化，双方新身体生命100。没有未结算旧任务 |

共**58次Prepare = 54次Finish + 4次Abort**，任务输出完整闭合；Finish分别10次AbilityEnded、2次AbilityCancelled、41次AbilityRejected、1次ContextLost。17次Enemy Begin与17次End相配，End均AttackingTag0。相关Blueprint Error / Accessed None / Fatal error / 项目LogET Error匹配为0；这是本轮相关日志范围，不代表引擎启动没有任何提示。

## 3. 用户反馈与证据边界

用户确认：BB四键非同步、Controller90 / 100 / 100 / 0.5、BT父连接与两个Decorator、攻击中方向固定 / 侧移效果、暂停鼠标 / 键盘输入边界、最终slomo1 / 血量 / GA / Debug配置、E-P与移动转向跳跃短基线、保存均通过。

固定Yaw、Lower Priority攻击分支等有原生逻辑与自动攻击继续运行的佐证，但本次**没有逐字段解析BT / BP或录制画面**。日志中的Unique0包括受击取消 / 死亡 / Stop，不能把它们全部当成侧移空挥；侧移表现按用户实测登记。三次恢复日志也不能单独识别究竟用了鼠标还是Enter / Space，该输入方式以反馈为准。

自身死亡停树本轮直接成立，样本在收招；玩家死亡时敌人攻击活动中明确取消。C2要求新Task能退出，不为未指定的同帧 / 窗口内自身死亡重新增加测试准备。D将复用已通过链路收尾，只补组合证据确有必要的部分，不重做C2全部检查。

## 4. 资产与保存

- 相对C1清单40项，**仅BB、AIController BP、BT和Enemy GA四项哈希改变**，其余36项不变，无新增UE资产，清单仍40。
- BB / Controller / BT正常保存于2026-10-09 23:12:39 / 23:13:50 / 23:17:41，配置在昨晚完成，今天执行运行检查。不能把这些写成今天15:20的新保存。
- Enemy GA在15:18:20.412保存临时负例，**15:18:59.810再次正常保存**，随后15:19新PIE真实10点伤害直接支持GE恢复。最终Class配置还以用户反馈为准，不要求重复保存制造差异。
- 地图 / Enemy BP / Player BP与C1哈希一致；本章复用既有身体 / 输入 / 导航，没有新测试图。完整[资产清单](../AssetManifest.csv)更新用途 / 当前阶段与实际SHA-256；[C2最终基线](Raw/M1_05C2_Baseline.json)记录53Source / 26cpp / 40资产，Accepted=true，PendingChecks为空。

## 5. 工时、Git与交接

本批手动**35分钟**（10 / 10 / 15）、等待**5分钟**，检查在10–15预算内。M1-05 A+B1+B2+C1+C2累计**147 / 等待25分钟**；项目累计手动**1521分钟（25小时21分钟）**、明确等待**120分钟**。未新建Controller测试函数 / 变量 / 按键，不计算与旧不同范围章节的精确节省率。

实际实现已由用户提交到**9cdbc75**，本机HEAD与缓存origin/main记录同哈希；本轮没有再次联网验证或代Commit / Push。原暂存的个人Editor偏好删除仍待Commit，磁盘文件保留。本次新增的验收摘要、资产清单与状态文档可另作收尾提交：`docs: accept M1-05C2 and record verification evidence`；按[C2指南最后清单](../M1_05C2_BehaviorTreeMeleeAttack.md#11-本章需要commit--push的内容固定放在文档最后)上传，Raw继续留本地。

本报告首次完成时D尚未下发。**后续更新：2026-10-10 [D复盘](../M1_05D_EnemyCombatReview.md)与[M1-05总验收](M1_05_Verification.md)已完成**，复用本轮真实交战 / 两种死亡 / 取消 / 暂停与Stop证据，没有新增运行改动或检查缺口；整章通过。可选视频不阻塞验收，C2 / D文档收尾可合并为`docs: close M1-05 enemy combat milestone`。性能测量、新阶段包、正式重试 / 倒地动画仍属于后续范围，不宣称最终作品完成。
