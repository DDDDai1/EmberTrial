# M1-03A2-02：Gameplay Effect 与动态生命边界

**2026-10-03已验收通过，手动50 min；A2总任务完成。** 本页保留交接时方案，下文未实施表述属于当时状态。结果见 [A2总验收](Reports/M1_03A2_Verification.md)，当前执行 [B3数据源重绑](M1_03B3_DataSourceRebinding.md)。

生成日期：2026-10-03。前置：[A2-01 已验收](Reports/M1_03A2_01_Verification.md)。**本批由你手动配置蓝图，预计 60–90 分钟。没有新增 C++ / 模块，不要求重复 Editor / Game Build 或刷新 CLion。** 本页是待执行方案，Effect 资产尚未创建，未宣称实测通过。

## 1. 本批目标

用真实 Gameplay Effect（GE）修改 ASC 属性，验证扣血 / 治疗的上下限、降低最大值夹取生命、提高最大值不治疗，以及修改后 Ensure 不重置。快照同时读取 **Current / Base**，避免界面看似正确而基础值仍越界。

数值规则沿用 A2-01 的 C++；蓝图只配置 Effect 与测试入口，不另存 Health、不直接 Set 属性、不补一套夹取规则。本批 Health=0 仍保持 GameplayReady=True，暂不触发死亡 / 禁用移动；死亡在 M1-04。正式 HUD 在 B。

Epic 的 [UE 5.5 Gameplay Effects 文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine?application_version=5.5) 说明 GE 的持续策略与配置职责。这里五个 GE 全用 **Instant**，不会保留在 Active Gameplay Effects 容器；本批不验证持续 Buff、复制 / 预测或复杂伤害计算。

## 2. 准备与资产清单

