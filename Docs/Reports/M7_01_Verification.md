# M7-01 灰烬短刃试做验收

**通过。** 灰烬短刃已导入 UE 5.5.4，正式地图中的右手挂载和原连段通过复验。用户于 2026-11-16 20:53 完成键鼠验收，报告手动 25 分钟，其中模型与导入 15 分钟，对位与实机 10 分钟。[操作指南](../M7_01_CinderBladeBlenderTrial.md)

| 检查 | 结果 |
| --- | --- |
| Blender 源件与 FBX | 使用 Blender 5.3.0（`E:/Blender/blender.exe`）。`SourceAssets/Weapons/CinderBlade/SM_ET_CinderBlade.blend` 重开和同名 FBX 回读通过。单网格，XYZ 为 8.8 × 3.4 × 56 cm，握柄中心为原点，276 个三角形，三个材质槽，1 套 UV。 |
| UE 网格与材质 | `/Game/EmberTrial/Art/Weapons/CinderBlade/SM_ET_CinderBlade` 已保存。LOD0 为 276 个三角形，Nanite 关闭，简单碰撞及凸包均为 0。材质为 `M_ET_BladeSteel`、`M_ET_BladeWrap`、`M_ET_BladeEmber`。 |
| 玩家右手 | `BP_ThirdPersonCharacter` 编译并保存。单个 `CinderBladeMesh` 挂到 `CharacterMesh0 → hand_r`。局部 Location（cm）为 `-7.556183 / 1.515425 / -0.376300`，Rotation（°，Roll / Pitch / Yaw）为 `10.741906 / -13.116233 / 178.911335`，Scale 为 `1 / 1 / 1`。组件为 NoCollision，Overlap、物理模拟及导航影响关闭。 |
| 持握与镜头 | 助手在 `L_ET_TrialRoute` 两次取样出生状态，并在原三段轻击播放时逐段核对。武器均只有一份，手骨内的偏移一致。用户确认待机、移动、三段轻击、闪避、锁定和 P 继续、Stop 重开均完成，未报告影响验收的穿插、遮挡或失败。助手截图中可见徒手动画的手指略张开，默认待机镜头里右臂遮住大部分刀身。 |
| 真实伤害 | 实机日志在 20:49:01–20:49:06 记录三次独立轻击各造成 25 点伤害，首个近战敌人生命依次为 100 → 75 → 50 → 25，每次伤害来自不同 Trace Serial。用户确认已做刀尖对照，反馈未记录单独一次刀尖掠过时的命中或落空结果。当前命中范围仍由原 25 cm 手骨扫掠决定，保留这组日志作为后续对照基线。 |
| 连段回归 | 20:49:38，`EmberTrial.M2.Combo.Flow` 返回 **Success**。自动化中的真实三段攻击各产生独立 Trace，测试目标 1000 → 925。 |

本地证据位于 `Artifacts/M7_01`。原始 FBX 回读对照为 `CinderBlade_Validation.json`，挂载取样为 `UEBladeGripRuntime.json`，筛选后的 UE 日志为 `Verification.log`。截图沿用本批已保存的 `BladeGrip_Side.png`（近景）和 `BladeRuntime_Combo02.png`（常规第三人称轻击）；用户未另附截图路径。

源件、UE 网格、三个材质和玩家蓝图已提交。七份本机资产的 SHA-256 与资产清单及 Git LFS 指针一致。核对时本地 `main` 与本地记录的 `origin/main` 均指向资产提交 `676e8fc`。

本次暂存 `Docs/Reports/M7_01_Verification.md`、`Docs/M7_01_CinderBladeBlenderTrial.md`、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。日志与截图留在本地 `Artifacts/M7_01`。

- 英文 `docs(art): record cinder blade grip and combat verification`
- 中文 `docs(art): 记录灰烬短刃持握与战斗复验`
