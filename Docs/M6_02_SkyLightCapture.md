# M6-02 降低 Continue 后的天空光捕获开销

已完成旧包三轮和候选包三轮对照。候选三轮均确认六面场景捕获消失，Continue 后一帧中位数为 21.16 ms。[验收结果](Reports/M6_02_Verification.md)

本章处理正式试炼图在同一进程内从主菜单继续时的渲染尖峰。先用现有包重复采样，再仅调整正式地图的 Sky Light，打一个候选包跑同样的路线。预计手动 25–40 分钟，另计打包等待。你完成编辑器设置和采集，我读取 Trace、对比画面并出验收结论。

## 1. 问题定位

M6-01 的 Continue 切图帧间隔为 58.088 ms，紧接着一帧为 111.944 ms。第二帧中 GameThread 有约 110.3 ms 等待帧同步，RenderThread 依次执行六项 `CaptureCommand`（合计约 83.0 ms）与 `UpdateCaptureContents`（20.8 ms）。52.5 ms 的 `LoadMap` 和返回主菜单时的 70.4 ms 尖峰分别统计。[M6-01 原始基线](Reports/M6_01_Verification.md)

UE 5.5.4 的 `FScene::UpdateSkyCaptureContents` 在 `SLS_CapturedScene` 分支渲染立方体的六个面，随后处理亮度与滤波。正式地图的资产中有 Sky Light 组件记录，M6-01 实际运行日志确认中画质将 `r.SkyLight.RealTimeReflectionCapture` 设为 0。编辑器视口按高画质运行时可能走实时捕获路径，性能比较使用独立包。引擎定位入口为 `Renderer/Private/ReflectionEnvironmentCapture.cpp` 和 `Engine/Private/Components/SkyLightComponent.cpp`。

试炼图当前没有需要持续改变天空照明的玩法，候选将天空光光源设为 UE 自带的日光立方体贴图。引擎仍会复制和滤波贴图，六个面的场景重绘应从这一路径消失。环境补光、反射和对角色的照明可能改变，保留候选需要可见画面通过复测。

## 2. 采修改前的对照

关闭 UE 编辑器和其他游戏窗口。使用同一台接电设备、相同电源档位和窗口条件，维持系统现有缓存。打开 PowerShell，整段复制执行。

```powershell
Set-Location 'E:/UE_Project/Demo'
$skyBefore = 'E:/UE_Project/Demo/Artifacts/Builds/M5_04/Attempt01'
& './Tools/Run-StageBuild.ps1' -BuildDirectory $skyBefore -EvidenceStage M6_02 -RunName Before01 -Width 1920 -Height 1080 -Trace -PerformanceBaseline
```

### 2.1 准备 A 点

1. 首屏应显示继续不可用。开始新试炼，从 Entry 出生，等约 60 秒。按反引号键打开游戏控制台，逐条查询下列项目。若分辨率或画质有出入，记录实际值。

   ```text
   t.MaxFPS
   r.VSync
   r.ScreenPercentage
   r.DynamicRes.OperationMode
   sg.ShadowQuality
   sg.GlobalIlluminationQuality
   r.SkyLight.RealTimeReflectionCapture
   Trace.Status
   ```

   前七项预期依次为 **0、0、100、0、1、1、0**。Trace 应处于采集中。
2. 关闭控制台，击败第一场近战，进入 `Checkpoint_A`。看到 **ACTIVE** 后停住，等待几秒让保存完成。只需打这一场。装备槽留空，第二场保持待触发。

### 2.2 同一进程做三次 Continue

1. 在 A 点打开控制台，输入 `Trace.RegionBegin M6_02_Continue01`。收起控制台，按 **P → 返回主菜单**。
2. 主菜单就绪后等约 5 秒，点击 **继续试炼**。角色应在 A 出生，第一道门保持开，生命与 HUD 正常。松开输入，在原地等约 5 秒。打开控制台输入 `Trace.RegionEnd M6_02_Continue01`，关闭这次计时。
3. 第一轮 Region 结束后，用控制台执行下方截图命令，再收起控制台让截图完成。保持出生时的镜头朝向，供候选包对照。

   ```text
   HighResShot 1 filename=E:/UE_Project/Demo/Artifacts/M6_02/Runtime/Before_A.png
   ```

