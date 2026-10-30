# EmberTrial · 烬火试炼

UE 5.5.4 / Windows / 离线单人的第三人称动作Demo，围绕Gameplay、UMG和对象生命周期逐步开发。当前[M2-06锁定 / 镜头 / 切换已验收](Docs/Reports/M2_06_Verification.md)，[M2-07A新版包集成已通过](Docs/Reports/M2_07A_Verification.md)，[M2-07B性能样本已分析](Docs/Reports/M2_07B_Verification.md)，[M2阶段已收尾](Docs/Reports/M2_Verification.md)，[M3-01遗物定义已验收](Docs/Reports/M3_01_Verification.md)，[M3-02A方案 / 预览已通过](Docs/Reports/M3_02A_Verification.md)，[M3-02B装备效果已通过](Docs/Reports/M3_02B_Verification.md)，[M3-02C1会话库存已通过](Docs/Reports/M3_02C1_Verification.md)，[M3-02C2-01身体装备GE所有权已通过](Docs/Reports/M3_02C2_01_Verification.md)，[M3-02最小装备闭环已通过](Docs/Reports/M3_02C2_02_Verification.md)，[03A列表模型 / 只读预览已通过](Docs/Reports/M3_03A_Verification.md)，当前下发[03B暂停页遗物浏览](Docs/M3_03B_RelicPausePanel.md)，待手动实现，尚未达到最终投递版本。

## 已实现的体验

- 三段轻攻击与输入缓冲、命中窗口 / 扫掠 / 去重、真实GameplayEffect伤害、受击与死亡封锁。
- 消耗体力的方向闪避、短无敌与恢复阶段攻击取消；有限次数的Q治疗在动画通知点生效。
- 近战敌人的感知、导航追击、转向 / 攻击、受击取消与失败重试。
- Poise / MaxPoise及三组一次初始化；归零触发一次GAS破势、五动作取消 / 阻断、1.2秒状态与占位表现、存活结束恢复当前上限。
- F站定施放与真实6游戏秒冷却，.4秒一次300cm / 60Poise范围结算；受击 / 破势 / 死亡取消不退款。
- Tab按需选择 / 解除镜头前方存活敌人，带距离迟滞 / 遮挡宽限与死亡 / 换控清理、Z / C方向切换；已有屏幕菱形标记 / 离屏隐藏再现，锁定时右肩镜头平滑跟随，解除后归还鼠标 / 肩位，换身体与结束恢复。
- 生命 / 体力 / 治疗次数 / 破势冷却HUD，独立主菜单、暂停、失败重试和本次进程内的检查点继续。

已新增两件原生遗物DataAsset、稳定ID / 参数 / 槽位与校验 / AssetManager注册，以及无副作用的两槽方案 / 数值预览；原生装备GE与身体自有零幅度句柄、GI逻辑库存已接入；真实换装 / 版本提交与新身体恢复已接通；初始空槽100 / 25，ET_Relic仅PIE可验证100/120与30，正式页面 / 祭坛仍后续。角色和动画仍使用UE模板及Control Rig占位资源。方向切换已实施并由用户报告检查完成；后续构筑和最终美术尚未完成；磁盘存档、多人及手柄交付不在当前已实现范围内。

## 获取与构建

1. 克隆仓库并安装Git LFS，执行`git lfs pull`，确认`.uasset` / `.umap`已下载为真实文件。
2. 安装UE **5.5.4**及对应MSVC / Windows SDK构建工具。CLion是当前编辑器，实际构建由UBT / UHT执行，CMake仅服务索引。
3. 保存并关闭UE编辑器，在仓库根目录运行下列PowerShell命令。更换机器时将EngineRoot改为实际引擎目录。

```powershell
.\Tools\Validate-ProjectConfig.ps1
.\Tools\Refresh-CLion.ps1 -EngineRoot 'D:\UE5\UE_5.5'
```

Refresh-CLion会正常构建Editor目标并生成三个模块各自的索引信息。随后打开[EmberTrial.uproject](EmberTrial/EmberTrial.uproject)；正式试玩从`/Game/EmberTrial/Maps/L_ET_MainMenu`开始，编辑器开发地图为`L_ET_VerticalSlice`。

Game Development构建、CLion打开 / 刷新、切回VS2022及源码创建说明见[IDE配置 / 切换说明](Docs/CodeEditor_CLion_VS2022.md)与[源码创建 / 索引刷新](Docs/CLion_UEClassCreation.md)。这是源码工程入口，仓库未提供可下载Release。最新本地验证包为`Artifacts/Builds/M2_07A/Attempt01/Windows/EmberTrial.exe`，已包括四能力 / 五HUD、Poise / F、锁定 / 镜头与检查点；两轮独立运行及23项阶段回归见[A报告](Docs/Reports/M2_07A_Verification.md)。完整Windows目录留本地，Cleanup / M1包只代表历史版本。

## 默认操作

| 操作 | 输入 |
| --- | --- |
| 移动 / 镜头 | WASD / 鼠标 |
| 跳跃 | Space |
| 轻攻击 / 连段 | 鼠标左键，按次输入 |
| 闪避 | 顶部数字2 |
| 治疗 | Q |
| 范围破势 | F |
| 目标锁定 / 解除 | Tab（已有标记与镜头跟随；锁定时镜头接管，Tab解除归还鼠标） |
| 范围交互 | E |
| 暂停 / 继续 | P；菜单也支持鼠标与键盘 |

