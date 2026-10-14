# M1-07B：独立包Insights性能基线

下发2026-10-13；更新2026-10-14。**B分析 / 最终函数Editor收尾完成，下一步[C1编辑器配置校验](M1_07C1_EditorValidation.md)。**短函数已由用户替换、Editor成功，功能正常按最新反馈登记；Game正常构建与include清理合并C1。原Trace / Combat02有效，不再采样；旧Attempt01不自动更新，C3统一Attempt02。历史排障步骤保留，当前结论见[报告7.6](Reports/M1_07B_Verification.md)。

你负责启动、按明确步骤采集一次、确认Insights能打开；我负责读取Trace / 导出数据、定位线程与尖峰并生成分析报告。**不要求你精确掐按键时间**：区间标记记录真实开始 / 结束，重试结合原生日志定位。预算45–75分钟，主要是首次熟悉工具；实际采样流程约5–10分钟。

## 1. 本批成果与范围

交付一份可打开的`.utrace`、对应Runtime.log / JSON，以及后续由助手完成的性能报告，说明：

- 当前Development包、硬件 / 画质 / 分辨率 / 电源与采集开销。
- 两段同条件Entry待机、一次真实AI交战、暂停区间、一次失败重试的时序与耗时证据。
- 帧持续时间、CPU线程 / GPU事件与尖峰的区别；最值得继续调查的区域，或当前未发现需改动的瓶颈。

只对真实问题优化。如果没有瓶颈，不为了展示“优化前后”虚构数据、关闭玩法或随意降画质。该单敌人灰盒基线不等于最终美术场景性能，更不等于另一台机器或Shipping已测。

## 2. 准备（约5–10分钟）

1. 保存并关闭UE编辑器；本次用已验收包：

   ```text
   E:\UE_Project\Demo\Artifacts\Builds\M1_07A\Attempt01\Windows\EmberTrial.exe
   ```

2. 笔记本接电，保持平时用于UE开发的同一电源 / 风扇档位，并在反馈写档位名称；不要求你改系统策略或驱动。关闭Build、索引、视频录制等明显后台负载，CLion可保存后关闭。Insights在**采集结束后**再打开，减少采集期干扰。
3. 本机A日志确认CPU为AMD Ryzen 7 7840H、GPU为RTX 4060 Laptop。驱动 / 内存等我可从Runtime提取；你只补充日志不能证明的电源 / 风扇状态。
4. Tools下的[Run-StageBuild.ps1](../Tools/Run-StageBuild.ps1)已由助手更新；不需要手动抄脚本。新增`-PerformanceBaseline`用于本批运行条件和B目录，原A命令仍保留原默认行为。脚本原先做语法 / WhatIf验证，本轮Trace01已实际启动 / 采集；补采沿用同一工具。
5. 本批不清Saved / Shader缓存、不改Startup Health、伤害、AI距离、slomo或Debug绘制；已有GameplayCue路径Warning保持，先测这版真实包，C再评估扫描范围配置。

## 3. 启动采样

PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -RunName Trace01 -Width 1920 -Height 1080 -Trace -PerformanceBaseline
```

工具打印Executable / Log / Trace / Metadata，输出自动使用独立时间戳：

```text
E:\UE_Project\Demo\Artifacts\M1_07B\Runtime\Trace01_时间戳.log
E:\UE_Project\Demo\Artifacts\M1_07B\Runtime\Trace01_时间戳.json
E:\UE_Project\Demo\Artifacts\M1_07B\Runtime\Trace01_时间戳.utrace
```

本批使用`cpu,frame,gpu,bookmark,region,loadtime,log`通道和`-statnamedevents`；记录CPU / GPU / 帧、区间、加载和业务日志，不同时开Memory / 全量资源追踪。直接写Trace文件，不需要先启动本地Trace Server或以管理员身份运行游戏。

`-PerformanceBaseline`发送固定运行期控制台命令：

| 条件 | 请求值 / 目的 |
| --- | --- |
| 窗口分辨率 | 1920×1080；与A的1280×720功能检查区别记录 |
| t.MaxFPS / r.VSync | 0 / 0，先看未限帧耗时 |
| 十个sg画质组 | 各2，High：ViewDistance、AntiAliasing、Shadow、GlobalIllumination、Reflection、PostProcess、Texture、Effects、Foliage、Shading |
| r.ScreenPercentage | 100 |
| r.DynamicRes.OperationMode | 0，关闭动态分辨率调整 |
| GI / Reflection / Nanite | 沿用包内配置；不是本批新增渲染技术 |

命令只用于本次进程，不改仓库Config。JSON中的Resolution与RuntimeProfileCommands是**请求条件**，下一节还要检查实际生效。`-WhatIf`只打印、不采集，正式命令不要带它；A已经打包，所以也不要再运行Refresh-CLion或重打包。

若脚本报错，保留完整文字与实际exe路径给我；先不要改执行策略、移除检查或清缓存。若窗口受屏幕 / DPI限制被缩小或裁剪，说明实际情况，不把请求值直接记为已验证1080p。

## 4. 确认实际条件与控制台（约3–5分钟）

1. 首屏菜单→开始新试炼；角色落在**Entry**。先不移动到敌人旁，也不激活检查点。本流程只用初始出生点，不沿用A进程会话。
2. 游戏控制台在**战斗世界、未暂停**时使用。按Esc下方的反引号 / 波浪键（键帽上有反引号或 ~ 的键）打开小控制台；再按一次可能展开大控制台，连续按可关闭。切英文输入法，输入一条命令后Enter执行，关闭控制台再操作玩法。不要把命令贴到PowerShell或Output Log。
3. 依次输入下面六条**查询**，每条一行Enter；只读查询无数值后缀：

   ```text
   t.MaxFPS
   r.VSync
   r.ScreenPercentage
   r.DynamicRes.OperationMode
   sg.ShadowQuality
   sg.GlobalIlluminationQuality
   ```

   预期分别0、0、100、0、2、2。无需你抄全输出，我会查本轮Runtime.log。若值不同，停在这里把日志给我，不能带着未确认条件继续标“基线完成”。
4. 可再查询`r.DynamicGlobalIlluminationMethod`、`r.ReflectionMethod`、`r.Nanite`。现有共享配置GI / Reflection方法为1，Nanite项目开关启用；运行值取实际查询。启用开关不代表灰盒资源本身使用了Nanite，也不能据此猜GPU瓶颈。
5. 输入`Trace.Status`确认有文件输出及CPU / Frame / GPU / Bookmark / Region等通道。再输入`stat unit`看Game / Draw / GPU是否有读数；观察后再次输入`stat unit`关闭覆盖层。**GPU无读数或Trace无输出给我实际结果，不填0ms代替缺失。**
6. 关闭控制台、保持Entry位置 / 初始镜头，预热约60秒。不用退出重启、清缓存或录视频。默认布局中Entry与敌人初始约12m，超出获得目标范围；若当前位置敌人主动追击，不修改AI配置，反馈位置 / 日志，我来调整采样点。

## 5. 一次采集：逐段操作（约5–10分钟）

`Trace.RegionBegin 名称`与`Trace.RegionEnd 同名`围出分析区间；名称大小写与拼写保持一致，无空格。下面命令已在本机UE5.5.4的TraceAuxiliary.cpp核对，不是新增蓝图节点。控制台输入 / 关闭耗时留在记录里，分析稳定区间时去掉首尾约1秒，并明确说明。

等待20秒可用手机计时或大致数秒；**不必精确到毫秒**，真正时长由Trace边界决定。不要为了按秒数暂停游戏。录入前后区间名可对照这页，不需要设计测试函数。

### 5.1 第一段待机：Idle01

**准备：**Entry，身体存活，敌人尚未进入追击；预热完成，控制台已关闭。

1. 打开控制台，输入`Trace.Bookmark M1_07B_WarmupReady`，Enter。
2. 输入`Trace.RegionBegin M1_07B_Idle01`，Enter；关闭控制台。
3. 同一站位 / 朝向，完全不输入约20秒。
4. 再开控制台，输入`Trace.RegionEnd M1_07B_Idle01`，Enter；关闭。

**结果：**一段正常世界待机，既非菜单也非P暂停；不拿暂停帧代替正常待机。

### 5.2 暂停区间：Pause01

**准备：**仍在安全Entry，未暂停。

1. 输入`Trace.RegionBegin M1_07B_Pause01`；关闭控制台。
2. 按 / 松P打开暂停页，等待约10秒；鼠标“继续游戏”。
3. 回到游戏输入后，再输入`Trace.RegionEnd M1_07B_Pause01`；关闭。

**结果：**范围包含打开 / 关闭与中间稳定暂停；我按Pause shown / resumed日志分开看菜单UI开销和切换瞬间。**不要在UI Only的暂停页或失败页强行打开控制台**，RegionEnd只在恢复游戏后输入。

### 5.3 真实交战：Combat01

**准备：**玩家与敌人均存活。先走近敌人，让其真实追击；不先杀死敌人。

1. 绕过出生附近的站桩训练目标，靠近真正的`MeleeEnemy_A`（原布局约X=600、Y=250cm，和训练目标不是同一对象）。先亲眼确认它朝你移动、转向或挥击，再输入`Trace.RegionBegin M1_07B_Combat01`，关闭控制台；没有AI行为时先不开始Combat标记。
2. 约15–20秒内正常侧移 / 转向、2闪避避开攻击，站定左键攻击一到两次；保持敌人存活。不要求每次都命中、不按3模拟受击，也不改Health / Damage制造结果。
3. 仍存活时输入`Trace.RegionEnd M1_07B_Combat01`，关闭。

**结果：**至少有真实AI追击或攻击事件，最好带一次真实GE伤害 / 受击；未出现的行为不冒充数据。若提前死亡或敌人被杀，先保留当前Trace：Retry回游戏后补同名End，反馈“Combat01混入死亡”。可在同次进程新试炼中补一段Combat02；只补该段，不重跑整个A或清缓存。我会分别标注有效区间，不能把失败页当交战稳定性能。

### 5.4 失败重试时序

**准备：**当前玩家存活、控制台可用。

1. 输入`Trace.Bookmark M1_07B_BeforeFailure`，关闭控制台。
2. 用现有数字4真实扣血到死亡；显示失败页后选择重新试炼。无需故意等10秒，也不用再次验证所有失败输入。
3. 新世界 / 新100到达后，输入`Trace.Bookmark M1_07B_AfterRetry`；关闭。

**结果：**完整死亡 / Failure / Retry / OpenLevel记录。两书签间包含人的操作等待，**不是自动加载耗时**；我以Travel requested、世界到达及加载事件定位真正转换区间。若受击 / 死亡或菜单输入出现功能失败，只反馈实际失败，不继续标B完成。

### 5.5 第二段同条件待机：Idle02

1. P返回主菜单→选择**开始新试炼**，回Entry / 新100，不选会话Continue。包与进程不变。
2. 同一Entry站位与初始朝向，等待约20秒让切图稳定。
3. 输入`Trace.RegionBegin M1_07B_Idle02`；关闭控制台，等待约20秒。
4. 输入`Trace.RegionEnd M1_07B_Idle02`，关闭。

**结果：**可比较的第二段热状态待机；不是第二次冷启动。不扩大成三十轮压力测试，两段差异明显再补窄采样。

### 5.6 停止与退出

1. 未暂停时输入`Trace.Bookmark M1_07B_End`，Enter；再输入`Trace.Stop`，Enter。关闭控制台。
2. P→返回主菜单→退出游戏，等游戏 / 日志窗口结束。Trace.Stop结束追踪，Runtime日志仍保留至正常退出。
3. 在`Artifacts\M1_07B\Runtime`确认本轮有`.utrace`、`.log`、`.json`，Trace大小大于0；**大于0只证明存在，还需要下一节成功分析。**保留原始Trace，不改扩展名、不覆盖A日志。

## 6. 打开Unreal Insights与最少界面练习（详细版，约15–25分钟）

**2026-10-13修订：原文把Frontend首页入口写成File菜单，不适用于你当前窗口，现已根据本机实际界面更正。**本节不要求重采Trace，也不要求独立完成性能分析。你先做到“打开正确文件 → 找到Idle01 → 选择其中一小段 → 看见CPU / GPU与Timers”，导出CSV是可选练习。

### 6.1 从当前首页打开正确文件

**先确认窗口：**顶部标题是`Unreal Insights Frontend`；左上有`Trace Store`，中间是记录列表，右下有蓝色`Open Trace`按钮。

本轮本地已找到的文件是：

```text
E:\UE_Project\Demo\Artifacts\M1_07B\Runtime\Trace01_20261013_141040_853.utrace
```

文件约2.42 GiB（2,594,864,617字节），所以后面分析可能较慢。若你另外补采了新文件，则改选对应的新时间戳，不删除这份原文件。

**按以下顺序操作：**

1. 如果当前Frontend已经打开，继续使用它；不再双击exe开第二份。若未打开，在资源管理器双击`D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealInsights.exe`。
2. 找到窗口**右下角蓝色Open Trace按钮右边独立的小向下箭头**。点击**小箭头**，不是蓝色按钮的文字区域。
3. 弹出的菜单上方`MISC`区域中，点击**Open Trace File...**。这才是选择磁盘文件的入口。
4. 在弹出的文件选择框底部，点击**File name / 文件名**输入框。将上面的完整`.utrace`路径粘贴进去，不带额外引号。
5. 点击对话框的**Open / 打开**。
6. 等待出现新的分析窗口。可以把新窗口最大化，方便阅读。

**你应该看到：**新的分析窗口有`Session`、`Timing Insights`标签；打开文件的名称对应`Trace01_20261013_141040_853`，Session信息应属于这次EmberTrial / UE5.5采样，而不是旧UnrealEditor / UE5.4记录。

**两种Open Trace不要混淆：**

| 入口 | 实际含义 |
| --- | --- |
| 蓝色Open Trace主按钮 | 分析首页列表里当前选中的记录 |
| 主按钮右侧小箭头 → Open Trace File... | 从磁盘选择本次`.utrace`文件 |

本机首页此前只有名为`20250720_222225`、UE5.4 / UnrealEditor的旧记录，约33 MiB；它不是这次2.42 GiB的游戏包Trace。首页列表没列出本次文件不代表采集失败，因为本批使用指定路径直接写文件。**不要更改Trace Store的Directory，也不用把大文件复制进Trace Store。**

### 6.2 判断是否还在分析，而不是立即重开

1. 观察新分析窗口，等待时间轴 / Timer列表出现并开始响应鼠标。若界面显示分析中的状态，先等待；本次文件较大，不能按几秒没出现图表就判失败。
2. 分析过程中不要连续再点击打开同一文件，避免多个Insights实例同时加载大Trace。
3. 若出现明确的`Failed` / 无法打开提示，截图并给我文件路径；不要改扩展名或删除文件。
4. 若长时间没有进展，反馈“已等待多久、当前窗口截图”，由我判断解析或内存问题；无需先重装工具或重采。

**确认点：**能进入本次Trace的Timing Insights，时间轴能缩放或悬停提示能出现。本节只确认能读取，暂不写FPS结论。

### 6.3 先认出三个面板：Frames、Timing、Timers

1. 点击分析窗口顶部的**Timing Insights**标签。它和旁边的`Session`是不同页面；Session用于看采集信息。
2. Timing Insights页面顶部有一排面板开关，包含**Frames、Timing、Timers、Callers、Callees、Counters、Log**。
3. 首先只认前三个。若对应面板已经显示，不再点击；这些按钮是**显示 / 隐藏开关**，重复点击会把面板关掉。
4. 若某个面板不见了，点击顶部同名按钮一次，把它显示出来。不要为了找面板先找不存在的Window菜单。

默认布局通常如下；你改过布局时，以面板标题为准：

```text
顶部：Frames / Timing / Timers ... 面板开关

