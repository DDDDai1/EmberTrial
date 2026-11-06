# M3-06B1：第三件代价型遗物验收

2026-11-07 **00:13**完成，UE **5.5.4**。**验收通过，无待补项。** 逆焰刻印复用既有生命 / 攻击修饰符，在真实装备、伤害与存活换装中生效；下一步是[M3-06B2-01治疗加成属性与Q施放快照](../M3_06B2_01_HealingBonusFoundation.md)，尚未实施。

## 1. 工程与来源核对

- 用户四份测试与下发候选按空白归一后完全一致；DefaultGame.ini唯一三条StartingRelics为life_ember / keen_edge / blood_pact，没有重复。
- 新实物为EmberTrial/Content/EmberTrial/Items/Relics/DA_ET_Relic_BloodPact.uasset，原生ETRelicDefinition、blood_pact、两槽 / Sort30，MaxHealth -20与LightAttackBonus +10。SHA256为`8EA9751992DFDE1BE3DFC42A5CBAD3EEC99A9AAB51BD8A4710CD658F49B2093A`，与本地HEAD bcdfe6a的LFS OID一致。
- 实际规模**233 Source / 133cpp（Core89 / UI41 / Editor3）、42自动化、310 Content / 来源93**。原92行来源指纹全部不变，只新增逆焰一行；不登记下一章尚未创建的资产。
- 本章用户已正常Editor Build、资产Valid / 保存重开并完成配置；助手本轮只读核对生产文件、归档证据和更新文档，没有修改正式Source / Config / Content或操作Git。

## 2. 专项与实际故障

四个既有专项：Inventory.Definitions、Inventory.Session、UI.Relics.ViewModels、UI.Relics.Presenter，**00:06:49完成，4 Success / 0 Fail / 0 Warning / 0 NotRun，退出码0**。证据位于Artifacts/M3_06B1/IdRepair/Automation.log及AutomationReport/index.json。

最初Definitions的“Blood Pact saved definition registered”为空：资产ID误写为blood_pack，配置查找blood_pact，因此不是类型校验器或C++缺少引用。助手此前获授权后在UE中改正并正常Save，清除重复配置，关闭UE后全新进程复跑四项成功；保留原失败日志 / 资产 / 配置备份。重复启动项不是空指针根因。本轮直接复用已成功专项，不要求用户再跑全42项或打包。

真实目录测试不再假定固定两件；两项排序 / 搜索纯值样本仍保留确定性。没有删除生命周期断言或把预期数量2机械改成3。

## 3. 实机与日志证据

用户确认第6节三组通过；以下时间为Asia/Shanghai，原Editor.log已归档至Artifacts/M3_06B1/Acceptance。

| 样本 | 依据与结果 |
| --- | --- |
| 三行 / 默认空槽 | 00:10:33.081日志Owned=3、两槽空；中文 / Sort顺序与预览前100/100按用户反馈通过 |
| 逆焰单件 | 00:10:49.689装备blood_pact，生命80/80；预览 / 真实35与双Modifier按用户反馈、数据核对通过 |
| 逆焰 + 砺锋 | 00:11:08.991真实轻击Requested40 / Actual40，目标100→60；玩家生命仍80 |
| 换为生息 | 00:11:15.407生命80/100，上限恢复不治疗；35伤害与槽位显示按用户反馈通过 |
| 卸装 / 重开 | 装卸过程中出现80/120后80/100，最终两槽空；00:12:16.721下一Play再次Owned3 / 空槽，用户确认新100与攻击25 |
| 输入 / 结束 | 普通P只读、祭坛卸装及最短旧输入按用户反馈通过；本轮PIE无Fatal / Assertion / Ensure / Accessed None |

本轮没有逐键录像或新性能采样，布局与输入观察不冒充日志可证明内容。最新已验证独立包仍是**M3_06A/Attempt01两件版本**，B1第三件目前只有编辑器 / 专项证据。

## 4. 工时与下一步

用户代码35 / 资产15 / 阅读10 / 检查10，共**70分钟（1小时10分）**，构建等待**5分钟**。助手修复和本次归档时间不重复叠加。

M3累计**2213分钟（36小时53分）/ 等待146分钟**；项目累计**5624分钟（93小时44分）/ 明确等待470分钟（7小时50分）**。M2保持1400 / 等待139。

B2-01先在原CombatAttributeSet加入HealingBonus，正式Q使用一次施放快照；B2-02再接第四件遗物、GE与比较界面。体力恢复 / 破势效率与列表压力仍待后续，完整六件目标和M3阶段尚未总关闭。

## 5. Commit / Push（固定放在最后）

当前仅提交本报告、B1指南收尾、来源清单、B2-01指南与实际共享状态，完整12文件见[B2-01末尾](../M3_06B2_01_HealingBonusFoundation.md)。B1实现已在本地bcdfe6a，候选与产物不上传。

- 英文：`docs: verify tradeoff relic and plan healing bonus`
- 专业中文：`docs: 完成代价型遗物验收并规划治疗加成`

Stage后再编辑需重新Stage / Review；Commit / Push由用户操作，助手未核验远端。
