# M8-01B 玩家移动与锁定步态验收

**通过。** 用户于 **18:14** 完成验收，手动 **60 分钟**。Blender Action、UE 动画与骨架、起停与反向、冲刺与起跳、锁定八向与切换目标、能力中断及 Retry／Continue 均正常。[操作指南](../M8_01B_PlayerMovement.md)

| 检查 | 结果 |
| --- | --- |
| 正式玩家配置 | `BP_ThirdPersonCharacter` 已启用玩家步态并引用 `ABP_ET_Player`。`IMC_Default` 通过唯一的左 Shift 映射触发 `IA_ET_Sprint`。 |
| 移动配置 | 正式玩家与 Probe 的移动参数一致。自由跑、冲刺、锁定速度上限分别为 **500、700、300 cm/s**。 |
| 正式地图依赖 | 主菜单、试炼路线及玩家入口沿硬引用和软引用检查了 **269 个项目资产包**，测试地图、测试 GameMode 和测试 IMC 均未进入正式依赖。 |
| 正式路线 PIE | 从主菜单进入 Entry。左键对近战目标造成 25 点伤害，Q 恢复至 100，F 播放 Montage。冲刺暂停后恢复普通跑，闪避清理冲刺状态。混合遭遇切锁与祭坛 E 通过。死亡 Retry 和菜单 Continue 均在 Checkpoint_A 出生。 |
| 原有自动化 | 连段、闪避位移、锁定选择、锁定镜头 **4 项通过**。 |

本机录屏为 `Artifacts/M8_01B/PlayerMovement_Side.mp4` 和 `Artifacts/M8_01B/PlayerMovement_Lock.mp4`。本次复验使用正式玩家和正式试炼地图。

## Commit / Push

本次提交包含本验收报告、进度、待办和 M8 路线。

- 英文 `docs(animation): record M8-01B player locomotion acceptance`
- 中文 `docs(animation): 记录 M8-01B 玩家移动与锁定步态验收`
