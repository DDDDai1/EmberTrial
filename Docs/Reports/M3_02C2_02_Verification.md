# M3-02C2-02：真实配装事务与新身体恢复验收

2026-10-29。用户**14:19**报告完成、无失败 / 不理解处；代码60 / 阅读配置10 / 检查10 / 构建等待5分钟。**本批验收通过，M3-02最小装备闭环收尾，进入[M3-03A列表数据模型与只读预览](../M3_03A_RelicViewModels.md)。**本轮只读核对，不重复已完成构建 / 自动化或额外要求旧流程复跑。

## 1. 源码、构建与资源

一新 / 九旧Source符合下发候选；四处差异均为空行或表达式换行，无逻辑变化。当前**176 Source / 98cpp（Core72 / UI23 / Editor3）、29自动化、296 Content / 来源77**；全部77条资产SHA与来源记录一致。本地HEAD `a3ae8ca`已含实现，起点工作区干净，未联网核验远端。

Editor / Game Development receipt为14:10:30 / 14:12:49；两目标Build与三索引由用户确认，随后四项真实Success。资产Save All按用户确认，无新资产或必要来源哈希变动；最新包仍M2_07A/Attempt01，不含M3，本轮无Cook / 性能采集。

## 2. 当次日志与证据边界

主日志`EmberTrial/Saved/Logs/EmberTrial.log`，以下为北京时间：

| 时间 | 直接实证 |
| --- | --- |
| 14:14:25.862 | Inventory.EquipmentTransactions Success |
| 14:14:43.908 | Inventory.EquipmentOwnership Success |
| 14:14:57.674 | Inventory.Session Success |
| 14:15:12.668 | Combo.Flow Success |
| 14:15:30.064 | 正式玩家空槽，Ready1 / Revision1 / 100/100 / Bonus0 |
| 14:15:50.062 / 14:15:59.129 | 生命与攻击遗物依次成功，最终Revision3 / 100/120 / Bonus5 |
| 14:16:08.798 / 14:16:13.399 | 重复遗物 / 未拥有ID均RuleRejected，Revision3与身体值不变 |
| 14:16:34.524 / 14:16:40.842 | 卸生命100/100→装回100/120，未免费治疗 |
| 14:16:49.844 / 14:16:59.076 | 正式Pause shown / resumed |
| 14:17:15.311 | 正式玩家攻击激活捕获Damage30；该次Trace Unique0，不能当真实命中证明 |
| 14:17:17.173 | Stop后玩家自有GE、库存 / 会话精确结束 |

四项BeginEvents / EndEvents区间无Warning / Error。Transactions使用真实GI / ASC，覆盖提交点与属性 / 领域事件顺序、重入Busy、旧版本 / 非法请求拒绝、活体夹限、重激活不治疗、Pending / 失败恢复、旧身体隔离与新身体恢复。其确认意图是模型模拟，**不是实机OpenLevel日志**。Combo.Flow有真实3×30 GE伤害与下一链恢复25的专项证据。

用户确认的真实左键命中30、P内拒绝 / 无补执行、Retry / Continue新120与资源默认、NewRun及Stop新GI空槽100 / 原25和短基线，按实机反馈通过。当前主日志只覆盖一轮短PIE，不额外包含这些切图样本；不把用户反馈写成独立日志实证。14:17:03的卸装发生在Pause resumed之后，是新命令成功，与暂停期间写入拒绝不矛盾。局部未见Accessed None / Ensure；引擎 / EOS环境Warning不算本模块失败，也不称整份日志零Warning。

## 3. 完成边界与下一步

会话拥有 / 配装ID、不可变版本方案、当前身体资格、所有拒绝发生在副作用前的提交点、同句柄属性写入、EquipmentReady、Pending后恢复和仅Editor PIE写入的开发入口已连通。活体上限提升不治疗、下降只夹限，新身体首次恢复按最终上限回满；原轻击连段 / 闪避取消规则保持。同步回调的后续世界变化仍不被包装为通用GAS原子事务或失败退款。

M3-02阶段已完成静态方案、真实GE、逻辑库存、身体所有权与事务恢复；正式UI / 祭坛权限、磁盘存档和完整6件遗物仍后续。下一批先做列表Item / 投影与只读比较，之后连接Presenter / UMG条目，不提前宣称已完成构筑界面或500条虚拟化性能验证。

本批人工80 / 等待5分钟；**M3累计310 / 35分钟，项目3721分钟（62小时01分）/ 明确等待359分钟（5小时59分）**。M3-02自身268 / 30，M3-01为42 / 5；M2仍1400 / 139，06C / 07C阅读未登记。助手设计 / 审查和未来估计不计用户实耗；首次PIE等待需求保持关闭。

## 4. Commit / Push（固定放在最后）

C2-02实现已本地提交a3ae8ca。本轮13份文档及未来三新UI / 三旧Core Source精确清单见[M3-03A末尾](../M3_03A_RelicViewModels.md#10-本批-commit--push固定放在最后)。运行Source / Config / Content / 清单 / Tools / Git索引保持，未重跑Build / 测试。Stage / Commit / Push由用户操作，Raw / 日志 / 包留本地。

当前建议英文`docs: verify relic transactions and prepare inventory view models`，中文译文`docs: 记录遗物换装事务验收并准备库存展示模型`。实现后英文`feat: add relic list models and side effect free equipment previews`，中文译文`feat: 新增遗物列表数据模型与无副作用装备预览`。
