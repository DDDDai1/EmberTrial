# M4-04 完整试炼路线验收

**通过。** 2026-11-13 00:03，用户确认一新三旧源码、Editor / Game 双构建、Core / UI / Editor 三模块索引、地图与菜单保存、配置检查和实机路线通过。手动 90 分钟，构建等待 5 分钟。

| 检查 | 证据（北京时间） |
| --- | --- |
| 自动化与打包配置 | 11 月 12 日 23:56:22，正式工程中的 `EmberTrial.M4.Encounter.CheckpointResume` 与 `EmberTrial.M4.Encounter.Mixed` 均为 Success。正式路线只有一条 `MapsToCook`，本次复跑配置脚本为 PASS，错误和警告均为 0。 |
| 菜单至出口 | 23:56:33 从主菜单以 `NewRun` 确认进入 `L_ET_TrialRoute`。23:56:51、23:57:47、23:58:31 分别生成 1 名、2 名、1 名敌人。前两场分别于 23:56:59、23:58:19 清场，B 点重试后的守关者于 23:59:29 清场。最终出口实际可通行由用户确认。 |
| 检查点与祭坛 | 23:57:06 激活 A，23:57:36 真实装备裂势印成功，23:58:27 激活 B。23:59:07 `Retry` 确认从 `Checkpoint_B` 出生，前两场恢复为已清场，新身体保留槽 A 裂势印。23:59:08 新守关者以 180 / 180 生命生成。 |
| 其余实机路径 | `Retry_A`、菜单 `Continue_B`、后续新游戏回 Entry 由用户确认。保留日志的正式实机段直接覆盖了首次菜单开局与 `Retry_B`。 |
| 资产 | 已保存的 `L_ET_TrialRoute.umap`、`BP_ETMainMenuController.uasset` 的 SHA-256 均与本地提交 `f2b0dd6` 的 Git LFS OID 一致。 |

正式日志在 `EmberTrial/Saved/Logs/EmberTrial.log`。两份资产的指纹、来源与用途已登记。[资产清单](../AssetManifest.csv) · [M4-05 胜利结算](../M4_05_TrialVictory.md)
