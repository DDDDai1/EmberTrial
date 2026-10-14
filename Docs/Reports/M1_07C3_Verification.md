# M1-07C3：Attempt02新版包验收

更新2026-10-14，UE **5.5.4**。用户14:40报告全项完成、无问题，手动10 / Build-Cook等待10分钟。**C3验收通过，无待补项；M1-07及M1最小纵向闭环收口。**结果同时采用用户实机反馈与直接读取的UAT / 产物 / 两轮日志；不是助手运行的第三轮游戏。

## 1. 实际产物与配置

- 启动exe：`E:\UE_Project\Demo\Artifacts\Builds\M1_07C3\Attempt02\Windows\EmberTrial.exe`。
- 真正Game exe：上述Windows目录内 `EmberTrial\Binaries\Win64\EmberTrial.exe`，14:36:33生成，与项目本次Game产物SHA256一致。
- 原UAT：`C:\Users\D1334\AppData\Roaming\Unreal Engine\AutomationTool\Logs\D+UE5+UE_5.5\Log.txt`，14:37:32结束；已读取原日志，另将工作区Editor日志中同次完整UAT输出保存为Raw内的`UAT_EditorOutput.txt`，便于以后不依赖外部滚动日志。
- Runtime：`Artifacts/M1_07C3/Runtime/Run01_20261014_143815_441.log / .json`、`Run02_20261014_144042_636.log / .json`。两PID不同，启动14:38:15 / 14:40:42，正常退出14:40:32 / 57。

UAT明确Target=EmberTrial、Win64 / Development、archive到Attempt02，Build / Cook / Stage / Package / Archive均COMPLETED，最终ExitCode0 / BUILD SUCCESSFUL。工具内部BuildCookRun **171.81秒**；用户登记等待10分钟仍按原反馈计入，不用工具时长覆盖用户整体等待口径。Cook有一条ShaderCompileWorker job completion port Warning，进程成功且产物正常；单独记录，不误记为Runtime错误或要求清缓存。Editor启动阶段的EOS / 图标等记录不混入独立进程错误统计。

75Source / 37cpp、48项清单资产逐份与C2一致；DefaultEngine.ini不变。DefaultGame.ini哈希变化，但非空行与C2合法配置快照一致，主要为空行 / 保存格式整理，Cue路径与Packaging / Maps语义保持，脚本再次PASS0/0。当前快照记录新哈希，不把“语义一致”写成字节不变。

包指纹范围为**启动exe、实际Game exe和五个Pak / IoStore容器**，不冒充所有第三方DLL审计。bootstrap哈希仍与A相同（553A0D…），它只是启动器；真正Game exe和五容器均已变化，且实际Runtime请求Radius90。因此不能仅凭bootstrap相同判断误运行旧包。原Attempt01七指纹仍保持A历史版。

## 2. Run01实际顺序

| 北京时间 | 直接证据 |
| --- | --- |
| 14:38:20 | 菜单Continue0，无Pawn；NewRun实际Entry / HUD100 |
| 14:38:41 | 玩家→训练目标真实25：100→75，Base同步 |
| 14:38:48 / 57 / 58 | 原型闪避开始、E激活M1_Target_A、暂停；随后恢复 |
| 14:39:09–27 | Move请求Radius90；结束Aborted / 距离98.9，之后十次敌人10伤害、HUD100→90→80→0与一次Death sealed / Failure |
| 14:39:47 | 第一次Retry确实Entry：此时尚未激活A，不改写成A点重试 |
| 14:39:52 | 真正Checkpoint_A activated |
| 14:39:55–14:40:14 | 第二次Move仍90，结束Succeeded / 距离89.4；再次100→90→80→0，真实死亡 |
| 14:40:15 | Retry请求 / Spawn / Snapshot均Checkpoint_A，新身体100 |
| 14:40:23–24 | 菜单Continue1；Continue请求 / Spawn / Snapshot均Checkpoint_A，新身体100 |
| 14:40:31–32 | 菜单退出，RequestExitWithStatus0 / 日志正常关闭 |

玩家**保持站定**、实际键鼠与中文显示依据用户报告；日志直接支持主动Move / 攻击 / 伤害时序，不宣称日志单独证明没有移动输入。第一轮Move Aborted处于攻击分支接管，98.9cm已在Melee100内，后续连续伤害真实成立；第二轮移动成功89.4。Request Radius90已带入包，**不要求每次转攻击都精确停90cm或必须Succeeded**，更不篡改原生Result。

这轮二十次真实敌人伤害 / 两次死亡，最终有完整A点链路，所以此前激活前的Entry重试不是未关闭失败，也不需要补测。两世界重建后原生Life / Controller / HUD各清理；会话仍仅保存地图 / 检查点，交互 / 敌人重置符合既定M1语义。

## 3. Run02与运行边界

Run02新进程首屏Continue0，NewRun实际Entry / HUD100；14:40:50在本轮才再次激活A。当前有玩家攻击Begin / End记录，具体左键来源与移动反馈按用户确认；不把空挥视为又一次伤害。P返回菜单、14:40:56请求退出，57日志关闭。

两Runtime匹配Error / Fatal / Accessed None / Ensure为0，Warning为0；没有No GameplayCueNotifyPaths回退Warning，也没有Editor模块ready / M1-07C1 / C2 Validator运行标记。Game receipt为5.5.4 / Development / Game，产品不含EmberTrialEditor；构建边界与运行结果共同支持Editor工具隔离，而不是仅靠“没看到一条日志”推断。

本次1280×720功能运行，没有Trace / PerformanceBaseline。B统计仍属于Attempt01，**没有Attempt02 FPS / 帧耗改善结论**。未测另一台电脑、真实TravelFailure或最终美术压力，不把局部验收扩展成这些条件已通过。

## 4. 工时与归档

C3实际10 / 等待10；累计已报告手动 **2011分钟（33小时31分钟）** / 已明确等待 **185分钟**。M1-07手动175 / 已明确等待45；M1从M1-01到本批手动1338分钟（22小时18分钟）。未报告的B最后替换时间及助手工作仍不补算。

证据快照：`Raw/M1_07C3_2026-10-14/Evidence.json`、`UAT_EditorOutput.txt`、`RuntimeExcerpt.txt`。来源清单没有新增资产；冻结B1哈希保持原值。本轮助手只读核查、写文档，未改Source / 资产 / Config、重新Build或运行游戏、Stage / Commit / Push。

## 5. Commit / Push

本报告、[M1-07总验收](M1_07_Verification.md)、[M1总验收](M1_Verification.md)、[阶段复盘](../M1_Review.md)、[M2路线与校准](../M2_Roadmap.md)及状态文字由用户追加提交。C2主体 / 合法Probe、C3启动工具若未提交，按对应指南末尾清单合并最新差异；DefaultGame实际空白差异核查后可随配置提交。Raw / 两Runtime / 包 / 生成文件仍本地，不Force Add。
