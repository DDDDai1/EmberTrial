# M1-07B：Insights基线分析与采集核查

原采集2026-10-13；更新2026-10-14，UE **5.5.4**。**B性能分析与最终函数Editor收尾接受。**真实交战材料有效、MoveRadius绑定已修复；精简函数与指南一致、Editor成功，最新功能正常采用用户反馈。Game最终编译转C1、旧Attempt01保留历史包，C3再生成Attempt02。此前23:49残余现象未有唯一根因，不把只读日志归因为玩法修复；细节与证据边界见7.6。

## 1. 材料与工程一致性

实际读取材料：

- `Artifacts/M1_07B/Runtime/Trace01_20261013_141040_853.utrace`：2,594,864,617字节，约2.42GiB；Insights解析时长811.338843秒。
- 同名Runtime.log / JSON；启动时间2026-10-13 14:10:40.868（北京时间）。Trace停止后游戏仍运行到14:24:26正常退出。
- 用户导出`Artifacts/M1_07B/Idle01_Timers.csv`；UTF-16，237,728行（含All汇总）。
- 助手导出与审查原件在`Docs/Reports/Raw/M1_07B_Analysis_2026-10-13`，包括Regions统计、逐事件CPU / GPU、线程与Timer字典、解析日志、ReviewEvidence / CpuSummary / GpuSummary JSON。均留本地。

对A基线重新逐项SHA256核对：**69份Source、46项清单资产、两份共享Config和七份包指纹文件全部一致**；包指纹范围为启动exe、实际Game exe与五份Pak / IoStore容器，不代表遍历所有第三方DLL。Source仍34cpp。冻结B1文档哈希保持`FBA8C03A1656C619EDF219864535D80AB577F3588A0F611E5746638EF1133E09`。

本轮没有改工程源码 / 资产 / 共享配置、执行Build或Cook，也没有Stage / Commit / Push。助手运行已安装Insights进行本地导出，并通过computer-use核对窗口与一次主线程CSV导出。

## 2. 实际条件与限制

| 项目 | 证据与结论 |
| --- | --- |
| 包 | A的Win64 Development Attempt01；启动exe指纹与A一致 |
| CPU / GPU | Ryzen 7 7840H / D3D12选择RTX4060 Laptop；NVIDIA驱动595.71 |
| 分辨率 | 启动参数1920×1080；包Saved配置也为1920×1080。用户不清楚窗口是否缩小 / 裁剪，未独立测量实际Viewport / Backbuffer尺寸 |
| 画质 / 帧率 | 十个sg组均2；六项查询直接确认t.MaxFPS=0、VSync=0、ScreenPercentage=100、DynamicRes.OperationMode=0、Shadow=2、GI=2。Lumen GI / Reflection Method=1、Nanite=1另有查询 |
| 电源 / 风扇 | 用户反馈接电准备完成，但具体电源 / 风扇档位“不清楚”；不登记成已确认性能模式 |
| 内存 | 启动14:10:43.736：系统物理可用558.51MB / 总14018.36MB，存在背景内存压力；进程当时190.41MB。这不是稳态内存快照，也不能证明后面某个尖峰由换页导致 |
| 通道 | Runtime确认Cpu / Frame / Gpu / Bookmark / Region / LoadTime / Log全部启用；没有采集Memory，不能推出泄漏或稳态内存峰值 |
| 开销 | Development、业务日志、statnamedevents与Trace均启用；没有同场景无Trace、Shipping或其他机器对照 |

最初两次NoUI解析在完整导出前结束，不采用其部分汇总。后续普通模式的`InsightsExport_Attempt03.log`明确记录**Analysis has completed**、执行Export.rsp及四Region / 116,445条CPU事件导出；GPU独立导出日志同样完整结束、退出0。早期误用GPU根Timer名字得到0行，只表示过滤未匹配；后续GPU1 / GPU2短区间实际导出已有数据，不把0行解释为GPU缺失。分析器的Tag Tracker诊断不是游戏Runtime错误，且本轮没有Memory分配记录可用于结论。

## 3. Region核对：名字不代替真实行为

起止为Trace相对秒数，来自Insights RegionProvider导出日志：

