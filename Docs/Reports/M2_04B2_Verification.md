# M2-04B2：正式治疗能力、Notify提交与Q输入验收

2026-10-17，UE5.5.4。用户21:07全项完成，无失败或手感问题，手动40 / 构建等待5分钟。**B2验收通过，无待补项。按用户本次要求，收尾后停止，不生成或进入C。**本轮仅只读核对工程与日志、更新文档 / 来源；没有代Build、运行UE / 测试、修改Source / 资产或操作Git索引。

## 1. 源码与构建

五新 / 十一旧共16份Source已落地，实际**110Source / 59cpp**。15份与最终候选的C++ Token一致，其中5份文本相同，其余仅排版；Validator唯一额外差异是移除非必需的`Misc/DataValidation.h`显式include，规则不变，正常Editor Build及七资产校验已通过。其余**94份旧Source**保持B1哈希，A资源事务 / 属性 / UI实现未改。

新测试实际文件为`EmberTrial/Source/EmberTrial/Tests/ETHealAbilityTests.cpp`，原指南计划名为ETHealingAbilityTests.cpp；类仍FETHealingAbilityTest、测试路径仍EmberTrial.M2.Healing.Ability，没有重复测试文件，不需为文件名重命名或重跑。本指南文件清单已同步实际路径。

此前C2280来自指南两处原生Tag花括号列表，助手曾按当次明确授权直接改成显式FGameplayTag数组；实物现已保留该修复，其他用户排版保留。最新Source20:48:49，Editor receipt **20:50:48**、三个模块DLL20:50:47；Game receipt **20:51:48**、exe20:51:47，均覆盖最终源码，版本5.5.4 / Win64 / Development。最新UBT为Editor up-to-date，不冒充另一次全量编译。

CLion三个CodeModel为**Core43 / UI13 / Editor3**，合计59；UI / Editor目标内容未变，旧JSON时间不等于索引缺失。正常双Build与导航结合用户确认接受，本轮没有重新运行构建。

## 2. 自动化与配置

以下四项在当前日志中均实际Success，时刻已由UTC换为本地：

| 本地时刻 | 测试 |
| --- | --- |
| 21:03:34.141 | EmberTrial.M2.Healing.Ability |
| 21:03:34.299 | EmberTrial.M2.Healing.Resource |
| 21:03:50.181 | EmberTrial.M2.Resources.StaminaRecovery |
| 21:04:09.767 | EmberTrial.M2.Combo.RecoveryDodge |

新Ability测试使用保存的GA / Montage / 原生Notify、真实ASC与固定治疗GE推进十三场景：激活拒绝、真实一次提交 / 重复通知、旧实例 / 已结束Task迟到回调、前后受击取消、Pause、耗尽、GE拒绝、前后死亡 / EndPlay、同步GE回调取消、失去控制权与生效前外部治满，以及治疗期间停止体力恢复。直接注入旧Notify / Task仅作负向用例，不替代正向真实通知。资源与恢复 / 连段取消两项相关回归通过，没有追加未受影响的旧矩阵。

**21:01:55–21:03:04**，GA_ET_Heal、玩家BP、IMC_Default、玩家 / 敌人攻击GA、Dodge、HitReact七资产各有本项目Valid / Issues=0。该校验结合测试支持Montage单个DefaultSlot片段 / 唯一原生分支通知、合法时刻、GA规则、输入引用与唯一Q映射；用户另确认15帧 / .5秒、AS无Notify、Amount40 / 无Cost-Cooldown GE、IA Boolean与非暂停触发等具体配置。

本轮未在UE独立遍历全部Details或求值Rig；资产名称扫描只支持ETGA_Heal父类、ETAN_HealCommit与AS / Slot引用线索，不把它当完整反射属性读取。此前Healing分类重复显示不作为重复组件证据：源码单次创建、正式初始化 / 可用与功能均正常；未声称已独立确认分类排版消失。

## 3. 正式地图实机证据与边界