上方：Frames —— 很多竖条组成的总览

中左：Timing —— 有时间刻度、下拉菜单和CPU / GPU轨道
中右：Timers —— 带Name、Count、Incl、Excl列的统计表

下方：可能有Log；右侧也可能有Callers / Callees
```

| 面板 | 如何认出来 | 这次拿它做什么 |
| --- | --- | --- |
| Frames | 密集竖条、帧索引，悬停有帧时长 | 看帧是否均匀、哪里有尖峰 |
| Timing | `All Tracks` / `CPU/GPU`等菜单，下面是秒数刻度与事件条 | 找Idle01，选一段时间，观察线程 |
| Timers | `Name` / `Count` / `Incl` / `Excl`表格 | 看所选时间内哪些事件花时间 |

**特别区分：**Frames总览的横向数字可能是**帧编号**；Timing刻度中的`s` / `ms`表示**时间**。不要把Frame编号1000当作采样第1000秒。Callers / Callees / Counters本次无需配置。

### 6.4 让CPU和GPU轨道显示出来

**准备：**你已经看到Timing大面板，其上边有`All Tracks`、`CPU/GPU`、`Other`、`Plugins`、`View Mode`这一排菜单。

1. 点击Timing内部工具条上的**CPU/GPU右侧小箭头**。这是Timing轨道菜单，和右侧Timers表上方的CPU / GPU筛选按钮不同。
2. 查看菜单中的**CPU Thread Tracks**是否勾选。如果没有，点一次使其勾选；已经勾选就保持。
3. 同样查看**GPU Track(s)**，确保勾选。
4. 点击菜单外空白处收起菜单。
5. 看Timing左侧每条轨道的名称，寻找**GameThread**、**RenderThread或RenderingThread**及**GPU**。线程名可能附带ID；不要求所有机器显示完全相同的渲染线程字样。
6. 轨道太多时，在Timing轨道区域按住**Shift再滚动鼠标滚轮**上下找；或拖动该面板右侧滚动条。不要用普通滚轮找上下轨道，普通滚轮在Timing里主要是缩放时间。

**你应该看到：**线程名旁边有一排或多排彩色事件块。当前时间范围没有事件时可以暂时空白，要结合后面Idle01区间再判断。

**如果仍然空白：**先确认选中的是本次游戏Trace，不是Frontend或旧记录；再确认轨道开关勾选、分析已完成。若在明确的Idle01区间仍找不到GPU或CPU，发截图与Trace路径，我来判断缺通道或显示问题；不能填“GPU=0ms”。

### 6.5 找到Idle01：看Timing Regions这一行的横条

**2026-10-13本轮已由助手computer-use操作完成。**当前打开的是`Trace01_20261013_141040_853.utrace`，已显示Timing Regions、选中`M1_07B_Idle01`并按F放大。**你现在可以直接进入6.6；如果屏幕已能看到这个名字，不再按Ctrl+R，避免隐藏这行。**下面是本次实际操作的复现方法。

#### 6.5.1 先明白要找的是什么

`M1_07B_Idle01`是你在第5.1节输入`Trace.RegionBegin M1_07B_Idle01`与对应End时，给待机区间起的名字。

它显示在**Timing面板中的Timing Regions这一行**，形状是一条横向色条。它不是文件名，也不是Timers表里的函数名；无需在右侧`Search timers or groups`搜索框中输入这个名字。

你应按从上到下的顺序找到：

```text
Timing的工具条：All Tracks / CPU/GPU / Other ...
时间刻度尺：带s或m的数字
Bookmarks：PSO、GC、Cmd等标记可能很多
Timing Regions：这里找M1_07B_Idle01横条
GPU：下面的密集彩色函数块不是本次要找的条
GameThread：更多CPU函数块
```

**确认依据是横条上的名字，不能仅凭颜色猜。**本轮选中后是黄色边框，完整文字已显示为`M1_07B_Idle01`。

#### 6.5.2 把Timing Regions这一行显示出来

1. 在Timing面板的事件区空白位置单击，让这个面板获得键盘焦点。不要点击右侧Timers搜索框或下方Log搜索框。
2. 先看左侧有没有**Timing Regions**文字：
   - **已经有：**保持，不按Ctrl+R。
   - **没有：**按**Ctrl+R一次**，再看左侧。
3. 正常会在**Bookmarks下面、GPU上面**出现Timing Regions这一行。
4. 此时即使这一行只有`Init...`或暂时空白，也不代表你的Idle01不存在；可能只是当前显示的时间太靠前。

本轮开始时该行没有显示，助手先让Timing获得焦点，再按Ctrl+R，使它出现。

#### 6.5.3 把显示时间扩大到能看见7～8分钟

**本轮要找的位置：**Idle01横条大约位于采样第**7分10秒至8分01秒**附近。这是Trace中的相对时间，不是电脑时钟；也不是上方Frames总览中的帧编号。

1. 看Timing的**时间刻度尺**。如果只看到`0`、`9.8s`、`19.5s`等，右侧最多到约`2m`，说明目前只看到了采样开头。
2. 将鼠标放在**Timing事件区**，靠近Timing Regions这一行，不放在上方Frames图或右侧Timers表。
3. **滚轮向下滚几格**，时间轴会缩小，让同样宽度容纳更多时间。
4. 每滚几格就看一次时间刻度。继续，直到可见范围包含约**7m至8m**。不要求固定滚几次，因为你当前缩放比例可能不同。
5. 左边出现负时间刻度是缩放位置造成的显示空白，不是录入了负时间数据，不需要修复。
6. 停止滚动，沿**Timing Regions这一行**从左到右看横条。本轮在这个时间段先出现Idle01，后面才是Pause / Combat等区间。

本轮助手先缩小两次，直到视图覆盖后面的采样时间，才看见几个带`M1_07B_...`字样的区间条。之前最前面两分钟里找不到这个名字，是因为Idle01录得更晚，不是你没有理解某个函数名。

#### 6.5.4 名字太短或挤在一起，如何确认

1. 在Timing Regions这一行里，找到位于约7～8分钟的横条。
2. 若条上只显示`M1_07B_Id...`，把鼠标停在**这条横条的中间**，短暂停留。
3. 看弹出的提示框**最上面的名字**。你要确认它完整显示为：

   ```text
   M1_07B_Idle01
   ```

4. 如果提示是`M1_07B_Pause01`、`M1_07B_Combat01`或`M1_07B_Idle02`，就是另一条；把鼠标移到前后相邻的横条再看。
5. 提示框下面的Duration是区间时长，不是名字；不需要你在此计算FPS或要求它恰好20秒。

本轮助手通过该提示框直接确认了**M1_07B_Idle01**，没有通过右侧Timers搜索或猜颜色定位。

#### 6.5.5 选中它，再放大看清完整名字

1. 确认名字正确后，**单击该横条一次**。
2. 保持焦点在Timing，按键盘**F一次**；不要同时按Ctrl或Shift。
3. 视图会围绕这条选中事件放大，Idle01横条变宽，完整名字更容易读。
4. 成功的判断：**Timing Regions这一行能清楚看到M1_07B_Idle01及其左右边界**；本轮已保留这个状态。

如果F没有反应，先确认鼠标刚才点中的是Timing Regions的横条；若在搜索框中出现字母f，说明焦点错了。不要继续在搜索框按F，回到横条单击后再按一次。

**这一步只定位并放大区间，还没完成6.6的时间范围选择。**接下来在Idle01中间选3～5秒，避开前后控制台操作，才用于小范围Timers练习。不要因为右侧表暂时仍显示很多统计，就认为这条名字找错了。

#### 6.5.6 找不到时按现象处理

| 当前现象 | 下一步 |
| --- | --- |
| 左侧完全没有Timing Regions | Timing获得焦点后Ctrl+R一次；已经有就不再按 |
| 有这一行，但只看见Init或空白 | 看时间尺是否还只到最前面2分钟；向下滚轮扩大到7～8分钟 |
| 很多彩色块，却没有M1_07B名字 | 确认正在看Timing Regions行，不是GPU / GameThread行 |
| 横条名字被截短 | 停在条中间读提示框的完整名字；确认后单击、按F |
| 只找到了Pause / Combat / Idle02 | 在同一行看前面的区间，确认当前Trace文件名称正确 |
| 看得到Idle01但Timers没只剩Idle数据 | 先定位完成，再按6.6选时间范围；点事件与选时间范围是两步 |
| 正确Trace覆盖整段时间、区域行仍无Idle01 | 给我当前截图 / 文件路径，我检查数据；不立即重采或创建新标记 |

**本轮结论：Idle01确实存在，定位已完成。**约7分10秒到8分01秒只是帮助你重现本次查找的位置，不替代后续对有效区间 / 帧耗时的分析；不会在这一小节提前标B性能验收通过。

### 6.6 在Idle01内部真正选择一个时间范围

**目标：**只框选Idle01中间约3–5秒用于练习，不要求精确数值，也不必自己记录开始 / 结束秒数。避开区间首尾录入控制台的瞬间。

1. 先通过6.5把Idle01放大到能看清左右边界。
2. 在Timing工具条下方找到**带`s` / `ms`数字的水平时间刻度尺**。它在彩色事件轨道上方，不是最上面的Frames竖条图，也不是Timer表列标题。
3. 选择Idle01中间的一个起点，把鼠标放在其上方**时间刻度尺的空白位置**，避开已有时间标记的小箭头。
4. **按住鼠标左键，不松手，水平向右拖动**到约3–5秒之后的位置。
5. 松开鼠标左键。
6. 观察是否有从上到下覆盖多条轨道的选中范围，以及起止位置 / 范围长度的显示；右侧Timers会按这个范围重新聚合。大Trace刷新可能需要等待。

**成功的样子：**不是只有一个小彩色块被描边，而是**一个纵向覆盖多轨道的时间区间**被选中。若范围过窄，重新拖选稍宽一点即可，不影响Trace原始数据。

**如果拖动后只是在移动画面：**落点在事件区，没在时间刻度尺。重新定位到刻度尺再拖选。

**如果拖动后只出现一条竖线：**可能点到了时间标记，或拖得太短；换刻度尺空白处并拉开距离。

**备用方式：**让Timing事件区获取焦点，按住**Ctrl+Shift**，再用鼠标左键在Timing事件区横向拖选，最后松开鼠标与按键。本机STimingView也支持这个范围选择方式；先尝试刻度尺方法即可。

不要为了“看全”立即再单击一个CPU事件，以免当前事件选择混淆；此时直接到Timers看统计。

### 6.7 Frames的最低限度练习：读一帧，不猜平均FPS

1. 鼠标移到上方**Frames**面板的一根竖条上，停留一下。
2. 查看悬停提示中的Frame索引与Duration；例如提示5ms，只代表那一帧，不代表整场游戏平均5ms。
3. 明显更高的条通常意味着该帧持续更长；本次先能看到提示就够了，不要求逐根记录。
4. 如想选Frames范围，可以先单击一帧，再按住Shift单击另一帧；这会选两帧之间的时间区间。**这是另一种选区方法，可能替换6.6的选择，不是导出Idle01前必须再做的步骤。**

若上方总览仍显示整个Trace，而Timing已放大Idle01，不要凭总览里某一根条的位置猜它就是Idle01；具体帧与区间匹配由我在分析时确认。

### 6.8 Timers表怎么读、怎么排序

**准备：**已按6.6选好Idle01内部范围，右侧Timers有数据。

1. 在Timers上方找到**Search timers or groups**搜索框，确保它是空的；残留关键字可能把大部分Timer过滤掉。
2. 找到**Group by**下拉框。原本可能是`Timer Type`，因此表里分为CPU / GPU组。为方便第一次看，改选**Flat**。
3. 如果表里有唯一的分组行，点其左侧小三角展开；让下面具体Timer名称可见。只出现CPU / GPU标题时，还没展开具体事件。
4. 找到右上**Mode**下拉框，先保留或选择**Instance**。第一次只学习统计含义，不用来声称“每帧平均耗时”。
5. 看表头，含义如下：

   | 界面缩写 / 名称 | 简单含义 |
   | --- | --- |
   | Name | 事件名称，例如某个Tick或渲染阶段 |
   | Count | 选中时间内的调用次数 |
   | Incl / Total Inclusive | 该事件自身与子事件包含的累计时间 |
   | Excl / Total Exclusive | 排除子事件后的自身累计时间 |

6. 点击**Incl**列标题，观察出现的排序箭头和行顺序。若大值在下方，再点一次，使大值排在前面。
7. 看前三到五条名字即可；不用抄整张表，也不用因为某项排第一就立即修改代码。`Wait...`可能代表等待，Inclusive父子相互包含，不可全部相加当作总CPU耗时。

如果列没出现，右键**表头行**，从Column Visibility中启用对应列；也可用本机提供的Reset Columns to Default。右键位置要在Name / Count那一排标题上，不是Timer数据行。

**关于另一种Mode：**Game Frame按游戏帧聚合，Rendering Frame按渲染帧聚合；本次可保持Instance。只有明确了Mode与区间，才解释Avg等列；不要拿按调用统计的平均值写成每帧成本。

### 6.9 可选：只导出当前范围的Timer统计

不会导出可以跳过，将原Trace交给我；无需因这一项再录一次。

1. 若刚用Frames改变过选区，先回6.6重新框选Idle01内部那段。
2. 点击Timers表中一条**具体Timer数据行**，让表获得焦点；不要点击Search输入框。
3. 按**Ctrl+A**，观察表中多行被选中；按**Ctrl+S**。
4. 如快捷键没弹出保存框，可在所选数据行上右键，选择**Export...**。
5. 在保存框的File name / 文件名中填写完整路径：

   ```text
   E:\UE_Project\Demo\Artifacts\M1_07B\Idle01_Timers.csv
   ```

6. 若有Save as type选项，选择CSV / Comma-separated对应格式；点击Save / 保存。已存在同名文件时改用`Idle01_Timers_02.csv`，保留原件。
7. 打开资源管理器，确认文件存在且大小大于0。无需打开Excel、转换格式或手动清理列。

这里使用的**Export...**导出Timer汇总，与你选择的时间范围相关；不要点`Export Timing Events (All)...`，后者可能导出海量逐事件数据。若保存框完全不同，截图并保留当前选区，我来定位。

### 6.10 本节到哪里就可以交给我

**必做：**正确的游戏Trace已打开；至少能观察Frames，以及Timing中的CPU / GPU是否有数据。尝试找到Idle01并选一小段；有哪一步未成功就写具体小节编号，不必为了反馈“完成”硬猜。

**可选：**Idle01_Timers.csv、一张包含Frames / Timing / Timers的截图。截图最好把分析窗口标题也带上，便于确认不是旧UE5.4记录；不必截整个桌面或所有应用。

可在第9节反馈中补这几行：

```text
第6节实际打开文件：Trace01_20261013_141040_853.utrace
Timing Insights / Frames / Timing / Timers：
GameThread / 渲染线程 / GPU：可见或实际缺失项
Idle01区域 / 时间范围选择：成功或卡在6.__第__步
可选CSV路径：已导出或跳过
```

我接着完成数值分析与性能结论。**能打开文件和导出表，不等于B性能分析已经验收通过。**

### 6.11 本节核对范围与参考

此前已用computer-use核对你实际Frontend：记录列表为旧UE5.4 Trace；主按钮打开选中记录，右侧下拉菜单含Open Trace File...。第6节最初说明核对时，临时旧分析窗口已关闭、保留Frontend，当时未加载本轮大Trace。采集后助手另已完整打开 / 导出本轮Trace，分析见报告；当前只有Combat02真实AI负载待补。分析窗口的顶部面板开关与布局有本机界面依据；Ctrl+R、F、CPU / GPU显示、拖选与Export快捷键同时核对UE5.5.4代码。

可对照[UE5.5 Timing面板](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-timing-panel-in-unreal-insights-for-unreal-engine?application_version=5.5)、[Frames面板](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-frames-panel-in-unreal-insights-for-unreal-engine?application_version=5.5)、[Timers与导出](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-timers-and-counters-tabs-in-unreal-insights-for-unreal-engine?application_version=5.5)。这些是帮助理解操作的参考，具体入口以上述本机核对为准。

## 7. 分析由我完成：通过条件

收到材料后，我会确认Trace可解析、通道 / 有效区间和实际CVar条件，再用Insights导出或界面查看：

| 项目 | 报告要说明 |
| --- | --- |
| 帧区间 | 待机两段、交战、暂停与切图分别统计；实际时长和帧数，不能用全Trace平均掩盖加载 |
| CPU | GameThread / Render / RHI与等待；重点Timer归属 / 调用次数，Inclusive父子不能重复累加当总成本 |
| GPU | 实际GPU帧 / 事件，缺失明确写缺失，不能用Draw替代GPU或填0 |
| 尖峰 | 一次最大的可解释尖峰，结合真实切图 / 加载 / GC / 业务事件；无证据不认定根因 |
| 重复性与限制 | Idle01 / 02的差异、Development / Trace / 日志开销，未测冷启动对照或其他硬件不扩展结论 |

帧持续时间包含等待和调度，CPU事件耗时与GPU时间有重叠，不能简单相加得FPS。若观察到明显帧率封顶、配置未生效或后台干扰，先记录原因再窄复采。只有数据支持瓶颈时才进入修改 / 同条件对比；无问题可直接保留基线进入C。

**本轮分析已完成，详见[报告](Reports/M1_07B_Verification.md)。**原Combat01无真实AI活动；Combat02现已有效，8.2绑定已通过，00:26新日志确认玩家站定 / 90.3cm自动切入攻击；当前只剩8.3精简函数手动落地、构建与短基线，不把尚未编译的文档版本标为已实施。现有数值是本机灰盒基线，不是优化收益。

## 8. 问题反馈方式

- 命令 / 脚本报错：完整文字、该轮JSON / Log路径；不要只发ExitCode或一句没反应。
- 控制台或Insights界面找不到入口：整体窗口截图，加“停在第几节第几步”；复杂分析 / 定位由我处理。
- 玩法导致采样提前中断：说明哪一段、死亡 / 敌人死亡 / 暂停情况，保留这轮Trace；只补该段。
- Trace解析失败 / GPU轨道缺失：utrace路径与Insights错误截图 / 日志，保持源文件。先排通道 / 版本 / 分析，不重装UE。

时间不必记录每个按钮的时分秒，区间 / 书签和原生日志会定位；完成反馈只写整体完成时刻和用时。

<a id="combat02"></a>

### 8.1 真实AI交战Combat02（已完成，保留采集步骤，不再执行）

**当前状态：Combat02补采已有效，以下是已执行的历史步骤。现在只操作8.2。**

**当时为什么补：**你已确认Combat01只有玩家左键 / 闪避，MeleeEnemy_A没有追击或挥击；原日志也没有Target acquired / Move / MeleeTask。待机、暂停、重试与CSV保留有效，不需要重做第4–6节，不需要再学习Insights或手动导出表。

**准备：**沿用同一个Attempt01，接电，保存并关闭UE / CLion，关闭正在分析大Trace的Insights窗口与明显后台重负载。具体风扇档位和窗口缩放不清楚就保留“不清楚”，不用改系统设置。保留原Trace01，不覆盖。

1. PowerShell只执行以下启动命令（本轮新文件自动加时间戳）：

   ```powershell
   Set-Location 'E:\UE_Project\Demo'
   .\Tools\Run-StageBuild.ps1 -RunName Combat02 -Width 1920 -Height 1080 -Trace -PerformanceBaseline
   ```

2. 菜单选择**开始新试炼**。在Entry预热约60秒，然后开始靠近敌人；本轮总操作控制在几分钟内，不在采集中长时间学习Insights。
3. **找到正确对象：**出生附近的静止训练人偶、M1_Target_A / B交互目标不是敌人。绕过它们，继续向房间较远处的Quinn走；原场景真正敌人叫MeleeEnemy_A，初始坐标约`600 / 250 / 120cm`，原Entry在其-X方向较远处。标签在独立包里可能不显示，以后面的真实行为判断。
4. 走近它，在它前方可见、没有墙阻挡的位置短暂停留。**先亲眼确认它向你移动 / 转向接近，近距离时会挥击。**不必抄日志、不必掌握精确触发秒数。若已贴近较远处Quinn仍完全站立，先停止推进第5步，按第8步结束Trace并退出，告诉我“真正敌人靠近仍无反应”，附该轮日志路径；我来窄排查，不改伤害 / 感知距离或重新Cook来凑结果。
5. 确认追击或挥击后，切英文输入法，按反引号 / 波浪键开**游戏控制台**，输入以下命令并Enter，然后关闭控制台：

   ```text
   Trace.RegionBegin M1_07B_Combat02
   ```

6. 约15–20秒内绕敌人移动、转向、按2闪避。保持它追击或有真实挥击；想反击可站定左键一到两次，**不要杀死敌人**。不按3模拟受击、不按4主动扣血、不改slomo或Health / GE。被真实攻击扣血一两次可以，不要求硬吃满伤害。
7. 自己仍存活时，游戏控制台输入并执行：

   ```text
   Trace.RegionEnd M1_07B_Combat02
   ```

   若意外死亡，保留数据，点Retry回到游戏后才补输入同名End，并反馈“Combat02混入死亡”；不在UI Only失败页强开控制台，不反复重采到不知道哪份有效。
8. 游戏控制台输入`Trace.Stop`，Enter；关闭控制台。按P → 返回主菜单 → 退出游戏。若第4步敌人一直不响应，本轮没有Begin / End也可以，直接Stop / 退出并报告现象。
9. 文件在`E:\UE_Project\Demo\Artifacts\M1_07B\Runtime`，前缀`Combat02_时间戳`，保留`.utrace / .log / .json`。**无需打开Insights、截图全桌面或再次导出CSV。**用下面四行反馈即可：

   ```text
   M1-07B Combat02补采完成 / 真正敌人仍无反应
   敌人真实追击 / 挥击；是否真实扣血；是否提前死亡：
   Runtime目录：E:\UE_Project\Demo\Artifacts\M1_07B\Runtime
   完成时刻 / 实际手动用时：
   ```

我检查新Trace里的Target acquired / PursuitMove / MeleeTask与实际CPU / GPU区间，合并原报告后关闭唯一必补项。不会要求重做原Idle / Pause / Retry矩阵，也不提前认定本次观察是AI功能Bug。

<a id="moveradius-fix"></a>

### 8.2 MoveRadius绑定补修（已通过，保留历史步骤，不重做本节）

**现在不再执行8.1补采。**你这轮确有真实追击 / 十二次敌人攻击请求 / 十次扣血，死亡前19秒可用于分析；报告已合并。当时收尾问题是Controller决定90cm停步，Move实际却使用150cm，导致150附近提前到达 / 重复Succeeded。这次不重做旧UI / 战斗矩阵、不重新Cook或再采Trace。

#### 8.2.1 只交换源码两行顺序

1. 保存UE资产并关闭UE编辑器；CLion保留。打开：

   ```text
   E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.cpp
   ```

2. 找到`UETBTTask_PursuitMove::UETBTTask_PursuitMove(...)`构造函数，现在这三行附近是：

   ```cpp
   BlackboardKey.SelectedKeyName = PursuitMoveTargetActorKey;
   AcceptableRadius.SetKey(PursuitMoveStopDistanceKey);
   AcceptableRadius = 150.0f;
   ```

3. **只把后两行换序**，最终必须是：

   ```cpp
   BlackboardKey.SelectedKeyName = PursuitMoveTargetActorKey;
   AcceptableRadius = 150.0f;
   AcceptableRadius.SetKey(PursuitMoveStopDistanceKey);
   ```

   150保留作常量默认值，实际运行应由黑板90覆盖；不要改Controller90 / 100 / 100、黑板字符串或其他Move属性。不能只把150改90遮住丢失绑定的问题。
4. 保存cpp。`FValueOrBBKey_Float`的float赋值会带上默认None键，所以必须把SetKey放到最后。之前A指南改名片段没有展示中间常量，且验收漏查顺序，已修正；不是要求你重建类 / 工程或清缓存。

#### 8.2.2 正常Editor / Game Build

UE保持关闭，PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor Build与索引刷新成功后，再执行Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

确认两次成功再打开工程；不是Live Coding，不改生成Module.EmberTrial.cpp。本次没有反射新增、模块或类路径变化，不需要重新创建C++类。构建报错发第一条具体错误与上下文。

#### 8.2.3 只确认一个BT属性

1. UE Content Browser搜索并打开`BT_ETMeleeEnemy`（/Game/EmberTrial/AI/BehaviorTrees）。
2. 选中**ET Pursuit Move To**任务，不选Service或父Sequence；Details找到**Acceptable Radius**。
3. 同一行右侧黑板键选择框必须是**PursuitStopDistance**；如果显示None，改选该键。左侧常量显示150或灰色都可以，绑定有效时实际值来自Controller写入的90。
4. 不改变两项Reach Test Includes Radius、不移节点或重新搭树。保存BT；若原本已选正确键，说明你只需要核对，按实际改动提交。

#### 8.2.4 约1分钟PIE，不再搭验收图

**准备：**L_ET_VerticalSlice，Default Player Start、单个敌人，玩家 / 敌人100/100、原Controller90 / 100 / 100 / 0.5、原GA25 / 10、slomo1 / Debug False；不使用旧独立包。

1. Play，从Entry走到真正敌人的正面视线范围，确认获得目标 / 开始移动。
2. 它走近时暂时站定，让它自己接近并挥击；不要主动向前贴近去掩盖停止距离错误。
3. 确认Output Log有`Move request ... Radius=90.0`（至少一条）和随后敌人真实Melee Requested。若仍150，先Stop，提供新日志时刻 / 完整行；不改另一个数字来凑结果。
4. 玩家HUD真实100→90→80，确认敌人可以主动接近后连续两次造成10伤害，然后Stop即可；这次不用等到死亡 / 重试，不检查旧E / P全矩阵。
5. Save All。反馈以下六行：

   ```text
   M1-07B MoveRadius补修完成 / 卡在8.2.__
   常量在前、SetKey在后；Editor / 索引 / Game Build：
   BT Acceptable Radius绑定PursuitStopDistance，已保存：
   实际Move Radius90；站定由敌人主动接近，HUD100→90→80：
   源码 / 资产保存，原默认值保留：
   完成时刻 / 手动用时 / 构建等待：
   ```

你不用抄全部日志，写大致完整时分，我读取最近PIE片段核对。通过后B收尾、进入C；**不用重采Insights，也先不单独花20多分钟出修复包**。C有限校验完成后统一生成Attempt02，再做包内真实追击 / 攻击短检查。当前Attempt01是含150回归的历史采样包，不是已修复的新包。

<a id="arrival-diagnostic"></a>

### 8.3 OnTaskFinished最终精简版（替换原诊断函数；待用户手动落地）

**本节已直接覆盖原来的长诊断函数，不另建函数 / 文档。**最新00:26日志确认：玩家站定，敌人在90.3cm进入攻击资格，23.322 / 25.001两次真实伤害使HUD100→90→80；当次自主接近攻击检查通过。原23:49的未攻击片段继续作为历史记录，未唯一定位其剩余原因；增加日志本身没有改变玩法，不能写成“日志代码修复了AI”。

原函数的大部分代码是一次排查用的快照，已经不需要每次结束都打印。最终版只保留**Controller、任务原始Result、Goal、实际Distance2D、请求Radius**，便于以后发现“90请求却提前停止”或目标切换；原生Super清理保留。它只负责清理与日志，不查询GAS / 动作状态，不改变攻击资格。

#### 8.3.1 include收尾

保存并关闭UE。打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.cpp`：

