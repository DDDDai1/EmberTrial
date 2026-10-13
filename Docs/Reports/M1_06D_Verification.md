# M1-06D：暂停迁移与菜单流程闭环验收

复核：2026-10-13，UE **5.5.4**。**功能验收通过，无待补功能项。**用户13:01报告三组检查、配置恢复与Save All完成。实际手动**105分钟**（代码35 / 资产60 / 检查10），构建等待5分钟。后续直接进入[M1-07阶段包与性能](../M1_07_ReleaseRoadmap.md)，不追加复盘用蓝图或重做旧矩阵。

## 1. 实现与构建证据

- 相对C最终基线，新增ETPauseWidget.h / cpp，修改六份Controller / UI模块 / FailureFlow / FailureWidget源码；其余**61份既有Source哈希不变**，无删除。当前**69Source / 34cpp**。
- D指南八个完整源码块与实际文件一致，比较忽略注释和空白。Core只发P释放意图与提示更新事件，不依赖UI；原FailureFlow统一持有互斥的Pause / Failure，HUD仍归UIBootstrap。
- 最后Source保存为2026-10-12 01:21:01；Editor receipt为01:22:04.976，Game receipt为01:23:14.114，均较晚，目标分别Editor / Game、Win64 / Development / UE5.5.4。用户确认正常双Build通过；本轮未重建。
- CLion compile_commands更新于01:22:15.247，去重34个项目cpp，包含新暂停Widget实现。索引记录不替代Game Build。
- 46项资产中只变动预计的四项：角色BP、游戏Controller BP、IMC_Default、WBP_PauseMenu；其余42项不变。来源清单更新这四项实际哈希，原M1-03B1文档冻结哈希保持。

## 2. 窄排障与保存

8.2角色旧菜单链经当次授权由助手computer-use清理；8.4经当次授权核对父类 / Graph / Designer，修正StatusText误绑定到按钮内、Button Slot与Vertical Box Slot边距混淆和底部提示宽度。暂停Widget12:36:26.498正常保存，Compile绿色成功，当前版本有两个按钮与独立提示、DisableNativeTick / Never。

用户发现左键失效后，助手只读定位到IA_ET_LightAttack后Branch.Condition未连接且默认True；映射仍正确。用户随后补线并保存。13:01:09.879角色正常保存；本轮再次只读打开资产，直接看到Is Game Paused.Return Value已连接该Condition，Started→Branch、True空、False→Request Light Attack.Target=Self，Compile绿色、All Saved。13:01:12出现一次实际正式攻击Begin，停止PIE时Cancelled1 / AttackingTag0。此前检查漏项与修正规则已补入D第8.1节，不把旧断线状态记为正确。

Controller12:47:29.195在None负例后正常保存；原Map恢复、两Class / 单组件 / 提示桥接与最终100/100 / Damage25 / slomo1 / Debug False按用户反馈登记。IMC_Default正常保存于10月12日14:46:40.217；WBP与输入资产哈希已登记。助手本轮没有修改Source / UE资产、触发Build / PIE或Stage / Commit / Push；只读打开角色资产不产生新保存。

## 3. 运行直接证据

[日志摘录](Raw/M1_06D_Acceptance_2026-10-13.log)与[结构化核查](Raw/M1_06D_Evidence_2026-10-13.json)仅留本地。窗口为**2026-10-13 12:42–13:02，北京时间**，原日志UTC加8。

| 观察 | 直接证据 |
| --- | --- |
| Pause打开 / 继续 / 清理 | 15份Pause shown与15份removed闭合，其中11次Pause resumed均Paused0；剩余由返回菜单或停止世界清理 |
| 生命周期 | 12组Game flow ready / end、12组Life bound / unbound、9组Session ready / end闭合，无旧生命周期残留 |
| 动作中暂停 | 12:44:24.138攻击Begin，随后暂停 / 继续，12:44:26.254正常End；12:44:01闪避开始、暂停 / 继续后正常结束，位移门控沿用已验收实现 |
| 暂停→主菜单→继续 | 三MainMenu、两Continue请求 / 到达闭合，Expected1 / Confirmed1 / Pending0；新世界HUD100、Controller / Life重新初始化 |
| None地图预检拒绝 | 12:47:16.649 Travel rejected，同毫秒Panel recovered.Paused1；12:47:23.083继续后Paused0，面板未永久锁死 |
| 真实死亡 / 重试 | 12:47:43.106 Death sealed / Failure queued，43.108只一份Failure shown；12:47:56.573 Retry，56.617新世界到达Confirmed1，新身体Ready100 |
| Pause中停止 / 重开 | 12:48:02 Pause shown，12:48:06移除 / Flow end / Session end；12:48:08新Flow / Ready100，无旧暂停页 |
| 真实攻击与清标签 | 五组玩家正式攻击Begin / End；一次目标真实25伤害；结束或取消AttackingTag0，包括补线后的最后一轮 |

本轮两次Continue直接日志使用**Entry**，没有新的Checkpoint_A激活日志。Checkpoint_A的ID / 出生 / Continue规则与资源相对C不变，沿用[C已通过证据](M1_06C_Verification.md)及用户本轮9.2反馈；不将D的Entry日志写成新A点实测。菜单返回后世界交互 / 敌人重置是整图重建契约，保持原世界的“继续游戏”与重载世界的“继续本次试炼”已在指南明确区分。

鼠标 / Enter / Space / P / 背景焦点、无补播不卡键、具体中文显示、E提示真实状态、失败页P拒绝与最后短基线采用用户实机完成反馈。日志没有编码每次具体物理按键，不以攻击日志单独证明输入键身份；输入补线另有上述只读图证据。

窗口内未检出Blueprint Runtime Error / Accessed None / LogScript Error / LogBlueprint Error / Fatal / Assertion / Ensure；无指定Flow配置 / BindWidget / Native Tick警告。None预检的单条Travel rejected是预期负例。未重新测试真正引擎TravelFailure或“暂停中外部死亡”时序；静态直接接管保护保留，不虚构已实测。

## 4. 结论、边界与工时

M1-06A / B / C / D功能闭环通过，见[总验收](M1_06_Verification.md)。阶段包、正式性能采样与项目校验工具由M1-07实施；历史M0包不代表新菜单 / 战斗流程的包内结果。

M1-06合计手动315分钟（5小时15分钟）、等待20分钟；项目累计手动**1836分钟（30小时36分钟）**、等待**140分钟**。助手GUI与复核不计用户手动时间。本批检查10分钟，沿用方案B；不建立大型测试台，不提前宣称性能优化或最终投递作品完成。

## 5. Commit / Push交接

按[D最后清单](../M1_06D_PauseFlow.md#12-本批需要commit--push的内容固定放在文档最后)提交两新 / 六旧Source、四旧资产与实际文档差异；本报告、总验收、AssetManifest / Progress / Backlog / Architecture / Introduction等更新一并加入。Raw备份 / 日志 / JSON、构建receipt / 索引、包 / Trace均留本地。Git索引与Push由用户执行，本轮未联网核验远端。
