# M2-04：有限次数治疗分批路线

2026-10-17，UE5.5.4。**[A已验收](Reports/M2_04A_Verification.md)，[B1治疗动画指南](M2_04B1_HealingAnimation.md)已下发待用户制作。**当前105Source / 56cpp / 来源58，B1计划三素材 / 预计来源61，未创建或烘焙。累计手动2451 / 明确等待240分钟保持，B2正式GA / Q与C次数HUD待后续接入；独立包仍M1 C3 Attempt02。

## 1. 最终行为与资源归属

玩家暂定按Q开始一次原地治疗动作，地面 / 存活 / 非满血 / 有次数 / 动作允许才接受。动作默认约1秒，在明确的动画Notify生效点回复40生命并消费一次；默认3/3次。长按不连续触发，忙碌期间不排队；受击 / 死亡可取消，暂停冻结动作，菜单输入不补播。

生效前取消不回血、不扣次数；生效成功后取消不撤回治疗，也不退款。每次激活最多成功提交一次；重复通知 / 旧实例回调不能重复治疗。开始时满足条件而到生效点已经满血 / 次数耗尽 / 状态失效，生效点拒绝且不消费，动作按清理协议结束。拒绝与已生效两种结果必须在日志中区分，不能在按键时先扣数，再猜动画是否成功。

次数归属当前身体的离散资源组件，生命仍属于ASC的Health AttributeSet。新试炼 / Retry创建新身体恢复默认；Pause与重新控制同一身体保留。**当前会话Continue重建世界，因此也按新身体初值恢复，不保存当前血量 / 次数。**不在这一章偷偷加入持久存档或让检查点自动补次数；后续持久存档再定义加载覆盖与版本规则。

## 2. 分批交付与预算

保持M2原治疗模块 **8h基础预算**，下表是手动实施 / 理解 / 相关检查估计，构建等待另记。复杂度增加时先缩小单批，使用既有全局缓冲，不将估计当实耗。

| 批次 | 交付 | 预计手动时间 | 检查边界 |
| --- | --- | ---: | --- |
| **A：资源与生效事务，已验收** | 原生HealingResource 3/3、固定原生Health GE、成功消费 / 重入保护、事件快照、PIE开发命令、校验与自动化 | 原100–150；实际60 / 等待5分钟 | 新资源测试一次；正式4→命令治疗、Pause、耗尽、Retry短流程5–10分钟；没有治疗动画 / Q或次数HUD |
| **[B1：治疗动画素材](M2_04B1_HealingAnimation.md)，已下发待制作** | 独立Control Rig Sequence / AS / Montage，明确姿势与生效时间；不污染原攻击 / 闪避素材 | 45–75分钟 | 独立预览 / 骨架 / 首尾 / root约5分钟；素材批不重Build或跑旧Gameplay矩阵 |
| B2：正式治疗能力与输入 | InstancedPerActor / ServerOnly GA、Montage Task、单次Notify提交、Healing互斥、受击取消 / 死亡清理、唯一Q Started | 100–160分钟 | 新时序测试覆盖前 / 后取消 / 旧通知 / Pause；实机Q与真实受击5–15分钟，不手动掐生效帧 |
| C：次数HUD与整章收口 | 事件Presenter / Widget、UIBootstrap所有权、晚订阅快照 / 身体切换 / 清理、真实交战与必要集成 | 70–100分钟 | 复用实际UI生命周期 / 新资源命令，新增必要稳定规则测试；不重建临时Controller验收图 |
| 原合计估计 | 分四个可操作批次 | 315–485分钟 | 约5小时15分–8小时5分；实际按用户反馈累计 |

Q目前是拟定正式键位，B2下发前核对实际IMC并只保留一条映射；不在A新增输入。A的开发入口保留为复用工具，不能把它代替正式治疗能力的验收。

## 3. 代码架构与提交时点

