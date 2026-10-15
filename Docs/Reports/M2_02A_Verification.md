# M2-02A：GAS闪避动作与成本验收

2026-10-15，UE **5.5.4**。用户15:45反馈全部完成、无问题，手动50 / 构建等待5分钟。**A批验收通过，无待补项。**本次读取实际源码、构建收据 / 产物、三模块索引、资产哈希和现存日志，没有修改Source / UE资产、重跑Build / 测试或操作游戏。

## 1. 实现与资产

实际 **90份Source / 46cpp / 51项来源清单**。相对M2-01B2，新增Dodge Ability两文件与CostContract测试，十份旧Source按指定范围更新，其余77份旧Source哈希不变。十三文件中十二份忽略空白后与指南一致；HitReact的一处`A || (B || C)`写为`A || B || C`，均为布尔短路或且求值顺序一致，语义等价，无需补修。

新能力拥有Montage Task、State.Dodging及本次成本提交。检查实际播放实例后才Commit；ApplyCost在父类GE应用前置提交标记，拦截同步重入 / 重复扣费。结束解绑回调，由GAS结束任务并移除Owned Tag；Character授予 / 清理Handle，正式Idle、攻击、旧Practice、受击取消链与恢复Tag订阅 / 解绑同步扩展。Runtime与Editor共用成本Schema检查。

新增`GA_ET_Dodge`和`GE_ET_DodgeCost`。成本资产为Instant、单Stamina Add(Base)、固定-25；GA使用现有闪避Montage、标准Cost Class、空Graph。唯一旧资产哈希变化为BP_ThirdPersonCharacter的Dodge Ability Class；其余48份旧清单资产保持，包括三既有GA，Compile / Save没有强行产生二进制差异。清单新增两行、更新玩家配置行，总51项；无外部美术引入。

共享Config / uproject、冻结M1-03B1指南和M1 C3 Attempt02七个包指纹保持。A仍是原地正式动作；原2为M0位移原型，正式方向位移 / 输入迁移留B，无敌窗口留C。没有M2新Cook，旧包不含本批能力。

## 2. 构建、索引和资产校验

- Editor receipt **15:28:44**；Core与Editor模块DLL均晚于本批源码。
- Game receipt **15:30:42**，exe15:30:41；UBT记录16个构建动作，链接完成，总70.65秒。最新Editor日志为up-to-date / 0.50秒，分别记录，不当作又一次全编译。
- 三CodeModel为Core30 / UI13 / Editor3，共46cpp；新Ability与测试均在Core目标。未变模块的索引文件时间较早符合未增文件的事实。
- 六个要求的资产均有本项目Valid / Issues0：新GE / GA、玩家和三既有GA。

15:44:30唯一AssetCheck Error为主动清空DodgeMontage的`ET-GA-001`负例；随后请求Accepted0，没有新Begin。恢复后 **15:44:55 Valid / Issues0**，该项关闭。日志没有玩法Error / Fatal、Accessed None或Ensure失败；编辑器资源图标 / EOS / 退出遥测Warning不归为本批战斗失败。

## 3. 自动化与运行证据

四项均直接读到一次Success，本轮审查没有重跑：

| 本地时间 | 测试 | 覆盖边界 |
| --- | --- | --- |
| 15:41:45 | Resources.StaminaFoundation | 属性初始化 / 夹限等既有规则 |
| 15:41:45 | Resources.StaminaRecovery | 既有恢复规则及新增Dodging阻断 / 继续 |
| 15:41:59 | Dodge.CostContract | 加载真实Cost GE；足量 / 不足 / 精确支付 / 重复与同步重入 / 无Cost / 死亡拒绝 |
| 15:42:16 | M0.PracticeAction.Lifecycle | 共享Practice入口变更的既有生命周期回归 |

CostContract使用已授予能力实例直接检查受保护成本方法，**没有完整激活Montage**；不以该Success代替动画端到端。旧DodgeMovement物理代码未改，本批无需重做墙坡 / 帧率矩阵。

正式运行有五组Begin / End，每次100→75、Cost25 / Dodging1，结束均CostSubmitted1 / Dodging0。其中三次正常结束，两次取消；15:43:37真实生命扣25后先取消闪避，再进入HitReact，15:44:09停止PIE时取消并清理。15:43:39真实死亡、15:43:52 Retry以及下一身体重新授予能力均有日志。该死亡发生在受击已取消闪避之后，不额外写成“仍活动闪避被致命伤直接取消”的独立样本。

忙碌重复、旧2 / 攻击 / E / 跳跃互斥、3取消不退款、Pause拒绝无补播、HUD与最终短基线按用户实机反馈接受。现存Display日志没有每次拒绝 / 暂停请求或恢复slomo1命令，不把缺少打印变成额外旧功能补测；最终slomo1和配置恢复按用户确认。实际Montage_Play底层失败、同步结束极端时序仅有代码保护，本次不宣称已动态覆盖。

## 4. 工时与下一批

本批手动 **50分钟**、构建等待 **5分钟**；累计手动 **2191分钟（36小时31分钟）**，已明确等待 **205分钟（3小时25分钟）**。M2累计手动180 / 等待20分钟；独立检查用时未拆报，不推算检查占比或长期效率。原90–150分钟是预算，不要求补足差额。

下一批为 **M2-02B：方向位移与2键正式输入迁移**。成功固定世界方向、碰撞与自身位移清理成立后再断开旧2；Montage结束与位移结束分开处理，暂停 / 取消 / 死亡沿用所有权边界。B完整操作另见[B指南](../M2_02B_DirectionalDodge.md)，本报告不要求预建空类或提前改输入。C再处理短无敌。

本地证据在`Raw/M2_02A_2026-10-15/Acceptance.json`、AcceptanceExcerpt与构建摘录；之前Preparation记录保留为下发时的历史状态。

## 5. 本次 Commit / Push（固定放在最后）

本地HEAD与缓存origin/main均 **c6b8a0f**，本批代码和资产已纳入；本轮未联网核验远端 / LFS。没有新差异的Source / uasset无需重复提交。

本轮提交本报告、AssetManifest两项新增 / 玩家行更新、A指南验收状态，以及Progress / Backlog / M2路线 / 闪避路线 / Architecture / VersionControl / AcceptanceWorkflow / Introduction实际状态更新。Raw / 日志 / receipt / CodeModel / Binaries / 历史包留本地。用户在CLion刷新Changes、重新Stage最新文档差异、检查Staged Diff后Commit / Push，可用`docs: verify GAS dodge action and stamina cost`。助手未操作Git索引或远端。
