# M2-03C：恢复阶段闪避取消验收

2026-10-16，UE5.5.4。用户14:15全项完成、无手感问题，手动40 / 构建等待5分钟。**本批验收通过，无待补项；M2-03三段轻攻击整章同时关闭。**助手只读当前源码、构建 / 索引、资产哈希与日志，没有代改Source / 资产、执行UE Build / 测试或操作Git索引。

## 1. 实现与构建

一新 / 七旧Source与[C指南](../M2_03C_RecoveryDodgeCancel.md)当前候选一致，LightAttack.cpp与新测试仅排版差异；其余92份旧Source保持B哈希。实际**100Source / 53cpp**，三CodeModel为37 + 13 + 3。Editor receipt 14:04:25 / Game receipt 14:06:08及相关新产物均覆盖最终源码，最新UBT记录Editor up-to-date；正常双目标Build与CLion导航结合用户反馈。

LightAttack只读查询当前玩家 / Montage实例与命中结束到自动淡出前的位置；包括第3段，不依赖HasNextStage。Dodge保留完整预检后，才由GAS PreActivate取消Ability.Attack.Light；非法 / 体力不足不先取消原攻击，旧输入 / 待切段Timer / Trace清理。原成本、RMS位移与.15保护实现保留，Dodging / HitReact / Dead阻断未删除；不手动改TagCount或新增蓝图状态链。

## 2. 两处编译补修已关闭

原指南FrozenTime写成float，而本机5.5.4 GetTimeSeconds返回double，造成TestEqual C2666。文档 / 候选改为double，用户手动修正；局部替换后漏掉bDodgePaused与Advance(.50f)，随后C2065已通过补回原声明 / 实际暂停推进修复。最终测试全文与更正候选忽略空白一致，双构建及三Success证明补项已关闭。

两次历史失败与修正保留在Raw；没有删除断言、缩短暂停检查或修改玩法参数。用户40分钟按整批登记，不重复追加未单独报告的排障时间。

## 3. 三项自动化与实机证据

| 本轮时刻（本地） | 结果与边界 |
| --- | --- |
| 14:09:01.866 | Combo.Flow Success，原三段 / 单槽缓冲 / 过期 / 旧回调 / 清理保持 |
| 14:09:02.384 | Combo.RecoveryDodge Success，三段恢复交接、预检拒绝保留原攻击、待切段清除、旧Task迟到回调、Pause / 受击 / 死亡 / EndPlay |
| 14:09:25.623 | Dodge.Invulnerability Success，真实命中保护、窗口后伤害、独立保护所有权等旧契约保持 |

14:14:14.642–643第一段、14:14:18.025–026第二段、14:14:21.025第三段都有原Attack Cancelled1 / AttackingTag0后Dodge Begin，每次成本25；自然位移约150.05cm / SourceTime .3001。更早有前摇2拒绝、真实目标每次25、活动Dodge的Pause / Resume、受击后能力 / 保护清理。14:15:09真实死亡，14:15:10.986 Retry到达确认，新身体Health / Stamina100并新HUD，14:15:13世界 / 观察者清理。

三段自然输入、无补播不卡键、敌人普通10 / HUD与最终配置结合用户全项反馈接受，不为本轮未独立打印的每个键追加旧矩阵。当前日志无玩法Error / Accessed None；编辑器图标Warning单独保留，不当作玩法故障。夹具会移动目标靠近手部隔离几何，不能替代自然站位、AI或硬件性能证明；这些边界沿用B。

## 4. 资产与工时

**58项来源资产、Config / uproject与冻结B1指南均保持B字节。**GA_Dodge无需为了新原生默认值人为制造资产差异；RecoveryDodge实际加载保存GA并检查新标签策略成功。Class Defaults逐项设置、Compile / Save及本项目Valid按用户反馈，本轮日志未独立找到项目Valid条目，不捏造时刻或要求重做校验。清单只更新GA_Dodge阶段说明 / 用途，SHA不变，无新动画 / Notify / GE蓝图 / UI / 地图或Build依赖。

C手动**40 / 等待5分钟**；M2-03合计**140 / 等待20分钟**，M2当前380 / 50分钟。项目累计**2391分钟（39小时51分钟）** / 已明确等待**235分钟（3小时55分钟）**。独立检查未拆报，助手设计 / 排障不计用户手动，不以录入速度缩减后续预算。

[M2-03整章验收](M2_03_Verification.md)汇总A / B / C，无新补验。下一开发项按M2路线为有限次数治疗，代码指南待后续下发；当前不创建空类或新包。独立可执行仍历史M1 C3 Attempt02，不能用旧包展示本章新能力。

## 5. 本次 Commit / Push（固定放在最后）

初查时新测试状态AM、暂存内容未包含完整实现；核对期间用户已提交`efaae3e`，新测试全文已纳入且与工作区一致，包含FrozenTime double与bDodgePaused / Advance(.50f)。最终HEAD / 缓存origin/main均efaae3e，Source工作区无差异，本轮未联网核验远端 / LFS；该暂存问题已关闭，不需要重复提交源码。

以下为本批已纳入提交的源码路径，供范围复查（当前无需重复Stage）：

```text
EmberTrial/Source/EmberTrial/Tests/ETLightComboCancelTests.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Dodge.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Dodge.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Public/Combat/ETCombatLifeComponent.h
EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp
```

当前只需Stage本报告 / 整章报告、来源58项的最新阶段说明与Progress / Backlog / 路线 / Architecture / VersionControl / 验收流程 / Introduction等本轮收尾；此前C指南与B报告已随用户提交纳入。Content本轮58项均无差异，不强制重存GA / 地图；若随后有合法新差异，再按实际Changes判断。

CLion刷新Changes → **Stage最新验收报告与文档** → 检查Staged Diff → Commit → Push。可用`docs: close light combo recovery cancel milestone`。Raw / 日志 / 生成CMake / IDE缓存 / Binaries / Intermediate / Saved及历史包留本地，不Force Add；仍由用户操作Git。
