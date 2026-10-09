# M1-05B1：视觉感知与黑板目标验收

复核日期：2026-10-08。**B1验收通过，第7.4节已按用户授权由助手补查闭环，不需要用户重复执行。**用户完成时间字段为“20”，不是完整时分；本地用户PIE样本18:25:32–18:28:54，助手补测18:32:16–18:37:37，分别记录，不推定为20:00或20:20。日志时间为UTC，本文换算为Asia/Shanghai。

## 1. 实现、构建与资产

- 两份ETAIController源码去除空白后与[B1指南](../M1_05B1_PerceptionTarget.md)全文一致。相对A基线仅这两份改变，其余45份Source逐份SHA-256不变；总数仍47份。助手本轮没有修改C++、运行新Build或新增测试图。
- Editor receipt 18:16:18、Game receipt 18:17:11：UE5.5.4 / Win64 / Development，类型分别Editor / Game。CLion compile_commands 18:16:30更新，23个项目cpp且含ETAIController.cpp。双Build / 导航由用户确认通过；receipt与索引已直接读取。
- 新BB_ETMeleeEnemy 18:20:11保存，BP_ETMeleeAIController 18:22:50保存，敌人BP 18:24:02保存。旧36项资产仅敌人BP和地图改变，其余34项哈希不变；登记两项后清单38。黑板Object-Actor / 非同步、Controller父类 / 资源、单继承感知组件 / 无蓝图业务据用户配置反馈，文件哈希本身不证明全部内部设置。
- 开始接管时地图仍有星号 / 1 Unsaved；用户最后磁盘保存为18:27:12。助手两轮PIE后执行Save All，18:37:37正式保存地图，最后界面All Saved / PIE停止。未改变Class Defaults、地图Actor布局或新增资产；PIE临时定位随停止丢弃。最终[47源码 / 38资产基线](Raw/M1_05B1_Baseline.json)与清单同步。

## 2. 用户运行证据与反馈范围

相关[原始日志摘录](Raw/M1_05B1_Acceptance_2026-10-08.log)保留用户五轮及助手两轮PIE、感知状态变化、生命 / UI和保存清理。

- 用户每轮一条Blackboard ready / Target=None，随后本身体Ready100/100；初次出生未立即取得目标。18:25:36首次SightAcquired，18:25:50 SightLost，18:25:52重新取得，Player和BlackboardTarget均为BP_ThirdPersonCharacter。两次变化间没有逐帧重复acquired。
- 第二轮四组获得 / 丢失支持实际感知转换；墙遮挡、绕开重获以及不选训练目标的具体画面 / 原因按用户通过反馈登记。日志只有SightLost，不能仅凭这一个Reason区分距离、角度或墙。18:27:50、18:27:59有导航重建；临时墙已删除、导航恢复据用户确认，助手最终编辑器可见18个场景Actor。
- 18:28:11.149玩家四次25伤害到零，Target cleared / TargetDead在同一死亡调用链发生，HUD为0；直到该轮结束未重新获取尸体。敌人死亡BodyDead本次现存日志没有对应样本，按用户通过反馈与源身体死亡订阅代码审查登记，不写为助手实际攻击验证。
- 用户第一轮带有效目标停止：18:25:59.752 ControllerEndPlay清空后End / Unpossessed，下一轮独立初始化并可重新获得。最后18:28:31–18:28:40有P打开 / Resume requested / Pause closed，但那轮没有acquired，因此单靠该段不足以证明“持有目标时暂停”的条件；助手仅补这个相关生命周期闭环。

## 3. 第7.4节助手补测

使用[computer-use技能](C:/Users/D1334/.codex/plugins/cache/openai-bundled/computer-use/26.1002.52244/skills/computer-use/SKILL.md)，控制现有UE5.5.4编辑器，未接管源码。复用正式地图、身体、Blackboard与现有暂停函数 / 按钮，无新验收按键或变量。

1. 第一轮18:32:16初始化100/100。通过现有UE控制台BugItGo临时定位到可见范围，18:32:52.262取得玩家，黑板实际读取正确。该调试命令会开启Ghost，18:34:00执行Walk恢复碰撞 / 地面运动后才进行有效暂停补查，看到原E提示恢复。
2. 工具模拟P按键未触发Completed；没有把这个结果当作用户真实P键故障或宣称助手直接用P打开成功。P输入沿用用户实机反馈与18:28原日志。助手使用现有`ke BP_ThirdPersonCharacter_C OpenPauseMenu`调用同一个暂停入口：18:35:31.337菜单实际打开，18:35:39.082鼠标点击继续，Resume requested / Pause closed、输入模式恢复。期间没有Target cleared或重复acquired，HUD100/100。
3. 18:36:12.534在目标仍有效时停止，恰好一次Target cleared / ControllerEndPlay / BlackboardTarget=None，随后End、Life stopped / ActorInfo cleared、Unpossessed OldBody=None / Ending1；同轮End后无新的目标或Ready快照。句柄 / 动态事件解绑通过现有代码审查确认，日志没有独立逐句柄计数，不虚构这种监测。
4. 第二轮18:36:24.830新Blackboard从None开始、两身体100/100；18:37:03.126重新取得玩家。再次执行Walk恢复临时模式，18:37:28.774带有效目标停止，清空 / End / Unpossess正常，之后无同轮新acquired。相同对象短名属于新的PIE World，不代表旧对象被复用。
5. 18:37:37 Save All落盘，地图无星号、All Saved，正式场景无临时测试节点 / 挡板。本次没有替用户重跑移动 / 转向的完整矩阵；相关基础输入按用户反馈与既有基线登记。

相关日志未匹配Blueprint Error / Blueprint Runtime Error / Accessed None / Fatal error、Blackboard unavailable或Invalid TargetActor。结束顺序可变，本次出现ControllerEndPlay符合指南允许范围；不要求必须TargetEndPlay或BodyNotReady先发生。

## 4. 工时与后续

用户手动15分钟：代码5 / 资产5 / 检查5，构建等待5。累计手动**1419分钟（23小时39分钟）**，明确报告等待**105分钟**；M1-05 A+B1手动45 / 等待10。助手补测、保存和文档整理另列，不计入用户时间。原预算65–110是估计，不改写成实际发生值，也不据此计算可比较的节省百分比。

B1完成感知 / 黑板与生命周期，当前仍不移动或攻击。下一批M1-05B2接真实Behavior Tree / Move To、忙碌停走和无路径退路，代码继续先写文档、用户手动实施。当前没有新Cook / 阶段包，M0旧包不代表AI阶段。
