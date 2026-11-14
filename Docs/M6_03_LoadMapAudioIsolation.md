# M6-03 定位 Continue 切图的音频开销

已完成两组三轮对照。正常音频与禁用音频的切图帧中位数为 74.45 ms 和 50.37 ms。[验收报告](Reports/M6_03_Verification.md)

M6-02 消除了天空光六面捕获。候选包仍有 80.73–85.61 ms 的切图帧。本章用同一个候选包，在两个独立进程对比正常音频与 `-nosound`。预计手动 10–15 分钟，使用现有 A 点存档，直接从主菜单开始。

## 1. 现有证据

已将 M6-02 的候选补采 Trace 按 `UEngine::LoadMap` 对齐。在 GameThread 时间线取该事件内各计时器的覆盖时长，同一计时器重叠区间取并集，单位 ms。

| M6-02 补采轮次 | LoadMap | 音频 Flush | FlushAsyncLoading | 其中等待异步加载线程 |
| --- | ---: | ---: | ---: | ---: |
| 1 | 77.78 | 33.35 | 25.58 | 18.38 |
| 2 | 73.16 | 28.28 | 25.34 | 17.89 |
| 3 | 77.90 | 33.55 | 25.94 | 18.39 |

UE 5.5.4 的 `UEngine::LoadMap` 在拆除旧世界时调用 `FAudioDevice::Flush`。该函数向音频线程派发清理并等待 fence，Flush 的范围还包括更新音频硬件和提交未完成的音频渲染命令。项目的 `UETAudioSubsystem` 同一时期会释放旧世界的 Sound Mix。当前 Trace 能确认音频路径耗时，但尚不足以把耗时归到 Sound Mix、Mixer 等待或设备回调中的某一项。

`-nosound` 通过启动参数关闭该测试进程的音频路径。它用于隔离音频子系统的整体影响。正式游戏与配置继续使用原音频行为。[上章帧时间与包身份](Reports/M6_02_Verification.md)

已用 `AudioOffProbe` 试跑。首屏读到 A 点存档，Continue 正确恢复第一场清场与角色资源，菜单与试炼均记录 `RoutingApplied=0`，正常退出。正式的三轮对照从第 2 节开始。

## 2. 两个进程使用同一候选包和 A 点存档

关闭 UE 编辑器和其他 EmberTrial 游戏窗口，设备接电。保持同一显示窗口、音频输出设备和电源档位。正常音频组期间保持系统和游戏音量原样，采样中不播放试听探针。打开 PowerShell 执行以下代码，使用现有 M6-02 包。

```powershell
Set-Location 'E:/UE_Project/Demo'
$package = 'E:/UE_Project/Demo/Artifacts/Builds/M6_02/Attempt01'
$seed = 'E:/UE_Project/Demo/Artifacts/M6_02/Runtime/Attempt01/User/After01_20261114_215142_739/Saved/SaveGames/EmberTrial_Run_A.sav'
$launch = @{
    BuildDirectory = $package
    EvidenceStage = 'M6_03'
    Width = 1920
    Height = 1080
    Trace = $true
    PerformanceBaseline = $true
    SeedCheckpointSave = $seed
}
& './Tools/Run-StageBuild.ps1' @launch -RunName AudioOn01
```

脚本将存档复制到当轮独立用户目录，保留源存档，并在 `Artifacts/M6_03/Runtime` 记录同前缀的日志、Trace 和 JSON。JSON 写入游戏 exe、IoStore TOC 和复制后的种子存档哈希。首屏应显示最近存档 `Checkpoint_A`。若提示没有可继续的试炼，保留运行日志和脚本输出，先排查存档路径。

### 2.1 AudioOn01 正常音频

1. 首屏点 **继续本次试炼**。确认 A 点标记、第一道门已开、生命与体力 100/100、治疗 3/3。在 A 点站定约 15 秒，让首次载入稳定。这次从磁盘继续用于预热。
2. 按反引号键打开游戏控制台，分别查询 `sg.GlobalIlluminationQuality`、`r.SkyLight.RealTimeReflectionCapture` 与 `Trace.Status`。前两项预期为 1、0，Trace 应在写文件。收起控制台。
3. 按下表做三轮。每一轮在 A 点输入 **Begin** 并按 Enter。收起控制台，按 **P → 返回主菜单**。菜单就绪后等约 5 秒，点击 **继续本次试炼**。在 A 点原地等约 5 秒，再输入该轮的 **End** 并按 Enter。

   | 轮 | Begin | End |
   | --- | --- | --- |
   | 1 | `Trace.RegionBegin M6_03_Continue01` | `Trace.RegionEnd M6_03_Continue01` |
   | 2 | `Trace.RegionBegin M6_03_Continue02` | `Trace.RegionEnd M6_03_Continue02` |
   | 3 | `Trace.RegionBegin M6_03_Continue03` | `Trace.RegionEnd M6_03_Continue03` |

4. 结束第三轮后输入 `Trace.Stop`，用主菜单退出。确认游戏已关闭再启动下一组。

### 2.2 AudioOff01 禁用音频的诊断组

回到**同一个 PowerShell 窗口**，运行下方命令。如果已经关闭 PowerShell，先重新执行第 2 节的变量定义，跳过启动 AudioOn01 的最后一行。

```powershell
& './Tools/Run-StageBuild.ps1' @launch -RunName AudioOff01 -NoSound
```

按 2.1 节重复预热、两个 CVar 查询、三组同名 Region、停止采集与退出。首屏和 A 点恢复应一致。暂停设置里如果出现音频路由不可用，符合关闭设备的诊断条件。可从日志核对音频设备行为，无需在两组里实际播放试听。

## 3. 验收与回传

核对两个 JSON 的包、存档哈希和启动参数。每组只统计三次预热后的同进程 Continue，先验证六个 Region 闭合且各含一次去菜单和一次回 A 的切图，再量取切图帧、`LoadMap`、`FAudioThreadTask.Flush`、`FlushAsyncLoading` 和切图后第一帧。子事件按嵌套关系解释。若关闭音频后 Flush 消失但整体帧没有相称下降，将继续检查剩余等待。若数据确认音频设备是重要因素，再决定针对具体音频调用做小范围优化。

```text
M6-03 采集完成
AudioOn01 / AudioOff01 实际包与 Runtime 目录（沿用可填沿用）：
两组首屏 A 点存档 / 首次出生与资源 / 画质和窗口：
音频输出设备 / 接电与电源档位（不清楚可填不清楚）：
AudioOn01 三轮 A Continue / 三组 Begin-End / 正常退出：
AudioOff01 三轮 A Continue / 三组 Begin-End / 正常退出：
两组 Trace.Stop / 异常或明显卡顿（无填无）：
完成时分 / 实际手动分钟：
```

## 4. Commit / Push

采集脚本与首次指南已提交。本批暂存本指南、`Docs/Reports/M6_03_Verification.md`、`Docs/Progress.md` 和 `Docs/Backlog.md`。采集文件、诊断导出和存档留在本地 Artifacts。

- 英文 `docs(perf): verify audio overhead during trial map travel`
- 中文 `docs(perf): 验收试炼切图的音频开销对照`