- 删除上一轮仅为临时诊断添加的`#include "Engine/World.h"`和`#include "GameFramework/CharacterMovementComponent.h"`；该文件其余现有函数没有使用它们。
- 在include区加入以下一行（已有则不重复），显式提供APawn定义，其余原include保留：

```cpp
#include "GameFramework/Pawn.h"
```

#### 8.3.2 用这个最终版覆盖整个OnTaskFinished

构造函数正确的“常量在前 / SetKey在后”和ExecuteTask不动，.h不改。将当前长函数整体替换为：

```cpp
void UETBTTask_PursuitMove::OnTaskFinished(UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    const AAIController* Controller = OwnerComp.GetAIOwner();
    const APawn* Body = IsValid(Controller) ? Controller->GetPawn() : nullptr;
    const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    const AActor* Goal = IsValid(Blackboard)
        ? Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName))
        : nullptr;

    const FString ControllerName = GetNameSafe(Controller);
    const FString GoalName = GetNameSafe(Goal);
    const double Distance2D = IsValid(Body) && IsValid(Goal)
        ? FVector::Dist2D(Body->GetActorLocation(), Goal->GetActorLocation()) : -1.0;
    const float Radius = IsValid(Blackboard) ? AcceptableRadius.GetValue(OwnerComp) : -1.0f;

    // Snapshot before cleanup; always retain native move task cleanup.
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
    UE_LOG(LogETPursuitMove, Display,
        TEXT("[M1-05B2] Move finished Controller=%s Result=%s Goal=%s Distance2D=%.1f Radius=%.1f"),
        *ControllerName, DescribeMoveResult(TaskResult), *GoalName, Distance2D, Radius);
}
```

