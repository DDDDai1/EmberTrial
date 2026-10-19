# M1-05A：敌人身体、AI控制权与导航基础

2026-10-19清理同步：保留历史接入、资产配置、排障和验收经验；完整类 / 旧完整方法改为当前Source入口，不按旧片段覆盖已完成工程。当前敌人已能感知、追击、转向并真实攻击；90 / 100 / 100厘米与0.5秒是现有配置，150 / 200属于早期阶段。普通功能开发保留手动实践约定，本轮清理按用户授权由助手直接修改；验证和本批提交清单见[Cleanup](Cleanup.md)。

日期：2026-10-07；复核2026-10-08；固定UE **5.5.4**。前置：[M1-04已验收](Reports/M1_04_Verification.md)。**状态：用户已完成，A验收通过。**手动30 / 等待5分钟；47份Source / 23cpp、双receipt / 索引与两轮初始化 / 四次伤害 / 受击死亡 / 结束清理已核对，见[A结果](Reports/M1_05A_Verification.md)。首次UHT命名错误已改正并构建通过；第5.1节保留排障记录，无需重复执行。继续[B1感知与有效目标](M1_05B1_PerceptionTarget.md)。以下为原A操作记录，当时助手未代改Source / 资产或Build / PIE；当前清理按顶部授权与验证说明执行。

## 1. 本批成果、时间与检查范围

新建一个独立敌人身体`BP_ETMeleeEnemy`，自动由原生`ETAIController`控制；身体具有自己的100/100生命，能沿现有战斗链被玩家击中、受击与死亡。地图加入一个导航范围，覆盖固定房间的可走地面。

**本批敌人站立待机，还不会感知 / 追击 / 攻击。**这是预期，B接追击，C接正式反击。攻击能力当前只接受本地玩家Controller，不以蓝图Timer强行绕过；A给敌人的Light Attack Ability Class保持None。

预计手动**40–75分钟**：代码15–25、资产 / 地图20–35、检查5–15；构建等待与排障另列。本批只新建两份C++文件、修改核心Build.cs一处，创建一份敌人BP并保存正式地图。没有新动画、UI、测试按键、复杂Controller验收图。

未改共享Character、攻击、Trace、Life、属性和UI源码；沿用M1-04已接受结果。只查新身体装配与控制权、被击链、导航覆盖、停止重开及一次玩家短基线，不重跑旧矩阵或Lifecycle / DodgeMovement。

## 2. 保存与文件准备

1. UE里Save All，然后**关闭UE编辑器**；CLion保持打开。
2. 使用原`E:\UE_Project\Demo\EmberTrial`工程窗口，不另开只包含两个文件的CLion项目。
3. 在CLion对应核心模块目录创建下面两个文件；不存在AI目录就创建目录。可以手动创建普通.h / .cpp并输入全文，不需要打开UE的New C++ Class向导。

| 新文件 | 完整路径 |
| --- | --- |
| ETAIController.h | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\AI\ETAIController.h` |
| ETAIController.cpp | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETAIController.cpp` |

不要移动 / 改名现有Character或复制玩家C++类。新Controller是正式AI对象的第一批实现；它观察自己受控身体的Ready并管理解绑，不是只为验收搭出的临时控制器。

## 3. 核心Build.cs添加AIModule

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrial.Build.cs`。**只在已有PublicDependencyModuleNames数组末尾增加`"AIModule"`**，原项保留，GameplayTasks / PhysicsCore仍在Private。完成后全文应为：

```csharp
using UnrealBuildTool;

