# M2-01A：体力基础验收

更新2026-10-14，UE **5.5.4**。用户22:58报告全项完成、无问题；手动60 / 构建等待5分钟。**M2-01A验收通过，无待补项。**依据用户实机反馈与直接读取的源码、双目标产物 / receipt、CLion CodeModel和当次日志；本轮助手没有重新运行Build或游戏。

## 1. 实施范围与源码核对

当前 **78份Source / 39cpp / 来源清单48项**。相对C3新增Stamina属性集两文件与StaminaFoundation测试，修改Character两文件和既有GameplayAssetValidator；其余72份旧Source逐份哈希不变。三新文件全文、三个角色接口 / 初始化函数、IsCombatDead与ValidateBody忽略空白后符合指南；Character其余实现与M1版本、校验器其余实现与C2指南一致。

`HealthAttributes`原默认子对象名称 / 类型保持；新`StaminaAttributes`独立创建。全部启动配置预检查通过后才写生命 / 体力、置成功标记并发布Ready，旧身体重新控制与重复Ensure保留数值；EndPlay撤销Ready。此前LNK2019源于IsCombatDead声明存在而实现遗漏，用户已补回原函数，当前定义唯一、正常链接通过，补项关闭。

48项登记资产均与C3相同，包括三个正式身体；Compile / Save没有产生新的二进制差异，不人为Stage无差异资产。共享Config / uproject、冻结B1文档与C3 Attempt02七个包指纹保持原值。此次没有新增体力蓝图 / GE / Widget，也没有新Cook；当前包仍是M1历史基线，不能验证新体力代码。

## 2. 构建与索引证据

- Editor receipt：5.5.4 / Win64 / Development / Editor，22:50:56；Core DLL同刻，晚于角色最终修复22:50:42。Editor校验器DLL22:48:25，晚于其源码22:45:52；不同模块按需要构建，时间不同正常。
- Game receipt：5.5.4 / Win64 / Development / Game，22:52:44；Game exe22:52:43。UBT备份日志记录8个动作、三个相关cpp编译、Game链接 / WriteMetadata完成，总60.09秒。最新UBT记录是22:53的Editor up-to-date / 0.47秒，不冒充一次全量编译。
- CLion三个CodeModel的实际源列表合计39cpp，包含新属性实现 / 测试和既有Editor校验器。构建 / 三索引符合用户结果，不据源列表推断未单独检查的导航或编辑器体验。

用户登记的整体等待5分钟保留，不以某次UBT内部时长替换。Game Target仍沿用原Runtime边界，没有加入Editor依赖。

## 3. 自动测试、静态配置与短基线

**22:57:10.301**，`EmberTrial.M2.Resources.StaminaFoundation`一项 `Result={Success}`，BeginEvents / EndEvents间无失败记录。源码与该次运行共同支持：40/80、0/80合法、100/60与NaN拒绝且不部分写生命、修正后成功、真实Instant GE的消耗 / 恢复 / 上限与Base夹限、重复Ensure保留、两个Controller控制同一身体保留、EndPlay不可重新初始化 / 快照清零。

两条非法启动在源代码以Warning输出，测试框架按预期消息捕获后在运行日志显示Verbose；这是预期负例，不是用户删掉保护或发生正式身体初始化失败。两次15/80的Already initialized及换控后40/80在日志直接可见。其他数值断言以该版本测试的Success为据，不虚构每个断言都单独打印了数值。

三个正式身体分别在 **22:55:53、22:56:20、22:56:25** 出现项目 `Valid / Issues=0`。新ET-BODY-004已实现，正常资产通过；该Editor错误分支没有单独做隔离负例，运行非法配置用例不能替代Editor负例测试。

两轮正式地图PIE：**22:57:35–58**与**22:57:59–22:58:02**。每轮玩家 / 敌人 / 训练目标各一次Stamina100/100，随后Ready=1；Stop各一次Ready=0和原ActorInfo / Life / UI清理，没有结束后重新初始化。玩家HUD的生命Ready100 / Fraction1有日志。

第一轮直接记录左键对应的正式攻击链、训练目标100→75 / Base75、原型闪避150.1cm / SourceTime0.300、E激活M1_Target_A、Pause显示 / 恢复。具体键鼠选择、中文、移动 / 转向 / 跳跃与最终保存按用户反馈接受。全日志没有Log级Error / Fatal、Accessed None或Ensure失败；Editor启动的图标 / EOS与关闭HTTP Warning单独保留，不当作体力玩法错误。

本批没有正式体力成本、恢复计时或资源UI；按2仍为M0原型，不消费体力符合当前范围。未运行旧M0测试、不重做旧HUD / 死亡矩阵、不以本批通过推断未来GAS闪避或持续Buff已完成。

## 4. 工时与后续

本批60 / 等待5；累计已报告用户手动 **2071分钟（34小时31分钟）**、已明确等待 **190分钟（3小时10分钟）**。M1手动1338分钟保持，M2当前手动60 / 等待5。助手核查与未知学习成本不补算。

M2-01仍分批推进：A属性 / Ready完成，后续接体力恢复与必要变化观察 / 显示；治疗次数已定契约，实际治疗留M2-04。下一小批尚未实施。原M2基础65小时和全局缓冲口径保持，单批完成不代表整个M2-01或四能力完成。

本地证据：`Raw/M2_01A_2026-10-14/Acceptance.json`、`AcceptanceExcerpt.txt`及已读取的构建 / CodeModel来源。原Preparation记录的是下发时状态，保留历史，不用原“Source未创建”的断言检查当前已实现工程。

## 5. 本次 Commit / Push（固定放在最后）

实现主体已进入本地 **4fedf14**，当前缓存origin/main仍e3ee554；本轮未联网确认远端，不宣称GitHub已同步。此次追加本报告、A指南验收状态、路线 / Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow / Introduction真实收尾差异；已有实现没有新差异便不重复提交。

三正式蓝图哈希不变，无新增体力UE资产；Raw / 日志 / receipt / CodeModel / Binaries / 旧包留本地。CLion刷新Changes、重新Stage最新验收文本、检查Staged Diff后Commit / Push；可用 `docs: accept M2 stamina foundation`。Git操作仍由用户执行。