**保留与删除的理由：**

- 距离与半径保留：前者是身体 / 目标中心水平距离，后者是黑板实际请求值；不能只打印Radius就认为已经到达该距离。没有有效对象时Distance=-1，黑板无效时Radius=-1，仍然执行原生清理。
- Goal / Controller名称与数值在Super前保存，避免原生清理之后目标 / 委托状态变化影响快照；Super在所有出口恰好调用一次，没有提前return绕过清理。
- 删除NavDistance、四组位置、半径布尔项、Idle / Ground / MeleeAllowed / NetMode等一次诊断字段。这些信息已在本次Raw保留，相关门控还有Controller自己的日志；不要把它们当成OnTaskFinished必需玩法逻辑。
- 使用通用AAIController / APawn / AActor读取位置，去掉本函数对具体Character状态 / GAS查询的依赖；不额外拆出只调用一次的辅助函数。
- `Result`保持引擎传入的真实值。00:26那条Failed出现在NearTarget关闭追击与攻击切换过程中，后续确实攻击成功；结合Controller的StopMovement与原生MoveTo非成功映射为Failed机制解释，不能简单等同AI攻击失败，更不能在这里擅自改写Succeeded。

这是减少日志计算 / 临时状态耦合的代码收尾，**不宣称带来可量化FPS提升，也不是再次修改AI距离。**

