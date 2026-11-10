# M4-01C1 远程发射入口验收

2026-11-10通过。提交`66d5082`包含三份新增Core文件和角色头中的一个测试friend。四份源码与隔离验证候选一致。

| 项目 | 结果与证据 |
| --- | --- |
| 双Build、索引 | Editor DLL与回执于14:07更新，Game Development exe与回执于14:09更新。Ninja模型含发射和专项两个cpp |
| 正式自动化 | `RangedShot`、`ProjectileDamage`、`ProjectileFlight`三项Success |
| 正式发射专项 | 14:29:48记录600 cm发射和实际12点命中。目标生命Current与Base均从100降到88。拒绝、遮挡及无残留投射物断言随专项通过 |
| 正式地图 | 日志确认加载`L_ET_VerticalSlice`。Entry出生、移动转向、P暂停恢复和停止PIE按用户实机反馈登记 |
| 工程规模 | 243份Source、140个cpp（Core95／UI42／Editor3）、46项自动化 |

本地证据位于`Artifacts/M4_01C1/Acceptance`。`Summary.json`记录源码指纹与构建产物时间，`Excerpt.log`保留关键行。UE原始日志为UTC，本文使用北京时间。

本批手动**40分钟**，构建等待**5分钟**。M4累计手动110分钟，等待15分钟。项目累计6399分钟（106小时39分），等待550分钟（9小时10分）。[M4-01C2远程GAS施放指南](../M4_01C2_RangedShotAbility.md)
