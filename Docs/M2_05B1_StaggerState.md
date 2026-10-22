# M2-05B1：正式破势状态与行动门控

2026-10-21，固定UE5.5.4。**[B1已验收](Reports/M2_05B1_Verification.md)**，五新 / 七旧已在工程；本章保留当时交付边界和装配 / 检查流程，重复源码已改为实际文件入口。后续修改以[B2新指南](M2_05B2_PoiseReaction.md)为准，不回贴B1旧版覆盖新实现。

## 1. 本批交付、暂留边界与用时

新增正式`GA_ET_Stagger`，通过`State.Stunned`表示默认1.2游戏秒的破势状态。接受后取消当前攻击 / 闪避 / 治疗 / 普通受击，阻止其重新激活，清理原Practice动作、移动 / 跳跃输入和活动AI移动；结束后解除自己拥有的状态与阻断关系。死亡优先取消它，EndPlay撤销自己的Spec和计时器，Pause冻结世界时间。

本批复用`Character.RequestStagger`生产反应入口在PIE控制台检查，不新增按键。**尚不自动观察Poise0、不扣 / 回填Poise、不播放新破势动画，也没有F范围技能或冷却。**强制请求时Poise仍100/100是预期；B2再接真实耗尽 / 恢复与占位动作。当前画面是旧动作被取消、身体暂时停住，镜头和P仍可用，不能将它称为最终破势表现。

GAS负责统一动作取消与阻断：新GA的`Cancel Abilities With Tag`与`Block Abilities With Tag`各包含四种既有Ability标签。这样不需要对四份旧GA再保存同一组Stunned阻断配置；Character公共Idle门控负责移动 / 跳跃 / E / AI，攻击入口额外拒绝连段缓冲，普通受击入口与体力恢复同样检查Stunned。

| 条件 | 本批行为 |
| --- | --- |
| 普通攻击 / 闪避 / 治疗 / 受击正在运行 | Stagger接管并取消它们；不返还已消费体力，治疗生效前取消不扣次数 |
| 已Stunned再次请求 | 拒绝，不重启或延长 |
| Stunned期间受真实生命伤害 | Health仍可下降，普通受击不覆盖破势；生命0走死亡最高优先级 |
| Stunned期间输入移动 / 跳跃 / E / 正式能力 | 拒绝，不排队；镜头与P允许 |
| Pause | 游戏时间与活动Timer冻结；恢复后继续剩余时间 |
| 自然结束 / 取消 | 只撤销本GA的Timer、Tag计数与Block关系；不恢复死亡封锁的MovementMode |
| 同身体失去 / 恢复控制 | 反应归身体，控制变化不重新创建Spec或重置时长 |
| EndPlay / 组件Deactivate | 先撤销句柄归属，再取消 / 移除自己的Spec，禁止旧回调结束新激活 |

原预计100–160分钟，实际实现 / 配置70、检查10、等待5。当前128 Source / 68cpp、Core48 / UI17 / Editor3、来源65；估计与实耗分列，不补算助手设计。

## 2. 准备与精确文件清单

Stop PIE → Save All → 关闭UE，CLion保存。下表记录已经完成的五新 / 七旧文件；不要再按旧批次重复创建。源码学习见3–6节链接，继续开发以B2增量指南为准；不用Live Coding。新GA / 组件属于`EmberTrial`，只有校验器属于`EmberTrialEditor`；不改UI、Build.cs、Target或uproject。

