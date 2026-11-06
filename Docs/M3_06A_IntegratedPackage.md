# M3-06A：当前遗物与设置闭环的新版独立包

2026-11-06，UE **5.5.4 / Win64 Development**。[C3音量卡已验收](Reports/M3_05C3_Verification.md)，无待补。本章生成包含当前M3代码 / 资源的新包。**本章已于23:35验收通过**：42项回归、Attempt01与两轮真实包内流程均完成，无待补。见[验收报告](Reports/M3_06A_Verification.md)，当前进入[M3-06B1代价型遗物](M3_06B1_TradeoffRelic.md)。以下原操作步骤保留供查阅，无需重跑。

## 1. 本章目的与范围

旧M2_07A/Attempt01不包含M3的遗物、祭坛和设置，不能代表当前工程。本章把**现有两件遗物、两槽装备、输入 / 显示 / 画质 / 音量设置**在真正Cook后的包内串起来，验证资源可达性、真实UI与跨世界 / 进程语义。

本章不新增Gameplay源码、地图或美术，不把两件遗物说成完整六件。原计划的体力恢复 / 破势效率 / 治疗提升 / 取舍型遗物、列表压力仍留后续；镜头震动选项等实际体验扩展随对应反馈系统接入，不加一个没有消费者的占位开关。这里是M3当前闭环的第一次包内检查，**不是整个M3完成、Shipping发布或最终招聘版本**。

实际233 Source / 133cpp（Core89 / UI41 / Editor3）、42项自动化、309 Content / 来源92。预计手动25–40分钟：准备 / 一次阶段回归5–10、包内检查15–20、记录5–10；Build / Cook等待暂估15–40分钟，按实际另记。

| 交付 | 本章位置 |
| --- | --- |
| 新包目录 | E:/UE_Project/Demo/Artifacts/Builds/M3_06A/Attempt01 |
| 启动exe | 上述目录下Windows/EmberTrial.exe |
| UAT控制台记录 | Attempt01/UAT_Console.log |
| 两轮运行日志 / 启动JSON | E:/UE_Project/Demo/Artifacts/M3_06A/Runtime |
| 专用个人设置 | 上述Runtime/M3PackageUser.ini，两轮共用 |

本地下发起点HEAD为`f363841`。助手已扩展已有Run-StageBuild.ps1的M3_06A归档 / 专用ini及目录保护，不需要你再改脚本；启动脚本不是构建脚本。配置静态检查已PASS / Errors0 / Warnings0；它不代替Cook或真实运行。

准备检查已通过：启动脚本及下文三段PowerShell语法、M3专用ini参数、缺少包目录时的拒绝、原M2性能参数兼容。`-WhatIf`未启动游戏、未创建Runtime文件；这不是新版包运行结果。

## 2. 保存与一次阶段回归

### 2.1 保存正式配置

1. Stop PIE，回正式`L_ET_VerticalSlice`，Save All。保持正式单敌人、祭坛RelicAltar_A、Entry / Checkpoint_A；本章不重搭场景或添加临时Actor。
2. 原默认基础生命 / 体力100、治疗3、F就绪，空槽轻击25 / 敌人10；地图未放临时装备效果或测试身体。若没有修改，沿用已有配置，不逐个重查Details。
3. 三张设置资产与两个宿主已于C3最终Valid；无需本批再重存所有未改资源。Source WAV和声音六资产保留原路由，DefaultGame.ini音频AlwaysCook / 两遗物PrimaryAsset规则保持。

### 2.2 42项仅在本次集成运行一次

**本轮已执行，并在第2.3节补修后42项全部Success；无需再次运行以下命令或重复Editor Build。** 以下保留原操作方法供以后阶段集成查阅。

1. UE保持**非Play**。打开Output Log，底部Cmd输入：

```text
Automation RunTests EmberTrial
```

2. 等本项目队列完成；当前应42项。只运行一条命令一次，不逐项复制成功日志，我会读取完整Editor日志核对本轮数量和结果。
3. 如果Fail、Crash或数量不符，停在这里，发送第一条实际失败断言、完成时分 / 日志路径；不要删断言或继续Cook失败候选。
4. 全部Success后Save All并关闭UE。CLion可关闭以留内存；不清缓存、删Intermediate或重复Editor / Game Build与索引。UAT后续会构建Game目标。

