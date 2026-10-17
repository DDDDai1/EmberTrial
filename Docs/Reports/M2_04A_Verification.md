# M2-04A：治疗次数与成功生效事务验收

2026-10-17，UE5.5.4。用户19:17全项完成，无特别问题，手动60 / 构建等待5分钟。**本批验收通过，无待补项。**以下区分源码 / 构建 / 日志直接证据和用户对画面 / 操作的确认；助手仅只读核对，没有代改Source / UE资产、执行Build / 自动测试或操作Git索引。

## 1. 源码、构建与配置

五新 / 五旧十份实现与[A指南](../M2_04A_HealingResource.md)候选的C++ Token全部一致；一份文本一致，九份只有排版 / 文件结尾换行差异。其余95份旧Source保持M2-03C哈希。实际**105Source / 56cpp**；三个CLion CodeModel实际为Core40 / UI13 / Editor3，新资源 / GE / 测试都在Core目标。

最新Source保存早于Editor产物：Editor receipt **18:19:24**，Core DLL18:19:10、Editor DLL18:19:14、UI DLL18:19:23；Game receipt **18:20:47**、exe18:20:46，均覆盖最终本批代码。版本均为5.5.4 / Win64 / Development。最新UBT记录Editor up-to-date；正常双目标Build和索引导航结合用户反馈，不将up-to-date冒充额外一次全量构建。

资源由Character创建固定名HealingResource的单个继承子对象，玩家独立初始化，敌人 / Neutral休眠。Health / Stamina的GameplayReady实现保持原样；资源无Tick / Timer、无UI或Montage依赖。固定原生Instant Health Add(Base) / SetByCaller.HealthDelta成功后扣一次，使用WasSuccessfullyApplied判断；同步Health / 次数回调期间重入拒绝，失活恢复不补满，EndPlay解绑 / 撤销快照。开发ET_Healing仅PIE修改，查询与正式治疗能力分工明确。

| 三个正式身体本项目校验时刻（本地） | 结果 |
| --- | --- |
| 玩家19:13:12.843 | BP_ThirdPersonCharacter Valid / Issues=0 |
| 敌人19:13:31.161 | BP_ETMeleeEnemy Valid / Issues=0 |
| 训练目标19:13:38.118 | BP_ETCombatTrainingTarget Valid / Issues=0 |

玩家出生3/3、资源可用与后续实际消费由日志直接支持；三身体单继承组件 / AutoActivate、Designer与Compile / Save按用户反馈及保存资产不变 / 原生默认值继承接受，没有替用户在UE逐项重新读取Details。

## 2. 自动化

**19:13:57.118，EmberTrial.M2.Healing.Resource = Success。**测试实际使用临时原生身体、ASC、Health AttributeSet和固定治疗GE，覆盖：

- 出生一次初始化、0次合法、非法4/3隔离、敌人 / 死亡 / 非法治疗量拒绝；治疗配置不撤销公共GameplayReady。
- ASC拒绝GE时不扣数、真实20→60成功消费、Current / Base夹限、满血 / 耗尽拒绝与一次成功一次扣数。
- 同身体Ensure / Possess / Deactivate→Activate保持次数、暂停可查不可改、另一身体独立初值。
- Health与次数两个同步回调递归请求均拒绝；成功后回调失活仍消费，结束撤销快照与解绑。

非法配置用例的Invalid healing settings为已登记预期消息，日志实际按Verbose输出，未造成测试失败。该测试证明资源服务；不证明治疗Montage / Notify、正式Q或次数HUD已经完成。后续B2还需保证同一次动作最多提交一次，不能把本批同步重入保护当作完整动画通知去重。

## 3. 正式地图实机与生命周期

| 日志时刻（本地） | 直接证据 |
| --- | --- |
| 19:14:13.948 / 19:14:17.495 | 正式玩家初始化3/3，查询100/100、Available1 |
| 19:14:32.645 | 满血治疗40：Applied0、100→100、3/3保持 |
| 19:14:46.629 | 既有4造成75血后治疗40：75→100、Applied1、次数2/3，真实Health事件与生命HUD更新 |
| 19:15:08.495 / 19:15:18.373 | P期间查询与治疗请求：75→75、2/3、Paused1；修改Applied0 |
| 19:15:51.489–19:16:04.107 | 恢复P后没有自动治疗；19:16:00又有一次开发伤害到50，随后主动治疗50→90、次数1/3 |
| 19:16:09.441 / 19:16:13.108 | 下一次90→100 / 0/3；随后75血、0/3治疗请求Applied0，资源不越界 |
| 19:16:24.247–249 | 真实玩家死亡，Failure一次排队 / 显示 |
| 19:16:29.518 / 19:16:31.681 | Retry到达Confirmed1，旧观察者清理，新身体生命 / 体力100、治疗3/3与新HUD快照 |
| 19:16:40–19:16:56 | 左键对训练目标真实25、正式2消费25 / 位移150.05cm、E激活M1_Target_A、P继续及Stop清理 |

