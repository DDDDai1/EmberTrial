# M0-05C1 排障与交接记录

更新：2026-09-27。**M0-05C1 验收完成。**用户补交完整测试结果，所有验收项通过、资产已保存，实际用时约 **80 分钟**。结合此前的磁盘资产与修复连线复核，C1 已无待确认项；下一步接收 C2 测试结果。

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

- 用户已确认：交换分支后功能修好，并补交下表全部实机验收结果。
- 已直接确认：保存的分支连接、映射、引用、Slot 开关与基础混合设置。
- C1 边界项已确认，无需重复补报。实机行为以本次用户测试为据，助手本轮未重新操作编辑器或执行独立试玩。
- C2-01 已下发：创建 / 挂载基础 C++ 组件和只读查询，不替换 C1 的输入或播放逻辑。用户将随后发送 C2 结果，届时单独审查，当前不预先标记通过。
- 尚未完成：动作状态运行、受击打断、实际闪避位移、C3 实机记录、M0-06 打包。

下一步见 [M0-05C2-01](../M0_05C2_01_ActionComponent.md)。

## 用户完整验收反馈

| 项目 | 用户结果 |
|---|---|
| 闪避 0 帧左上臂 Z=25、关闭重开与重新烘焙 | 是；当前 AS_M0_Dodge_01 |
| 三个 Montage 独立预览 | 正常 |
| ABP_Manny 复用 DefaultSlot，Always Update Source Pose=True | 是 |
| ABP_Manny / ABP_Quinn / 角色蓝图 Compile | 通过 |
| 1/2/3 播放并自动返回待机 | 通过 |
| 长按单次 / 播放中不重启 / 结束后可再触发 | 通过 |
| 移动与空中拒绝，站定重新按键可触发 | 通过 |
| 动作中暂停恢复 / 菜单按键不补播 | 通过 |
| E / P / 移动 / 转向 / 跳跃与三次 PIE | 正常 |
| 资产保存 | 是 |
| 实际用时 | 约 80 分钟 |

该反馈补齐了此前仅确认“播放恢复”时保留的验收缺口；实际闪避位移与受击打断仍属于后续 C2，未混入 C1 的通过结论。
