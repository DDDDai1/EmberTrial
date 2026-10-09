# M0-06：首次 Windows Development 包验收

日期：2026-10-01。**M0-06 验收通过。**打包完成、Game 构建、实际包文件和一轮独立运行日志已直接核对；两次独立启动、画面与具体输入边界按用户实机反馈登记。

## 1. 用户结果与证据范围

用户报告：打包成功，UE 编辑器关闭后运行正常；角色 / 场景 / 三段动画、1 / 2 / 3、闪避打断、E-A/B、中文 UI、暂停 / 鼠标与键盘继续 / 不补播 / 不卡键、最后移动跳跃均通过。退出后第二次启动与状态重置通过，配置已保存、临时设置已恢复。

完成时间约 20:46；手动操作 **30 分钟**，构建 / Cook 等待约 **26 分钟**，分开计账。

用户随后澄清：**第一轮运行日志未单独保存。**目前能直接读取的一份日志是 20:45:47–20:46:34 的运行样本，不能写成“两份日志均已读取”。现有样本没有覆盖闪避受击打断、攻击中暂停和全部键盘输入场景，这些项以用户反馈为据。无需为补齐过程日志重跑完整矩阵；后续采用不同日志文件名保留多轮证据。

## 2. 实际配置与构建

`EmberTrial/Config/DefaultGame.ini` 已保存：Build=Always、PPBC_Development、FullRebuild=True、ForDistribution=False；Pak / IoStore / Prerequisites 开启，bCookAll=False，bSkipEditorContent=False；MapsToCook 为 `/Game/ThirdPerson/Maps/ThirdPersonMap`。BuildTarget 为空由 UAT 解析，实际命令明确 `-target=EmberTrial`。

默认启动地图仍为 ThirdPersonMap，GameMode 为 EmberTrialGameMode。

直接读取 `EmberTrial/Binaries/Win64/EmberTrial.target`：

| 字段 | 实际值 |
|---|---|
| TargetName / TargetType | EmberTrial / Game |
| Platform / Architecture | Win64 / x64 |
| Configuration | Development |
| 引擎 | 5.5.4，Changelist 40574608 |
| 编译工具链 | MSVC 14.38.33145 / Windows SDK 10.0.22621.0 |

日志确认 UHT 生成反射代码、项目源码及两个测试 cpp 编译、链接 EmberTrial.exe，**本次确实重新编译了 Game 目标**。测试 cpp 参与编译不代表此次运行了 Lifecycle / DodgeMovement 自动测试。

打包事件来自 `E:\UE_Project\Demo\EmberTrial\Saved\Logs\EmberTrial.log`；另直接读取 UAT `C:\Users\D1334\AppData\Roaming\Unreal Engine\AutomationTool\Logs\D+UE5+UE_5.5\Log.txt` 交叉核对。

| 北京时间 / 阶段 | 结果 |
|---|---|
| 20:26:24 | 开始 Serialized UAT，Win64 / Development / EmberTrial，Build / Clean / Cook / Stage / Pak / IoStore / Archive |
| 20:28:14 | BUILD COMMAND COMPLETED；Build command time=108.25 s |
| 20:44:05 | Cook Commandlet 摘要：Success，0 errors / 0 warnings |
| 20:44:14 | COOK COMMAND COMPLETED；Cook command time=959.94 s |
| 20:44:22 | STAGE 与 PACKAGE COMMAND COMPLETED |
| 20:44:23 | ARCHIVE COMMAND COMPLETED；BUILD SUCCESSFUL；AutomationTool ExitCode=0 |
| 完整 BuildCookRun | 1077.37 s，约 17 分 57 秒；AutomationTool 自报 17 分 58 秒 |

用户报告的约 26 分钟是体验等待时间，日志中的约 18 分钟是这次 UAT 任务计时，两种口径同时保留。Cook 的 0 警告摘要不等同整个 UAT / 编辑器日志没有任何警告。

## 3. 包文件与独立运行

启动程序：`E:\UE_Project\Demo\Artifacts\Builds\M0_06\Attempt01\Windows\EmberTrial.exe`，168,448 字节；其启动的是包内 `EmberTrial/Binaries/Win64/EmberTrial.exe`，299,743,232 字节。

Windows 输出目录读取到 **59 个文件、1,054,283,318 字节（约 0.98 GiB）**，包含 Development PDB 和 prerequisites；不是最终 Shipping 包体或压缩下载大小。Pak / IoStore 容器和 `Engine/Extras/Redist/en-us/UEPrereqSetup_x64.exe` 均存在。

运行日志：`E:\UE_Project\Demo\Artifacts\M0_06\EmberTrial_Standalone.log`。

| 北京时间 | 直接读取事件 |
|---|---|
| 20:45:47 | 打开日志；Development；引擎 5.5.4；Base Directory 指向 Attempt01 的包内 Binaries/Win64 |
| 20:45:50 | 加载 ThirdPersonMap；两个练习 Actor 初始化；PracticeAction 从 Idle 初始化 |
| 20:46:09.739 → 10.767 | 攻击 Begin → Completed |
| 20:46:10.933 → 11.231 / 11.633 | 闪避位移 150.0 cm，SourceTime=0.300；之后 Montage Completed / Idle |
| 20:46:11.864 → 12.500 | 受击 Begin → Completed |
| 20:46:14.042 → 15.234 | Pause opened → Resume requested → Pause closed |
| 20:46:18.656 / 20.572 | Target_Changed / Target_B 激活 |
| 20:46:32.870 → 34.059 | ViewportClosed 请求退出，状态码 0；Game engine shut down；日志正常关闭 |

启动参数为 `-windowed -ResX=1280 -ResY=720 -log -abslog=...`；没有 PIE 地图前缀，基础路径在打包目录。可确认这是实际 Game 包运行样本。日志加载了 Roboto 和 DroidSansFallback 字体，中文外观正确以用户反馈为据。

该运行日志未匹配到 Blueprint Runtime Error、Accessed None、LogBlueprint / LogScript Error、Fatal error、Assertion failed 或 `: Error:`。保留一条初始化时 shader preload 的 Warning；不将其作为功能失败或稳定帧率测量。本批没有采集 Insights / GPU 性能证据，也未验证其他电脑。

## 4. 留档与下一步

筛选构建事件、运行事件、完整当前运行日志副本和 receipt 摘要保存在 `Docs/Reports/Raw/M0_06_*_2026-10-01.*`，均被 Git 忽略；试玩包仍保留在 Artifacts。

已登记手动用时 **643 + 30 = 673 分钟（11 小时 13 分钟）**；本次已报告等待 26 分钟另列。未报告的助手排障、阅读与早期等待不补算。

本轮没有修改工程 C++ / UE 资产、运行游戏或重打包。下一步校准结果见 [M0-07](M0_07_Calibration.md)，随后进入 [M1-01 正式纵向地图](../M1_01_VerticalSliceMap.md)。