1. Save All，回 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`。
2. BP_ThirdPersonCharacter → Class Defaults：**Initial Health=100、Initial Max Health=100**，Compile / Save。沿用一个 ASC / 原生属性集，不更换父类，不增添组件。
3. 打开 BP_ETPlayerController，确认旧 4–9 临时事件已清理；本批用顶部数字键 **0、4、5、6、7、8、9**。1/2/3、E/P 保留。
4. Content Browser 在 `/Game/EmberTrial/Tests` 下新建 `Effects` 文件夹。五个测试资产保留到后续回归；临时函数 / 输入验收后删除。

| 新资产（均位于 /Game/EmberTrial/Tests/Effects） | Attribute | Modifier Op | Magnitude |
|---|---|---|---:|
| GE_ET_Test_Damage25 | ETHealthAttributeSet.Health | Add / Additive | -25 |
| GE_ET_Test_Heal40 | ETHealthAttributeSet.Health | Add / Additive | 40 |
| GE_ET_Test_Max60 | ETHealthAttributeSet.MaxHealth | Override | 60 |
| GE_ET_Test_Max120 | ETHealthAttributeSet.MaxHealth | Override | 120 |
| GE_ET_Test_MaxFloor | ETHealthAttributeSet.MaxHealth | Override | 0 |

磁盘目录为 `E:/UE_Project/Demo/EmberTrial/Content/EmberTrial/Tests/Effects/`。不要放入角色 Default Starting Data，不在 BeginPlay 自动应用。

## 3. 创建并配置五个 Effect

以 Damage25 为例，其他资产按表重复：

1. 文件夹空白处右键 → Blueprint Class → All Classes，搜索 **GameplayEffect**，选择原生 GameplayEffect 父类；名称 `GE_ET_Test_Damage25`。
2. 打开资产的 Class Defaults；若为 Data Only 界面，直接使用其属性面板即可，不需要 Event Graph。
3. **Duration Policy=Instant**。
4. Modifiers 点 `+`，只保留 **一个**元素，展开 Index 0。
5. Attribute 下拉选择 **ETHealthAttributeSet.Health**。不要选 MaxHealth 或其他引擎属性。
6. Modifier Op 选择 **Add / Additive**；Modifier Magnitude → Magnitude Calculation Type 选择 **Scalable Float**。
7. 展开 Scalable Float，将其 **Value=-25**；Curve Table 留 None，不选 Attribute Based / Custom Calculation / Set By Caller。
8. Evaluation Channel 保持默认；Source / Target Tags、Executions、Gameplay Cues、额外 GE Components 都不新增。Compile / Save。

其余四个资产依表配置：Heal40 是 Health +40；三个 Max 是 **MaxHealth Override**，值分别 60 / 120 / 0。每份均 Instant、一个 Modifier、常数 Scalable Float。MaxFloor 的 0 是输入给 C++ 边界规则的测试值，最终 MaxHealth 应为 1。

如果找不到本项目属性，先确认父类是 GameplayEffect、A2-01 正常构建且当前 UE 已重启；不要随便选一个替代属性。报错或字段不同可截完整 Class Defaults / Modifier 面板给我。

## 4. 临时函数一：ETTempPrintHealthSnapshot

在 BP_ETPlayerController 新建普通函数 **ETTempPrintHealthSnapshot**，输入 **CaseLabel（String）**，无输出。所有 Print String 勾选 Print to Log；不做 Text Binding / Tick。

### 4.1 执行与对象引脚

~~~text
函数入口
 → Is Valid（Input Object = Get Controlled Pawn，Target=Self）
   无效 → Print String：[M1-03A2-02] No current Pawn → 结束
   有效 → Cast To BP_ThirdPersonCharacter（Object=同一个当前 Pawn）
     Cast Failed → Print String：[M1-03A2-02] Unexpected Pawn type → 结束
     成功 → Branch（Condition=Get Gameplay Health Snapshot.Return Value）
       False → Print String：[M1-03A2-02] Snapshot unavailable → 结束
       True → 基础值有效判断（下节）→ 打印完整快照
~~~

Get Gameplay Health Snapshot 的 **Target 接 Cast 的 As BP Third Person Character**。该纯节点的 Out Health / Out Max Health 就是当前值；不要把 Controller Self 当 Target。

### 4.2 Base 查询与完整打印

添加 **两个 Get Float Attribute Base**（Ability System Blueprint Library 的纯节点，有 Actor / Attribute 输入）：

| 节点 | Actor | Attribute | 用途 |
|---|---|---|---|
| 第一个 | Cast 的角色输出 | ETHealthAttributeSet.Health | Return Value 为 HealthBase |
| 第二个 | 同一个角色输出 | ETHealthAttributeSet.MaxHealth | Return Value 为 MaxHealthBase |

两个节点的 **Successfully Found Attribute** 输出接 Boolean AND；AND 接一个 Branch Condition。上节快照 True 白线接此 Branch。

- False：Print `[M1-03A2-02] Base attribute unavailable`，结束，不能把默认 0 当有效生命。
- True：Print String 的 In String 接 Format Text，格式如下；Text → String 按自动转换连接。

~~~text
[M1-03A2-02] Case={Case} Ready=True Health={H}/{M} Base={HB}/{MB}
~~~

Case 接 CaseLabel；H / M 接 Snapshot 的 Out Health / Out Max Health；HB / MB 接两个 Base 查询的 Return Value。纯节点没有白色执行线，只连接数据引脚。Compile / Save。

本批全部是 Instant 且无其他属性加成，所以正常每份快照 **Health=HealthBase、MaxHealth=MaxHealthBase**。预期数字允许显示成 100 或 100.0，二者相同。

## 5. 临时函数二：ETTempApplyHealthEffect

在同一 Controller 新建普通函数 **ETTempApplyHealthEffect**，输入：

- **EffectClass**：GameplayEffect **Class Reference**（类引用，紫色引脚）；不要选 Object Reference / Soft Reference。
- **CaseLabel**：String。无输出。

### 5.1 门控

~~~text
函数入口 → Branch（Condition=Is Game Paused）
  True → Print：[M1-03A2-02] Rejected paused → 结束
  False → Is Valid（Input Object=Get Controlled Pawn，Target=Self）
    无效 → Print：[M1-03A2-02] No current Pawn → 结束
    有效 → Cast To BP_ThirdPersonCharacter（Object=当前 Pawn）
      失败 → Print：[M1-03A2-02] Unexpected Pawn type → 结束
      成功 → Branch（Condition=Is Gameplay Ready，Target=Cast 的角色）
        False → Print：[M1-03A2-02] Rejected not ready → 结束
        True → Is Valid（Input Object=下方 ASC 查询结果）
          无效 → Print：[M1-03A2-02] Missing ASC → 结束
          有效 → 下方应用节点
~~~

ASC 查询使用 **Get Ability System Component** 纯节点，选有 **Actor** 输入的 Ability System Blueprint Library 版本，Actor 接 Cast 的角色。**不要使用默认 Actor=Self**，Controller 不持有本角色 ASC。

### 5.2 应用节点与引脚

从 ASC 返回值拖线，搜索 **Apply Gameplay Effect to Self**；选择 Target 为 Ability System Component、带 Gameplay Effect Class / Level / Effect Context 的版本。不选 Gameplay Ability 版本，也不选带 Spec Handle 的版本。

| 引脚 | 接法 |
|---|---|
| 白色执行输入 | 上节 ASC Is Valid 的有效出口 |
| Target | ASC 查询返回值 |
| Gameplay Effect Class | 函数输入 EffectClass |
| Level | **1.0** |
| Effect Context | **Make Effect Context** 返回值；该节点 Target 也接同一 ASC |
| 白色执行输出 | 调用 ETTempPrintHealthSnapshot（Target=Self、CaseLabel=本函数输入） |

Make Effect Context 在本机 API 中是 const 查询，通常为纯节点；若编辑器显示执行引脚，则把它放在 ASC 有效出口和 Apply 之间。不能从另一个 Pawn / Controller 创建上下文。

**不使用 Apply 的 Active Gameplay Effect Handle.IsValid 判断成功。** Instant Effect 不留下持续效果句柄；以应用后的 Current / Base 快照和错误日志判定。调用到节点也不自动意味着成功，因此必须按第 7 节核对数值。每个调用点都选定非空 EffectClass，不运行时修改 GE 资产默认值。Compile / Save。

## 6. 临时输入：只在松开时执行

在 Controller Event Graph 添加顶部数字键事件，全部使用 **Released**，Pressed 不接线；Consume Input 保持 True，Execute When Paused **False**。

| 键 | 调用 | EffectClass / CaseLabel |
|---|---|---|
| 4 | ETTempApplyHealthEffect | GE_ET_Test_Damage25 / D25 |
| 5 | 同上 | GE_ET_Test_Heal40 / H40 |
| 6 | 同上 | GE_ET_Test_Max60 / M60 |
| 7 | 同上 | GE_ET_Test_Max120 / M120 |
| 0 | 同上 | GE_ET_Test_MaxFloor / MFloor |
| 8 | ETTempPrintHealthSnapshot | CaseLabel=Read |
| 9 | 下面的重复初始化链 | CaseLabel=Ensure |

9 Released → Is Valid（当前 Pawn）→ Cast To BP_ThirdPersonCharacter → Ensure Gameplay Initialized（Target=Cast 角色）→ ETTempPrintHealthSnapshot（Target=Self、CaseLabel=Ensure）。无效 / Cast Failed 只 Print 并结束；不要用 9 创建对象或重新设置 Initial 数值。Ready / 初始化成功与否以 C++ 日志和快照为准。

两个临时函数只查询当前 Pawn，不创建持久角色引用。不要加 Event Tick、Delay、额外 BeginPlay 初始化。Compile / Save，进入新地图 PIE，先按 8 确认 100/100、Base 同样 100/100。

## 7. 分组验收

### 7.1 一轮连续数值测试

在 **同一轮 PIE** 中按下表顺序。每次松开键后自动打印快照；手动 8 可补读。每行都要求 Current / Base 相同、Ready=True。

| 顺序 | 操作 | 预期 Health / MaxHealth | 验证目的 |
|---:|---|---|---|
| 1 | 出生 → 8 | 100 / 100 | 确认最终配置 |
| 2 | 4 一次 | 75 / 100 | GE 扣血 |
| 3 | 9 三次 | 75 / 100，每次相同 | 扣血后 Ensure 不补血、不再 Ready=1 |
| 4 | 4 三次 | 50 → 25 → 0 / 100 | 到零；本批不死亡 |
| 5 | 4 再一次 | 0 / 100 | 过量扣血，Base 也不得负数 |
| 6 | 5 一次 | 40 / 100 | 从零正常治疗，排除隐藏负 Base |
| 7 | 5 两次 | 80 → 100 / 100 | 过量治疗，Base 也夹取 |
| 8 | 4 一次 | 75 / 100 | 排除隐藏超上限 Base |
| 9 | 6 一次 | 60 / 60 | 最大值下降，生命与其 Base 一同压低 |
| 10 | 7 一次 | 60 / 120 | 最大值增加不自动治疗 / 不恢复旧生命 |
| 11 | 9 三次 | 60 / 120，每次相同 | Health 和 MaxHealth 都不重置到 100 |
| 12 | 5 两次 | 100 → 120 / 120 | 治疗使用新的上限 |
| 13 | 0 一次 | 1 / 1 | Max=0 输入被夹到 1，生命同步降低 |
| 14 | 5 一次 | 1 / 1 | 极小上限的过量治疗 |
| 15 | 7 一次 | 1 / 120 | 上限恢复仍不补血 |
| 16 | 4 → 5 | 0 / 120 → 40 / 120 | 新上限下扣血与治疗，Base 同步 |
| 17 | 9 → 8 | 40 / 120 | 修改后的最终幂等快照 |

从初始 100/100 到 40/120 的这一轮只有出生的一条 Health initialized / Ready=1；EndPlay 有 Ready=0 / ActorInfo cleared，各一次。没有持续 Buff，所以 Current 与 Base 不一致应停止在该行定位，不能在 BP 写 Clamp 掩盖问题。

### 7.2 暂停与重开

在上轮的 40/120 状态按 P 打开菜单：在菜单内分别完整按下 / 松开 4、5、6、7、0、8、9，之后继续游戏。按 8 仍应 **40/120**，没有菜单期间 / 恢复后的补执行；移动 / 转向与暂停输入原规则正常。Stop → 新 Play / 8 应回 **100/100**，新身体重新初始化，不继承上轮修改。

### 7.3 负最大值与未就绪拒绝

1. Stop，打开 GE_ET_Test_MaxFloor，将 Scalable Float Value 临时从 0 改 **-20**，Compile / Save。新 PIE 从100/100按0，应为 **1/1、Base=1/1**；按7为 **1/120**，不治疗。Stop；把该资产值恢复 **0**、Compile / Save。
2. Stop 后角色 Class Defaults 临时设 **Initial Health=100、Initial Max Health=60**，Compile / Save。新 PIE 出现 A2-01 的预期 Warning；按4、5、6、7、0 都只得到 **Rejected not ready**，按8快照不可用，不执行 Effect。
3. Stop，角色恢复 **100/100**、Compile / Save。新 PIE 出生正确，4 / 5 / 6 / 7 再检查一组：75/100 →100/100 →60/60 →60/120；Stop 撤销 / 清理正常。

没有对 NaN / Infinity、持续效果到期、多人或死亡进行测试，不能据此宣称这些已支持。空 Pawn / 非玩法 Pawn 的代码出口已设计；与正式 HUD 一起的真实替换回归在 B / C。

## 8. 清理与原功能回归

Stop，Controller 删除本批 **0、4–9** 键事件、相关临时 Cast / Print 以及 **ETTempApplyHealthEffect / ETTempPrintHealthSnapshot** 两个函数。没有其他临时变量需要保留；既有 UIBootstrap / StatusWidget 不删除。Compile / Save。

五个 GE 保留 Tests/Effects，MaxFloor 的最终值 **0**；角色最终100/100；不把 Effect 加到角色启动配置或地图 Actor。回新地图做一次原1/2/3、2→3、E-A/B、P鼠标 / 键盘继续、恢复无补播 / 卡键、移动 / 转向 / 跳跃回归。Stop / Save All。

若数值和 Blueprint Compile 都通过，本批无需为资产配置重复正常 C++ Build；本批也没有新增 C++ 自动化测试。属性事件 / Presenter / 血条在 B 实施，M1-03 总任务尚未完成。

## 9. 反馈模板

~~~text
M1-03A2-02 完成 / 卡在第 __ 节第 __ 行
五个 GE 路径 / 父类 / Instant / 单 Modifier：正确
Damage / Heal Add，Max Override；Scalable Float 值：正确
两个临时函数 / 数字0、4–9；Blueprint Compile：通过
出生100/100；Current / Base 查询：通过
75/100后重复9三次不重置、不再Ready=1：通过
扣到零 / 过量扣血 / 从零治疗；Base同步：通过
过量治疗后扣血；Base无隐藏越界：通过
最大值下降60/60、上升60/120不治疗：通过
60/120重复9三次，两属性不重置：通过
Max输入0 / -20均夹到1，恢复上限不治疗：通过
修改后暂停 / 菜单无执行无补播 / 重开回100/100：通过
非法100/60时Effect拒绝，恢复后正常：通过
Ready撤销 / ActorInfo清理 / 无错误：通过
临时输入 / 两函数已删除；GE保留、MaxFloor=0：是
角色最终100/100，原1/2/3、2→3、E/P与基础输入：正常
资产已保存：是
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟
~~~

报错先发完整文字、步骤 / 表格行号、操作前后数值；配置问题截 GE 的完整 Modifier，连线问题截函数执行线和数据引脚。普通报错不授权助手改源码；只有你明确要求直接操作时才接管。
