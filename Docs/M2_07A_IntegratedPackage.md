# M2-07A：新版独立包与战斗集成检查

更新2026-10-25，UE **5.5.4 / Win64 Development**。**[本章验收通过](Reports/M2_07A_Verification.md)，当前进入[M2-07B必要性能窗口](M2_07B_PerformanceCapture.md)。**Attempt01新包、阶段23项与两轮运行已完成；用户手动30 / 等待10。下文保留实际操作供以后复查，本轮无需再次Build / Cook / 跑全组或重做流程。

**首次PIE创建等待已按用户决定关闭，不再优化、计时或列为阻塞项。**本章检查当前独立包的功能，不采Trace、不比较FPS，也不重新设计旧功能验收逻辑。

## 1. 本批交付、时间与阶段分工

旧Cleanup/Attempt01不含后来治疗次数HUD、Poise / F / 冷却HUD、锁定与方向切换，不能拿它验证当前代码。本批生成新包，把已完成能力 / UI / AI与世界重建串成一次短流程，并保留可定位的两轮运行日志。

预计手动 **25–40分钟**：准备 / 自动化5–10，包内检查10–15，保存 / 反馈5–10，少量排错缓冲；Build / Cook等待暂估15–45分钟，按实际另记。没有新Source，不需要先重复Editor Build、Game Build或Refresh-CLion；UAT会正常构建Game目标。

| M2-07批次 | 本批边界 | 进入条件 |
| --- | --- | --- |
| **A：本章已验收** | 一次23项阶段回归、新Development包、两轮独立运行与相关交战 / UI短查 | 已通过，手动30 / 等待10 |
| B：必要性能窗口 | A通过后，只采新版包的稳定待机与真实交战；分析新增能力 / UI / 镜头开销 | [B指南](M2_07B_PerformanceCapture.md)已下发；不测已关闭的PIE等待，不照搬M1全矩阵 |
| C：M2复盘与下一阶段校准 | 助手汇总架构 / 生命周期、真实Bug、已有证据与限制，整理简历候选素材和后续范围 | 证据齐备后编写；不是最终投递或最终面试准备 |

M2-07原7小时模块预算仍作上限规划比较，以上批次不另叠加到全项目预算，也不要求人为耗满。C分项实耗此前未报，当前仍保留“未登记”，不自动把估算记为实耗。

| 输出 | 本批位置 |
| --- | --- |
| 新包目录 | `E:\UE_Project\Demo\Artifacts\Builds\M2_07A\Attempt01` |
| 实际启动入口 | 上述目录下`Windows\EmberTrial.exe` |
| 打包控制台记录 | `Attempt01\UAT_Console.log` |
| Run01 / Run02独立日志与JSON | `E:\UE_Project\Demo\Artifacts\M2_07A\Runtime` |
| 本批验收报告 | [实际报告](Reports/M2_07A_Verification.md)已保存，含新包指纹 / 两轮证据及覆盖限制 |

下发时本地HEAD为`2b62e6a`，原C与镜头 / 出生补修已在该提交；本地main与origin/main跟踪引用一致，未联网核验远端。本轮只扩已有启动脚本的M2日志归档选项并写文档，实际玩法仍157 Source / 86cpp、Core60 / UI23 / Editor3、23项自动化、来源75。

## 2. 保存与阶段回归（约5–10分钟）

### 2.1 保存当前正式工程

1. Stop PIE，回`L_ET_VerticalSlice`，执行File → **Save All**。
2. 确认是正常单敌人场景，没有上一章临时SwitchProbe / 挡板。若未再改场景，沿用上一章已核对的19Actor / 单MeleeEnemy_A，不逐个重查全部Details。
3. 不修改IA、Montage、GE、Camera、血量、AI或碰撞。正式资源仍生命 / 体力100、治疗3次、轻攻击25、敌人10；镜头当前Pitch-18 / MaxShoulderYaw12。
4. 助手下发前已执行`Tools/Validate-ProjectConfig.ps1`，结果**PASS / Errors0 / Warnings0**。若你之后没有改共享Config，无需重复；改过才重新执行该脚本。

### 2.2 23项自动化只跑一次

**本次阶段全组已于2026-10-25 18:38:10北京时间23项Success，无需重跑。**下面保留原执行方法；全组不升级成后续每小章固定要求。

1. **非PIE**状态，在UE下方打开**Output Log**。若没有该页，用Window → Developer Tools → Output Log，或底部Output Log按钮。
2. 在Output Log底部选择**Cmd**输入栏，不是Python；点击输入栏，输入下面一行并按Enter：

   ```text
   Automation RunTests EmberTrial
   ```

