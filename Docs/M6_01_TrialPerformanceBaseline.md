# M6-01 正式试炼性能基线

使用 M5-04 已验收的 Win64 Development 包，采一轮待机、真实交战和存档加载。你操作游戏并标记区间，我读取 Trace 和运行日志，统计帧时间、线程与 GPU 耗时，并定位主要尖峰。预计手动 20–30 分钟。

## 1. 启动固定版本

关闭 UE 编辑器，暂停构建、录屏和索引。设备接电，记录当前电源档位和实际窗口是否缩小或裁剪。保持日常缓存。

```powershell
Set-Location 'E:/UE_Project/Demo'
$package = 'E:/UE_Project/Demo/Artifacts/Builds/M5_04/Attempt01'
& './Tools/Run-StageBuild.ps1' -BuildDirectory $package -EvidenceStage M6_01 -RunName Trace01 -Width 1920 -Height 1080 -Trace -PerformanceBaseline
```

脚本在 `Artifacts/M6_01/Runtime` 生成同前缀的日志、JSON 和 Trace 文件。每次采集使用独立的用户目录，重采可用 `Trace02`。JSON 同时记录外层启动器和实际游戏二进制的 SHA-256。

## 2. 核对运行条件

进入主菜单，开始新试炼。从 Entry 出生后按反引号键打开**游戏控制台**，逐条输入下列命令并回车。请把控制台中的结果留在运行日志里。

```text
t.MaxFPS
r.VSync
r.ScreenPercentage
r.DynamicRes.OperationMode
sg.ShadowQuality
sg.GlobalIlluminationQuality
Trace.Status
```

前六项预期依次为 **0、0、100、0、1、1**。启动参数请求 1920×1080、中等画质、动态分辨率关闭。实际值若有差异，回传差异。我会按日志和截图记录实际口径。

关闭控制台，在 Entry 站定预热约 60 秒。可在预热后输入以下命令保存一张引擎截图，用于核对实际尺寸。等约 5 秒后再开始标记待机。

```text
HighResShot 1 filename=E:/UE_Project/Demo/Artifacts/M6_01/Viewport01.png
```

## 3. 标记采样区间

| 区间 | 操作 |
| --- | --- |
| `M6_01_Idle01` | 游戏控制台执行 `Trace.RegionBegin M6_01_Idle01`，关闭控制台，站在 Entry 不动约 20 秒，再执行 `Trace.RegionEnd M6_01_Idle01`。 |
| `M6_01_Combat01` | 先执行 `Trace.RegionBegin M6_01_Combat01`。关闭控制台，进入第一场，等近战敌人实际追击、挥击或扣血，交手约 15–20 秒。可做一次攻击、闪避或破势。然后执行 `Trace.RegionEnd M6_01_Combat01`。实际交战起点由 AI 和命中日志定位。 |
| `M6_01_Continue01` | 打完第一场并激活 A，等 `Checkpoint saved`。在可操作时执行 `Trace.RegionBegin M6_01_Continue01`，P 返回主菜单，点击继续。确认 A 出生且可操作，再执行 `Trace.RegionEnd M6_01_Continue01`。该段是同进程地图切换样本，助手会从日志另外量取发起 Continue 到确认到达的时间。 |

每组 Begin 和 End 使用同一名称。若交战提前结束，仍关闭对应区间并说明发生了什么。结束后在可操作状态执行 `Trace.Stop`，从游戏菜单正常退出。

## 4. 核对文件与回传

打开 UE 5.5.4 的 Unreal Insights，选择 **Open Trace File**，打开脚本显示的 `.utrace`。在 **Timing Insights** 确认 Frames 与 CPU 轨道有数据。GPU 轨道或截图若缺失，记录实际情况。帧时间统计、区间首尾取舍和尖峰归因由助手完成，用户无需导出整份 CSV。

```text
M6-01 采集完成
实际Attempt / Trace、Runtime与JSON路径：
接电、画质、实际窗口与截图（不清楚可填不清楚）：
Idle01 与 Combat01 / 敌人实际追击、挥击或扣血：
Continue01 / A保存、菜单继续与A出生：
Trace.Stop / 正常退出 / Insights可见的轨道：
失败或明显问题（无填无）：
完成时分 / 手动分钟：
```

## 5. Commit / Push

暂存 M5-04 验收报告、本指南、M5-04 指南、README、Progress、Backlog、Architecture 和 `Tools/Run-StageBuild.ps1`。Trace、运行日志、截图、测试存档与包留在本地 Artifacts。

- 英文 `chore(perf): verify persistence package and prepare trial profiling`
- 中文 `chore(perf): 验收持久化集成包并准备试炼性能采集`
