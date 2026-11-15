# M6-05 Continue 异步加载归因

2026-11-15 已复用两组 Trace 完成加载归因。[验收报告](Reports/M6_05_Verification.md)

与 M6-04 一起采集。用同一个包、同一份 A 点存档与两个已计划的音频缓冲组，核对 Continue 时 GameThread 等待的异步加载工作。M6-03 在禁用音频的一轮仍测到 72.42 ms 的切图帧，其中 `FlushAsyncLoading` 为 42.28 ms。当前证据能定位等待区间，具体是文件读取、包处理还是调度等待，留给本章核对。[M6-03 验收](Reports/M6_03_Verification.md)

## 1. 和 M6-04 一起做

1. 用当前版 `Tools/Run-StageBuild.ps1` 按 [M6-04 第 1–3 节](M6_04_AudioCallbackBufferComparison.md#1-准备与启动) 完成 Buf1024 和 Buf512。每组仍是三轮 Continue、一次 `Trace.Stop` 和随后两路试听。沿用 `M6_04_Continue01` 至 `M6_04_Continue03`，无需另外打标。
2. 两组查询 `Trace.Status` 时，留意 **Enabled channels** 是否包含 **LoadTime** 与 **File**。当前脚本会为两组同时开启它们。看到缺项或没来得及看时，记下情况并照常完成，本地日志也会保留查询结果。
3. 保留 `Artifacts/M6_04/Runtime` 下两组原始 `.utrace`、同名前缀的 `.log` 与 `.json`。文件保留在本机即可，由我读取并分析。无需额外跑游戏、打开 Insights、导出 CSV 或重新打包。

如果已用前一版脚本完成某一组，保留它并在回传中注明 `File` 是否启用。两版都提供 `LoadTime`，仍可先对照 CPU 与包加载，文件活动证据按可用范围记录。两组尽量使用同一版脚本，以保持 Trace 采集开销一致。

## 2. 归因和验收口径

- 按日志与 Trace 对齐每轮从主菜单进入 `L_ET_TrialRoute` 的 `UEngine::LoadMap`。初次从磁盘继续用于预热，试听后的继续在 Trace 停止后进行。
- 从 GameThread 量取 `FlushAsyncLoading`、`MainThreadWaitingOnAsyncLoadingThread`，再核对同窗的异步加载线程、包请求与对象创建、序列化、PostLoad 范围。每组分别比较常见耗时和最慢一轮，并与 M6-03 的异常样本对照。
- 用 `File` 时间线核对是否有与等待重叠的可见读取。IoStore 容器读可能只能定位到容器或部分请求。只有对上请求与时间线的部分才按包归因，证据不足时保留线程级结论。
- 输出六轮切图耗时表，记录可确认的长尾来源和仍待验证的阶段。M6-05 的归因可以独立验收，项目配置变更另以 M6-04 的性能及听感结果决定。

## 3. 一次回传两章

先填 [M6-04 回传模板](M6_04_AudioCallbackBufferComparison.md#4-验收与回传)，再附下面两行。两组文件都在默认目录时可写“沿用”。

```text
M6-05 与 M6-04 同批采集
两组 Trace.Status 的 LoadTime / File（没看可填由日志核对）：
Trace、日志和 JSON 位置（沿用可填沿用）：
```

## 4. Commit / Push

两章采集准备可合为一次提交。暂存 M6-04 与 M6-05 操作指南、`Tools/Run-StageBuild.ps1`、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。

- 英文 `chore(perf): capture audio buffers and async loading in the same trial traces`
- 中文 `chore(perf): 合并试炼切图的音频缓冲与异步加载采样`
