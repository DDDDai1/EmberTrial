# M2-07B：新版独立包的必要性能窗口

2026-10-25，UE **5.5.4 / Win64 Development**。前置[A验收](Reports/M2_07A_Verification.md)通过，使用已验证的**M2_07A/Attempt01**。本批不改玩法C++、资产或共享Config，不重新Build / Cook、跑23项或搭验收蓝图。

**只采稳定待机与真实敌人交战两段。**不重做M1的Pause / Idle02 / Retry采集，不计已关闭需求的首次PIE创建等待。本阶段是灰盒负载观察，不是最终作品三轮固定路线达标或Shipping性能认证。

## 1. 分工、时间与输出

你负责启动 → 确认条件 → 两段采集 → 打开正确Trace；我负责从Trace / Runtime读取有效时间、导出统计、判断线程 / GPU事件与可能瓶颈并写报告。**不要求你精确掐按键时间、读伤害日志或手动计算P95。**

预计手动20–35分钟：准备 / 查询5–10，实际采样约3–5，打开Insights / 确认轨道5–10，反馈约5；工具解析等待另记。若已有Insights使用经验，不强迫重复学习或导出大CSV。

| 材料 | 位置 |
| --- | --- |
| 已验证包 | `E:\UE_Project\Demo\Artifacts\Builds\M2_07A\Attempt01\Windows\EmberTrial.exe` |
| 新采集目录 | `E:\UE_Project\Demo\Artifacts\M2_07B\Runtime` |
| Trace / Runtime / JSON | 同一`Trace01_日期_时间戳`前缀，脚本自动生成 |
| 视口辅助截图 | `E:\UE_Project\Demo\Artifacts\M2_07B\Viewport01.png` |
| 性能报告 | 采集并分析后由助手生成`Docs/Reports/M2_07B_Verification.md`，当前没有空报告 |

现有[Run-StageBuild.ps1](../Tools/Run-StageBuild.ps1)已由助手扩展M2_07B选项、独立输出目录及中等画质条件；语法 / WhatIf检查通过，**还没有实际采集或得出性能结论**。M1旧命令仍使用原High=2及M1目录。

## 2. 准备与固定条件（约5分钟）

1. 保存并关闭UE编辑器、CLion；停止Build / Cook / 索引 / 录屏，Insights等采集结束再打开。不是要求更改系统设置或关闭安全软件。
2. 笔记本接电，使用平时开发档位；记得名称就反馈，不清楚可填“不清楚”，不先改风扇 / 电源策略。不要清Shader / DDC / Saved缓存。
3. 原包与默认资源 / AI不改。A已确认当前源与包一致；不要把旧Cleanup或M1 Attempt01混进来。
4. 本批请求**1920×1080、Medium=1、未限帧**，对应Introduction的1080p中等画质方向。A的1280×720功能样本、M1的High=2不同，不能直接算“提升百分比”。最终三轮路线 / 完整美术负载另在后续验证。

| 条件 | 本批请求值 |
| --- | --- |
| t.MaxFPS / r.VSync | 0 / 0 |
| 十个sg画质组 | 各1，Medium：视距 / 抗锯齿 / 阴影 / GI / 反射 / 后处理 / 纹理 / 特效 / 植被 / Shading |
| r.ScreenPercentage / 动态分辨率 | 100 / r.DynamicRes.OperationMode=0 |
| 通道 | cpu、frame、gpu、bookmark、region、loadtime、log，另有statnamedevents |

这些都是本次进程的运行命令，不写共享Config。GI / Reflection等实际渲染状态仍以Runtime / Trace为准，Medium下不保证所有Lumen功能都启用；不要把项目开关当作实际Pass测量。没有Memory分配通道，本批不能据此判断内存泄漏。

## 3. 启动本轮采样

