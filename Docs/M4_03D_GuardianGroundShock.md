# M4-03D 守关者地面冲击与两阶段选招

2026-11-12 正式工程验收通过。[验收报告](Reports/M4_03D_Verification.md) · [M4-04 完整试炼路线](M4_04_TrialRoute.md)

本章让同一名守关者在一场战斗中使用三种招式。沿用 M4-03C 的AI停步和转向逻辑，能力在起招时依据水平距离与当前阶段选择动作。地面冲击先使用双手蓄力占位动画，约0.80秒后以守关者为中心结算一次26点伤害。效果以本轮实机为准，后续美术阶段再替换动作及落地表现。

| 接战条件 | 一阶段 | 二阶段 |
| --- | --- | --- |
| 已进入AI攻击范围，距离大于140 cm | 固定方向突进，22 | 固定方向突进，22 |
| 距离不超过140 cm | 正面横扫，18 | 地面冲击26、横扫18交替，第一次近距离起招为地面冲击 |

一次近距离招式开始播放后推进轮换，随后被取消也保留该次轮换结果。中距离突进不消耗近距离轮换。预计手动45–70分钟，构建等待另计。

## 1. 应用补丁

保存并关闭UE，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
git apply --check .\Docs\Patches\M4_03D_GuardianGroundShock.patch
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留错误输出' }
git apply .\Docs\Patches\M4_03D_GuardianGroundShock.patch
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留错误输出' }
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

补丁新增守关者能力子类的头、实现与选招测试，更新原轻攻击首段选择、扇形查询及其测试。守关者仍持有一份轻攻击Spec。选中的蒙太奇和伤害在起招时快照，后撤或转阶段均不会替换已经启动的动作。圆形范围沿用一次采样、高差限制、逐目标墙挡、无敌与伤害GE。近距绕背补修已合入该补丁，当前工作区也已直接修复。已经应用过补丁的工作区从第4.1节复验即可。[源码补丁](Patches/M4_03D_GuardianGroundShock.patch)

## 2. 制作地面冲击蒙太奇

1. 在内容浏览器找到 `/Game/EmberTrial/Animations/Ranged/Montage/AM_ET_RangedShot_01`。右键 **Duplicate** 创建 **AM_ET_GuardianGroundShock_01**，把新副本移动到 `/Game/EmberTrial/Animations/Guardian`。打开新副本。确认使用 `AS_ET_BreakPulse_01`，保留一个 `DefaultSlot` 片段，片段 Play Rate=1、Loop Count=1。
2. 在蒙太奇 **Asset Details** 设置 **Rate Scale = 0.50**，Blend In与Blend Out各0.10，Enable Auto Blend Out=True。
3. 在 **Notifies** 轨单独选中原发射通知并删除。右键轨道空白 → **Add Notify State → ET Sector Attack Window**。右键新状态条本体，设 **Notify Begin Time=0.400000** 秒并按Enter，再设 **Anim Notify State Duration=0.133333** 秒并按Enter。时间轴上约第12–16帧，实际约0.80秒触发一次。
4. 左键单选新状态条，在 **Details** 勾选 **Full Circle**，设 **Range=210 cm**、**Height Tolerance=50 cm**。Full Circle启用后 Half Angle 灰色是预期。Notifies保留一条圆形窗口，保存蒙太奇。
5. 预览前摇和双手发力，确认当前打开的是新AM。旧远程发射AM及旧横扫AM保留各自的通知。

## 3. 配置守关者能力

1. 在 `/Game/EmberTrial/Abilities` 空白处右键 → **Blueprint Class → All Classes**，选择原生 **ETGA_GuardianAttack**，命名 **GA_ET_GuardianAttack**。打开并在 **Class Settings → Parent Class** 核对。Event Graph留空。
2. 打开 **Class Defaults**，逐项填写。新建资产需要手动填好既有伤害GE和攻击执行策略。

| 字段 | 值 |
| --- | --- |
| Attack Montage / Attack Damage | `AM_ET_GuardianSweep_01` / 18 |
| Charge Montage / Charge Damage | `AM_ET_GuardianCharge_01` / 22 |
| Ground Shock Montage / Ground Shock Damage | `AM_ET_GuardianGroundShock_01` / 26 |
| Close Range | 140 cm |
| Damage Effect Class | `GE_ET_LightAttackDamage` |
| Attack Max Horizontal Speed | 3 cm/s |
| Net Execution Policy / Instancing Policy | Server Only / Instanced Per Actor |
| Replication Policy / Retrigger Instanced Ability / Enable Combo | Do Not Replicate / False / False |

3. **Compile → Save**，运行 **Validate Assets**。打开 **BP_ETGuardian → Class Defaults**，把 **Light Attack Ability Class** 指向新 `GA_ET_GuardianAttack`。保持 `BP_ETGuardianAIController` 和原180/180、阶段阈值及速度。**Compile → Save All**。本章沿用已有地图、黑板和BT。

## 4. 分组验收

UE 5.5.4隔离工程的Editor、Game构建及54项 `EmberTrial` 自动化全部通过。以下检查验证正式蒙太奇、能力资产与整场战斗。