4. 在同一进程中再按第 1–2 步操作两轮，区间名依次改为 `M6_02_Continue02`、`M6_02_Continue03`。每轮都从当前 A 点返回主菜单，再点击继续。无需重打一场。误操作也关闭该 Region，记下轮次和现象。
5. 执行 `Trace.Stop`，从菜单正常退出。脚本启动时显示的日志、JSON 和 Trace 同前缀，均在 `Artifacts/M6_02/Runtime`。每次启动使用独立的设置和存档目录。JSON 同时记录 exe 与 IoStore 内容目录的 SHA-256，方便区分只改地图的包。第一种包结束后无需打开 Insights 或手动导表。

## 3. 设置候选天空光

1. 启动 UE 5.5.4，打开 `/Game/EmberTrial/Maps/L_ET_TrialRoute`。在 Outliner 搜索 **SkyLight**，选择场景中的 **SkyLight** Actor。若 Details 没显示光照设置，选它的 **Sky Light Component**。截图记录当前 **Mobility**、**Real Time Capture**、**Source Type**、**Cubemap**、**Cubemap Resolution** 和 **Intensity Scale**，供回退。
2. 在 **Details → Light** 中先取消勾选 **Real Time Capture**，再把 **Source Type** 切到 **SLS Specified Cubemap**。如果字段不在屏幕上，在 Details 搜索框分别搜 `Real Time Capture` 和 `Source Type`，每次搜完清空再继续。
3. 在 **Cubemap** 资产选择器选 **DaylightAmbientCubemap**，确认资产引用为 `/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap`。搜不到时，在 Content Browser 的 **Settings → Show Engine Content** 打开引擎内容，搜索这个名称，选中 **Texture Cube**，再点 Sky Light 的 Cubemap 字段旁 **Use Selected Asset from Content Browser** 左箭头。保持原 Mobility、Cubemap Resolution 和强度，方向光、曝光和大气也沿用正式地图的值。
4. 保存正式地图。关掉编辑器前重开这张地图，确认组件仍指向这张立方体贴图。在视口检查 A 点和二号房入口，留意角色和门在背光时是否仍易辨认。发现明显变暗、色偏或贴图无法保存时先反馈，保留记录以便调整候选。

## 4. 打候选包

退出 UE 编辑器及游戏窗口。在 PowerShell 整段执行。若 `Attempt01` 已存在，改用下一个 Attempt 编号，并同步第 5 节。

```powershell
Set-Location 'E:/UE_Project/Demo'
$skyAfter = 'E:/UE_Project/Demo/Artifacts/Builds/M6_02/Attempt01'
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,EmberTrial -ErrorAction SilentlyContinue) {
    throw '请先关闭 UE 编辑器和 EmberTrial 游戏窗口。'
}
if (Test-Path -LiteralPath $skyAfter) {
    throw 'Attempt 已存在，请选择新编号。'
}
New-Item -ItemType Directory -Path $skyAfter | Out-Null
$skyProject = (Resolve-Path './EmberTrial/EmberTrial.uproject').Path
$skyUatArgs = @(
    'BuildCookRun', "-project=$skyProject", '-target=EmberTrial', '-platform=Win64',
    '-clientconfig=Development', '-unrealexe=UnrealEditor-Cmd.exe', '-installed',
    '-nop4', '-unattended', '-utf8output', '-nocompileeditor',
    '-build', '-cook', '-stage', '-pak', '-iostore', '-compressed',
    '-package', '-prereqs', '-nodebuginfo', '-archive', "-archivedirectory=$skyAfter"
)
$skyUatLog = Join-Path $skyAfter 'UAT_Console.log'
& 'D:/UE5/UE_5.5/Engine/Build/BatchFiles/RunUAT.bat' @skyUatArgs | Tee-Object -FilePath $skyUatLog
if ($LASTEXITCODE -ne 0) {
    throw "UAT 失败，保留 $skyUatLog 并反馈第一条错误。"
}
foreach ($skyOutput in @('Windows/EmberTrial.exe', 'Windows/EmberTrial/Binaries/Win64/EmberTrial.exe', 'Windows/EmberTrial/Content/Paks/EmberTrial-Windows.utoc')) {
    if (!(Test-Path -LiteralPath (Join-Path $skyAfter $skyOutput))) {
        throw "归档缺少 $skyOutput。"
    }
}
```

