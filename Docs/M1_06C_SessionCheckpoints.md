# M1-06C：会话检查点与“继续本次试炼”

2026-10-19第20批清理同步：会话 / 检查点 / GameMode以当前Source为准，不用早期全文覆盖检查点继续逻辑。本轮按用户授权直接清理，原功能与历史验收保留；当前检查和提交清单见[Cleanup](Cleanup.md)。

下发：2026-10-11；验收更新：2026-10-12；UE **5.5.4**。前置[B功能验收](Reports/M1_06B_Verification.md)通过。**本批已由用户手动实施并验收通过，无待补检查。**[C报告](Reports/M1_06C_Verification.md)核对67Source / 33cpp / 来源清单46、十份代码、双构建与检查点重试 / 继续 / 重置 / ID回退修正。实际手动85分钟（代码45 / 资产30 / 检查10）、等待5。历史主体和B报告已提交，当前功能状态以报告为准；本轮清理提交见Cleanup。

下文保留手动实施指南与复用入口。原C的C++由用户手动完成，当时助手仅核查证据并同步材料；当前清理按本轮授权直接修改Source并执行相应验证，Git仍由用户操作。此前经当次授权仅设置9.2第8步播放选项，并澄清9.3默认文本入口。报错文字优先，Designer截图辅助，需要接管时由你明确授权。

## 当前菜单边界

菜单Widget的bContinueAvailable只是已确认会话的呈现快照，不保存检查点；bBusy控制按钮，Controller自己的pending同时保护直接请求。开始 / 继续仍由RunSession提交，失败恢复仍重新读取HasContinueSnapshot；焦点设置与三按钮意图保持，不加入新的Tick或蓝图业务。下方历史完整类改为当前Source入口。

## 当前会话与出生契约

[RunSession声明](../EmberTrial/Source/EmberTrial/Public/Flow/ETRunSessionSubsystem.h) / [实现](../EmberTrial/Source/EmberTrial/Private/Flow/ETRunSessionSubsystem.cpp)保存已确认地图 / ID，与Pending目标 / 意图区分；菜单的Continue资格读已确认快照，不因新菜单早于PostLoad而永久禁用。它不保存Transform、Actor、ASC或磁盘数据。

| 时机 | 当前职责 |
| --- | --- |
| 请求 | 预检地图 / 世界 / 意图；先登记Pending，再解除暂停和OpenLevel，重复请求拒绝 |
| 选择出生点 | [GameMode](../EmberTrial/Source/EmberTrial/Private/Framework/ETGameMode.cpp)在生成Pawn前读待定ID，排除PIE临时Start；Entry必须唯一，请求ID缺失 / 重复则回Entry |
| 出生确认 | StartPlay在Super后确认已有Pawn，报告实际选择ID；这是地图出生报告，不是身体GAS Ready |
| 到达 | 同GI PostLoad核对目标包名；玩法地图还需出生报告，确认后才提交快照，再释放Pending |
| 失败 | 清Pending后广播，保留旧快照供重试；来源World / 同GI规则按原实现区分 |
| 激活 | [CheckpointStart](../EmberTrial/Source/EmberTrial/Private/Flow/ETCheckpointStart.cpp)只转发候选，Session验证同世界、本地受控、Ready / 存活身体和唯一ID；先提交再广播 |

CheckpointStart只做Overlap与一次NextTick补查，结束清Timer / Overlap / 自有委托，无常驻Tick。原组件名与默认值保持，Inactive文字 / 颜色共用内部常量。GameMode三处会话查询共用私有GetRunSession，每次读取当前GI，无新缓存。

以下配置与分组检查保留为历史学习入口，不用旧全文覆盖当前Source；本轮未重新执行的真实切图 / 故障分支不会写成新Success。

## 1. 本批效果、范围与预算

正式地图保留原入口，新增一个可见检查点。入口稳定ID为`Entry`，检查点为`Checkpoint_A`：

| 操作 | 结果 |
| --- | --- |
| 首次菜单开始，或直接正式地图PIE | 从Entry出生；建立本次会话 |
| 存活玩家走进检查点范围 | 记录当前地图 + Checkpoint_A；3D提示由CHECKPOINT变为ACTIVE |
| 死亡后重新试炼 | 重载地图，新身体从最近检查点出生、100/100 |
| 死亡后返回主菜单 → 继续本次试炼 | 同一进程内重载最近会话地图 / 检查点，新身体100/100 |
| 菜单开始新试炼 | 成功进入后会话改为Entry；不沿用上一轮Checkpoint_A |
| 检查点ID失效 / 重复 | 有唯一Entry时回退入口，确认到达后修正会话ID；不随机选择 |
| Stop PIE，再启动菜单PIE | 新GameInstance，无会话；继续禁用 |

