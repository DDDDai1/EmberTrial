# M6-05 Continue 异步加载归因验收

**分析验收通过。** 复用 M6-04 两组共六轮，外加 M6-03 禁用音频的三轮作为参照。六轮都能在 GameThread 等待区间内看到 `FAsyncLoadingThread` 处理关卡及依赖包，`SerializeExport` 是其中耗时最多的可见阶段。M6-02 改用静态天空光捕获后引入的 `DaylightAmbientCubemap`，是六轮中可归包耗时最高的单项。[M6-02 验收](M6_02_Verification.md) · [M6-04 验收](M6_04_Verification.md)

## 六轮与禁音长尾

按 Region、日志和 GameThread `LoadMap` 对齐。取每个 `MainThreadWaitingOnAsyncLoadingThread` 的真实区间，计算与加载、文件事件相交的时长。可见包阶段按相交区间并集统计。天空光异步列包含在可见包阶段内，GameThread 的 PostLoad 以整个 `LoadMap` 为观察范围。文件列为可见读取与等待的交集，不同线程的区间可重叠，耗时不能直接相加。

| 组 / 轮 | 主线程等待 ms | 可见包阶段 ms | 其中序列化 ms | 其中天空光包 ms | 天空光 GameThread PostLoad ms | 可见容器读取 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 1024 / 01 | 12.66 | 8.77 | 5.44 | 1.97 | 3.43 | 1.48 |
| 1024 / 02 | 18.52 | 11.06 | 8.04 | 5.00 | 5.80 | 1.12 |
| 1024 / 03 | 17.21 | 10.90 | 7.62 | 4.63 | 7.45 | 1.42 |
| 512 / 01 | 15.75 | 11.20 | 7.94 | 3.72 | 4.66 | 1.58 |
| 512 / 02 | 17.38 | 11.09 | 7.78 | 4.87 | 5.26 | 1.38 |
| 512 / 03 | 18.08 | 10.39 | 7.13 | 4.17 | 5.03 | 1.56 |
| 禁音 / 02 | 30.75 | 18.26 | 13.14 | 9.38 | 9.97 | 未采集 |

- **六轮可见包处理。** 异步线程的 `SerializeExport` 覆盖 5.44–8.04 ms，`CreateExport` 覆盖 2.34–2.80 ms。除天空光包外，还读到 `L_ET_TrialRoute` 本体、设置与失败界面资产、练习目标和 AI Controller 等包处理。主线程等待中仍有 3.89–7.69 ms 无法用已解码的包阶段解释。
- **容器活动。** 六轮每轮都有 29 次与等待相交的可见 `EmberTrial-Windows.ucas` 读取，对应完成请求共 7.25 MiB，交集并集为 1.12–1.58 ms。通过 Trace 中的文件句柄与读取句柄配对，可定位到容器。现有证据无法把这些读取逐笔对到包，也无法推断实际磁盘读取耗时、I/O 排队或缓存命中。
- **禁音长尾。** 第 2 轮 `LoadMap` 65.58 ms、切图帧 72.42 ms、异步 Flush 42.28 ms。该轮加载线程可见包处理增至 18.26 ms，天空光包在等待期间约 9.38 ms，在 GameThread PostLoad 另占约 9.97 ms。禁音组另外两轮的可见包阶段约 10.36 和 10.71 ms。第二轮仍有约 12.49 ms 主线程等待未归因，且旧 Trace 未启用 File。

## 后续处理

M6-06 单独验证 512 帧进入配置后的表现，沿用已实测消除六面捕获的天空光设置。若 M7 更换环境光资产，优先将 `DaylightAmbientCubemap` 的包处理作为复测项，同时比较 A 点画面、切图帧与后一帧。[M6-06 操作指南](../M6_06_AudioBufferPackageVerification.md)

本次直接解码原始 Trace，无需新增采集。分析文件在 `Artifacts/M6_05/Analysis`，其中 `TravelComparison.csv` 为 CPU 与帧时长，`LoadingAttribution.csv` 为等待与包、文件活动交集。逐 Trace 的 `LoadingEvents.json` 保留已解码事件，覆盖 GameThread、加载线程、I/O 线程和相关 Worker。包归因使用该轮 Trace 的包／对象地址及事件时间，防止跨轮复用地址串接。
