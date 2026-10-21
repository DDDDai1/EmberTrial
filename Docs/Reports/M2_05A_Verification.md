# M2-05A：韧性属性基础验收

2026-10-21，UE5.5.4。用户20:46全项完成，无失败或特别说明；实现 / 配置60分钟、检查10分钟、构建等待5分钟。**A验收通过，无待补项。**本轮只读核对Source / 资产 / 构建与运行日志，更新报告和开发文档；未修改实际源码 / UE资产、代Build / 测试 / PIE / Cook或操作Git索引。

## 1. 源码、构建与索引

三新 / 三旧共六份Source已落地。五份与候选有效C++ Token一致；Character.cpp仅将Poise默认子对象创建位置从Stamina之后移至Recovery之后，其他有效Token一致。所有子对象仍在构造函数内、BeginPlay前创建，Recovery构造不读取Poise，实际Ready与测试成立，不需为此重排。IsGameplayReady上方旧注释未补“Poise”，条件本身已包含三组，不影响行为；下一批相应门控说明会同步。

实际**123份Source / 65cpp**；CodeModel **Core45 / UI17 / Editor3**。原120份Source中仅Character.h / .cpp、玩法Validator三份改变，其余117份保持。新Poise属性集与真实ASC测试齐全，没有用重命名覆盖旧Health / Stamina集。

最新Source保存20:38:59，Editor receipt **20:40:08**、Game receipt **20:42:05**，对应模块 / exe均晚于Source；双目标构建和索引结合用户反馈接受，本轮没有重跑。运行中的GameplayReady仍表示三组出生配置全部合法并完成一次初始化，Poise0是有效值，不代表死亡。

## 2. 项目校验与自动化

日志UTC均已换为本地时刻：

| 时刻 | 直接证据 |
| --- | --- |
| 20:43:56 | 正式敌人[M1-07C2] Valid / Issues0 |
| 20:44:00 | 训练目标[M1-07C2] Valid / Issues0 |
| 20:44:05 | 玩家[M1-07C2] Valid / Issues0 |
| 20:44:41.455 | EmberTrial.M2.Combat.PoiseFoundation最终Success |
| 20:45:00.097 | EmberTrial.M2.Resources.StaminaFoundation最终Success |
| 20:45:14.317 | EmberTrial.M2.Healing.Ability最终Success |

新测试覆盖真实ASC / Instant GE的Base与Current夹限、上限下降 / 上升、非有限值、合法0、重复Ensure、换控保留与EndPlay；非法Poise在Health / Stamina写入前拒绝，修正后可首次初始化。两条Invalid startup poise为测试预期，非正式身体错误。旧体力基础和正式治疗相关回归通过，不重跑未变整组。

三正式身体的Poise原生引用、Initial100/100、Compile / Save由用户确认；本轮资产 / 配置字节均保持已提交C基线，继承原生默认值即可成立，无需人为改值制造Content差异。结构校验和出生日志提供旁证，不冒充再次逐项读取UE Details。

## 3. 实机与覆盖边界

**20:45:25**，训练 / 敌人 / 玩家各一条Poise initialized 100/100，位于各身体Ready=1之前；**20:46:09**第二轮Play再次各100/100。旧生命 / 体力 / 次数HUD及世界结束清理有日志，左键25、正式2、4→Q、E / P、移动转向跳跃与最终配置按用户全项通过确认。

两轮玩法区间未见Error / Accessed None；编辑器退出时有未完成HTTP遥测请求的Warning，作为环境边界保留，不当作韧性逻辑失败或写成全日志零警告。

本批没有扣Poise的正式技能、自动破势、恢复或新HUD，这些尚未实现；不把测试中的Transient GE调整当成已有范围技能。没有新增临时Controller图或测试资产，没有新Cook / Insights；独立包仍Cleanup/Attempt01，未包含C HUD或本批Poise。

## 4. 工时与下一批

本批手动**70分钟**（实现60、检查10）/ 等待5；M2-05当前70 / 5，M2累计**635 / 69**。项目累计**2646分钟（44小时6分）/ 明确等待254分钟（4小时14分）**。助手设计 / 审查与未报告学习不补算，预算不按录入速度线性缩减。

下一批为[M2-05B1：破势状态与行动门控](../M2_05B1_StaggerState.md)。为控制单批规模，原B拆为B1状态 / 门控与B2归零联动 / 恢复 / 表现。B1先交付独立GAS状态、取消 / 阻断、定时与清理；不提前加入范围查询 / 冷却，也不把Poise0自动触发留成未说明的半成品。B1仍为手动指南，实际Source尚未改动。

## 5. 本次 Commit / Push（固定放在最后）

用户已提交C至本机HEAD **5a340f5**，无需重复Stage旧C文件。本批三份新Source当前为AM，**索引仍是0字节骨架，完整实现只在工作区**；必须在CLion重新Stage最新完整内容，检查Staged Diff。

本次源码为[A第2节](../M2_05A_PoiseFoundation.md#2-准备与精确文件清单)的三新 / 三旧六文件。三正式身体无资产字节差异，不强制重存。文档包含本报告、A指南、破势路线、B1新指南、来源64项及Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / M2路线 / README / Introduction最新差异，详见[A末尾](../M2_05A_PoiseFoundation.md#11-本批-commit--push固定放在最后)。建议Commit：

```text
feat: establish poise attributes and initialization contract
```

用户自行Stage / Commit / Push；助手未联网核验服务器或修改索引。B1代码仅在指南及本地Raw候选中，尚未实施；Raw / 日志 / 生成文件 / Windows包体不上传。
