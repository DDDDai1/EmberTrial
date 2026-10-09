# M1-02C2-01：生命周期 Widget / UI Bootstrap 验收

日期：2026-10-03。结论：**M1-02C2-01 通过，Native Tick 警告补修已完成。** M1-02 总任务仍进行中；下一批验证真正的新 Pawn 替换，不把同一 Pawn 的恢复控制当成新对象替换。

## 用户实机反馈

用户约 **19:02** 完成，手动 **40 min**、构建 / 索引等待 **5 min**：

- UI Build.cs / 四个新文件保存，C1 保留；Editor / Game Build、索引与 UMG 头文件导航通过。
- Widget 父类、两个 BindWidget TextBlock、无 Text Bind、Tick Never 正确；手动加入 DisableNativeTick 后正常双目标 Build，Widget Compile 警告消失。
- Controller 只有一个 UIBootstrap，StatusWidgetClass 正确；初始一份面板、Pawn 名称和日志正确。
- 正常状态三次 7 重建、空 Pawn 时的 7 快照、三轮 4→5、旧 Widget 不再处理 PawnEvent、重复 5 通过。
- 6 销毁后显示 None、7 重建仍正确、5 不执行；暂停菜单鼠标 / 键盘 / 背景输入、无补播 / 卡键及三次 PIE 清理通过。
- 临时节点 / ETTempPawn 删除，组件和 Widget 保留；最终原动作、打断、E/P、移动 / 转向 / 跳跃正常，源码与资产保存。

## 直接核对的实现 / 构建 / 资产

| 检查 | 结果 |
|---|---|
| 五份实施文件 | UI Build.cs、Widget .h/.cpp、Bootstrap .h/.cpp 去除空白后与修正后的指南一致 |
| Tick 契约 | 原生 Widget 的 UCLASS 已有 meta=(DisableNativeTick)；本批事件驱动 SetText，没有 Tick / 动画 / 延迟更新需求 |
| 依赖边界 | UI 公开依赖 Core / CoreUObject / Engine / UMG，私有依赖 EmberTrial；核心仍不依赖 UI / UMG |
| Widget 生命周期 | Construct 清旧订阅、订阅后立即读 C1 快照；不保存 Pawn；StopObserving 与 NativeDestruct 幂等解绑 |
| Bootstrap | 本地 Controller 校验、CreateWidget 的所属玩家、AddToPlayerScreen、重建前主动解绑 / 移除、EndPlay 释放存在 |
| C1 保留 | Subsystem 的两个文件保持既有实现；弱引用、Pawn EndPlay 与 Controller 清理路径保留 |
| Editor receipt | 14:21:01，UE 5.5.4 / Win64 / Development / Editor |
| Game receipt | 14:22:05，UE 5.5.4 / Win64 / Development / Game |
| 最新 UBT | 18:53:35 开始，Editor Target is up to date，1.18 s；先前 14:22:22 的 Editor 检查同为 up-to-date / 0.41 s |
| CLion 索引 | CMake 3.20、两个 CodeModel 保留；编译数据库有 13 个 cpp，UI 4 个，含 Widget / Bootstrap |
| WBP_ETLifecycleStatus | 实际目录为 /Game/EmberTrial/ui/Prototype；33,069 bytes，14:23:35 保存 |
| BP_ETPlayerController | 68,177 bytes；运行结束后 19:02:21 保存；本次哈希已更新资产清单 |

WBP 的 SHA256：`BDA44C2287665670457DE27F7E1BB9EFEFBCD7103238AF5558868B7128B697E1`。

Controller 的 SHA256：`8F25D817C3BF88A5B7CA57246C1DB46BAD02DA8580D5FDE33E9C27D68151864B`。

现存 UBT 文本已被后续检查覆盖，未保留本次两个目标的完整编译动作；不能把 up-to-date 当成重新全量编译。双目标成功按用户反馈、两个 receipt 及后续运行共同记录。布局此前已逐项通过编辑器只读检查，警告消失按本次复验登记。

## 运行日志直接证据

读取 `EmberTrial/Saved/Logs/EmberTrial.log`，最后写入 **19:02:21**。下表将 UE UTC 时间转换为 UTC+8。

| 本机时间 | 实际记录 |
|---|---|
| 18:59:25–18:59:52 | 第一轮初始化；正常 Pawn 下多次 Widget 重建；18:59:43 变为空 Pawn；18:59:47 / 48 重建立即得到 None；结束清理 |
| 19:00:19–19:01:40 | 第二轮初始化、三次正常重建；19:00:43 清空、19:00:47 空快照、19:01:01 恢复原 Pawn；19:01:08 / 09 / 10 各有一轮完整失去 / 恢复 |
| 19:01:55–19:02:12 | 第三轮初始化；19:01:57 清空，19:02:05 / 06 重建仍为 None；Controller / Widget / LocalPlayer 结束清理 |
| 19:02:21 | Controller 最终保存 |

计数：3 次 UI local ready / Controller bound / Controller cleared / Deinitialize；15 次 Widget created / removed、15 次 subscribed / unsubscribed、15 次 ConstructSnapshot。C1 共 14 次 Pawn changed，Widget 共 11 次 PawnEvent；每轮初始 Pawn 变化发生在 Widget 订阅前，随后由 ConstructSnapshot 补读，差值符合该顺序。

按每轮 PIE 单独维护活动订阅集合审查：**无同时活动的两个状态 Widget、无解绑后的 Widget PawnEvent、结束时活动集合为空。** 实例后缀在不同 PIE 中可能复用，不能跨世界仅凭 C_0 判断旧对象残留。

未匹配到 Accessed None、Blueprint Runtime Error、指定蓝图 / 脚本错误、Fatal / Assert，以及 BindWidget / WidgetClass / Subsystem 缺失或 AddToPlayerScreen 失败。

**证据边界：**这份日志中的 Pawn=None 不能独自证明具体按了 4 还是 6；按 6 销毁、重复 5 的不执行、暂停输入完整矩阵、临时节点删除与最终原功能回归按用户实机反馈验收。保存记录证明资产保存发生，不独立证明图内节点已删除。本轮没有运行新的 Build / PIE / 自动测试，也没有修改 C++ 或 Blueprint。

选择的原始片段保留在 `Docs/Reports/Raw/M1_02C2_01_Acceptance_2026-10-03.log`。

## 工时与后续

累计已报告手动 **843 min（14 h 03 min）**，等待 **55 min**，分别由先前 803 / 50 加本次 40 / 5 得到；未补算助手工时。资产来源清单新增状态 Widget，现为 **19 项**。

下一步按 [M1-02C2-02 新 Pawn 替换验证](../M1_02C2_02_PawnReplacement.md) 手动制作临时蓝图测试，验证替换 / 旧对象销毁 / 新实例恢复。通过后再做 M1-02 总验收；ASC、GameplayReady、正式 HUD 与正式重生仍在后续阶段。
