# M1-05B2：真实追击实施复核与等待分支补修

复核：2026-10-08。**M1-05B2验收通过，两项窄补及等待父连接修复全部关闭。**20:09活动Move受击取消 / 恢复通过；20:27补修后6次失败请求间隔0.549–0.551秒，已保存BT运行结构正确。用户手动共32 / 等待5分钟；日志UTC换算Asia/Shanghai。助手只读源码 / 资产 / 日志并更新文档，没有操控编辑器、修改Source / UE资产或执行Build。第3.2节保留修复前的错误证据，最终结论见第3.3节。

## 1. 源码、构建与资产

- 六份文件去除空白后与[B2指南](../M1_05B2_BehaviorTreePursuit.md)全部一致。相对B1仅既有Controller两文件改变，其余45Source逐份SHA-256不变；新增Service / Task四文件后51Source。
- 两个新头文件实际在`Source/EmberTrial/Public/AI`，cpp在Private/AI，指南建议的头文件位置是Private/AI；内容一致且已成功构建，不是此次运行失败。此目录差异如实登记，后续源码结构整理再收口，不为本轮新增额外功能矩阵。
- Editor receipt19:24:42、Game receipt19:26:27均UE5.5.4 / Win64 / Development，TargetType分别Editor / Game；CLion compile_commands19:25:17有25份项目cpp，包括两个新BT cpp。实际双Build与头文件导航用户确认通过，receipt / 索引直接读取；没有重跑Build。
- BB19:30:13、BT19:40:01、Controller BP19:40:36已正常落盘。旧38资产仅BB / Controller BP改变，其他36项哈希不变；BT新增登记后清单39。原敌人BP和地图与B1完全相同，地图最后保存18:37:37。临时箱删除 / 导航恢复据用户确认及19:47:21删除、19:47:24导航重建日志；最终哈希未留下地图变更，但不据此推断全部运行时布局。
- 三键 / 类型、根Service、Decorator Both、Radius绑定先依据用户反馈；运行中Radius=150、Tree Accepted=1提供佐证。本次只读解析BT运行节点，发现等待父连接错误，详见第3节；不能将节点摆放在右侧当作父连接正确。
- 窄补后51Source / 39资产哈希仍与上一B2检查点一致。20:08:47.930有导航恢复重建与MapCheck零错误 / 警告；20:09:17仅有地图自动保存，最后正常保存仍18:37:37。用户报告Save All / 普通追击恢复，此部分据反馈，不把自动保存当正式落盘或日志没有的恢复PIE。
- 上面20:09检查点为补修前状态，已备份到[补修前检查点](Raw/M1_05B2_PreRepairCheckpoint_2026-10-08.json)。最终[验收基线](Raw/M1_05B2_Baseline.json)51Source / 39资产，`AllStageChecksAccepted=true`、PendingChecks为空；Source不变，仅BT与地图相对补修前哈希更新。B1基线保留。

## 2. 直接日志已覆盖部分

[原始摘录](Raw/M1_05B2_Acceptance_2026-10-08.log)保存五轮初始化、请求与结果、暂停、伤害、死亡和清理。当前有5次Accepted=1启动，20次Move request，Radius全部150，未匹配本批配置拒绝或Blueprint Error / Accessed None / Fatal error。

