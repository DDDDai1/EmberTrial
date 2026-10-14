# M1：最小纵向闭环总验收

更新2026-10-14，UE5.5.4。**M1-01至M1-07完成，按当前M1范围无未关闭功能项。**不代表Introduction的P0 /P1全部完成；最终四类能力、构筑、第二敌人 /守关者、持久存档、美术 /发布 /面试仍待后续。

| 章节 | 已完成交付 | 原证据 |
| --- | --- | --- |
| M1-01 | 正式灰盒地图 /阻挡 /出生与模板回归 | [地图](M1_01_Verification.md) |
| M1-02 | Core /UI Runtime边界、GameMode /Controller、LocalPlayer Pawn生命周期、UI装配 | [对象 /模块](M1_02_Verification.md) |
| M1-03 | Character持有ASC /AttributeSet、Ready一次初始化、Health GE边界、Presenter /UMG快照与重绑 | [GAS与HUD](M1_03_Verification.md) |
| M1-04 | 正式轻攻击、Notify窗口 /Trace去重、真实GE伤害、受击 /死亡与清理 | [战斗](M1_04_Verification.md) |
| M1-05 | AI感知 /黑板 /BT、追击 /转向 /自动攻击、阵营与有限失败重试 | [敌人](M1_05_Verification.md) |
| M1-06 | 独立菜单、互斥Pause /Failure、Retry /Continue、会话检查点、输入焦点与世界清理 | [流程](M1_06_Verification.md) |
| M1-07 | Development独立包、实际Insights、两个Editor校验器 /配置脚本、Attempt02 | [阶段包 /工具](M1_07_Verification.md) |

当前75Source /37cpp、三个模块（Core /UI为Runtime，Editor只做工具）、来源清单48项。48为登记的使用 /原创资源，不是Content总资产数。Source /资产与C2一致，新包对应本次Game构建与共享配置。

当前可运行：`E:\UE_Project\Demo\Artifacts\Builds\M1_07C3\Attempt02\Windows\EmberTrial.exe`。包内玩家25 /敌人10、HUD同步 /死亡、Radius90真实追击、Checkpoint_A的Retry /Continue与新进程清会话有新证据；键鼠 /中文 /站定按用户反馈。原M1中不同身体、取消 /Pause、碰撞 /帧率等检查复用各批报告，不在本汇总重跑。

已知边界：单敌人灰盒、单段正式攻击、M0原型闪避 /3调试受击 /4伤害入口、死亡冻结占位。Continue为当前进程地图 /检查点，切图重建世界、退出清除；没有磁盘持久化或世界进度存档。复杂状态 /资源消耗 /连击 /锁定 /四能力正式闭环留M2。另一设备、真实TravelFailure、暂停中外部死亡、最终美术压力未专项实测，保留各报告限定。

本阶段登记手动 **1338分钟（22小时18分钟）**；M0+M1累计手动2011分钟（33小时31分钟）、已明确等待185分钟另列。只统计用户反馈，不补算助手设计 /排障、未报告学习 /额外练习或未知等待；不将22小时当团队总成本或据此线性压缩后续。

工程事实、模块分工、Bug案例、性能口径与[M2校准](../M2_Roadmap.md)收于[M1阶段复盘](../M1_Review.md)。Source维护和Git操作沿用原约定；本轮助手仅总结证据 /文档，无新的工程实现或运行测试。
