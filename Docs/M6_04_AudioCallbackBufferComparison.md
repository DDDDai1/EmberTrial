# M6-04 音频回调缓冲对照

比较 Windows XAudio2 的 **1024 帧与 512 帧**。两组都启用音频，使用同一个 M6-02 包和同一份 A 点存档。启动脚本为当前进程覆盖回调缓冲，并把请求值写入 JSON。正式配置仍为 1024。无需编译、Cook 或改 UE 资产。

M6-03 整体关闭音频后，Continue 切图帧中位数下降 24.08 ms。这个差值包含音频设备路径的整体影响。在 48 kHz 下，1024 帧和 512 帧分别对应约 21.33 ms 与 10.67 ms 的音频块。实际 Flush 受任务调度、渲染队列和设备回调影响，是否因此缩短由本章对照确认。[M6-03 验收](Reports/M6_03_Verification.md)

准备检查已通过。用 `Buf512Probe` 短启动，Realtek 扬声器的 XAudio2 日志显示实际采用 512 帧，主菜单显示 A 点存档，游戏正常退出。

## 1. 准备与启动

预计手动 15–20 分钟。关闭 UE 编辑器和其他 EmberTrial 窗口。保持设备接电、扬声器有声、两个进程使用同一个音频输出设备。PowerShell 整段执行，首屏应显示最近存档 `Checkpoint_A`。

```powershell
Set-Location 'E:/UE_Project/Demo'
$seed = 'E:/UE_Project/Demo/Artifacts/M6_02/Runtime/Attempt01/User/After01_20261114_215142_739/Saved/SaveGames/EmberTrial_Run_A.sav'
$launch = @{
    BuildDirectory = 'E:/UE_Project/Demo/Artifacts/Builds/M6_02/Attempt01'
    EvidenceStage = 'M6_04'
    Width = 1920
    Height = 1080
    Trace = $true
    PerformanceBaseline = $true
    SeedCheckpointSave = $seed
}
& './Tools/Run-StageBuild.ps1' @launch -RunName Buf1024 -AudioCallbackFrames 1024
```

日志、JSON 和 Trace 保存到 `Artifacts/M6_04/Runtime`。每组独立复制种子存档，包与种子哈希可从 JSON 对照。两组请求相同的 1920×1080 与中档渲染 CVar。是否真正采用 1024 或 512，以各自日志的 `Callback Buffer Frame Size To Use` 和 `Output buffers initialized` 为准。本章两个 Trace 同时供 M6-05 使用。[M6-05 指南](M6_05_AsyncLoadingAttribution.md)

## 2. 每组的三轮 Continue

1. 首屏点 **继续本次试炼**，在 A 点确认第一场已清、生命和体力 100/100、治疗 3/3。站定约 15 秒预热。检查游戏内窗口显示为 1920×1080。
2. 打开游戏控制台，依次查询 `sg.GlobalIlluminationQuality`、`r.SkyLight.RealTimeReflectionCapture`、`Trace.Status`。前两项预期为 1、0，Trace 应在写文件，已启用通道预期包含 LoadTime 与 File。本节测量期间让角色留在 A 点。
3. 每轮先在 A 点输入对应 **Begin** 并回车。收起控制台，按 **P → 返回主菜单**，等菜单就绪约 5 秒。点 **继续本次试炼**，在 A 点再等约 5 秒，打开控制台输入对应 **End** 并回车。先完成本轮 End，再开始下一轮。

   | 轮次 | Begin | End |
   | --- | --- | --- |
   | 1 | `Trace.RegionBegin M6_04_Continue01` | `Trace.RegionEnd M6_04_Continue01` |
   | 2 | `Trace.RegionBegin M6_04_Continue02` | `Trace.RegionEnd M6_04_Continue02` |
   | 3 | `Trace.RegionBegin M6_04_Continue03` | `Trace.RegionEnd M6_04_Continue03` |

4. 第三轮结束后输入 `Trace.Stop`。保持当前游戏打开，接下来的听感检查在 Trace 停止后完成。

## 3. 听感检查与切换到 512

两组在同一设备和音量下做相同检查。依次在 A 点输入下面两条游戏控制台命令，分别按回车，等第一段播完再播放第二段。

```text
et.AudioProbe music
et.AudioProbe effects
```

记录两路是否有声，是否听到爆音、断续或明显丢音。然后按 P 返回主菜单，再继续到 A 点，各试听一次，核对换图后两路仍可用。该次 Continue 在 Trace 停止后进行。

从主菜单退出，回到同一个 PowerShell 窗口执行下方命令。若已关闭 PowerShell，先重新执行第 1 节变量定义，跳过启动 Buf1024 的最后一行。

```powershell
& './Tools/Run-StageBuild.ps1' @launch -RunName Buf512 -AudioCallbackFrames 512
```

重复第 2 节和本节听感检查。若候选有爆音或断续，记录发生在首次试听、换图后试听，还是两种情况都发生。用主菜单正常退出。

## 4. 验收与回传

先核对两组实际采用的缓冲分别为 1024、512，音频设备一致，采样率与排队缓冲数一致，每组三组 Region 都闭合。分析会在 GameThread 与任务线程同时寻找 `FAudioThreadTask.Flush`，比较 Flush、`LoadMap`、切图帧的中位数和最慢一轮，并单独记录异步加载波动。结合实机听感判断是否值得下一批修改正式配置。如果实际值回退、收益不稳定或出现听感退化，将保留当前正式值并归档结果。

```text
M6-04 采集完成
Buf1024 / Buf512 实际包与 Runtime（沿用可填沿用）：
两组 A 点存档 / 资源 / 1920×1080 / CVar 查询：
接电 / 电源档位 / 实际音频设备（不清楚可填不清楚）：
Buf1024 三轮 Continue / 三组 Begin-End / Trace.Stop：
Buf512 三轮 Continue / 三组 Begin-End / Trace.Stop：
1024 首次与换图后两路听感（有声、爆音、断续、掉音）：
512 首次与换图后两路听感（有声、爆音、断续、掉音）：
正常退出 / 最明显问题（无填无）：
完成时分 / 实际手动分钟：
```

## 5. Commit / Push

本批暂存本指南、`Tools/Run-StageBuild.ps1`、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。运行日志、Trace、JSON、存档和专用配置留在本地 Artifacts。

两章一起提交时使用 [M6-05 的合并提交建议](M6_05_AsyncLoadingAttribution.md#4-commit--push)。

- 英文 `chore(perf): add audio callback buffer comparison for trial travel`
- 中文 `chore(perf): 增加试炼切图的音频回调缓冲对照`