- 第一轮19:45:00.694取得玩家并Allowed1，19:45:02.760移动Succeeded，02.910在148.8cm、Holding1停追；随后202.2、200.7、202.4、201.9cm恢复，148.6–149.3cm再停。最后一轮也有200.6–200.9cm恢复与147.3–150.9cm停步。这支持迟滞规则实际执行；具体路线与移动目标跟随画面按用户反馈通过。
- 部分近处停止前Move finished=Failed，紧接NearTarget，符合Controller主动撤销路径的设计；不能把这些Failed记为无路径样本。19:46:57–19:47:16第二轮存在多次SightLost / 重新取得与对应取消；低箱绕行的实际画面用户确认通过，日志本身不证明箱体高度或路径形状。
- 19:49:38.666、40.059、41.209玩家真实攻击分别使敌人100→75→50→25，且身体HitReact标签与Montage正常开始 / 结束。19:49:42.826归零时BodyDead清目标并Tree stopped，随后死亡封锁；同轮直到退出没有新Move request。下一轮独立100/100与BT启动正常。
- 第四轮19:50:01.952路径InProgress，03.088–08.202和09.552–10.968两次暂停 / 恢复；期间无Move finished或重复目标，恢复后11.651于150.4cm停追。鼠标 / 键盘继续与恢复输入根据用户反馈，日志确认两次同一正式入口。
- 19:50:21.918在202.1cm重新追击，22.484玩家归零时当前Move结束、TargetDead关门控 / 清目标，HUD0；尸体未被再次取得，树至38.919退出时才停止。支持玩家死亡只停追、自身 / 结束才停整树。
- 第五轮19:51:16.264仍有路径请求，16.770停止PIE时Move finished、关门控、Tree stopped、Target cleared、Controller End与Unpossess / Ending1齐全；后无新的本轮Move。结束时两条Allowed0日志分别来自停止与强制清目标，不等于重复启动树。
- `slomo 0,2`是中途输入，随后19:48:36正确输入0.2，19:49:27.553恢复1；不把逗号命令当成功慢速证据。最终短基线、临时设置恢复与资产保存按用户反馈登记。

## 3. 首轮证据缺口与用户选择（历史）

1. **移动中受击取消 / 恢复：**19:48:56.549已Succeeded，57.448已经NearTarget /149.9cm；19:49:01.788才RequestHitReact。该样本证明原生受击请求与标签事件，但受击前路径已结束。当前全部四次HitReact Count1都没有相邻Busy门控转换，不能证明“活动路径被受击中断”。
2. **目标稳定时无路径等待：**20次Move request全部InProgress，没有立即Result=Failed；现有失败对应SightLost、NearTarget、TargetDead或退出。没有持续有效目标下约0.5秒间隔的独立失败请求序列，不能把SightLost取消当无路径退路。

当时两项都是触发条件缺失，尚无运行失败证据。用户选择**“我按简化步骤补测这两项”**，按第11节执行并于20:09反馈。下面是本次读取的实际结果，取代此前待取样状态。

### 3.1 活动路径受击取消与恢复：通过，关闭

[窄补原始摘录](Raw/M1_05B2_NarrowCheck_2026-10-08.log)保存本轮相关记录；[统计与BT结构证据](Raw/M1_05B2_NarrowEvidence_2026-10-08.json)保存失败间隔与实际父连接。

20:06:00.034发起Move / InProgress；20:06:28.287受击Tag Count1，同一时刻当前Move结束、Allowed0 / Busy / Distance246.0cm。20:06:40.935 Count0、Allowed1 / CanChase / 246.0cm，40.951新Move / InProgress；50.769 Succeeded、52.952 NearTarget150.0。受击前没有Succeeded / NearTarget结束路径；20:06:57.187恢复slomo1、20:07:10.449结束清理。此项不再重测。

### 3.2 修复前：无路径等待失败与父连接定位（历史）

20:08:06.655取得玩家并Allowed1；06.671–18.571有**358次立即Failed请求**，357个间隔最小0.032、最大0.035、平均**0.033333秒**。期间没有SightLost / Target cleared，直到18.629退出才清目标。成功制造了需要的触发条件，但没有等待0.5秒；不能按用户“通过”文本覆盖实际日志。

只读解析已保存BT（SHA256 `91D0386210AED02F01C8604554B620FCBD58A6F5B131D497EF39BC90A960D26E`）的运行节点，确认数值正确、父连接错误：

