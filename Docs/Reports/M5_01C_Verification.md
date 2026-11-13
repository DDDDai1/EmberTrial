# M5-01C 检查点自动存档验收

**通过。** 2026-11-13 21:08（北京时间），手动 30 分钟，构建等待 10 分钟。用户反馈无失败或不理解处。

| 验收项 | 结果与证据 |
| --- | --- |
| 七份源码、双构建、三索引 | 七份源码及正式地图纳入本地提交 `3d29958`。Editor、Game target receipt 分别在 21:01:33、21:02:16 更新。CMake 模型于 21:01:33 更新，三索引由用户确认。 |
| 三遭遇 ID、A／B 自动存档配置 | 用户确认配置完成。正式地图已保存，SHA-256 与提交中的 Git LFS OID 一致。两个启用点的实际写入见下。 |
| 四项自动化 | `RoundTrip`、`Slots`、`Validation`、`CheckpointResume` 均在正式日志报 Success。 |
| Entry、100/100、A 重入 | 21:05:23 从 Entry 出生，生命 100/100。21:05:49 只写入一次 A，序号 1、清场 1、空槽、治疗 2。重入操作由用户确认。 |
| B 与两槽装备 | 21:06:28 写入 B，序号 2、清场 2，槽 A 生息余烬、槽 B 裂势印，剩余治疗 2。两条装备提交均成功。 |
| PIE 双槽与 Stop | 本地 `Saved/SaveGames` 保留 `EmberTrial_Run_PIE_A.sav`（2451 字节）与 `EmberTrial_Run_PIE_B.sav`（2583 字节）。21:06:43 日志记录当前游戏流程与会话结束。 |

[M5-01C 操作指南](../M5_01C_CheckpointAutosave.md) · [M5-02A 主菜单读档](../M5_02A_ContinueFromDisk.md)