#### 8.3.3 最终落地：正常双Build + 一条短基线

1. 保存cpp，UE关闭后PowerShell执行：

   ```powershell
   Set-Location 'E:\UE_Project\Demo'
   .\Tools\Refresh-CLion.ps1
   ```

2. Editor / 索引成功后，再补最终Game Development Build（诊断轮只构建了Editor，不能把旧Game receipt当新函数已编译）：

   ```powershell
   & 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
   ```

3. 开正式地图、Play，走入真正敌人视线后**松开移动键站定**。让敌人自己接近并连续两次真实命中，HUD100→90→80；确认结束日志只保留Controller / Result / Goal / Distance2D / Radius，随后Stop。正常切入攻击不要求Result必须Succeeded，记录真实结果即可；不重做旧UI / 暂停 / 输入矩阵。
4. Save All；不重新Cook / 采Trace。只反馈：

   ```text
   M1-07B 最终函数已替换
   精简函数 / include，Editor / 索引 / Game Build：
   玩家站定自主接近；HUD100→90→80；精简日志：
   已保存；完成时分 / 手动用时 / 构建等待：
   ```

**2026-10-14收尾更新：**短函数已由用户替换，Editor构建00:38:40成功；功能正常依据最新反馈。最新留存PIE日志早于替换，Game exe仍旧版；Game Build与include清理合并[C1](M1_07C1_EditorValidation.md)正常构建，不再执行上面的B短检查。本节步骤作为历史交接保留；B分析 / Editor收尾已接受，C3统一Attempt02，旧包不自动更新。