public class EmberTrial : ModuleRules
{
    public EmberTrial(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "GameplayAbilities", "GameplayTags", "AIModule"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "GameplayTasks", "PhysicsCore"
        });

        PrivateIncludePaths.Add(ModuleDirectory);
    }
}
```

公开头文件继承AAIController，所以AIModule在Public。本批没有NavigationSystem C++查询，不必提前增加该依赖，也不用改UI模块、两个Target或.uproject插件列表。

## 4. 两份Controller文件全文

### 4.1 ETAIController.h

当前源码：[ETAIController.h](../EmberTrial/Source/EmberTrial/Public/AI/ETAIController.h)。


### 4.2 ETAIController.cpp

当前源码：[ETAIController.cpp](../EmberTrial/Source/EmberTrial/Private/AI/ETAIController.cpp)。


注意引擎成员原名是**`bStopAILogicOnUnposses`**，末尾只有一个s，按代码输入，不改成Unpossess。`ETAIController.generated.h`保持.h的最后一条include。

这里没有新增Tick / Timer / Move To / Run Behavior Tree，没有复制Health或创建第二个ASC。AIController基类仍有自身的引擎更新，不把“未新增业务Tick”描述成“整个AIController完全不Tick”。弱引用与显式解绑负责身体生命周期；Ready=true快照可在PossessSnapshot或ReadyChanged出现，二者顺序都合法。**Ready不是Alive**，死亡不撤销Ready，本批也没有借Ready事件实现完整AI死亡中止。

## 5. 正常Build与CLion索引

三个文件保存后，UE保持关闭。PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先正常Editor Build，再更新两个模块CodeModel。成功后在原CLion工程Reload CMake Project；新cpp属于EmberTrialCodeModel，工程cpp预计22→**23**，Source文件45→**47**。把光标放在`#include "AIController.h"`文件名上，Ctrl+B应可到本机引擎头文件。

继续Game Development构建：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

双目标成功后再打开UE原工程。失败先发**第一条实际编译错误及上下5–10行文字**，停在Build；不清缓存、不删除Intermediate，不在失败状态继续新建BP。这里的构建是代码验证，尚未Cook / 打包新M1游戏。

### 5.1 首次UHT报错：Character参数与父类成员重名

2026-10-07用户报告`HandleBodyReadyChanged`参数Character与AController作用域冲突。已读取本机`GameFramework/Controller.h`确认父类有`UPROPERTY() TObjectPtr<ACharacter> Character`；UHT禁止本类反射函数参数遮蔽这个继承成员。**原指南命名有误，不是用户照步骤输入的问题。**本指南第4节已统一修正，用户随后改名并双Build成功，源码与修正版一致；以下保留历史修法，不再作为当前待修。

已经按旧版输入的文件只需改名，不改变函数类型 / 逻辑：

1. `.h`把声明改为：

   ```cpp
   void HandleBodyReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);
   ```

2. `.cpp`中以下三处的变量 / 参数及其所有使用，从`Character`改为`BodyCharacter`：**OnPossess的局部变量、HandleBodyReadyChanged的参数、LogBodySnapshot的局部变量**。第4.2节已提供完整修正版，可对照逐处改名；不要只改.h而漏掉.cpp函数体，也不要改`AEmberTrialCharacter`类名、include路径或原Character里的Ready委托。
3. 同步改两个局部变量，避免进入C++编译后又出现继承成员遮蔽的C4458警告 / 错误。回调参数名可以与委托参数名不同，类型与顺序仍是`AEmberTrialCharacter* / bool`，AddUniqueDynamic与RemoveDynamic保持原写法。
4. 保存两文件、UE保持关闭，重新执行本节`Refresh-CLion.ps1`；成功后再Reload CMake和Game Build。无需清缓存、删除Intermediate、重建工程或修改引擎父类。`UBT failed with exit code 6`是脚本报告前面UHT失败的结果，不是PowerShell脚本本身的缺陷。

## 6. 敌人Blueprint：直接从原生身体创建

### 6.1 新资产与父类

1. Content Browser在`/Game/EmberTrial/Characters`下创建`Enemies`目录；完整资产目录对应`E:\UE_Project\Demo\EmberTrial\Content\EmberTrial\Characters\Enemies`。
2. 目录空白处右键 → **Blueprint Class → All Classes**，搜索`EmberTrialCharacter`，选择原生**EmberTrialCharacter**，命名`BP_ETMeleeEnemy`。
3. 打开后Class Settings确认Parent Class=EmberTrialCharacter。**不要Duplicate玩家 / 训练目标 / Waiting，也不要继承BP_ThirdPersonCharacter。**本机5.5.4已有AttributeSet子对象复制失效记录，新建可避免沿用失效默认引用。
4. 本批不创建Controller Blueprint，直接使用新原生ETAIController。

