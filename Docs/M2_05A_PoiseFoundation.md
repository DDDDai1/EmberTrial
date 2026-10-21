# M2-05A：韧性属性基础

2026-10-21，固定UE5.5.4。**[本章验收通过](Reports/M2_05A_Verification.md)，无待补项。**用户20:46全项完成，实现60 / 检查10 / 等待5分钟。完整实现归并到当前Source，保留配置 / 构建 / 检查说明供复查，无需重新录入。下一入口[M2-05B1破势状态与行动门控](M2_05B1_StaggerState.md)待手动实现，范围见[路线](M2_05_BreakPulseRoadmap.md)。

## 1. 目标、边界与用时

给共享战斗身体新增独立`UETPoiseAttributeSet`，包含`Poise / MaxPoise`。ASC是实时数值的唯一来源，Character只保存出生配置。正式默认100 / 100；初始化 / Base与Current夹限 / 上限变化 / 非有限值 / 同身体换控不重置先有稳定契约，后续范围破势才有可靠的结算目标。

**本批不会出现新的可按技能或韧性HUD。**普通左键伤害、受击、死亡、闪避与治疗照旧；Poise=0本批是有效资源值，不等于Health=0，也不自动触发破势、死亡或恢复。破势状态、优先级、回填、范围检测、动画生效点与冷却分别在后续批次接入。不临时加蓝图技能代替正式能力，不为A制作新动画 / GE资产 / 测试Pawn。

| 契约 | 本批实现 |
| --- | --- |
| 出生上限 | 有限数且MaxPoise≥1 |
| 出生当前值 | 有限数，0≤Poise≤MaxPoise；非法时全部三组属性拒绝初始化 |
| 运行值 | Base与Current均夹在合法区间；非有限输入回退当前安全值 |
| 上限变化 | 降低时压低越界当前值；提高上限不补韧性 |
| 初始化 | Health / Stamina / Poise全部配置通过后，先各上限后当前值，完成后才Ready |
| 重复Ensure / 换控 | 保留同一身体实时值；不是恢复入口 |
| EndPlay | 撤销Ready，快照返回false并清零输出，拒绝重初始化 |
| 性能 | 新属性无Tick / Timer、无Actor扫描或UI轮询 |

本批实际手动70分钟（代码 / 配置60、检查10）/ 等待5；当前123 Source / 65cpp、Core45 / UI17 / Editor3、来源64。新PoiseFoundation与两项相关旧测试均Success，三身体Valid，正式两轮出生与旧能力 / HUD短基线通过。B1预计128 / 68仅为下一批计划，不混入本章实物计数。

## 2. 准备与精确文件清单

1. Stop PIE → Save All → 关闭UE，CLion保存现有文件。新反射类型正常构建，不用Live Coding。
2. 创建三份新文件，再修改三份既有文件。**Poise属性集属于EmberTrial核心模块，不属于UI或Editor模块。**注意`Attributes`为复数；保留原Health / Stamina文件，不能改名替代它们。
3. 按第3–6节输入代码，第7节Build，之后才打开UE配置 / 校验 / 短查。

| 类型 | 相对于仓库根目录的路径 |
| --- | --- |
| 新 | `EmberTrial/Source/EmberTrial/Public/Attributes/ETPoiseAttributeSet.h` |
| 新 | `EmberTrial/Source/EmberTrial/Private/Attributes/ETPoiseAttributeSet.cpp` |
| 新 | `EmberTrial/Source/EmberTrial/Tests/ETPoiseFoundationTests.cpp` |
| 修改 | `EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h` |
| 修改 | `EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp` |
| 修改 | `EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp` |

现有GAS / Automation依赖已齐全，本章不修改Build.cs / Target / .uproject，不修改UI或输入。不要把测试文件拼成Texts或移到Public；使用既有Tests目录。

## 3. Poise属性集：当前实现

- [ETPoiseAttributeSet.h](../EmberTrial/Source/EmberTrial/Public/Attributes/ETPoiseAttributeSet.h)
- [ETPoiseAttributeSet.cpp](../EmberTrial/Source/EmberTrial/Private/Attributes/ETPoiseAttributeSet.cpp)

Poise / MaxPoise只读接口公开，初始化宏保持private；Base / Current共用夹限。Max≥1，当前在0..Max，非有限输入回退安全值。下降上限通过ASC压低越界Base / Current，上升不补韧性。没有Tick / Timer、动画 / AI或Tag状态副本。