1. **自动化。** 控制台运行 `Automation RunTests EmberTrial.M4.Guardian`，确认 `AttackRules`、`ChargeWindow`、`CloseTracking`、`Phase`、`SweepSector`、`SweepWindow` 各一次Success。再运行 `Automation RunTests EmberTrial.M2.Combo.Flow`，检查玩家三段连击。
2. **一阶段两招。** 打开 `L_ET_GuardianArena` 从Entry进场。让守关者从中距离起招，日志应有 `Phase=1 Move=Charge`，正面命中扣22。冲近后先保持贴近，等下一招 `Phase=1 Move=Sweep`，正面命中扣18。若一直突进，确认角色未持续后撤，或在突进结束后主动贴近。
3. **二阶段地面冲击。** 用真实伤害打到90生命以下，等橙色阶段标记出现。贴近时应有 `Phase=2 Move=GroundShock`，地面上即使绕到守关者背面仍在圆形范围，命中扣26。下一次贴近应有 `Move=Sweep`。再观察一次地面冲击时闪避或跳离命中高度，确认HUD和日志的实际结果。日志中地面冲击窗口有 `FullCircle=1`。
4. **收尾。** 受击或破势打断一次，观察本次招式收尾后仍能正常出招。清场开门，Stop重开后恢复180与一阶段。回正式地图短查左键、2、P与移动。时点难观察时可用 `slomo 0.5`，最后恢复 `slomo 1`。

若守关者追击后持续拒绝攻击，先检查新GA父类、三份不同的Montage引用、伤害GE和 Enable Combo。若有地面冲击日志但背后范围内始终无伤害，检查新AM选中的状态条是否勾选 **Full Circle**，并核对距离、遮挡及高差。

```text
M4-03D 完成
四新九旧源码 / 双Build / 三索引：
新冲击AM / 新GA与三Montage / 守关者引用 / Compile-Save-Valid：
Guardian六项 / Combo.Flow：
一阶段真实突进22与横扫18 / 二阶段冲击26和横扫轮换：
冲击背面与避让 / 受击取消 / 清场和Stop重开 / 原输入短查：
近距后摇绕侧背 / 二阶段冲击绕背：
失败或最明显占位问题 / 完成时分 / 手动时间 / 构建等待：
```

### 4.1 绕背丢失目标补修

22:45:05.594 守关者横扫空挥，22:45:05.627 视野丢失立即清空黑板目标，下一帧近战任务以 `ContextLost` 取消后摇。22:45:17.277 突进中也出现同样的事件顺序。两处现象都来自原生AI的视野规则。起招后锁定朝向期间，玩家离开前方视锥，`HandleSightUpdated` 就会清空 `TargetActor`。

守关者已发现玩家后，视锥失去目标时会检查角色中心间的三维距离和Visibility遮挡。默认 **300 cm** 内无遮挡会继续保留同一目标。现有行为树决策服务会复核该条件，超距或隔墙就清空，玩家死亡、销毁或换控也会退出追踪。重新进入视锥后恢复正常感知。首次发现仍靠原视野，普通近战和远程守卫沿用各自原来的目标丢失规则。

`Close Tracking Radius` 位于守关者蓝图 **Class Defaults → EmberTrial → Guardian**，继承默认300 cm。现有守关者蓝图和地图直接使用默认值，无需手动改资产。

1. 用新构建打开 `L_ET_GuardianArena`，先在守关者正面触发战斗。让一次横扫开始，趁前摇或后摇绕到侧后方。预期横扫仍按出招朝向判断，可空挥。后摇结束后守关者重新朝向玩家，日志可以看到 `Guardian close tracking retained`，同一段无遮挡的近距绕背不再出现 `SightLost` 清空目标。
2. 进入二阶段后，在近距离等到地面冲击起手再绕背。仍处于210 cm及允许高差内时，地面冲击可以在背面命中26。横扫与突进仍沿出招方向执行。
3. 自动化入口为 `Automation RunTests EmberTrial.M4.Guardian.CloseTracking`，覆盖未发现时背面不获取、攻击期间同一黑板目标、动作结束后保留、超距、遮挡、恢复视野、死亡和普通守卫回归。复验只需反馈 **一阶段后摇绕侧背／二阶段冲击绕背／通过或失败时分**。

正式工程Editor与Game Development构建、CLion三模块索引及55项全量自动化通过，包含新增近距追踪专项与远程守卫回归。结果保存在本地 `Artifacts/M4_03D_CloseTracking/Report/index.json`。

## 5. Commit / Push

| 批次 | 提交范围 | 英文建议 | 中文建议 |
| --- | --- | --- | --- |
| M4-03C验收与新指南 | C验收报告、C指南回链、README、Introduction、Progress、Backlog、Architecture、AssetManifest、VersionControl、AcceptanceWorkflow_Options及本指南与补丁 | `docs: verify guardian charge and guide phase attacks` | `docs: 验收守关者突进并补充阶段选招指南` |
| M4-03D实现 | 补丁十三份Core文件，新 `AM_ET_GuardianGroundShock_01.uasset`、`GA_ET_GuardianAttack.uasset`，更新的 `BP_ETGuardian.uasset` | `feat(combat): add guardian phase attacks and close tracking` | `feat(combat): 实现守关者阶段招式与近距追踪` |

三份资产走Git LFS。源码和资产实机通过后提交，`Artifacts` 中隔离工程、日志和构建结果保留本地。
