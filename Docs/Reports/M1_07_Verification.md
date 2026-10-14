# M1-07：阶段包、性能基线与校验总验收

更新2026-10-14，UE5.5.4。**A / B / C1 / C2 / C3全部完成，无当前待补项。**复用各批证据汇总，本次不额外创建测试图、Build、PIE或Trace。当前75Source /37cpp /48项来源，主功能包为C3 Attempt02。

| 批次 | 交付与证据 | 已报告手动 / 等待分钟 |
| --- | --- | ---: |
| [A](M1_07A_Verification.md) | 独立Development包 / 菜单、真实战斗与Checkpoint流程；后来发现Radius回归，旧包保留历史范围 | 30 /25 |
| [B](M1_07B_Verification.md) | 原Insights与Combat02真实AI分析、统计口径 / 限制；Move150→90绑定补修、精简日志 | 75 /新增等待未单列 |
| [C1](M1_07C1_Verification.md) | Editor模块、BT / UMG规则、断键负例恢复；Game / include补项关闭 | 40 /5 |
| [C2](M1_07C2_Verification.md) | 九配置、GA资源负例恢复、已有两自动化、地图 / Cue共享配置脚本 | 20 /5 |
| [C3](M1_07C3_Verification.md) | 当前Attempt02、Radius90 / 真实伤害、A点Retry / Continue、新进程Entry；CueWarning关闭、Editor隔离 | 10 /10 |

手动175分钟（2小时55分钟）、已明确等待45分钟，未知等待不记为0。B最后短函数替换未报告时间不推算；项目累计手动2011 /已明确等待185。构建UAT171.81秒与用户C3等待10分钟为不同口径，保留各自事实。

当前包：`E:\UE_Project\Demo\Artifacts\Builds\M1_07C3\Attempt02\Windows\EmberTrial.exe`。bootstrap仍同A哈希，但实际Game exe /五容器改变、Runtime实测90；不能以启动器哈希代替版本核查。

性能基线**属于旧Attempt01**：两Idle内部主循环约7.350 /7.122ms，GPU场景Scope约6.709 /6.488ms；Combat02死亡前19秒主循环6.630ms、GPU场景6.003ms。不是呈现FPS、完整GPU Frame或新包已测优化收益。环境电源 /实际Viewport未知、内存压力 /Development+Trace影响继续保留。

静态校验真实负例只验证ET-AI-003和ET-GA-001；正常资产和代码检查不能扩写成全部错误分支均有测试。工具不证明输入白线、Mesh兼容、UI订阅或完整导航；相关实机证据见旧分批和当前包。不引入大型验收平台，按方案B复用入口。

下一阶段见[M2路线](../M2_Roadmap.md)，当前灰盒与最终招聘作品之间的缺口见[M1复盘](../M1_Review.md)。包 /Raw /Trace仍本地，报告和实际源码 /配置 /可编辑资产由用户按章末清单Commit /Push。