### 6.2 Class Defaults：身体参数与控制权

点击**Class Defaults**，先清空Details搜索，逐项搜索下表名称。配置**蓝图默认值**，不是PIE运行实例。对每一行填完再清空搜索，避免上一关键字藏起属性。

| 属性 | 最终值 / 资源 |
| --- | --- |
| Initial Health / Initial Max Health | **100 / 100** |
| Light Attack Ability Class | **None**，C批再装配敌人攻击 |
| Auto Possess Player | **Disabled** |
| Auto Possess AI | **Placed in World or Spawned** |
| AI Controller Class | **ETAIController**，不是默认AIController / ETPlayerController |
| Auto Receive Input（Input分类） | **Disabled** |
| Can Affect Navigation Generation | **False**，身体不参与地图导航烘焙 |
| Use Controller Rotation Pitch / Yaw / Roll | **False / False / False** |
| Default Mapping Context | `/Game/ThirdPerson/Input/IMC_Default` |
| Jump Action | `/Game/ThirdPerson/Input/Actions/IA_Jump` |
| Move Action | `/Game/ThirdPerson/Input/Actions/IA_Move` |
| Look Action | `/Game/ThirdPerson/Input/Actions/IA_Look` |

四份输入资源只保留共享原生Character的完整默认装配；AI自动控制不会给它创建玩家输入，也不新增敌人IMC或抢占玩家键盘。GameMode Default Pawn、PlayerController Class和地图GameMode Override都保持原值。

### 6.3 CharacterMovement：保留正常落地 / 可移动能力

在Components点选**继承CharacterMovement**，按下表设置。不要复制训练目标的Movement=None / Gravity0，它是固定靶，不是敌人的移动配置。

| 属性 | 最终值 |
| --- | --- |
| Default Land Movement Mode | Walking |
| Default Water Movement Mode | Swimming，保持常规默认 |
| Gravity Scale | 1.0 |
| Max Walk Speed | **220 cm/s**，后续追击先用这个占位速度 |
| Min Analog Walk Speed | 0 |
| Braking Deceleration Walking | 2000 |
| Orient Rotation to Movement | True |
| Use Controller Desired Rotation | False |
| Rotation Rate | **Yaw=360°/s，Pitch=0，Roll=0**；Details的X / Y / Z显示时填**0 / 0 / 360** |
| Run Physics with No Controller | False |

本机Rotator Details按Roll / Pitch / Yaw排列，因此X / Y / Z显示对应0 / 0 / 360；展开具名子项则填Roll=0、Pitch=0、Yaw=360。**C++构造参数顺序不同**，`FRotator(0, 360, 0)`表示Pitch=0、Yaw=360、Roll=0，不能把它的三参数顺序直接抄进Details三个框。

### 6.4 Capsule：挡住世界、可被命中

点选**继承CapsuleComponent**：

| 属性 | 最终值 |
| --- | --- |
| Capsule Radius / Half Height | **42 / 96 cm** |
| Collision Presets | Custom |
| Collision Enabled | **Query and Physics** |
| Object Type | Pawn |
| WorldStatic / WorldDynamic / Pawn | **Block / Block / Block** |
| Visibility | **Overlap** |
| Camera | **Ignore** |
| PhysicsBody / Vehicle / Destructible | Block；其他自定义通道保留原值，本项目未新增 |
| Generate Overlap Events / Simulate Physics | False / False |
| Can Ever Affect Navigation（若在组件Advanced可见） | False |

不要先把所有通道Ignore：敌人需要胶囊阻挡地面 / 墙和其他身体。Visibility=Overlap供当前攻击Sweep收集候选；不是Block，也不是Ignore。Generate Overlap Events=False不会禁用主动Sweep查询。胶囊Pawn Block与导航不烘焙身体是两个不同的设置。

### 6.5 Mesh与既有受击资源

