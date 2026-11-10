# M4-01A 投射物飞行基础验收

2026-11-10通过。三份Core源码已提交至`c667e1e`。统一换行并忽略末尾空白后，三份内容均与已验证候选一致。

| 项目 | 结果与证据 |
| --- | --- |
| 双Build | Editor DLL及目标回执于13:24:50更新，Game Development exe及目标回执于13:26:35更新。用户确认两次构建成功 |
| CLion索引 | CMake模型与Ninja缓存已刷新，缓存包含投射物与专项两个cpp。用户确认索引完成 |
| 飞行专项 | 正式工程`EmberTrial.M4.Combat.ProjectileFlight`于13:27:50返回Success。扫掠、薄墙阻挡、来源忽略、只广播一次、到期静默销毁通过 |
| 正式地图短查 | 日志确认`L_ET_VerticalSlice`于13:28从Entry出生、生命与体力100、P暂停与恢复及停止PIE。移动转向和左键短查按用户实机反馈登记 |
| 工作区 | 本批三个文件已提交。总计237份Source、136个cpp（Core91／UI42／Editor3）、44个自动化入口 |

本地证据归档于`Artifacts/M4_01A/Acceptance`。`Summary.json`记录源文件SHA-256和构建产物元数据，`Excerpt.log`摘录正式工程关键行。UE原始时间戳为UTC，表内时间为北京时间。

本批手动**40分钟**，构建等待**5分钟**。项目累计6329分钟（105小时29分），构建等待540分钟（9小时）。[M4-01B投射物伤害指南](../M4_01B_ProjectileDamage.md)
