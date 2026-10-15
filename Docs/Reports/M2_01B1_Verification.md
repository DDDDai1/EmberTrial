# M2-01B1：体力恢复验收

更新2026-10-15，固定UE **5.5.4**。用户13:27报告全项完成、无问题；手动40 / 构建等待5分钟。**B1验收通过，无待补项。**依据现有源码、双目标产物 / receipt、三模块CodeModel、当次日志与用户画面反馈；助手本轮没有重跑Build、测试或游戏。

## 1. 实施与文件路径

当前 **83份Source / 42cpp / 来源清单48项**。对比A：五新 / 七旧Source，另外71份旧Source哈希保持。五个新文件忽略空白后全文符合B1指南；七个修改文件扣除指定插入 / 替换后与A一致，原IsCombatDead与旧业务保留。

原生Instant StaminaDelta GE只负责增量；恢复组件拥有计时器与订阅，实际降低后0.75世界秒延迟、20点每秒、0.10秒采样，正式忙碌 / 死亡 / EndPlay停止。世界时间积分、极小float增量积累、观察序号与消耗修订号保护均在实际源码；通用GE不等于已实现正式能力的成本事务。

用户误建的Public / Private Effects与Resource目录，已按当次明确授权移至各自Abilities/Effects与Resources，并改正Character / Controller三处include；四个文件移动时字节保持。当前实际源码与三模块索引都使用正确目录。除此以外，本批业务代码均由用户实现，本轮验收不再改Source。

三正式身体和全部48项资产哈希与A相同，Compile / Save未产生新的二进制差异；共享Config、uproject、冻结HealthPresenter指南、M1 C3 Attempt02七个指纹不变。没有新UE资产或Cook，旧包不能验证B1。

## 2. Build与索引

- Editor receipt13:16:50，Core DLL13:16:41，Editor校验器DLL13:16:44，均对应Win64 / Development / UE5.5.4。
- Game receipt13:18:49 / exe13:18:48；UBT备份记录12个动作，包含三个新增cpp、Controller / Character / Tags及两个资源测试，链接与WriteMetadata完成，总55.12秒。
- 最新UBT记录13:19:04是Editor up-to-date / 0.44秒，不能当作再次全量编译。
- 三CodeModel实际列表合计42cpp，包含正确位置的Effects / Resources新实现与测试，没有旧目录副本。

用户登记的5分钟等待保持，不用单次UBT内部耗时替换。

## 3. 自动化、正式PIE与覆盖边界

两项各一次Success：**13:23:19.772 StaminaFoundation，13:23:19.862 StaminaRecovery**。新测试覆盖真实GE消耗、延迟与再次降低重置、世界暂停时资源 / 时钟冻结、恢复后无暂停或忙碌整段补量、上限增加 / 满值停表、重复观察、旧回调拒绝、Deactivate / Activate、非法速率、真实生命归零死亡和EndPlay。非法速率一条Warning被框架作为预期消息捕获，运行日志显示Verbose；没有忽略所有Warning。

三正式身体分别在13:22:28、45、50通过项目Valid / Issues0。ET-STAM-001正常参数已检查；未另做Editor负例，不用运行速率负例代替静态负例证据。

两轮正式PIE约 **13:25:34–13:26:49、13:27:04–13:27:12**；每轮三身体各一次Observe ready100/100、对应Observe stopped。第一轮开发查询100 / Timer0，注入-25后75 / Timer1；Time42.846，NotBefore43.596，实际Recovery begin43.662，约1.2秒后Full停表。第二次-25后约0.33秒Stop，Unsubscribe停表；新一轮重新100/100。未发现Stop后旧恢复继续。

正式Pause前后两次读数及非零拒绝没有在此次日志留下独立命令样本，按用户实机反馈接受；暂停资源与世界时钟的定量结论另有新自动测试Success支持，不宣称日志打印了未采集的每项数值。左键真实25有日志；E中文、P恢复、原2、移动 / 转向 / 跳跃与保存按用户反馈接受。全日志无Log级Error / Fatal、Accessed None或Ensure失败。

当前按2仍是M0原型，不消费体力，也没有正式Dodging Tag阻断恢复；这符合本批边界，正式迁移留M2-02。没有体力UI；下一批才接事件显示。不追加旧HUD / 帧率 / 死亡全矩阵，未检查的未来功能不写成已完成。

## 4. 工时与后续

本批40 / 等待5；累计用户手动 **2111分钟（35小时11分钟）**、已明确等待 **195分钟（3小时15分钟）**。M2累计手动100 / 等待10；M1手动1338分钟保持。不补算未知学习或等待。

下一批[B2体力Presenter / HUD](../M2_01B2_StaminaHUD.md)只读资源、复用LocalPlayer观察和现有Widget校验。B1完成不代表M2-01B整章、正式体力成本或四能力完成。Raw证据保存于`Raw/M2_01B1_2026-10-15/Acceptance.json`与摘录；此前Preparation是历史下发状态，不用其“新文件尚不存在”的断言重验当前实现。

## 5. 本次 Commit / Push（固定放在最后）

实际HEAD仍4fedf14，本轮没有联网核验GitHub。五新文件按B1正确路径纳入；Tag / Character / Controller各两文件与Gameplay Validator纳入完整最新版本。旧Effects / Resource路径仍有先前Stage记录，当前为AD，新正确路径未跟踪：**提交前必须重新Stage四个旧路径的删除与四个新路径的完整文件**，并重新Stage测试最新内容；不能只提交上次暂存状态。

本报告、B1验收状态及路线 / Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow / Introduction实际差异随文档提交；B2只有指南，尚无B2实现可提交。48项资产无差异，不人为Stage。Raw / 日志 / receipt / CodeModel / Binaries / 旧包留本地；用户在CLion检查Staged Diff后Commit / Push，助手不操作索引或远端。