## 4. Character声明与原生归属

[EmberTrialCharacter.h](../EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h)保存一个PoiseAttributes默认子对象引用和InitialPoise / InitialMaxPoise出生配置，并公开GetGameplayPoiseSnapshot；没有另一份运行值。FETPoiseFoundationTest friend仅用于瞬态夹具，不增加生产测试开关。

## 5. 三组属性一次初始化

[EmberTrialCharacter.cpp](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)在构造中一次创建Poise子对象；Ensure先核对同身体ASC / 三个属性集与六个出生值，再按各上限→当前写入，成功标记提交后发布Ready。重复Ensure或换控只读，EndPlay继续先封口 / 撤销Ready，再取消 / 清理。

合法Poise0仍Ready，不等同Health0或已破势；本批没有自动反应。公开快照在不可用时清零输出并返回false。验收时Poise子对象创建位置在Recovery之后，所有子对象仍在BeginPlay前完成，详见报告；不要用旧整份Character覆盖最新代码。

## 6. 项目规则与长期自动化

- [ETGameplayAssetValidator.cpp](../EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp)：正式身体必需PoiseAttributes与ET-BODY-005出生合法性，只读CDO，不纳入故意非法Waiting资产。
- [ETPoiseFoundationTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETPoiseFoundationTests.cpp)：真实ASC / Instant GE、Base / Current、非法Poise不能部分初始化Health / Stamina、非有限值、0、重复Ensure / 换控和EndPlay。两条Invalid startup poise为预期Warning，其他失败不能加Expected隐藏。

重复的六份候选代码在验收后归并为当前Source链接；后续以源码为唯一实现，以下步骤仅供复查。

## 7. 正常双目标Build与CLion索引

### 7.1 Editor与索引

确认六份文件都已保存、UE已关闭，在PowerShell运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后在原CLion工程Reload CMake；Core CodeModel应由43变45，UI17 / Editor3保持，总65cpp。新属性两文件与新测试属于EmberTrialCodeModel，Ctrl+B能到AttributeSet与新头文件。CMake只服务索引，不使用CodeModel目标构建游戏。

### 7.2 Game Development

保持UE关闭，执行：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标通过才打开UE。报错发第一条真实C++ / UHT / 链接错误及文件行号 / 上下文，不只贴UBT exit code 6。generated.h必须是各新头最后一个include；h声明与cpp实现一致，尤其别删旧IsCombatDead。不要把清缓存作为第一步。

## 8. 三正式身体配置与项目校验

### 8.1 逐个检查，均不新增组件

依次打开以下三个既有Blueprint，点击Class Defaults：