日志中的50→90是暂停恢复后额外一次4伤害的实际前置值，与指南示例75→100不同，不是自动补治疗或数值错误。具体鼠标 / 键盘、无补播不卡键、移动转向跳跃、单份可见HUD、Stop再开与最终配置恢复按用户全项通过反馈接受；本轮保存日志直接保留一轮PIE及其中Retry，不编造额外PIE启动记录，也不为未独立打印的画面边界追加旧矩阵。

当前日志无玩法Error / Accessed None / Fatal。编辑器图标、在线服务权限与退出HTTP Warning保留在Raw，不作为治疗失败；没有执行额外UE操作来消除这些背景记录。

## 4. 资产、工时与阶段边界

**来源58项资产全部保持M2-03C字节，Config / uproject与冻结B1指南也未变。**新增原生子对象与3/3默认值通过继承生效，用户Compile / Save未产生新的资产差异，不强制重存。来源清单仅更新三个正式身体的验收阶段 / 使用说明，SHA保持；原生GE不是新美术资产，不新增来源行。

本批手动**60 / 等待5分钟**；独立检查未拆报，不重复计时。M2当前**440 / 等待55分钟**，M2-04当前60 / 5；全项目累计**2451分钟（40小时51分钟）** / 已明确等待**240分钟（4小时）**。助手设计 / 审查不并入用户录入时间，不以本批速度压缩整章预算。

A已关闭，下一批为[M2-04路线](../M2_04_HealingRoadmap.md)中的B1治疗动画素材，验收时完整操作指南尚未下发；随后已生成[B1治疗动画指南](../M2_04B1_HealingAnimation.md)，从第2节开始。正式治疗能力 / Q与次数HUD仍待B2 / C，本批不是治疗整章完成。没有新Cook / Insights采集，独立包仍历史M1 C3 Attempt02，不能作为本批新资源的演示证据。

## 5. 本次 Commit / Push（固定放在最后）

初查十份Source已Stage完整实现；核对期间用户完成`685a0f3`提交，包含A完整十份Source与原指南 / 此前收尾，最终HEAD / 缓存origin/main一致、Source工作区无差异。源码提交已完成，不需要重复Stage。本轮新增报告、来源说明与状态收尾仍待提交：CLion刷新Changes → **Stage最新文档差异** → 检查Staged Diff → Commit → Push。助手未联网核验远端 / LFS或操作Git索引。

以下十份完整源码已经纳入685a0f3，供范围复查：

```text
EmberTrial/Source/EmberTrial/Public/Resources/ETHealingResourceComponent.h
EmberTrial/Source/EmberTrial/Private/Resources/ETHealingResourceComponent.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETHealingEffect.h
EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETHealingEffect.cpp
EmberTrial/Source/EmberTrial/Tests/ETHealingResourceTests.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Public/Framework/ETPlayerController.h
EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp
EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp
```

文档包含本报告、A指南 / 治疗路线、Progress / Backlog / M2路线 / Architecture / AcceptanceWorkflow_Options / VersionControl / Introduction和AssetManifest；上章C / 整章报告、旧路线交接等尚未提交的收尾可合并。三个正式身体及其余58资产本轮均无差异，**不强制Stage Content**，不Force Add或为提交制造重存。

A主体已提交，当前文档收尾可用Commit说明：`docs: verify healing resource milestone`。Raw证据 / 候选 / 本地复核脚本、CMake / IDE缓存、Binaries / Intermediate / Saved、日志 / 视频 / 历史包留本地。仍由用户完成Stage / Commit / Push。

当前B1指南 / 数值JSON与新协作约定已下发，可和A文档收尾合并提交；B1新增资产仍待用户制作，不将后续素材写成A已经交付。