3. 等队列执行完。本项目当前应有**23项**完成记录；正常为Success，不要求逐条复制日志或逐条输入23个命令。助手会从本轮完整日志提取总数和失败项。
4. 若有Fail、Crash或完成数量不符，**先停在这里**，发送第一条失败断言 / 崩溃信息、完成时分和日志路径；不要继续Cook一个已知测试失败的候选，也不删断言来通过。
5. 全部通过后，Save All并**关闭UE编辑器**。建议保存后也关闭CLion以留出内存，但不改IDE / 系统设置；PowerShell可以保留。后面的Cook会自行使用引擎命令行进程。

当前源码已完成正常编译 / 阶段全组；不要为“刷新”再删Intermediate、重建工程文件或重复全组。自动化通过也不代表真实窗口 / 输入 / Cook已经通过，包内只补下面的相关样本。

## 3. 用UAT生成新包（等待另计）

沿用项目已有Development / Pak / IoStore / 压缩 / Cook地图配置。本章用PowerShell直接调用UE5.5.4的**现有RunUAT**，让编辑器在打包期间保持关闭；不是改用CMake构建游戏。命令参数已对照本机AutomationTool源代码核对。

### 3.1 创建独立Attempt并执行

1. 打开PowerShell，完整执行下面代码块。变量名均为本章专用；本段不会删除旧包，也不会覆盖已存在的Attempt目录。

   ```powershell
   Set-Location 'E:\UE_Project\Demo'

   $m2AttemptPath = 'E:\UE_Project\Demo\Artifacts\Builds\M2_07A\Attempt01'
   if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
       throw '请先保存并关闭UE编辑器，再执行本段。'
   }
   if (Test-Path -LiteralPath $m2AttemptPath) {
       throw 'Attempt目录已存在；请改为Attempt02等新的同级目录，保留原结果。'
   }
   New-Item -ItemType Directory -Path $m2AttemptPath -Force | Out-Null

   $m2PackageArgs = @(
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
       "-archivedirectory=$m2AttemptPath"
   )
   & 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\RunUAT.bat' @m2PackageArgs |
       Tee-Object -FilePath (Join-Path $m2AttemptPath 'UAT_Console.log')
   $m2PackageExitCode = $LASTEXITCODE
   if ($m2PackageExitCode -ne 0) {
       throw "UAT失败，ExitCode=$m2PackageExitCode；保留UAT_Console.log，发送首条实际错误。"
   }
   if (!(Test-Path -LiteralPath (Join-Path $m2AttemptPath 'Windows\EmberTrial.exe'))) {
       throw 'UAT已结束但预期exe不存在；先发送实际输出目录，不要拿旧包代替。'
   }
   Write-Host "本轮包已生成：$m2AttemptPath\Windows\EmberTrial.exe"
   ```

2. 等待Build / Cook / Stage / Package / Archive完成。不要同时在CLion触发Build或重开UE；不使用`-skipcook` / 旧Cook数据代替本次Cook。
3. 看到`BUILD SUCCESSFUL`、AutomationTool ExitCode=0并有实际exe，才进入第4节。`UAT_Console.log`是打包输出，不是Runtime运行日志。
4. 如果本段开始就提示Attempt01已存在，只改第一行`$m2AttemptPath`为同级**Attempt02**，然后重新执行完整段；下方启动命令也改成同一实际Attempt。失败后保留旧Attempt，修好后用新Attempt，不清缓存作为默认修复。

`-nocompileeditor`只跳过Editor目标构建，`-build`仍会构建正式Game；不会把Editor校验器编进Runtime。当前MapsToCook沿用主菜单 / 正式地图和历史ThirdPersonMap，不同时裁剪资源 / 字体 / 插件。

### 3.2 核对包的位置

1. 打开实际Attempt目录，应有**Windows**子目录及其内`EmberTrial.exe`。不要再手动多套一层Windows。
2. 保留完整Windows目录，里面的Engine、EmberTrial与Pak / IoStore容器等一起使用；不能只复制根部启动exe。
3. 没有生成预期目录或出现“缺类 / 缺资产 / Cook错误”时，停在这里反馈首条错误，不启动旧Cleanup包凑结果。

助手会读取UAT / 工程Game receipt、实际包内Game二进制及容器指纹，将新包与当前输入对应；**根目录启动器的SHA本身不能证明玩法代码已更新**。这部分不用你手动计算。

## 4. 启动Run01并保留独立日志

`Tools/Run-StageBuild.ps1`已由助手增加`EvidenceStage=M2_07A`，其他历史默认和行为保持；无需手动修改脚本。脚本语法与WhatIf已检查，未启动旧包，也未伪造新包结果。

