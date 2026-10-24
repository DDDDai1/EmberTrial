# M2-06A：目标筛选与锁定生命周期验收

2026-10-24，UE5.5.4。用户12:44反馈全项完成，无失败或特别说明；代码 / 配置80、检查10，手动90分钟，构建等待5分钟。**A验收通过，无待补项。**本轮只读核对并写文档，未代改实际Source / UE资产、执行构建 / 自动化 / PIE / Cook或操作Git。标记与自动跟随镜头仍未实现，不能把目标身份完成写成全部锁定体验完成。

## 1. 实现与构建

三新 / 三旧共六Source，格式化并归一换行后全部与下发候选一致；其余141旧Source / Config保持D基线。实际**147 Source / 80cpp，Core57 / UI20 / Editor3**，20项自动化（Core17 / UI3）。最后源码12:31:34；Core / Editor / UI DLL12:32:34 / 38 / 43，Editor receipt12:32:43、Game exe / receipt12:34:43 / 44覆盖最终Source；用户确认双目标与三模块索引完成，无需重复构建。

Controller拥有唯一原生TargetLock组件及单Started输入。按请求Overlap / 身体去重、Enemy / Ready / 存活 / 中心范围过滤，再按真实视点三维角度与距离评分；先量化再同分按对象路径决胜，Visibility阻挡拒绝首次选择。1200cm获取 / 1500cm脱锁、.10游戏秒维持、.35秒遮挡宽限；已有目标不自动抢换，无目标无Timer。弱来源 / Ready / 死亡封口 / EndPlay / Possess与代次负责清理；不改镜头 / 动作方向或伤害规则。

## 2. 资产与专项

新输入资产实际为**/Game/EmberTrial/Input/Actions/IA_ET_LockTarget**，与原指南建议的LockToggle只是命名差异；原生属性没有硬编码资产路径，用户已在Controller与IMC配置正确引用，真实Tab功能通过。保留实物名，指南和Push清单同步，不为名字重命名 / 创建重复资产。

唯一两个旧资产变化为IMC_Default与BP_ETPlayerController，其余288份旧UE资产保持；来源登记新增IA至**72行**，本地291份UE资产。Boolean / 无Triggers / 唯一Tab映射、原生继承组件 / 六默认值、Compile / Save按用户反馈与运行接受；助手未打开UE逐项遍历Details。本批无需新增Editor规则或WBP。

**12:42:20.733，EmberTrial.M2.TargetLock.Selection：Success。**真实测试World覆盖视角 / 距离、Neutral / 背向 / Waiting过滤、同分、非法迟滞、Toggle / 单Timer、换控、遮挡与Pause、目标死亡 / 销毁、旧代次、脱锁距离、失活、源死亡及EndPlay。夹具不创建真实Viewport或验证键鼠 / UI；原生Controller未配置IA的input unavailable Display出现在测试ETPlayerController_0，属于夹具，不是正式BP缺引用。其余19项本轮沿用，不能写成20项当次全组Success。

## 3. 实机证据与边界

12:43:03.106锁定真实BP_ETMeleeEnemy_C_1；12:43:09.189 ToggleOff / Timer0；12:43:15.073 NoCandidate。12:43:30.273再次锁定，12:43:34.857敌人真实死亡封口、树停止、锁定立即清理；同帧正式左键25令25→0，锁定没有另行修改伤害。

12:43:54.471持有目标，12:43:56.523玩家死亡封口 / Failure与锁定清空；12:43:58.978新一轮可重新锁定，12:44:00.416 PawnChanged清理。12:44:46.043后续新一轮Locked、12:44:47.342主动解除。长按单次、Neutral / 背向、存活受击或破势保留、P鼠标 / 键盘恢复不补Tab、Retry / Stop新身体及五HUD / 四能力短基线按用户反馈接受；未由日志单独区分的画面与键鼠不补造精确事件。

未见本轮Fatal、Blueprint Error、Accessed None或自动化Error，不等同全日志零Warning。当前独立包仍Cleanup/Attempt01，不含后来次数HUD / Poise / F / 冷却HUD / 锁定；无新Cook或Insights。

## 4. 工时与下一批

A手动90 / 等待5；M2-06当前**90 / 5**，M2累计**1160 / 109**，项目累计**3171分钟（52小时51分）/ 明确等待294分钟（4小时54分）**。只计用户报告，不追加助手设计或未单独拆报学习 / 排错。

下一批B1先接锁定标记、玩家界面坐标 / 边界与View生命周期；B2再接相机。B原100–160粗估拆为B1 80–120与B2 80–130，增加60–90使用M2既有缓冲，不叠加全项目预算。A实际低于原粗估，模块当前按已用90 + 剩余220–350 = 310–440分钟，构建等待另记；仅90为实耗。

## 5. 当前 Commit / Push（固定放在最后）

当前实现已由用户提交`5bd5354`；实际Source / UE资产与本地HEAD一致，助手未改索引或联网核验GitHub。本轮Stage A验收报告、来源72、M2-06路线 / B1新指南、A指南和本轮状态文档，不重复Stage无差异Source / 资产。建议Commit（英文）：`docs: verify target locking and prepare the lock marker`。中文译文：`docs: 记录目标锁定验收并准备锁定标记开发`。B1实物完成后按其末尾清单提交六新UI源码、新WBP / 原Controller实际差异及报告；Raw / 日志 / 生成物 / 包体留本地，Git由用户Stage最新内容 / Commit / Push。
