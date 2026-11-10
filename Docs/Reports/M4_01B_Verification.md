# M4-01B 投射物GAS伤害验收

2026-11-10通过。正式实现已提交至`4097e43`，包含三份新增及四份既有Core文件。

| 项目 | 结果与证据 |
| --- | --- |
| 源码核对 | 七份文件的实现与候选一致。`ETCombatDamage.h`实际在`Public/Combat`，命名空间内缩进调整，函数签名与逻辑一致。指南按实际位置更新 |
| 构建与索引 | Editor DLL与目标回执约13:51更新，Game Development exe与目标回执约13:52更新。Ninja模型含共享伤害和新专项两个cpp，用户确认双Build、索引与保存 |
| 正式自动化 | `ProjectileDamage`、`ProjectileFlight`、`Combo.Flow`、`Dodge.Invulnerability`四项Success |
| 投射物伤害 | 合法命中使生命Current与Base从100降到84。无敌、同阵营、墙和来源销毁均有对应拒绝记录 |
| 正式地图 | `L_ET_VerticalSlice`从Entry启动。13:55:50玩家命中训练目标，生命Current与Base从100降至75，实际伤害25。随后正常停止PIE。P短查按用户实机反馈登记，本轮日志覆盖地图启动、命中及停止 |
| 工程规模 | 240份Source、138个cpp（Core93／UI42／Editor3）、45项自动化 |

本地证据位于`Artifacts/M4_01B/Acceptance`。`Summary.json`记录七份源码指纹、实际头路径及产物时间。`Excerpt.log`保留正式工程关键行。UE原始日志使用UTC，本文时间为北京时间。

本批手动**30分钟**，构建等待**5分钟**。M4累计手动70分钟，等待10分钟。项目累计6359分钟（105小时59分），等待545分钟（9小时5分）。[M4-01C1远程发射指南](../M4_01C1_RangedShot.md)