| Region | 起止 / 时长 | 实际内容 |
| --- | --- | --- |
| Idle01 | 429.721037–481.014642 / 51.294s | 可用于待机；内部440–450s用于CPU / GPU同窗比较 |
| Pause01 | 508.392636–558.041207 / 49.649s | 包含暂停前后控制台和恢复；真正Pause shown→removed只有14:19:14.872–14:19:31.675，约16.803s |
| Combat01 | 592.458997–627.279081 / 34.820s | 玩家攻击 / 闪避；**没有真实AI追击 / 攻击**，仅保留为玩家动作附加样本 |
| Idle02 | 758.036349–789.519791 / 31.483s | 再次NewRun后的Entry待机；内部768–778s用于比较 |

结合启动时间、Runtime暂停时序与区域位置，Pause只取远离边界的**520–528s**（8秒）分析；不把49.649秒全算成暂停。待机内部窗口避免Region输入首尾；两次同为Entry但后台负载 / 电源档位未完全受控，差异不宣称优化收益。用户预热准备按反馈记录，未把Region时长当作预热时长。

## 4. CPU：逐事件与主循环间隔

官方ExportTimingEvents导出GameThread的`FEngineLoop::Tick`（Timer3319），包含116,445条，其中最后一条EndTime为Inf，是Trace停止时尚未闭合的事件，排除。下表只统计完整落入窗口的有限事件；间隔用相邻Tick.StartTime之差计算，P95用最近秩法`ceil(0.95*N)`，单位ms。事件数与间隔样本数并非总相同，窗口尾部下一事件开始不在窗口时不计间隔。

| 实际窗口 | 完整Tick数 | Tick均值 | 主循环间隔均值 | 间隔P95 | 间隔最大 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Idle01内部440–450s | 1359 | 7.349 | 7.350 | 10.149 | 11.233 |
| Idle02内部768–778s | 1403 | 7.122 | 7.122 | 9.900 | 11.332 |
| 实际暂停内部520–528s | 1096 | 7.292 | 7.292 | 10.062 | 10.851 |
| Combat01整段，仅玩家动作 | 4779 | 7.284 | 7.285 | 10.104 | 11.045 |

两段完整Idle Region的循环间隔均值另为7.356 / 7.113ms，P95为10.262 / 9.908ms，和内部结果同量级。内部窗口平均差约3.1%，没有代码变动或严格干扰控制，不写成“优化提升3.1%”。

**这些是游戏主循环间隔与Tick持续时间，不是显示器实际呈现FPS，也不是排除等待后的CPU运算成本。**FrameProvider、GameThread、RenderThread和GPU时间互相有重叠，不能相加换算FPS。

Region统计中Idle01的`FDrawSceneCommand`每调用Inclusive约6.908ms、`RenderViewFamily`约6.861ms，均包含下层 / 等待，不能相加。RHI_Translate在多线程累计68.738s，WaitForTasks累计54.456s，不可拿它们除以窗口长度声称一帧成本或直接判定RHI瓶颈。

Idle01的`Slate::Tick (Time and Widgets)`6973次、Inclusive1.181377s，约**0.169ms / 调用**；这包括Slate整体，不是单个HealthHUD专属耗时。没有证据要求重写当前Presenter / HUD或添加轮询优化。

用户的小范围CSV里Tick384次、Inclusive2.862888s，约7.455ms / 调用，适合作为首次表格练习。其All累计118.075s是父子包含 / 多线程汇总，不是选区墙钟时间；不能写“待机118秒”或用All计算帧率。该CSV没有CPU / GPU类型列，同名事件可能分属两类，所以具体GPU结论采用下面带ThreadId / TimerId的逐事件导出。

## 5. GPU：实际场景范围与渲染阶段

GPU1 / GPU2按同一窗口导出，分别82,962 / 85,658 / 66,926条事件（包含与边界相交项）；本轮实际事件均在GPU1，GPU2无记录。剔除窗口边界后，筛选Depth0、名字`Unaccounted -  Frame: <编号>`的完整场景范围：

