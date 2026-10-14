# M1-07：当前阶段包、性能基线与最小校验

更新2026-10-14，UE **5.5.4**。**A / B / C1 / C2 / C3全部完成，[M1-07总验收](Reports/M1_07_Verification.md)通过。**当前75Source / 37cpp / 清单48，最新包C3 Attempt02。本章手动175 / 已明确等待45分钟；项目累计2011 / 185分钟。[M1复盘](M1_Review.md)与[M2路线](M2_Roadmap.md)已形成，M2代码尚未实施。

## 1. 本章交付与顺序

| 批次 | 具体交付 | 预算 / 实耗（手动与等待分别记录） |
| --- | --- | --- |
| **[A 当前阶段Windows包](M1_07A_StagePackaging.md)，已验收** | Win64 Development Build / Cook / Stage / Package；独立菜单、战斗、检查点与重试；新进程会话重置 | 实际手动30 / 等待25分钟 |
| **[B 实际Insights采样](M1_07B_InsightsBaseline.md)，分析 / Editor收尾完成** | 原Trace与真实AI Combat02分析；CPU / GPU口径和环境限制；MoveRadius绑定补修、日志精简 | 已报告55+5+10+5=75分钟；最后精简替换与新增等待未报告，不推算 |
| **[C1 Editor模块与BT / UMG校验](M1_07C1_EditorValidation.md)，已验收** | Editor专用Data Validation；黑板半径绑定、三项开关、必需控件名称 / 类型；一份隔离BT负例恢复 | 实际手动40 / 等待5分钟 |
| [C2 关键配置与复用检查](M1_07C2_GameplayValidation.md)，已验收 | 能力 / GE / Montage引用、Controller距离关系、输入 / 启动 / Cook地图；选择高频规则，复用现有自动化 | 实际手动20 / 等待5分钟 |
| [C3 Attempt02与M1阶段收口](M1_07C3_UpdatedPackage.md)，已验收 | 新包Build / Cook / Stage / Package成功；真实AI接近与伤害、A点Retry / Continue、新进程Entry；M1报告与M2排期完成 | 实际手动10 / Build与Cook等待10分钟 |

C原60–120分钟是有限校验的初批预算，现具体化为C1；C2扩展和C3新版包另列，不能将所有工具与重新打包成本塞进该初批数值。沿用用户选定的方案B，先做能发现真实错误的少量规则，再看复用收益；不要求一次建设大型验收平台。

B最终短函数与指南一致、Editor于00:38:40构建成功，功能正常采用用户反馈。当时留存PIE日志早于替换；**C1已在10月14日13:18补齐Game最终函数编译和include清理**，C2无Runtime源码变化，仍使用该有效Game产物。不再要求B重采或重复短PIE。旧残余现象根因未唯一确定，只读日志不改变玩法；历史与当前接受边界见[B报告7.6](Reports/M1_07B_Verification.md)。

## 2. A与后续检查边界

A先验证新Runtime UI / 地图在Cook后齐全，B复用同一Attempt01作测量。C3已将后续Source / Config统一Build / Cook为Attempt02并通过独立运行。原包 / 原Trace保留历史测量；C3没有采新Trace，所以没有Attempt02性能收益结论。

按方案B，各批只检查新增边界与相关风险。C1使用Content Browser的Validate Assets，不搭Controller验收图、不新增临时输入；C2复用已有Lifecycle / DodgeMovement等入口，按涉及范围选择。C3只验证新包实际AI接近 / 伤害及短闭环，不复制A完整输入矩阵；发现真实失败再局部扩大。

性能基线的主循环间隔、GPU场景Scope与呈现FPS分开。电源档位 / 实际Viewport未知、启动内存压力与Development日志 / Trace开销已记录；单敌人灰盒数据不承诺最终美术场景60FPS。M2深化和美术接入后按变化重新测量，不在C1静态工具改动后做无依据的FPS对比。

## 3. 阶段收口标准

1. 当前阶段Win64包脱离Editor运行，菜单 / 战斗 / 检查点闭环有独立证据；A与C3均已通过，当前发布基线为Attempt02。
2. 真实Insights材料可打开、可复述统计口径 / 限制，包含真实AI负载；B已完成，不只有平均FPS截图。
3. 有可复用有限校验入口：正常配置通过，隔离负例报告准确路径 / 规则并可恢复；C1 / C2均已验收，C3构建与Runtime进一步核对Editor模块隔离。
4. 关键配置 / 引用与复用自动化结果按实际证据记录；C2已验收；覆盖范围与未测负例明确，不声称所有输入白线 / 资产均被工具覆盖。
5. M1报告 / Bug / 资源来源和真实工时已更新，M2范围与预算已校准；最终招聘作品继续完成后续内容与打磨。

实际工程两个Runtime与一个Editor模块、两个校验器，75Source / 37cpp / 48项来源。[M1总验收](Reports/M1_Verification.md)完成；下一阶段从M2-01资源基础开始。

## 4. 本章 Commit / Push（固定放在最后）

按各批指南最后的清单选择实际差异。C1主体已在本地e3ee554；C2两新Source、Editor Build / Cue配置、合法GA Probe与工具 / 清单若尚未提交，按C2末尾清单纳入。C3提交启动工具EvidenceStage、C3 / M1-07 / M1报告、M1复盘、M2路线及Introduction / 状态 / 排期更新；M2还没有新增Source或资产。Game Target / Runtime Build.cs保持既有模块边界。

Source已暂存过旧长诊断版时，需重新Stage最新修改。正式BT / HUD只有真实差异才提交，包、日志、Trace、运行JSON、Raw、CMake / Intermediate / Binaries / Saved继续留本地，不Force Add。用户在CLion检查Staged Diff后Commit / Push；助手未代操作Git索引或远端。
