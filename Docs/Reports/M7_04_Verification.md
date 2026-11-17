# M7-04 正式路线材质与光照验收

**通过。** 用户于 2026-11-17 22:41 完成验收，手动配置 **20 分钟**，实机检查 **5 分钟**，共 **25 分钟**。[操作指南](../M7_04_RouteLightingAndSurfaces.md)

| 检查 | 结果 |
| --- | --- |
| 正式图回读 | 独立启动 UE 5.5.4 并只读重开 `L_ET_TrialRoute`。地面、十面墙、三座闸门合计 **14 处**，均在地图实例的 Element 0 应用对应 PBR 实例，碰撞均为 Query and Physics。三门使用各自实例的 ExitGate 材质覆盖。 |
| 灯光 | 方向光强度 **4.5**、启用色温 **5200 K**。天空光强度 **0.90**，指定 `DaylightAmbientCubemap`，Real Time Capture **关闭**。 |
| 同机位对照 | A 点视口可见石板接缝、墙体砌块与祭坛提示。第二场两侧的风化石墙与锈蚀金属闸门易于区分。闸门几何仍为平面灰盒。用户未报告画面问题。 |
| 实机 | 用户确认清场后闸门隐藏且可穿过，祭坛 E 可打开，Stop 后重开正常。日志于 22:36:27 记录首场清场，22:36:31 激活并保存 A（清场计数 1），22:36:32 记录祭坛输入与页面打开。 |
| 自动化 | `EmberTrial.M4.Encounter.Mixed` 于 22:38:26 返回 **Success**。 |

同机位对照使用 UE 编辑器视口，分辨率均为 **1280 × 720**。终版图从已保存地图只读补拍。证据目录为本地 `Artifacts/M7_04`。

| 视角 | 修改前 | 终版 |
| --- | --- | --- |
| 检查点 A 朝祭坛 | `Route_Altar_Before.png` | `Route_Altar_Final.png` |
| 第二场入口朝闸门 | `Route_MixedGate_Before.png` | `Route_MixedGate_Final.png` |

完整的 14 处材质覆盖和灯光参数见 `RouteFinalVerification.json`，实机和自动化摘录见 `M7_04_Acceptance.log`。终版地图与 17 份路线资产已随 `bd42051` 提交，本机实物与该提交的 18 个 Git LFS OID 一致。

## 本次文档 Commit / Push

暂存本报告、`Docs/M7_04_RouteLightingAndSurfaces.md`、`Docs/AssetManifest.csv`、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。终版截图与日志保留在本地证据目录。

- 英文 `docs(art): verify trial route surfaces and lighting`
- 中文 `docs(art): 记录试炼路线材质与光照验收`
