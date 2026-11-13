# M4-05 试炼出口与胜利结算验收

**通过。** 回传标题写为 M4-04，检查项与 M4-05 指南一致，本次按 M4-05 归档。完成于 2026-11-13 01:05（北京时间），手动 50 分钟，构建等待 5 分钟。

| 检查 | 结果与证据 |
| --- | --- |
| Editor / Game 双构建、CLion 三模块索引 | 用户确认通过。两个正式 Win64 Development target receipt 分别更新于 00:24:23、00:26:17。 |
| 出口、WBP、Controller | 用户确认出口绑定三个遭遇。正式地图、胜利 WBP、Controller 分别在 00:29:28、00:31:44、00:32:12 保存；胜利 WBP 有项目 Valid 日志。三份资产的 SHA-256 均与本地提交 `7d0096a` 的 Git LFS OID 一致。 |
| 三项专项自动化 | 正式日志中 `EmberTrial.M4.Trial.Victory`、`EmberTrial.M4.Encounter.CheckpointResume`、`EmberTrial.M4.Encounter.Mixed` 分别在 00:32:31、00:32:45、00:32:58 报 Success。 |
| 打包前全组回归 | 使用正式工程二进制后台复跑，01:08:39 完成，57 项成功，0 失败。配置脚本复查 PASS，0 错误、0 警告。 |
| 原失败页 Retry_B | 本轮由用户实机确认。此次保留的 M4-05 日志没有对应 Retry 请求，上一章保留的正式日志有 Retry_B、前两场恢复、装备保留与新守关者 180 的直接证据。 |
| 清场数与 P 门控 | 两轮均在守关者清场后约 3 秒出现一次胜利页，清场数为 3，日志均为 `Paused=1`。胜利页中的 P 和移动键检查由用户确认。 |
| 键盘「再玩一次」 | 用户确认键盘触发。01:02:45 日志显示同图 `NewRun`、实际出生 Entry、两个装备槽为空。第一场在新世界重新生成。 |
| 鼠标返回主菜单与继续禁用 | 用户确认鼠标触发。01:03:46 发起 `MainMenu`，01:03:47 菜单报告 `Continue=0`，随后新试炼于 01:03:51 从 Entry 开始。 |

正式实机与专项证据来自 `EmberTrial/Saved/Logs/EmberTrial.log`。M4-06 将验证同一流程在 Win64 Development 包中的表现。[M4-06 打包与集成指南](../M4_06_TrialIntegratedPackage.md)