这是**当前进程内的会话快照**，不是SaveGame磁盘存档。只保存地图包名与FName稳定ID，不保存Actor指针 / 任意Transform / ASC / 生命 / 敌人状态。重试与继续仍重建整图，敌人、训练目标与交互回初始状态。菜单文案明确“继续本次试炼”，关闭程序后会话清除；M5再扩展跨启动存档。

NewRun的“清空旧检查点”在成功到达后提交；请求前校验或加载失败保留已确认的旧快照，避免一次无效点击丢失会话。新的待定出生ID在生成Pawn之前可读，GameMode选择实际出生点，再由到达确认提交。检查点激活必须来自当前世界、当前玩家控制的存活Ready身体，切图中 / 暂停时拒绝。

原预算**80–130分钟手动**：阅读5–10、代码40–65、资产25–40、相关检查10–15；实际手动**85分钟**，检查10 / 等待5。项目累计手动**1731分钟（28小时51分钟）/ 等待135分钟**。零新增付费资产，3D英文小标签是占位提示，正式UMG继续使用现有中文字体链。原C阶段将D暂停迁移和M1-07包 / 性能留给后续，这些工作现已完成；当前状态见Progress。

## 2. 准备与文件清单

1. UE Save All，然后关闭UE编辑器；暂停编辑下表十个文件。需要新增反射成员 / 类，使用正常UBT构建，不用Live Coding或自动Hot Reload验证本批。
2. CLion打开已有工程根目录`E:\UE_Project\Demo\EmberTrial`，不要单独打开新文件夹成另一个工作空间。
3. 新建两份Checkpoint文件；可以手动创建目录 / 文件，按本文全文粘贴。若通过UE创建类，父类选择PlayerStart、类名ETCheckpointStart，随后关闭UE再替换全文，遵守既有CLion新类指南。
4. 下表八个既有文件均**替换全文**；本批只两个新文件，不重复添加Session / Menu类，不修改FailureFlow / CombatLife / 玩家角色源码。保留每个头文件最后的generated include顺序。

路径均相对于仓库根目录`E:\UE_Project\Demo`：

| 操作 | 文件 |
| --- | --- |
| 替换 | `EmberTrial/Source/EmberTrial/Public/Flow/ETRunSessionSubsystem.h` |
| 替换 | `EmberTrial/Source/EmberTrial/Private/Flow/ETRunSessionSubsystem.cpp` |
| 替换 | `EmberTrial/Source/EmberTrial/Public/Framework/ETGameMode.h` |
| 替换 | `EmberTrial/Source/EmberTrial/Private/Framework/ETGameMode.cpp` |
| 新建 | `EmberTrial/Source/EmberTrial/Public/Flow/ETCheckpointStart.h` |
| 新建 | `EmberTrial/Source/EmberTrial/Private/Flow/ETCheckpointStart.cpp` |
| 替换 | `EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h` |
| 替换 | `EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp` |
| 替换 | `EmberTrial/Source/EmberTrialUI/Public/FrontEnd/ETMainMenuController.h` |
| 替换 | `EmberTrial/Source/EmberTrialUI/Private/FrontEnd/ETMainMenuController.cpp` |

Core不依赖UI；现有Build.cs / Target / .uproject与引擎版本不变。实施后实际**67份Source / 33个cpp**，已核对编译数据库；资产验收后清单**45→46**，新BP与两旧资产哈希已登记。

## 3. RunSession：已确认快照与待定出生ID

### 3.1 ETRunSessionSubsystem.h全文

当前源码：[ETRunSessionSubsystem.h](../EmberTrial/Source/EmberTrial/Public/Flow/ETRunSessionSubsystem.h)。这里保留历史接入位置，完整实现只在Source维护。

### 3.2 ETRunSessionSubsystem.cpp全文

当前源码：[ETRunSessionSubsystem.cpp](../EmberTrial/Source/EmberTrial/Private/Flow/ETRunSessionSubsystem.cpp)。这里保留历史接入位置，完整实现只在Source维护。

## 4. GameMode：生成Pawn之前选择稳定出生点

