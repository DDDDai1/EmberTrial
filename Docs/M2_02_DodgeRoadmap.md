# M2-02：正式方向闪避分批路线

更新2026-10-18。[M2-02 A / B / C已全部验收](Reports/M2_02_Verification.md)，本页保留分批设计依据，并维护当前闪避职责。功能基线已到M2-04B2，当前进行[代码与文档清理](Cleanup.md)；实时规模与状态见[Progress](Progress.md)。M2-02历史用户手动110 / 等待15分钟，原12h是设计估算，不外推后续投入。

## 1. 最终行为与暂定默认

玩家地面按正式2键，优先按当前移动输入对应的世界方向闪避，无输入则按角色前方；方向开始固定。默认150cm / 0.30世界秒、一次25体力，C增加提交后默认0.15世界秒无敌并已验收。三种时段独立；真实敌人保护内拒绝、窗口后正常受击，实机与自动化均有证据。

位移完成不等于动作结束；动作仍受Montage生命周期控制，结束回到可移动状态。受击 / 死亡能取消，取消只清自己的Montage / 位移 / Tag / Timer，不残留漂移；暂停不推进世界时间。角色朝向 / 视觉适配复用现有Quinn占位，具体转向与原Montage左右姿势在位移批次确认，不承诺单段侧闪动画已经覆盖所有方向表现。

## 2. 为什么分三批

| 批次 | 交付与边界 | 不提前宣称的部分 |
| --- | --- | --- |
| **A：GAS动作与成本提交** | 原生Dodge Ability / 资产配置 / Character授予与请求；Dodging与恢复 / 受击门控；拒绝不扣、成功一次扣25；原地Montage生命周期与窄自动化 | 本批先用已有ke调用新角色请求，旧2保持原型；没有正式方向位移或无敌，不把原地能力当最终闪避 |
| B：方向位移与正式输入迁移 | 激活时读取当前移动输入并固定方向；复用已验证扫碰 / RootMotion思路，给位移明确唯一所有者；接Enhanced Input Started，2键从旧Practice断开 | 只有新位移可用后才迁移原2，避免同一键双重执行；仍不代表已具无敌 |
| C：短无敌与集成收口 | 权威伤害入口按窗口拒绝；独立窗口所有权 / 取消清理；相关自动化、真实交战 / 世界重建和必要包更新合并安排 | 不扩连击 / 治疗 / 网络预测，不重做旧UI全矩阵 |

A先把资源与生命周期规则落稳，使用长期保留的角色请求入口经ke验证，不增加临时蓝图函数。B之前原2仍按M0原型工作；两种请求以忙碌检查互斥，不能同时占用动画或位移。完成B后同一正式键只保留一个GAS入口，旧原型测试按历史用途保留，不让原型与新GA共同负责同一次闪避。

## 3. 责任与事务边界

- **Ability**拥有本次激活、Montage Task、成本提交和正式Tag；单机权威 / 本地玩家、Ready / 存活、地面 / 忙碌与合法配置检查在真正提交前完成。
- **成本GE**采用独立Instant Stamina Add(Base)固定负值资产，默认-25，本Demo允许1–100点；复用标准CheckCost / CommitAbility / ApplyCost。原生SetByCaller StaminaDelta保留恢复 / 开发注入用途，这是核对标准成本接口后的设计细化。不足体力拒绝整次请求，不能夹到0冒充支付。实际Montage启动后再一次提交，提交前失败不消费，之后取消不退款；A实现保护同步回调和重复提交。
- **恢复**增加Dodging监听，身体正式空闲查询与攻击 / 原型动作门控同步扩展；不能只给新Ability添加Tag而遗漏恢复或旧入口。
- **位移**只拥有自己的RootMotion Source或AbilityTask及对应结束，方向不追随中途输入 / 镜头；原M0距离 / 墙坡经验用于验证，不能用每帧SetActorLocation绕过碰撞。
- **受击与死亡**保留现有优先级；新增Dodge的取消链与当前HitReact / CombatLife集成。位移先结束时不得提前解除整个动作，Montage结束也不得留下Source。
- **无敌**是有明确开始 / 结束与所有者的短窗口；不仅改UI或跳过受击动画。真正伤害提交前检查，诊断4键是否绕过按开发入口语义单独写明，不能拿自伤键当真实敌人攻击窗口证据。
- **UI**继续观察真实资源事件，不为动画自行减25或用Tick猜恢复；当前B2无需改结构。