确认UE关闭后执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M2_07A\Attempt01' -EvidenceStage M2_07A -RunName Run01
```

如果实际是Attempt02，替换BuildDirectory，其他参数不改。**不要省略BuildDirectory**，脚本为兼容历史仍默认指向M1旧包。

本轮为1280×720窗口功能检查，不传Trace / PerformanceBaseline。控制台打印Executable / Log / Metadata；每轮有独立时间戳，日志和JSON保存在`Artifacts\M2_07A\Runtime`。JSON仅证明启动参数 / 路径，不证明玩法通过。

若在自己关闭游戏前窗口自动退出，保留该轮日志和Crash目录，报实际现象；不直接把它写成“退出正常”。

## 5. Run01：一条集成流程（约10–15分钟）

按顺序做即可，不需要掐精确时间点或读控制台里的成功长日志。以下只查已完成系统在**新包**中的结合，不复做斜墙 / 坡面 / 多帧率 / 所有取消点矩阵。

### 5.1 菜单、出生与四份资源显示

1. 首屏为正式主菜单，“继续本次试炼”应禁用。鼠标点击**开始新试炼**，从Entry出生。
2. 确认角色和占位动画正常、没有持续陷地；生命100 / 体力100 / 治疗3次 / F就绪，原本地状态面板只有一份。这里只观察明显异常，不计启动秒数。
3. 移动、转向、跳跃各一次。靠近原交互目标按E，确认中文提示更新，再P打开菜单，用**Enter或鼠标继续任选一种**恢复；输入正常、没有补动作。完整焦点 / 十次开关沿用前章，不重跑。

### 5.2 安全位置检查连段、闪避、破势与对应HUD

1. 在训练目标旁站定，按之前学过的节奏完成一次**左键1→2→3三段轻攻击**。不是按数字1 / 2 / 3；看三段动画衔接，训练目标应真实每段扣25。助手从日志读段Serial / GE，不要求新做敌人血条或用肉眼读精确数值。
2. 留出地面空间，按上方**2**一次：按移动输入方向闪避、体力只消费一次25，然后延迟恢复。松开再继续，不反复长按测旧边界。
3. 站在存活训练目标300cm内、动作已结束时按**F**一次，看到施放与冷却HUD；就绪后再按一次，观察训练目标一次破势反应 / 结束恢复。不要为此修改GA / Poise / AI；若已误杀训练目标，新试炼重置即可，只补本段。
4. 按P → 在菜单中按一次2 / Q / F → 用任一种方式继续，确认没有补执行，移动键不卡住。这个组合只做一次。

F主要削减Poise，不额外扣生命；Q暂留到真实受伤后再查，满血Q拒绝本来就是预期。训练目标属Neutral，不能Tab锁定，这不是包内锁定失效。

### 5.3 先激活检查点，再与正式敌人交战

1. 走进**Checkpoint_A**真实触发范围，确认ACTIVE / 检查点激活提示；E交互练习目标不等于激活检查点。后面的Retry / Continue才有可辨认出生点。
2. 从敌人可见的导航区域接近**MeleeEnemy_A**，保持一段距离，松开移动键，让它自行追击并挥击。按**Tab**锁定，看到单菱形与右肩镜头。若刚出生Tab没有目标，先接近，不扩大1200cm获取半径。
3. 当前正式地图只有一名Enemy，分别按Z、等约半秒再按C：都应**保留当前目标**，没有另一合法候选时不绕回 / 不丢锁。此处验证两个IA已Cook、输入抵达与空侧语义；**不声称验证了包内双敌人成功切换**。该正例沿用C的编辑器检查与Selection自动化，不临时给正式地图增第二敌人。
4. 有空间时靠近观察一次贴身构图：相机仍位于玩家后侧、适度俯视、能辨认敌人；不要求量出79cm或21°，也不关闭墙边SpringArm碰撞。若出现遮挡，记当时墙边 / 距离与5–10秒视频。
5. 让敌人真实命中一次，确认生命HUD下降。用2或移动拉开安全距离，**站定后按Q**：治疗在通知点后生效，恢复至上限，治疗次数3→2；体力、F冷却HUD互不代替。若被敌人打断、通知前未治疗且未扣次数是正常取消，拉开后重试本步即可。
6. 正式左键对敌人命中一次，观察真实受击，玩家生命HUD不显示敌人的生命。不要为了检查所有能力反复击杀 / 重开；训练目标和敌人各自独立的生命 / Poise日志由助手核对。

### 5.4 真实死亡、重试与会话重建

1. 保持敌人存活，让其真实攻击至玩家死亡，出现**一份Failure页**。不使用数字4开发伤害替代本次真实交战样本。
2. 点击**重试**，从Checkpoint_A出生：新生命100、体力100、治疗3、F就绪，单HUD、自由镜头 / 无旧锁。移动、转向各一次即可。
3. P → 返回主菜单 → **继续本次试炼**：仍从Checkpoint_A出生，新身体同样使用默认资源，没有旧锁 / 旧冷却面板。
4. E交互状态、训练目标 / 敌人的生命会随世界重建恢复，属于既定会话语义；只保留地图和检查点ID，不当作存档Bug。
5. P返回菜单 → **退出游戏**，确认游戏窗口 / 进程真正结束。日志和JSON保持原时间戳，不覆盖为固定文件名。

中途意外死亡也可以完成这一条路径；只补没看到的新包能力样本，不从头重做整条。失败时发送具体步骤、输入与现象，我读取同轮日志定位。

## 6. Run02：真正新进程（约1–2分钟）

Run01退出后，执行：

```powershell
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M2_07A\Attempt01' -EvidenceStage M2_07A -RunName Run02
```

1. 新进程首屏Continue重新禁用；开始新试炼从Entry出生，默认资源和单HUD正常。
2. 移动 / 转向、左键或2任选一个动作、P继续各一次。
3. 从菜单正常退出。不重做Run01战斗、检查点与所有能力。

两轮各有独立Runtime.log / JSON。若启动脚本仍报UE进程存在，先检查是否另一个工程UE还开着；不要删掉脚本保护来绕过。

## 7. 助手负责整理的证据

收到反馈后，我负责读取日志和产物，不要求你手动抽取伤害 / Serial / 时间戳：

- 一次23项阶段回归的实际完成数、失败 / Warning及对应Source时间，区别旧日志与本轮。
- UAT成功、Game目标 / receipt、实际包内Game二进制 / 容器、启动路径、两轮时间与JSON；不靠根启动器SHA断言更新成功。
- 轻攻击 / 闪避 / 治疗 / 破势的相关真实结算、五HUD / 标记装配、AI伤害、Failure / Retry / Continue / 新进程清理。
- Z / C应分别留下`Switch rejected Reason=NoCandidate Direction=Left / Right`的本轮记录，证明两个输入抵达Core；仅画面保持原锁还不足以证明按键已生效。由助手读取，不要求你停下来抄日志。
- 含Native Editor校验器的模块只在Editor目标；Runtime没有加载Editor校验器的证据，结合Game Target / receipt检查，不仅凭“没看到日志”。
- 用户可见输入 / 中文 / 取景与日志各自证明什么；单敌人包不冒充双敌人成功切换，未记录的环境参数也不补造。

有问题只修对应范围并复查，不追加旧整章矩阵。通过后生成本批验收报告，进入B的必要性能窗口；首次PIE等待需求已关闭，**不重新开题**。本章不是Shipping发布、最终招聘包或面试总复盘。

## 8. 最简反馈与报错方式

```text
M2-07A 完成 / 卡在第__节第__步
23项EmberTrial阶段回归：
实际Attempt / Build-Cook-Stage-Package / exe路径：
UE关闭、两轮确实使用新版包；Runtime目录（沿用默认可填沿用）：
菜单-Entry / 默认资源与单HUD / 无持续陷地：
左键三段真实伤害 / 2消费恢复 / F两次与破势 / P无补执行：
Tab单标记与后侧俯视 / Z-C空侧保持 / 真实敌人追击和伤害 / Q治疗扣次：
Checkpoint_A ACTIVE / 敌人致死 / Retry_A / Continue_A新资源无旧锁：
Run01正常退出 / Run02新进程Continue禁用与Entry短查：
失败或明显画面问题（无填无）：
完成时分 / 本批手动用时 / Build-Cook等待：
```

日志路径不知道可填“不清楚”，保留原目录，我先定位；不用复制长成功日志。失败发**第一条实际错误或断言的文字**及文件 / 行号；UI或资源设置问题发含资产名与字段的截图；镜头 / 输入 / 动画问题发5–10秒视频和操作顺序。UAT最后的exit code只作结果，实际原因在它上方第一条Error附近。

可选附上M2-06C实际手动 / 等待时间；未记就写“未记录”，不影响进入本章，不强迫补算。

## 9. 本章 Commit / Push（固定放在最后）

本地183b80c已包含A相关实现 / 工具与下发材料，本轮新增[实际验收报告](Reports/M2_07A_Verification.md)，并生成B指南和更新当前入口；Source / Content / Config保持，不重存资产或重复Stage未变文件。完整本批清单见[B末尾](M2_07B_PerformanceCapture.md#9-本批-commit--push固定放在最后)。

建议英文：`docs: record M2 package acceptance and prepare profiling`。

中文译文：`docs: 记录M2开发包验收并准备性能采集`。

CLion Refresh Changes → 重新Stage最新内容 → Review Diff → Commit → Push，由用户操作。Raw / 包 / UAT与Runtime日志 / JSON / 截图 / Trace / Saved / Binaries / Intermediate / CMake索引留本地；助手未Stage / Commit / Push或联网核验远端。