本次是自M2之后的阶段集成；不是以后每个小章都跑42项。真实键鼠 / 听感仍按下文补Cook相关样本，自动化不能代替它们。

### 2.3 受伤测试基线补修（已直接完成）

2026-11-06，用户报告BreakPulseAbility原第299行：预期施放后生命40，实际100。完整首次日志为**39 Success / 3 Fail**；另外两项StaggerAbility、Healing.Ability也依赖同一旧测试前提。

原因：三项测试在BeginPlay前设置InitialHealth=40，而M3装备组件会在新身体恢复最终装备后，按最终上限补满生命。空槽最终上限100，因此真正进入能力场景时已经100。这符合Retry / Continue的新身体规则；直接改断言为100会失去原来的受伤 / 治疗验证条件。

助手已直接修改以下三份既有测试，**不需要你复制代码**：

```text
EmberTrial/Source/EmberTrial/Tests/ETBreakPulseAbilityTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETStaggerAbilityTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETHealAbilityTests.cpp
```

- 移除初始化前的InitialHealth=40，等原有BeginPlay与落地准备完成，再通过ASC设置受伤生命40；每个场景先验证Current / Base均40，再触发待测能力。
- 保留“F不扣施放者生命40”“破势中伤害40→30”“Q真实通知治疗40→80 / 消费一次”等原断言，以及暂停、取消、死亡、旧回调、冷却检查；没有跳过场景、删除断言或禁用装备组件。
- 正式Gameplay实现、装备恢复逻辑、资产与共享Config均未修改。文件数量与自动化数量保持233 Source / 42项。

验证：UE关闭后正常Editor Development Build成功（约42秒），随后命令行无界面执行完整EmberTrial一次，**21:21完成：42 Success / 0 Fail / 0 Warning / 0 NotRun，退出码0**。三项修复用例分别于21:21:01、21:21:02、21:21:04成功。完整日志及JSON位于：

```text
E:/UE_Project/Demo/Artifacts/M3_06A/HealthFixtureFix/Before_Automation.log
E:/UE_Project/Demo/Artifacts/M3_06A/HealthFixtureFix/EditorBuild.log
E:/UE_Project/Demo/Artifacts/M3_06A/HealthFixtureFix/Automation.log
E:/UE_Project/Demo/Artifacts/M3_06A/HealthFixtureFix/AutomationReport/index.json
```

这是自动化修复结果，不代替包内中文、输入焦点或声音试听。助手构建 / 测试时间不计入你的手动实耗。接下来直接做第3节UAT；其-build会正常处理Game目标，不需要为这三份既有文件另刷CLion索引。

## 3. UAT正常Build / Cook / Stage / Package

### 3.1 选择新Attempt并执行整段

打开PowerShell整段执行。本段使用新目录；已存在就换Attempt02等，不删除 / 覆盖旧结果：

```powershell
Set-Location 'E:\UE_Project\Demo'
$m3AttemptPath = 'E:\UE_Project\Demo\Artifacts\Builds\M3_06A\Attempt01'
if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
    throw '请先保存并关闭UE编辑器。'
}
if (Test-Path -LiteralPath $m3AttemptPath) {
    throw 'Attempt目录已存在，请改为Attempt02等新目录，并同步修改后面的启动路径。'
}
New-Item -ItemType Directory -Path $m3AttemptPath -Force | Out-Null
$m3PackageArgs = @(
    'BuildCookRun'
    '-project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject'
    '-target=EmberTrial'
    '-platform=Win64'
    '-clientconfig=Development'
    '-unrealexe=UnrealEditor-Cmd.exe'
    '-installed'
    '-nop4'
    '-unattended'
    '-utf8output'
    '-nocompileeditor'
    '-build'
    '-cook'
    '-stage'
    '-pak'
    '-iostore'
    '-compressed'
    '-package'
    '-prereqs'
    '-nodebuginfo'
    '-archive'
    "-archivedirectory=$m3AttemptPath"
)
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\RunUAT.bat' @m3PackageArgs |
    Tee-Object -FilePath (Join-Path $m3AttemptPath 'UAT_Console.log')
$m3PackageExitCode = $LASTEXITCODE
if ($m3PackageExitCode -ne 0) {
    throw "UAT失败：$m3PackageExitCode。保留UAT_Console.log，发送首条实际错误。"
}
if (!(Test-Path -LiteralPath (Join-Path $m3AttemptPath 'Windows\EmberTrial.exe'))) {
    throw 'UAT已结束但预期exe不存在，先反馈实际输出路径，不使用旧包代替。'
}
Write-Host "本轮包：$m3AttemptPath\Windows\EmberTrial.exe"
```