| 身体 | 资产路径 |
| --- | --- |
| 玩家 | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` |
| 训练目标 | `/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget` |
| 正式敌人 | `/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy` |

每一个都按相同步骤：

1. Details搜索`Initial Max Poise`，确认为100；再搜索`Initial Poise`，确认为100。它们是出生配置，不是运行时扣除后的数值。
2. 两项默认从原生继承就是100时，无需人为改成别的再改回；旧生命 / 体力100/100、HealingResource 3/3等配置保持。
3. Details搜索`Poise Attributes`应有一项原生继承引用。AttributeSet不是ActorComponent，不要求出现在Components可添加列表；**不要Add Component、不在蓝图NewObject / Add Attribute Set**。
4. Compile → Save。若实际资产无字节差异，后续Push不强求重存；若出现旧绑定 / 类引用错误，先停下发具体错误，别删除继承ASC或旧属性集。

### 8.2 验证三身体

Content Browser逐个选上述身体 → 右键Asset Actions → Validate Assets。预期各有项目`[M1-07C2] Valid`、Issues=0，不只看UE总体“已验证”。新规则正常配置不会报ET-BODY-005。

非法配置与NaN场景由自动化覆盖，本批不另建负例Blueprint或临时Controller节点。旧历史Waiting身体保留原意，不放入正式地图、不将它设成Default Pawn。

## 9. 分组短查：约5–10分钟

### 9.1 三项相关自动化，各一次

非PIE状态，在Output Log逐条执行，等上一项最终完成再输入下一项：

```text
Automation RunTests EmberTrial.M2.Combat.PoiseFoundation
Automation RunTests EmberTrial.M2.Resources.StaminaFoundation
Automation RunTests EmberTrial.M2.Healing.Ability
```

新Poise测试应Success；两个旧项分别检查扩展公共Ready后原体力基础和正式治疗仍正常。现有其他未变战斗测试沿用已验收结果，不重跑完整13项旧矩阵。自动化覆盖不到视觉与键鼠，下面只补必要短基线。

### 9.2 一次出生快照与旧HUD

**准备：**正式地图`L_ET_VerticalSlice`，不改场景 / GameMode，Play。

**操作与预期：**
1. Output Log应有三正式身体各一条`[M2-05A] Poise initialized`、Poise=100 / MaxPoise=100，位于各身体Ready=1之前；不要求你精确抓时刻，我读日志核对。
2. 玩家只显示原生命100、体力100、治疗3/3；本批没有新韧性面板。无Missing gameplay dependencies / Accessed None。
3. 左键真实训练目标25、2一次25体力、4后Q一次治疗和次数3→2；E提示与P一种继续方式正常。移动 / 转向 / 跳跃正常。
4. 这时普通攻击**不会扣Poise**，也没有新范围技能键；这是本批预期，不去复制一套临时技能来检查。

### 9.3 Stop与第二次出生

Stop PIE后再Play，三身体新的Poise100/100，玩家生命 / 体力100与治疗3/3，Stop无结束后重新Ready或旧观察错误。随后Stop，回正式地图，Save All。三身体默认值保持100/100；slomo1、Debug False、原伤害25/10与动画 / 输入 / UI配置保持。

本批不改Poise为0来手工测试破势，因为该状态功能尚未接入。合法0、部分初始化拒绝、换控保留、上限改变与非有限值由本批真实ASC自动化检查，不再人工搭相同矩阵。

## 10. 反馈与解释训练

```text
M2-05A 完成
三新 / 三旧Source；关闭UE后Editor Build / Core45-UI17-Editor3索引 / Game Build：
三正式身体Poise原生引用、Initial100/100；Compile / Save / Valid：
PoiseFoundation / StaminaFoundation / Healing.Ability各一次：
三身体Poise初始化在Ready前；原生命-体力-次数HUD正常：
左键25 / 2 / 4→Q / E-P与移动转向跳跃短基线：
Stop再开新Poise100/100与原资源默认值、无清理错误：
Source / 资产保存、配置已恢复；失败或特别说明（无填无）：
完成时分（如21:18） / 手动代码与配置 / 检查 / 构建等待：
```

能口头解释：为什么Poise=0仍Ready；为什么非法Poise要在Health / Stamina写入前拒绝；为什么提高MaxPoise不能偷偷恢复当前值；为什么普通受击、破势和死亡应分开决定。解释不额外搭验收代码，可在后续复盘继续展开。

A反馈已核对并保存[验收报告](Reports/M2_05A_Verification.md)，当前直接进入[B1手动指南](M2_05B1_StaggerState.md)。无需重交本章反馈或重复已通过矩阵；新普通开发仍由用户手动实现。

## 11. 本批 Commit / Push（固定放在最后）

用户已将C提交至本机HEAD 5a340f5，不重复Stage旧C。A三份新Source当前索引仍为0字节骨架，工作区才有完整实现；必须重新Stage并查看Staged Diff。

本次六份Source：

```text
EmberTrial/Source/EmberTrial/Public/Attributes/ETPoiseAttributeSet.h
EmberTrial/Source/EmberTrial/Private/Attributes/ETPoiseAttributeSet.cpp
EmberTrial/Source/EmberTrial/Tests/ETPoiseFoundationTests.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp
```

本轮文档共13项：

```text
Docs/Reports/M2_05A_Verification.md
Docs/M2_05A_PoiseFoundation.md
Docs/M2_05_BreakPulseRoadmap.md
Docs/M2_05B1_StaggerState.md
Docs/AssetManifest.csv
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Docs/M2_Roadmap.md
README.md
Introduction.md
```

三身体Content / 其他资产 / Config本批字节保持，不强制重存。B1只下发指南，实际B1 Source尚未写入工程。建议本次Commit：

```text
feat: establish poise attributes and initialization contract
```

用户刷新Changes → 重新Stage最新完整修改 → 检查Staged Diff → Commit → Push。Raw / 日志 / 缓存 / 生成CodeModel / Saved / Binaries / Intermediate及包体留本地，不Force Add。B1实施后的独立提交范围见[B1末尾](M2_05B1_StaggerState.md#11-本批-commit--push固定放在最后)。
