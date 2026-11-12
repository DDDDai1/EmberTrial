# M4-04 完整试炼路线

已完成正式工程验收。[M4-04 验收报告](Reports/M4_04_Verification.md) · [M4-05 胜利结算](M4_05_TrialVictory.md)

从主菜单开始进入同一张地图。玩家经过教学庭院，击败第一名近战守卫，在安全区使用祭坛，穿过近远程混合遭遇，最后挑战三招守关者。门后设置两个检查点，失败重试或菜单继续时回到最近激活的位置。最终出口用于确认清场与通行，胜利结算留在下一章。预计手动 **90–130 分钟**，构建等待另计。

```text
Entry → 教学庭院 → 近战门 → Checkpoint_A / 祭坛
      → 混合遭遇门 → Checkpoint_B → 守关者门 → 试炼出口
```

## 1. 应用源码并构建

保存并关闭 UE，在 PowerShell 执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
git apply --check .\Docs\Patches\M4_04_TrialRoute.patch
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留错误输出' }
git apply .\Docs\Patches\M4_04_TrialRoute.patch
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留错误输出' }
$project = (Resolve-Path '.\EmberTrial\EmberTrial.uproject').Path
$build = 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat'
& $build EmberTrialEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if ($LASTEXITCODE -ne 0) { throw 'Editor 构建失败，请保留首条错误' }
.\Tools\Refresh-CLion.ps1 -SkipBuild
& $build EmberTrial Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE
```

补丁新增检查点专项测试并更新三份既有源码。遭遇在世界启动后查询同地图已确认的出生检查点，匹配配置才恢复为已清场并解除门碰撞。恢复时没有生成或击杀事件。Editor 显式重建清单，确保新专项被纳入。 [源码补丁](Patches/M4_04_TrialRoute.patch)

## 2. 建立一张连续灰盒地图

1. 在内容浏览器复制 `/Game/EmberTrial/Maps/L_ET_MixedEncounter` 为 **L_ET_TrialRoute**。双击打开，确认顶部显示新地图名。沿用复制来的 GameMode。Outliner 中保持只有一份原生 Entry PlayerStart，原混合遭遇、两出生点、两扇侧墙留在原位。
2. 选中原地图地面和外墙，分别填下表。所有坐标是世界位置，单位 cm。旋转保持0，使用原 Cube 和 BlockAll。最终地面表面Z=0，外墙内侧 X=-3200至3300、Y=-1000至1000。

| Outliner 现有对象 | Location X / Y / Z | Scale X / Y / Z |
| --- | --- | --- |
| SM_ET_Floor | 50 / 0 / -10 | 65 / 20 / 0.2 |
| SM_ET_Wall_NegX | -3225 / 0 / 150 | 0.5 / 20 / 3 |
| SM_ET_Wall_PosX | 3325 / 0 / 150 | 0.5 / 20 / 3 |
| SM_ET_Wall_NegY | 50 / -1025 / 150 | 65 / 0.5 / 3 |
| SM_ET_Wall_PosY | 50 / 1025 / 150 | 65 / 0.5 / 3 |

3. 将 `Start_Entry` 移到 **-2950 / 0 / 120**，Yaw=0，`Player Start Tag` 保持 **Entry**。将教学交互目标 `Target_VS_A` 和 `Target_VS_B` 移到 **-2700 / -250 / 50** 和 **-2700 / 250 / 50**。保留两者原E提示与状态配置。
4. 现有 `Start_Checkpoint_A` 移到 **-1050 / 0 / 120**，Yaw=0。复制它，命名 `Start_Checkpoint_B`，移到 **970 / 0 / 120**，Yaw=0。在实例 Details 搜索 **Player Start Tag**，分别设 `Checkpoint_A`、`Checkpoint_B`。检查不要改到普通 Actor Tags。把 `RelicAltar_A` 移到 **-970 / -270 / 0**，Yaw=90°。
5. 给教学庭院、祭坛岔口和最终出口放三个 **Text Render** Actor。它们用来提示操作或方向。统一 Yaw=180°、Horizontal Alignment=Center、World Size=32，位置和文字按表设置。

| Location X / Y / Z | Text |
| --- | --- |
| -2660 / 0 / 220 | `LMB ATTACK   2 DODGE   E INTERACT` |
| -900 / -270 / 220 | `RELIC ALTAR [E]` |
| 2820 / 0 / 220 | `TRIAL EXIT` |

### 2.1 设置检查记录

2026-11-12 检查编辑器中尚未保存的 `L_ET_TrialRoute` 实时状态。地图使用 `BP_ETGameMode`，唯一的原生 Entry 的位置、朝向与标签正确。地面及四面外墙的位置、旋转、缩放、Cube 网格与 BlockAll 碰撞均符合表格。两个交互目标的位置、祭坛的位置与朝向、两份原混合出生点和两块原混合侧墙均符合要求。三个 Text Render 的位置、文字、Yaw、水平居中和字号均符合要求。检查点 A、B 的位置和 `Player Start Tag` 正确。

**需要调整：**`Start_Checkpoint_A` 和 `Start_Checkpoint_B` 当前的 **Yaw 均为 -90°**，本章要求为 **0°**。在 Outliner 分别选中这两个对象，在 Details 的 Transform → Rotation 中将蓝色的 **Z / Yaw** 改为 **0** 并按 Enter。这样从两个检查点重生时角色朝向路线前方。调整后保存地图。

本次检查读取的是编辑器内的当前状态。交互、检查点触发和重生朝向尚需按第5节实机验收。导航范围及新增遭遇属于第3节配置。

## 3. 装配三个遭遇与关门边界

1. 分别复制原混合遭遇的正负Y两块出口侧墙。第一对复制体放到 **X=-1250**，第二对放到 **X=2550**。每对的 Y 为 **-575 / 575**，Z=150，Scale均为 **0.5 / 8.5 / 3**。命名分别用 `SM_IntroExit_Wall_NegY / PosY`、`SM_GuardianExit_Wall_NegY / PosY`。中间原混合门两侧墙仍在 X=750。三个门洞都应只有中间一条300 cm通道。
2. 复制 `Spawn_Mixed_Melee` 两次。新 TargetPoint 分别命名 **Spawn_IntroMelee** 与 **Spawn_RouteGuardian**，位置依次为 **-1850 / 0 / 120**、**1950 / 0 / 120**，Yaw=180°。
3. 在 Outliner 复制两次 **MixedEncounter_A 实例**，改名 **IntroEncounter_A** 和 **GuardianEncounter_Route**。每次选新实例后填写位置，再配置 Spawns 和 Cleared On Spawn Checkpoint Ids。三个 Actor 都用现有 `BP_ETMixedEncounter`，Scale全1，Yaw=0。入口和门使用蓝图原相对设置，门位于各 Actor X+1100。

| 实例 / Location X,Y,Z | `Spawns` | `Cleared On Spawn Checkpoint Ids` |
| --- | --- | --- |
| IntroEncounter_A / -2350, 0, 120 | 移除复制来的两项，新增一项 `BP_ETMeleeEnemy` → `Spawn_IntroMelee` | 两项，`Checkpoint_A`、`Checkpoint_B` |
| MixedEncounter_A / -350, 0, 120 | 保留原两项，`BP_ETMeleeEnemy` 与 `BP_ETRangedGuard_Native` 指向各自出生点 | 一项，`Checkpoint_B` |
| GuardianEncounter_Route / 1450, 0, 120 | 移除复制来的两项，新增一项 `BP_ETGuardian` → `Spawn_RouteGuardian` | 空 |

4. 在每个 Spawns 元素里，`Enemy Class` 使用资产下拉，`Point` 用吸管从 Outliner 选当前地图的标记。核对总共四个出生点，地图无预放敌人。三个 `ExitGate` 的碰撞应挡住门洞，门两侧墙体和外墙无缝衔接。
5. 选 `Nav_ET_Room`，把 Actor Location 改为 **50 / 0 / 150**，Scale 保持 **1 / 1 / 1**。在 **Details → Brush Settings → Box** 填 **X=6500 / Y=2000 / Z=500**，Hollow=False。运行 **Build → Build Paths**，在编辑器按 **P** 查看三个房间与四个出生点是否全在绿色导航区。关闭导航显示并 **Save All**。

## 4. 接到主菜单与 Cook 列表

1. 打开 `/Game/EmberTrial/Blueprints/Framework/BP_ETMainMenuController`，点 **Class Defaults**，Details 搜索 **Gameplay Map**，选择 **L_ET_TrialRoute**。Compile → Save。菜单地图仍为 `L_ET_MainMenu`。
2. 打开 **Project Settings → Packaging**，搜索 **List of maps to include in a packaged build**，新增一项指向 `/Game/EmberTrial/Maps/L_ET_TrialRoute`。原主菜单和开发地图条目保留。保存后在 `DefaultGame.ini` 确认新行如下。

```ini
+MapsToCook=(FilePath="/Game/EmberTrial/Maps/L_ET_TrialRoute")
```

3. 从仓库根目录执行 `./Tools/Validate-ProjectConfig.ps1`。本章从主菜单PIE验收，独立包放到完整结算接入后集中验证。

## 5. 分组验收

隔离工程的 UE 5.5.4 Editor 与 Game 双构建及56项 `EmberTrial` 自动化全部通过。以下检查验证正式地图和入口。

1. **自动化。** 运行 `Automation RunTests EmberTrial.M4.Encounter`，应有 `CheckpointResume` 与 `Mixed` 各一次Success。专项覆盖 Entry不恢复、A只恢复第一门、B恢复前两门、待确认快照不能恢复，守关者门始终从关闭开始。
2. **一条完整路线。** 在 `L_ET_MainMenu` Play 并点开始。确认进入 `L_ET_TrialRoute` 的 Entry。沿+X经过教学目标，近战一名出现并被击败后第一门可通过。激活A，按E打开祭坛并装备一件遗物。进混合遭遇后击败第一名仍关门，击败第二名才开门。穿过混合门踩到B，守关者一名出现，最终击败后通过第三门抵达 `TRIAL EXIT`。日志应有三条成功清场，顺序为 **1、2、1** 规模的三个房间。
3. **检查点最短复查。** 第二轮激活A后，在混合遭遇死亡并Retry。确认出生A，第一门已开，混合遭遇可重新生成。推进到B，P返回菜单，点继续，确认出生B、前两门开放、已装备遗物保留、守关者新180生命。再通过一次守关者死亡Retry，确认仍从B出生并重置守关者。Stop后从菜单开始新试炼，回到Entry且三门重置。
4. 若房间提前放行，查该轮是否出现 `Encounter failed` 并保留第一条原因。恢复错门时对照三个实例的ID数组和 `Spawn selected`。若按钮仍进入开发图，核对主菜单Controller蓝图的 **Gameplay Map** 与 Cook 新条目。

```text
M4-04 完成
一新三旧源码 / 双Build / 三索引：
正式地图 / 一Entry两检查点 / 三门 / 导航 / 菜单引用与Cook配置：
CheckpointResume / Encounter.Mixed / Config检查：
菜单到出口 / 三遭遇1-2-1 / 祭坛真实装备：
Retry_A / Continue_B / Retry_B与新180 / 装备保留 / 新游戏Entry：
失败或特别说明 / 完成时分 / 手动时间 / 构建等待：
```

## 6. Commit / Push

| 批次 | 提交范围 | 英文建议 | 中文建议 |
| --- | --- | --- | --- |
| M4-03D验收与路线指南 | D验收报告、D指南回链、进度与资产来源，以及本指南与补丁 | `docs: verify guardian combat and guide trial route setup` | `docs: 验收守关者战斗并补充试炼路线装配指南` |
| M4-04实现 | 补丁四份Core文件、`L_ET_TrialRoute.umap`、`BP_ETMainMenuController.uasset`、`DefaultGame.ini` | `feat(level): connect the trial route and checkpoint gates` | `feat(level): 串联试炼路线与检查点门控` |

地图与蓝图走Git LFS。文档组还包括 README、Introduction、Progress、Backlog、Architecture、AssetManifest、VersionControl 与 AcceptanceWorkflow_Options。隔离工程和日志保留在本地 `Artifacts`。