- **Character**创建一个继承的HealingResource默认子对象；不在Possess重新创建或重置次数。原GameplayReady仍表示Health / Stamina基础可用，治疗资源另给可用快照；非法治疗配置不能把既有战斗整体锁死。
- **HealingResource**拥有初始 / 上限 / 当前整数、可用性、成功生效服务与变化事件；无Tick / Timer，不知道UI或Montage。A固定原生GE只允许正向Health Add(Base) / SetByCaller.HealthDelta；拒绝不扣数，GE成功后扣一次。同步Health和次数回调期间保护重入；Deactivate / Activate保留次数，EndPlay解绑。
- **GA_Heal（B2）**拥有动作与提交权：检查激活与生效两个时点，当前激活代次 / Montage实例与单次提交标志先封锁重复通知，然后调用资源服务。未成功可按规则结束，不能把同一通知多次重试到成功；旧动作的回调也不能命中新动作。
- **Notify（B2）**只携带动画事件并转交当前合法实例，不直接减次数 / 改血。生效点来自动画数据，不用世界Delay猜播放进度；Pause / 倍速 / 取消均沿实际Montage与世界时间。
- **状态门控（B2）**统一Ability.Heal / State.Healing与既有Attack / Dodge / HitReact / Dead。攻击与闪避不默认取消治疗；受击优先取消治疗，死亡最高优先级。同步检查Character空闲、原Practice、E / 移动 / 跳跃与StaminaRecovery，避免只加Tag却漏掉其他入口。
- **Presenter / Widget（C）**观察当前身体与资源事件，订阅后立刻读快照；不改业务数值。NoPawn / Unsupported / Waiting / Ready有明确显示与解绑，不缓存旧身体强引用，不Tick轮询。HUD变化不能触发再次治疗。

局部事务边界是固定治疗GE成功返回与次数消费；不是对任意外部回调或整个世界的数据库回滚。Instant成功要检查WasSuccessfullyApplied，不能用持续效果句柄IsValid判断。B2还需将“一次动作最多提交一次”与A的“同步回调不可重入”组合，两者责任不同。

## 4. 蓝图、美术与工具

纯C++负责资源、规则、GAS、生命周期 / 日志和稳定自动化；Blueprint只配置GA / Montage / 输入 / UI样式及既有桥接。A三正式身体确认继承组件与3/3 / AutoActivate，不新增节点。固定原生GE不另造同功能蓝图GE或配置资产。

B1沿用SK_Mannequin / Quinn_Simple / Control Rig零成本素材路线，先用上肢收拢 / 短暂停留表达治疗，保留可辨识的生效点；独立新资源，不覆盖原M0 / Combo。无需新增模型、音效或付费资源才能证明逻辑；粒子 / 声音 / 最终动作打磨按后续表现阶段安排。动画数值表在B1完整指南中给出，A不需要提前制作素材。

继续CLion + MSVC / UBT / UHT正常双目标构建、原Editor Validator、Output Log与Automation。只在新负载 / 瓶颈出现时采Insights；本章资源没有常驻更新，不机械重采历史性能窗口。Editor / 开发命令逻辑不进入成品作弊流程。

## 5. 当前操作与验收方式

[A报告](Reports/M2_04A_Verification.md)已通过：五新 / 五旧、双Build、56cpp索引、三正式身体Valid和Healing.Resource Success，实机真实治疗 / Pause / 耗尽 / Retry / 短基线成立。实际105Source / 56cpp（Core40 / UI13 / Editor3），58资产字节保持；原生子对象与默认值继承无需人为制造Content差异。原A操作指南保留供复查，无需再录入或重跑。

当前从[B1第2节](M2_04B1_HealingAnimation.md#2-准备路径与建立空白sequence)制作独立三素材。五帧完整L/R/S / 三FK开关、8–18停留 / 15帧预留、烘焙与Montage配置和5–10分钟短查已写明；本批Notifies空，B2再接原生Notify、正式GA与Q，C接次数HUD。无需Build / 旧自动化或临时播放图；当前没有额外补验。

2026-10-17新增协作约定：每批收到完成反馈后，先核对证据并保存验收报告；**无待补项时同一轮直接生成下一批完整操作指南并更新当前入口**，不再等待用户另发“进入下一章”。有实际缺口时先处理该窄项，不把失败写成通过。该约定不授权助手自动修改Source / UE资产、执行Build或Stage / Commit / Push；手动实践与Git分工保持。

## 6. 本次 Commit / Push（固定放在最后）

当前Stage [B1指南](M2_04B1_HealingAnimation.md)、[数值JSON](Data/M2_04B1_HealPoseValues.json)、治疗路线 / 进度 / 任务板 / 架构 / 验收 / 版本管理 / Introduction交接与自动进入下一批约定；A验收报告 / 来源三个身体说明等未提交收尾可合并。Source主体已在685a0f3，本轮没有新B1源码或资产。B1实物完成后按[指南末尾](M2_04B1_HealingAnimation.md#9-本批-commit--push固定放在最后)提交三新LFS资产、核实后的来源61项与报告 / 状态。Raw / 日志 / 生成文件 / 历史包本地；Stage / Commit / Push由用户完成，助手未联网核验远端。
