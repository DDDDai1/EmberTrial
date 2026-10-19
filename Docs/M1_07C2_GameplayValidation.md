# M1-07C2：战斗引用、输入与共享配置校验

更新2026-10-19，UE **5.5.4**。[C2原验收](Reports/M1_07C2_Verification.md)记录14:09、手动20 / 等待5及九正式资产。规则已随M2功能扩展，第21批整理源码与本页；首次创建步骤为历史记录，当前实现以Source为准。功能开发暂停在M2-04B2。

## 1. 成果、时间与范围

新增独立 `UETGameplayAssetValidator`，继续使用 Asset Actions → Validate Assets。它读取设计时 Class Default Object（CDO），不生成角色、不授予能力、不运行 BeginPlay / 初始化生命值。少量反射集中在两个读取函数，用于检查现有类的私有配置，不增加Runtime公开Getter或编辑器依赖。

预计手动 **45–75分钟**：新代码25–45、配置 / 资产5–10、专项检查10–15、记录5；构建 / 索引等待约3–10分钟另列。需要一份可复用GA副本，不搭Controller验收函数。C1实际40 / 等待5已记录，工具总体投入继续按实耗校准。

| 规则 | 本批检查 |
| --- | --- |
| ET-REF-001 / 002 | 所选Blueprint可编译、原生类型符合本批支持范围 |
| ET-BODY-001 / 002 / 003 | 三个正式身体的ASC / AttributeSet / Trace / Life引用；初始生命值合法；受击能力引用，玩家 / 敌人攻击能力与玩家基础输入引用非空 |
| ET-GA-001 / 002 / 003 | 攻击 / 受击Montage非空、有Skeleton / 正时长 / DefaultSlot；ServerOnly / InstancedPerActor；攻击伤害 / 速度与GE引用合法 |
| ET-GE-001 | 伤害GE为Instant、单Health Add(Base) / SetByCaller.HealthDelta、无Execution，符合现有Runtime守卫 |
| ET-AI-005 / 006 | Controller的Stop / Resume / Melee / Retry关系与Runtime守卫一致，Tree与Controller的Blackboard引用一致 |
| ET-INPUT-001 | IMC_Default恰有一条左键映射，指向IA_ET_LightAttack |
| ET-CONFIG-001–004 | 脚本读取共享Default.ini，检查正式默认地图 / Editor地图 / 两Cook条目、文件存在和Cue扫描路径 |

规则限定显式正式资产路径及命名为GA_ETValidation_*的隔离样例；既有Waiting / 非法健康Tests不纳入。当前M2原生检查还覆盖部分Montage / Notify / Tag结构，范围见下表；它仍不证明Started白线、UI组件装配、实际Mesh播放、导航、Runtime订阅或真实加载。共享配置脚本不读取平台 / Saved覆写；C3历史包不包含M2功能。


### 1.1 当前M2扩展与只读边界

| 规则 | 当前检查 |
| --- | --- |
| ET-BODY-004 / ET-STAM-001 | 体力初值有限且范围合法；恢复组件AutoActivate与Rate / Delay / Interval范围 |
| ET-DODGE-001–004 | 成本GE与Montage、位移、短无敌、恢复取消策略；复用Dodge原生配置检查 |
| ET-COMBO-001 / 002 | 连段资源与窗口结构；正式玩家开启、正式敌人关闭连段 |
| ET-HEAL-001 | 玩家治疗资源组件次数与AutoActivate配置 |
| ET-HEAL-002 / 003 | Heal的治疗量 / 动画 / Notify / Tag / 空Cost-Cooldown契约；HitReact取消治疗策略 |
| ET-HEAL-004 | 玩家HealAction类型与路径；IMC恰有一条Q→Heal，Q与Heal不可另绑 |
| ET-SETUP-002 | Object / 浮点字段缺失或类型失配；与某个资源引用为None分开报告 |

这些编号是已实现规则，本轮未新增。HasValidConfiguration、ReadCostAmount及其他能力检查仍调用Runtime原生实现，避免Editor另写一份近似动画 / Tag判定。身体先查公共子对象，再按玩家 / 训练目标身份检查差异；不因校验而初始化身体或资源。

