# M3-06B3-03 第五件遗物与恢复比较验收

2026-11-08验收通过。用户确认Editor Build、Game Build、三模块索引、五件资产配置与Valid、第四比较行和布局均通过。日志中的实机检查截至北京时间13:28:07。

## 版本与资产

本地提交`2a8d1dd`包含五份既有源码、一份启动Config和凝息护符。六份文本与候选按空白归一一致。凝息护符使用`breath_charm`、Sort 50、双槽和每游戏秒+5点体力恢复。实物SHA256与提交中的LFS OID相符，94条原来源记录的资产指纹均通过。新资产已登记。[资产来源清单](../AssetManifest.csv)

当前为233份Source、133个cpp（Core89 / UI41 / Editor3）、42个自动化入口、312份Content和95项资产来源。

## 专项与实机

| 专项 | 北京时间 | 结果 |
| --- | --- | --- |
| Inventory.Definitions | 13:26:20 | Success |
| Inventory.Session | 13:26:34 | Success |
| UI.Relics.ViewModels | 13:26:48 | Success |
| UI.Relics.Presenter | 13:27:02 | Success |

13:27:23，祭坛成功装入凝息护符，玩家有效恢复速率为25。闪避扣至75，恢复满值停表。13:27:49卸下后有效速率为20，第二次闪避也回满停表。13:28:02清理原世界，13:28:05的新玩家初始化为体力100、恢复加成0，并以默认速率20开始观察。日志同步记录列表五项。双Build、索引、Valid与实际布局依据用户实机反馈。

日志节选和核验摘要保存在本地`Artifacts/M3_06B3_03/Acceptance`。

## 工时与后续

手动**40分钟**，构建等待**5分钟**。M3累计2613分钟（43小时33分），等待176分钟。项目累计6024分钟（100小时24分），等待500分钟（8小时20分）。

B4-01接入F的破势加成属性和施放快照。第六件装备与比较随后接入。当前已验收的独立包仍为M3_06A/Attempt01两件版。[B4-01操作指南](../M3_06B4_01_BreakPulsePoiseBonusFoundation.md)

## Commit / Push

功能已在本地提交`2a8d1dd`。收尾与下章指南的文档提交见[B4-01末尾](../M3_06B4_01_BreakPulsePoiseBonusFoundation.md#8-commit--push)。

- 英文 `docs: verify stamina recovery relic and plan break pulse bonus`
- 中文 `docs: 完成体力恢复遗物验收并规划破势加成`