| 实际窗口 | 场景范围数 | 均值ms | P50ms | P95ms | 最大ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Idle01内部440–450s | 1359 | 6.709 | 6.461 | 9.485 | 9.934 |
| Idle02内部768–778s | 1403 | 6.488 | 6.235 | 9.383 | 9.572 |
| 实际暂停内部520–528s | 1096 | 6.702 | 6.442 | 9.591 | 9.802 |

本机`DeferredShadingRenderer.cpp:1567`的`RDG_GPU_STAT_SCOPE_VERBOSE(GraphBuilder, Unaccounted, *FrameNumDescription)`说明它是包住场景渲染的GPU范围。**名字里的Unaccounted不代表整个范围都是无法归因的独占成本。**它包含子Pass / 间隔；范围外还有RayTracingGeometry、DistanceFields与SlateUI等。上表不冒充整张GPU Frame、所有队列忙碌时间或最终呈现延迟。

Idle01内部同一GPU1、完整包含在这些场景范围内的Depth1阶段累计时间除以1359个场景范围，得到以下观察值（ms / 场景范围）：

| 阶段 | 观察值 |
| --- | ---: |
| RenderDeferredLighting | 2.206 |
| VolumetricCloud | 1.694 |
| ShadowDepths | 0.428 |
| Postprocessing | 0.397 |
| CaptureConvolveSkyEnvMap | 0.309 |
| Basepass | 0.191 |

这避免把某阶段多次调用误当一次 / 帧。相同阶段的不同Depth含嵌套，不能继续把所有Inclusive相加。后续扩美术 / 光照时优先观察这些实际GPU范围；当前数据尚未构成确定的CPU / GPU瓶颈诊断，不因云或光照排前面就立即关闭效果。暂停仍渲染场景与UI，因此暂停GPU范围和待机接近属于可解释现象。

## 6. 尖峰与重试

- 全Trace最大**有限Tick**：相对5.972683–6.810834s，**838.151ms**，是导出里的第一个主循环Tick，位于启动期而非预热后的稳定窗口。该汇总没有展开其加载 / PSO调用树；不认定具体根因、不拿启动峰混入Idle P95。若后续优化首屏加载，再专项展开。
- 首次NewRun：Runtime14:11:16.335请求、16.550确认到达，**215ms**；相对35.374516–35.604717s的Tick约230.200ms，与首次进场时间附近一致，但不把它简单等同某个资源读盘耗时。
- 代表性Retry：14:21:43.565请求、43.639到达，**74ms**；相对662.608207–662.685232s的Tick为**77.025ms**，对应同一重试切图附近。整图重新创建身体 / UI的同步过程是该长帧的主要调查区域；没有进一步调用树证据，不精确归因为GC或某个资产。
- 玩家14:21:35.461死亡、失败页出现，随后8秒以上才选择Retry，是用户操作等待，不能算成加载耗时。Idle02前还发生MainMenu / NewRun，不能将其称作Retry后立刻的纯恢复窗口。
- Trace停止边界的Inf事件不参与最大值，不能写无限长帧。现有Runtime无匹配Error / Fatal / Accessed None / Ensure；唯一Warning是已登记GameplayCueNotifyPaths回退扫描，留C评估，未证明它造成上述峰值。

## 7. Combat02补采、真实交战与本次新发现

### 7.1 材料与有效窗口：提前死亡不要求重采

实际文件`Artifacts/M1_07B/Runtime/Combat02_20261013_195848_809.utrace`为338,148,518字节，约322.5MiB；同名Log / JSON启动19:58:48.928、正常退出20:00:40。本轮Insights完整解析106.2秒Trace，导出15,074条GameThread Tick与321,618条GPU事件（GPU导出50–85s）；原件 / 命令 / 证据在`Docs/Reports/Raw/M1_07B_Combat02_2026-10-13`。最后未闭合Tick排除，仍使用第4–5节口径。

同一包、元数据 / Runtime确认原Profile与六项值一致；69Source / 46资产 / 两Config / 七包指纹重新核对仍与A相同。启动可用物理内存633.85MB，环境仍有压力；电源档位与实际Viewport未知限制不变。NewRun19:58:57.444到达，19:59:41.170首次获得目标，间隔约43.7秒；这次没有严格等满进入世界后的60秒预热，不宣称与原Idle完全同温度 / 视角对照，也不因该限制重采。