## 2. 文件清单与依赖

当前两文件均已存在，按第3节链接阅读；首次接入时在CLion创建，文件名结尾为 **Validator**。修改反射类前Save All、关闭UE：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\
├─ Public\Validation\ETGameplayAssetValidator.h          新增
├─ Private\Validation\ETGameplayAssetValidator.cpp       新增
└─ EmberTrialEditor.Build.cs                             修改
```

原C1的Project Validator、模块入口、两个Target与uproject保持职责边界；当前规模统一见[Progress](Progress.md)。隔离GA Probe保留合法配置。

[Editor.Build.cs](../EmberTrial/Source/EmberTrialEditor/EmberTrialEditor.Build.cs)集中维护依赖。Core / UI不引用Editor；DataValidation与AssetRegistry为公开父类接口服务，GAS / Tags / EnhancedInput / InputCore只作Editor私有依赖，不用旧列表覆盖当前内容：

[EmberTrialEditor.Build.cs 当前源码](../EmberTrial/Source/EmberTrialEditor/EmberTrialEditor.Build.cs)。实现与配置以该文件为准，本文保留职责和排障说明。

这些私有模块分别提供GAS类型、原生Tag、输入映射结构和EKeys。Editor.Build.cs本轮只读核对，没有修改。

## 3. 当前源码入口与规则分派

### 3.1 ETGameplayAssetValidator.h

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Public\Validation\ETGameplayAssetValidator.h`。

[ETGameplayAssetValidator.h 当前源码](../EmberTrial/Source/EmberTrialEditor/Public/Validation/ETGameplayAssetValidator.h)。实现与配置以该文件为准，本文保留职责和排障说明。

文件名与generated.h前缀一致；generated.h为最后一个include。不要添加同名UFUNCTION，不需要新增Validator蓝图。

### 3.2 ETGameplayAssetValidator.cpp

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp`。

[ETGameplayAssetValidator.cpp 当前源码](../EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp)。实现与配置以该文件为准，本文保留职责和排障说明。

说明：这个校验器用反射读配置、用类型化SDK检查GE / Montage / 输入。某个私有属性改名后会报ET-SETUP-002，不能静默跳过。规则不是通用资产库契约；未来增加新能力或身体时，显式更新所支持路径与规则，避免把练习 / 故意非法样例扫成正式错误。原生C++已有Runtime守卫仍保留，工具不会代替它们。


### 3.3 路径、反射与错误汇总

cpp内部ETGameplayValidationPaths集中六组被重复使用的路径：玩家身体、训练身体、玩家 / 敌人攻击、闪避成本GE、Heal Action。前五项是Package路径，最后一项是带对象名的引用路径；新增资产时需显式更新Profile，不能改成扫描所有子类，也不能混用两种路径格式。

RequireObject先区分字段不存在与引用None；ReadNumber以空Optional保留反射失败，不能当作0继续判断。ValidateBlueprint先查编译状态 / 原生父类 / CDO，再分派身体、能力、Controller或GE；各分支返回边界保持。每资产单独计数，AddIssue逐条登记失败，零问题才登记AssetPasses；不支持类型返回NotValidated。

本轮保持白名单、字段名、原值范围、日志、规则顺序和诊断文本。没有创建新配置系统或删除Runtime守卫，也没有用资源Valid替代运行时动作回归。

## 4. 共享配置与现成脚本

文件：`E:\UE_Project\Demo\EmberTrial\Config\DefaultGame.ini`。搜索有没有以下section；当前C1基线没有。若没有，在文件末尾增加一次；若已存在，在该section下补路径，避免重复section / 同一条目：

```ini
[/Script/GameplayAbilities.AbilitySystemGlobals]
+GameplayCueNotifyPaths=/Game/EmberTrial/Abilities
```

这里扫描已有Abilities目录及其子目录；未来本项目GameplayCue资源放入Abilities/Cues，若放到其他目录则明确扩展路径并同步更新脚本规则。当前没有自定义Cue，本批不新增特效。该配置针对已记录的“未指定路径→扫描整个/Game”Warning，**不声称FPS或加载时间已经改善**；C3两轮实际包日志已核对该回退Warning消失，证据见[C3报告](Reports/M1_07C3_Verification.md)；不延伸为M2包结论。

其余DefaultGame.ini内容和DefaultEngine.ini保持既有值；特别是两正式MapsToCook、默认菜单 / 编辑器正式图不删。保存后PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Validate-ProjectConfig.ps1
```

