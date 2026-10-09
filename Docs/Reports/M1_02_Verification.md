# M1-02C2-02 与 M1-02 总验收

日期：2026-10-03。结论：**C2-02、C2 总任务及 M1-02 全部通过。** 正式对象 / 模块边界、本地 Pawn 观察、Widget 装配 / 重建与真实新对象替换已完成；ASC / GameplayReady / 正式 HUD 从 M1-03 开始。

## C2-02 用户反馈

用户约 **19:45** 完成，手动 **30 min**，没有新增构建等待：

- Probe 父类 Pawn，不放地图、NoCollision、AutoPossess Disabled；临时三个函数、两个变量与 4–7 按键编译通过。
- 原角色 → Probe → 原角色、重复 4/5 不重复 Possess、Probe 上 Widget 重建与旧 Widget 解绑通过。
- 销毁旧 Probe 不影响当前原角色 / UI，重新 Spawn 新实例后显示正确。
- 销毁当前 Probe 后显示 None，空 Pawn 重建快照、直接 4 新建恢复、5 返回原角色通过。
- 三轮切换 / 三次 PIE 清理、无 Accessed None；临时按键 / 函数 / 变量删除，正式组件 / Widget 保留。
- Probe 保留在 Tests，不设为默认 Pawn；原 1/2/3、2→3、E/P、移动 / 转向 / 跳跃正常，资产保存。

## 直接核对的日志

读取 `EmberTrial/Saved/Logs/EmberTrial.log` 中 **19:40:48–19:46:00** 的本批片段。下表为 UTC+8，本批最后一次 PIE 在 19:46:00 结束；与用户“约 19:45”完成一致。

| 时间 | 直接证据 |
|---|---|
| 19:40:48–19:42:00 | 第一轮初始化，19:41:46 Spawn / 控制 Probe_C_0，Stop 后 Widget / Controller / LocalPlayer / Probe 清理 |
| 19:42:01 | 第二轮正确新地图 / BP_ETGameMode、原角色及一份状态面板 |
| 19:42:06 | Spawn / 控制 Probe_C_0，UI 显示该实例 |
| 19:42:42 / 43 | Probe 下两次 Widget 重建，新 Widget 立即读当前 Probe |
| 19:42:48 | 返回原角色，UI 同步原角色 |
| 19:43:04 | 请求销毁旧 Probe_C_0，随后 Probe EndPlay；此时没有 Pawn changed / None 快照，原角色仍为当前对象 |
| 19:43:27 | 新 Spawn Probe_C_1、控制 / UI 同步 |
| 19:43:41 | 销毁正在控制的 Probe_C_1；当前 Pawn / UI 清空，随后该实例 EndPlay |
| 19:43:50 | 空 Pawn 时重建，ConstructSnapshot Pawn=None |
| 19:43:54 | 空 Pawn 直接生成 Probe_C_2、控制成功，UI 从 None 变为该实例 |
| 19:44:01 / 05 | Probe_C_2 下重建快照正确，随后返回原角色 |
| 19:44:08–11 | 三轮原角色 / Probe_C_2 往返，每次变化只有当前 Widget 响应 |
| 19:44:30 | 第二轮完整结束清理，仍存活的 Probe_C_2 EndPlay |
| 19:45:37 | Controller 最终保存 |
| 19:45:55–19:46:00 | 最终保存后的第三轮 PIE：正常原角色 / 一份面板，攻击 Begin / Completed，随后完整清理 |

本批片段计数：3 组 local ready / Controller bound / Controller cleared / Deinitialize；7 组 Widget 创建 / 移除、订阅 / 解绑及 ConstructSnapshot；30 次 Pawn changed、27 次 PawnEvent。三次出生事件在 Widget 订阅之前发生，由初始快照补读。4 次 Probe Spawn、2 次销毁请求、4 次 Probe EndPlay（含两次 Stop 清理）、12 次 Possess complete。

按每轮 PIE 维护活动 Widget 集合检查：无重复活动订阅、无解绑后的 PawnEvent，结束集合为空；未匹配到本批蓝图 / 脚本错误、Accessed None、Fatal / Assert、Spawn 失败或 UI 配置缺失。

**证据边界：**第三轮保留的是最终清理后的正常运行 / 攻击样本，不是第三套完整 Probe 操作。重复按键的无动作、Probe 各项编辑器属性、临时图表删除及原功能完整矩阵按用户反馈验收；文件保存证明保存发生，不独立证明节点已删。本批已有实机与日志证据，不要求再次跑整套测试。

原始片段保留在 `Docs/Reports/Raw/M1_02C2_02_Acceptance_2026-10-03.log`。

## 源码 / 资产基线

本轮 27 份 Source 文件与 C2-01 验收基线 SHA256 全部一致；本批没有新增 C++，无需重新 Build。既有构建依据沿用 A / B / C1 / C2-01 的正常 Editor / Game 验收。

| 资产 | 保存 / 哈希 |
|---|---|
| BP_ETLifecycleProbePawn | Tests/Blueprints；46,006 bytes；19:22:58；SHA256 `0EE8305893A2D4EA4942802F18C158CD490518118EE98AC1E84BBADB93FD1539` |
| BP_ETPlayerController | 22,217 bytes；19:45:37；SHA256 `ACF420688F5947317E3F4DC981DF2EBCB79ABC143B58D8FFD27A48AF5E082EEF` |
| WBP_ETLifecycleStatus | 与 C2-01 相同，未再次改写；SHA256 `BDA44C2287665670457DE27F7E1BB9EFEFBCD7103238AF5558868B7128B697E1` |
| L_ET_VerticalSlice | 保持 B 阶段地图基线；SHA256 `FD55FF978C177CB11B9D4456E6266D325804AD5AFF974027210EF27B16377F33` |

来源清单新增隔离 Probe，现有 **20 项**。本轮助手仅读取证据、更新文档 / 清单，没有编辑工程源码 / UE 资产、执行 Build / PIE 或提交 Git。

## M1-02 总验收

| 子任务 | 完成范围 / 证据 |
|---|---|
| A | UI Runtime 注册、双目标构建 / 独立索引 / 模块加载，[报告](M1_02A_Verification.md) |
| B | 复用并公开现有 Character、新 GameMode / Controller、地图装配 / 旧地图回归，[报告](M1_02B_Verification.md) |
| C1 | 本地 Controller / Pawn 委托、立即快照、弱引用、EndPlay 清理，[报告](M1_02C1_Verification.md) |
| C2-01 | Widget / Bootstrap、事件 SetText、重建前主动解绑、Never / DisableNativeTick，[报告](M1_02C2_01_Verification.md) |
| C2-02 | 实际新 Pawn 替换、旧对象销毁隔离、None 后新建恢复，见本页 |

M1-02 已报告手动 **180 min（3 h）**，构建 / 索引等待 **29 min**；项目累计手动 **873 min（14 h 33 min）**、等待 **55 min**。时间分别累计，未补算助手审查或未报告等待。

本次结论不扩大到正式重生、GAS、完整 HUD、正式 UI 页面栈、切图闭环或新版本打包；这些功能仍按主计划推进。M0 Windows 包仍对应旧版本。

下一批按 [M1-03A1：ASC 接入](../M1_03A1_ASCFoundation.md) 手动编码 / 构建 / 验证。ASC 归属与初始化取舍见 [ADR-002](../ADR/ADR_002_ASCOnCharacter.md)。
