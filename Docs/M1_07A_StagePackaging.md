# M1-07A：当前阶段Windows Development包

下发：2026-10-13，UE **5.5.4**。前置[M1-06功能总验收](Reports/M1_06_Verification.md)通过。**当前仅下发操作指南与启动工具，尚未生成或验收M1包。**本批无C++ / Blueprint新功能；你手动核对共享配置、打包、独立运行和Push。出现报错给完整日志路径或首条错误，我来定位。

## 1. 本批成果与时间

拿到当前菜单 / GAS战斗 / AI / HUD / 暂停 / 失败 / 会话检查点的**Win64 Development独立包**，UE编辑器关闭后真实运行。历史M0包只包含早期原型，不用于本批验收。

预计手动30–50分钟：准备 / 设置10–15、启动 / 操作10–15、记录 / 恢复5–10；Build / Cook等待另计约20–45分钟，参考M0实际26分钟，本次取真实用时。B再基于这份包采集Insights，A不要求你同时做性能测量、录长视频或写新验收图。

输出目录：`E:\UE_Project\Demo\Artifacts\Builds\M1_07A\Attempt01`。若失败，保留该目录与日志，修复后用Attempt02；不清缓存、不覆盖M0包。本批启动工具[Run-StageBuild.ps1](../Tools/Run-StageBuild.ps1)已写好，仅启动包并记录独立日志 / 元数据，不修改工程、配置或Git，不自动判定玩法通过。

## 2. 准备与启动 / Cook配置（约5–10分钟）