预期最后一行：`[M1-07C2] Config PASS Errors=0 Warnings=0`。脚本已由助手创建，不需要手抄PowerShell。若在添加Cue配置前执行，预期只有ET-CONFIG-004提示，最后PASS / Warnings=1；**本批完成要求Warnings=0**。

脚本只读项目共享配置 / 文件，不修改INI、不启动UE、不Cook。ET-CONFIG-001对应默认图、002对应Cook条目、003对应umap文件缺失、004对应当前Cue扫描目录。它没有使用UE完整INI层合并或解析umap内容；存在平台 / Saved覆写时以实际Project Settings / C3包为准。助手的脚本负例在Raw隔离副本中测试，用户无需修改正式地图配置制造错误。

## 5. 正常构建与注册

Save All并确认UE关闭，按顺序执行；首个error出现就停下来发文字日志：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor / 索引通过后：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion仍是三个CodeModel，新cpp / h在EmberTrialEditor中。Game这次通常Up to date（没有Runtime C++改动），仍应正常退出。启动UE后原模块ready仍显示M1-07C1，不需要为新校验器修改Module.cpp。第6节出现本批`[M1-07C2]`项目日志，才证明新原生Validator被发现；单纯Editor Build成功不能替代。

## 6. 六个入口的正式配置检查（约3–5分钟）

每个Blueprint先 **Compile → Save**，然后从Content Browser缩略图右键 → **Asset Actions → Validate Assets**。当前正式参数不需要重新设置；按资产原配置直接查。

