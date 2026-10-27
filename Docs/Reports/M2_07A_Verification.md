# M2-07A：新版独立包与战斗集成验收

2026-10-25，UE **5.5.4 / Win64 Development**。用户18:54报告全项完成、无失败或明显画面问题，手动30 / Build-Cook等待10分钟。**A验收通过，无阻塞补项，进入[M2-07B必要性能窗口](../M2_07B_PerformanceCapture.md)。**本轮助手只读核对产物 / 日志并写收尾和下一批材料，没有重打包或重复实机。

## 1. 当前输入、构建与实际包

本地HEAD为`183b80c`，工作区在本轮审查开始时干净；本地main与origin/main跟踪引用一致，本轮未联网核验远端。实际157 Source / 86cpp、Core60 / UI23 / Editor3，23项自动化，来源75 / 本地294份UE资产；75条清单SHA均与实物一致。

| 材料 | 实际位置 / 结果 |
| --- | --- |
| 新包 | `Artifacts/Builds/M2_07A/Attempt01/Windows` |
| 启动入口 | 该目录的`EmberTrial.exe`，168,448字节 |
| 实际Game二进制 | `Windows/EmberTrial/Binaries/Win64/EmberTrial.exe`，304,082,944字节；18:46:40生成，SHA与工程Binaries内Game exe相同，晚于当前Source |
| UAT记录 | `Artifacts/Builds/M2_07A/Attempt01/UAT_Console.log`；Build / Cook / Stage / Package / Archive均完成，BUILD SUCCESSFUL、ExitCode0 |
| UAT时间口径 | BuildCookRun工具段117.75秒、AutomationTool约1分58秒；用户另记等待10分钟，包含其实际操作等待，不用工具段替换用户记录 |
| 工程receipt | `EmberTrial/Binaries/Win64/EmberTrial.target`，Game / Win64 / Development / UE5.5.4；未在归档包内找到独立.target，以工程receipt、Game exe指纹及UAT / Runtime共同关联 |
| 指纹范围 | 根启动器、实际Game exe、5份Pak / IoStore容器共7文件；保存在本地Raw，不代表逐一验证全部第三方DLL |

包内根启动器的SHA不是玩法更新依据。当前Game Target仅装配Core / UI，Editor模块为Editor类型；receipt产物未含EmberTrialEditor，Runtime有UI正常加载、没有本项目Editor校验器加载记录。结合构建目标确认模块边界，不仅凭“未见一条日志”。

**2026-10-25 18:38:10北京时间**，本轮阶段全组为23成功 / 0警告 / 0失败 / 0未执行，进程ExitCode0；当次JSON位于本地`Docs/Reports/Raw/M2_07A_StaminaRecoveryFix_2026-10-25/Automation/index.json`。不要求用户再次重跑，正常Editor编译与后续UAT Game编译已覆盖当前源码。

## 2. 两轮独立运行与功能证据

两轮均明确启动Attempt01，脚本JSON记录1280×720窗口请求、Trace=False、PerformanceBaseline=False、EvidenceStage=M2_07A；本轮不是性能采样，也未独立量取窗口实际渲染尺寸。UE关闭、中文 / 焦点 / 镜头观感及无持续陷地按用户实机反馈接受。

- Run01：`Artifacts/M2_07A/Runtime/Run01_20261025_184929_046.log`与同名JSON；18:49:29启动、18:52:52正常退出。
- Run02：同目录`Run02_20261025_185301_613.log`与同名JSON；18:53:01启动、18:53:17正常退出。

下列时间均已由UTC日志转为北京时间：

| 功能 | 本轮直接证据 |
| --- | --- |
| 菜单 / Entry / 默认资源 | 两个新进程均Menu Continue=0；NewRun确认Entry，生命 / 体力100、治疗3、F就绪；每个新身体独立Presenter / View |
| 出生 | 包内实际执行Spawn grounded，StartZ120→PawnZ92.15；用户确认没有持续陷地，不把它扩写成包内逐帧骨骼采样 |
| 三段轻攻击 | Run01 18:49:50–51，玩家Stage1 / 2 / 3独立TraceSerial；训练目标100→75→50→25，三次各25 |
| 正式闪避 / 体力 | 18:50:06起真实位移约150.05cm / SourceTime0.3001秒，体力一次100→75、延迟后恢复；后续动作 / 暂停边界按用户检查 |
| F / Poise / HUD | 18:50:26与35两次Pulse，训练目标100→40→0；一次Stagger接受，正式冷却 / 恢复和显示按日志与用户可见反馈共同接受 |
| 真实敌人与Q | 敌人真实10点伤害 / 玩家HUD变化；18:51:46 Healing applied，Amount配置40、Charges2/3；不过度声称在满值夹限时实际也增加40 |
| 锁定与两个IA | 18:51:13锁正式Enemy；18:51:27 Left、28 Right各一次NoCandidate，均指向同一当前目标；证明两个输入抵达Core及空侧保持，不冒充包内双敌人成功切换 |
| 检查点 | 18:50:53真实Checkpoint_A激活，与E练习交互区别明确 |
| 死亡 / Retry / Continue | 玩家真实伤害归零后Failure；18:52:39 Retry确认Checkpoint_A、44 Continue确认Checkpoint_A；新身体默认资源、旧源 / View停订阅并清理 |
| 新进程与退出 | Run02重新Continue=0 / Entry，正式2与P继续有记录；两轮均菜单Quit requested后正常LogExit，无崩溃退出 |

两轮完整Runtime未发现`Log…: Error / Warning / Fatal`、Ensure或Accessed None；`r.PrecomputedVisibilityWarning:0`只是CVar名称，不误计为Warning。没有把成功日志要求用户逐条抄回；相关原句和哈希收录本地`Docs/Reports/Raw/M2_07A_Review_2026-10-25/Evidence.json`。

## 3. 覆盖边界与工时

本批覆盖当前新包的相关串联流程；精确成本 / 取消 / 旧订阅生命周期由23项自动化及前章边界共同支持。没有重复全部坡面 / 多帧率 / 多Pawn / 全DPI矩阵，没有新增包内双敌人正例、Shipping、另一台设备或最终美术场景的验证。首次PIE创建等待已按用户要求关闭，不追加调查。

本批用户手动30 / 等待10：M2-07累计30 / 10，M2已量化1370 / 129分钟，项目已量化3381分钟（56小时21分）/ 明确等待314分钟（5小时14分），另有M2-06C未登记部分；不补造该部分，也不把助手审查 / 工具运行计为用户实耗。当前最新功能基线改为M2_07A/Attempt01，旧Cleanup包只留历史证据。

## 4. Commit / Push（固定放在最后）

本地183b80c已包含A实现 / 工具与下发材料；本轮新增本报告、M2-07B采集指南及当前入口更新，脚本仅扩展M2性能日志路由与Medium采样条件。精确本批范围见[B末尾](../M2_07B_PerformanceCapture.md#9-本批-commit--push固定放在最后)。

建议英文：`docs: record M2 package acceptance and prepare profiling`。

中文译文：`docs: 记录M2开发包验收并准备性能采集`。

不重新Stage未变Source / Content / Config；Raw / 包 / Runtime / JSON / 截图 / Trace留本地。Git由用户操作，助手未Stage / Commit / Push或联网核验远端。
