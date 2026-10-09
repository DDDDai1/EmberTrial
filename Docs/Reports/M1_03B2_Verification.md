# M1-03B2 生命值HUD验收

日期：2026-10-04。用户约20:12完成，手动60分钟 / 构建与索引等待5分钟；清理后的Controller于20:16:56再次正常保存。

**本批验收通过。**两份Widget源码、Bootstrap接入 / 结束保护、正式生命值WBP和所有分组实机反馈通过。B1带入的旧状态Widget重建结束保护已补齐，关闭该补项。M1-03B总任务仍进行中，真正不同身体 / 旧数据源隔离在B3，不把本批同一身体的控制恢复当成正式重生。

## 1. 用户实机结果

用户确认第9.1–9.9全部通过：四BindWidget / 父类 / 无绑定 / Never；单组件两Class；中文与左右面板；扣血 / 治疗 / 零值 / 不变去重；最大值与比例；晚加入75快照及旧Widget仍存活时解绑；重启观察不换对象 / 不重置；NoPawn / 三轮控制恢复；非法配置Waiting及恢复100/100；暂停焦点 / 输入无补执行；十次重建 / 三轮PIE与结束。

临时输入 / 函数 / 两变量已删除、原玩法全部正常、源码 / 资产已保存。因首次磁盘正常保存时间早于测试结束，助手请用户补查清理与Compile / Save；用户确认完成，最新文件与保存记录也已核对。无需重跑已通过测试。

## 2. 直接证据

### 源码与双目标构建

33份源码相对B1：新增ETHealthHUDWidget .h/.cpp两份；修改Bootstrap两份；其他29份不变。四份内容忽略空白后与操作文档一致。包含DisableNativeTick、四个必需绑定、先订阅再快照、移除前主动解绑、NativeDestruct幂等清理；View不读ASC / 写属性。旧RebuildStatusWidget、新重建入口和重启观察均有结束标记检查。

Editor receipt19:06:48，Game receipt19:08:42，均为UE5.5.4 / Win64 / Development对应目标。CLion compile_commands有16份cpp，新增Widget在UI目标；双目标Build / 索引刷新 / 引擎与Presenter导航由用户确认成功。本轮助手未执行新构建。

### 日志范围与统计

[原始片段](Raw/M1_03B2_Acceptance_2026-10-04.log)选取本地EmberTrial.log中 **19:52–20:11:32** 的验收运行，保留638条相关记录。按每轮PIE分别审查，避免同名对象在不同世界的重用造成误判。

| 检查 | 直接结果 |
| --- | --- |
| PIE运行 | 17轮 |
| Health Widget创建 / 移除 | 41 / 41 |
| View订阅 / 解绑 | 41 / 41 |
| 保留旧Widget有效记录 | 24条 |
| ViewDataChanged | 66条 |
| Waiting / NoPawn View记录 | 21 / 20条 |
| 未就绪Effect入口拒绝 | 4条 |
| 同一轮View解绑后仍收到ViewDataChanged | 0条 |
| 指定运行错误模式 | 0条（Accessed None / Blueprint Runtime Error / Fatal error / 运行时缺失必需控件） |

具体证据包括：

- 19:58真实扣血到Ready0/100；19:59的60/60→60/120→100/120→120/120与比例一致。
- 19:59:38新View.SubscribeSnapshot直接75/100，之后治疗与扣血只有当前View响应；多次重建的旧对象仍有效。
- 20:02三次重启观察后当前View仍更新；20:03同一身体NoPawn / 恢复路径保持既有数据。
- 20:04:57精确非法100/60拒绝，四Effect入口Rejected not ready；Waiting下重启与重建保持不可用状态。
- 20:06:23–26十次重建，最后20:06:34一次扣血仅当前Widget响应75/100；20:07、20:09后续PIE出生重新100/100，三轮清理完整。
- 正常出生有Widget先Waiting，再接ReadyTrue / ViewDataChanged的真实顺序，验证数据就绪晚于View订阅。无需人为加Delay。

界面实际位置 / 字体 / 无重复可见面板、Designer Never与无Bind、完整暂停焦点 / 按键矩阵、最终原玩法及节点清理按用户实机反馈登记。日志与落盘记录支持其相关路径，不代替全部画面检查。

### 资产落盘

| 文件 | 正常保存时间 | SHA-256 |
| --- | --- | --- |
| WBP_ETHealthHUD | 19:53:57 | `7059F8BD61699BFFDFD3B9C98EB6E7AFAD1F8BFE2C20F4A1580F2C2783AAB2A0` |
| BP_ThirdPersonCharacter | 20:05:38 | `EB9F22637319E624DB74B0BF5EFE02D608BABDB66783A83797D0B658CB3D6080` |
| BP_ETPlayerController | 20:16:56 | `0AD8611FF2982FE6E23614365A51187F09A2C6A424FCE77A059C8CC79DF28612` |

Controller最新文件22128 bytes，替代测试阶段213582 bytes的版本。正常保存与哈希已核对；临时图表清理本身以用户确认登记。本批新WBP纳入来源清单，现共26项；B3测试Character尚未创建。源码基线见[33份哈希](Raw/M1_03B2_SourceHashes.json)。

## 3. 后续

当前累计手动 **1118分钟（18小时38分钟）**，已报告等待 **73分钟**另列。B1操作文档保持原样。

[B3数据源更换测试](../M1_03B3_DataSourceRebinding.md)采用两个隔离Character与既有裸Pawn，通过Controller临时蓝图验证不同属性源、旧身体Effect / 销毁隔离、当前对象销毁后None / 新实例恢复、早晚就绪及清理。它仍是待用户实施方案，未创建其资产 / 图表、未执行PIE。B3通过后再完成B / M1-03总回归。