RegionProvider发现两条同名记录：

- 第一条`M1_07B_Combat02`：**61.841781–86.771919s / 24.930s**，包含死亡与失败页开头。
- 第二条：**96.911859–96.911859s / 0秒**，位于Retry以后；没有可用区间，不能当第二段交战或合并进统计。

根据采样起点与Runtime事件时序交叉定位，保守只取**63–82s（19秒）**：避开开始输入、死亡边界与第二条空标记。原Runtime记录19:59:49.501起真实扣血、20:00:13.958死亡 / Failure shown；在内部窗口仍有多次真实攻击、伤害 / HUD更新、SightLost / 重获与移动。死亡前样本足够，所以提前死亡不是补采失败，不要求再次操作控制台或补Trace。

| 死亡前内部63–82s | 样本数 | 均值ms | P50ms | P95ms | 最大ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 主循环开始间隔 | 2864 | 6.630 | 6.436 | 8.929 | 10.261 |
| GPU场景范围（不冒充整GPU Frame） | 2865 | 6.003 | 5.807 | 8.429 | 10.088 |

Tick均值另为6.630ms。GPU场景内Depth1均值 / 场景范围：RenderDeferredLighting2.571、VolumetricCloud0.624、ShadowDepths0.444、Postprocessing0.396ms。战斗期间镜头 / 可见场景不同，不能用与Idle较低的时间声称代码优化收益；此时没有代码变化，数值属于Attempt01本次实测。

真实业务证据（以下为**整轮Runtime**计数，不全部冒充内部19秒）：3次Target acquired；28次Move request（4次InProgress、24次立即Succeeded）；12次敌人Melee Requested，10次真实10点伤害 / Base同步，玩家100→0，HUD同步、Death sealed / Failure shown一次。20:00:21.319 Retry请求、21.393到达 / 新身体100；死亡→用户点击等待不算加载时间。Runtime Error / Fatal / Accessed None / Ensure匹配0，已登记GameplayCue路径Warning仍存在。

### 7.2 已确认问题：实际Move150与决策90不一致

这次真实交战补采额外暴露配置 / 默认绑定回归：

1. 19:58:57.430黑板日志**Stop90 / Melee100**，57.443树启动**Resume100**，`ETAIController.cpp:185`确实把PursuitStopDistance写入Float键。
2. 19:59:41.177、54.467等真实路径请求却为**Radius150**；43.983第一次移动成功后，44.260–48.930约每0.27秒重复立即Succeeded，直到玩家进一步靠近至94.7cm才开始攻击。这说明两处到达判定没有使用同一距离；不是逐帧请求风暴，也没有证据称它导致FPS瓶颈。
3. Combat02分析时的`ETBTTask_PursuitMove.cpp`构造函数先`AcceptableRadius.SetKey(...)`、后`AcceptableRadius = 150.0f`。UE5.5.4的AcceptableRadius是`FValueOrBBKey_Float`；float构造临时结构再赋值会同时复制默认None Key，因此**后一次常量赋值清掉刚设置的键**。
4. 正确顺序应先给常量，再SetKey；BT节点也应明确绑定PursuitStopDistance，以免资产实例保持旧None / 常量配置。包运行150已成立，但资产保存的每个字段没有逐项读取，不宣称只交换源码两行就已验证资产或旧包自动修复。

原B2指南顺序正确，M1-06A第7.1节改名示例只展示了两项引用、未把中间常量赋值一起展示，容易改变相对顺序；当时验收只核对改名，漏查此项。现在补齐原指南并记录历史验收遗漏。**助手没有直接改Source / BT或Build；修复仍由用户按B指南8.2手动执行。**

### 7.3 当时收口与包边界（8.2绑定已完成，当前以7.5为准）

真实交战的性能材料缺口已关闭，原Idle / Pause / Retry继续有效，无需再采Insights。当前测量没有证明需要立即重写HUD或进行渲染优化；但发现真实行为参数回归，不能掩盖后继续下发C。

