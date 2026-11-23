# M8-01B 玩家移动与锁定步态

M8-01A 的走跑基准已经通过实机验收。本批把移动接成一套可从游戏镜头辨认的表现。自由移动增加起步、跑停、反向转身和持键冲刺。锁定移动增加朝目标的前后左右步态。制作时使用同一个测试场和玩家骨架，最后用正式路线短测整套移动，再进入战斗动作。[M8-01A 基准](M8_01A_PlayerLocomotion.md) · [M8 路线](M8_Roadmap.md)

## 1. 游戏里的行为

| 操作 | 行为 | 动画设计 |
| --- | --- | --- |
| WASD 起步、继续、松开 | 自由移动上限 **500 cm/s**。转向继续跟随移动输入。 | 起步先转移重心再迈出第一步。松键后按实际制动收步，重按可以直接起步或接回循环。 |
| 跑动中急转约 180° | 方向变化由 CharacterMovement 处理，初始转身触发阈值为速度 **200 cm/s**、方向差 **110°**。 | 选左右转身，先以支撑脚减速再换向。超过阈值只触发一次，等方向变化完成再允许下一次。 |
| 地面自由移动时持左 Shift | 冲刺上限初值 **700 cm/s**。松 Shift 回到普通跑，松方向键收步。 | 起步更低更前倾。冲刺循环步幅比普通跑更大，持刀手收在身体近侧。从高速松键时使用独立的冲刺收步。 |
| Tab 锁定后 WASD | 角色面向当前有效目标。方向输入继续按相机坐标计算，最大速度初值 **300 cm/s**。 | 前进、后退、左右横移各有独立步态。A/D 绕目标时，胸口方向和脚的行进方向可以明显区分。 |
| 跳跃、攻击、闪避、治疗、破势、受击或暂停 | 进入已有动作或暂停流程时终止冲刺。恢复后要重新按 Shift。跳起沿用实际起跳速度，落地后按普通移动规则接续。 | 原地落地用 Land，高速落地优先接走跑。全身 Montage 通过现有槽位接管表演，结束后接当前移动状态。 |

冲刺按位移状态实现，可以在站定时按住 Shift 后再给方向。首轮冲刺速度不绑定体力消耗，原有闪避继续独立处理资源。锁定时按住 Shift，解除锁定后需重新按 Shift。

## 2. 已核对的接入点

- `AEmberTrialCharacter::Move` 从 Controller 的 Yaw 得到前、右方向，并调用 `AddMovementInput`。移动与跳跃共用 `CanAcceptLocomotionInput`。正式动作通过 GAS 查询，旧练习动作仍参与门控。
- `AETPlayerController::UpdateRotation` 和 `UETTargetLockCameraComponent` 控制锁定镜头。身体朝向用 `UETTargetLockComponent::GetLockedTarget` 所返回的当前目标确定。越肩镜头包含横向偏移和 Yaw 调整。
- `UETFailureFlowComponent` 管理暂停／失败 UI。在恢复游戏时调用 `FlushPressedKeys`。冲刺意图须在暂停完成时主动撤销。也在换控、EndPlay、死亡及动作门控失效时清理。
- 运行测试用 `/Game/EmberTrial/Tests/Animation/L_ET_LocomotionProbe`。正式接入时只替换玩家的 Anim Class、输入映射和移动配置。复用现有 BP 父类、默认 GameMode、左右手挂点和技能引用。

## 3. 制作与速度校准

### 3.1 冲刺循环

本地冲刺工作文件 `Artifacts/M8_01B/Quinn_SprintSample.blend` 保留 `AS_ET_PlayerRun_Reference` 和 `AS_ET_PlayerSprint_Sample` 两条 Action。样片以已验收的跑步循环为参考，在 Blender 重做骨盆和躯干前倾、右持刀手姿态与双腿触地目标。已回导并保存到 `/Game/EmberTrial/Animations/Player/Locomotion/Clips/AS_ET_PlayerSprint_Sample`，采用原 `SK_Mannequin`，30 fps，17 个帧间隔。准备接入时以该片作为冲刺循环。

1. 在 Blender 切换两条 Action，用同一个正侧视角看首尾、躯干角度、右肘离躯干距离以及足尖伸展。确认双腿支撑轮换接得上，脚底没有穿透地面。
2. 在 UE 打开回导后的序列，Preview Mesh 用 `SKM_Quinn_Simple`。`Enable Root Motion` 关闭，Force Root Lock 采用 Anim First Frame。以第 0 帧为起点，`L_Plant` 在第 4 帧，`R_Plant` 在第 12 帧。用于冲刺循环的测试初值为 **1.10×**，播放倍率只设在一个地方。
3. 先以 700 cm/s 做三周期边走边录。量测承重脚掌在世界坐标中的水平跨度，以 **约 3 cm 以内**作为平地匀速目标。再分别看加速过渡、松 Shift 保持 W、松开方向键收步。每个问题都记录在正确片段上，避免用整条片全局加速调整某一个脚的触地时间。

### 3.2 起步、收步、转身与锁定

