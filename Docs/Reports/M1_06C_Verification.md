# M1-06C：会话检查点与菜单继续验收

复核：2026-10-12，UE **5.5.4**。**本批功能验收通过，无待补功能检查。**用户00:30反馈全部完成、无失败；实际手动**85分钟**（代码45 / 资产30 / 检查10），构建等待5分钟。检查点重试、菜单继续、开始重置和失效ID修正均有运行证据。

助手本轮只读工程 / 构建记录 / 索引 / 日志，更新验收文档、来源清单与本地证据；未修改Source / UE资产、运行Build / PIE或代Stage / Commit / Push。之前仅经用户当次授权操作9.2第8步播放设置，并改正文档入口；9.3默认文字入口也已澄清。D暂停迁移尚未下发。

## 1. 源码、构建与归属

- 相对[C下发基线](Raw/M1_06C_DesignCheck_2026-10-11.json)，新增两份CheckpointStart文件、修改八份Session / GameMode / MainMenu Widget / Controller文件；其余**57份既有Source哈希不变**，无删除。当前**67Source / 33cpp**。
- [C指南](../M1_06C_SessionCheckpoints.md)十个全文代码块与实际文件一致，比较忽略空白 / 注释。Core不依赖UI；Build.cs / Target / .uproject、角色、CombatLife / FailureFlow、战斗 / AI、旧HUD与Presenter保持下发版本。
- 最后Source修改**2026-10-11 23:42:05.837**；Editor receipt **23:43:06.102**、Game receipt **23:44:40.776**均较晚，Win64 / Development / UE5.5.4、目标类型正确。用户确认双Build成功；助手未重新构建。
- 实际`compile_commands.json`于**23:43:16.064**更新，去重**33个项目cpp**，包含新CheckpointStart实现。

Session只跨图保存地图包名与稳定ID，区分待定请求与已确认快照；GameMode在Pawn生成前选择ID，到达后提交实际选择。唯一Entry是地图契约，失效ID回退后会话同步修正，不对已死身体治疗 / 传送。CheckpointStart原生Overlap只接受本地受控、存活Ready身体；一次NextTick补出生重叠，结束清Timer与委托，无常驻Tick。菜单资格读取已确认快照，按钮另受pending门控，Continue不受Menu BeginPlay早于PostLoad的时序误禁用影响。

## 2. 运行直接证据

[日志摘录](Raw/M1_06C_Acceptance_2026-10-12.log)、[结构化证据](Raw/M1_06C_Evidence_2026-10-12.json)、[最终基线](Raw/M1_06C_Baseline.json)仅留本地。以下时间为**Asia/Shanghai，原日志UTC加8**；主要验收窗口为**2026-10-12 00:25:49–00:29:58**，另保留00:01:33–00:01:36的直接地图PIE。

| 用例 | 直接证据 |
| --- | --- |
| 首次开始 / Entry | 00:25:49菜单Pawn=None / Continue0；00:25:56 NewRun请求Entry，实际选择Entry、Snapshot committed.Entry，Confirmed1 |
| 激活 / 去重 / 重试 | 00:26:12.266唯一激活Checkpoint_A；重入后没有第二条激活。00:26:33 Retry请求 / 实际选择 / 提交均Checkpoint_A，新HUD100/100 |
| 菜单立即可继续 | 00:27:01 Menu ready.Continue1早于同毫秒MainMenu arrived；00:27:10 Continue请求 / 出生 / 提交Checkpoint_A，确认成功 |
| 开始重置 / 随后继续 | 00:27:47 NewRun请求 / 提交Entry；00:28:00 Continue也请求 / 提交Entry，没有保留上一轮Checkpoint_A |
| 失效ID负例 | 00:28:49.259开发命令Applied1；00:29:03请求Missing_Checkpoint_Test，Matches0 / FallbackEntry，实际出生Entry并提交Entry |
| 修正后的下一次请求 | 00:29:26.417 Retry已请求Entry，Confirmed1，不再使用失效ID；两条fallback Warning来自同次登录 / 出生选择，不是两次切图 |
| Stop / 新会话 | 00:29:28 Session end；00:29:35新Session ready、菜单Continue0，未沿用上一轮ID |
| 直接地图入口 | 较早00:01:33直接PIE，Requested / Actual均Entry，Gameplay world ready.Spawn=Entry / Pending0，00:01:36正常结束 |

