# M1-07A：当前阶段Windows独立包验收

复核：2026-10-13，UE **5.5.4**。**功能验收通过，无待补功能项。**用户13:47报告全部完成；实际手动30分钟（准备10 / 检查10 / 记录10），Build / Cook等待25分钟。直接证据已核对，下一批为[M1-07B Insights采样](../M1_07B_InsightsBaseline.md)。本轮没有重建、启动游戏、改Source / UE资产或操作Git索引 / Push。

## 1. 实际包与构建

- 启动exe：`E:\UE_Project\Demo\Artifacts\Builds\M1_07A\Attempt01\Windows\EmberTrial.exe`。实际Game exe位于该Windows目录的`EmberTrial\Binaries\Win64\EmberTrial.exe`，大小303,401,984字节，13:41:46生成；保留完整Windows目录。
- 三份启动JSON均指向上述包，bootstrap SHA256为`553A0D33C24699E77687D05A3CECF78CD41B73BC8B463D6BD3A18F247D35BE7D`，与现文件一致。bootstrap哈希单独不证明Cook资源；另记录实际Game exe与五个Pak / IoStore文件的哈希和大小，见[本地基线](Raw/M1_07A_Baseline.json)。
- 原UAT日志：`C:\Users\D1334\AppData\Roaming\Unreal Engine\AutomationTool\Logs\D+UE5+UE_5.5\Log.txt`。为避免下次打包覆盖，经只读授权保存[本地副本](Raw/M1_07A_UAT_2026-10-13/Log.txt)。本次13:39:25开始，命令为Win64 / EmberTrial / Development，包含build、cook、stage、package、archive至Attempt01；各阶段完成，最终BUILD SUCCESSFUL、ExitCode=0。
- 日志记录本次BuildCookRun为214.75秒，不能替代用户报告的整段构建等待25分钟；工时按用户反馈计，二者分别保留。
- 当前Game receipt为Win64 / Game / Development / UE5.5.4；运行日志也直接确认5.5.4与Development。`DefaultGame.ini`已保存Build=Always；两正式Cook地图与历史ThirdPersonMap保留。
- 相对D，69份Source / 34cpp、46项来源资产哈希全部不变，来源清单也不变；本批没有新增Gameplay / UI实现。资产版本、构建与实际运行相互支持，不以生成目录出现单独判定通过。

## 2. 独立运行证据

实际保留三次启动，两个Run01分别是补充试跑和完整流程，不能因RunName相同覆盖或合并：

| 日志文件（Artifacts/M1_07A/Runtime） | 直接记录 |
| --- | --- |
| Run01_20261013_134347_204.log | 初始Menu.Pawn=None / Continue=0；NewRun从Entry；两组正式攻击Begin / End、真实伤害、Checkpoint_A激活；菜单退出正常 |
| **Run01_20261013_134507_654.log** | 完整主证据：NewRun Entry、一次目标25伤害、五组Pause显示 / 移除、三次Resume.Paused0、Checkpoint_A激活、真实死亡 / Failure、Retry与Continue均从A出生、新100、正常退出 |
| **Run02_20261013_134647_616.log** | 新进程初始Continue=0；NewRun Entry / 新100、暂停返回菜单、真正退出；短基线具体操作采用用户反馈 |

完整Run01直接时序（北京时间；原日志UTC加8）：

- **13:45:51.781**：Checkpoint activated.Id=Checkpoint_A。
- **13:46:09.326**：只一份Failure shown；真实死亡提交与HUD归零在此前日志中。
- **13:46:18.772 → 18.836**：Retry请求Checkpoint_A → Gameplay world ready.Spawn=Checkpoint_A → Snapshot committed / Confirmed1；新身体Health100。
- **13:46:24.633 → 24.681**：返回菜单，Continue=1。
- **13:46:27.018 → 27.080**：Continue请求Checkpoint_A → Spawn=Checkpoint_A / Confirmed1；另一新身体Health100。
- **13:46:38.931**：日志正常关闭；Run02于13:46:47另起新进程，首菜单Continue=0，随后从Entry开始。

