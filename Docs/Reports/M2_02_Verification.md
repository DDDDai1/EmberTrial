# M2-02：正式方向闪避整章验收

2026-10-15。**A / B / C全部通过，无待补项。**玩家2键已迁移至正式GAS能力；移动输入决定世界方向，无输入身体前方，方向固定；默认150cm / .30世界秒、一次25体力、提交后.15秒无敌。位移、保护与Montage分别结束，受击 / 死亡 / Pause / EndPlay遵守各自所有权。

## 1. 分批结果

| 批次 | 交付与证据 | 用户手动 / 明确等待 |
| --- | --- | ---: |
| [A](M2_02A_Verification.md) | 真实Cost GE / 幂等提交、原地Montage、Dodging / 恢复门控；两资源 / CostContract / Lifecycle Success，六Valid | 50 / 5分钟 |
| [B](M2_02B_Verification.md) | 方向RMS Task、正式2输入；CostContract / Movement Success、墙坡 / 方向短查 | 30 / 5分钟 |
| [C](M2_02C_Verification.md) | 原生短保护GE与正式伤害提交门控；计时观察补修后Invulnerability Success、实机两次保护 / 普通伤害 / 世界重建 | 30 / 5分钟 |
| 合计 | M2-02完成 | **110 / 15分钟** |

当前96Source / 50cpp / 51项来源清单；相对M2-01的87 / 44，新增9份Source，其中6cpp；两新UE资产为A的GA_ET_Dodge / Cost GE，B更新既有GA / 玩家输入，C无资产字节变化。未新增UI / 常驻轮询或Build依赖。

## 2. 关键设计与真实修正

Ability唯一拥有动作、成本与保护Handle；Task拥有本次RMS，Montage拥有动作结束；GE Timer拥有自然到期。方向读取当前EnhancedPlayerInput值，避免残留LastInput；成本在实际Montage / Source准备后提交一次。窗口应用防同步取消，结束只移除本次效果，独立保护保留。正式玩家 / 敌人共用提交点按目标当前Tag拒绝，Trace候选沿用单次去重，窗口后新的攻击能正常受击。

C首次测试以.16秒要求.15秒GE消失，遇到测试World的Timer Pending / 浮点帧边界而Fail；修正只把观察时点移到.18秒并等量缩短之后等待，保留正式时长及第二击时序，复测Success。原失败与修正记录均保留。该案例适合复盘Timer生命周期和测试容差，不将扩大保护参数或删断言当作修复。

## 3. 边界与下一章

三段轻攻击、有限次数治疗、范围破势、锁定与正式动画打磨仍待后续。3 / 4为诊断旁路，无敌规则不冒充任意GE的全局免疫；固定步长结果不当作硬件性能。历史M1 C3 Attempt02未重Cook，不能用它展示本章的新能力；M2期末按收口安排新包。

M2-01 + 02用户手动累计 **240分钟** / 等待30；项目累计2251 / 明确215。代码录入、学习、检查未逐项拆时，助手设计 / 排障未量化，因此保留M2整体65h及后续三段攻击10h基础预算；不由本批110分钟外推完整研发速度。下一 **M2-03三段轻攻击**先分清段落 / 窗口、有限输入缓冲与过期、推进 / 取消和伤害次数，再分批给手动代码与动画步骤；现在从[A动画素材指南](../M2_03A_ComboAnimationAssets.md)开始，当前无新C++类待建。

## 4. 本次 Commit / Push（固定放在最后）

按[C报告最后清单](M2_02C_Verification.md#5-本次-commit--push固定放在最后)重新Stage三新 / 八旧Source与最新文档，尤其三份AM新文件的工作区最新内容；源码包含计时补修。GA / 地图 / Config无实际差异时不强制提交。HEAD / 缓存origin/main仍c010520，远端本轮未联网核验；由用户Commit / Push，Raw / 产物 / 旧包留本地。
