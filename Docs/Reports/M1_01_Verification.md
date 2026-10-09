# M1-01：正式纵向测试地图验收

验收日期：2026-10-02。结论：**M1-01 通过**，可以进入 M1-02A 模块骨架。此结论覆盖新地图与现有 M0 功能冒烟，正式战斗 / HUD / 重试尚未实现。

## 用户实机结果

用户约 20:10 完成，手动用时约 **20 分钟**，确认以下项目通过：

- 地面、四面墙、单个 PlayerStart；两次 Play 的出生、移动 / 转向 / 跳跃及墙壁阻挡。
- 1 / 2 / 3、闪避受击中断、墙边位移；E 的 A / B 提示、重新 Play 状态重置。
- P 暂停、鼠标 / 键盘继续；地图与资产均已保存。

没有直接读取关卡 Actor 的数量 / Transform，也没有本批录像。几何布置、两次出生、具体键鼠操作及视觉表现按用户反馈登记。

## 文件与日志证据

| 项目 | 直接读取结果 |
|---|---|
| UE 资源 | `/Game/EmberTrial/Maps/L_ET_VerticalSlice` |
| 文件 | `E:\UE_Project\Demo\EmberTrial\Content\EmberTrial\Maps\L_ET_VerticalSlice.umap` |
| 大小 / 最后保存 | 59,562 bytes / 2026-10-02 20:09:25 |
| SHA-256 | `D658617D310F9A808DE4D8512D5CFA2F10BFF180C1B2CC35FBC81137CCDBDB2B` |
| 日志 | `E:\UE_Project\Demo\EmberTrial\Saved\Logs\EmberTrial.log`，UE 5.5.4 |
| 归档片段 | `E:\UE_Project\Demo\Docs\Reports\Raw\M1_01_Acceptance_2026-10-02.log`（本地忽略文件） |

日志时间原文是 UTC；下表换算为本机 UTC+8：

| 本机时间 | 可直接确认的事件 |
|---|---|
| 20:09:25 | 新地图最终一次保存 |
| 20:09:33 | 新地图创建 PIE 世界；M1_Target_A / B 初始化；当前角色 PracticeAction 为 Idle |
| 20:09:40–41 | Attack Begin → Completed |
| 20:09:42 | 闪避后被受击打断；Travel2D=83.3 cm，SourceTime=0.167 s；随后 HitReact 正常结束 |
| 20:09:49 | 闪避窗口结束，Travel2D=30.2 cm / SourceTime=0.300 s；Montage 随后结束 |
| 20:09:53 / 55 | M1_Target_A / B 激活 |
| 20:09:59–20:10:03 | Pause opened → Resume requested → Pause closed |
| 20:10:04 | 新地图 PIE 开始正常清理 |

30.2 cm 样本与用户的靠墙阻挡场景相符；有碰撞时不要求达到配置的 150 cm。日志本身只记录结果，不能独立证明墙体姿态。

当前日志直接保留 **一次**新地图 PIE；两次测试与重开重置按用户反馈登记，不要求补跑。未匹配到 Blueprint Runtime Error、Accessed None、LogBlueprint / LogScript Error、Fatal error 或 Assertion failed；这项检查不代表所有潜在错误都已排除。

## 交接

- 新地图加入 [资产来源清单](../AssetManifest.csv)，当前为 16 项。
- 已登记手动用时：M0 673 min + 本批 20 min = **693 min（11 h 33 min）**。此前报告的构建等待 26 min 另计。
- 默认启动 / Cook 地图仍保持原设置；旧 M0 包的内容不会因源码地图保存而更新。M1-07 再切换并验证阶段包。
- 下一批：[M1-02A：Runtime 模块与 CLion 索引](../M1_02A_RuntimeModules.md)。所有代码仍由用户按文档手动实现。
