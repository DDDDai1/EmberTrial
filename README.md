# EmberTrial · 烬火试炼

UE 5.5.4 / Windows / 离线单人的第三人称动作Demo，围绕Gameplay、UMG和对象生命周期逐步开发。当前[M2-06A目标锁定已验收](Docs/Reports/M2_06A_Verification.md)，[B1锁定标记指南](Docs/M2_06B1_TargetMarker.md)已下发、待实施，尚未达到最终投递版本。

## 已实现的体验

- 三段轻攻击与输入缓冲、命中窗口 / 扫掠 / 去重、真实GameplayEffect伤害、受击与死亡封锁。
- 消耗体力的方向闪避、短无敌与恢复阶段攻击取消；有限次数的Q治疗在动画通知点生效。
- 近战敌人的感知、导航追击、转向 / 攻击、受击取消与失败重试。
- Poise / MaxPoise及三组一次初始化；归零触发一次GAS破势、五动作取消 / 阻断、1.2秒状态与占位表现、存活结束恢复当前上限。
- F站定施放与真实6游戏秒冷却，.4秒一次300cm / 60Poise范围结算；受击 / 破势 / 死亡取消不退款。
- Tab按需选择 / 解除镜头前方存活敌人，带距离迟滞 / 遮挡宽限与死亡 / 换控清理；当前只有目标身份，标记和自动镜头后续接入。
- 生命 / 体力 / 治疗次数 / 破势冷却HUD，独立主菜单、暂停、失败重试和本次进程内的检查点继续。

角色和动画仍使用UE模板及Control Rig占位资源。锁定标记 / 镜头 / 切换、后续构筑和最终美术尚未完成；磁盘存档、多人及手柄交付不在当前已实现范围内。

## 获取与构建

1. 克隆仓库并安装Git LFS，执行`git lfs pull`，确认`.uasset` / `.umap`已下载为真实文件。
2. 安装UE **5.5.4**及对应MSVC / Windows SDK构建工具。CLion是当前编辑器，实际构建由UBT / UHT执行，CMake仅服务索引。
3. 保存并关闭UE编辑器，在仓库根目录运行下列PowerShell命令。更换机器时将EngineRoot改为实际引擎目录。

```powershell
.\Tools\Validate-ProjectConfig.ps1
.\Tools\Refresh-CLion.ps1 -EngineRoot 'D:\UE5\UE_5.5'
```

Refresh-CLion会正常构建Editor目标并生成三个模块各自的索引信息。随后打开[EmberTrial.uproject](EmberTrial/EmberTrial.uproject)；正式试玩从`/Game/EmberTrial/Maps/L_ET_MainMenu`开始，编辑器开发地图为`L_ET_VerticalSlice`。

Game Development构建、CLion打开 / 刷新、切回VS2022及源码创建说明见[IDE配置 / 切换说明](Docs/CodeEditor_CLion_VS2022.md)与[源码创建 / 索引刷新](Docs/CLion_UEClassCreation.md)。这是源码工程入口，仓库未提供可下载Release。本地已生成当前M2验证包，入口为`Artifacts/Builds/Cleanup/Attempt01/Windows/EmberTrial.exe`；通过主菜单、治疗 / 闪避正式接口、AI致死、A点Retry / Continue及新进程清空检查，各批输入方法与用户真实键鼠确认见[Cleanup](Docs/Cleanup.md#cleanup-closure)；该包不含后来完成的次数HUD、Poise、F、冷却HUD与锁定。完整Windows目录留本地，旧M1包只代表历史版本。

## 默认操作

| 操作 | 输入 |
| --- | --- |
| 移动 / 镜头 | WASD / 鼠标 |
| 跳跃 | Space |
| 轻攻击 / 连段 | 鼠标左键，按次输入 |
| 闪避 | 顶部数字2 |
| 治疗 | Q |
| 范围破势 | F |
| 目标锁定 / 解除 | Tab（目前无自动镜头或标记） |
| 范围交互 | E |
| 暂停 / 继续 | P；菜单也支持鼠标与键盘 |

数字1 / 3 / 4及PIE开发命令是现有兼容或诊断入口，作用与保留原因见[架构](Docs/Architecture.md)。Continue / Retry会重建世界与身体，只保留本次进程内的地图和检查点ID；普通交互Actor的激活状态不会跨世界保存。

## 阅读入口

- [完整计划与JD覆盖](Introduction.md)：最终目标、分期、预算与作品边界。
- [当前进度](Docs/Progress.md) / [任务板](Docs/Backlog.md)：已完成与待完成项。
- [架构与生命周期](Docs/Architecture.md) / [AI黑板契约](Docs/AI_BlackboardContract.md)：职责、状态来源、事件与清理顺序。
- [当前开发：M2-06B1锁定标记](Docs/M2_06B1_TargetMarker.md)：完整UI代码、详细Designer / 装配、短查与中英Commit清单；[锁定路线](Docs/M2_06_TargetLockRoadmap.md)。
- [最近验收：A目标筛选 / 生命周期](Docs/Reports/M2_06A_Verification.md)：真实请求 / 死亡清理与自动化证据。
- [最近验收：D冷却HUD](Docs/Reports/M2_05D_Verification.md) / [M2-05整章](Docs/Reports/M2_05_Verification.md)：范围破势与真实敌人交战证据。
- [最近完成指南：M2-05C2-02通知 / 范围](Docs/M2_05C2_02_BreakPulseRange.md)：手动代码、配置、短查与Push清单；[本章验收](Docs/Reports/M2_05C2_02_Verification.md)。
- [代码清理记录](Docs/Cleanup.md)：逐批修改、验证证据、未覆盖范围和本批提交清单。
- [验收方案B](Docs/AcceptanceWorkflow_Options.md)：按影响范围测试、复用入口与阶段验收。
- [资产来源清单](Docs/AssetManifest.csv)：来源、用途及授权记录；复用资产时需分别核对原授权。

核心玩法位于[EmberTrial](EmberTrial/Source/EmberTrial)，UI位于[EmberTrialUI](EmberTrial/Source/EmberTrialUI)，只在编辑器使用的校验器位于[EmberTrialEditor](EmberTrial/Source/EmberTrialEditor)。已有20项自动化覆盖Practice、资源、韧性基础 / 状态 / 耗尽、F施放 / 冷却 / 范围、闪避、连段、治疗、UI纯规则、破势冷却与目标锁定生命周期；它们不替代输入焦点、动画观感与独立包验收。
