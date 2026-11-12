# M4-02 混合遭遇与出口门

2026-11-12 已完成地图补修和实机复验。后续守关者地图从本次补修后的副本创建。[M4-02验收](Reports/M4_02_Verification.md) · [M4-03A操作](M4_03A_GuardianPhase.md)

在一间独立灰盒战斗室完成一场遭遇。玩家跨过入口后生成一名近战和一名远程守卫。第一名敌人死亡时出口保持关闭，第二名敌人死亡后门消失并可穿过。Retry从该地图的检查点重开，下一次进场重新生成两名敌人。预计手动75–110分钟，构建等待另记。[远程守卫验收](Reports/M4_01D2_Verification.md)

## 1. 应用源码并构建

保存并关闭UE，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
git apply --check .\Docs\Patches\M4_02_MixedEncounter.patch
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留错误输出' }
git apply .\Docs\Patches\M4_02_MixedEncounter.patch
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留错误输出' }
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

补丁新增原生 `AETEncounter` 的头文件、实现及专项测试三份文件，并在 `EmberTrialCharacter.h` 增加测试访问声明。开启UE前确认双Build成功，CLion重新加载三个目标。[源码补丁](Patches/M4_02_MixedEncounter.patch)

遭遇Actor从实例的 `Spawns` 读取敌人类和 `TargetPoint`。合法玩家进入后一次生成并监听各敌人的 `OnDeathSealed`，每个死亡只计一次。遭遇配置错误、生成失败或敌人被外部销毁时记录 `Encounter failed`，放行以免卡关，已生成且仍登记的敌人会清理。成功击杀全部敌人时才会记录 `Encounter cleared`。

## 2. 在独立地图装配房间

1. 从内容浏览器复制 `L_ET_VerticalSlice` 为 `/Game/EmberTrial/Maps/L_ET_MixedEncounter`。打开新地图，核对关卡标签名。保留原GameMode、地面、四墙、`Start_Entry`、`Start_Checkpoint_A` 和导航体。移除这张副本里的预放近战、远程与训练目标三个实例。用原 `BP_ETMeleeEnemy` 和 `BP_ETRangedGuard_Native` 作为生成类。
2. 在 **Place Actors** 搜索 `Target Point`，放两个并按表填写关卡世界坐标。Yaw填旋转Z。再搜索 `Cube` 添加出口两侧的固定墙，使用 Engine Basic Shapes 的100 cm立方体和 `BlockAll`。两墙靠内的一端分别在 Y=-150 和 Y=150，留300 cm出口。

| Outliner 名称 | 世界位置 X/Y/Z | Yaw | Scale X/Y/Z |
| --- | --- | --- | --- |
| `Spawn_Mixed_Melee` | `150 / 220 / 120` | `180°` | `1 / 1 / 1` |
| `Spawn_Mixed_Ranged` | `380 / -350 / 120` | `180°` | `1 / 1 / 1` |
| `SM_MixedExit_Wall_NegY` | `750 / -575 / 150` | `0°` | `0.5 / 8.5 / 3` |
| `SM_MixedExit_Wall_PosY` | `750 / 575 / 150` | `0°` | `0.5 / 8.5 / 3` |

3. 在 `/Game/EmberTrial/Blueprints` 通过 **Blueprint Class → All Classes → ETEncounter** 新建 `BP_ETMixedEncounter`。在组件树选中继承的 `EntryTrigger` 和 `ExitGate`，分别填写下表。保存蓝图，然后拖入新地图。实例命名 `MixedEncounter_A`，Actor世界位置 `-350 / 0 / 120`，旋转全0，Scale全1。

| 组件 | 配置 |
| --- | --- |
| `EntryTrigger` | 相对位置全0。Box Extent `120 / 950 / 140`。维持 Query Only，Pawn Overlap，Generate Overlap Events勾选，Can Ever Affect Navigation不勾选 |
| `ExitGate` | Static Mesh=`/Engine/BasicShapes/Cube`。相对位置 `1100 / 0 / 30`，相对Scale `0.5 / 3.02 / 3`。Collision Presets=`BlockAll`，Hidden in Game不勾选，Can Ever Affect Navigation不勾选 |