1. 点选继承**Mesh**：Skeletal Mesh=`SKM_Quinn_Simple`（`/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`）；Animation Mode=`Use Animation Blueprint`；Anim Class=`ABP_Quinn`（`/Game/Characters/Mannequins/Animations/ABP_Quinn`）。
2. 打开既有`BP_ThirdPersonCharacter`，选其Mesh，读取**Relative Location / Rotation / Scale**三组数值；逐项填到敌人Mesh。复制的是已验收的Mesh相对胶囊Transform，**不是Actor / 胶囊坐标**；不要猜Z / Yaw。此步骤只读玩家，不修改 / 重新父类化玩家。
3. 敌人Mesh：Collision Presets=`NoCollision`，Generate Overlap Events=False，Simulate Physics=False，Visible=True，Hidden in Game=False，Can Ever Affect Navigation=False。
4. Visibility Based Anim Tick Option=`Always Tick Pose and Refresh Bones`。当前需要可靠预览受击与后续骨骼命中；远处动画节流后期通过测量再加，不为单个占位敌人先隐藏逻辑问题。
5. 点选继承**CombatLife**，Hit React Ability Class填既有`GA_ET_HitReact`：`/Game/EmberTrial/Abilities/GA_ET_HitReact`，Auto Activate保持True。若字段灰色，点击**Class Defaults**，清空搜索并展开继承CombatLife的默认配置，或搜索`Hit React Ability Class`后填写。这是蓝图默认值，不能在地图实例 / PIE对象上改；不另Add一个Life。
6. Class Defaults搜索`Draw Debug`，设置继承CombatTrace的Draw Debug=False；CombatTrace使用现有`hand_r`与25cm默认，不扩大半径。本批不改玩家Debug。
7. Components里单个继承**AbilitySystem、CombatTrace、CombatLife**，不Add第二份；**不添加PracticeAction、UIBootstrap、额外交互范围、训练Cube或Widget**。
8. 继承CameraBoom的Do Collision Test=False；FollowCamera的Auto Activate=False。保留继承组件，不删除它们或改共享原生类；敌人不作为玩家镜头。
9. Event Graph保持空，不接BeginPlay / Tick / Delay / SetHealth / AI Move To。已有未连执行的默认事件可以保留或删掉，不增加业务节点。

**Compile → Save**。没有Missing gameplay dependencies、BindWidget或Native Tick等新增警告；敌人身体本批不创建UMG。

## 7. 正式地图：一个敌人、一个导航范围

### 7.1 放置敌人

