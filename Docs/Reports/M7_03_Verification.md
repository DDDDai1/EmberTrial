# M7-03 角色材质区分验收

**通过。** 用户于 2026-11-16 21:52 完成验收，手动配置 25 分钟，实机检查 10 分钟，共 **35 分钟**。[操作指南](../M7_03_CharacterPalettes.md)

| 检查 | 结果 |
| --- | --- |
| 材质与蓝图 | 八份 MIC 已保存。只读重开四份蓝图，分别核对 `Quinn_01` 与 `Quinn_02` 引用正确的 `MI_ET_*`。对照本批修改前数据，四个角色的 Skeletal Mesh、Skeleton 和 Anim Class 分别仍为 `SKM_Quinn_Simple`、`SK_Mannequin` 与 `ABP_Quinn`。 |
| 玩家与近战 | 用户确认外观辨识、短刃随动、闪避与重开正常。正式地图实战日志记录玩家对近战守卫造成 25 点轻击伤害。 |
| 混合遭遇 | 实机截图中可辨玩家冷蓝灰、近战铜色、远程灰绿，以及近战头顶的锁定菱形和远程弹丸。21:50:12 记录正式锁定，21:50:13、21:50:17 和 21:50:19 记录远程实际发射。 |
| 守关者 | 用户确认深酒红外观。21:48:54 日志记录守关者在 **55 / 180** 生命时进入二阶段，随后执行冲击与横扫。 |
| 自动化 | `EmberTrial.M2.Combo.Flow` 在 21:49:27 返回 **Success**，`EmberTrial.M4.Guardian.Phase` 在 21:49:42 返回 **Success**。 |

本地证据保存在 `Artifacts/M7_03`。第三人称实机截图为 `CharacterPalettesPic.png`，四份蓝图的实读结果为 `CharacterVisualsAfter.json`，筛选后的实机与自动化日志为 `M7_03_Acceptance.log`。

八份材质实例和四份角色蓝图均已随 `82a58cd` 提交，十二份本机二进制文件的 SHA-256 与 Git LFS 指针一致。核对时本地 `main` 和本地记录的 `origin/main` 均指向该提交。

## 本次文档 Commit / Push

暂存本报告、`Docs/M7_03_CharacterPalettes.md`、`Docs/AssetManifest.csv`、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。资产清单与进度文档也记录了 M7-04 的启动，可随下一批文档一起暂存。

- 英文 `docs(art): record character palette and combat verification`
- 中文 `docs(art): 记录角色材质区分与战斗复验`