1. 等Build / Cook / Stage / Package / Archive完成，期间不重开编辑器或让CLion同时Build。不要用skipcook替代本次Cook，也不改字体 / 插件 / 资源裁剪规则。
2. 成功需UAT ExitCode0 / BUILD SUCCESSFUL且存在实际Windows/EmberTrial.exe；失败的首条Error比末尾exit code有用，日志由我读取。
3. 保留完整Windows目录；根exe只是启动入口，不能单独复制它。助手会核对包内Game二进制和容器、receipt / UAT，不靠根启动器SHA断言代码已更新。

## 4. Run01启动与日志

UE关闭后执行，BuildDirectory必须显式指定：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M3_06A\Attempt01' -EvidenceStage M3_06A -RunName Run01
```

若实际Attempt02，改这里和第6节为同一个目录。不要运行UnrealEditor.exe -game代替包，也不要省略BuildDirectory；本脚本对M3要求明确目录。

脚本为每轮创建独立时间戳.log / .json，记录实际exe / 参数 / SHA，并自动让两轮使用同一`Artifacts/M3_06A/Runtime/M3PackageUser.ini`。该专用文件只保存个人设置，不是试炼存档；首次文件不存在属于正常情况，不复制C3旧ini来制造重启结果。

本章不传Trace / PerformanceBaseline、不比较FPS。JSON只证明启动路径 / 参数，不证明玩法通过；如果游戏自己提前退出，保留日志并报告，不能当正常退出。

## 5. Run01一条集成流程

### 5.1 首屏与当前设置（约3–5分钟）

1. 真正首屏应主菜单，新会话Continue禁用。原中文标题 / 按钮与右列四卡（输入、显示、画质、音量）可读，右列可滚动且焦点会跟随。**音量卡首次即自行可用**，无须先进试炼。
2. 输入卡读到灵敏度1 / 不反转；本章保持这个最终值，不再重跑输入偏好矩阵。画质卡明确选择**中 / 60 FPS / VSync关**并应用一次（已是该组则沿用）；显示卡只选一个不同的可用窗口尺寸预览，再取消恢复，检查确认 / 取消按钮可用。无须重做超时 / 独占全屏旧矩阵。
3. 音量卡先把音乐改0%，不应用时当前值应不变；点击应用。游戏控制台`~`执行`et.AudioProbe music`应静音、`et.AudioProbe effects`应能听到，确认声音软引用 / Cook和类路由在包内有效。
4. 仍用音量卡明确设**总50% / 音乐50% / 音效100%**并应用，作为两轮重启的保存组。收起控制台；没有新音乐循环是预期，本包仍用占位探针。
5. 原开始按钮 / Tab焦点可正常使用。开始新试炼从Entry出生，生命100 / 体力100 / 治疗3 / F就绪，单份原HUD，无旧锁定或持续陷地。

### 5.2 祭坛真实浏览与换装（约3–5分钟）

1. 接近**RelicAltar_A**，看到[E]提示，按E打开祭坛页面。界面应真实列出**生息余烬 / 砺锋残片两行**，中文、详情与槽A / B比较正常；选择 / 比较不自动改生命。
2. 在祭坛页选生息余烬，目标**槽A** → 点击**确认装备**。生命应**100/120**：上限上升不免费治疗。再选砺锋残片，目标**槽B** → 确认装备；面板显示两槽已装备。
3. 关闭祭坛页，到存活训练目标旁站定，左键一段真实命中，应基础25+遗物5=**30**，日志由我提取，不新加敌人血条或读数蓝图。
4. 普通P打开的遗物页应显示已装备，但确认装备 / 卸装不可提交；安全祭坛权限没有被普通暂停页面绕过。输入 / 设置卡仍能浏览。继续后移动转向正常。
5. 本次保留两件装备进入后续Retry / Continue，不在这里卸掉；真实卸装 / 重复装备拒绝旧矩阵已验证，本包先验证新增系统可达与结合。

### 5.3 战斗短样本、检查点与新身体（约5–8分钟）

1. 原上方2闪避一次，体力消费25后恢复；F施放一次显示冷却。P暂停 / 用鼠标或键盘一种继续后无补动作。左键 / 2 / Q / F完整取消点与帧率矩阵沿用M2，不重做。
2. 走进**Checkpoint_A**触发范围并确认ACTIVE，不用E练习目标代替检查点。让MeleeEnemy_A真实自行追击和挥击，Tab锁定看到单标记 / 后侧镜头，真实伤害更新玩家HUD。
3. 受伤后拉开安全距离，站定Q完成一次治疗，观察延迟治疗与次数3→2；被打断则只重试这一项。已装备生命遗物，上限是120，治疗应按真实当前值夹到120，不能仍按100判错。
4. 保持敌人存活，让敌人真实攻击致死，出现一份Failure页。点击Retry，应从Checkpoint_A出生；**装备保留、生命120/120、体力100/100、治疗3、F就绪、无旧锁**。这里新身体生命应120，不是原空槽100。
5. P返回菜单 → Continue，仍从Checkpoint_A出生并恢复同一两槽装备、新身体120；音量 / 画质 / 输入属于个人偏好，同样保持，旧页 / 标记无重影。
6. 正常回菜单退出。练习目标是否已激活、敌人生命等会随世界重建重置，符合当前会话语义；本包未实现磁盘试炼存档。

中途意外死亡只补没看到的相关样本，不要求整条从头重跑。普通P / 祭坛的区别、两槽装备与新身体生命，是本批集成重点。

### 5.4 Run01与锁定补测记录（完成）

2026-11-06，Attempt01 UAT日志显示BUILD SUCCESSFUL / ExitCode=0；已使用本批独立包的Run01窗口与专用ini。以下记录仅覆盖实际观察到的内容：

- 首次菜单Continue禁用；输入设置显示1.00倍 / 不反转。显示预览从1280×720切至1365×767，手动取消后回到1280×720。
- 画质通过界面应用中 / 60 FPS / 垂直同步关；音量草稿改音乐0时当前值仍100%，应用后显示音乐0，再应用总50% / 音乐50% / 音效100%。日志记录了实际应用请求。
- 新试炼从Entry进入，出生显示生命100/100、体力100/100、治疗3/3和破势就绪；普通P暂停页显示两条遗物、只读比较与无装备命令。P继续 / 再打开暂停通过。
- 用户协助走到祭坛并按E后，助手在实际祭坛页面为槽A装备生息余烬、槽B装备砺锋残片。界面显示两槽已装备，生命100/120，攻击预览25→30。
- 用户协助移动到训练目标并打开普通P后，界面显示两槽均已装备、仍为只读；日志显示Checkpoint_A已激活。助手继续游戏并点击左键，日志确认对训练目标真实造成30伤害，生命70→40。
- 用户协助战斗：2闪避体力100→75并恢复至100；F成功施放，目标韧性100→40；敌人实际追击并造成10伤害；Q在90/120时成功治疗至120/120，次数3→2。敌人最终击败玩家，显示一份失败页。
- 助手点击Retry，实际从Checkpoint_A出生，新身体120/120、体力100、治疗3、F就绪；暂停页显示两槽保留。返回菜单并Continue后再次从Checkpoint_A出生，日志确认两件遗物仍在对应槽位，新身体生命120/120。
- 包内试听：音乐0%、音效100%时，用户确认音乐探针无声、音效探针有声；日志显示两路探针均启动。随后助手将音量恢复为总50% / 音乐50% / 音效100%，并从菜单正常退出Run01。
- **Tab锁定补测通过：** Run01遗漏此项，另用同一新版包启动独立的`LockSupplement_20261106_232951_035`进程。用户靠近敌人并按Tab，确认单标记与镜头正常；日志先记一次无候选拒绝，随后23:30:24成功锁定`BP_ETMeleeEnemy_C_1`，标记投影启用。助手恢复游戏后也观察到一个标记和朝向敌人的视角。补测从菜单正常退出，日志保存在`Artifacts/M3_06A/Runtime/`，不冒充Run01。

第5节按上述实际操作与用户反馈完成。电脑操作接口无法持续按住移动键，移动、部分战斗和真实听感由用户协助；第6节Run02随后由用户完成并已核对，本章结果见验收报告。

## 6. Run02新进程只读（约2分钟）

Run01真正退出后执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M3_06A\Attempt01' -EvidenceStage M3_06A -RunName Run02
```

