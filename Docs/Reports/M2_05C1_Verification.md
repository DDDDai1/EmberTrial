# M2-05C1：破势施放动画素材验收

2026-10-23，UE5.5.4。用户13:49反馈全项完成、无失败或占位表现问题。**C1验收通过，无待补项；同轮下发[C2-01施放与冷却](../M2_05C2_01_BreakPulseAbility.md)。**手动制作 / 配置40、检查5，共45分钟；烘焙 / 加载等待5分钟。

## 1. 资产、保存与现有基线

三份真实资源均已存在，13:48保存记录与文件一致；新增后本地共有287份uasset / umap，相比B2新增3份、原284份哈希不变。实际Montage目录为`Montage`单数，与最初计划的Montages不同；有效资产无须搬移，指南 / JSON / 来源清单已改为真实路径。

| 资产 | 本机相对路径 | 本地保存时间 | SHA256 |
| --- | --- | --- | --- |
| AS_ET_BreakPulse_01 | `EmberTrial/Content/EmberTrial/Animations/BreakPulse/Clips/AS_ET_BreakPulse_01.uasset` | 13:48:23 | `B803D9BEFA41CF416694A4EBC422BFBBA7A82C656BB11FBBD29541365164F339` |
| AM_ET_BreakPulse_01 | `EmberTrial/Content/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01.uasset` | 13:48:21 | `963357370FEA0EFDFE54039BEDFAF54EA356AB52B08318C9FB8010E4EAB6FBB2` |
| LS_ET_BreakPulse_01 | `EmberTrial/Content/EmberTrial/Animations/BreakPulse/Sequence/LS_ET_BreakPulse_01.uasset` | 13:48:23 | `6612FD0362D9866FEB3D1E7EE15A2FC62BE7EFCDC1258A936119CAB05433DBA5` |

131份Source与B2接受快照完全相同；70cpp、Core50 / UI17 / Editor3、16项自动化保持。现有受保护Source外配置 / 资产291项无差异，包括旧GA_Stagger、HitResult资源、正式身体 / 地图 / Rig / Skeleton。本素材批没有理由重新Build或重跑旧自动化，本轮也未代执行UE / PIE / Cook。

## 2. 视觉、配置与证据边界

用户确认五姿势0/8/12/20/30、三FK False / Linear、0=30与12=20、16帧停留；Quinn_Simple独立播放、前推 / 停留 / 收回、root / 双脚固定与首尾均正常。AM唯一AS、DefaultSlot、Loop1 / Rate1、Blend .10-.10、自动退出，AS / AM Notifies为空；受害反应GA仍沿用旧HitResult。原左键 / 2 / Q / E / P与基础输入短基线按用户实机反馈接受。

本次独立核对的是文件 / 哈希 / 正常保存记录与源码基线，没有通过UE反序列化逐个读取关键帧、Skeleton或Montage字段。用户在采样率 / 帧数 / 秒数行填写“完成”，未给实数；JSON继续保留30fps、End31对应的**预期**31帧间隔 / 32采样点 / 约1.0333秒，不把它当实测值。Actual字段为空并注明边界，后续GA按实际资源长度与结构检验，不为补抄数值重开整套验收。

保存日志支持本批三资源落盘；检索结果未找到本次逐项键鼠操作的完整日志，因此不伪造每个动作的时间戳。没有新Cook / Insights；Cleanup/Attempt01仍不含后来的次数HUD / Poise / 新素材和范围技能。施放者动画已完成，F能力、真实范围结算 / 冷却尚未落地，不与被动Stagger混淆。

## 3. Git收尾与下一批

核对起点工作区与暂存区干净，本地HEAD `b3cbc24c56e167ca19c32567dd565c094c8ef5a5`。用户已通过f4127ff / 77e556f补交完整B2，再提交C1三素材；此前98be3e0的空文件问题关闭。131份Source逐一与HEAD比较，归一CRLF / LF后全部一致；17份原始字节差异仅换行，不是源码缺失。三资源HEAD LFS OID与本地SHA一致；未联网核验GitHub或代Stage / Commit / Push。

C2拆为两个可独立检查的实施批：**C2-01**为F Started、玩家施放、真实6游戏秒Duration GE、受击 / Stagger / 死亡取消与暂停；**C2-02**再接12帧提交通知与范围 / 阵营 / 遮挡 / 去重和Poise GE。本轮仅准备忽略目录候选和完整手动指南，实际Source与资产保持C1；不提前登记未来136Source / 73cpp / 17测试、GA / IA两资产为完成。

C2-01五份新文件及八份修改文件已从指南代码块重新拼接，与候选格式化结果一致；generated.h顺序、引用路径、来源68项哈希、文档相对链接及冻结指南均检查通过。依据本机UE5.5.4源码核对冷却Tag查询 / 剩余时间和Duration GE接口；这些属于静态准备检查，不是UBT编译或自动化Success。

## 4. 工时

C1手动45 / 等待5；M2-05累计**285 / 20**，M2累计**850 / 84**。项目累计**2861分钟（47小时41分）/ 明确等待269分钟（4小时29分）**。只累计用户报告，助手设计、未独立拆报学习 / 排错不补算。

余下C2-01预计80–130、C2-02 70–110、D60–90分钟，剩余210–330，总模块495–615分钟；构建等待另列。分批是为了限制单次变更与检查负担，预算仍使用M2既有基础与缓冲，不重复叠加项目预算。

## 5. 本次 Commit / Push（固定放在最后）

C1三素材与完整B2均已在本地HEAD，无差异不重复Stage。当前只提交C1报告、实际路径与状态收尾、来源68项、C2-01指南及相关当前文档；精确入口见[C1末尾](../M2_05C1_BreakPulseAnimation.md#9-本批-commit--push固定放在最后)。

```text
docs: verify break pulse animation and prepare casting ability
```

C2-01实际Source / GA / IA仍不存在，实施后按其最后清单另交，不先创建空文件。Raw / 日志 / 生成物 / 包体留本地，Git由用户操作。