## 9. 最简反馈模板

```text
M1-07B 采集完成 / 卡在第__节第__步
实际包仍为Attempt01；UE关闭、接电 / 风扇档位：
Trace / Runtime.log / JSON完整路径（或Runtime目录）：
六项CVar查询符合；窗口分辨率有无缩小 / 裁剪：
预热约60秒；Idle01 / Idle02 / Pause01区间：
Combat01有效 / 提前中断及Combat02补段：
真实失败Retry；BeforeFailure / AfterRetry书签：
Trace.Stop后菜单正常退出；Trace文件大小：
Insights成功打开；CPU / GPU / Frames是否可见：
可选截图 / Idle01_Timers.csv路径：
失败或特别说明（没有填无）：
完成时间（完整时分）：
实际用时：准备__ / 采集__ / Insights学习__ / 记录__
```

## 10. 本批需要Commit / Push（固定放在文档最后）

- 修改`Tools/Run-StageBuild.ps1`：PerformanceBaseline、B输出目录 / 条件元数据、Trace Log / Region通道。默认A启动命令兼容；工具本身无需新增C++类或资产，8.2另外需要修改既有Task.cpp及核对既有BT。
- 新增`Docs/M1_07B_InsightsBaseline.md`、`Docs/Reports/M1_07A_Verification.md`；A指南、M1-07路线、Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow_Options / Introduction实际更新。
- 新增`Docs/Reports/M1_07B_Verification.md`：真实CPU / GPU分析、环境限制、当次自主攻击及最终Editor收尾 / Game转C1状态；B指南8.3与状态摘要实际更新一起提交。当前建议`docs: close M1 performance analysis and plan editor validation`；明确Game最终构建 / Attempt02仍待后续，不写新包已通过。8.2已改Task.cpp，BT内容哈希未变（核对 / 保存不等于有差异），不人为重复加入。重新Stage用户已实施8.3的当前完整Task.cpp（含正确SetKey顺序与精简日志），BT仅有真实差异时提交；同时将M1-06A原指南 / 验收补注与报告最新变化重新Stage。Config本次无新改动，不人为加入。DefaultGame.ini的A改动若已提交不重复加入。
- `Artifacts\M1_07B`的utrace / Log / JSON / 截图 / CSV、A包与Raw均留本地，不Force Add、不上传大型Trace到仓库；截图有展示价值时后续另准备精简公开成果。

CLion：Save All → 刷新Changes → 最新Unstaged变化重新Stage → 检查Staged Diff → Commit → Push。建议本批下发提交`perf: prepare M1 packaged Insights capture`，分析完成后再`docs: record M1 performance baseline`；Commit / Push由用户执行。
