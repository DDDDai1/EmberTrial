# M1-03A2-01：生命属性 / GameplayReady 验收

日期：2026-10-03。结论：**A2-01 通过。** Health / MaxHealth、原生属性集注册、一次初始化、就绪查询 / 通知与结束撤销已实施；真实 Effect 动态边界及正式 HUD 尚未验收。

## 用户实机反馈

用户约 **21:31** 完成，手动 **30 min**，构建 / 索引等待 **5 min**。两份 AttributeSet 与角色接入保存，关闭 UE 后 Editor / Game Build、索引与导航通过；单个 ASC / 属性集、100/100 出生、重复 9 三次、40/80 配置、非法 100/60 拒绝、恢复 100/100 后三轮 PIE / Ready 撤销通过。Controller 临时 8/9 已删除并 Compile / Save；原 1/2/3、2→3、E/P 与移动 / 转向 / 跳跃正常。

## 直接核对

| 项目 | 结果 |
|---|---|
| Source 范围 | 共 29 文件；对比 A1 基线，新增 AttributeSet .h/.cpp，只修改角色 .h/.cpp；其余 25 份相同 |
| AttributeSet | 原生子对象；Health / MaxHealth 使用 FGameplayAttributeData；基础值 / 当前值范围钩子、最大值下降后的生命调整存在 |
| 初始化 | ASC ActorInfo / 同一 GetSet 实例与合法配置先校验，MaxHealth → Health；成功标志使后续 Ensure 不重复写值 |
| Ready | 状态先提交再广播；查询 / 快照存在；EndPlay 先撤销，再取消能力 / 清理 Cue / ActorInfo；Possess 不重置初值 |
| Editor receipt | 21:20:40；UE 5.5.4 / Win64 / Development / Editor |
| Game receipt | 21:18:52；UE 5.5.4 / Win64 / Development / Game |
| CLion | 编译数据库 14 个 cpp；新 AttributeSet.cpp 归属 EmberTrialCodeModel |
| Controller 资产 | 保存于 21:31:01；SHA256 `03AC914118728C2DAFE85161C7C76F8F15E961BCF260C0D6EE1360F17AAEC048` |
| 玩家 Blueprint | 保存于 21:30:41；SHA256 `682AF1810AEEF0FCEC69D5D6B253AE84153ED90634A0633EBD775A40F25FC311` |

本轮没有重新编译 / 启动 PIE，也未解析 Blueprint 图表或 Class Defaults。构建成功按用户反馈与 receipt 记录；保存时间 / 哈希不能单独证明临时节点删除或最终配置为 100/100。

## 日志范围与差异

当前 EmberTrial.log 保留五轮 PIE（下列为 UTC+8 本地时间）：

- **21:22:28–21:22:30**、**21:22:36–21:22:57**：100/100 各一次初始化与 Ready=1；结束 Ready=0 在 ActorInfo cleared 前。
- **21:28:28–21:29:10**：100/100，有三条有效快照及五次 Already initialized；没有重复初始化 / Ready=1。
- **21:30:10–21:30:21**：实际配置 **Health=80 / MaxHealth=40**；两条预期 Warning、两条 Snapshot unavailable，没有初始化成功 / Ready=1；结束仅清 ActorInfo。
- **21:30:30–21:30:36**：40/80 初始化、有效快照、一次 Already initialized、结束撤销 / 清理。

计数：Health initialized **4**、Ready=1 / Ready=0 **各 4**、Already initialized **6**、非法配置 Warning **2**、有效快照 **4**、不可用快照 **2**、ActorInfo cleared **5**。未匹配到 Missing gameplay dependencies、Accessed None、蓝图 / 脚本 Error、Fatal / Assertion failed；每轮结束清理后没有该轮重新 Ready。

**用户反馈的非法 100/60 与日志中的 80/40 不同，但都覆盖 Health > MaxHealth 的同一拒绝条件。** 精确 100/60、最终恢复 100/100 后三轮 PIE、删除临时节点及完整原玩法矩阵按用户实机反馈登记；现存日志不独立覆盖这些最终操作。无需重跑整套 A2-01；下一批开头确认最终 100/100。

片段：[原始证据](Raw/M1_03A2_01_Acceptance_2026-10-03.log)。就绪委托的真实 UI 订阅 / 解绑与 Effect 范围不能凭本批代码或初值快照记作已验证，分别留待 B 与 A2-02。

## 工时与交接

累计手动 **948 min（15 h 48 min）**，已报告等待 **63 min**，由前一基线 918 / 58 加本次 30 / 5 得到。没有新增 UE 资产，来源清单维持 **20 项**，更新两份 Blueprint 的阶段 / 哈希。

下一步：[A2-02 Gameplay Effect / 动态边界](../M1_03A2_02_GameplayEffects.md)。默认由用户创建测试 Effect、配置临时 Blueprint 输入并验收；本轮助手只读工程证据、更新文档 / 清单，没有修改源码 / 资产、执行 Build / PIE。
