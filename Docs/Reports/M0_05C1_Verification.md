# M0-05C1 排障与交接记录

更新：2026-09-27。用户确认修复后功能恢复；磁盘连线复验通过。C1 全部边界项尚待明确反馈，不能仅据单次播放恢复记为全部通过。当前可以进入不替换既有输入的 C2-01 组件准备。

## 故障原因和修复

现象：站定后按 1/2/3 无动作、无成功打印、无运行错误。

2026-09-24，助手只读导出 `BP_ThirdPersonCharacter.TryPlayPracticeMontage` 的图表与引脚：水平速度 `Vector Length XY <= 3.0` 的 Branch（该版本节点为 `K2Node_IfThenElse_2`）把 True 接到了 Return、False 接到 Is Any Montage Playing 的 Branch。因此静止速度为 0 时直接返回，未执行 Montage Play 与成功日志。这是合法但方向错误的逻辑，蓝图编译可以通过。

用户自行交换两条执行线并确认功能恢复。2026-09-27 重新导出保存后的资产，确认：

```text
速度 <= 3
  True  → Is Any Montage Playing → False → Montage Play
  False → Return
```

故障位于三种动作共用的函数，所以三键都受影响。助手此前自动按键未命中入口断点，同时已有 P 也未响应；该次自动输入不足以判断游戏输入映射失效，未作为根因证据。临时入口断点已在当时的编辑器中移除。

## 保存资产的只读复核

使用项目脚本 `Scripts/Editor/inspect_m0_montage_prereqs.py`，经 UE 5.5.4 Python commandlet 读取，未修改 / 保存 UE 资产。

| 项目 | 结果 |
|---|---|
| 玩家 | SKM_Quinn_Simple / ABP_Quinn_C；父类 ABP_Manny |
| 当前映射 | /Game/ThirdPerson/Input/IMC_Default |
| 三个动作映射 | IA_PracticeAttack=1、IA_PracticeDodge=2、IA_PracticeHit=3；键盘上排 |
| IA 参数 | Boolean，Trigger When Paused=False；动作及映射层 Triggers / Modifiers 为空 |
| 角色 Montage 变量 | 分别引用 AM_M0_Attack_01_Montage、AM_M0_Dodge_01_Montage、AM_M0_HitResult_01_Montage |
| Montage 骨架 | 三个均为 SK_Mannequin |
| 基础混合参数 | Blend In≈0.05 s、Blend Out≈0.10 s、Auto Blend Out=True、Trigger Time=-1、Rate Scale=1 |
| ABP_Manny | 已有 DefaultSlot；Always Update Source Pose=True |
| 速度检查 | True 继续忙碌检查，False 返回；与修复要求相符 |

图表导出使用 UE 自带 ObjectExporterT3D，随后只读解析节点 / 引脚信息，不编辑二进制或重新导入图表。检查原始产物放在本机 `Docs/Reports/Raw`，被 Git 忽略。

本轮同时复核 M0-05B：目标参数差异为 0，补修验收见 [B 记录](M0_05B_Verification.md)。独立检查进程退出 0，汇总 0 errors / 0 warnings。这证明提取成功及上述资产数据，不替代运行时画面或完整回归测试。

## 验收边界与交接

- 用户已确认：交换分支后功能修好。
- 已直接确认：保存的分支连接、映射、引用、Slot 开关与基础混合设置。
- 待明确确认：长按单次、忙碌时不重启、移动 / 空中拒绝、暂停恢复与菜单输入不补播、E/P 和三次 PIE；以及 C1 实际用时。
- C2-01 已下发：创建 / 挂载基础 C++ 组件和只读查询，不替换 C1 的输入或播放逻辑。C2-02 接管播放前需要补齐上述边界回归。
- 尚未完成：动作状态运行、受击打断、实际闪避位移、C3 实机记录、M0-06 打包。

下一步见 [M0-05C2-01](../M0_05C2_01_ActionComponent.md)。
