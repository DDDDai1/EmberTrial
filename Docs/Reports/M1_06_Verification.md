# M1-06：菜单、失败、检查点与暂停总验收

复核：2026-10-13，UE5.5.4。**A / B / C / D功能验收完成，无未关闭功能项。**采用方案B复用各批证据，本次汇总不增加运行测试或临时蓝图。当前69Source / 34cpp / 来源清单46；当前阶段独立包与性能结果留[M1-07](../M1_07_ReleaseRoadmap.md)，尚未提前计为完成。

| 批次 | 已成立的行为 | 证据 | 手动 / 等待 |
| --- | --- | --- | --- |
| A | 独立菜单世界，开始 / 退出、键鼠焦点、旧页清理与新战斗输入 | [A报告](M1_06A_Verification.md) | 45 / 5分钟 |
| B | 玩家Death sealed后一次失败呈现，非玩家隔离、整图Retry / 主菜单、请求拒绝可恢复 | [B报告](M1_06B_Verification.md) | 80 / 5分钟 |
| C | Entry / Checkpoint_A、Overlap激活、重试 / 会话继续、NewRun重置、缺失ID回退并修正快照 | [C报告](M1_06C_Verification.md) | 85 / 5分钟 |
| D | 单FailureFlow统一Pause / Failure，P释放与Slate KeyUp、焦点 / Flush、暂停返回菜单与结束清理 | [D报告](M1_06D_Verification.md) | 105 / 5分钟 |

责任链：Core Controller发送意图 → UI Controller组件检查当前身体 / 页面 / pending → Widget只呈现和发按钮意图 → RunSession校验并请求OpenLevel → 旧世界解绑 / 新Controller与HUD读新身体快照。GameMode在Pawn生成前选择检查点；GameInstance只保留地图包名与稳定ID，不保留旧Actor、ASC或Widget引用。

当前“继续本次试炼”只在进程内有效，重建世界、新身体满血、敌人与练习交互回初始；直接“继续游戏”保持同一世界。退出程序后会话清除，M5再接磁盘存档和世界进度。死亡表现仍为冻结占位，未承诺倒地动画已完成。真正TravelFailure、暂停中外部死亡和另一台机器兼容性未专项运行，分别在既有报告标明。

实际问题记录：A的Unity匿名命名空间碰撞已修；旧空HUD Warning在B收口；C明确非持久存档；D修正StatusText层级、两种Padding入口与左键Branch断线。左键最终连接Is Game Paused查询并保存，今后最小门控校验应检查关键Condition是否有真实数据来源，Compile成功不能代替行为检查。

总计手动**315分钟（5小时15分钟）**、等待20分钟。项目累计手动1836分钟（30小时36分钟）/ 等待140。下一步执行M1-07A，获取当前流程的Win64 Development独立包；后续采样与工具基于这一版真实产物，不用M0旧exe补证。

## Commit / Push

本报告与四份分批报告、实际来源清单 / 状态文档随最新D实现提交；D Source / 四旧资产以[D最后清单](../M1_06D_PauseFlow.md#12-本批需要commit--push的内容固定放在文档最后)为准。下一章新增文件单列在M1-07A章末。Raw、Artifacts、生成文件留本地，用户手动Commit / Push。