1. 主菜单Continue应重新禁用，音量卡首次自行可用、读50 / 50 / 100，画质卡读中 / 60 / VSync关。**先不点应用**，游戏控制台只查询`et.AudioSettings`与`et.GraphicsSettings`，由日志证明新进程读回；执行一次`et.AudioProbe effects`确认声音仍可用。
2. 开始新试炼从Entry出生，新GI两槽空、原100/100、体力100、治疗3。P遗物列表仍有两件初始拥有项，但已装备标记清除。**个人设置跨进程保留、试炼装备不跨进程保留**，两者不能混为存档故障。
3. 移动 / 转向、左键或2任选一次，P继续一次，再正常退出。不重跑第一轮战斗或再装备。

两轮完整Runtime日志 / JSON / 专用ini原样保留。我会读取，不要求你复制长成功日志或计算SHA。

### 6.1 Run02核对记录（完成）

实际Run02_20261106_233205_616：23:32:34音量只读0.5 / 0.5 / 1、RoutingApplied1，23:32:41画质只读1 / 60 / 0；之前无UI Apply。23:32:54 Entry / Owned2 / 空槽 / 100生命，音效探针启动与用户试听通过；最短输入 / P按用户反馈通过，23:33:11菜单正常退出。完整42项、UAT / Game二进制与两轮证据已核对，本章无待补。用户80分钟 / 等待10，详见验收报告。


