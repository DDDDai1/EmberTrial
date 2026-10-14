# M1-07：当前阶段包、性能基线与最小校验

更新：2026-10-14，UE **5.5.4**。A独立包、B实际性能分析与最终函数Editor收尾完成。当前进入 **[C1编辑器配置校验](M1_07C1_EditorValidation.md)**，代码仅在指南，待用户手动实施。69Source / 34cpp / 清单46，仅追击Task.cpp相对A改变；累计已报告手动1941分钟（32小时21分钟）/ 已明确等待165，最后函数替换实耗未报告。

## 1. 本章交付与顺序

| 批次 | 具体交付 | 预算 / 实耗（手动与等待分别记录） |
| --- | --- | --- |
| **[A 当前阶段Windows包](M1_07A_StagePackaging.md)，已验收** | Win64 Development Build / Cook / Stage / Package；独立菜单、战斗、检查点与重试；新进程会话重置 | 实际手动30 / 等待25分钟 |
| **[B 实际Insights采样](M1_07B_InsightsBaseline.md)，分析 / Editor收尾完成** | 原Trace与真实AI Combat02分析；CPU / GPU口径和环境限制；MoveRadius绑定补修、日志精简 | 已报告55+5+10+5=75分钟；最后精简替换与新增等待未报告，不推算 |
| **[C1 Editor模块与BT / UMG校验](M1_07C1_EditorValidation.md)，待实施** | Editor专用Data Validation；黑板半径绑定、三项开关、必需控件名称 / 类型；一份隔离BT负例恢复 | 预计手动60–100 / 等待5–15分钟，专项检查5–10分钟 |
| C2 关键配置与复用检查，待C1通过后下发 | 能力 / GE / Montage引用、Controller距离关系、输入 / 启动 / Cook地图；选择高频规则，复用现有自动化 | 暂估手动60–100，待C1真实成本与最终范围校准；检查5–15分钟 |
| C3 Attempt02与M1阶段收口，待C2后下发 | 将已改游戏源码 / 配置统一Build / Cook成新包；包内真实自主接近攻击短流程；报告 / Bug与M2排期复盘 | 暂估手动20–40 / Build与Cook等待20–45分钟；不重做全矩阵 |

C原60–120分钟是有限校验的初批预算，现具体化为C1；C2扩展和C3新版包另列，不能将所有工具与重新打包成本塞进该初批数值。沿用用户选定的方案B，先做能发现真实错误的少量规则，再看复用收益；不要求一次建设大型验收平台。

B最终短函数与指南一致、Editor于00:38:40构建成功，功能正常采用用户反馈。最新留存PIE日志早于替换，Game可执行仍10月13日23:47版本；**Game最终函数补编译合并C1**，include清理同时完成。不再要求B重采或重复短PIE。旧残余现象根因未唯一确定，只读日志不改变玩法；历史与当前接受边界见[B报告7.6](Reports/M1_07B_Verification.md)。

## 2. A与后续检查边界

A先验证新Runtime UI / 地图在Cook后齐全，B复用同一Attempt01作测量。游戏Source或Cook资产变动后，旧包不能验证新实现；当前Source已改，所以C3需统一Attempt02。原包 / 原Trace仍保留为历史测量，不声称自动更新或已测出优化收益。

按方案B，各批只检查新增边界与相关风险。C1使用Content Browser的Validate Assets，不搭Controller验收图、不新增临时输入；C2复用已有Lifecycle / DodgeMovement等入口，按涉及范围选择。C3只验证新包实际AI接近 / 伤害及短闭环，不复制A完整输入矩阵；发现真实失败再局部扩大。

性能基线的主循环间隔、GPU场景Scope与呈现FPS分开。电源档位 / 实际Viewport未知、启动内存压力与Development日志 / Trace开销已记录；单敌人灰盒数据不承诺最终美术场景60FPS。M2深化和美术接入后按变化重新测量，不在C1静态工具改动后做无依据的FPS对比。

## 3. 阶段收口标准

1. 当前阶段Win64包脱离Editor运行，菜单 / 战斗 / 检查点闭环有独立证据；A已通过。C3更新Source后补新版包与相关短流程。
2. 真实Insights材料可打开、可复述统计口径 / 限制，包含真实AI负载；B已完成，不只有平均FPS截图。
3. 有可复用有限校验入口：正常配置通过，隔离负例报告准确路径 / 规则并可恢复，Editor模块不进入Game；C1尚待实施。
4. 关键配置 / 引用与复用自动化结果按实际证据记录；C2尚未完成，不提前声称所有输入与资产都被工具覆盖。
5. M1报告 / Bug / 资源来源和真实工时更新，用于校准M2；灰盒包不是最终招聘作品。

实际工程当前仍两个Runtime模块；C1计划新增四Source后才会有Editor模块。C1 / C2 / C3未实施、未打包或验收；不提前记新增资产数量或工具收益。

## 4. 本章 Commit / Push（固定放在最后）

按各批指南最后的清单选择实际差异。当前提交B收尾 / C1指南与路线 / 状态、最新Task.cpp；C1实施后提交四新Source、uproject / Editor Target与合法Probe，Game Target / Runtime Build.cs无需添加Editor依赖。

Source已暂存过旧长诊断版时，需重新Stage最新修改。正式BT / HUD只有真实差异才提交，包、日志、Trace、运行JSON、Raw、CMake / Intermediate / Binaries / Saved继续留本地，不Force Add。用户在CLion检查Staged Diff后Commit / Push；助手未代操作Git索引或远端。
