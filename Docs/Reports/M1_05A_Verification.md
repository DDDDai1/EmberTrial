# M1-05A：敌人身体与AI控制权验收

复核日期：2026-10-08；用户完成时间20:46，对应本地2026-10-07日志与保存记录。**A验收通过。**助手读取源码、构建产物、索引和日志并维护文档 / 清单，未修改工程Source或UE资产，未运行新Build / PIE。

## 1. 源码与构建

- Source45→47，新增ETAIController.h / cpp，核心Build.cs仅增加Public AIModule；其余44份既有源码与M1-04D基线逐份SHA-256一致。两份Controller去除空白后与A指南修正版全文一致。
- Character参数 / 两个局部变量均已改BodyCharacter。首次UHT遮蔽错误属于原指南命名遗漏，当前用户修改与成功构建已关闭，不作为待修项。
- Editor receipt：2026-10-07 20:25:53；Game receipt：20:27:58。均为UE5.5.4 / Win64 / Development，目标分别Editor / Game。23cpp索引20:26:19更新且包含ETAIController.cpp；已存在对应编译对象。用户确认双Build / 头文件导航通过，本次未重新编译。
- Controller只观察本身体Ready，立即快照、弱引用、显式解绑及结束保护；未新增业务Tick / Timer / BT / 路径请求。攻击仍LocalOnly / 本地玩家Controller限定，A没有敌人主动攻击。

## 2. 资产与运行证据

新敌人BP于20:38:44正常保存，地图20:42:45正常保存；旧登记35项中仅地图变化，其余34项哈希不变。登记敌人后清单36项，保存[47源码 / 36资产基线](Raw/M1_05A_Baseline.json)。父类、单组件 / Quinn / 输入资源、100/100 / 碰撞 / Auto Possess和最终布局依据用户配置与通过反馈；文件 / SHA不单独证明内部BP设置。

相关[原始日志](Raw/M1_05A_Acceptance_2026-10-07.log)217行，保存了导航构建、资产保存与20:43:17–20:45:59两轮PIE：

- 20:41:49 Build Paths启动 / 完成，Default Recast代理半径35。连续绿色覆盖及无身体烘焙洞据用户观察；导航构建成功不证明已经Move To，也不把Recast代理半径35误写为身体胶囊42。
- 每轮各一条Possessed / Authority1、一条PossessSnapshot Ready0与一条ReadyChanged Ready1 / Snapshot1 / 100/100；各有一个敌人Life observing、Hit Class为GA_ET_HitReact、Granted1。GAS PlayerController字段None符合AI归属，并不表示实际AIController为空。
- 第二轮四次真实Damage Applied1：100→75→50→25→0，Current / Base同步；三次敌人Hit begin / end，HitTag均0。20:45:54.621一次Death committed / sealed，TraceTick0 / MoveMode0 / AnimPaused1；再击TargetDead拒绝。玩家本轮只有100/100 Ready HUD，无敌人属性进入玩家Presenter的日志。
- 两轮各一条Controller End，之后同轮不再有Body snapshot；身体Ready撤销、Life stopped / ActorInfo cleared正常。Unpossessed在End后OldBody=None / Ending1表示已解绑，不是对象丢失。死亡身体最后Dead Tag Remaining0。
- 未匹配相关片段中的Blueprint Error、Blueprint Runtime Error、Accessed None或Fatal error。导航显示、脚部 / 动画、HUD视觉、E/P与移动跳跃按用户通过反馈登记。

两轮名字重复ETAIController_0 / BP_ETMeleeEnemy_C_1是不同PIE World里的新实例，不能当作沿用旧对象。日志直接证明第二轮重新初始化100；死亡后的再开状态由用户反馈补充，不虚构第三轮日志。旧训练目标保留，感知 / 追击 / AI攻击尚未实现。

## 3. 工时与下一批

用户手动30分钟：代码10 / 资产地图15 / 检查5；等待5分钟。累计手动**1404分钟（23小时24分钟）**，明确报告等待**100分钟**。实际低于A原预算40–75分钟，按真实值记录；助手读证据 / 设计文档不算用户工时。

继续[B1感知与有效黑板目标](../M1_05B1_PerceptionTarget.md)，然后B2真实BT追击。沿用这个敌人 / 地图，不新增测试按键或重搭旧Controller矩阵。没有新Cook / 包，M0旧包不代表当前AI阶段。

