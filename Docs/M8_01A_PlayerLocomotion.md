# M8-01A 玩家移动与动画制作基准

本批先接通自己的玩家 AnimBP，校准原地走跑与实际移速，建立 Blender 到 UE 的出片流程。目标是在开发场真正跑起来，能观察起步、刹停、转向和空中衔接，也能在同一套图里播放原攻击与技能。该图是后续冲刺、锁定步态和战斗动画的接入基础。[M8 路线](M8_Roadmap.md) · [M7-07 发布候选](Reports/M7_07_Verification.md)

我已提取基线并完成一轮 UE → Blender → UE 的样片回导验证。你本轮从第 3 节的样片导入开始，预估手动 **60–110 分钟**，按实际 UE 配置和复拍耗时校准。

## 1. M8-01 的交付顺序

| 批次 | 可玩的交付 | 本批重点 |
| --- | --- | --- |
| **A 玩家移动基准** | 独立动画蓝图、走跑和空中状态，验证一个 Blender 原地步态样片 | 解决骨架、比例、脚底接触、根骨和 Montage 叠接 |
| **B 完整玩家移动** | 自然的起停与转身、独立持键冲刺、锁定横移和后退，回接正式玩家 | 按键、实际移速与动画一致，受击、Pause、解锁后状态恢复 |
| **C 玩家战斗动画** | 三段短刃攻击、闪避、受击、破势、治疗与死亡的正式表现 | 全身重心、左右挥击和节奏差异，按有效帧复全链路 |
| **D 敌人动作与整体验收** | 近战快慢两招，远程瞄射与退让回射，守关者三招及两阶段衔接，四类角色合测 | 起手可读、攻击有收招、响应可取消，全部通关后再进 M8-02 |

走跑源片可用于验证步幅。上线前的起步、刹停、转身、锁定步态和冲刺以 B 的实机验收为准，前景持刀与动作风格在 C 校准。M8-03 换模型后回验蒙皮、步幅和挂点，M8-04 用最终刀刃复核接触点。

## 2. 已读取的工程基线

| 项目 | 本机 UE 5.5.4 实际值 |
| --- | --- |
| 玩家与三类敌人 | `SKM_Quinn_Simple`、`SK_Mannequin`、`ABP_Quinn`。三敌人的蓝图默认 Max Walk Speed 为 220 cm/s |
| 玩家 | Max Walk Speed 为 **500 cm/s**。WASD 和方向键共用 `IA_Move`。现有映射没有左 Shift、左 Ctrl 或冲刺 Action |
| 有效插槽 | 父蓝图 `ABP_Manny` 的 `DefaultSlot` 已开启 **Always Update Source Pose**。新图延续这个行为 |
| 步态源 | `MF_Walk_Fwd` 56 帧间隔、1.8667 s，原始 root 行程 448 cm，对应 **240 cm/s**。`MF_Run_Fwd` 53 帧间隔、1.7667 s，原始 root 行程约 1,061 cm，平均约 **600 cm/s**，速度随段落变化。两者 Enable Root Motion 都关闭 |
| 玩家攻击 | `AM_ET_Light_01/02` 命中约 .267–.467 s，接续约 .600–.800 s。`AM_ET_Light_03` 命中 .200–.400 s。GA 和 Montage 仍应收同样数量的通知 |
| 其余生效点 | Q 治疗 .500 s、F 破势 .400 s、远程发射 .400 s。守关者三招还设有 Montage Rate Scale，后续用游戏实际时刻换算 |

