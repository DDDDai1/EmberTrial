# M4-03C 守关者突进

2026-11-12 正式工程验收通过。[验收报告](Reports/M4_03C_Verification.md) · [M4-03D 地面冲击与选招](M4_03D_GuardianGroundShock.md)

本章在现有守关者房间接入突进。守关者在约2.3–2.5米处停步，收臂约0.44秒，再沿发招朝向突进。空旷地面计划移动240 cm，移动窗口0.30秒，同次接触只结算一次22点伤害，撞墙提前停。两个阶段本轮均使用突进，原横扫能力和资产保留，后续与地面冲击一起接入选招。预计手动50–75分钟，构建等待另计。

## 1. 应用补丁

保存并关闭UE，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
git apply --check .\Docs\Patches\M4_03C_GuardianCharge.patch
if ($LASTEXITCODE -ne 0) { throw '补丁检查失败，请保留错误输出' }
git apply .\Docs\Patches\M4_03C_GuardianCharge.patch
if ($LASTEXITCODE -ne 0) { throw '补丁应用失败，请保留错误输出' }
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

补丁新增地面位移辅助函数、突进窗口配置和专项测试。突进与闪避共用水平 Root Motion Source 的创建、碰撞与身份清理。蒙太奇通知仍由 CombatTrace 验证实例身份，只有突进通知会启动移动与接触扫掠。窗口结束、墙挡、离地或取消时清理该次位移。命中沿用轻攻击GE与既有闪避无敌判定。[源码补丁](Patches/M4_03C_GuardianCharge.patch)

## 2. 配置蒙太奇与能力

