# M7-02 双槽遗物祭坛验收

**通过。** 正式路线中的双槽祭坛已替换圆柱灰盒，保留原有遗物交互。用户于 2026-11-16 21:26 完成验收，手动共 **35 分钟**，其中导入与配置 15 分钟，实机检查 20 分钟。[操作指南](../M7_02_RelicAltarModel.md)

| 检查 | 实际结果 |
| --- | --- |
| 模型与材质 | UE 网格为 `/Game/EmberTrial/Art/Props/RelicAltar/SM_ET_RelicAltar`，实测 **124 × 76 × 102.5 cm**、LOD0 **750 个三角形**、1 个 UV 通道。简单碰撞和凸包均为 0，Nanite 关闭。Basalt、Bronze、Ember 三槽分别引用对应材质。 |
| 蓝图 | 日志确认 `BP_ETRelicAltar` 已保存，用户确认 Compile 通过。只读重开核对 `Marker` 相对位置与旋转均为 0、Scale 为 1，引用新网格。旧 `Label` 已设 Hidden In Game。Marker 为 NoCollision，Overlap 和导航影响关闭。 |
| 正式地图 | 保存的 `L_ET_TrialRoute` 只有一份 `RelicAltar_A`，世界位置 **-970 / -270 / 0 cm**、Actor Yaw **0°**、Marker 相对 Yaw **0°**、Scale **1 / 1 / 1**。交互半径和垂直差均为 **180 cm**，视线检测目标偏移仍为 **(0, 0, 80) cm**。第三人称截图中底座贴地、铜边与正面符号可辨，地图原 `RELIC ALTAR [E]` 指示保留。 |
| 实机 E 与 P | 21:22:19 以 `Mode=Altar` 打开。21:22:21 装备生息余烬至槽 A，21:22:25 装备裂势印至槽 B，日志确认两次成功且库存记录包含两槽。21:22:27 普通 P 进入 `Mode=Pause`，用户确认遗物区只读与范围外 E 拒绝。21:24:57 重开正式图后再次出现祭坛打开日志。 |
| 自动化 | 21:23:23，`EmberTrial.M3.Interaction.RelicAltar` 返回 **Success**。用户未报告画面或交互问题。 |

`L_ET_VerticalSlice` 也随本批保存并提交。只读重开确认该测试图的单个祭坛沿用同一网格与交互配置，Actor 位置为 **-570 / -810 / 0 cm**，Yaw **90°**。正式图和测试图的数值分别记录在本地 `Artifacts/M7_02/UERelicAltarPlacement.json`。

本地证据位于 `Artifacts/M7_02`。实机截图为 `UERelicAltarPic.png`，筛选后的实机和自动化日志为 `M7_02_Acceptance.log`，导入核对数据为 `UERelicAltarAssets.json`。

资产提交为 `b43112b`，包含源件、UE 网格与材质、蓝图和两张地图。九份二进制资产的本机 SHA-256 与 Git LFS 指针一致。核对时本地 `main` 和本地记录的 `origin/main` 均指向该提交。

## 本次文档 Commit / Push

暂存本报告、`Docs/M7_02_RelicAltarModel.md`、`Docs/AssetManifest.csv`、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。

- 英文 `docs(art): record relic altar integration and interaction verification`
- 中文 `docs(art): 记录遗物祭坛接入与交互验收`
