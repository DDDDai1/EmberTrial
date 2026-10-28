# M2-07：战斗集成、性能样本与复盘总验收

2026-10-27，UE5.5.4。A独立包 / B性能分析通过，C材料交付收尾，进入[M3-01遗物静态定义](../M3_01_RelicDefinitions.md)。本报告汇总已有证据，没有新增一次全组 / 打包 / 键鼠或性能执行。

| 批次 | 已有证据 | 用户手动 / 明确等待 |
| --- | --- | ---: |
| [A新版独立包](M2_07A_Verification.md) | 23项全组Success；Attempt01 Build / Cook / Stage / Package成功，两进程真实玩法 / 菜单 / Retry / Continue与退出 | 30 / 10分钟 |
| [B性能分析](M2_07B_Verification.md) | 同包指纹，Idle与真实AI交战，六CVar查询 / 十sg回显、完整Trace导出、有效窗与完整区间尖峰 | 30 / 10分钟 |
| [C技术复盘](M2_07C_Verification.md) | 真实链路 / 生命周期 / Bug案例 / 性能边界、M3范围校准；用户要求继续下一章 | 未登记 |
| 已量化合计 | 未登记不等于0，不计助手整理 / 分析 | **60 / 20分钟** |

最新可运行基线是`Artifacts/Builds/M2_07A/Attempt01/Windows/EmberTrial.exe`，157 Source / 86cpp、Core60 / UI23 / Editor3、23自动化、294 Content / 来源75；M3尚未实施。旧Cleanup / M1包只代表历史版本。

B内窗待机 / 交战主循环间隔P95约5.915 / 5.714ms；完整Combat另有3次>16.7ms Tick、最大30.013ms，未唯一归因。1920×1080为请求，PNG2048×1081为实际截图尺寸，电源 / 风扇档位未知；不作严格1080p / Shipping / 最终美术认证，不与M1 High直接计算提升。当前无持续瓶颈证据，无补采 / 工程修复要求；首次PIE等待需求保持关闭。

包内双敌人切换正例、全分辨率焦点、第二设备、磁盘存档、装备页面与最终动作仍未在本阶段完成；不能把已有专项 / 单包短查扩成这些结论。M2后续内容边界见[M2总报告](M2_Verification.md)。

## Commit / Push（固定放在最后）

新增本报告与C / M2总报告，提交范围统一见[M3-01末尾](../M3_01_RelicDefinitions.md#11-本批-commit--push固定放在最后)。英文`docs: close M2 and prepare relic data definitions`，中文译文`docs: 完成M2阶段收尾并准备遗物数据定义`。运行工程与原包不变，Raw / Trace / 日志留本地，Git由用户操作。