按[B指南8.2](../M1_07B_InsightsBaseline.md#moveradius-fix)只交换两行、确认BT键，双目标Build后做约1分钟PIE：有效目标下实际Radius90、敌人主动接近并造成连续两次10伤害。未改玩家 / UI，旧矩阵不重跑。反馈通过后关闭该收尾项、进入C有限校验，校验规则应纳入此类绑定错误。

此时**不重新Cook / 不采第三次Trace**。Attempt01保留为已测历史包，包含本次Radius回归，不宣传成修复后的包。C源码 / 配置收口后再统一出Attempt02，并做包内真实接近 / 攻击短检查；这一后续新包任务必须保留，不误记当前旧exe已自动修复。不要用旧包验证新Source。

### 7.4 23:50反馈：绑定通过，主动攻击条件未通过

用户报告双Build / 保存完成、实际Radius90、HUD100→90→80，但补充敌人停下后不攻击、仍需玩家贴近。以后一句具体异常修正前面的笼统“完成”，不能误记全项通过。

直接核对：Task.cpp先常量后SetKey正确；Editor DLL / receipt于23:46:16 / 18，Game receipt23:47:29，用户确认双Build成功。相对A仅Task.cpp一份Source变化，其他68份与46项资产 / 两Config不变；BT哈希仍8D13643C5E7E2D0B09DB981C001FAB40D051366108BAAFEE06C81855EC5CC0A2，核对 / Save没有新内容差异。

23:49:39.627实际Radius90 InProgress；42.241第一次Succeeded，之后重复Move / Succeeded，直到47.807在83.8cm才Melee Allowed1 / Prepare。48.162 / 49.840两次真实10伤害成立，但发生在玩家再次靠近之后。所有18条Move request为90，绑定修复成立；自主接近后攻击仍未通过，非当前设计的正常表现。

助手用UE只读Python / ObjectExporterT3D查看保存资产与类默认：AcceptableRadius.Key=PursuitStopDistance；两项ReachTestIncludes均Key=None / Default=False；TrackMovingGoal=True / ProjectGoalLocation=False；Selector攻击第一分支LowerPriority，追击第二分支Both，Root有Pursuit Service。排除建议再次取消正确的开关或移动正确节点。脚本首轮属性接口未暴露根节点，后改用已知导出子对象路径 / 原生属性名完成读取；接口读取失败不当作游戏运行错误。仅加载地图 / 读取 / 导出Raw，没有保存资产或运行PIE。

地图敌人Capsule42、Scale1、MeshNoCollision；玩家类默认Capsule35、MeshQueryOnly但PawnIgnore。尚无证据指向它们导致首次停止，不盲目改碰撞。引擎PathFollowing的Success也可能由目标碰撞等条件结束，现有日志只写Result / 请求Radius，没有成功瞬间的实际中心距离，不能只根据90宣称此时距离小于100，更不能编造确定根因。

原始片段 / 只读BT属性、地图身体与结构化结果在`Raw/M1_07B_MoveRadius_2026-10-13`。B指南8.3提供仅增强原Move finished日志的手动C++，记录中心 / Nav距离、到达半径 / 开关、地面 / Idle / Melee资格与位置，保持Super清理、没有玩法修改。该代码尚未实际写入Source / Build，下一次只需Editor Build / 一次5–8秒站定复现，不重新采Trace或Cook。取得数字后再给确定修复，C暂缓。

### 7.5 2026-10-14新到达快照：当次自主攻击通过，函数精简待落地

用户报告Editor Build通过、玩家站定 / 敌人自主攻击，手动5分钟；未提供完成时分或单列构建等待。源文件00:18:05，Editor DLL / receipt00:24:53晚于源码；Game receipt仍2026-10-13 23:47:29，不能宣称长诊断函数已有新Game Build。

00:26:19.670 Move Radius90；22.950 Melee gate Allowed1 / Distance90.3；22.951快照中心 / Nav距离均90.3、两半径开关0、Idle1 / Ground1 / MeleeAllowed1 / 目标匹配1 / Standalone0；随后NearTarget关闭追击并进入敌人攻击，23.322 / 25.001真实10伤害使100→90→80。用户站定描述与主动接近 / 攻击时序一致，本次样本通过，不再要求重采Trace。

该快照Result=Failed与追击主动停止 / 攻击交接同帧出现。Controller在到达90+1容差后StopMovement，原生MoveTo将非成功的AITask结束映射为Failed；本轮后续攻击确实成功，不能把这一行单独称作攻击失败，更不改写引擎Result以美化日志。

相对A仍只有Task.cpp一份Source变化，68旧Source、46资产与两Config不变。文档代码与当前Source均只添加只读快照、保留Super，并没有修改攻击条件；23:49的残余未攻击原因没有唯一实证，历史记录保留，不宣称“加日志修好了玩法”或“永久消除偶发问题”。

按用户要求直接覆盖B指南8.3.2的原长函数：最终版保留Controller / Result / Goal / Distance2D / Radius，在原生清理前取值、清理后输出；删除Nav距离 / 位置 / 多项状态、通用Actor读取减少具体Character依赖。清理必需、距离与半径诊断可持续保留；没有可量化性能收益宣称。临时World / CharacterMovement include改为显式Pawn.h；没有新.h / 模块 / 资产 / 架构改动。

原日志片段与结构化结果位于`Raw/M1_07B_FinalFunction_2026-10-14`。最终代码目前仅文档，Source仍长诊断版；用户手动替换 / 正常Editor + Game Build / 一条站定两次命中短基线后完成B收尾、进入C。旧包后续统一Attempt02，不用本次再Cook。

### 7.6 最终精简版落地：Editor收尾接受，Game检查转C1

用户最新报告“修改完成，功能也都正常，进入下一步”。直接读取当前Task.cpp，22行OnTaskFinished与指南最终函数完全一致；保留清理前缓存与无条件Super，常量150先赋值、后SetKey顺序正确。69份Source中仅Task.cpp相对A改变，其他68份、46项清单资产、两Config与七包指纹文件仍一致；冻结B1哈希不变。当前仍无C模块，未提前把计划代码计为实施。

- Source保存2026-10-14 **00:38:21**；Editor DLL / receipt **00:38:40**，最新UBT记录Compile / Link / Metadata四步成功，总6.97秒。该时间是引擎构建记录，不换算成用户报告的等待工时。
- Game exe仍2026-10-13 **23:47:28**、receipt23:47:29、Game对象文件也是此前版本，确认尚未编译当前短函数。这项正常Game Build合并[C1](../M1_07C1_EditorValidation.md)验证编辑器模块隔离时执行，不能标成已成功或由新Editor DLL替代。
- 最新留存PIE Log于 **00:37:33**关闭，早于最终Source / Editor构建；其中00:26仍是长诊断版。**最终短函数运行正常依据用户反馈**，直接日志证明的是前一轮站定 / 两次真实伤害；不把旧日志当新版本运行证据，也不要求重复已通过的玩法矩阵。
- 两个旧include World / CharacterMovement仍在，显式Pawn.h尚未加；现有构建正常，这是不阻塞的头文件清理，合并C1手动步骤，不声称已完成。

B采样 / 分析、绑定补修与Editor最终收尾接受；后续待办明确转交C1 Game构建和C3 Attempt02 / 包内真实接近攻击短检查。旧Attempt01未修改且包含原Radius150，不作为新Source验证。最后替换的手动时长 / 完成时分 / 单列等待未报告，累计已报告1941 / 已明确165保持。原最终函数 / UBT / 结构化核查在`Raw/M1_07B_Closure_2026-10-14`；助手只读核查 / 写文档，没有代改Source / 资产、Build / PIE或Git操作。

## 8. 实际时间与提交

B原55 + Combat02补5 + 绑定补修10 + 到达快照5 = **B75分钟**；累计**1941分钟（32小时21分钟）**。新增等待未单列，已明确累计165分钟不变，不记成零。M1-07 A+B手动105 / 已明确等待25；最终精简版已由用户实施但未报告新耗时，不推算其实耗。

提交最终指南 / 报告与状态摘要；将最新已落地Task.cpp重新Stage，不把旧长诊断暂存版本当最终函数。BT内容哈希不变，未发生差异不人为加入。原素材 / Raw / CSV / Trace / Runtime与包仍本地；Commit / Push由用户操作。B精简版Editor通过 / 功能正常反馈已登记；Game构建转C1，旧Attempt01仍不是更新后的工程包。