主要窗口共**三次NewRun、三次Retry、两次Continue、三次MainMenu请求 / 到达**，全部Expected1 / Confirmed1 / Pending0；八次游戏快照提交。两组Session ready / end、八组Checkpoint ready / end、五组菜单ready / removed / end闭合。六份失败页显示 / 移除、八组Life订阅 / 解绑与Flow初始化 / 结束、八份Health HUD创建 / 移除及新Ready100快照闭合。相关Blueprint Error / Accessed None / Fatal / 项目LogET Error为0，没有未确认到达。

## 3. 资产保存与来源

| 资产 | 最后正常保存（本地时间） |
| --- | --- |
| 新BP_ETCheckpointStart | 2026-10-11 23:59:06.793 |
| 正式L_ET_VerticalSlice | 2026-10-12 00:19:11.248 |
| WBP_ETMainMenu | 2026-10-12 00:25:14.313 |

运行出生位置证实Entry **(-600, 0, 120)**、Checkpoint_A **(-600, 650, 120)**，未发现位置调整。朝向、唯一ID / 无实例覆盖、单触发 / Marker NoCollision、四BindWidget / 无Binding / Tick Never、三按钮中文及Compile / Save按用户完整反馈接受；本轮未逐字段解析二进制Blueprint或操控Designer。

[来源清单](../AssetManifest.csv)登记一个原创检查点BP（依赖UE引擎Cylinder / 默认3D文字），更新地图与主菜单Widget用途 / 保存 / SHA-256，**45→46项**；其余**43项既有资产哈希不变**。没有新增付费资源、复制引擎模型或改正式敌人 / 玩家资产。46个路径唯一，实际文件哈希一致。

## 4. 证据边界

- 英文标签颜色 / 朝向、Enter / 鼠标具体输入、P键盘继续、E / 左键 / 2 / 移动跳跃与无补播不卡键依据用户明确反馈；未据静态代码伪称独立操作。
- 当前保留日志没有菜单Quit requested，也没有按10.4顺序单独留下“菜单退出后再直接地图”的样本；退出按用户反馈，直接图登记另有00:01的实际记录。主要日志最后是新菜单开始→游戏→Stop / 清理，不改写为菜单Quit；没有矛盾，不要求重做旧矩阵。
- 失效ID是仅PIE开发命令注入，随后出生 / Retry / 提交走正式链路；**不是引擎真正TravelFailure**。重复ID / 缺失Entry有代码保护，本批未分别实测。
- 检查点只在当前进程有效，未实现磁盘SaveGame、奖励 / 遭遇恢复；跨独立进程退出再启动、当前Cook / 包与性能仍待相应阶段。本轮没有运行自动化或构建测试台。

## 5. 工时与提交交接

C **手动85 / 等待5分钟**，检查10分钟，在原80–130 / 检查10–15预算内；没有新Controller验收图。M1-06 A+B+C共**手动210 / 等待15分钟**；项目累计**手动1731分钟（28小时51分钟）/ 等待135分钟**。助手复核不计用户手动工时，不由单样本推断节省率。

主体十份Source / 三资产、C指南与B漏跟踪报告已提交**3351b533323214e4b050a2763135ce2bb300af66**，本机HEAD / 缓存origin/main一致；本轮未联网核验远端 / LFS。无需重复提交主体或旧AM骨架。本轮新增报告、来源清单和指南 / 状态文档更新由用户按[C末尾清单](../M1_06C_SessionCheckpoints.md#13-本批需要commit--push的内容固定放在文档最后)追加Commit / Push；Raw与缓存留本地。

本轮完成C验收与收尾；下一批D负责暂停迁移与闭环复盘，尚未下发，不追加开发或重复旧战斗 / HUD检查。