不要新增BeginPlay传送玩家的蓝图。检查点Actor本身继承PlayerStart，GameMode遍历这类出生标记，选择唯一匹配ID。入口缺失 / 重复是地图配置错误；直接返回空出生点，不用引擎默认FindPlayerStart退到世界原点。

### 4.1 ETGameMode.h全文

当前源码：[ETGameMode.h](../EmberTrial/Source/EmberTrial/Public/Framework/ETGameMode.h)。这里保留历史接入位置，完整实现只在Source维护。

### 4.2 ETGameMode.cpp全文

当前源码：[ETRunSessionSubsystem.cpp](../EmberTrial/Source/EmberTrial/Private/Flow/ETRunSessionSubsystem.cpp)。这里保留历史接入位置，完整实现只在Source维护。

本机5.5.4的默认FindPlayerStart会在Choose返回null时退到WorldSettings。这里覆盖Find并转到自己的Choose，确保无有效Entry时不悄悄出生在0 / 0 / 0。正式地图BP不能另写Choose / Find覆盖；本批离线单玩家不使用Portal参数或旧StartSpot。

## 5. CheckpointStart：触发、出生与占位反馈

激活区只查询Pawn；每次进入检查当前玩家身份 / Ready / 死亡 / 暂停 / 会话。一次NextTick扫描处理刚出生已重叠的情况，随后无常驻Tick。Session原生事件驱动标签状态，结束主动解绑；敌人进入不写快照。

### 5.1 ETCheckpointStart.h全文

当前源码：[ETCheckpointStart.h](../EmberTrial/Source/EmberTrial/Public/Flow/ETCheckpointStart.h)。这里保留历史接入位置，完整实现只在Source维护。

### 5.2 ETCheckpointStart.cpp全文

当前源码：[ETCheckpointStart.cpp](../EmberTrial/Source/EmberTrial/Private/Flow/ETCheckpointStart.cpp)。这里保留历史接入位置，完整实现只在Source维护。

## 6. MainMenu Widget：继续意图与会话文案

四个既有BindWidget保持原名称 / 类型；新增的是C++按钮事件，不增加UMG控件或Graph。Continue资格由Controller读取已确认会话快照传给View。HasContinueSnapshot只表示快照存在，不能用travel pending暂时掩盖它：菜单BeginPlay早于PostLoadMap，返回菜单时应立即显示正确继续资格；重复请求仍由BeginTravel / Controller门控拒绝。

### 6.1 ETMainMenuWidget.h全文

当前源码：[ETMainMenuWidget.h](../EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h)。

### 6.2 ETMainMenuWidget.cpp全文

当前源码：[ETMainMenuWidget.cpp](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp)。

## 7. MainMenu Controller：路由继续与清理第三个按钮

继续使用Session已确认的地图，Start使用原GameplayMap配置。两者不是一回事；Start失败恢复时重新读资格，不把旧会话误清空。失败 / 结束保护和唯一Widget保留。

### 7.1 ETMainMenuController.h全文

当前源码：[ETMainMenuController.h](../EmberTrial/Source/EmberTrialUI/Public/FrontEnd/ETMainMenuController.h)。

### 7.2 ETMainMenuController.cpp全文

当前源码：[ETMainMenuController.cpp](../EmberTrial/Source/EmberTrialUI/Private/FrontEnd/ETMainMenuController.cpp)。

## 8. 正常构建与CLion索引

十个文件保存后，确认UE已关闭。在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

1. 本脚本先正常Editor Build，再更新两模块CodeModel；成功后在CLion执行Reload CMake Project。
2. 本批只新增CheckpointStart.cpp，工程cpp预计**32→33**；源文件65→67。确认新头 / cpp属于Core目标，引擎PlayerStart / Sphere / TextRender头能跳转。不要把Unity生成cpp计入数量。
3. 再做正常Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

4. 双目标成功才重新打开工程；本批不重新Cook / 打包，也不要求重跑未改的Lifecycle / DodgeMovement。
5. 若只有exit6，往上找第一条真实UHT / C++ Error及其上下文，文字粘贴给我；不要先清缓存。若旧调用报ShowReady / NotifyGameplayWorldReady参数不匹配，检查第3 / 4 / 6 / 7节八个旧文件是否都替换完整。

## 9. 资产与地图：逐项配置

本批**不新增任何Blueprint业务连线**。新BP只负责资源配置，菜单只改显示文字 / 原按钮设置，出生点由GameMode原生函数选择。既有数字4开发伤害、失败页重试 / 返回菜单、P继续保留。

### 9.1 新建BP_ETCheckpointStart

