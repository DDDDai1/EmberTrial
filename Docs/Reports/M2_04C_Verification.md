# M2-04C与有限次数治疗整章验收

2026-10-20，UE5.5.4。用户21:13反馈全项完成，无失败或布局问题；代码与配置60分钟、检查10分钟、构建等待3分钟。**M2-04C验收通过，无待补项，M2-04整章关闭。**本轮只读核对源码 / 保存 / 日志，更新报告、来源与开发文档；未修改实际Source / UE资产、执行Build / 测试 / PIE / Cook或操作Git索引。

## 1. 源码、构建与索引

四份新UI Source与两份既有Bootstrap已落地，六份与指南候选的有效C++ Token一致；差别为排版 / 注释，未发现功能偏移。清理基线其余114份旧Source字节保持。当前实际**120份Source / 63cpp**，CLion CodeModel **Core43 / UI17 / Editor3**。

最新Source保存于20:58:59；Editor receipt **21:00:01**、UI DLL **20:59:59**，Game receipt **21:01:48**、exe **21:01:47**，均覆盖最终源码。UBT后续日志为目标up-to-date，不冒充新一次全量编译。双目标Build与索引 / 导航结合用户反馈接受，本轮未重建。

Presenter通过当前LocalPlayer / Pawn Ready / 原生资源变化事件取整数快照，弱来源与观察代次隔离旧回调；失活时重新观察也绑定资源事件，后续Activate能恢复。Widget只展示数据，订阅后立即读缓存，无Tick / Timer / Text Binding。Bootstrap强持有Presenter / View，重建只换View，EndPlay先封口、停View再停止Presenter；原状态 / 生命 / 体力与失败流程保留。

## 2. 自动化、资产保存与编辑中报错

以下时刻均已由日志UTC换为本地：

| 本地时刻 | 直接证据 |
| --- | --- |
| 21:03:26.966 | EmberTrial.M2.Healing.Resource最终Success |
| 21:03:47.069 | EmberTrial.M2.Healing.Ability最终Success |
| 21:09:54 | 新WBP正常保存，项目[M1-07C1] Valid |
| 21:10:24 | BP_ETPlayerController正常保存至Content原路径 |

21:04–21:05新WBP尚未搭齐时曾出现三项BindWidget Warning / Error；随后必需控件齐全、正常保存 / 项目Valid，正式PIE时没有再次出现，不将编辑过程的中间错误算作当前未解决问题，也不隐去历史记录。21:10:42至最终Stop的玩法日志无Warning / Error / Accessed None。

WBP父类、三项控件名称 / 类型、无Binding、Tick Never、中文与位置，以及Controller唯一UIBootstrap配置由用户逐项确认；保存 / Valid与运行实例提供旁证，本轮未再次操作UE独立遍历Designer Details。自动化是资源 / 能力生产者回归，不冒充新Presenter的完整自动化覆盖。

## 3. 新HUD实机与生命周期

| 本地时刻 | 直接日志证据 |
| --- | --- |
| 21:10:42 | 首次Presenter等待→Ready 3/3，新View创建并订阅 |
| 21:10:50 | 正式Heal Begin后约.48秒提交，次数一次3→2，生命HUD同步 |
| 21:10:55–58 | P暂停与恢复闭合 |
| 21:11:17 / 24 | 两次重建：旧View明确退订 / 移除，新View后缀0→1→2，Presenter仍为同一实例 |
| 21:11:42 / 56 | 同Presenter重启观察，重新快照仍2/3，没有资源补满 |
| 21:12:28 | 下一次正式治疗，资源变化快照仅一条1/3 |
| 21:12:36 / 43 | Deactivate后Unavailable；失活期间重启观察仍Unavailable |
| 21:12:58 | Activate false后恢复Ready 1/3，保持同身体原次数 |
| 21:13:06 | 再次正式提交后Ready 0/3，不变成Waiting |
| 21:13:14–15 | 真实死亡 / Failure / Retry，旧View与Presenter清理，新身体3/3 |
| 21:13:23–24 | 暂停返回菜单 / Continue，重建后新3/3，无关卡HUD遗留到菜单 |
| 21:13:32–38 | 另一次菜单 / Continue，最终Stop，旧View退订 / Presenter停止 |

显示位置、满血 / 耗尽拒绝、暂停Q不补执行、晚加入立即2/3、唯一面板、Stop再开及左键 / 2 / E / P / 基础输入，按用户全项通过确认。默认View数据应用日志为Verbose，未要求为捕获每次SetText开启额外日志；旧View退订与单份Presenter变化可直接查。日志名会在新世界重用，不能只按同名后缀判断同一UObject。

NoPawn / Waiting与旧身体解绑的分支有源码检查、世界重建 / Stop样本；没有重新搭Unsupported Pawn等旧矩阵，沿用既有LocalPlayer基线，不声称本轮全部变体实机覆盖。出生时Ready通知和资源组件可用通知之间，同一帧曾短暂Unavailable，随后立即Ready；这是两个就绪边界依次通知，未观察到持续错误展示。

## 4. 来源、工时与整章结论

新增`EmberTrial/Content/EmberTrial/ui/HUD/WBP_ETHealingHUD.uasset`；既有资产仅`BP_ETPlayerController.uasset`字节变化。其余62项旧来源资产、Config / uproject保持，冻结M1-03B1指南SHA保持。来源清单更新Controller与新WBP，现**64项**；不因父类继承或查看而强制重存其他资产。

| 已验收批次 | 手动分钟 | 构建 / 加载等待分钟 |
| --- | ---: | ---: |
| A：次数资源与成功生效事务 | 60 | 5 |
| B1：占位动画 | 15 | 1 |
| B2：正式能力 / Notify / Q | 40 | 5 |
| C：整数次数HUD | 70（实现60、检查10） | 3 |
| M2-04合计 | **185（3小时5分）** | **14** |

M2累计手动**565 / 等待64分钟**；项目累计**2576分钟（42小时56分）/ 等待249分钟（4小时9分）**。助手设计 / 审查未计入用户手动，未报告学习成本不补算，不能据录入速度等比缩减后续计划。

M2-04完成有限次数资源、固定GE成功消费、正式Q、唯一Notify提交、前后取消语义和事件HUD；旧身体同世界保留次数，新身体Retry / Continue重置。持久存档、补给点恢复次数与最终动画美术不在本章已实现范围。当前独立包仍Cleanup/Attempt01，不包含C；本轮无新Cook或性能采样。下一入口[M2-05A：韧性属性基础](../M2_05A_PoiseFoundation.md)为手动指南，尚未实现。

## 5. 本次 Commit / Push（固定放在最后）

当前四份新Source在CLion中是`AM`：**暂存区内容仍是0字节创建骨架，工作区才有完整实现。**这是提交准备问题，不影响本地已构建 / 验收的功能。提交前必须对四份新文件重新Stage，再在Staged Diff确认实际代码；两份Bootstrap、新WBP、既有Controller和文档也要Stage最新差异。助手没有代改索引。

源码 / 资产精确路径及文档清单见[C指南末尾](../M2_04C_HealingHUD.md#11-本批-commit--push固定放在最后)。C验收与下一章文档可同次提交；M2-05A源码这轮尚未写入工程，不存在待提交的A实现。建议Commit：

```text
feat: add event-driven healing charges HUD
```

用户自行检查Staged Diff → Commit → Push。Raw / 日志 / 包体 / 生成CodeModel / Saved / Intermediate / Binaries留本地，不Force Add。HEAD仍b1fbb30，未联网核验远端或LFS服务器。