| 类型 | 相对于仓库根目录的路径 |
| --- | --- |
| 新 | `EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Stagger.h` |
| 新 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Stagger.cpp` |
| 新 | `EmberTrial/Source/EmberTrial/Public/Combat/ETCombatPoiseComponent.h` |
| 新 | `EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp` |
| 新 | `EmberTrial/Source/EmberTrial/Tests/ETStaggerAbilityTests.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_HitReact.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp` |
| 旧 | `EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp` |

## 3. 新GA：状态、世界时间与精确清理

实际实现见[ETGA_Stagger.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Stagger.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Stagger.cpp)。GAS拥有Tag与四能力取消 / 阻断，世界Timer和激活代次隔离旧回调；结束先清自己Timer，再由父类撤销计数。不要手工清空所有Stunned计数或在结束后强行Walking。

## 4. 新身体组件：只拥有授予句柄与反应入口

实际实现见[ETCombatPoiseComponent.h](../EmberTrial/Source/EmberTrial/Public/Combat/ETCombatPoiseComponent.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp)。B1按当前身体Ready授予一次能力，Deactivate / EndPlay先撤销身份再Cancel / Clear自己的Spec；本批还不观察Poise。

## 5. 新自动化：真实能力取消、暂停与结束

实际测试见[ETStaggerAbilityTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETStaggerAbilityTests.cpp)，七场景覆盖四动作接管、暂停、Timer / 旧代次 / Tag计数、死亡与EndPlay。最终三项Success与实机边界见[报告](Reports/M2_05B1_Verification.md)。

## 6. 七份既有源码逐处接入

- [Tags.h](../EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp)：原生Ability.Stagger / State.Stunned唯一注册。
- [Character.h](../EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h) / [.cpp](../EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp)：单CombatPoise子对象、请求入口、公共Idle与攻击缓冲门控。
- [HitReact](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_HitReact.cpp)：普通受击不覆盖破势；[StaminaRecovery](../EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp)：共用阻断数组包含Stunned。
- [Validator](../EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp)：GA与身体配置ET-STAGGER-001 / 002，只读不执行能力。

以上是已实现后的学习索引；完整历史代码在Git中，不在文档重复维护第二份运行源码。

## 7. 关闭UE后正常双Build与索引

保存十二份Source，确认UE关闭，PowerShell先运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor成功后在已有CLion工程Reload CMake。预期Core48 / UI17 / Editor3，共68cpp，五个新文件有正确目标；不要新开仅包含两个文件的目录或手改生成CMake。再保持UE关闭运行Game：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

新Native Tag / 类反射使用正常构建；Build失败发首条真正C++ / UHT / 链接错误及文件行号，不只发UBT exit code 6。不要先删Tests或改断言。两个目标通过再打开UE。

## 8. 新GA与三身体装配

### 8.1 新建GA_ET_Stagger

1. Content Browser到`/Game/EmberTrial/Abilities`，空白右键 → Blueprint Class → All Classes，搜索`ETGA_Stagger`原生类，创建`GA_ET_Stagger`。不要复制旧HitReact / Heal GA，也不创建Gameplay Effect代替能力。
2. 打开Class Settings确认Parent Class=ETGA_Stagger。Class Defaults搜索`Stagger Duration`设1.2，暂不配Montage；本类目前没有动画属性。
3. Event Graph保持空，不写Activate Ability、Delay、Add Tag或End Ability蓝图链。计时与清理由C++完成。
4. Cost Gameplay Effect Class / Cooldown Gameplay Effect Class=None。Instancing=Instanced Per Actor，Net Execution=Server Only，Replication=Replicate No，Retrigger=False。原生已给默认，逐项确认。
5. 标签由原生提供，核对下表，不给其他GA另加一组新配置。若UE面板标签名与你看到的显示不同，先用本项目校验确认，不按猜测改旧GA；缺失时发截图。

| 新GA属性 | 精确内容 |
| --- | --- |
| Asset / Ability Tags | Ability.Stagger，仅一项 |
| Activation Owned Tags | State.Stunned，仅一项 |
| Activation Blocked Tags | State.Stunned、State.Dead |
| Cancel Abilities With Tag | Ability.Attack.Light、Ability.Dodge、Ability.Heal、Ability.HitReact |
| Block Abilities With Tag | 与Cancel相同四项，注意不要误填State标签 |

Compile → Save → Content Browser右键Asset Actions → Validate Assets，预期项目[M1-07C2] Valid / Issues0。新资源路径`/Game/EmberTrial/Abilities/GA_ET_Stagger`。

### 8.2 逐个配置三正式身体

依次打开玩家`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`、训练目标`/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget`、敌人`/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy`。

每个身体按相同步骤：

1. Components选原生继承的`CombatPoise`，应只有一份；不要Add Component。若未出现，先确认关闭UE后的Editor正常Build，以及打开的父类仍为EmberTrialCharacter。
2. Details搜索`Stagger Ability Class`，设`GA_ET_Stagger`；Auto Activate=True。此处是新组件属性，不是旧CombatLife的Hit React Ability Class。
3. 保持CombatLife原HitReact Class、三个属性集 / ASC、Initial Health / Stamina / Poise100/100、玩家Healing3/3、三正式能力Class与旧输入原值。
4. Compile → Save，再逐个Validate Assets。三身体均应项目[M1-07C2] Valid / Issues0，不只看UE总计Validated。

旧攻击 / 闪避 / 治疗GA本批无计划变更，不重做标签列表；GAS阻断由新Stagger统一拥有。NoPawn与故意非法Waiting测试资产不新增 / 不改，不放入地图。

## 9. 三组短查与相关自动化：约8–12分钟

不搭临时Controller图，复用`ke`调用正式反应接口。游戏内控制台命令在PIE视口按`~`打开后输入，Enter，随后关控制台。必须在当前游戏World，不是PowerShell或未Play的编辑器世界。`Called succeeded`只表示函数分派成功，真正接受看`[M2-05B1] Stagger request Accepted=1`。

### 9.1 玩家状态、镜头与P

1. 为方便观察，可先在GA_ET_Stagger Class Defaults把Duration临时改4.0，Compile / Save，再Play；仅这一项临时值，不修改C++。最终必须恢复1.2，第9.4的测试会检查。
2. 从正式地图新Play，先靠近尚未激活的A/B使提示可见，并避开敌人；出生三资源仍正常，Poise100/100。安全位置游戏控制台执行：

```text
ke BP_ThirdPersonCharacter_C RequestStagger
```

3. 立即关控制台，尝试移动 / 跳跃 / 左键 / 2 / Q / E，应拒绝；镜头仍可转动。本批没有新动画，身体暂时停住是预期，不要求最终动作视觉。你不必在4秒里试完全部键，精确门控由自动化覆盖。
4. 同一段中P暂停，菜单正常；继续后剩余状态结束可行动，无补播 / 卡键。若操作慢错过该段，再请求一次即可，不把精确按键时点作为验收任务。
5. 日志应有一次begin / end；反应中的再次请求Accepted0，不增加时长。Poise和生命不因强制反应发生变化，资源不会被本GA消费或回填。

### 9.2 敌人移动与新旧流程

1. 让既有MeleeEnemy_A正常发现玩家并追击。游戏控制台执行：

```text
ke BP_ETMeleeEnemy_C RequestStagger
```

2. 关控制台，敌人应暂时停走，状态结束后重新判断目标 / 继续追击或攻击。无需手掐它挥击到哪一帧；活动攻击 / 闪避位移 / 治疗取消由真实自动化另查。
3. 玩家左键25、4后Q、正式2与原HUD短基线正常。一次Stop再Play检查新身体 / 单HUD和清理；若便于操作，可在临时4秒反应中Stop，但不要求复做全部菜单矩阵。

### 9.3 恢复正式配置并保存

Stop PIE，把GA_ET_Stagger Duration恢复**1.2** → Compile → Save → Validate Assets。新GA与三身体的Class保持正确，Poise / Health / Stamina默认100/100、Healing3/3，slomo1、Debug False不变，回正式地图Save All。不能把4秒临时值Push。

### 9.4 三项自动化，各一次

非PIE的Output Log逐条执行，等上一项最终完成再输入下一项：

```text
Automation RunTests EmberTrial.M2.Combat.StaggerAbility
Automation RunTests EmberTrial.M2.Combo.RecoveryDodge
Automation RunTests EmberTrial.M2.Healing.Ability
```

新测试七组实际检查：时长 / 暂停 / 旧回调 / Tag计数 / 清Timer，攻击、闪避、治疗、受击被取消，死亡及EndPlay优先级；两旧项覆盖公共门控接入后的连段恢复取消与治疗。三项应各Success；不新增模仿getter的测试，不重跑未变14项全矩阵。若新测试失败，贴第一条断言与上下文，先定位原因，不放宽时长 / 漂移断言。

本批不证明Poise0会自动触发、范围命中 / 冷却或新破势动画，因为这些尚未接入；也未新跑完整导航 / Unsupported Pawn矩阵，同身体换控以生命周期结构检查为主。记录范围，不为填表造临时验收代码。

## 10. 完成反馈

```text
M2-05B1 完成
五新 / 七旧Source；关闭UE后Editor Build / Core48-UI17-Editor3索引 / Game Build：
新GA父类 / 1.2秒 / Tags与Cancel-Block四项 / 无Cost-Cooldown / 空Graph：
三身体单CombatPoise / Class与AutoActivate / 四资产Compile-Save-Valid：
玩家强制反应 / 门控 / 镜头与P / 结束可行动无补播：
敌人活动追击停止、结束重判恢复 / 原能力与HUD短基线：
StaggerAbility / RecoveryDodge / Healing.Ability各一次：
Stop再开与清理 / 正式Duration1.2与原默认值恢复 / Save All：
失败或最明显表现问题（无填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

日志文字优先；Blueprint装配问题截图Components / Details / 父类，输入与追击问题写操作顺序和结果。无需逐条手抄日志或精确记录测试时点，我读取日志核对。无待补项时保存B1报告并下发B2，继续由你手动C++与UE配置。

## 11. 本批 Commit / Push（固定放在最后）

M2-05B2工作区已验收；用户新提交HEAD`98be3e0`只新增三份0字节文件，完整B2仍未入库。将十二Source、GA_ET_Stagger及待提交B1 / B2报告、C1指南 / JSON、来源65项与状态文档重新Stage并补交，精确清单见[B2末尾](M2_05B2_PoiseReaction.md#12-本批-commit--push固定放在最后)，建议`feat: complete poise reaction and recovery`。无需回滚已有提交；C1三UE素材尚未创建。Raw / 日志 / 生成文件 / 包体留本地，Git由用户操作，助手未核验远端。