| 本地时刻 | 直接日志证据 |
| --- | --- |
| 21:04:18 / 21:04:23 | 正式玩家新身体100/100、3/3，可用；一条Heal granted |
| 21:04:28 | 满血正式请求Accepted0，没有开始治疗 |
| 21:04:33–36 | 4伤害100→75；随后Q Begin，晚于开始的真实Commit令75→100 / 次数2/3，生命HUD同步；正常End / HealingTag0 |
| 21:05:04 / 21:05:13 | 后续两次独立治疗，各一次Applied1，次数1/3再到0/3；夹限到100 |
| 21:05:23–24 | 玩家真实死亡、Failure一次、Retry Confirmed1，旧观察者 / HUD清理，新身体双100 / 3次 |
| 21:05:38–39 | 治疗正常结束后正式Dodge一次扣25，150.05cm / SourceTime .3001，独立结束 |
| 21:05:54 | 治疗开始后约.2秒受击取消，CommitAttempted0 / HealingTag0，没有提交消费 |
| 21:06:28 / 21:06:47 | 真实敌人各10伤害、受击与HUD同步；21:06:45治疗15→55、一次40 / 一次消费，正常结束 |
| 21:06:30–44 / 21:06:47 | P打开 / 继续闭合，最终Stop撤销生命 / 体力观察和Widget、Session结束 |

实机长按 / 连按无补执行、动作互斥、治疗中P冻结恢复、菜单键不补播、Continue新身体、动作中Stop再开、E与基础输入 / 最终配置恢复按用户全项通过确认；不把普通P样本伪装成活动治疗暂停记录。精确前后取消 / 暂停 / 重复事件由自动化另有断言支持。本轮直接保存一次PIE及其中一次Retry，不编造第二次PIE / Continue样本，也不为缺少独立打印追加旧功能验收图。

当前日志无玩法Error / Accessed None / Fatal；资源非法4/3记录来自预期自动化，非正式玩家错误。编辑器图标缺失、EOS权限 / 退出HTTP与编辑期间Slate等待Warning保留Raw，不当治疗失败或性能结论。

## 4. 资产、工时与停止位置

61项旧来源资产中，实际仅**玩家BP、IMC_Default、AM_ET_Heal_01**三项SHA改变；新增**GA_ET_Heal / IA_ET_Heal**两项，来源清单现**63项**。其余58项旧资产、Config / uproject与冻结B1指南不变；四旧GA继承原生新策略且项目Valid，无差异不要求重存。制作LS / AS、共享Rig / Skeleton / AnimBP与地图保持原样。

本批**40 / 等待5分钟**，独立检查未拆报；M2-04当前**115 / 11**（A60/5、B1为15/1、B2为40/5），M2当前**495 / 61**。累计手动**2506分钟（41小时46分钟）** / 已明确等待**246分钟（4小时6分钟）**；助手设计 / 排障不计入用户手动，预算不按录入速度缩减。

正式治疗动作 / Q已完成；次数Presenter / HUD仍未实现，因此M2-04整章尚未完成。本轮无新Cook / Insights，独立包仍历史M1 C3 Attempt02。用户明确要求本批验收结束后先不进入下一章，本轮只保存报告 / 当前交接，**不生成C指南、不预建新类或资产，等待用户的其他安排**；不将这个单次例外改写成永久取消后续自动衔接约定。

## 5. 本次 Commit / Push（固定放在最后）

初查HEAD / 缓存origin/main为fd94aad，16份暂存Source已包含相同C++ Token；核对期间用户完成**a6c5ab8**提交，包含完整B2实现。最终HEAD / 缓存origin/main一致、Source工作区无差异，五份资产HEAD中的LFS OID与本地SHA匹配。助手未联网核验远端 / LFS服务器或操作索引。当前只需在CLion Stage最新报告 / 来源 / 状态文档差异，不重复Stage无变化的源码 / 资产。

已提交的源码范围见[B2第2节](../M2_04B2_HealingAbility.md#2-准备与准确文件清单)五新 / 十一旧清单，测试使用实际ETHealAbilityTests.cpp；C2280最终两处修复已纳入完整提交。以下五份LFS资产也已提交，供范围复查：

```text
EmberTrial/Content/EmberTrial/Abilities/GA_ET_Heal.uasset
EmberTrial/Content/EmberTrial/Input/Actions/IA_ET_Heal.uasset
EmberTrial/Content/EmberTrial/Animations/Healing/Montages/AM_ET_Heal_01.uasset
EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset
EmberTrial/Content/ThirdPerson/Input/IMC_Default.uasset
```

一起Stage本报告、B2指南 / Progress / Backlog / M2路线 / 治疗路线 / Architecture / AcceptanceWorkflow_Options / VersionControl / Introduction与AssetManifest63项的最新差异。B1报告与其他已明确未提交收尾可合并；四旧GA与制作源无差异不强制加入。Raw / 日志 / 候选脚本、生成索引 / IDE缓存、Binaries / Intermediate / Saved与历史包本地。主体已提交，当前文档收尾建议`docs: verify formal healing ability milestone`；本次没有下一章待提交内容。
