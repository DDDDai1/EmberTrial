# M4-01D2 远程守卫验收

2026-11-11 22:44 完成。用户确认双 Build、三模块索引、五项专项、实机和保存均通过。实现提交为 `7a6f8bb`。

| 检查 | 结果 |
| --- | --- |
| 资产 | 正式地图使用 `BP_ETRangedGuard_Native`。`RangedGuard_A` 位于 `250 / -650 / 120`、Yaw `180°`，Scale `1 / 1 / 1`。黑板、行为树、Controller、原生身体蓝图及地图的本机 SHA-256 均与提交中的 LFS OID 一致 |
| 正式专项 | 日志记录 `RangedGuard`、`RangedShot`、`RangedShotAbility`、`ProjectileDamage`、`ProjectileFlight` 五项 Success |
| 追击和射击 | 22:42:03 守卫按650 cm半径寻路成功，在638.4 cm结束追击，随后发射，首发命中使玩家生命100→88。后撤样本记录854.9 cm恢复追击并重新停步 |
| 中断和重开 | 日志记录普通受击、守卫死亡停树、重新开局生命100和行为树启动。退让、遮挡、暂停、近战短查与E/P按用户实机反馈登记 |

正式实机在一次近身交战中记录到短时间内反复 `SightLost` 和重新获得目标。日志证实了感知切换，近身退让的画面以本轮实机反馈为准。摘录保存在 `Artifacts/M4_01D2/Acceptance/Excerpt.log`。[D2操作](../M4_01D2_RangedGuardAI.md) · [资产来源](../AssetManifest.csv)

本批手动80分钟，构建等待5分钟。M4累计手动330分钟，等待35分钟。项目累计手动6619分钟（110小时19分），等待570分钟（9小时30分）。[M4-02操作](../M4_02_MixedEncounter.md)