上述步态行程是源片尺度。跑位片的平均速度作为参考，正式循环的参考速度取裁定后的完整周期。以支撑脚相对于地面的滑动量校准播放速率，避免只按平均速度拉伸加速段。[UE 5.5 Blend Space](https://dev.epicgames.com/documentation/en-us/unreal-engine/blend-spaces-in-unreal-engine?application_version=5.5) · [动画同步组](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-sync-groups-in-unreal-engine?application_version=5.5)

## 3. 首批制作和接入

### 3.1 导入已生成的 Blender 样片

1. 用 Blender 5.3.0 打开工作文件 `Artifacts/M8_01A/Quinn_WalkInPlace.blend`，时间轴为 **30 fps / 1–57**。视口里有 Quinn 预览网格，活动 Action 是 `AS_ET_PlayerWalk_BlenderSample`，源 Action `MF_Walk_Fwd_UE_Reference` 也在文件内。保持活动 Action，按 Space 播放，确认角色原地走动。Blender 将这次 UE 导出的 `root` 解析为 Armature 对象，根位移记录在该对象的 Location 通道，脚骨和手骨仍是其下的骨骼。更换工作片时先检查根轨道的归属。
2. UE Content Browser 建立下表目录后，在 `Clips` 中点 **Import**，选择 `Artifacts/M8_01A/AS_ET_PlayerWalk_BlenderSample.fbx`。选动画导入，**Import Mesh 关闭**，**Import Animations 打开**，Skeleton 指向已有 UE5 `SK_Mannequin`，采样 30 fps、Scale 1。若只能看到 Interchange 导入面板且无法选择旧骨架，在 UE 底部 **Output Log** 查询并记下 `Interchange.FeatureFlags.Import.FBX`，输入 `Interchange.FeatureFlags.Import.FBX 0` 后重新 Import。完成后把该变量设回原值。本机回导验证采用 UE 5.5.4 的旧 FBX 导入器。
3. 打开导入的 `AS_ET_PlayerWalk_BlenderSample`，Preview Mesh 选 `SKM_Quinn_Simple`。时间约 **1.867 s**，**56 个帧间隔 / 57 个采样点**，Enable Root Motion 关闭。分别看首尾和中间支撑步，骨架、大小、朝向与 UE 源片应一致。该样片来自 UE 的走路源片，在 Blender 固定根骨平移，是后续创作的技术样片。将它用作第 3.3 节走路位置的样本。

本机已在新 Blender 进程重开工作文件，活动 Action 与原地轨道保存正确。后台回导只使用未保存包。UE 确认原骨架、161 条骨骼轨道、原时长和 0 cm 根骨平移。9 个采样时刻中，骨盆、右手、双脚的局部旋转差均小于 0.001°，局部平移差最大约 2.7 cm，出现在骨盆。实际蒙皮与脚底表现由第 5 节实机复核。证据留在本地 `Artifacts/M8_01A` 的 `Baseline.json`、`BlenderWalk.json`、`BlenderReopen.json` 与 `UERoundTrip.json`。[UE 5.5 FBX 动画导入](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-animation-pipeline-in-unreal-engine?application_version=5.5)

**新动画资产的位置**

| 内容 | 路径与命名 |
| --- | --- |
| 玩家动画蓝图 | `/Game/EmberTrial/Animations/Player/ABP_ET_Player` |
| 移动 Blend Space 1D | `/Game/EmberTrial/Animations/Player/Locomotion/BS_ET_PlayerGrounded` |
| 独立样片和移动素材 | `/Game/EmberTrial/Animations/Player/Locomotion/Clips`，动画源保持 `SK_Mannequin` |
| 隔离运行动画的玩家和 GameMode | `/Game/EmberTrial/Tests/Animation/BP_ETLocomotionProbe` 和 `BP_ETLocomotionGameMode`。如果先用开发图临时配置，验证后还原地图 Override |

### 3.2 配置全新的动画蓝图

1. 在 Content Browser 进入 `Animations/Player`。右键 **Animation → Animation Blueprint**，选择工程里的 UE5 `SK_Mannequin` 和父类 **AnimInstance**。打开后在 Preview Scene Settings 将 Preview Mesh 设成 `SKM_Quinn_Simple`。骨架完整路径是 `/Game/Characters/Mannequins/Meshes/SK_Mannequin`。
2. 在 My Blueprint 新建三个变量。Float `GroundSpeed` 和 `VerticalSpeed` 都默认 0，Boolean `bIsFalling` 默认 False。Event Graph 中找到 **Event Blueprint Update Animation**，执行接 **Is Valid?**，其 Input Object 接 **Try Get Pawn Owner**。Valid 白线接 **Cast To Character**，Object 同样接 Pawn Owner。
3. 从 **As Character** 拉出 **Get Velocity**。用 **Vector Length XY** 的返回值设 `GroundSpeed`。**Break Vector** 的 Z 接 `VerticalSpeed`。从 **As Character → Get Character Movement → Is Falling** 取 Bool 设 `bIsFalling`。三个 Set 的白线顺序为 GroundSpeed → VerticalSpeed → bIsFalling。Is Not Valid 和 Cast Failed 可一起接自定义函数 `ClearMotionReadout`，按同样顺序把值清回 0、0、False。该图只读取运动快照。
4. 在 AnimGraph 右键添加名为 `Locomotion` 的 **State Machine**，其 Pose 接 **Slot 'DefaultSlot' → Output Pose**。选中 Slot，在 Details 确认 Slot Name 为 `DefaultGroup.DefaultSlot`，打开 **Always Update Source Pose**。
5. 在 Class Defaults 搜索 **Root Motion Mode**，选 **Root Motion from Montages Only**，保证普通移动片不会推动胶囊。保留原有使用 CharacterMovement 和 Root Motion Source 的闪避与突进。Compile。双击 Locomotion 创建下一节的状态。

### 3.3 Blend Space 与状态转移

1. 先按第 4 节第 1 点准备三条序列。在 `Player/Locomotion` 创建 **Blend Space 1D**，使用同一 UE5 骨架。Horizontal Axis 名称 `GroundSpeed`，Min 0、Max 500，Smoothing Time 初值 **0.08 s**。拖入待机、原地走和原地跑三个样本，分别落到 **0、240、500**。精确值在样本的设置中填写。逐点预览 0、240、500，再缓慢横扫观察踝关节。
2. 在 State Machine 添加 `Grounded`、`JumpStart`、`FallLoop`、`Land`。将 Entry 指向 Grounded。Grounded 内放上一步 Blend Space Player，Speed 接 `GroundSpeed`，Pose 到该状态 Output。JumpStart、FallLoop、Land 各放单条序列，首轮可用现有同骨架 `MM_Jump`、`MM_Fall_Loop`、`MM_Land`，只开启 FallLoop 的 Loop Animation。
3. 从状态框外缘拖一根转移线到目标状态，双击箭头编辑规则。把下表的布尔表达式接到 **Can Enter Transition / Result**。靠近地面的平台边缘也要能走进 FallLoop。

   | 从 → 到 | 条件 | 初始 Blend |
   | --- | --- | --- |
   | Grounded → JumpStart | `bIsFalling && VerticalSpeed > 30` | .08 s |
   | Grounded → FallLoop | `bIsFalling && VerticalSpeed <= 30` | .08 s |
   | JumpStart → FallLoop | `bIsFalling && VerticalSpeed <= 0` | .08 s |
   | JumpStart → Land | `!bIsFalling` | .07 s |
   | FallLoop → Land | `!bIsFalling` | .07 s |
   | Land → Grounded | `!bIsFalling && (Time Remaining (Ratio) < 0.2 || GroundSpeed > 180)` | .10 s |
   | Land → JumpStart | `bIsFalling && VerticalSpeed > 30` | .05 s |
   | Land → FallLoop | `bIsFalling && VerticalSpeed <= 30` | .05 s |

4. Land → Grounded 的 Time Remaining (Ratio) 在转移规则图中右键搜索并选择 `MM_Land` 对应的资产播放器比率节点。先以 .2 为退出阈值。跑动落地过硬时观察接触帧与 Blend，再调整速度分支或先补移动落地片。落地动作不消耗输入，地面判定由现有 CharacterMovement 持有。
5. **Compile → Save**。在 Debug Filter 选择实际的 Probe，观察 State Machine、三个变量和 Slot。确认攻击期间还有底层姿态更新。后续动画更新若成为性能问题，用 Insights 核对再迁为线程安全读值。

### 3.4 只在开发图使用新的玩家 AnimBP

1. 右键现有 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` → **Create Child Blueprint Class**，把派生类放进 `Tests/Animation` 命名 `BP_ETLocomotionProbe`。在子类选 **Mesh (CharacterMesh0)**，Details 的 **Animation Mode** 选 **Use Animation Blueprint**，**Anim Class** 赋 `ABP_ET_Player`。Mesh 和右手挂载继承父类。Compile / Save。
2. 打开 `L_ET_VerticalSlice`，先抄下当前 **World Settings → GameMode Override** 和默认 Pawn。以实际使用的 GameMode 蓝图创建一个子类 `BP_ETLocomotionGameMode`，在子类 Class Defaults 把 **Default Pawn Class** 设 Probe。Player Controller 等沿用父类配置。仅在开发图临时设 GameMode Override 为该子类。
3. 在 **Selected Viewport** 或 **New Editor Window (PIE)** 运行。Spawn 用 Default Player Start。检查 Debug Filter 显示 `BP_ETLocomotionProbe`，并看自己创建的图随跑、跳和攻击变化。Stop 后恢复开发图原 GameMode Override，再 Compile／Save 用于保留的测试资产。正式路线的玩家类与地图不需要保存修改。

## 4. 原地循环、同步和速度校准

1. 从同一骨架验证的参考片出发。在 UE 中 Duplicate `MF_Idle` 和 `MF_Run_Fwd` 到 `Clips`，分别命名 `AS_ET_PlayerIdle_Base`、`AS_ET_PlayerRun_Base`。走路沿用已回导的样片，另存副本后调整时长或标记。在新副本的 Asset Details 搜索 Root Motion，关闭 **Enable Root Motion**，对带根骨平移的跑步副本打开 **Force Root Lock**，**Root Motion Root Lock = Anim First Frame**。首次预览先看全长，再裁取同一脚相位到下一次同脚相位的连续周期。
2. 以左脚与右脚首次承重为标记，在走跑副本各标 `L_Plant` 和 `R_Plant`。实际帧以脚掌接触与身体重心转移为准。两条有效循环保持相同标记名和循环顺序。通过 Blend Space 预览看混合区是否换脚急抖，避免把一个循环的抬脚帧对到另一个循环的支撑帧。[UE 动画同步组](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-sync-groups-in-unreal-engine?application_version=5.5)
3. 先沿同一条地面直线录正侧面，对照 Debug 中的 GroundSpeed。取一个周期，在站立支撑期间用相同世界地标看脚掌位置。若跑片按实际裁出的速度为 600 cm/s，500 cm/s 的 Rate 初值为 **0.833**。若实际周期测得 550 cm/s，改为 **0.909**。Rate 由所选周期的速度算出。目标是支撑脚不持续滑过地标，起步和停步重量自然，不追求与模板数字相等。
4. 在同一个平坦地面复测站定、首步触地、完整走跑周期、换相和落地。实机正侧面检查连续三个周期，两个脚的承重帧分别与地面地标对齐，目标是每个支撑段可见滑移约 **3 cm 以内**。原地走路的实机检查可以在 Probe 的 Character Movement 中临时设 Max Walk Speed **240**，检查后恢复继承默认值 **500**。常规越肩镜头另录一段。若有明显跨地标滑移，保留视频和实际 Rate，优先回到第 3 点校准，再做起停与转向的正式过渡。

## 5. 验收与反馈

| 分组 | 操作 | 通过条件 |
| --- | --- | --- |
| 制作链 | Blender 重开本地源；在 UE 原骨架预览导入的走路样片 | 骨轴方向正常、比例正确、循环不弹、根骨留在原点，使用原 `SK_Mannequin` |
| 移动表演 | Probe 依次走直线、松键急停、掉头、跑中跳跃、落地立刻移动、原地连跳一次 | 站定和移动姿态连续，空中不播走跑。记录侧面可见的支撑脚滑动和最不自然的过渡 |
| 动作接口 | 待机左键连段，2 闪避，受到一次伤害后站定按 Q，站定按 F；其中一次动作中按 P 再继续 | DefaultSlot 仍在同一更新链，一次动作或攻击窗口只产生原有结果。暂停继续无额外执行 |
| 现有回归 | 跑 `EmberTrial.M2.Combo.Flow`、`EmberTrial.M2.Dodge.Movement` 各一次；恢复开发图 Override，实际进原正式图走、跳、左键攻击一次 | 自动化 Success，当前正式角色仍走原动画链，Stop 重开无新错误 |

反馈附一次 **越肩实机短录屏**，条件允许再留一次同路线侧视录像。遇到报错，优先给完整 `Output Log` 文字，包括关卡、播放方式、最早一条错误和复现键序。动画错位或接线问题给截图／短视频，并注明帧或状态名。

```text
M8-01A 完成
Blender 源重开 / UE 样片资产路径与 Skeleton：
Probe ABP / Root Motion Mode / DefaultSlot：
走跑样本实际选段、Rate、观察到的支撑脚滑动：
起停、转向、跳跃和跑中落地：
左键连段 / 2 / Q / F / P 复验：
Combo.Flow / Dodge.Movement：
开发图原 Override 已恢复 / 正式图短查：
短录屏路径 / 最明显待改动作：
完成时分 / 实际手动分钟：
```

## 6. Commit / Push

文档可先提交本操作指南、M8 路线、进度和看板。实机通过后提交可分发的 AnimBP、Blend Space、移动剪辑和两份独立测试蓝图，并同步资产清单。由 UE 模板片生成的样片，发布范围随原资产许可确认。开发图恢复后检查 Git 差异，提交范围以实际保留的资产为准。模板参考 FBX、Blender 工作文件和对比视频留在本地 `Artifacts/M8_01A`。

- 英文 `feat(animation): establish player locomotion and animation authoring baseline`
- 中文 `feat(animation): 建立玩家移动动画与动作制作基准`
