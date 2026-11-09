# M3-06C2 500条遗物 ListView 回收验收

2026-11-09验收通过。用户确认Editor Build、Game Build与三模块索引完成。正式工作区的测试源码与候选按空白归一一致，96项资产实物指纹通过。

## 专项与实机

北京时间22:21:16，`EmberTrial.UI.Relics.ListViewStress`为Success。正式编辑器记录如下。

```text
[M3-06C2] Rows=500 Panels=2 PeakDisplayed=6 MaxUniquePerPanel=12 ReusedAssignments=156 Samples=32 MeanMs=0.674 MaxMs=12.425
```

固定300×360 Slate几何下，500条数据的同时显示峰值为6，每个面板最多12个条目实例，记录到156次重新绑定。两轮通过旧委托解绑、条目文字与槽位徽记、选中高亮清理。均值0.674 ms、最大12.425 ms的口径是列表布局预处理和Tick，保留实际峰值。

22:21:21进入正式地图，日志记录六件与空双槽。22:21:22、22:21:31、22:21:37分别打开新的普通P面板，随后三轮均记录移除和遗物面板解绑。用户确认正式六件、切换及P重开。日志节选保存在`Artifacts/M3_06C2/Acceptance`。

随后以同一正式工程运行C3阶段全组，22:24:10结束，43项均为Success。该次ListView专项再次记录6／12／156，计时为均值0.637 ms、最大15.063 ms。两次最大值分别保留，后续帧数据采用独立采样口径。

## 工时与后续

手动**25分钟**，构建等待**5分钟**。M3累计2848分钟（47小时28分），等待201分钟。项目累计6259分钟（104小时19分），等待525分钟（8小时45分）。

当前234份Source、134个cpp（Core89 / UI42 / Editor3）、43个自动化入口、313份Content、96项资产来源。C3使用新Attempt完成六件版Win64 Development包及两轮独立验证。已验收的独立包为M3_06A/Attempt01两件版。[C3操作指南](../M3_06C3_SixRelicIntegratedPackage.md)

## Commit / Push

当前C2测试在工作区已完成，Git暂存区仍只有3行初始内容。按[C3第7节](../M3_06C3_SixRelicIntegratedPackage.md)重新Stage完整测试，再分组提交工具和文档。

- 英文 `test: verify 500-row relic ListView recycling`
- 中文 `test: 验证500条遗物的ListView条目回收`