Z / C及出生 / 近景补修已验收，证据与限制见[C报告](Docs/Reports/M2_06C_Verification.md)。首次PIE创建等待需求关闭，不再追加优化或验收。上表保留此前已验收输入。数字1 / 3 / 4及PIE开发命令是现有兼容或诊断入口，作用与保留原因见[架构](Docs/Architecture.md)。Continue / Retry重建世界与身体，保留本次进程的地图 / 检查点及拥有 / 已装备遗物ID，新身体重建装备后按最终上限恢复；普通交互Actor状态仍不跨世界保存，退出进程清空。

## 阅读入口

- [完整计划与JD覆盖](Introduction.md)：最终目标、分期、预算与作品边界。
- [当前进度](Docs/Progress.md) / [任务板](Docs/Backlog.md)：已完成与待完成项。
- [架构与生命周期](Docs/Architecture.md) / [AI黑板契约](Docs/AI_BlackboardContract.md)：职责、状态来源、事件与清理顺序。
- [当前开发：M3-03C遗物搜索 / 筛选 / 排序](Docs/M3_03C_RelicQueryControls.md)：指南已准备，待手动实施；[03B含Tick补修已验收](Docs/Reports/M3_03B_Verification.md)，新进程三轮显示无Ensure。[M3路线](Docs/M3_Roadmap.md)。
- [M3-02A验收](Docs/Reports/M3_02A_Verification.md)：两槽资格 / 汇总、替换 / 卸下 / 预览，双库存专项当次Success。
- [M3-01验收](Docs/Reports/M3_01_Verification.md)：两遗物静态定义、两Valid与Definitions Success；[原操作步骤](Docs/M3_01_RelicDefinitions.md)供复查。
- [M2总验收](Docs/Reports/M2_Verification.md) / [07收尾](Docs/Reports/M2_07_Verification.md) / [技术复盘](Docs/M2_07C_TechnicalReview.md)：已完成行为、实际Bug与证据边界。
- [最近分析：M2-07B性能报告](Docs/Reports/M2_07B_Verification.md)：同包Medium待机 / 真实AI交战；保留PNG尺寸差异与完整区间孤立尖峰，不作严格1080p / Shipping认证。[采集步骤](Docs/M2_07B_PerformanceCapture.md)供复查，当前无需重采。
- [最近验收：M2-07A新版包](Docs/Reports/M2_07A_Verification.md)：23项全组与两轮独立运行通过；[打包步骤](Docs/M2_07A_IntegratedPackage.md)保留复查，当前无需重做。
- [M2-06C方向切换实现](Docs/M2_06C_TargetSwitching.md) / [验收](Docs/Reports/M2_06C_Verification.md)：Z / C与出生 / 近景补修，首次PIE等待需求已关闭。
- [B2右肩镜头完整实现](Docs/M2_06B2_LockCamera.md)与[锁定路线](Docs/M2_06_TargetLockRoadmap.md)：当前可用镜头参数和恢复规则。
- [最近完成：B2右肩镜头](Docs/Reports/M2_06B2_Verification.md)：双Build / 三专项及真实取景、偏移所有权 / 恢复证据。
- [最近验收：B1锁定标记](Docs/Reports/M2_06B1_Verification.md)：纯规则专项、真实重建 / 清理与投影口径。
- [目标基础：A目标筛选 / 生命周期](Docs/Reports/M2_06A_Verification.md)：真实请求 / 死亡清理与自动化证据。
- [最近验收：D冷却HUD](Docs/Reports/M2_05D_Verification.md) / [M2-05整章](Docs/Reports/M2_05_Verification.md)：范围破势与真实敌人交战证据。
- [最近完成指南：M2-05C2-02通知 / 范围](Docs/M2_05C2_02_BreakPulseRange.md)：手动代码、配置、短查与Push清单；[本章验收](Docs/Reports/M2_05C2_02_Verification.md)。
- [代码清理记录](Docs/Cleanup.md)：逐批修改、验证证据、未覆盖范围和本批提交清单。
- [验收方案B](Docs/AcceptanceWorkflow_Options.md)：按影响范围测试、复用入口与阶段验收。
- [资产来源清单](Docs/AssetManifest.csv)：来源、用途及授权记录；复用资产时需分别核对原授权。

核心玩法位于[EmberTrial](EmberTrial/Source/EmberTrial)，UI位于[EmberTrialUI](EmberTrial/Source/EmberTrialUI)，只在编辑器使用的校验器位于[EmberTrialEditor](EmberTrial/Source/EmberTrialEditor)。当前30项自动化；M2-07A的23项阶段全组Success沿用，M3定义 / 规则 / 装备效果 / 会话库存专项已过，03A两相关项当次Success；B扩原Combo.Flow的真实三段伤害快照沿用，覆盖Practice、资源、韧性基础 / 状态 / 耗尽、F施放 / 冷却 / 范围、闪避、连段、治疗、UI纯规则、破势冷却、目标锁定生命周期、标记边界与镜头角度；它们不替代输入焦点、动画观感与独立包验收。