| 运行节点 | 实际序列化连接 / 值 | 正确结构 |
| --- | --- | --- |
| 根Selector（export11） | Children=1，仅Sequence（12） | 左Sequence、右IdleOrRetry两个子节点 |
| Sequence（12） | Children=3，任务依次Move（17）、IdleOrRetry（14）、ArrivalSettle（15） | 仅Move与ArrivalSettle两个任务 |
| IdleOrRetry（14） | ParentNode=12 / Sequence；Wait Time=0.5 | ParentNode=11 / Selector；0.5不变 |
| ArrivalSettle（15） | ParentNode=12 / Sequence；Wait Time=0.25 | 保持当前父连接与0.25 |

本机5.5.4的`BTComposite_Sequence.cpp`确认子任务失败直接返回，只在Succeeded时执行下一任务，所以Move失败跳过同Sequence后两个Wait。`BTComposite_Selector.cpp`确认失败尝试下一子节点；当前根没有第二子节点，因而高频重新搜索。0.2秒Service不是重试间隔，不修改它来掩盖连线错误。

仅按[指南第12节](../M1_05B2_BehaviorTreePursuit.md)把IdleOrRetry顶部输入从Sequence改接Selector底部输出、保持右侧；Save BT后重查三次失败等待、导航恢复 / Save All与普通追击。不改C++、不Build、不重做受击和旧矩阵。第6.2节已增加沿线检查父节点，避免仅凭位置判断父子关系。

### 3.3 20:27等待分支补修：通过，关闭

[本次原始摘录](Raw/M1_05B2_RepairCheck_2026-10-08.log)与[间隔 / BT结构证据](Raw/M1_05B2_RepairEvidence_2026-10-08.json)已保存。用户手动改连接并反馈全部通过、额外2分钟。

- BT于20:26:21.538正常保存，SHA256更新为`23D82E7EE08C1CAD2EF4A40916686B8EC5F5604E465B6B69A26AB37C92672085`。只读解析确认Selector两个子节点为Sequence与IdleOrRetry；Sequence两个任务为Move与ArrivalSettle；IdleOrRetry Parent=Selector / Wait0.5，ArrivalSettle Parent=Sequence / Wait0.25。数值保留，父连接已修正。
- 20:27:17.686 Tree Accepted1，21.828取得当前玩家、Allowed1；21.845、22.394、22.945、23.494、24.045、24.595共6次Move request / finished Failed。5个请求间隔0.549 / 0.551 / 0.549 / 0.551 / 0.550秒，平均**0.550秒**，期间没有SightLost / Target cleared，24.668停止PIE才关门控、停树、清目标。
- Wait常量为0.5秒；实际请求之间还包含任务结束、行为树重新搜索与帧调度，当前约0.55秒符合“失败后等待再重试”，不要求墙钟间隔精确0.500。已消除修复前约30Hz的失败请求。
- 20:26:25.741正式地图落盘，当前SHA256 `2A6B081614AE32427085A9AC800B3F5B14640C6FC3506A9121F8429E050C4EB1`。20:27:38.181导航恢复重建、38.207 MapCheck零错误 / 警告。此日志窗口没有恢复后新PIE或再次地图Save，slomo1、最终Save All与普通追击恢复按用户反馈通过；正式保存发生在窄测前，不将其误写成窄测后保存。
- 51Source与补修前完全一致；39资产仅BT / 地图哈希改变，其他37项不变。没有新增源码、资产、构建或测试入口。受击与其他已通过项保留，B2无剩余PendingCheck，可以准备M1-05C。

## 4. 工时

实施手动25分钟：代码10 / 资产10 / 检查5，等待5；20:09窄补5 + 20:27补修2，**本批累计手动32 / 等待5分钟**。项目累计手动**1451分钟（24小时11分钟）**，明确报告等待**110分钟**；M1-05 A+B1+B2手动77 / 等待15。全部实耗据用户反馈登记；助手只读复核不算用户手动时间。

用户常规检查报5分钟，与指南首次预算15–25分钟不同，不计算精确节省率。方案B窄补识别并修复了一个实际连接错误，额外7分钟完成两项专项与修复，未重做旧矩阵。B2已验收；Cook / 阶段包、Insights性能测量与AI主动攻击仍待后续实现。