| 顺序 | Content Browser资源路径 | 预期本项目日志 |
| --- | --- | --- |
| 1 | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` | M1-07C2 Valid / Issues=0 |
| 2 | `/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy` | M1-07C2 Valid / Issues=0 |
| 3 | `/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget` | M1-07C2 Valid / Issues=0 |
| 4 | `/Game/EmberTrial/AI/Controllers/BP_ETMeleeAIController` | M1-07C2 Valid / Issues=0 |
| 5 | 左侧目录树的 `/Game/EmberTrial/Abilities` 文件夹右键 → **Validate Assets in Folder** | 三个GA与Effects中的伤害GE各有M1-07C2 Valid；这是递归目录校验 |
| 6 | `/Game/ThirdPerson/Input/IMC_Default` | M1-07C2 Valid / Issues=0 |

原C2第5项的三GA为GA_ET_LightAttack、GA_ET_EnemyLightAttack、GA_ET_HitReact，GE为Effects/GE_ET_LightAttackDamage；原九资产验收保留在历史报告。当前目录还包含GA_ET_Dodge、GA_ET_Heal与Effects/GE_ET_DodgeCost，正式范围共12项；加合法GA Probe，Gameplay校验共13项。Blueprint类型覆盖其中12项，InputMappingContext类型覆盖IMC_Default；检查本项目Valid标记，不能只看UE总计“完成”。

如果报错只处理对应规则：ET-BODY参考Class Defaults的引用 / 初始数值，ET-GA参考能力的Montage / 伤害 / 策略，ET-GE参考伤害Modifier，ET-AI参考Controller距离 / Tree / BB，ET-INPUT参考左键行。若ET-BODY-001是HealthAttributes None，先发该资产与规则日志，沿用此前从原生类新建的排障经验，不自动调用运行初始化掩盖CDO问题。

## 7. 隔离GA的缺失引用→恢复（约3–4分钟）

1. 优先使用已存在的合法GA_ETValidation_AttackProbe；以下为原C2首次创建步骤，不重复生成。首次从正式 **GA_ET_LightAttack** 缩略图右键 → Duplicate，命名 **GA_ETValidation_AttackProbe**。将副本拖到已有 `/Game/EmberTrial/Tests/Validation` 文件夹，弹窗选Move Here；路径为 `/Game/EmberTrial/Tests/Validation/GA_ETValidation_AttackProbe`。原Abilities目录若留下该副本Redirector，对该目录执行Fix Up Redirectors in Folder。正式角色的能力Class保持原值。
2. 打开副本，进入 **Class Defaults**；Details搜索 `Attack Montage`，先记住当前值，清空为 **None**。只改这一项，Compile → Save。
3. 回副本缩略图，Validate Assets。预期Invalid且Message Log含 **ET-GA-001**，指向这个隔离副本并提示AttackMontage为None。历史C2的单段Probe仅一条问题；若由当前开启连段的GA新建副本，清空首段Montage还会触发ET-COMBO-001，不再以Issues=1作为统一预期。原生编译通过并不代表资源引用有效，这就是本负例的用途。
4. 回副本Class Defaults，将Attack Montage恢复为刚才原值，Compile → Save → Validate Assets，应为 `[M1-07C2] Valid ... Issues=0`。副本保留**合法配置**，不授予正式角色、不放地图。

历史C2只实测ET-GA-001负例；当前未为每条规则建立独立错误样例。BT Probe也保留合法状态；普通批次不重复制造负例，仅规则变更 / 相关失败时复用。第21批未重做交互式断引用或断键。

## 8. 复用既有自动化（约1–2分钟）

不运行Play，在 **Output Log底部命令输入框** 输入一条（不用重新创建测试C++文件）：

```text
Automation RunTests EmberTrial.M0.PracticeAction
```

这是已有组，预期 **Lifecycle** 与 **DodgeMovement** 两项各一次 `Test Completed. Result={Success}`；可在Output Log搜完整测试路径核对，DodgeMovement含三种步长，是一项测试。测试创建自己的临时世界并清理，结束后编辑器继续正常。它复用已有动作生命周期 / 位移基线，不证明本批GA真实攻击或新工具所有错误规则；相关Gameplay实现未变，旧实机矩阵不重复。

以上两项是历史C2的最小回归；当前清理按[方案B](AcceptanceWorkflow_Options.md)复用现有13项自动化。GA Probe保持合法，配置脚本应PASS / Warnings=0；不因一次静态清理重复设计临时输入或Cook。最终新版包仍留统一收尾。

## 9. 最简反馈

```text
M1-07C2 完成 / 卡在第__节第__步
两新文件 / Editor私有依赖；Editor Build / 索引 / Game Build：
Cue路径已保存；配置脚本PASS Errors0 / Warnings0：
九个正式资产均有M1-07C2 Valid / Issues0：
GA Probe None→ET-GA-001，恢复→Valid；已保存且未授予正式身体：
已有Lifecycle / DodgeMovement各一次Success：
源码资产Save All，失败或特别说明（无则填无）：
完成时分 / 实际手动用时 / 构建等待：
```

报编译错误发首个error的文字 / 文件行号；配置和资源问题发规则ID / 对象路径，必要时附Class Defaults或Details截图。规则失败后局部处理，不补搭Controller输入测试图。入口说明见[官方Data Validation，UE5.5](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine?application_version=5.5)，具体API签名按本机5.5.4头文件核对；原C2接入由用户手动完成；当前清理由助手按授权修改并运行构建 / 校验，结果见[Cleanup](Cleanup.md)，不计入历史用户手动工时。

## 10. 本批需要 Commit / Push（固定放在文档最后）

第21批清理的四份Validator Source与五份同步Markdown，共9项，逐文件清单见[Cleanup末尾](Cleanup.md#本批-commit--push固定放在最后)。模块入口 / Build.cs / 两Target / uproject与资产未改，不重复提交原C1 / C2实现。CLion刷新Changes → 重新Stage最新差异 → 检查Staged Diff → Commit → Push；由用户操作Git。Raw / 日志 / 生成文件留本地。