4. 在关卡Outliner选中 **MixedEncounter_A 实例**。Details搜索 `Spawns`，点两次 `+`，展开两个元素。第0项选择 `BP_ETMeleeEnemy` 和 `Spawn_Mixed_Melee`，第1项选择 `BP_ETRangedGuard_Native` 和 `Spawn_Mixed_Ranged`。`Point` 用吸管从Outliner取对应的 `TargetPoint`，`Enemy Class` 通过资产下拉选择。
5. **Build → Build Paths**，用编辑器的 **P** 查看两出生点与入口之间的绿区。门中线 `X=750, Y=0` 要能看到一整块封口，门后留出约200 cm供通过。按P关闭导航显示并 **Save All**。直接打开新地图验收，主菜单的试炼入口继续使用 `L_ET_VerticalSlice`。

## 3. 正式验收

隔离工程的 UE 5.5.4 Editor Build 和49项 `EmberTrial` 自动化已通过。正式工程装配后的地图操作按下列步骤验收。

先运行 `Automation RunTests EmberTrial.M4.Encounter.Mixed`。专项使用原生测试玩家与两种正式敌人蓝图，验证进入触发、两次死亡、重复进入、无关对象死亡和重复出生点失败处理。

1. **进场与开门。** 在新地图PIE。默认Entry出生后沿 `+X` 走向房间，入口箱覆盖横向通道，越过后日志应出现一次 `Encounter started ... Enemies=2`。分别观察近战追上和远程停步发射。击败一个，确认 `Remaining=1` 且门仍挡路。击败另一个，确认一次 `Encounter cleared`，走过 `X=750` 检查门后空间。
2. **失败和重开。** Stop，再Play，门应恢复关闭。跨入后让敌人击败玩家，点现有的Retry按钮。出生后应看到闭门，重新进入应得到两个新身体与一条新 `Encounter started`。完成后Stop并保存。退出PIE后可回原地图做一次玩家输入与远程守卫短查。

实机无生成时先看是否出现 `Encounter failed`。`InvalidSpawns` 检查两个类和两份不同的 Point，`EnemySpawnFailed` 检查出生标记是否在地面上方、与其他胶囊或墙分离，以及生成类的 Auto Possess AI 与属性就绪。若完全无遭遇日志，确认正在新地图、跨过 `EntryTrigger`、玩家胶囊与触发器均启用重叠事件。

```text
M4-02 完成
三份新源码与测试访问声明 / 双Build / 三索引：
新地图 / BP / 两个生成点 / 两侧墙与导航 / Save All：
Encounter.Mixed：
一次生成两名 / 近战与远程 / 击败一名门关闭 / 两名后可穿过：
Stop重开 / 死亡Retry / 两名重新生成：
失败或特别说明 / 完成时分 / 手动时间 / 构建等待：
```

## 4. Commit / Push

| 批次 | 提交范围 | 英文建议 | 中文建议 |
| --- | --- | --- | --- |
| 验收和指南 | D2验收、资产来源与进度更新，本指南及源码补丁 | `docs: verify ranged guard and guide mixed encounter setup` | `docs: 验收远程守卫并补充混合遭遇装配指南` |
| M4-02实现 | `Public/Flow/ETEncounter.h`、`Private/Flow/ETEncounter.cpp`、`Tests/ETMixedEncounterTests.cpp`、`Public/Characters/EmberTrialCharacter.h` 的测试访问声明、`BP_ETMixedEncounter.uasset`、`L_ET_MixedEncounter.umap` | `feat: add mixed encounter and combat exit gate` | `feat: 新增混合遭遇与战斗出口门` |

资料组包含 `README.md`、`Introduction.md`、`Docs/Progress.md`、`Docs/Backlog.md`、`Docs/Architecture.md`、`Docs/AssetManifest.csv`、`Docs/VersionControl.md`、`Docs/AcceptanceWorkflow_Options.md`、D2指南、D2验收报告、本指南及补丁。

`.uasset` 和 `.umap` 随当前 LFS 规则提交。`Artifacts` 中的隔离工程、测试报告和构建产物保留在本地。
