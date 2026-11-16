# M7-03 玩家与三类敌人的材质区分

用护甲色与明度区分玩家、近战守卫、远程守卫和守关者。四者使用同一套 Quinn 网格与动画，通过独立材质实例配色。本批已完成蓝图配置与实机复验，手动 **35 分钟**。[验收报告](Reports/M7_03_Verification.md)

## 1. 已生成的角色材质

在 Content Browser 打开 `/Game/EmberTrial/Art/Characters/Palettes`。八份 `MI_ET_*` 已保存，每个角色两份，`_01` 继承 `MI_Quinn_01`，`_02` 继承 `MI_Quinn_02`。两套材质的贴图与 UV 对应关系随各自父材质继承，两个槽要按编号分别设置。

| 角色 | 护甲外观 | `Quinn_01` 线性 Tint | `Quinn_02` 线性 Tint | 原胸部徽记着色 |
| --- | --- | --- | --- | --- |
| 玩家 | 冷蓝灰护甲，暖琥珀点缀 | .34 / .53 / .66 | .54 / .71 / .80 | .95 / .49 / .15 |
| 近战守卫 | 明铜色护甲，暖红点缀 | .62 / .27 / .09 | .84 / .44 / .16 | .93 / .23 / .07 |
| 远程守卫 | 灰绿护甲，浅黄绿点缀 | .17 / .37 / .22 | .32 / .63 / .43 | .72 / .83 / .41 |
| 守关者 | 深酒红护甲，浅金点缀 | .23 / .055 / .095 | .47 / .13 / .20 | 1 / .66 / .26 |

徽记的三层 `LogoLayer*_Brightness` 统一设为 **1.25 / 2.5 / 5**，亮度从原模板的高发光值收敛到适合辨识的范围。上表为参数值，正式地图中的曝光、敌人识别距离和战斗表现以第 3 节为准。

保存的蓝图已只读核对，四者均为 `SKM_Quinn_Simple`、`SK_Mannequin`、`ABP_Quinn`，材质槽依次为 `Quinn_01` 和 `Quinn_02`。材质生成工具为 `Tools/Build-CharacterPalette.py`，只创建缺少的项目材质实例。八份资产已在独立 UE 进程回读，父材质、Tint、徽记色和亮度均一致。证据留在本地 `Artifacts/M7_03/CharacterVisualsBefore.json` 和 `Artifacts/M7_03/VerifyCharacterPalette.log`。

## 2. 在四份角色蓝图设置材质覆盖