## 7. 助手复核与阶段后续

我负责核对42项本轮结果、UAT / Game receipt / 包内二进制与容器、两轮日志、新包的库存 / 装备事务与真实伤害、中文 / 焦点 / 听感反馈边界。Editor校验器仅属于Editor目标，结合Game Target与实际包检查，不凭“日志没出现Editor”单独断言。

有错误只修对应问题并窄复验。通过后再按剩余M3需求扩展六种遗物与对应消费者、进行列表压力验证；之后依据实际变更更新包，不把A通过等同整个M3完工。正式模型 / 动画、完整配乐、镜头震动反馈及M5磁盘存档仍按原范围推进。

首次PIE等待需求已关闭，不重新计时或开题。没有性能采样数据，不拿本章功能包宣称达到1080p / 60 FPS或Shipping性能。

## 8. 最简反馈

```text
M3-06A 完成 / 卡在第__节第__步
42项EmberTrial阶段回归：
实际Attempt / Build-Cook-Stage-Package / exe路径：
UE关闭，两轮确实使用新版包；Runtime目录（沿用默认可填沿用）：
首屏中文四卡 / 音量自行就绪 / 画质应用与显示取消 / 真实试听：
祭坛E两遗物 / 两槽真实装备100-120与30 / 普通P只读：
2与F / AI真实伤害与Q / Checkpoint_A ACTIVE：
敌人致死Retry_A与菜单Continue_A / 两槽保留、新120与资源：
Run02新进程Continue禁用、设置只读0.5-0.5-1与1-60-0：
Run02空槽100 / Entry / 最短输入、两轮正常退出：
Source资产已保存、临时配置恢复或未改；失败或明显差异（无填无）：
完成时分 / 准备 / 检查 / 记录 / Build-Cook等待：
```

失败发第一条实际error / assertion及行号；UI发含资产名或可见布局的截图；输入 / 动画问题用5–10秒视频加操作顺序。UAT最后exit code只作结果，原因看前面的第一条Error。路径不清楚可以填不清楚，我先定位。

## 9. Commit / Push（固定放在最后）

本地c03948c已含三份受伤测试修复，9c0f9d5已含06A准备文档 / 启动工具。当前只Stage本指南收尾、[验收报告](Reports/M3_06A_Verification.md)、[M3-06B1指南](M3_06B1_TradeoffRelic.md)及实际更新的共享状态；准确十二文件清单见B1末尾。

- 英文：`docs: verify M3 package and prepare tradeoff relic`
- 专业中文：`docs: 完成M3开发包验收并准备代价型遗物`

本章无额外生产文件改动，不重复Stage旧源码或资产。包 / Runtime / UAT / JSON / 个人ini / 候选留本地，Git提交由用户操作，助手未核验远端。
