# M1-03B3 收尾：修正 Waiting 样例并局部复验

日期：2026-10-05。**用户约22:17、手动10分钟完成，局部验收通过，Waiting补项关闭；本指南保留作过程记录，无需重复执行。**证据见[B3验收](Reports/M1_03B3_Verification.md)。采用已选[方案B](AcceptanceWorkflow_Options.md)，只处理Waiting样例失效，其他已通过组沿用。原预计手动10–20分钟，没有C++ / Build / CLion索引要求。

## 1. 已定位的原因与本次范围

UE5.5.4只读默认对象检查确认：原玩家100/100、合法Probe40/80的HealthAttributes指针有效，Waiting样例100/60的该指针为None；三个对象的ASC和四项输入资源都有值。Waiting默认对象下仍有HealthAttributes子对象，但原生成员引用已丢失，所以组件面板看起来正常不能证明这个指针有效。

这与Epic的[UE-273844](https://issues.unrealengine.com/issue/UE-273844)一致：复制创建了原生AttributeSet默认子对象的Actor蓝图，可能使复制资产的属性集指针失效；官方列出影响5.4 / 5.5、目标修复5.6。原B3第2.3节让我安排Duplicate，应改为直接从原生类新建。保留5.5.4，用资产创建路线避开该问题。

本次只做：新建一个干净Waiting测试蓝图、临时三节点控制该身体、两种配置各Play一次，然后清理并恢复正式地图。无需恢复B3五函数 / 五变量 / 所有按键，不重复扣血 / 旧源销毁 / 十次HUD重建 / 帧率矩阵。之前未就绪Effect拒绝和旧源隔离结果继续沿用；本次重点验证真正的非法配置拒绝与相同新资产的合法初始化。

我已完成的检查是加载已保存资产并读取默认对象，未修改 / 保存UE资产；没有运行新的Gameplay验收。下面仍由你手动执行，结果回报后再关闭B3补项。

## 2. 从原生类创建干净样例

1. 打开UE5.5.4工程，Stop PIE，打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`，先Save All。
2. 在 `/Game/EmberTrial/Tests/Blueprints` 右键 → Blueprint Class → All Classes，选择原生 **EmberTrialCharacter**，命名 **BP_ETHealthWaitingCharacter_Rebuilt**。不要Duplicate旧Waiting或Probe。
3. Class Settings确认Parent Class为原生EmberTrialCharacter。继承的AbilitySystem保留一个，不手动加ASC / AttributeSet / PracticeAction / UIBootstrap。
4. 按下表配置Class Defaults与组件；没有创建新函数、变量或输入事件的要求。

| 位置 | 值 |
| --- | --- |
| Class Defaults → EmberTrial / Startup | Initial Health=100；Initial Max Health=60 |
| Class Defaults → Pawn | Auto Possess Player=Disabled；Auto Possess AI=Disabled |
| Class Defaults → Input → Default Mapping Context | `/Game/ThirdPerson/Input/IMC_Default` |
| Jump / Move / Look Action | `/Game/ThirdPerson/Input/Actions/IA_Jump`、IA_Move、IA_Look |
| CapsuleComponent → Collision | Collision Presets=NoCollision |
| CharacterMovement | Gravity Scale=0；Max Walk Speed=0；Jump Z Velocity=0 |
| CameraBoom | Do Collision Test=False |
| 继承Mesh | Skeletal Mesh / Anim Class=None |

Components → Add Static Mesh，命名HealthProbeMesh，挂Capsule下：Static Mesh=`/Engine/BasicShapes/Cube`；Location与Rotation均0；Scale=(0.35,0.35,0.35)；Collision Presets=NoCollision；Simulate Physics=False。它只帮助识别测试身体，不需要制作美术。

Event Graph不添加BeginPlay / Tick / Delay，不修改原生初始化。旧样例的自定义EndPlay日志不必复制。**Compile → Save**。

旧 `BP_ETHealthWaitingCharacter` 暂时保留，视为失效样例，不拖入地图、不用作默认Pawn、不在本次测试中引用；无需现在删除或重命名它。最终资产清单会把它标为停用，新的Rebuilt资产验收后登记。

## 3. 最小临时入口：地图中的一个身体与三节点

### 3.1 放置测试身体

在L_ET_VerticalSlice拖入新的Rebuilt蓝图实例，Actor Label设为 **ET_B3_WaitingClosure**，Location=(400,0,120)、Rotation=(0,0,0)。确认实例的Auto Possess Player / AI仍Disabled。原来的PlayerStart与地图GameMode Override保持原值。

本轮不改正式GameMode / Default Pawn Class，也不往BP_ETPlayerController增加节点。只在地图Level Blueprint临时切换控制对象。

### 3.2 三个节点与连接

在场景中选中ET_B3_WaitingClosure，然后工具栏Blueprints → Open Level Blueprint，进入Event Graph。

| 节点 | 如何创建 / 设置 |
| --- | --- |
| Event BeginPlay | 复用已有无连接事件；没有时右键搜索Event BeginPlay创建，不创建第二个同名事件 |
| Get Player Controller | 右键搜索；Player Index=0 |
| Possess | 从Get Player Controller.Return Value蓝色引脚拖线，搜索Possess，保证Target类型为Controller |

在Level Blueprint空白处右键，选择 **Create a Reference to ET_B3_WaitingClosure**，得到场景Actor引用。若找不到此项，先回场景重新选中该Actor，再打开Level Blueprint的右键菜单。

**白线：**Event BeginPlay → Possess的执行输入。

**数据线：**Get Player Controller.Return Value → Possess.Target；场景Actor引用输出 → Possess.In Pawn。Target不接Level Blueprint的Self；In Pawn不接原玩家，也不接类引用。Possess执行输出保持不连。

如果已有BeginPlay业务白线：加入Sequence，BeginPlay → Sequence，Then 0接原白线，Then 1接上述Possess。记录原连接，最后只恢复这一个连接；保留原业务节点。不额外创建函数或变量。

Level Blueprint **Compile**。此时保持临时地图改动未保存；如果已保存也可以，最后删除临时内容再正常保存即可。

## 4. 两轮局部复验

### 4.1 非法100/60确实走配置拒绝

**准备：**新样例Class Defaults为100/60，Compile / Save，Level Blueprint Compile；Play使用Selected Viewport，不使用Simulate。

**操作：**Play一次，等初始化结束，看左上HUD和状态面板，查看Output Log。等待数秒即可，不按旧4–9 / J / K测试键。

**画面预期：**当前Pawn为新的Rebuilt实例；生命HUD显示等待就绪，血条 / 数字不作为有效生命展示。Cube / 测试镜头是占位效果；没有这个身体的M0攻击 / 交互 / 暂停业务属于预期。

**必须出现的日志：**

```text
Invalid startup health Character=BP_ETHealthWaitingCharacter_Rebuilt_C_... Health=100.0 MaxHealth=60.0
```

同一对象还有ActorInfo ready以及Presenter `State=WaitingForGameplayReady Pawn=BP_ETHealthWaitingCharacter_Rebuilt_C_...`。允许短暂NoPawn / Unavailable过渡，以初始化结束后的状态为准。

**不应出现：**该新对象的Missing gameplay dependencies、Health initialized或Ready=1；以及新的Blueprint Runtime Error / Accessed None。

这是刻意保留的非法配置Warning，属于此用例的预期。如果仍出现依赖Error、HUD观察的仍是原玩家，或Class Defaults与日志数值不符，Stop后发本节结果 / 相关日志，先停在此处；不靠添加Delay或Ensure反复重试。

### 4.2 同一个新资产的合法40/80可以就绪

**准备：**Stop后只把Rebuilt Class Defaults临时改为Initial Health=40、Initial Max Health=80，Compile / Save；测试实例不要单独覆盖这两项，场景临时Possess连接保留。

**操作：**再Play一次，观察相同蓝图的新实例。

**画面预期：**当前身体为Rebuilt，生命HUD40/80、血条50%，状态面板对应这个Pawn。无需增加扣血 / 治疗或输入测试节点。

**日志预期：**同一对象一次Health initialized=40/80、一次Ready=1；Presenter Ready / HUD Ready最终40/80、Fraction=0.500。该对象不再有Invalid startup health或Missing gameplay dependencies。PIE结束正常解绑 / 清理；具体中间订阅顺序不限。

**清理：**Stop，立即把Rebuilt Class Defaults恢复为100/60，Compile → Save。这是保留的非法配置样例，不把它改为正式玩家。

## 5. 删除临时入口并恢复正式地图

1. Level Blueprint删除本次新增的Possess、Get Player Controller、场景Actor引用；如果为本次新建BeginPlay，也删除它。若加过Sequence，删除该Sequence并恢复之前记录的原白线；不要删除原有业务。
2. Level Blueprint Compile。回场景删除本次放置的ET_B3_WaitingClosure，保留新Rebuilt测试资产。
3. Save All。确认地图GameMode Override与Default Pawn没有被更改，Tests资产不在地图。
4. 在正式地图Play一次短基线：当前Pawn为原BP_ThirdPersonCharacter，出生100/100、生命HUD一份，移动 / 转向及P打开关闭一次正常。Stop后正常保存。

本轮只增加测试资产和临时地图入口，正式Controller / C++ / UI实现没变，因此原1/2/3、闪避打断、E-A/B、完整暂停焦点和旧源隔离沿用已通过基线，不重新列全套矩阵。

## 6. 简短反馈

```text
M1-03B3 收尾完成 / 卡在第__节
新样例从原生EmberTrialCharacter直接创建：是
100/60：Invalid startup health；HUD Waiting；无依赖Error：通过
临时40/80：Health initialized / Ready；HUD40/80与50%：通过
已恢复新样例100/60、删除Level Blueprint临时节点和场景实例、Compile / Save：是
正式原身体100/100、单HUD、移动转向 / P短基线：正常
大致完成时间：今天__:__
实际手动用时：约__分钟
```

日志不用手工整理全份，我按完成时间读取。失败时给上述失败项和对应日志文字；如果控制的仍是原身体，提供临时三节点连线截图，包含Possess.Target / In Pawn及Actor名称。

结果通过后关闭B3补项、更新新样例来源记录并汇总M1-03B结论；本文件只覆盖B3收尾，没有要求开始下一节或全面搭建测试平台。