| 资产 | 制作依据 | 接缝检查 |
| --- | --- | --- |
| `AS_ET_PlayerRun_Start`、`AS_ET_PlayerSprint_Start` | 以待机首帧、身体前倾、第一脚卸重、首个有效承重、进入循环同脚相位为关键姿势。启动和起跳时按不同边界选择状态。 | 首周期从匹配的支撑脚进入，同一个首步持续按键只播放一次。 |
| `AS_ET_PlayerRun_Stop`、`AS_ET_PlayerSprint_Stop` | 采实际松键到速度低于 30 cm/s 的时间。以速度仍高时的一次制动落脚为主体，再收重心回待机。 | 以 500/700 cm/s 与当前 2000 cm/s² 制动估算 **0.25/0.35 s** 为起点，按实际曲线校正。反向重按按实时输入退出收步。 |
| `AS_ET_PlayerPivot_L/R` | 各做一条独立侧向转身片。起手先屈膝支撑，髋先转，胸跟上，另一脚跨到新方向。 | 根骨留在原点。在 PIE 对照角色 Yaw 变化。旋转由移动组件推动，脚不能在转向尾段与胶囊脱节。 |
| `BS_ET_PlayerLocked` 与四向片 | 轴用身体本地前后与左右速度，范围从 −300 到 +300 cm/s。中心为待机，四个轴端放 300 cm/s 下的原地循环。前向可从走步片开始校准。后撤用较短的落脚与持续面向。横移按侧跨、收脚和侧向承重点制作。 | 八方向实测。斜向合成有持续叠脚、镜像刀手错误或脚底急跳时，补对应斜向片。 |

继续使用 30 fps 和当前 Quinn Reference Pose。在 Blender 为每条新片留独立 Action，导出动画，逐条在 UE 比对比例、根骨、首尾和手中短刃。持刀的左、右方向分别核对右手，不对带武器的整套动作直接镜像。

## 4. 程序与动画蓝图的接入

1. 新建 Boolean `IA_ET_Sprint`，绑定 **Left Shift**。测试时给 Probe 使用继承了本批之前全部键位的测试 IMC，在 Probe 上明确填写冲刺 Action。先用 Started／Completed／Canceled 核对正常按下和释放，暂停、切目标和 Retry 后核对无陈旧输入。
2. 在原生 CharacterMovement 的速度入口计算冲刺和锁定上限。玩家功能使用显式启用配置并默认关，先仅在 Probe 启用。按住 Shift 的输入意图、已有动作门控、脚下移动模式及锁定目标共同决定有效冲刺。用组件的基础速度作为普通移动结果，避免直接写入和事后恢复共享 `MaxWalkSpeed`。测试启用前后普通移动上限仍为 500。
3. 锁定接管身体 Yaw 时，从 Pawn 到当前目标求平面方向并按旋转速率更新，超近位置保留上次稳定朝向。解除后回归原生自由转向。仅在玩家有效受控、地面且允许移动时应用。已有能力的瞬时方向、位移及攻击窗口由各自链路处理。
4. 给玩家 AnimBP 提供只读运动快照，包括当前速度、期望移动方向、身体局部前／侧速度、局部角差、锁定目标有效性、有效冲刺和地面状态。基于状态转移控制起步／停步／转身，按速度阈值留少量迟滞，使用资产剩余时间退出一次性动画。用已有 Grounded、JumpStart、FallLoop、Land 接空中路径，保留 `DefaultSlot` 和 Always Update Source Pose。
5. Probe 通过后将已保存的 AnimBP、玩家专用设置和冲刺映射应用于正式玩家。普通近战、远程、守关者、人偶分别确认原有速度与动画。正式地图走到混合遭遇锁定两类敌人，解除后冲刺至祭坛，继续依次测动作中断、Retry 和菜单 Continue。

## 5. 验收与反馈

| 分组 | 复现 | 通过条件 |
| --- | --- | --- |
| 起停与转向 | 步行转跑、松键后重按、跑中反向、连续左右改变方向 | 起步与制动可辨，方向响应及时。相反输入可以打断收步。无反复触发和脚底横漂。 |
| 冲刺 | 左 Shift+W，松 Shift 保持 W，重按后松 W，长按 Shift 跳跃和落地 | 侧视可区分普通跑与冲刺，地面稳定速度与配置一致。起步、循环、收步都可观察。 |
| 锁定 | Tab 锁敌，前后左右和四个斜向，切换目标，贴近再拉开，解除 | 身体面向稳定，镜头维持现有锁定结果。后撤和横移片可辨，近距不急转。 |
| 中断与流程 | 冲刺中执行左键、2、Q、F、真实受击。按住 Shift 时暂停、死亡 Retry 及 Continue | 运动状态正确恢复。攻击窗口、闪避方向和技能资源仍符合现有规则。 |
| 原能力回归 | `EmberTrial.M2.Combo.Flow`、`EmberTrial.M2.Dodge.Movement`、`EmberTrial.M2.TargetLock.Selection`、`EmberTrial.M2.TargetLock.CameraRules` | 现有自动化通过。Stop 重开、击败目标后的解锁及三类敌人移动正常。 |

留一次侧视与一次游戏镜头录像，附拖影、脚滑、穿插中最明显的一处所在动作和时间点。

```text
M8-01B 完成
Blender Action / UE 动画与骨架 / Saved：
起停与180°反向 / 最明显的过渡：
Shift冲刺、放键与起跳 / 实际速度与播放倍率：
锁定四向与斜向 / 切目标、贴近与解锁：
左键-2-Q-F、受击与Pause / Retry与Continue：
自动化结果 / 正式图短查 / Side与游戏镜头路径：
完成时分 / 手动分钟：
```

## 6. Commit / Push

实现和验收完成后提交改动的玩家代码、输入映射、玩家动画、测试图用配置及本指南。参考导出、Blender 工作文件和录屏位于本地 `Artifacts/M8_01B`。

- 英文 `feat(animation): add sprint transitions and directional lock-on locomotion`
- 中文 `feat(animation): 加入冲刺过渡与锁定方向步态`
