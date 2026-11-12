# M4-03B 守关者横扫

守关者在原房间里用横扫替换占位挥击。它停步朝向玩家，收臂蓄力约0.67秒，挥出时结算一次前方扇形伤害。玩家可以退出范围、绕到侧后或用闪避穿过命中时点。两阶段继续共享这套招式，第二阶段保留更快追击。突进、地面冲击和阶段选招随后接入。预计手动40–60分钟，构建等待另计。

## 1. 应用补丁

保存并关闭UE，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
git apply --check .\Docs\Patches\M4_03B_GuardianSweep.patch
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留错误输出' }
git apply .\Docs\Patches\M4_03B_GuardianSweep.patch
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留错误输出' }
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

确认 Editor / Game Build 成功，CLion三个目标完成重新索引。新增 `ETAttackSector.h/.cpp` 和 `ETAttackSectorTests.cpp`。已有 Attack Window 通知和 CombatTrace各改两份文件。扇形查询在新通知窗口内采样一次，按角色中心测水平距离、正面角度及高度，并逐目标检验Visibility遮挡。候选仍通过原轻攻击的去重、阵营、闪避无敌和GE伤害链。受击、破势或死亡撤销当前攻击后，旧通知会被原蒙太奇实例身份挡住。[源码补丁](Patches/M4_03B_GuardianSweep.patch)

## 2. 创建守关者横扫资产

### 2.1 蒙太奇

1. 在内容浏览器进入 `/Game/EmberTrial/Animations/Combo/Montages`。将 `AM_ET_Light_03` **Duplicate** 为 `AM_ET_GuardianSweep_01`，移动新副本到新建的 `/Game/EmberTrial/Animations/Guardian`，打开新副本。它使用 `AS_ET_Light_03` 的收臂蓄力到挥出姿势。保留一个片段、`DefaultSlot`、片段Play Rate=1与Loop Count=1。
2. 在新蒙太奇 **Asset Details** 将 **Rate Scale** 设为 **0.50**，Blend In / Blend Out各 **0.10**，Enable Auto Blend Out保持True。时间轴仍按原资产秒数显示，运行速度减半。
3. 在 **Notifies** 轨道选中副本中旧的 `ET Attack Window` 状态条，按 **Delete** 删除。右键轨道空白处 → **Add Notify State → ET Sector Attack Window**。创建后在状态条本体上右键，填写 **Notify Begin Time = 0.333333** 后按Enter。再右键同一条，填写 **Anim Notify State Duration = 0.133333** 后按Enter。时间轴对应30 fps的第10–14帧。运行约0.67秒命中，窗口期间只采样一次。
4. 左键单独选中这条新状态条，在 **Details** 搜索 `Range`、`Half Angle`、`Height Tolerance`，确认 **155 cm / 60° / 70 cm**。60°从朝向中心线分别向左右展开，总开角120°。通知数值的时间操作与设置位置可对照旧版UE 5.5.4的步骤。[Notify时间操作](M1_04B1_AttackTrace.md)
5. 预览一遍，确认收臂到挥出时头和双脚连续，播放自然结束。确认 `Notifies` 里只有一条横扫窗口，保存新蒙太奇。

### 2.2 能力与身体

1. 在 `/Game/EmberTrial/Abilities` 将 `GA_ET_EnemyLightAttack` **Duplicate** 为 `GA_ET_GuardianSweep`，打开副本，点 **Class Defaults**。只修改 **Attack Montage = AM_ET_GuardianSweep_01** 和 **Attack Damage = 18**。核对 **Damage Effect Class = GE_ET_LightAttackDamage**、**Attack Max Horizontal Speed = 3**、**Net Execution Policy = Server Only** 和 **Enable Combo = False**。Compile → Save。
2. 打开 `/Game/EmberTrial/Characters/Enemies/BP_ETGuardian`，点 **Class Defaults**，把 **Light Attack Ability Class** 指到 `GA_ET_GuardianSweep`。保留已有AI Controller、180/180、阶段速度和原生组件。Compile → Save。用 **Validate Assets** 检查新GA，最后 **Save All**。
3. 打开 `L_ET_GuardianArena`。关卡中的唯一生成类仍是 `BP_ETGuardian`。行为树与地图配置继续沿用 M4-03A。

## 3. 分组验收

隔离工程已通过UE 5.5.4 Editor与Game构建、52项 `EmberTrial` 自动化。以下实机验证你配置后的蒙太奇和能力。

1. **专项。** 在控制台运行 `Automation RunTests EmberTrial.M4.Guardian.Sweep`，等待 `SweepSector` 和 `SweepWindow` 各一次Success。前者覆盖正面、侧翼、背面、距离、高差和遮挡，后者用真实蒙太奇通知检查只采样一次与取消后无迟到候选。
2. **真实伤害与避让。** 新Play，从Entry触发单个守关者。站定让它第一次横扫命中，HUD应 **100→82**，日志同一 `Serial` 先有 `Sector sample ... Candidates=1`，再有 `Requested=18.0 ... Actual=18.0`。下一次等收臂动作已开始，再往侧后或后撤，观察一次空挥。用2闪避时以HUD和日志为准，单纯躲出范围也能避免伤害。若你在命中瞬间仍在扇形内，闪避无敌时会记录 `Invulnerable`。
3. **取消与旧功能。** 在一次收臂期间用真实轻攻击打断，确认这一挥没有扇形采样，恢复后新一挥仍能命中。遇到难卡的时点可在当前PIE控制台用 `slomo 0.5`，完成后恢复 `slomo 1`。继续打到半血，橙色标记和更快追击仍可见，清场门可通过。最后回 `L_ET_MixedEncounter` 做一次普通近战10伤害与P/移动短查。

若看见横扫动画但正面贴近仍始终不掉血，先找 `Sector sample`。缺失时核对选的是 **ET Sector Attack Window**、放在新 **Montage** 上，并且正式身体引用的是新GA。若有采样但 `Candidates=0`，确认命中瞬间仍在155 cm正面范围，或回报实机时刻和走位供日志比对。

```text
M4-03B 完成
三新四旧源码 / 双Build / 三索引：
新AM与新GA / 守关者引用 / Compile-Save-Valid：
SweepSector / SweepWindow：
正面真实18 / 收臂后避让 / 受击取消与恢复：
原阶段、清场开门 / 普通近战10 / P与移动：
失败或最明显占位问题 / 完成时分 / 手动时间 / 构建等待：
```

## 4. Commit / Push

| 批次 | 提交范围 | 英文建议 | 中文建议 |
| --- | --- | --- | --- |
| M4-03A验收与横扫指南 | A验收报告、A操作指南回链、README、Introduction、Progress、Backlog、Architecture、AssetManifest、VersionControl、AcceptanceWorkflow_Options及本指南和补丁 | `docs: verify guardian phases and guide sweep setup` | `docs: 验收守关者阶段并补充横扫配置指南` |
| M4-03B实现 | 补丁的7份Core文件、新 `AM_ET_GuardianSweep_01.uasset`、新 `GA_ET_GuardianSweep.uasset`、更新的 `BP_ETGuardian.uasset` | `feat(combat): add telegraphed guardian sweep` | `feat(combat): 实现带前摇提示的守关者横扫` |

三份资产走Git LFS。源码和资产在本章实机通过后提交。构建产物、日志和隔离工程保留在本地 `Artifacts`。
