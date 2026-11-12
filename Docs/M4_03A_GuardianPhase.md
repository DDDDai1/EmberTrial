# M4-03A 守关者战斗与阶段转换

2026-11-12 正式工程验收通过。[验收报告](Reports/M4_03A_Verification.md) · [M4-03B 横扫指南](M4_03B_GuardianSweep.md)

这一章交付一间可独立打完的守关者房间。进入触发区后生成一名180生命的守关者，先使用既有近战追击和挥击。生命降到90或更低、当前动作恢复后，头顶阶段标记变橙，追击速度从220提高到330 cm/s。击败守关者即可通过出口，死亡后从Entry重开。预计手动45–75分钟，构建等待另计。

本轮制作一个可玩的首领骨架。后续会在同一身体上接入横扫、突进和地面冲击及各自的前摇反馈，再将房间接到正式关卡路线。

## 1. 应用源码并构建

保存并关闭UE，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
git apply --check .\Docs\Patches\M4_03A_GuardianPhase.patch
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留错误输出' }
git apply .\Docs\Patches\M4_03A_GuardianPhase.patch
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留错误输出' }
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

补丁新增 `AETGuardianCharacter` 的头文件、实现与专项测试。CLion重新加载Core、UI、Editor三模块后，确认两个角色文件与测试文件可见。打开 `ETGuardianCharacter.cpp` 时应能跳转到原生角色与GAS头文件。[源码补丁](Patches/M4_03A_GuardianPhase.patch)

原生守关者类监听自身ASC的生命和动作标签。到达阈值后在下一帧检查生命和行动状态，忙碌时等到标签撤销后再次检查。当前生命为零或死亡已封口时，结束观察并隐藏阶段标记。二阶段一经切换便保持到当前身体结束，切换后释放生命及动作标签订阅。蓝图负责Mesh、能力资产和默认值，阶段、死亡优先级和移动速度由C++管理。

## 2. 新建守关者蓝图

1. 在内容浏览器进入 `/Game/EmberTrial/Characters/Enemies`。空白处右键 → **Blueprint Class → All Classes**，搜索 **ETGuardianCharacter**，以该原生类为父类创建 `BP_ETGuardian`。打开后在 **Class Settings → Parent Class** 核对父类。Event Graph可以留空。
2. 点 **Class Defaults**，逐项用Details搜索框查找并填写。搜下一项前清空上一项。下拉结果重名时以资产路径为准。

| 搜索字段 | 值 |
| --- | --- |
| `AI Controller Class` | `/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController` |
| `Auto Possess AI` / `Auto Possess Player` | `Placed in World or Spawned` / `Disabled` |
| `Combat Team` | `Enemy` |
| `Initial Health` / `Initial Max Health` | 180 / 180 |
| `Light Attack Ability Class` | `/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack` |
| `Use Controller Rotation Pitch / Yaw / Roll` | False / False / False |
| `Phase Two Health Fraction` / `Phase Two Walk Speed` | 0.50 / 330 cm/s，沿用原生默认 |

3. 清空搜索。在 **Components** 点选表中对应的继承组件，再在右侧Details配置。每种原生组件保留一份。

| 组件 | 配置 |
| --- | --- |
| `Mesh` | Skeletal Mesh=`SKM_Quinn_Simple`，Animation Mode=`Use Animation Blueprint`，Anim Class=`ABP_Quinn`。Location=0 / 0 / -90，Rotation的Yaw=-90°，Pitch与Roll为0，Scale全1。Collision Presets=`NoCollision`，Can Ever Affect Navigation=False，Visibility Based Anim Tick Option=`Always Tick Pose and Refresh Bones` |
| `CapsuleComponent` | Capsule Radius=42，Capsule Half Height=96。Collision Presets选`Custom`后设置为`Query and Physics`，Object Type=`Pawn`，Visibility=`Overlap`，WorldStatic / WorldDynamic / Pawn=`Block`，Camera=`Ignore`。其余响应对照既有近战敌人 |
| `CharacterMovement` | Max Walk Speed=220，Orient Rotation to Movement=True，Use Controller Desired Rotation=False |
| `CombatLife` / `CombatPoise` | Hit React Ability Class=`GA_ET_HitReact` / Stagger Ability Class=`GA_ET_Stagger`。两组件均Auto Activate=True |
| `CombatTrace` | 沿用手骨`hand_r`、25 cm与Draw Debug=False |
| `CameraBoom` / `FollowCamera` | Do Collision Test=False / Auto Activate=False |
| `PhaseMarker` | 使用原生的文字、位置、尺寸和朝向。从入口看向 +X 时可读 `GUARDIAN I`。战斗中换边时可以转到入口侧观察文字 |

