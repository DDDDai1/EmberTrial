# M4-01C3 远程资产验收

2026-11-11通过。`c9bf60a`包含三份新资产和既有能力专项更新，测试源码与隔离验证版一致。

| 项目 | 结果与证据 |
| --- | --- |
| 资源与保存 | 远程Montage、弹体BP和GA的磁盘SHA-256均与本地提交的LFS OID一致。专项从已保存资产读取引用，验证能力实例继承默认值 |
| 双Build与索引 | Editor DLL与回执约20:57更新，Game Development exe与回执约20:58更新。CLion模型包含更新后的能力专项 |
| 正式自动化 | `RangedShot`、`RangedShotAbility`、`ProjectileDamage`、`ProjectileFlight`四项Success |
| 真实发射 | 20:59:30从600 cm发射`BP_ETRangedProjectile`，命中扣12，目标生命Current与Base均为88。发射前的可见无碰撞网格检查随专项通过 |
| 地图短查 | 日志确认`L_ET_VerticalSlice`加载。移动与P短查按用户实机反馈登记 |
| 工程规模 | 250份Source、144个cpp、47项自动化、316份Content、99条资产来源 |

本地证据位于`Artifacts/M4_01C3/Acceptance`。`Summary.json`记录三份指纹和构建产物时间，`Excerpt.log`保留关键行。UE原始日志为UTC，本文使用北京时间。三份资产的复用关系已登记。[资产来源清单](../AssetManifest.csv)

本批手动**50分钟**，构建等待**5分钟**。M4累计手动220分钟，等待25分钟。项目累计6509分钟（108小时29分），等待560分钟（9小时20分）。[M4-01D1远程能力授予指南](../M4_01D1_RangedGuardAbilityGrant.md)