打开`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。从Content Browser拖一份BP_ETMeleeEnemy到场景，Outliner命名**MeleeEnemy_A**。

| 配置 | 数值 |
| --- | --- |
| Actor Location X / Y / Z（cm） | **600 / 250 / 120** |
| Actor Rotation X / Y / Z | **0 / 0 / 180**，面向-X |
| Actor Scale | **1 / 1 / 1** |

Z=120先略高于地面，PIE由正常重力落到胶囊中心约96。保留原TrainingTarget_A、交互A / B、地面 / 四墙和**一个PlayerStart**，不放第二个玩家角色或第二个敌人。不调整正式GameMode默认身体；地图原Override保持BP_ETGameMode。

该位置在20m房间内、离默认玩家出生约12m；若已有布局改动造成模型重叠，换一个同样在房间内的空位，并反馈最终位置。敌人实例Details中的Auto Possess / AI Controller Class不得覆盖为别的值。

### 7.2 创建NavMeshBoundsVolume

1. 通过Place Actors / Add搜索**Nav Mesh Bounds Volume**（类名NavMeshBoundsVolume），拖一份到场景，不是NavModifierVolume / 普通BlockingVolume。
2. Outliner命名`Nav_ET_Room`。Actor Transform：Location **0 / 0 / 150**，Rotation **0 / 0 / 0**，Scale **1 / 1 / 1**。
3. 清空Details搜索，展开**Brush Settings**，Brush Shape选择Box（如已为Box保持）。展开Box参数，填尺寸**X=2000 / Y=2000 / Z=500 cm**，Hollow=False。这是Brush尺寸，不是Actor Scale；不要在Scale填2000。
4. 在场景选择导航体，确认边界覆盖整个地面，从Z=-100至400，包含地面顶面0及两个身体位置。尺寸 / 参数修改后点击视口空白处提交。
5. 保持地面 / 四墙Static与原BlockAll；打开**Build → Build Paths**生成导航。若引擎已自动生成，也等构建结束后继续，不额外改为Dynamic Runtime Generation。
6. 在**非PIE的关卡视口**按P显示导航；若P没反应，用视口Show → Navigation开启。地面中央与敌人 / 玩家落地位置应有连续绿色可走区域，墙内 / 靠边缘可能收缩，这是代理半径造成的正常边界。
7. 敌人胶囊 / Mesh不应烘焙成一个永久绿色洞；世界固定障碍留下不可走区域则正常。`RecastNavMesh-Default`若自动出现在Outliner就保留，不手工重复添加。
8. 再按P关闭编辑器导航显示，**Save All**，保存地图和敌人资产。

导航不绿先停在本节：检查放的是正确Volume、Brush实际尺寸、体积包住地面顶面、地面有碰撞且Can Ever Affect Navigation=True；Build Paths后再看。不要修改Character胶囊大小或删除四墙试错。**绿色覆盖只证明生成 / 覆盖，本批不以此声称寻路跟随已通过**；B用真实Move To验证。

## 8. 分层检查，目标5–15分钟

不新增测试函数 / 变量 / 键，不要求手抄全部日志。Output Log可筛选`M1-05A`看控制权，查看战斗时取消筛选以看到M1-04 / Life。不要清空磁盘日志，反馈完成时刻供助手读取。

### 8.1 第一轮出生与独立归属

**准备：**正式地图，Default Player Start，单个MeleeEnemy_A，100/100、Draw Debug=False，导航构建 / 保存完成。

**操作：**Play，点击视口获得输入，等落地约1秒。先观察玩家HUD与敌人位置；看本轮M1-05A日志。若想看实际Controller，在PIE中F8脱离视口，选敌人运行实例查看Controller，结束观察直接Stop，不在运行实例改默认值。

**预期：**玩家原身体 / HUD100/100，敌人Quinn待机、脚部贴地、不下落穿墙、不主动追击 / 攻击；一份ETAIController拥有这一份敌人。日志有一条Possessed，Authority=1；随后至少一条Body snapshot为**Ready=1 / Snapshot=1 / Health=100 / MaxHealth=100 / Dead=0**。PossessSnapshot先Ready=0、再ReadyChanged=1是正常；若一开始已Ready，也可直接得到有效快照。

**重要解释：**现有M1-03A1的`ActorInfo ... Controller=None`打印的是GAS的**PlayerController字段**，AI没有PlayerController时None合法；判断AI控制权以本节`Possessed Controller=ETAIController... Body=BP_ETMeleeEnemy...`和实际Pawn.Controller为准，不能为了消除None给敌人配置ETPlayerController。

**清理：**如果F8查看过，Stop后重新Play执行下一组；无需增加临时Possess图。

### 8.2 玩家攻击新敌人：受击 / 死亡 / HUD隔离

**准备：**当前新一轮正常游戏速度、敌人100/100，玩家100/100。敌人本批不还手，沿用玩家正式攻击25伤害。

**操作：**绕过出生附近训练目标，走到MeleeEnemy_A的**-X一侧**，面向+X、站定接近后左键一次，等攻击与受击结束。命中成功后再完成三次独立攻击，每次等动作结束；最后再空挥一次靠近它的攻击。遇到空挥先调整站位 / 朝向，不扩大半径或窗口。

**预期：**日志必须指向`BP_ETMeleeEnemy...`，不是旧TrainingTarget。四次成功伤害100→75→50→25→0；前三次有对应敌人Hit begin / end，最后一次执行敌人Death committed / sealed且不另起受击。敌人归零冻结，无后续有效伤害；再击若轨迹接触会TargetDead拒绝，若没有候选则不强求该条日志。玩家HUD始终100/100；胶囊挡世界和身体，敌人没有第二份玩家面板。

**清理：**Stop。这是实例生命变化，不改Class Defaults、能力伤害或GE；下一PIE自然重置100/100。本批不检查敌人攻击与玩家死亡后AI停追，它们尚未实现。

### 8.3 重开、结束清理与玩家短基线

**准备：**停止上组PIE，地图 / Blueprint默认值未修改。

**操作：**再Play，确认新敌人100/100、一个新Controller；移动 / 转向 / 跳跃，P打开后鼠标或键盘继续一次，再回移动；原E在A或B旁确认一次即可。最后Stop，Compile / Save敌人，Save All。

**预期：**新一轮独立出生，无上一轮死亡 / 旧身体数据串入；只有玩家HUD，输入和暂停恢复正常。结束有对应Controller的`End`，以及既有Character / Life清理；End之后同一个Controller不再输出Body snapshot。`Unpossessed`和ReadyChanged=0是否先出现取决于引擎结束顺序，**不要求固定顺序或每轮两条都有**。无Missing gameplay dependencies / Accessed None / 新Error。

**清理：**停止PIE、编辑器导航显示关闭、单个正常敌人 / Nav_ET_Room保留；无临时节点 / 挡板 / 帧率 / slomo修改。原3 / 4开发入口继续保留，不要求重跑窗口内死亡或旧HUD矩阵。

## 9. 出错时发什么

| 现象 | 提供材料 |
| --- | --- |
| C++ Build失败 | 第一条编译错误及上下5–10行文字，不用截整页VS / CLion日志 |
| 找不到原生Controller / 父类 | 双构建结果和你停在哪一步；源文件位置文字，必要时Classes下拉截图 |
| 无Possessed / Snapshot始终0 | 完成时间 + 敌人Class Defaults的Auto Possess / AI Controller Class / Initial Health截图；日志由助手读取 |
| 有伤害但不播受击 | 对应完成时间，敌人Mesh Anim Class与Class Defaults Hit Class截图 |
| 模型飘浮 / 下陷 / 朝向怪 | Mesh相对Transform与Actor Transform截图，加5–10秒运行视频 |
| 导航不绿 / 留大洞 | 非PIE关卡导航全景与Volume Brush尺寸截图，说明Build Paths结果 |

先定位失败组，保留已通过且无关结果。截图用于布局 / Details，文字用于编译错误，短视频用于动画 / 时序。普通报错不自动授权助手修改C++；需接管时明确提出即可。

## 10. 简短反馈模板

```text
M1-05A 完成 / 卡在第__节
Editor Build / 索引23cpp / Game Build：
敌人原生父类 / 单ASC-Trace-Life / 无Practice与M0业务：
AI Controller与Auto Possess / Ready100/100快照：
导航覆盖玩家与敌人 / 无身体烘焙洞：
四次真实伤害、受击与一次死亡 / 玩家HUD隔离：
重开100/100 / Controller结束清理 / P与输入短基线：
单敌人 / 单导航体、默认值与资产已保存：
失败或特别说明（位置如有调整）：
大致完成时间（完整时分）：
实际用时：代码__ / 资产地图__ / 检查__；构建等待__分钟
```

A通过后我读取本地源码、构建与运行日志，登记新敌人资产 / 地图实际变更；再下发B的感知、BB / BT与真实追击。不提前把未实施AI或新功能标为验收通过。

## 11. 本机接口核对与当前证据范围

本批代码根据本机5.5.4的`AIModule/Classes/AIController.h`与`Private/AIController.cpp`核对构造、OnPossess / OnUnPossess、StopMovement、AI逻辑开关；根据`Engine/Classes/GameFramework/Pawn.h`与`Private/Pawn.cpp`核对Auto Possess规则；根据`UnrealEd/Classes/Builders/CubeBuilder.h`核对Box尺寸。共享Ready委托 / Health查询以当前Character源码为准。

首次下发时接口核对不代替UHT / 正常双目标构建和实机结果。当前用户已完成两份Controller / AIModule、47份Source / 23cpp并通过双Build；新敌人创建 / 保存、地图与36项清单基线已登记，相关日志 / 用户观察边界见[A验收](Reports/M1_05A_Verification.md)。A仍不包含感知、真实路径跟随或AI攻击，不把身体已受控当成整章完成。