1. UE打开`E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`，停止PIE；确认5.5.4。本批保留CLion代码编辑器与现有MSVC / Windows SDK。
2. 确认M1-06D最后补修已Compile / Save，包括左键`IA_ET_LightAttack`后的Condition确实接Is Game Paused；不用重建临时Controller测试函数。File → Save All；地图标签和改过的BP无星号。
3. Edit → Project Settings → **Project → Maps & Modes**。Game Default Map=`L_ET_MainMenu`，Editor Startup Map=`L_ET_VerticalSlice`。**当前本地配置已正确，核对即可。**两者的资源路径分别是`/Game/EmberTrial/Maps/L_ET_MainMenu`与`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。
4. 世界各自的GameMode Override沿用A / B / C已通过配置：菜单BP_ETMenuGameMode、正式图BP_ETGameMode；本批不更改全局GameMode或新建地图。
5. Project Settings左侧选**Project → Packaging**，每次检查后清空搜索框，再搜索下一项：

| 字段 | 本批值 |
| --- | --- |
| Build | **Always**（本次明确让UAT执行Game构建；当前IfProjectHasCode可改为Always） |
| Build Configuration | **Development** |
| Full Rebuild | True，当前已有 |
| For Distribution | False |
| Use Pak File / Use Io Store | True / True |
| Use Zen Server as cooked output store | False |
| Cook everything in the project content directory | False |
| Include Prerequisites Installer | True |

6. 搜索`List of maps`，展开**List of maps to include in a packaged build**。当前已有三项，保持：

   ```text
   /Game/ThirdPerson/Maps/ThirdPersonMap
   /Game/EmberTrial/Maps/L_ET_MainMenu
   /Game/EmberTrial/Maps/L_ET_VerticalSlice
   ```

   不必删除历史ThirdPersonMap；两张正式流程地图必须都在列表中。File Path用`/Game/...`包路径，不带磁盘盘符、Content前缀或`.umap`后缀。若原列表一致，不重复添加。
7. 原`Directories to Always Cook`、国际化 / 字体、Compression、编辑器内容排除等保持已有配置；本批不同时精简资源，先验证中文和资源在当前包中齐全。关闭Project Settings，File → Save All。
8. 打包期间暂停修改Source / BP / 地图，CLion可保留但不同时触发Build或Live Coding；不用先额外跑一次相同Game Build，下一节UAT会执行。

## 3. 在UE执行本次打包

1. 顶部**Platforms → Windows**，Binary Configuration选**Development**；若有Build Target列表选**EmberTrial**，单Game目标不显示该列表正常。
2. 选择**Platforms → Windows → Package Project**。
3. 输出文件夹选：

   ```text
   E:\UE_Project\Demo\Artifacts\Builds\M1_07A\Attempt01
   ```

4. 等待任务完成，通过Show Log查看Build / Cook / Stage / Package进度；期间不修改工程。成功要同时看到Packaging Complete、AutomationTool **ExitCode=0 / BUILD SUCCESSFUL**及实际输出文件，不能只根据中途Compile成功或目录出现判断。
5. 正常结构应包含：

   ```text
   Attempt01\Windows\EmberTrial.exe
   Attempt01\Windows\EmberTrial\Binaries\Win64\...
   Attempt01\Windows\EmberTrial\Content\Paks\... .pak / .utoc / .ucas
   Attempt01\Windows\Engine\...
   ```

   实际生成目录若不同，给我完整exe路径。本次保留**整个Windows文件夹**，运行 / 后续分享不能只拷一个exe。
6. 保留本次UAT日志。以Output Log打印的**实际Log file路径**为准；也可在`%APPDATA%\Unreal Engine\AutomationTool\Logs`查本次日志。只需给路径，我可自行提取；不要求你抄整份输出。Save All后**关闭EmberTrial UE编辑器**。

## 4. 第一次独立启动与日志

1. 在PowerShell执行以下命令。命令由你手动执行，脚本已在Tools下，不需要再逐行输入脚本源码：

   ```powershell
   Set-Location 'E:\UE_Project\Demo'
   .\Tools\Run-StageBuild.ps1 -RunName Run01
   ```

2. 脚本默认打开Attempt01中的`Windows\EmberTrial.exe`，1280×720窗口，并打印Executable / Log / Metadata路径。运行日志和JSON位于：

   ```text
   E:\UE_Project\Demo\Artifacts\M1_07A\Runtime\Run01_时间戳.log
   E:\UE_Project\Demo\Artifacts\M1_07A\Runtime\Run01_时间戳.json
   ```

   每次启动生成新时间戳，**不覆盖第一轮日志**。看到日志窗口正常，操作焦点放在游戏窗口；游戏窗口没有Editor工具栏 / Content Browser。脚本检测到UnrealEditor仍运行会拒绝启动，先保存并关闭Editor再执行。
3. 若使用Attempt02等输出，明确传入**包含Windows子文件夹的那层目录**：

   ```powershell
   .\Tools\Run-StageBuild.ps1 -BuildDirectory 'E:\UE_Project\Demo\Artifacts\Builds\M1_07A\Attempt02' -RunName Run01
   ```

4. 本批不传`-Trace`，性能采集在B。A的窗口分辨率是功能检查条件，不能当作1080p性能指标。若PowerShell因执行策略拒绝脚本，不改全局策略；可直接手动执行：

   ```powershell
   New-Item -ItemType Directory -Path 'E:\UE_Project\Demo\Artifacts\M1_07A\Runtime' -Force
   & 'E:\UE_Project\Demo\Artifacts\Builds\M1_07A\Attempt01\Windows\EmberTrial.exe' -windowed -ResX=1280 -ResY=720 -log '-abslog=E:\UE_Project\Demo\Artifacts\M1_07A\Runtime\Run01_Manual.log'
   ```

   第二轮手动改为Run02_Manual.log；若已有同名文件，使用Run03等新名字，保留旧证据。两种启动方式选一种即可。

## 5. 一条包内流程（约10–15分钟）

这里查Cook / 独立进程 / 默认地图差异，沿用M1已通过规则；无需每键十次、不加测试函数、不重跑换Pawn矩阵。

### 5.1 菜单、基础战斗与暂停（约3–4分钟）

**准备：**UE已关闭，Run01启动，暂未开始试炼。

1. 首屏是中文主菜单，无战斗HUD / Pawn，**继续本次试炼不可用**。用鼠标开始新试炼；进入正式房间、HUD100 / 单HUD，鼠标隐藏，无需再点视口即可移动 / 转向 / 跳跃。
2. 靠近训练目标或敌人，**松开移动键站定后左键一次**。看到正式攻击动画和真实伤害；接着2闪避一次，结束后基础输入恢复。玩家移动或空中拒绝攻击仍是既定规则。
3. 走近M1_Target_A，E激活、离开提示隐藏；中文提示正常。P暂停，按 / 松左键与E，然后鼠标继续：暂停中未执行、恢复不补播；再P打开，用Enter或Space继续一次，关闭后无需额外点击视口。

**预期日志：**Menu ready / NewRun→正式图Confirmed1、Game flow ready、攻击Begin / End与伤害、Pause shown / removed / resumed.Paused0。不要为了取这些日志额外修改Debug绘制、伤害数值或帧率。

### 5.2 真正检查点、失败重试与菜单继续（约4–6分钟）

**准备：**Run01同一进程，角色存活100/100。

1. 找到**Checkpoint_A检查点标记**，走进其触发范围，标签从`CHECKPOINT`变为`ACTIVE`。**M1_Target_A的E交互不是检查点激活**，它们是两个独立Actor。应有`Checkpoint activated ... Id=Checkpoint_A`一类日志，具体格式由原生实现输出。
2. 用保留的数字4（Damage25）四次真实致死；HUD0，只出现一份中文失败页。按 / 松P不打开暂停页；鼠标“重新试炼”进入新世界、从Checkpoint_A出生、身体 / HUD100。保留4只是当前Development包的既有开发入口，最终投递版另行收口。
3. 新身体P暂停→返回主菜单；无战斗HUD，继续本次试炼可用。选择继续，应重新从**Checkpoint_A**出生、新100、无旧暂停页、基础输入正常；记录Continue / Snapshot / Confirmed1即可，不再测缺失ID注入或None地图。
4. 重载后敌人与M1_Target_A/B交互回初始是正常行为；同一暂停页直接“继续游戏”保持原世界。中文主菜单 / Pause / Failure / HUD及三段动作资源均应与PIE一致。

**恢复：**保留原配置。最后P返回主菜单，用“退出游戏”真正关闭这个独立进程，日志结束后保留Run01.log / JSON。

### 5.3 第二次启动（约1–2分钟）

1. 编辑器仍关闭，执行：

   ```powershell
   .\Tools\Run-StageBuild.ps1 -RunName Run02
   ```

2. 新进程首屏“继续本次试炼”重新禁用；开始新试炼从Entry出生、HUD100。移动 / 转向和站定左键一次正常即可，**不重做5.1 / 5.2整套**。
3. P返回主菜单→退出游戏。两次完整运行日志都保留，反馈Runtime目录路径即可，我读取时按RunName / JSON区分，不要求你精确掐按键时间。

## 6. 失败材料与窄排查

| 现象 | 反馈内容 / 先查 |
| --- | --- |
| Build / Cook / Stage失败 | 阶段、完整UAT日志路径；第一条实际错误及附近20–40行，不只给ExitCode6 / Unknown Error |
| 先进入旧ThirdPersonMap | 实际exe路径和新UAT完成时间；Game Default Map是否MainMenu、是否误运行M0包 |
| 返回 / 重试缺地图 | 两正式地图是否在MapsToCook，Cook错误、Travel请求与失败片段 |
| 包内中文方框 / 空白UI / 动画缺失 | 与PIE差别的局部截图或5–10秒视频、Runtime日志路径；按实际引用与Cook证据修复 |
| 左键失效但1有效 | 当前包是否在13:01补线之后重新Cook，角色IA_ET_LightAttack门控Condition是否真实连接；不是重新绑另一份GA |
| Checkpoint仍Entry | 是否走进Checkpoint_A并见ACTIVE / 激活日志；E激活M1_Target_A不记录检查点 |
| 启动脚本报错 | 完整报错、BuildDirectory、实际exe路径；不清UE / CLion缓存、不重装工具链 |

本批首次验证Standalone进程；不将同机结果写成另一台电脑兼容性已测。出现具体失败保留Attempt与Run日志，再修复对应范围。

## 7. 最简反馈

```text
M1-07A 完成 / 卡在第__节
Win64 Development Build / Cook / Stage / Package：
实际exe完整路径 / UAT日志路径：
UE关闭后独立启动；首屏菜单与新会话Continue禁用：
左键真实攻击 / 2 / E中文 / P鼠标与键盘恢复：
Checkpoint_A确实ACTIVE → 死亡Retry → 菜单Continue均从该点出生：
菜单 / Pause / Failure / HUD中文、动画与单UI：
真正退出 / Run02新会话Entry与短基线：
Runtime两轮日志所在目录：
共享配置已保存；临时设置恢复（无临时修改可填无）：
失败或最明显差异：
完成时间（完整时分）：
实际手动用时：准备__ / 检查__ / 记录__；Build / Cook等待__
```

我核对包receipt / UAT / 两轮Runtime与当前Source / 资产版本，通过后再下发B的实际采样步骤。A通过前不提前标M1收口，也不以旧包FPS决定优化方向。

## 8. 本批需要Commit / Push（固定放在文档最后）

| 类别 | 本批内容 |
| --- | --- |
| 新手写工具 | `Tools/Run-StageBuild.ps1` |
| 新开发文档 | `Docs/M1_07_ReleaseRoadmap.md`、`Docs/M1_07A_StagePackaging.md` |
| 当前D收尾 | `Docs/Reports/M1_06D_Verification.md`、`Docs/Reports/M1_06_Verification.md`；D指南与AssetManifest / Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow_Options / Introduction等实际差异 |
| 实际共享Config变化 | 若Build改为Always，提交`EmberTrial/Config/DefaultGame.ini`；Maps已一致则不重改DefaultEngine.ini；其余只有实际必要变动才纳入 |
| A完成后摘要 | 助手后续实际生成的M1-07A验收报告与状态 / 清单更新，本轮尚未预写“包内通过” |

D八Source / 四旧资产若还未提交，先按[D末尾清单](M1_06D_PauseFlow.md#12-本批需要commit--push的内容固定放在文档最后)提交真实实现；若已经提交，不重复加入无差异文件。A没有新增C++或UE资产，不为提交清单重存地图 / BP。

`Artifacts/Builds/M1_07A`完整包、`Artifacts/M1_07A/Runtime`日志 / JSON / Trace、Raw、Binaries / Intermediate / Saved、生成索引继续留本地，不Force Add，不将包放到Git LFS替代正式可编辑资产。分享包在后续发布准备中另做，不自动上传。

CLion：Save All → 刷新Changes → 将最新Unstaged变化也Stage → 检查Staged Diff → Commit → Push。阶段实现 / 工具可用`build: prepare M1 standalone package workflow`；A实际通过后报告可用`docs: record M1 packaged flow acceptance`。Commit / Push由用户操作。
