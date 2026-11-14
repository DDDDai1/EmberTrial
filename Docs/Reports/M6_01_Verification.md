# M6-01 正式试炼性能基线验收

**采集与分析通过。** 用户于 2026-11-13 22:39（北京时间）完成采集，手动 25 分钟，反馈无明显问题。三段均由 Unreal Insights 导出并确认有起止时间。

| 验收项 | 证据 |
| --- | --- |
| 包与采集文件 | `M5_04/Attempt01`，Win64 Development，UE 5.5.4。实际游戏 exe SHA-256 与 M5-04 验收一致。Trace、Runtime.log 和启动 JSON 前缀均为 `Trace01_20261113_222649_403`。Trace 为 1,534,391,500 字节，解析时长约 349.5 秒。 |
| 环境与画质 | Ryzen 7 7840H、RTX 4060 Laptop，驱动 595.71。用户确认接电，具体电源档位及窗口裁剪情况未提供。请求 1920×1080，原始截图为 1920×1080。六项查询依次为 0、0、100、0、1、1。 |
| 真实交战 | 第一场近战实际感知、追击和挥击，玩家生命从 100 降至 50，共五次实际 10 点命中。 |
| A 点保存与同进程继续 | A 写入 v2 序号 1。P 返回菜单后点继续，实际从 A 出生、第一场恢复清场。Continue 发起到确认到达约 57 ms，`LoadMap` 日志为 52.478 ms。 |
| Trace 与退出 | CPU、Frame、GPU、Bookmark、Region、LoadTime、Log 已开启。22:32:39 停止 Trace，22:32:43 游戏请求退出状态 0。导出文件含 GameThread、RenderThread 和 GPU1 事件。 |

## 帧时间与场景 GPU 范围

主循环间隔为相邻 `FEngineLoop::Tick` 的起点之差。统计完整落入 Region 的闭合事件，分位数采用最近秩法。GPU 列采用 GPU1 上深度为零的场景渲染包裹范围，覆盖场景渲染范围，单位均为 ms。完整区间包含操作、切图和控制台边界。

| 区间（Trace 秒） | 样本 | 均值 | P95 | P99 | 最大 | >50 ms | GPU 场景 P95 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Idle01，210.975–253.385 | 10425 | 4.067 | 4.464 | 4.709 | 10.737 | 0 | 3.520 |
| Combat01，273.820–309.635 | 7674 | 4.666 | 5.298 | 5.610 | 36.711 | 0 | 4.435 |
| Continue01，324.514–339.438 | 4007 | 3.722 | 5.177 | 5.605 | 111.944 | 3 | 4.272 |

两段稳定样本的 P95 与 P99 低于项目战斗预算。数据为单轮 Development 基线，包含 Trace、日志和计时开销，最终发布验证需使用正式美术与固定路线的多轮样本。GPU 列用于定位场景渲染时段，呈现端耗时未测。

## 尖峰归因

| Trace 起点 | 主循环间隔 | CPU 事件 |
| --- | ---: | --- |
| 274.673 s，交战 | 36.711 ms | `UWorld_Tick` 约 35.0 ms，其中角色移动约 17.95 ms，PostPhysics 等待约 15.8 ms。单轮尚未定位 PostPhysics 的具体任务。 |
| 328.699 s，返回主菜单 | 70.369 ms | `UEngine::LoadMap` 约 66.0 ms，包含约 23.6 ms 音频线程 Flush、对象回收及组件注销。 |
| 332.059 s，Continue 切图 | 58.088 ms | `UEngine::LoadMap` 约 52.5 ms，包含约 29.6 ms 音频线程 Flush 和约 7.9 ms 异步加载等待范围。 |
| 332.117 s，Continue 后一帧 | 111.944 ms | GameThread 约 110.3 ms 处于 `Frame Sync Time` 等待。同窗口 RenderThread 的六项 `CaptureCommand` 和一项 `UpdateCaptureContents` 对应天空光场景捕获与滤波。候选包待复测。[M6-02](../M6_02_SkyLightCapture.md) |

首次后台解析在结束导出前关闭，报告了待机区间未闭合。重新完整解析后导出了三个有起止时间的区间和对应的 CPU、GPU 事件。此次 Trace 采用已列出的性能通道，内存分配与泄漏未测。

原始采集在 `Artifacts/M6_01/Runtime`，截图为 `Artifacts/M6_01/Viewport01.png`，CPU、GPU 与主循环导出保存在 `Artifacts/M6_01/Analysis`。[采集指南](../M6_01_TrialPerformanceBaseline.md)
