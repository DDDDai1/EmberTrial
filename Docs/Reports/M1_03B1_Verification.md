# M1-03B1 Health Presenter 验收

日期：2026-10-04。用户约14:14完成，手动60分钟 / 构建与索引等待5分钟。

**功能验收通过，交接B2。**有一处源码一致性补项：现有 `RebuildStatusWidget()` 没有加入 `bUIShuttingDown` 判断，下一批完整Bootstrap代码补齐。该项不等同于已修复；B1原操作文档按用户要求保持原样。M1-03B总任务仍进行中，正式血条View和真实换身体回归尚未验收。

**后续状态：2026-10-04 B2已核对补齐该结束保护并验收通过，补项关闭。**本报告其他内容保留B1验收时证据与工时；当前状态见 [B2验收](M1_03B2_Verification.md)。

## 1. 用户实机结果

- 双目标Build / 索引 / 头文件导航通过；单个UIBootstrap，旧状态Widget配置保留。
- 出生Ready100/100、Fraction=1；扣血 / 治疗事件、满血不变去重、75/100重启观察三次不重置通过。
- 重订阅后一次扣血只有一条变化Snapshot；三轮失去 / 恢复控制、同一身体属性保留、重复恢复无重复Possess通过。
- 非法100/60时Waiting、Effect拒绝、重启观察不初始化；恢复100/100、三次PIE / 清理 / 暂停无执行无补播通过。
- 临时输入 / 变量 / 函数已删除，原1/2/3、2→3、E/P与基础输入正常，源码 / 资产已保存。

## 2. 直接读取的证据

### 源码与构建

对照A2基线共31份源码：新增Presenter两文件；修改UI Build.cs、Bootstrap .h/.cpp三文件；其余26份不变。核心模块未引入UI依赖，UI公开UMG / 私有GameplayAbilities依赖符合模块边界。Presenter弱引用数据源、原生属性委托句柄、Ready事件先订阅后查询、立即快照 / 去重、重订阅保持对象身份和View订阅、Shutdown / BeginDestroy清理均已实现。

Editor receipt时间13:50:48，Game receipt时间13:52:06，均为UE5.5.4 / Win64 / Development对应目标。CLion compile_commands含15份cpp，新Presenter属于UI目标。receipt和索引不替代用户编译结果；本次用户双目标Build同时通过。

旧状态Widget重建函数只有 `!HasBegunPlay()` 保护，缺少结束标记检查；Presenter重启与EndPlay已有结束标记。B2将统一入口保护，并保留原状态面板行为。

### 运行日志

保存的[原始片段](Raw/M1_03B1_Acceptance_2026-10-04.log)来自当前EmberTrial.log，直接覆盖两轮PIE：

| 时段 | 直接证据 |
| --- | --- |
| 14:11:11–14:12:49 | Presenter先Waiting，随后角色100/100初始化 / Ready，再发布Ready100/100；75/100下三次StartSnapshot保持同一Presenter与属性；再次扣血50/100只一条HealthChanged；治疗到90/100；四轮NoPawn→Ready90/100；停止观察后无旧Ready |
| 14:13:14–14:13:39 | 精确非法100/60初始化拒绝；Presenter持续Waiting；三次重启不初始化；停止与ActorInfo清理正常 |

选取片段有5条原生HealthChanged、1条ReadyTrue、8条StartSnapshot、2次Shutdown、1次Health initialized和2次ActorInfo清理。满血再治疗的请求存在，但没有对应的新变化快照。没有匹配到Accessed None、Blueprint Runtime Error、Fatal error等指定错误模式。

**本次直接观察到了Presenter早于GameplayReady的路径。**恢复100/100后的三轮PIE、完整暂停输入矩阵及最终临时蓝图清理按用户反馈登记；现存日志没有覆盖这些全部动作。B2才验证真实Widget订阅 / 晚加入快照 / 显式解绑；真正不同身体及旧数据源失效仍留B3。

### 落盘记录

| 文件 | 最后保存时间 | SHA-256 |
| --- | --- | --- |
| BP_ThirdPersonCharacter | 14:13:44 | `12C9E52B889C87E5DB8323B0E3A0D3B72C762914B44123C012300992D326DCEE` |
| BP_ETPlayerController | 14:19:53 | `CECE16F361EFC30CB5134A880D5065CD3168F30BAE235C0506B0E930E2197EDB` |

Controller的后续保存也纳入最终记录。哈希和保存日志证明文件落盘，不单独证明图表内临时节点已删除。资产来源清单维持25项；B1没有新UE资产。源码基线见[31份哈希](Raw/M1_03B1_SourceHashes.json)。

## 3. 后续与工时

- 当前累计手动1058分钟（17小时38分钟），已报告构建 / 索引 / Cook等待68分钟另列。
- [B2手动操作](../M1_03B2_HealthHUD.md)：两份原生Widget文件、Bootstrap接入 / 结束保护、一个WBP；逐节点连接和独立分组验收。
- 后续文档必须明确执行线、数据引脚、Target、Is Valid对象、有效 / 无效分支；验收按“准备 → 操作 → 画面与日志预期 → 清理”组织。这是用户本轮新增要求，B1操作文档不修改。
- 本轮助手只读取工程证据、保存报告和编写后续文档；未修改Source / UE资产、执行新Build或PIE。