三次运行合计五组Flow ready / end、七组Pause shown / removed、一组Failure shown / removed、三组Session ready / end、三组玩家攻击Begin / End，均闭合。每次均有菜单退出命令与Log file closed，非强杀退出。主证据与Run02启动参数均1280×720、TraceEnabled=False；这些是功能检查条件，不是1080p性能基线。

三次启动的[摘录](Raw/M1_07A_Runtime_2026-10-13.log)及JSON / 原日志留本地。原记录实际包含三次，全部保留。本轮没抓取屏幕或重新操作；中文、单UI、动画、左键身份、2 / E、鼠标 / Enter / Space输入和无补播不卡键根据用户实机报告。Run02没有攻击Begin记录，不把该日志写成第二进程新攻击实测；同包正式攻击有Run01直接记录，短基线采用用户反馈，不再重复整套验收。

## 3. 警告与证据边界

三份Runtime未发现Error、Fatal、Assertion、Ensure、Accessed None或Blueprint Runtime Error；没有BindWidget / Native Tick警告。每次首次ASC使用均有一条**GameplayCueNotifyPaths未配置，退回扫描/Game**的Warning，Cook也出现同类警告。UE5.5.4的AbilitySystemGlobals.cpp明确执行AddGameplayCueNotifyPath('/Game')后继续初始化；本包无GameplayCue配置业务，真实GE伤害、受击 / 死亡均正常，因此该警告不阻断A功能验收。

扫描范围是后续资源增大时的启动成本事项，登记给M1-07C有限配置收口；B保留原包先测，不为消除警告提前改Config重打包，也不宣称已测到性能瓶颈。Cook另有ShaderCompileWorker JobGroup完成端口Warning，最终Cook ExitCode0、资源运行正常，保留原记录，无Shader编译失败证据。

本机独立进程已测，另一台电脑兼容性、Shipping、最终死亡表现、持久世界进度及真正TravelFailure仍未本批实测。本包是Development灰盒基线，M1全部收口仍需B实际Trace分析与C有限校验。

## 4. 工时与后续

本批30 / 25分钟，项目累计手动**1866分钟（31小时6分钟）** / 等待**165分钟**。M1-07目前A完成，B待用户采集、C待开始；预计时间不计为实际工时。采用方案B，一条包内流程与一次新进程检查，没有临时蓝图、重复换Pawn或命中矩阵。

本地HEAD为193eb4e，D统一暂停 / 失败实现已进入该提交；仅查看本地记录，未联网核验远端。接下来沿用本包，启动工具新增运行期采样条件 / B输出目录；工具语法和WhatIf检查不等于已完成Trace采样。

## 5. 本批Commit / Push

提交本报告、A指南 / M1-07路线与Progress / Backlog / Architecture / Introduction / VersionControl等实际状态差异。DefaultGame.ini的Build=Always已保存，只有出现未提交差异才加入，不重存无变化资产。B新增指南与Tools修改按[B末尾清单](../M1_07B_InsightsBaseline.md#10-本批需要commit--push固定放在文档最后)提交；Git由用户操作。

Raw、UAT副本、Runtime日志 / JSON、包和后续utrace / CSV留本地，沿用.gitignore，不Force Add。当前没有新UE资产来源，AssetManifest不为本批重复改哈希。

2026-10-13后续B补采发现：Attempt01仍能真实追击 / 伤害 / 死亡，但Controller Stop90而Move Radius150，源码默认绑定顺序回归已定位。A原验收当时没有保留90cm到达参数证据，不将原“无待补”扩展为此新问题已查。旧包保留历史基线；B8.2手动补修 / PIE后，C收口统一生成Attempt02并核对包内自主接近 / 连续攻击。当前不再宣传Attempt01为已修复90cm参数版本。