1. 在 Content Browser 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`。左侧 **Components** 选继承的 **Mesh (CharacterMesh0)**。玩家蓝图同时有 `CinderBladeMesh`，这一步应选名字为 **Mesh** 的骨骼网格组件。
2. 在 Details 搜索 **Materials**，找到 Mesh 的材质列表。**Element 0（Quinn_01）** 设置为 `MI_ET_Player_Quinn_01`，**Element 1（Quinn_02）** 设置为 `MI_ET_Player_Quinn_02`。用下拉框搜索完整名称，避免选中模板的 `MI_Quinn_01`、`MI_Quinn_02`。点 **Compile → Save**。
3. 打开 `/Game/EmberTrial/Characters/Enemies` 下的三份蓝图。每份重复同样的 **Mesh → Materials → Element 0 / 1 → Compile → Save**。

   | 蓝图 | Element 0 / Quinn_01 | Element 1 / Quinn_02 |
   | --- | --- | --- |
   | `BP_ThirdPersonCharacter` | `MI_ET_Player_Quinn_01` | `MI_ET_Player_Quinn_02` |
   | `BP_ETMeleeEnemy` | `MI_ET_Melee_Quinn_01` | `MI_ET_Melee_Quinn_02` |
   | `BP_ETRangedGuard_Native` | `MI_ET_Ranged_Quinn_01` | `MI_ET_Ranged_Quinn_02` |
   | `BP_ETGuardian` | `MI_ET_Guardian_Quinn_01` | `MI_ET_Guardian_Quinn_02` |

4. 重开四份蓝图各看一眼 **Viewport**，确认两槽都有新材质，肩、背和手臂可辨，无默认灰格或错位纹理。Mesh 资产、Anim Class、相对变换和玩家右手的 `CinderBladeMesh` 按第 1 节的基线使用。通过蓝图组件覆盖后，来源于这四个 class 的遭遇生成和 Retry 也会继承对应材质。
5. 如果有半身颜色异常或纹理对不上，先按表核对 **Element 0 → _01**、**Element 1 → _02**，再在正确的材质实例中核对 **Parent**。如果四份颜色在场景光下靠得太近，留同一位置和视角的截图，后续按实测调各角色的 `Tint`。

## 3. 分组验收

2026-11-16 已通过。两槽配置、混合遭遇同框、守关者二阶段与两项自动化均已核对。[日志与截图路径](Reports/M7_03_Verification.md)

| 组别 | 操作 | 通过条件 |
| --- | --- | --- |
| 玩家和第一名近战 | 在 `L_ET_TrialRoute` 从 Entry 开始，看玩家背甲与短刃，然后接近首名铜色近战。实际打一段轻击，再闪避一次。 | 玩家与近战易于区分，短刃仍随原右手，攻击有伤害，闪避和受击表现按原逻辑。 |
| 混合遭遇 | 到 A 后进入第二场，或用可用 A 点存档直接接近。站在能同时看到两名敌人的位置，锁定其中一名，观察铜色近战接近与灰绿远程准备射击。 | 实际第三人称距离能区分两类敌人。正常看见追击和远程准备动作，锁定标记仍跟随目标。留一张同框截图到 `Artifacts/M7_03`。 |
| 守关者与重开 | 继续到 B，或者用可用 B 点存档直接进守关者房。确认深酒红外观；打到半血或用专项验证原阶段。Stop 后重新 Play，确认玩家配色与短刃仍在。 | 守关者外观可辨，阶段标志或切换行为正常。新一次 Play 不出现模板材质回退。 |
| 自动化 | Stop 后，在 Output Log 分别执行下列命令，上一条出结果后再执行下一条。 | 两项返回 `Success`。 |

```text
Automation RunTests EmberTrial.M2.Combo.Flow
Automation RunTests EmberTrial.M4.Guardian.Phase
```

```text
M7-03 完成
八份MI / 四份蓝图的两槽覆盖 / Compile-Save重开：
玩家与近战区分 / 原短刃、攻击伤害与闪避：
第二场两敌同框 / 锁定 / 远程准备动作：
守关者外观与阶段复查方式 / Stop再开：
Combo.Flow / Guardian.Phase：
截图路径 / 最难分辨的角色或其他问题（无填无）：
完成时分 / 配置分钟 / 实机检查分钟：
```

## 4. Commit / Push

### 4.1 文档单独提交

暂存本指南、`Docs/Backlog.md`、`Docs/Progress.md` 和 `Introduction.md`。含实物哈希的 `Docs/AssetManifest.csv` 随材质资产提交。

- 英文 `docs(art): document Quinn character palette setup and checks`
- 中文 `docs(art): 记录 Quinn 角色配色配置与复验步骤`

### 4.2 材质与蓝图接入

暂存 `Tools/Build-CharacterPalette.py`、`Docs/AssetManifest.csv` 和 `EmberTrial/Content/EmberTrial/Art/Characters/Palettes` 内的八份实例。蓝图配置与验收后，将表中的四份角色 `.uasset` 与材质一并提交。截图与日志留在本地 `Artifacts/M7_03`。

- 英文 `feat(art): distinguish the player and enemies with Quinn palettes`
- 中文 `feat(art): 为玩家与敌人配置可区分的 Quinn 材质配色`

### 4.3 验收文档

材质与蓝图已随 `82a58cd` 提交。本次补交验收记录。[暂存清单](Reports/M7_03_Verification.md#本次文档-commit--push)

- 英文 `docs(art): record character palette and combat verification`
- 中文 `docs(art): 记录角色材质区分与战斗复验`