预期 UAT 输出 `BUILD SUCCESSFUL` 和退出码 0。打包时若提示 DaylightAmbientCubemap 缺失或无法 Cook，保留首条相关日志。

## 5. 采候选并回传

```powershell
Set-Location 'E:/UE_Project/Demo'
$skyAfter = 'E:/UE_Project/Demo/Artifacts/Builds/M6_02/Attempt01'
& './Tools/Run-StageBuild.ps1' -BuildDirectory $skyAfter -EvidenceStage M6_02 -RunName After01 -Width 1920 -Height 1080 -Trace -PerformanceBaseline
```

完整重复第 2.1–2.2 节，包括同样的三组 Region 名。截图路径改为 `E:/UE_Project/Demo/Artifacts/M6_02/Runtime/After_A.png`。采样完用 `Trace.Stop` 停止 Trace。从 A 点短查移动、镜头、P 继续和第二场入口，再从菜单正常退出。若某轮有新报错或明显画面差异，保留现象和轮次。

我会按 `UEngine::LoadMap` 对齐两组 Trace，逐轮记录切图帧和后两帧的间隔、GameThread 帧同步等待、RenderThread 六面捕获或贴图复制与滤波、以及 >50 ms 的帧数。重点看目标事件是否消失以及最明显的卡顿是否缩短，同时核对 A 出生、门状态和两张截图。三轮来自同一进程内的热切图，首轮和后两轮分开列出。如果六面捕获消失但尖峰还在，会继续定位剩余等待。如果画面验收失败，根据第 3.1 节记录恢复原值并保存。

```text
M6-02 采集完成
Before 实际包路径 / Trace 与 Runtime 目录（沿用可填沿用）：
SkyLight 原设置 / 候选引用 / 地图保存重开：
After 实际 Attempt、exe、UAT 与 Runtime（沿用可填沿用）：
两包画质 / 实际窗口尺寸 / 是否接电：
Before 三次 A 点 Continue / After 三次 A 点 Continue：
A 点两张截图 / 补光、反射、角色与门可读性差异：
候选移动 / P继续 / 第二场入口短查 / 两包正常退出：
失败或明显卡顿（无填无）：
完成时分 / 手动分钟 / Build-Cook 等待分钟：
```

### 5.1 候选窄补采

2026-11-14 已完成。实测运行名为 `After02Seeded`。使用同一个候选包，将既有 A 点测试存档复制到本轮独立用户目录，首次进图用于预热，随后完成三轮正式记录。[三轮数据](Reports/M6_02_Verification.md#帧时间对照)

复用 `M6_02/Attempt01`。现有 Before 三轮和截图已经可用。沿用第 5 节命令，把 `-RunName After01` 改为 `-RunName After02`。

1. 新进程按第 2.1 节到 A，等 **ACTIVE**。维持接电和原窗口条件。截图沿用本次已有文件。
2. 在同一进程连续执行三轮。每一行先输入 Begin，关闭控制台后按 **P → 返回主菜单 → 等约 5 秒 → 继续试炼 → 在 A 点等约 5 秒**，最后输入同行的 End。

   | 轮次 | Begin | End |
   | --- | --- | --- |
   | 1 | `Trace.RegionBegin M6_02_Continue01` | `Trace.RegionEnd M6_02_Continue01` |
   | 2 | `Trace.RegionBegin M6_02_Continue02` | `Trace.RegionEnd M6_02_Continue02` |
   | 3 | `Trace.RegionBegin M6_02_Continue03` | `Trace.RegionEnd M6_02_Continue03` |

3. 第三轮 End 后执行 `Trace.Stop`，从菜单正常退出。每轮结束确认回到了 A，再开始下一轮。原 After01 会作为额外样本保留。

```text
M6-02 候选窄补采完成
After02 三次 A 点 Continue 与三组 Begin/End：
Trace.Stop / 正常退出：
失败或明显卡顿（无填无）：
完成时分 / 额外手动分钟：
```

## 6. Commit / Push

采集脚本和地图候选已提交。本批暂存本指南、`Docs/Reports/M6_02_Verification.md`、`Docs/Progress.md` 和 `Docs/Backlog.md`。Trace、运行日志、CSV、专用配置、截图、测试存档与两个包均留在本地 Artifacts。

- 英文 `docs(perf): record skylight capture comparison for trial resume`
- 中文 `docs(perf): 记录试炼继续流程的天空光捕获对照结果`
