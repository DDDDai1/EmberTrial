# M1-07C3：Attempt02新版包与M1阶段收口

更新2026-10-14，固定 UE **5.5.4**。**[C3验收](Reports/M1_07C3_Verification.md)已通过**：Attempt02打包成功，两轮Runtime完整；实际手动10 / 等待10分钟。[M1-07总验收](Reports/M1_07_Verification.md)、[M1总验收](Reports/M1_Verification.md)、[阶段复盘](M1_Review.md)与[M2路线](M2_Roadmap.md)已完成。本指南保留实施步骤供以后复查，本轮无需重复操作。

## 当前维护说明（第27批）

Run-StageBuild脚本补充路径 / 性能口径 / ShouldProcess说明；WhatIf仍在创建目录、写证据、启动游戏之前返回。本批对现有Attempt02运行WhatIf，确认无新证据 / 游戏进程；没有实际启动包或重做C3验收。旧包只代表M1，最新源码含M2，记录见[第27批](Cleanup.md#cleanup-batch-27)。


## 1. 本批目的、时间与输出

将B的MoveRadius90绑定修复、最终短日志与C2 Cue配置实际带进新包，并确认两Editor校验器不作为Runtime模块加载。原Attempt01与B Insights保留历史测量，不拿旧exe验证新Source / Config，也不重新采Trace证明尚未测得的优化收益。

预计手动 **20–35分钟**：准备5–10、包内检查8–12、记录 / 保存5–10；Build / Cook等待约20–45分钟另记（A实际25分钟）。不搭临时蓝图、不新增负例，不重跑换身体 / 多目标 / 坡面 / FPS矩阵。

| 文件 / 目录 | 本批位置 |
| --- | --- |
| 新打包目录 | `E:\UE_Project\Demo\Artifacts\Builds\M1_07C3\Attempt02` |
| 实际启动exe | 上述目录下 `Windows\EmberTrial.exe` |
| 独立运行日志与JSON | `E:\UE_Project\Demo\Artifacts\M1_07C3\Runtime` |
| 已生成的收尾材料 | C3 / M1-07 / M1验收、M1复盘、实际工时与M2路线校准，见顶部链接 |

新版包只作当前阶段交付，不是最终招聘作品。菜单 / 会话、战斗与AI、事件HUD、性能分析和编辑器工具的证据可形成M1闭环；最终美术、战斗深化、存档世界进度、最终作品 / 面试准备继续按Introduction推进。

## 2. 保存与检查（约5分钟）

1. 停止PIE，File → **Save All**。确认九个C2正式资产、两合法Probe与地图无未保存标记；不再清空Probe字段，也不把Probe配置到正式角色 / Controller。
2. PowerShell执行：

   ```powershell
   Set-Location 'E:\UE_Project\Demo'
   .\Tools\Validate-ProjectConfig.ps1
   ```

   预期Config PASS / Errors0 / Warnings0；这是已通过配置的打包前快速检查，不要求再次点九个资产校验或跑已有自动化。
3. UE **Edit → Project Settings → Project → Packaging**，核对沿用A已通过值：Build=Always、Build Configuration=Development、Full Rebuild=True、For Distribution=False、Use Pak / Io Store=True、Cook everything=False。两正式MapsToCook保留，历史ThirdPersonMap也保留。本批不同时裁剪字体 / 插件 / 原资源。
4. 在Maps & Modes确认 **Game Default Map=L_ET_MainMenu**；Editor Startup Map仍L_ET_VerticalSlice。两个地图的Override沿用已通过配置，不为本批改GameMode / Pawn。
5. 保存后暂停修改Source / 资产。CLion可以开着，但打包期间不同时触发Build或Live Coding。**不用先额外执行一次Game Build**，下一节UAT会执行Build / Cook。

本轮代码已正常构建且无Runtime源码新改，新的Cook仍必要：旧包没有最新半径绑定实现与Cue配置。Editor模块Type / Target边界保持C1 / C2，不临时删除Editor源码来打包。

## 3. 生成Attempt02（等待另计）

1. UE顶部 **Platforms → Windows**，Binary Configuration选 **Development**。若显示Build Target，选 **EmberTrial**（Game）；不是EmberTrialEditor。
2. 选择 **Platforms → Windows → Package Project**。
3. 输出文件夹选择：

   ```text
   E:\UE_Project\Demo\Artifacts\Builds\M1_07C3\Attempt02
   ```

   当前该目录尚不存在，可在文件夹选择窗口新建后选择。UE会在里面生成Windows子目录；不用再自行多套一层Windows。
4. 等待 **Build / Cook / Stage / Package / Archive** 完成。通知成功后，确认 `Attempt02\Windows\EmberTrial.exe` 存在。保留完整Windows目录，不能只搬启动exe。
5. 若失败，保留Attempt02与完整UAT日志，发首条实际error附近20–40行和日志路径；修复后用同级 **Attempt03** 新目录。不要删除旧Attempt01，不清引擎 / 项目缓存作为默认修复。
6. 在Output Log用 `AutomationTool` / `BUILD SUCCESSFUL` / `AutomationTool exiting with ExitCode=0` 查完成记录。UAT常见日志目录：`C:\Users\D1334\AppData\Roaming\Unreal Engine\AutomationTool\Logs\D+UE5+UE_5.5`；若界面显示其他路径，以实际值为准。你可只反馈打包完成时分与exe路径，我会先自行定位日志。
7. 打包成功后Save All，**关闭UE编辑器**。CLion可以保留，独立检查期间不用打开Insights或另一个游戏实例。

打包成功本身不代表新包行为已通过，下面需真正从新版exe启动。

## 4. 启动新包并留下独立材料

助手已给 `Tools/Run-StageBuild.ps1` 添加可选 **EvidenceStage**，A / B原命令兼容。按下面命令明确选择新版目录和C3输出位置；不要使用省略BuildDirectory的旧A启动命令。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M1_07C3\Attempt02' -EvidenceStage M1_07C3 -RunName Run01
```

脚本先显示Executable / Log路径，启动1280×720窗口并写同名JSON / Runtime.log。**这是功能检查条件，不做B的1920×1080性能对比**，不传Trace / PerformanceBaseline。

若使用Attempt03，以上BuildDirectory替换成实际目录即可；EvidenceStage仍M1_07C3。脚本正常启动要求UE关闭；`-WhatIf`仅打印路径且不启动进程，可在UE仍开着时预览。助手已用历史包路径进行dry-run验证C3输出路由，没有启动旧包或生成Runtime假结果。

## 5. Run01：相关包内短流程（约5–8分钟）

### 5.1 新进程与基础资源

1. 首屏必须是正式主菜单，“继续本次试炼”禁用。点击 **开始新试炼**，从Entry出生，HUD100、角色 / 场景正常。
2. 移动、转向、跳跃各一次。站定后**左键攻击一次**训练目标，确认动画和真实25点伤害；按 **2** 闪避一次。这里用正式左键，不用1代替左键通过。
3. 走近M0交互目标，**E一次**确认中文提示更新；按 **P** 打开暂停，再用 **Enter或鼠标继续择一**恢复。确认输入立即恢复。原A已验完整焦点矩阵，本轮不重复十次开关 / 每种继续方式。
4. 走入 **Checkpoint_A** 触发范围，确认状态ACTIVE / 激活提示。E激活M1_Target_A不等于检查点激活。这样后面的重试有可辨认出生点。

### 5.2 新包真实AI：关键更新边界

1. 从敌人视线内可走位置接近 **MeleeEnemy_A**，需要与它相隔一段距离，让它真正追击；不要一开始就贴身。走入视线后 **松开所有移动键，站定**。
2. 敌人应自行走近、停止并连续两次挥击，HUD **100→90→80**。关键是**你站定，它主动完成接近与攻击**；不再次走过去帮助它进入攻击范围。若先前练习造成玩家扣血，先开始新试炼恢复100，**重新激活Checkpoint_A**，再进入敌人视线测这段；新试炼会重置检查点，不能沿用上一轮的激活。
3. 玩家不移动，让敌人继续真实攻击至死亡，出现一次Failure页；鼠标点击 **重试**。应回Checkpoint_A、新身体HUD100，移动 / 转向可用。无需用4人工制造死亡，也不再搭测试时间点。
4. 日志中的实际Move Radius、伤害、死亡 / Retry由我读取；你只需描述上述可见行为正常 / 异常，不在测试中反复开控制台找精确毫秒。

### 5.3 菜单与会话仍可用

1. 重试后 **P → 返回主菜单**，Continue应可用。点击 **继续本次试炼**，仍从Checkpoint_A出生，新身体HUD100。
2. 这次切图会重建敌人 / 练习交互；会话保存地图 / 检查点ID，**不保存世界战斗 / 交互状态**，与既定M1语义一致。
3. P返回菜单 → **退出游戏**，确认进程真正退出；保留控制台给出的Log / JSON，不复制覆盖为同一个固定文件名。

这是一条新版包相关流程，复用正式功能；若有具体失败，只补对应步骤，不重做C1 / C2校验矩阵。

## 6. Run02：真正新进程（约1–2分钟）

Run01真正退出后执行：

```powershell
.\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M1_07C3\Attempt02' -EvidenceStage M1_07C3 -RunName Run02
```

首屏Continue重新禁用；开始新试炼从Entry出生 / HUD100，移动转向正常。站定左键一次、P返回菜单并退出即可；不重做Run01战斗 / Checkpoint链路。同名RunName也有独立时间戳，两个完整Runtime.log / JSON会保留在C3目录。

## 7. 我负责读取的证据与阶段复盘

以下证据已核查，详细结果见[C3报告](Reports/M1_07C3_Verification.md)：

- 最新Source / Config / 清单与打包时间、UAT成功、工程Game receipt及包内实际Game exe / Pak指纹。本次包内没有独立归档的.target receipt，采用工程receipt、Game exe哈希与UAT / Runtime共同关联。
- 包内 **Move request Radius90**，真实敌人10点伤害 / HUD同步、自主接近按你的站定反馈；Result=Failed可能是NearTarget主动停止，不要求人为改成Succeeded。
- GameplayCueNotifyPaths回退到/Game的Warning是否消失；Core / UI正常、没有Editor validation module ready / Validator运行记录。没有Warning不等于已测性能收益。
- 一次Checkpoint_A激活、真实死亡 / Retry、Menu / Continue、新进程Entry及退出清理；区别日志直接证明与键鼠 / 中文等你的实机观察。
- 更新C3验收、M1-07总报告与M1阶段复盘：对象 / 模块分工、数据流、真实Bug案例、现有证据 / 覆盖限制、工具成本与M2范围 / 排期。**这些整理由我完成，你不需要再建验收蓝图或填写另一套旧功能表。**

M1总复盘已随C3通过完成。B性能数值仍属于历史Attempt01，本次没有新Trace；Blender、美术表现、死亡动画、战斗深化、最终存档 / 招聘作品与面试安排按后续里程碑推进。

## 8. 最简反馈与失败材料

```text
M1-07C3 完成 / 卡在第__节第__步
Attempt02（或实际Attempt）Build / Cook / Stage / Package：
实际exe路径；UAT日志路径（不清楚可填不清楚）：
UE关闭后从新版包独立启动；菜单 / Entry / HUD100：
左键真实25 / 2 / E中文 / P一种继续方式：
Checkpoint_A ACTIVE；玩家站定→敌人自行接近，HUD100→90→80：
敌人真实致死 / Retry_A / P返回菜单 / Continue_A：
Run01退出 / Run02新进程Continue禁用与Entry短检查：
Runtime两轮日志目录：
已保存；失败或特别说明（无则填无）：
完成时分 / 手动用时 / Build-Cook等待：
```

Build / Cook失败发首个实际错误与日志路径；包内异常发发生步骤、前后可见状态、局部截图或5–10秒视频，日志由我读取。敌人停下不攻击时，明确说是否玩家保持站定，不以“伤害正常”替代这项条件。这里不要求你掌握调试时间点。

## 9. 本批需要 Commit / Push（固定放在文档最后）

第27–29批一起提交，完整文件清单和建议Commit信息见[Cleanup末尾](Cleanup.md#本批-commit--push固定放在最后)。本页原实施已完成，不重复Stage历史配置 / 资产；Raw / 日志 / 生成文件留本地，助手不操作Git索引。