## 4. 当前实现的生命周期

| 所有者 | 当前实现 | 结束规则 |
| --- | --- | --- |
| Dodge Ability | [ETGA_Dodge.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Dodge.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Dodge.cpp) | 拥有激活、Montage Task、一次成本和本次窗口Handle |
| 移动Task | [ETAbilityTask_DodgeMove.h](../EmberTrial/Source/EmberTrial/Public/Abilities/Tasks/ETAbilityTask_DodgeMove.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/Tasks/ETAbilityTask_DodgeMove.cpp) | 只移除自己的RMS；移动完成后动画可继续 |
| GAS窗口GE | [ETDodgeInvulnerabilityEffect.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETDodgeInvulnerabilityEffect.cpp) | 按游戏时间自然到期；提前取消只清本次Handle |

当前顺序是：预检 -> Montage确实启动 -> RMS准备成功 -> Commit一次扣费 -> 真实扣量核对 -> 应用短无敌。150cm / .30秒位移与.15秒无敌不决定动画何时解锁；扣费后的取消不退款。

ReadyForActivation、成本GE与窗口GE可能同步触发监听并结束能力。ActivationSerial在开始 / 结束递增，各同步调用后核对当前序号与IsActive；这些检查保护不同回调点，不能仅因重复出现就合并。

结束时先确认不是GAS延期结束，再使旧序号失效、解绑Task信号并快照旧窗口Handle；父类结束Task后只移除快照的Handle，不清其他来源Tag，也不覆盖回调中可能建立的新激活。Task的FinishMove先封口，再释放RMS、复制回调并EndTask，最后通知Ability；OnDestroy复用同一释放入口。

范围1–300cm / .05–.50秒是当前安全校验契约，150 / .30 / .15和成本25仍通过既有配置承载。RMS名称与优先级是引擎运动协议，保留字面量并说明用途，不把每个常量都变成蓝图调参项。

## 5. 历史检查与用户手动分工

各批仍由用户手动实现C++ / 蓝图资产、正常双Build和相关5–15分钟检查。复杂时序 / 数值不足 / 重复提交 / 取消后清理由长期自动化兜底，避免手动赶0.15秒；现场只做能稳定观察的新行为。需要当次电脑接管时再根据实际问题明确范围，不将此前授权扩成持续接管。

A修改资源成本路径时复用资源测试并新增必要的提交测试；B触发方向 / 墙坡 / Pause位移专项；C触发真实敌人伤害 / 死亡窗口。没有改变的菜单 / 字体 / UI绑定沿用证据，不重复十次重建。阶段包仍按变更与收口需求合并，旧M1 Attempt02不当作正式闪避证据。

当前A / B / C全部通过，无需重复操作；[C报告](Reports/M2_02C_Verification.md)记录首次计时断言失败、.18观察 / .15正式时长保留与单项复测Success。正式2仅RequestDodge、Task拥有RMS、Montage拥有动作结束、窗口按本次GE Handle清理；旧Practice / 历史测试保留。M2-03连段与M2-04B2治疗已完成；后续功能不在本次清理中开展。当前新增代码按Source查询，不按早期章节重建。

2026-10-18清理第3批复核：四份闪避Source仅整理格式 / 注释，有效Token一致；正常Editor / Game Build与现有11项自动化通过。未重做实机或新包，不把历史窗口实机证据写成此次新验收。具体范围 / 证据见Cleanup。

## 6. 本次 Commit / Push（固定放在最后）

本次为清理第3批，四份闪避Source与同步文档的实际清单见[Cleanup末尾](Cleanup.md)。M2-02阶段的历史交付与提交证据仍见[C报告](Reports/M2_02C_Verification.md#5-本次-commit--push固定放在最后)，不要将历史清单当作当前待提交文件。Content / Config、Raw日志、生成文件和旧包不因文档清理重存或强制加入版本管理。
