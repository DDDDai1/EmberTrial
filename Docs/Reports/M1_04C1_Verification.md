# M1-04C1：正式受击验收

日期：2026-10-06。用户全项通过、无失败或特别说明，**C1验收通过**；C2死亡尚未实施。助手本轮只读证据并写文档 / 清单，未改项目Source或UE资产，未执行Build / PIE。

## 1. 源码、构建与资产

- Source由41到45：新增CombatLife与HitReact四文件，修改Tag / Character各两份及LightAttack.cpp。其余36份与B2基线逐份SHA-256一致。C1指南13段C++去除空白后全部匹配实际源码，四份新文件与各自全文完全一致。
- CombatLife不Tick、按身体观察真实Health、独立授予 / 清理受击Handle；回调同步读取GEModData。HitReact为ServerOnly，可服务没有Controller的训练身体；正式攻击与练习闪避取消、结束清理和门控已核对。
- Editor receipt20:25:14、Game receipt20:27:55：UE5.5.4 / Win64 / Development，目标分别Editor / Game。EmberTrial/cmake-build-debug/compile_commands.json于20:25:22更新，22个独立工程cpp。用户确认Build / 导航成功；本轮助手未重新编译。
- GA_ET_HitReact20:30:59、目标20:38:08、地图20:38:43、玩家Blueprint最终21:03:11保存。原34项中玩家 / 目标 / 地图改变，其余31项哈希不变；新增GA后登记35项。内部Class / Montage / Quinn配置和4入口保留据用户实机确认，文件存在 / 哈希不独立证明蓝图连线。

保存[45份Source / 35项资产基线](Raw/M1_04C1_Baseline.json)。M1-03B1旧操作文档未修改；Game Build仍不等于新Cook / 打包，M0包属于旧阶段。

## 2. 日志与反馈的证据范围

用户填写完成时间20:40；当前EmberTrial.log保留的实机样本实际是**21:01:16–21:05:17**。两个时间分别记录，不称日志精确覆盖20:40，也不因时间不同要求重跑。相关[原始片段](Raw/M1_04C1_Acceptance_2026-10-06.log)保存了配置保存、六轮PIE样本和中途编译错误。

- 12条Life observing / 12条Life stopped，六轮各两身体；17条Hit begin / 17条Hit end，结束HitTag均0，包括停止PIE时Cancelled1。
- 3次正式攻击真实结算目标：100→75、75→50及下一轮100→75；目标随HealthLoss触发受击。没有Controller仍能激活；脚部 / 姿势正常据用户观察。
- 玩家真实25伤害与HUD75、50、25、0同步。受击期间继续扣血的两次HealthLoss请求Accepted0 / BusyOrMissing1，没有重新开始当前受击。
- 21:03:39攻击在窗口前被3取消：Disarm / Tick0、AttackingTag0，然后正式Hit begin。21:03:42闪避被3取消：Travel2D50.0 / SourceTime0.100、练习状态Idle，再开始受击。
- 6组准确Arm / Disarm均最终Tick0；不能用忽略大小写的“Arm Owner”直接计数，因为它也匹配Disarm。
- 受击中暂停有直接样本，恢复后完成；暂停期间3/4无执行 / 无补播、完整输入门控、最后E/P和基础输入按用户通过反馈登记。最后一轮有HUD100出生和真实扣血 / 暂停后停止，没有将其冒称为所有短基线日志。
- 20:43:36有一条历史蓝图编译Error：ETPracticeActionComponent的Target未连接；20:44及21:03正常保存，之后有成功受击 / 闪避取消 / 玩家损伤样本，用户确认最终Compile / Save通过。21:01–21:05玩法片段未检出Accessed None、Blueprint Runtime Error、LogScript Error或Fatal error。历史编译错误保留归档，不标为当前仍失败。

致命伤害目前只出现DeathPendingC2=1，未执行死亡；已有受击仍可自然结束。这正是C2需要补齐的行为，不把0血当成死亡验收。

## 3. 工时与下一批

C1手动25分钟（代码10 / 资产10 / 检查5），构建等待5分钟。M1-04 A / B1 / B2 / C1合计手动150 / 等待17分钟；全部已报告累计**1348分钟，22小时28分钟**，等待**90分钟**另计。

继续[C2一次死亡与行动封锁](../M1_04C2_DeathState.md)：复用CombatLife和数字4，只补死亡 / 直接依赖检查，不重搭Controller验收函数或旧HUD矩阵。M1-04C / M1-04 / M1总体仍未完成，D及正式重试 / 新包后续推进。