1. Content Browser进入`/Game/EmberTrial/Blueprints/Flow`，没有Flow文件夹则创建。
2. 右键 → Blueprint Class → All Classes，搜索**ETCheckpointStart**，选择对应原生类，创建`BP_ETCheckpointStart`。不要选择普通Actor、普通PlayerStart或原玩家BP。
3. 打开BP，Class Settings确认Parent Class为ETCheckpointStart；Components应有继承的CollisionCapsule、ActivationTrigger、Marker、Label及编辑器图标 / 箭头。**不要再Add一份Sphere、Mesh或Label。**
4. 点**Class Defaults**，在Details搜索`Player Start Tag`，将其确认设为 **Checkpoint_A**。清空搜索后确认Actor Scale为1 / 1 / 1。本批只此一个检查点；不要设为Entry或None。
5. **这里的Player Start Tag是PlayerStart的单个FName属性，不是Actor → Tags数组，也不是Outliner名称。**默认值统一设在BP Class Defaults，随后地图实例不单独覆盖它，避免同一资源默认值与实例值分开维护。
6. Components选**ActivationTrigger**，核对下表。原生构造已给值；若蓝图此前保存过其他值，按表恢复。

| ActivationTrigger属性 | 值 |
| --- | --- |
| Relative Location / Rotation | 均0 / 0 / 0 |
| Relative Scale | 1 / 1 / 1 |
| Sphere Radius | 120 cm |
| Collision Enabled | Query Only |
| Object Type | WorldDynamic |
| Collision Responses | Pawn=Overlap，其余Ignore |
| Generate Overlap Events | True |
| Can Ever Affect Navigation | False |
| Simulate Physics | False |

碰撞分类被父类隐藏时可在Details搜索具体属性；值已由C++设置，不为了找一个隐藏字段改父类。只读核对玩家继承Capsule的Object Type仍Pawn、Generate Overlap Events=True，保留现有Visibility配置。既有交互范围已正常触发，不先改玩家碰撞。

7. Components选**Marker**，Static Mesh选择Engine Basic Shapes的 **Cylinder**（完整资产`/Engine/BasicShapes/Cylinder`）。找不到时Content Browser打开Show Engine Content，再搜索Cylinder；使用引擎基础模型，勿导入外部包。
8. Marker设置：

| 属性 | 值 |
| --- | --- |
| Relative Location X / Y / Z | 0 / 0 / -114 cm |
| Relative Rotation | 0 / 0 / 0 |
| Relative Scale X / Y / Z | 1.5 / 1.5 / 0.08 |
| Collision Enabled | No Collision |
| Generate Overlap Events | False |
| Can Ever Affect Navigation | False |
| Simulate Physics / Cast Shadow | False / False |
| Visible / Hidden in Game | True / False |

Actor放在Z=120时圆盘中心约Z=6，作为可走过的地面标记。材质先保持引擎默认；不用创建材质、粒子、灯光或Blender模型。Marker不参与攻击遮挡 / 导航。

9. Components选**Label**，保留Relative Location **0 / 0 / 60**、Rotation0 / 0 / 0、Scale1 / 1 / 1；World Size **24**、Horizontal Alignment **Center**、Visible=True、Hidden in Game=False，NoCollision / 不影响导航。不要手动绑定文字：运行由原生RefreshDisplay写CHECKPOINT / ACTIVE及青 / 绿色。英文占位避免额外引入3D中文字体资产。
10. BP Event Graph保持无本批业务逻辑；Compile → Save。Components只有一份触发、标记与标签。

### 9.2 正式地图：一个入口 + 一个检查点