PowerShell完整执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M2_07A\Attempt01' -EvidenceStage M2_07B -RunName Trace01 -Width 1920 -Height 1080 -Trace -PerformanceBaseline
```

必须保留BuildDirectory与EvidenceStage；脚本历史默认仍是M1旧包，单独只写Trace参数会选错对象。正式命令不要带WhatIf；它只打印，不会采集。

控制台会显示**同一前缀**的Log / Trace / Metadata路径，例如：

```text
Artifacts\M2_07B\Runtime\Trace01_20261025_本次时间戳.log
Artifacts\M2_07B\Runtime\Trace01_20261025_本次时间戳.json
Artifacts\M2_07B\Runtime\Trace01_20261025_本次时间戳.utrace
```

进入菜单 → 开始新试炼，回到Entry。前一轮A的检查点不跨进程保留；本轮先不激活检查点、不走近敌人。

## 4. 确认条件和视口（约3–5分钟）

### 4.1 使用游戏控制台，不是PowerShell

1. 在战斗世界、未暂停时，按Esc下方的**反引号 / 波浪键**打开控制台。切英文输入法。
2. 每次复制下面的一行到游戏控制台，Enter执行；按该键关闭控制台再操作角色。不要把Trace命令粘进PowerShell。
3. 依次查询六项，**没有数值后缀**的是查询，不是设置：

   ```text
   t.MaxFPS
   r.VSync
   r.ScreenPercentage
   r.DynamicRes.OperationMode
   sg.ShadowQuality
   sg.GlobalIlluminationQuality
   ```

4. 预期依次为**0、0、100、0、1、1**。你不必抄长输出，我从本轮日志核对；若明确不同，先反馈，不临时调一串值后当作同条件。
5. 再输入`Trace.Status`，应有文件输出和上述相关通道。输入一次`stat unit`观察Game / Draw / GPU是否有读数，再输入一次`stat unit`关闭覆盖层。缺GPU读数如实说明，不填0ms。

### 4.2 预热与一张视口截图

1. 关闭控制台，保持Entry、初始镜头，静置约60秒。是正常世界待机，**不按P暂停**；不用精确到毫秒。
2. Entry与敌人初始相隔约12m，正常不会主动追来。若敌人已靠近，不改AI / 位置，记下现象并反馈，我来判断安全待机样本。
3. 预热后打开控制台，输入：

   ```text
   HighResShot 1 filename=E:/UE_Project/Demo/Artifacts/M2_07B/Viewport01.png
   ```

4. Enter → 关闭控制台，再等约5秒。这张引擎原始截图用于辅助核对实际渲染尺寸，**不放在待机分析区间内**；截图本来不含UMG，不把缺HUD当故障。
5. 如果窗口被屏幕 / DPI缩小或裁剪，反馈“有 / 无 / 不清楚”，不为了凑1080p修改Windows缩放。请求分辨率不是已验证的实际视口尺寸；我结合截图和日志记录，不要求你当场重采。

再次采样时RunName用Trace02；截图也改Viewport02，保留已有原件，不覆盖失败尝试。正常只采一轮，不预设三轮压力测试。

## 5. 两段采集（实际操作约3–5分钟）

Begin / End必须使用完全相同名称。区间由Trace记录真实时间，人工约20秒足够，不要求你掐点。控制台录入会留在首尾，分析时由我剔除边界。

### 5.1 稳定待机：M2_07B_Idle01

1. Entry、未暂停、没有敌人活动，打开控制台输入：

   ```text
   Trace.RegionBegin M2_07B_Idle01
   ```

2. Enter → 关闭控制台，保持同一站位 / 镜头，不输入约20秒。
3. 打开控制台输入：

   ```text
   Trace.RegionEnd M2_07B_Idle01
   ```

4. Enter → 关闭。待机样本结束；无需加Idle02、暂停或菜单区间。

### 5.2 真实交战：M2_07B_Combat01

**必须包含敌人真正追击或主动挥击，玩家HUD至少一次真实扣血。只做玩家空挥 / 闪避不能算交战样本。**

1. 仍在安全位置先输入：

   ```text
   Trace.RegionBegin M2_07B_Combat01
   ```

2. 关闭控制台，走近MeleeEnemy_A，让它追着你移动并主动挥击；Tab锁定。准备走近的部分留在Trace里，由我根据AI / 命中日志切出真正交战内部窗口，你不用判断精确起点。
3. 看见真实敌人活动后，约15–20秒内做**左键单点一次、2一次、F一次**，受伤后拉开 / 站定Q一次，继续观察敌人。**不要连按左键打死唯一敌人**，也不要求在受击精确帧闪避；被打断或拒绝按真实结果保留。
4. 若仍存活，开控制台输入：

   ```text
   Trace.RegionEnd M2_07B_Combat01
   ```

5. 关闭控制台。敌人真实伤害 / 受击、能力与HUD活动由日志对应，主观取景问题另记，不为采集改伤害、AI、无敌时间或slomo。

**提前死亡怎么处理：**保留本轮，不立即重采。能在Failure页开控制台就输入同名RegionEnd；不方便时先Retry再结束也可，我会裁掉死亡 / 暂停 / 切图段。若结束命令没执行，不反复补造同名Begin；按下一节停Trace并告诉我，我读取死亡前有效部分后判断。通常数秒以上、含真实AI活动的内部窗口就可分析，不由你先判“失败”并整轮重来。

### 5.3 及时停止、退出与保留文件

1. 输入`Trace.Stop`并Enter；它停止追踪，不是退出游戏。不要继续开着Trace学习Insights或等十几分钟。
2. P返回菜单 → 正常退出；死亡页则返回菜单退出。日志保留到退出。
3. 在`Artifacts\M2_07B\Runtime`应有本轮同前缀`.utrace`、`.log`、`.json`，Trace大小大于0。仅有文件不代表采集已通过，下一节确认可打开，最终由我分析。

## 6. 打开正确Trace并找到自己的区间（详细步骤）

### 6.1 先取得本轮文件名

游戏已退出后，在PowerShell执行下面只读命令，显示最新Trace完整路径：

```powershell
Get-ChildItem -LiteralPath 'E:\UE_Project\Demo\Artifacts\M2_07B\Runtime' -Filter '*.utrace' |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1 -ExpandProperty FullName
```

只复制它输出的完整路径，不带引号。你要找的是**M2_07B目录、本次Trace01时间戳**，不是M1旧Trace，也不是Run01功能日志。

### 6.2 从Frontend选择磁盘文件

1. 双击`D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealInsights.exe`，已有Frontend就复用，不开多份。
2. 在Frontend首页**右下角蓝色Open Trace按钮右边的小箭头**点击一次。
3. 菜单选**Open Trace File...**，不是点蓝色主按钮来打开列表中的旧记录。
4. 文件选择框的File name里粘贴刚才完整`.utrace`路径，Open。
5. 等新的分析窗口打开并完成解析，不因几秒没出现轨道就连续重开。明确报Failed时给路径和截图；不改Trace Store目录或复制文件进它。

如果打开的是旧`M1_07B`或UnrealEditor记录，关闭这一错误分析页，再选择本轮文件；不删除原Trace。采样期间可能已有较多事件，解析等待按实际另记。

### 6.3 显示Timing / CPU / GPU

1. 分析窗口选择顶部**Timing Insights**页。顶部Frames、Timing、Timers是显示开关，已显示不要重复点掉。
2. 在Timing面板内的**CPU/GPU下拉菜单**，确认CPU Thread Tracks与GPU Track(s)勾选。
3. 左侧找GameThread、RenderThread或RenderingThread、GPU。轨道太多用面板滚动条或Shift+滚轮上下找；普通滚轮主要缩放时间。
4. CPU / GPU缺数据、一直空白时给截图与Trace路径；不能把缺失写成0ms，也不先认定需要改代码。

### 6.4 找Idle01：它在Timing Regions横条上

`M2_07B_Idle01`是第5.1节的**区间名称**，不是文件名 / 函数名；**不要在右侧Timers搜索框找它**。

1. 点击Timing事件区空白处，让该面板获得焦点。
2. 左边已有**Timing Regions**这一行就保持；没有才按**Ctrl+R一次**显示。它通常位于Bookmarks下方、GPU轨道上方。
3. 看Timing的秒数时间尺，不看Frames里的帧编号。鼠标放事件区、滚轮向下几格，把可见时间扩大到覆盖本轮记录；本轮不是旧M1的固定7–8分钟，不照抄那个位置。
4. 沿Timing Regions这一行找带`M2_07B_...`的**横向区间条**，不是GameThread里的密集彩色函数块。
5. 文字被截短时，把鼠标停在横条中央，读提示框顶部完整名字。确认是**M2_07B_Idle01**，不是Combat01。
6. 单击这条横条，保持Timing焦点，按**F一次**放大到它的范围。成功依据是完整名字和左右边界可见，不猜颜色。

若只是看不到名字，先检查文件 / 区域行 / 时间范围，不立刻重采。找Combat01用同样方法，**两条都找到即可**；提前死亡造成Combat尾部较长由我处理。

### 6.5 可选：选择内部3–5秒，看Timers聚合

已会操作可跳过，不强制CSV导出或填写性能数字。

1. 在Idle01中间选一段，避开前后控制台输入。
2. 鼠标放在Timing上方带`s / ms`的**时间刻度尺空白处**，按住左键横向拖动约3–5秒后松开。
3. 成功时应有纵向覆盖多轨道的选区，Timers随选区聚合；只给某个彩色块描边不是选中时间范围。
4. 如果是在平移画面，落点不在刻度尺；回到刻度尺重试。也可在事件区Ctrl+Shift+左键拖选。

本批只需要能打开正确文件、看见相关轨道并找到两条区间。我会用正式导出处理有效帧 / 线程 / GPU统计，你不用抄All累计时间算FPS。

## 7. 分析与后续安排

我收到反馈后核对原包7份指纹、157 Source / Content / Config输入、分辨率请求和截图 / CVar实际条件，确认真实AI时段，统计可用帧 / 主循环间隔、P50 / P95 / P99和尖峰，区分CPU / Render / GPU的重叠与等待。不会相加Inclusive或把同名GPU事件误当CPU耗时。

当前Development / Trace / 业务日志均有开销；Medium与旧M1 High、A的720p条件不同，不直接宣称优化收益。只有发现可复现瓶颈才提出窄修复；未发现时如实记录。缺字段标未知，提前死亡保留有效部分，不补造电源档位、完整GPU帧或Shipping数字。

采集有效后我生成分析 / 验收报告，并按约定进入M2-07C技术复盘；只有关键通道缺失或无真实交战等问题才补相关片段，不重新做A整章。

## 8. 最简反馈

```text
M2-07B 采集完成 / 卡在第__节第__步
实际包仍为M2_07A/Attempt01；UE与明显后台负载已关闭、接电：
电源 / 风扇档位（不清楚可填不清楚）；窗口有无缩小 / 裁剪：
本轮Runtime目录（沿用默认可填沿用）/ Trace文件大小：
六项CVar查询 / Trace.Status与Game-Draw-GPU读数：
Viewport01.png已生成（失败如实写）：
约60秒预热 / Idle01 / Combat01已标记：
敌人真实追击或挥击 / 玩家扣血；是否提前死亡：
左键单点-2-F-Q实际执行或被打断情况：
Trace.Stop / 正常退出 / Insights打开本轮文件：
CPU-GPU-Frames可见 / 找到两条Timing Regions：
失败或特别说明（无填无）：
完成时分 / 实际手动用时 / 工具解析等待：
```

不要求你抄伤害、时间轴精确秒数、成功长日志或性能统计；报错发第一条错误文字，界面定位问题发含窗口标题 / 面板的截图，动态镜头问题给5–10秒视频。原始文件不上传Git，助手直接读取本机路径。

## 9. 本批 Commit / Push（固定放在最后）

A相关实现 / 工具 / 下发文档已在本地183b80c；本轮新增：

```text
Docs/Reports/M2_07A_Verification.md
Docs/M2_07B_PerformanceCapture.md
```

同步修改：

```text
Tools/Run-StageBuild.ps1
Docs/M2_07A_IntegratedPackage.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Docs/M2_Roadmap.md
Docs/M2_06_TargetLockRoadmap.md
Docs/M2_05_BreakPulseRoadmap.md
README.md
Introduction.md
```

当前只提交A收尾 / B准备材料时，建议英文`docs: record M2 package acceptance and prepare profiling`，中文译文`docs: 记录M2开发包验收并准备性能采集`。

B采集并分析后，再加入实际`Docs/Reports/M2_07B_Verification.md`及上述最新文本；建议英文`perf: record the M2 runtime profiling baseline`，中文译文`perf: 记录M2阶段运行时性能基线`。报告未生成就不创建空文件、不提前写作性能已验证。

Source / Content / Config / AssetManifest本轮保持，不重复Stage；包、Runtime / JSON、Trace、截图、CSV、Raw、Saved / Binaries / Intermediate / CMake索引留本地，不Force Add。CLion Refresh Changes → Stage最新内容 → Review Diff → Commit → Push，由用户操作，助手未改索引、Commit / Push或联网核验远端。
