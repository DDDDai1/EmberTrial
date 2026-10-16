# M2-03A：第2、3段动画与Montage验收

2026-10-15，UE5.5.4。用户约22:37全项完成、无问题，手动50 / 烘焙或加载等待10分钟。**A素材批验收通过，无待补项。**本次只读文件 / 哈希 / 日志与名称引用线索，结合用户预览反馈收尾；没有打开UE读取完整资产属性、重做Rig求值、修改Source / 资产、执行Build / 测试或操作Git索引。

## 1. 新实物与变更范围

六份新资产均在`/Game/EmberTrial/Animations/Combo`，最终目录只有这六份uasset，没有残留在Clips的Montage或额外Redirector文件：

| 制作源 | 烘焙动画 | Montage |
| --- | --- | --- |
| Sequence/LS_ET_Light_02 | Clips/AS_ET_Light_02 | Montages/AM_ET_Light_02 |
| Sequence/LS_ET_Light_03 | Clips/AS_ET_Light_03 | Montages/AM_ET_Light_03 |

日志有六路径的保存 / 移动记录：两LS最后22:26:45，两AS约22:27:08 / 22:28:12，两AM最后22:36:04 / 22:36:18。文件名 / 最终路径符合指南，文件大小和SHA已记录。二进制名称扫描可看到对应AS路径、SK_Mannequin、DefaultSlot与ETANS_AttackWindow；这只支持名称 / 引用线索，不冒充完整属性、实际通知数目 / 时长或曲线读取。

当前 **96Source / 50cpp / 来源清单57项**。96份Source、51份旧资产、Config / uproject / 冻结B1指南与C验收哈希完全一致，未覆盖旧M0 / GA / 输入 / AnimBP / Rig；地图没有Git差异。来源清单新增六项，旧51行内容与哈希保持，不新增外部资源或虚构授权。

## 2. 预览与短基线

用户已确认两套五帧、FK开关False、Linear / 首尾、AS独立播放、同Skeleton / 采样设置、Quinn_Simple预览与root / 双脚固定；两AM对应各自AS，DefaultSlot / Loop1 / Blend .10 / .10 / 自动退出，各一条命中状态，第2段8帧 / 第3段6帧开始、均持续6帧。反馈未逐项填实际秒数 / 采样点，不据此捏造精确数值；这些具体数值未由助手在UE重新读取。

22:36:44–22:36:52正式地图PIE有正确GameMode / 原身体、Health与Stamina初始化 / HUD，原AM_M0_Attack_01_Montage正常Begin → 一窗口 → End / Attacking0，随后UI / ActorInfo / 世界清理。证明旧左键仍单段；2 / E / P及移动转向跳跃短查按用户完成反馈接受，日志没有独立对应打印，不要求为此补跑旧矩阵。

编辑器22:26:17出现一条LogRHI GPU profiler scratch buffer不足记录，表示该次GPU分析样本被丢弃；它在PIE前，未发现玩法 / 资产Error或Accessed None，不把它解释为渲染丢帧、资产损坏或性能改善。此阶段未采性能，不修改CVar或要求重录。原记录保留在Raw。

## 3. 工时与交接

A手动 **50分钟** / 等待 **10分钟**，检查独立耗时未拆报。M2当前290 / 等待40分钟；项目累计 **2301分钟（38小时21分钟）** / 已明确等待 **225分钟（3小时45分钟）**。助手设计 / 核对不计入用户手动；不从本批录入速度推算后续代码 / 动画成本，M2-03基础10h暂保持。

下一B接现有LightAttack的三段装配、单槽有限输入缓冲与过期、每段独立Serial / 去重和当前Montage身份；[B完整指南已于10月16日下发](../M2_03B_LightCombo.md)，从第2节手动实施，不重做A或搭临时验收图。A通过仅关闭素材批，正式左键仍单段，本批没有新C++、测试Success、Cook或连段Gameplay验收。

本地`Raw/M2_03A_2026-10-15/Acceptance.json`与AcceptanceExcerpt保留直接证据 / 用户反馈边界；Preparation保留指南下发时的历史状态，不覆盖为当前结果。

## 4. 本次 Commit / Push（固定放在最后）

新`Animations/Combo`目录六资产当前未跟踪，按[A最后清单](../M2_03A_ComboAnimationAssets.md#10-本批-commit--push固定放在最后)Stage六份真实uasset、本报告、来源清单、指南 / 数值JSON / 路线 / 状态文档。旧Content / 地图没有差异，不额外制造提交。

HEAD / 缓存origin/main仍c010520，C三新AM / 八旧Source与B / C收尾文档仍未提交；若一并提交，重新Stage三份新源码的工作区最新版本，包含测试.12 / .04修正，检查Staged Diff。Raw / 日志 / Binaries / Intermediate / Saved / 生成CMake / IDE缓存及旧包留本地。助手未联网核验远端 / LFS或操作索引。

用户Commit / Push，可用`feat: add second and third light attack animation placeholders`；源码C实现如另次提交，按其清单单独处理。本批不把整个M2-03标记完成。

2026-10-16交接更新：只读确认HEAD / 缓存origin/main均2202e74，A六素材、来源记录与旧C实现均已提交；上面的c010520 / 未跟踪描述保留为验收当时状态。本轮仅B新指南与状态 / A交接文档待用户提交，远端未联网核验。