1. 打开`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，保持现有GameMode Override **BP_ETGameMode**、默认身体 / Controller、敌人 / 训练目标 / 交互和导航。
2. Outliner选择原来的普通**PlayerStart**。只改其**Player Start Tag=Entry**，可以把Outliner显示名改为`Start_Entry`便于辨认；显示名不作为ID。
3. 入口Transform保留已验收值，原方案为 **Location -600 / 0 / 120、Rotation 0 / 0 / 0、Scale1 / 1 / 1**。若你已调整位置，保留安全位置并在反馈写实际值，避免本批移动训练目标。
4. 从Content Browser拖**一份BP_ETCheckpointStart**到地图，Outliner命名`Start_Checkpoint_A`，填：

| Actor属性 | 值 |
| --- | --- |
| Location X / Y / Z（cm） | **-600 / 650 / 120** |
| Rotation X / Y / Z（度） | **0 / 0 / -90**，朝向入口一侧 |
| Scale | **1 / 1 / 1** |
| Player Start Tag | 继承BP默认Checkpoint_A，**实例不覆盖** |

5. 搜索实例的Player Start Tag。如果右侧有重置箭头，点它恢复BP默认；确认仍为Checkpoint_A。以后ID由BP Class Defaults维护，本批不在地图实例另填相同值。
6. 保证检查点胶囊 / 出生位置与墙、敌人、训练目标 / 交互箱没有重叠，正下方有地面；入口距离检查点约650cm，大于触发范围，开始新试炼不会出生即激活它。如现有布局占用该位置，只平移检查点到安全空地，入口与检查点间距至少400cm，在反馈记坐标。
7. Outliner搜PlayerStart可能不会显示自定义类名；还要搜`Start_` / `BP_ETCheckpointStart`核对：**总共两个可用出生标记，Entry与Checkpoint_A各一份**。不是继续遵守M1-01的“全图单个PlayerStart”；本批正是增加第二种出生点。
8. UE5.5.4本机界面：点击顶部播放工具栏**最右端的三个竖点 ⋮**（在播放 / 暂停 / 停止等按钮右侧，Platforms左侧；悬停提示为`Change Play Mode and Play Settings`）。弹出菜单中找到**SPAWN PLAYER AT…**分组，点击**Default Player Start**，确认其左侧为选中圆点；不用Current Camera Location / Play From Here。同一菜单核对**Number of Players=1**，展开**Net Mode → Play Standalone**并确认选中；顶部Modes保持Selected Viewport即可进行单玩家PIE。此处不是视口内的下拉箭头，也无需进入Project Settings。2026-10-12已由助手通过Computer Use选择并复核上述三项；本次未启动PIE，不代表本章验收完成。
9. Build → Build Paths，按P只检查原导航没有在新标记处挖出孔洞，再按P关闭导航显示；不重跑AI无路径矩阵。File → Save All。若地图采用外部Actor保存，关联ExternalActors / ExternalObjects也属于地图交付。

### 9.3 原WBP_ETMainMenu：继续可点击与文案

1. 打开`/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`，Parent仍**ETMainMenuWidget**，原四BindWidget为StartButton / ContinueButton / QuitButton / StatusText，名称 / 类型不变。
2. Hierarchy选**StartButton下面的Text控件**，Text改为 **开始新试炼**；再选**ContinueButton下面的Text控件**，Text改为 **继续本次试炼**。改的是按钮里的文字，不重命名两个Button。
3. 选**ContinueButton本身**，Details的Is Enabled设 **True**，Is Focusable=True，Visibility=Visible。运行时原生代码根据真实快照禁用 / 启用，所以Designer默认不要留下禁用值。
4. StartButton / ContinueButton / QuitButton均保持Click Method **Down And Up**、Press Method **Down And Up**；Touch Method原设置保留。不要改成按下立即执行，避免键盘松开落到新世界。
5. 三按钮Navigation保留既有自动导航，Continue不能设Hidden / Collapsed；继续可用时Start获得初始焦点，Tab一次应能到Continue。没有会话时Continue禁用，键盘会跳过。
6. 修改**StatusText控件在设计界面里的默认显示文字**；界面没有叫“Designer Text”的属性，实际入口是**Details → Content → Text（详细信息 → 内容 → 文本）**。依次操作：

   1. 在WBP_ETMainMenu编辑器右上角切到**Designer（设计器）**，不要停在Graph。
   2. 左侧**Hierarchy（层级）**点击已有的**StatusText**（Text Block控件），不要选整个Widget、按钮或按钮内的文字。
   3. 右侧**Details（详细信息）**清空搜索框，展开**Content（内容）**，找到**Text（文本）**输入框；也可以在Details搜索`Text`，再定位Content分类下的Text字段。
   4. 在这个Text输入框填写 **会话检查点仅在本次运行有效**。修改显示内容，不改顶部控件名称StatusText；Text右侧不要创建Bind / Binding。

   如果Details没有显示，通过顶部**Window → Details**打开；确认左侧选中StatusText后再找上述字段。运行时ShowReady会覆盖默认文字、显示实际会话状态。保持现有中文字体链、字号 / 布局、无Text Binding；Widget Is Focusable / Tick Never等既有配置不变，最后与第7步一起Compile → Save。
7. Event Graph不用写OnClicked / Cast / OpenLevel节点，原生已绑定三意图。Compile → Save，无BindWidget / Native Tick Warning。

### 9.4 保留配置核对与保存

- BP_ETGameMode父类仍ETGameMode，Classes原值不变，Graph无ChoosePlayerStart / FindPlayerStart覆盖；如有旧自定义覆盖，请先给我说明，不把它和新原生选择混用。
- BP_ETMainMenuController仍原类、原WBP_ETMainMenu与Gameplay Map，菜单无UIBootstrap / Pawn；正常Compile → Save。父类新函数不是要求你再连一遍按钮事件。
- BP_ETPlayerController的单UIBootstrap / 单FailureFlow及WBP_ETFailure / MainMenu Map、原玩家BP的P死亡Branch均保留，本批无必需编辑。
- Game Default Map仍L_ET_MainMenu、Editor Startup Map仍L_ET_VerticalSlice，Cook原三张地图列表不变。**不添加Checkpoint BP为Cook Map条目。**
- Save All；没有手动生命 / 攻击 / AI / 帧率改值，玩家100/100、4=Damage25、slomo1、Debug False保持原基线。

## 10. 分层验收：四组，约10–15分钟

用正式按钮与既有4损伤入口。你只观察表现并给完成时分，我读日志统计ID、请求、到达与清理。不新增Controller测试变量 / 函数，不要求你抄实例名或掌握亚秒时点。

### 10.1 Entry → 激活 → 同ID去重 → 死亡重试（约3–4分钟）

**准备：**最终配置正确，打开L_ET_MainMenu，Play；不要从新检查点旁用Play From Here启动。

**按顺序：**

1. 新会话Continue禁用，中文文案说明当前进程有效；鼠标开始新试炼。
2. 确认从原Entry附近出生、100/100 / 单HUD，检查点Label为CHECKPOINT，没有开局自动改到Checkpoint_A。
3. 走向检查点圆盘（原坐标在入口右侧Y+650，初始朝+X时可用D靠近），进入120cm球体附近，Label变绿色ACTIVE。
4. 离开到球体外，再走回一次；状态仍ACTIVE，不连续重复激活日志。敌人跟进范围也不应替你写新快照。
5. 站在检查点附近，按4扣到零；鼠标点正式失败页的重新试炼一次。
6. 新身体在检查点出生，朝向按检查点箭头，生命100/100、新单HUD / 无旧失败页；Label在一次初始化之后为ACTIVE，站着不重复激活。同一世界重复ID与新世界恢复快照是两件事。

**日志预期：**

```text
[M1-06C] Travel requested Intent=NewRun ... Checkpoint=Entry Paused=0
[M1-06C] Spawn selected Requested=Entry Actual=Entry ...
[M1-06C] Snapshot committed ... Checkpoint=Entry Intent=NewRun
[M1-06C] Checkpoint activated ... Id=Checkpoint_A Body=当前玩家
... 原Death sealed / Failure shown ...
[M1-06C] Travel requested Intent=Retry ... Checkpoint=Checkpoint_A Paused=0
[M1-06C] Spawn selected Requested=Checkpoint_A Actual=Checkpoint_A ...
[M1-06C] Snapshot committed ... Checkpoint=Checkpoint_A Intent=Retry
[M1-06C] Travel arrived Intent=Retry ... Expected=1 Confirmed=1 Pending=0
```

引擎可能在登录和实际生成时各选择一次Start，不把两条相同Spawn selected当成两次切图。判断重复切图看Travel requested / arrived。**清理：**不改参数，保留当前重试后的活身体，继续10.2。

### 10.2 菜单继续与开始重置（约3–4分钟）

**准备：**10.1已激活Checkpoint_A并重试，当前活身体在检查点。

1. 按4到零，正式失败页点击返回主菜单；不Stop PIE。
2. 菜单Pawn / 战斗HUD隔离，**继续本次试炼立即可用**，状态文字说明关闭游戏后会话清除。
3. 默认焦点在Start，Tab到Continue，按**Enter并松开**；确认从Checkpoint_A出生、100/100、输入恢复，没有补攻击 / 跳跃。若焦点在别处，用Tab回Continue。
4. 再按4到零 → 返回菜单；这次点击**开始新试炼**。
5. 确认从Entry出生、100/100，检查点Label为CHECKPOINT，未保留上轮激活。此时不要走近它。
6. 在Entry按4到零 → 返回菜单 → 鼠标继续一次，仍应从Entry出生。这证明NewRun已提交新快照，而不是画面暂时回入口、后台仍留旧ID。

**预期：**第一次Continue requested.Checkpoint=Checkpoint_A，随后Actual同ID、Confirmed1；NewRun只请求Entry，随后继续请求也为Entry。菜单清理第三个Continue委托，旧FailureFlow / HUD / AI结束沿用B日志。

**清理：**保留当前活身体，继续10.3。没有None GE / 帧率 / slomo临时设置，不重测旧阵营或HUD重建。

### 10.3 缺失ID回退：一个可复用PIE开发命令（约2–3分钟）

**准备：**当前活身体回到Entry附近，远离检查点触发区；保持PIE，不更改任何资产ID。本批GameMode保留一个开发命令，只在WITH_EDITOR构建的PIE世界允许临时覆盖会话ID；Game目标 / 独立包返回false，没有运行时写入效果。无需测试Graph / 新按键 / 逐对象名字复制。

1. 点正在运行的视口，按键盘反引号键打开控制台，输入以下整行并Enter：

```text
ke BP_ETGameMode_C ETDebugSetCheckpointId Missing_Checkpoint_Test
```

2. 看控制台有Debug checkpoint result **Applied=1**，然后关闭控制台。这里按类名调用当前世界唯一GameMode，不加实例后缀或通配符；若显示未找到函数 / Applied0，给我原文，不继续假设注入成功。
3. 保持在触发区外，按4归零 → 正式失败页鼠标重新试炼。请求带Missing_Checkpoint_Test，新地图没有此ID；应有Spawn fallback Warning，安全回Entry、100/100，Confirmed1。
4. 不靠近检查点，Entry处再次4归零 → 重新试炼一次。这次请求 / 出生Entry，不再因同一失效ID回退，证明上次到达已修正会话。
5. Stop PIE。正式资产始终Entry / Checkpoint_A，无临时Tag或Graph要恢复；Save All，继续10.4。

**预期：**Debug override.Id=Missing_Checkpoint_Test，Spawn fallback Requested=Missing_Checkpoint_Test Matches=0 Fallback=Entry，随后Snapshot committed.Checkpoint=Entry；下一次Retry请求Entry。Spawn选择可能在登录 / 生成时各有回退提示，切图请求仍只有一次；不按Warning条数判断重载次数。

这只注入一个畸形会话ID，实际Spawn选择、重试、到达提交及第二次请求均走正式链路；不是直接调用回退函数然后宣称完整流程通过。命令保留供后续相关规则复验，无磁盘副作用。重复ID保护也存在，本批不再多搭一组重复Actor操作，不把该分支写成已实机验收。

### 10.4 会话结束、直接地图入口与短基线（约1–3分钟）

**准备：**10.3已Stop，正式BP默认ID一直为Checkpoint_A。

1. 从L_ET_MainMenu重新Play，Continue应禁用；这验证新GameInstance不沿用上一轮ID。菜单退出一次，PIE正常结束。
2. 打开正式地图直接Play，应该从Entry出生并完成地图 / Entry登记；检查点未开局激活，玩家100/100 / 单HUD。
3. 只做移动 / 转向 / 跳跃、E原目标提示、一次站定左键和2，再P打开 / 键盘继续；确认无补播 / 卡键。不重做三帧率、两种死亡、长按矩阵。
4. Stop → Save All，编辑器回正式地图。确认Entry / Checkpoint_A各一份、Checkpoint BP无临时Tag、原生命 / 4 / slomo / Debug参数保持。

**预期：**新Session ready / end，不同PIE不会恢复上一轮；直接图Gameplay world ready.Spawn=Entry，旧Checkpoint end / Session end / UI清理闭合。真正退出独立进程再启动在M1-07包验证，本轮Stop不冒充包内实测。

## 11. 完成反馈（复制填写；我读取日志）

```text
M1-06C 完成
两新 / 八旧Source；Editor Build / 索引33cpp / Game Build：
Entry / Checkpoint_A各一份；新BP父类 / 单触发 / NoCollision标记 / ID无实例覆盖：
菜单四BindWidget / 无Binding / Never / 三按钮与中文；Compile / Save：
10.1 Entry开始 / 首次激活 / 重入去重 / 死亡重试Checkpoint_A：
10.2 菜单Enter继续Checkpoint_A / 新试炼回Entry / 随后继续Entry：
10.3 PIE开发命令Applied1 / 缺失回退Entry / 下一次会话ID修正：
10.4 Stop后Continue禁用 / 直接地图Entry / 退出清理：
最后E / P键盘继续 / 左键与2 / 移动转向跳跃、HUD100：
Checkpoint_A保持、唯一Entry、100/100、4=Damage25、slomo1、Debug False / Save All：
检查点最终坐标 / 朝向（如有调整）：
失败或特别说明：
大致完成时间（完整时分，例如21:18）：
实际用时：代码__ / 资产__ / 检查__；构建等待__分钟
```

**2026-10-12 00:30反馈已验收**：十份Source与指南一致、33cpp / 双receipt正确，新BP与两旧资产正常保存；ID激活、Retry / Continue / NewRun、缺失回退 / 会话修正与旧源清理有日志，具体Designer / 输入边界按用户反馈登记。详见[C报告](Reports/M1_06C_Verification.md)，来源清单46项。本批无待补，不必重复第10节。

## 12. 静态依据、边界与排错

本机UE5.5.4源码已核对：PlayerStart.h的PlayerStartTag为EditAnywhere FName；APlayerStart使用FObjectInitializer构造；GameModeBase的Choose / Find为BlueprintNativeEvent，出生由Find → RestartPlayerAtPlayerStart → SpawnDefaultPawnFor进行。默认Find无Start时回到WorldSettings，因此本批自定义Find避免此回退。LoadMap先执行World.BeginPlay，再发PostLoadMapWithWorld；待定出生ID必须先可读，正式快照后确认。

引擎入口亦见[Epic FindPlayerStart 5.5 API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/GameFramework/AGameModeBase/FindPlayerStart?application_version=5.5)。核心设计以本机5.5.4实际源码为准，相关组件 / Timer / Delegate / Button接口沿用当前版本。下发时静态核对与后续实测分开；**现已验收双目标构建及指定运行用例**，范围 / 未实测分支见[C报告](Reports/M1_06C_Verification.md)。

| 现象 | 优先定位 / 给我的材料 |
| --- | --- |
| UHT shadowing | 第一条真实错误、参数名与所属类；本批动态Overlap使用标准签名，没有在Controller反射参数里用Character |
| 开始出生到检查点 | NewRun.Checkpoint是否Entry；Entry标签是否填在Player Start Tag、是否唯一；BP GameMode是否另有Find / Choose覆盖 |
| 无Pawn / 黑屏 | Entry invalid.Matches、GameMode Override、两个标记 / 安全生成空间；不通过出生后传送掩盖标记错误 |
| 走入圆盘不激活 | Trigger的Pawn Overlap / Generate Overlap Events、玩家Pawn Object Type、Ready / Alive、地图登记；NPC不会被接受 |
| 重试出生正确但标签不ACTIVE | 一次Probe / Session确认前后日志、当前Tag / Label；不要加Tick轮询或Delay链 |
| 返回菜单Continue仍禁用 | Menu ready.Continue、已确认地图快照，HasContinueSnapshot不能含!bTravelPending；检查第3 / 6 / 7节替换是否完整 |
| 开始后后台旧ID未清 | NewRun requested.Entry、Spawn selected、Snapshot committed.Entry，再次Continue.Checkpoint；仅画面回入口不足以证明重置 |
| 缺失ID负例不起作用 | 是否当前正式地图PIE、命令类名BP_ETGameMode_C与Applied1、是否远离触发区避免正常重新激活；给Requested / Actual日志 |
| 继续后补跳 / 鼠标未捕获 | 按钮Down And Up、SetInputMode / Flush保留、Game input ready与操作顺序；只补当前输入边界 |

本批没有磁盘保存、已击败敌人恢复、奖励 / 持久化、真实加载故障专项、Cook / 包或性能测量。重复ID / 缺失Entry的代码保护存在，实机只指定缺失检查点ID负例；不要把未跑分支写成已测。所有新组件无常驻Tick，查询只在激活 / 出生选择，当前两个标记线性遍历足够；后续规模增大再考虑注册表，不为原型提前引入大型框架。

## 13. 本批需要Commit / Push的内容（固定放在文档最后）

历史C功能与资产已提交，D暂停迁移也已完成。当前第20批六份Source与七份Markdown共13项，统一按[Cleanup末尾](Cleanup.md)提交，不重复制造旧地图 / BP / 来源清单差异。

保存后在CLion刷新Diff，重新Stage最新修改，Commit / Push既有远端；[重新Stage步骤](M1_06B_FailureAndRetry.md#1411-clion重新stage最新修改简单步骤)继续保留。Raw、构建 / 测试报告、索引和个人设置留本地，助手不操作Git索引。
