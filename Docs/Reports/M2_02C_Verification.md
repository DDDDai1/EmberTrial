# M2-02C：短无敌与闪避集成验收

2026-10-15，UE **5.5.4**。用户约21:29反馈全部完成、无问题，手动30 / 构建等待5分钟。**C批验收通过，无待补项；M2-02 A / B / C全部收口。**本次只读取代码、产物、索引和现存日志，保存文档 / 来源清单；没有修改Source / 资产、重跑UE Build / 测试或操作Git索引。

## 1. 实现、构建与资产

实际 **96份Source / 50cpp / 51项来源清单**。相对B三新：原生Invulnerability Effect.h / .cpp、ETDodgeInvulnerabilityTests.cpp；八旧：两GameplayTags、Dodge.h / .cpp、LightAttack.cpp、Character.h、CombatLife.h、Editor Validator.cpp。十文件忽略空白匹配原指南；测试仅按补修改两处Advance（.10→.12、后续.06→.04），保留旧断言文本，不影响语义。其余85份B旧Source保持，包括Task、旧成本 / 位移 / 资源 / Practice测试及全部UI。

原生Duration GE用命名TargetTags组件授予State.Invulnerable，成功扣费后通过独立Spec锁定.15秒。Ability保存本次弱ASC / ActiveEffect Handle，防同步取消、按自身Handle清理；正式玩家 / 敌人共享LightAttack伤害提交点读取目标当前Tag。3 / 4仍为诊断旁路，Health夹限 / 治疗不变；窗口、位移与Montage分别结束。

最终Editor receipt / Core DLL为 **21:26:22**，Game receipt **21:25:52** / exe21:25:51，均晚于测试最后保存21:25:07；Editor模块DLL21:12:22晚于其Validator21:10:49。最终Editor4动作 / 8.50秒，Game3动作 / 38.84秒。三CodeModel **34 + 13 + 3 = 50cpp**，含两新cpp；未改Build依赖。

GA_ET_Dodge在 **21:16:58** 有本项目Valid / Issues0。用户确认.15 / 150 / .30 / Cost25 / 原Montage、Compile / Save和最终恢复。**51份资产字节均与B一致**：新增属性继承原生默认值，无需为Push强制制造uasset差异；没有新GE蓝图或新外部美术。来源清单仅更新GA用途 / 来源说明，SHA保持。Config / uproject / 冻结B1指南 / 历史Attempt02包保持。

## 2. 首次失败与补修后证据

- **21:17:22 CostContract Success、21:17:23 Movement Success**，记录在EmberTrial-backup-2026.10.15-13.24.43.log。
- 同轮Invulnerability仅到期观察断言Fail。测试在闪避后.16秒要求Tag消失，忽略测试World在Tick间建立Timer时Pending激活及逐帧到期 / 浮点边界。确认11份Source匹配原指南，是指南测试假设问题。
- 手改观察为 **.18秒**，后续等待减少.02秒，第二击最终观察仍.44秒；正式GE仍.15秒。最终双构建已包含改动。
- **21:27:25 Invulnerability单项Success**，最新EmberTrial.log无Error / Fatal / Accessed None / Ensure或LogET Warning。复用未变的两旧测试成功结果，不要求在新日志重跑三项来凑同一组打印。

新测试真正运行保存的Montage Notify → Sweep → 正式伤害GE / HitReact：首击被保护时Health / Base100、零变化事件、Dodge继续，同一Swing不补伤；第二击在窗口后变90 / 90、触发正式受击并取消Dodge。另验证非法时长不支付、Tag同步取消不漏保护且不退款、独立保护叠加2→取消本次仍1、Pause / Resume、开发致死及活动窗口EndPlay。输出RealSweep汇总；不通过伪造HitResult或手动广播替代正式链路。

## 3. 实机与覆盖边界

最新日志有 **11组**Blueprint玩家Window begin / cleanup、成本Begin / 动作End，均一次扣25、结束Dodging0 / RemainingTag0。正常到期后Cleanup Removed0符合短窗口先于Montage结束；取消时Removed1也有记录。真实敌人普通10点伤害及正式受击成立，**21:28:42.142、21:28:43.808两次真实伤害因Invulnerable拒绝**。

动作中P显示 / 继续后结束、开发致死 / Failure、Retry到达、返回MainMenu、Continue到达与新生命 / 体力Widget、随后Stop重开 / 清理均有日志。HUD视觉、按键手感、E与最终配置由用户反馈补充；没有为未逐项打印的短基线重搭Controller验收图。没有M2新Cook / 性能结论，不将固定步长当硬件帧率，不宣称任意GE全局免疫或子帧精度。

## 4. 工时与交接

C实际手动 **30分钟** / 等待 **5分钟**，独立检查耗时未拆报，不推算比例。M2-02 A / B / C共 **110 / 15分钟**；M2当前240 / 30，项目累计 **2251分钟（37小时31分钟）** / 已明确等待 **215分钟（3小时35分钟）**。未把助手设计 / 排障时间计入用户手动，也不按录入速度线性压缩动画和后续预算。

[整章报告](M2_02_Verification.md)关闭闪避；下一M2-03三段轻攻击按原10h预算分批；[A动画素材指南](../M2_03A_ComboAnimationAssets.md)已下发，先制作独立两段，C++ / 缓冲后续，不预建空类。Raw/M2_02C_2026-10-15/Acceptance.json、InitialTestExcerpt、AcceptanceExcerpt与双构建摘录留本地；原Preparation / TimerBoundaryReview保留历史阶段语义。

## 5. 本次 Commit / Push（固定放在最后）

本地HEAD与缓存origin/main仍 **c010520**，C实现尚未提交；本轮未联网核验远端 / LFS。三新Source当前均为 **AM**（已Stage版本与工作区最新内容不同），八旧有修改；请在CLion **重新Stage三新 / 八旧的全部最新内容**，尤其补修后的测试。AM不是编译问题，不需删除缓存。

一起提交本报告、整章报告、C指南、AssetManifest的GA说明，以及Progress / Backlog / M2路线 / 闪避路线 / Architecture / VersionControl / AcceptanceWorkflow / Introduction和未提交的B收尾。当前Content / Config无差异，不强制保存制造新资产。Raw / 日志 / receipt / CodeModel / Binaries / Intermediate / Saved / 生成IDE文件及旧包留本地。

检查Staged Diff → Commit → Push，可用`feat: add owned dodge invulnerability and combat damage gate`。Git仍由用户操作；无敌 / 方向闪避通过不代表三段连击、治疗与范围破势已完成。