1. 在 `/Game/EmberTrial/Animations/Guardian` 复制 `AM_ET_GuardianSweep_01` 为 `AM_ET_GuardianCharge_01`。打开新副本，在 **Asset Details** 设置 **Rate Scale = 0.60**。保留原片段 `AS_ET_Light_03`、DefaultSlot、片段Play Rate=1、Loop Count=1、Blend In/Out各0.10与Auto Blend Out。
2. 在新蒙太奇 **Notifies** 轨删除旧 `ET Sector Attack Window`。在轨道空白处右键 → **Add Notify State → ET Charge Attack Window**。右键新状态条本体，在 **Notify Begin Time** 输入 **0.266667** 后按Enter。再次右键，在 **Anim Notify State Duration** 输入 **0.233333** 后按Enter。时间轴上是30 fps的第8帧至第15帧，运行约0.44秒起冲。时间输入方式与横扫相同。[横扫通知配置](M4_03B_GuardianSweep.md#21-蒙太奇)
3. 左键选中新状态条，在 **Details** 核对 **Travel Distance=240 cm、Travel Duration=0.30 s、Contact Radius=28 cm、Contact Lead=62 cm**。新蒙太奇只留这一条状态通知。预览确认收臂后进入挥出，保存。编辑器的单独动画预览用于看姿势，真实位移在PIE验证。
4. 在 `/Game/EmberTrial/Abilities` 复制 `GA_ET_GuardianSweep` 为 `GA_ET_GuardianCharge`。打开新副本，在 **Class Defaults** 设 **Attack Montage = AM_ET_GuardianCharge_01、Attack Damage = 22**。核对 Damage Effect Class=`GE_ET_LightAttackDamage`、Attack Max Horizontal Speed=3、Net Execution Policy=Server Only、Enable Combo=False。Compile → Save。

## 3. 单独配置守关者接战距离

1. 在 `/Game/EmberTrial/AI/Controllers` 复制 **BP_ETMeleeAIController** 为 **BP_ETGuardianAIController**。打开新副本，点 **Class Defaults**，按表填写。保留原黑板、行为树、感知组件和任务连接。Compile → Save。

| 字段 | 值 | 作用 |
| --- | --- | --- |
| Pursuit Stop Distance | 230 cm | 接近到突进起手距离附近 |
| Pursuit Resume Distance | 250 cm | 玩家后撤后恢复追击 |
| Melee Attack Distance | 250 cm | 进入后可停步转向并开始突进 |
| Melee Retry Delay | 0.5 s | 沿用失败重试间隔 |
| Use Ranged Attack | False | 沿用近战任务的转向、停止与取消 |

2. 打开 `BP_ETGuardian` → **Class Defaults**。设 **AI Controller Class = BP_ETGuardianAIController**，**Light Attack Ability Class = GA_ET_GuardianCharge**。守关者的180/180、两阶段速度和组件配置照旧。Compile → Save，验证新GA与新Controller，最后 **Save All**。
3. 打开 **L_ET_GuardianArena**，当前地图Play。地图仍从原生成点创建一名 `BP_ETGuardian`。普通近战继续使用原Controller和原10伤害GA。

## 4. 分组验收

隔离工程的 UE 5.5.4 Editor、Game 构建和53项 `EmberTrial` 自动化全部通过。突进专项实测空旷地面240.1 cm，身体转向后仍沿原方向，墙挡与取消后均无残留位移。下列步骤验证正式资产与键鼠表现。

1. **自动化。** 控制台依次执行 `Automation RunTests EmberTrial.M4.Guardian.ChargeWindow`、`Automation RunTests EmberTrial.M4.Guardian.Sweep`、`Automation RunTests EmberTrial.M2.Dodge.Movement`。第一项验证真实蒙太奇起冲、命中去重、墙挡与中途取消。其余覆盖横扫和玩家闪避。
2. **突进命中与躲避。** 守关者接近后停止转向。站在它正前方，让第一下命中，HUD应 **100→78**。日志有 `Charge begin`、同次22伤害与 `Charge end`。下一轮在收臂后向侧方移动，观察它沿固定方向冲过原位置。只测空旷地面时完整位移接近240 cm，遇到玩家身体或墙时实际位移缩短。
3. **挡墙与取消。** 先让守关者在开阔处锁定方向并开始收臂，再向现有出口侧墙或地图边墙侧后方移。确认守关者没有穿墙，墙后无22伤害。若现场没有合适路线，墙挡以专项结果为准。可选一次用 **P** 检查突进冻结及继续后的剩余位移，或用真实轻攻击/破势检查取消后立即停止。时点难抓可暂用 `slomo 0.5`，结束恢复 `slomo 1`。
4. **最短回归。** 打到半血确认原阶段标记仍会切换，完成一次击杀开门。Stop后新守关者恢复180。回混合遭遇地图短查普通近战10和玩家闪避。保存资产。

```text
M4-03C 完成
四新五旧源码 / 双Build / 三索引：
新AM、新GA、新Controller / 守关者两个引用 / Compile-Save-Valid：
ChargeWindow / SweepSector与SweepWindow / Dodge.Movement：
固定方向、真实22与侧移避让 / 墙挡 / P或取消：
半血转换、清场开门、Stop新180 / 普通近战10和玩家闪避：
失败或最明显占位问题 / 完成时分 / 手动时间 / 构建等待：
```

## 5. Commit / Push

| 批次 | 提交范围 | 英文建议 | 中文建议 |
| --- | --- | --- | --- |
| M4-03B验收与突进指南 | B报告、B指南回链、README、Introduction、Progress、Backlog、Architecture、AssetManifest、VersionControl、AcceptanceWorkflow_Options及本指南和补丁 | `docs: verify guardian sweep and guide charge setup` | `docs: 验收守关者横扫并补充突进配置指南` |
| M4-03C实现 | 补丁的九份Core文件，新 `AM_ET_GuardianCharge_01.uasset`、`GA_ET_GuardianCharge.uasset`、`BP_ETGuardianAIController.uasset`，更新后的 `BP_ETGuardian.uasset` | `feat(combat): add telegraphed guardian charge` | `feat(combat): 实现带前摇的守关者突进` |

四份资产走Git LFS。正式源码与资产实机通过后提交，`Artifacts` 中隔离工程、日志和构建结果保留本地。