4. 点击 **Compile → Save**。核对没有额外的PracticeAction、玩家HUD或远程能力资产。其余碰撞字段可对照[近战敌人胶囊配置](M1_05A_EnemyFoundation.md#64-capsule挡住世界可被命中)。

## 3. 配置一名守关者与出口

1. 在内容浏览器进入 `/Game/EmberTrial/Maps`，右键补修后的 `L_ET_MixedEncounter` → **Duplicate** → `L_ET_GuardianArena`。双击打开，确认顶部关卡标签为 **L_ET_GuardianArena**。
2. 在新地图的 **Outliner** 选 `MixedEncounter_A`，按 **F2** 改标签为 `GuardianEncounter_A`。保持它的位置、触发器和出口门配置，沿用 `BP_ETMixedEncounter` 资产。下列Spawns改动在关卡实例的Details进行。
3. 在 `GuardianEncounter_A` 的Details搜索 **Spawns**。第1项原远程守卫用该项右侧菜单 **Delete** 删除。展开剩余第0项，`Enemy Class` 改为 `BP_ETGuardian`，`Point` 选择现有 `Spawn_Mixed_Melee`。确认数组只剩一项。
4. 在Outliner选 `Spawn_Mixed_Melee`，改标签为 `Spawn_Guardian`。把世界位置设为 **X=150、Y=0、Z=120**，Yaw=180°，Scale全1。回到 `GuardianEncounter_A` 确认第0项Point仍指向改名后的标记。删除未使用的 `Spawn_Mixed_Ranged`。
5. 保留复制来的地面、侧墙、Entry、检查点和导航体。守关者由触发器生成，关卡里应能找到一个出生标记和一个遭遇Actor。用 **Build → Build Paths** 重建导航，按编辑器 **P** 确认从Entry到守关者之间有连续可走区域，再按P隐藏导航。**Save All**。从当前地图直接Play做本章验收，正式菜单入口和Cook配置继续使用已有地图。

## 4. 分组验收

隔离工程用 UE 5.5.4 完成Editor与Game Development构建，50项 `EmberTrial` 自动化全部通过。实机以第3节创建的正式资产为准。

| 分组 | 操作与预期 |
| --- | --- |
| 专项与旧遭遇 | 控制台先运行 `Automation RunTests EmberTrial.M4.Guardian.Phase`，完成后运行 `Automation RunTests EmberTrial.M4.Encounter.Mixed`。两项各一次Success。守关者专项验证半血边界、攻击标签阻断和恢复、二阶段提速、治疗保持阶段、致死优先和新身体重置 |
| 实机一，进场与转阶段 | 从Entry进场前确认场内无守关者、出口关闭。越过入口后应只有一个身体、一条 `Encounter started ... Enemies=1`。观察一阶段的追击和真实10伤害。用空槽单点左键，等每段结束再点，四次真实命中应把180降到80。等受击或当前动作结束，朝入口侧观察橙色 `GUARDIAN II` 与更快追击。日志应有一次 `Guardian phase entered ... Phase=2 Health=80.0/180.0 WalkSpeed=330.0`。若装备、连段或额外击中导致生命不同，反馈实际命中次数即可 |
| 实机二，开门与重试 | 继续击败守关者。确认标记消失、停止攻击、出口一次清场放行。Stop后重开，让新守关者击败玩家，点Retry。重新进场应重新生成一名180生命的一阶段身体，出口重新关闭。做一次P继续及移动短查，然后Save All |

如果迟迟未转阶段，先确认日志中的真实生命已经≤90且大于0。若动作已结束仍未转换，回报 `BP_ETGuardian` 的两个阶段配置值和完成时分，我会核对命中、受击结束和阶段日志。若完全没生成，先在Outliner核对新地图实例的Spawns、Point和AI自动接管字段。

```text
M4-03A 完成
三新源码 / 双Build / 三索引：
新守关者BP与新地图 / 单出生点 / Save All：
Guardian.Phase / Encounter.Mixed：
一阶段180生命与真实攻击 / 半血动作恢复后只转一次 / 标记与提速：
死亡清场开门 / Retry再入新180与一阶段 / P与移动：
失败或特别说明（无填无） / 完成时分 / 手动时间 / 构建等待：
```

## 5. Commit / Push

| 批次 | 文件 | 英文建议 | 中文建议 |
| --- | --- | --- | --- |
| 混合遭遇补修与守关者指南 | `L_ET_MixedEncounter.umap` 的LFS更新、M4-02验收、当前进度与资产清单、M4-03A本指南和补丁 | `fix(level): remove preplaced actors from mixed encounter` | `fix(level): 移除混合遭遇中的预放角色` |
| M4-03A实现 | `Public/Characters/ETGuardianCharacter.h`、`Private/Characters/ETGuardianCharacter.cpp`、`Tests/ETGuardianPhaseTests.cpp`、`BP_ETGuardian.uasset`、`L_ET_GuardianArena.umap` | `feat(combat): add two-phase guardian encounter` | `feat(combat): 新增两阶段守关者遭遇战` |

资料组还包括 `README.md`、`Introduction.md`、`Docs/Progress.md`、`Docs/Backlog.md`、`Docs/Architecture.md`、`Docs/AssetManifest.csv`、`Docs/VersionControl.md`、`Docs/AcceptanceWorkflow_Options.md` 和 M4-02操作指南。源码应用及实机验收后提交实现组。两张地图和守关者BP走Git LFS，`Artifacts`留本地。
