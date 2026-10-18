# M1-03B2：事件驱动生命值HUD

2026-10-18清理同步：本页保留历史布局、接入差异和验收经验，HUD整文件以当前Source链接为准，不按旧步骤覆盖现有工程。当前功能基线已到M2-04B2，正在分批清理；B1冻结指南不改。本次验证和提交见[Cleanup](Cleanup.md)。

**2026-10-04已验收通过，手动60 / 等待5 min；清理后的正常保存已补核对。**结果见 [B2验收](Reports/M1_03B2_Verification.md)，下一步 [B3数据源重绑](M1_03B3_DataSourceRebinding.md)。下文保留当时交接方案，“未写Source”等表示交接时状态，不要求重做已通过项。

原方案日期：2026-10-04，原预计手动 **120–180分钟**，构建 / 索引等待另计。可分“代码与面板”“临时蓝图与验收”两次完成。

## 当前显示与验证边界（清理同步）

两个HUD仅共享[私有数值格式函数](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETResourceHUDText.h)：0位小数、随locale取整与分组，仍返回保留本地化历史的FText；调用方提供原LOCTEXT Pattern，不合并生命 / 体力的状态枚举、标题、日志级别或订阅。

四个BindWidget名称 / 类型、DisableNativeTick、Ready=0仍可显示、NoPawn / Waiting折叠数值和进度条、晚订阅快照与重建解绑保持。格式化不改变真实属性，也不重算比例。

新增[原生格式测试](../EmberTrial/Source/EmberTrialUI/Private/Tests/ETResourceHUDTextTests.cpp)覆盖en-US / de-DE / zh-Hans、取整 / 分组、调用方Pattern、FText跨locale重建与完整文化状态恢复。它不等于UMG布局、输入焦点或重绑定的实机验证；正常构建与真实资产结构校验另行记录，未执行不写通过。

## 1. 本批目标与边界

左上角新增一个生命值面板，显示“生命值 / 100 / 100 / 血条”；右上角保留原状态面板。生命值变化由Presenter事件推动，无UMG Text / Percent Binding，无Tick、轮询或Delay。无Pawn / 未就绪时隐藏数值与血条，显示原因；Ready且0生命值仍显示0/100和空条，死亡系统后续开发。

数据路线：`Character ASC → Health Presenter → Health HUD Widget`。Bootstrap持有唯一Presenter和当前Widget；Widget只弱引用Presenter，不获取ASC，不写角色属性，不重算属性规则。蓝图负责布局、Class配置和可删除的验证入口；C++负责订阅、快照、渲染与清理。

本批验证Widget晚加入立即快照、重建主动解绑、Presenter重启保留View订阅、无Pawn / Waiting、暂停和结束。真实新身体 / UnsupportedPawn / 旧ASC事件隔离留B3。沿用现有测试GE与引擎UMG控件，无新增付费美术。

另补齐B1源码的一处遗漏：`RebuildStatusWidget()` 同时检查 `bUIShuttingDown`，防止结束过程中重新创建状态面板；当前Source已包含。B1文档不修改。

## 2. 文件与操作顺序

| 操作 | 完整路径 |
| --- | --- |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETHealthHUDWidget.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETHealthHUDWidget.cpp` |
| 完整替换 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Bootstrap\ETUIBootstrapComponent.h` |
| 完整替换 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Bootstrap\ETUIBootstrapComponent.cpp` |
| 新UE资产 | `/Game/EmberTrial/UI/HUD/WBP_ETHealthHUD` |

1. 停止PIE。确认角色Class Defaults的Initial Health / Initial Max Health为100/100，保存全部资产并关闭UE。
2. 在CLion现有工程窗口中创建两份Widget文件，输入第3–4节；完整替换Bootstrap两文件为第5节。文件名不带U前缀，类名带U。
3. 第6节正常Editor Build / 索引 / Game Build通过后重开UE，进行第7节Designer与组件配置。
4. 在 **BP_ETPlayerController** 中添加第8节临时蓝图，不在角色或Widget里添加；随后按第9节逐组验收。
5. 第10节删除临时入口并回归，第11节反馈。不要删除旧状态Widget或Presenter实现。

UI Build.cs已具备公开UMG、私有EmberTrial / GameplayAbilities依赖，本批不修改依赖。不要手工创建`.generated.h`，不要生成第二个UIBootstrap或Presenter。

## 3. 新建 ETHealthHUDWidget.h

当前声明见下方Source链接。`generated.h` 必须是最后一个include；四个BindWidget名称与第7节大小写完全一致。

当前源码：[ETHealthHUDWidget.h](../EmberTrial/Source/EmberTrialUI/Public/Widgets/ETHealthHUDWidget.h)。

四个绑定控件、订阅入口与弱引用保持，具体职责见本页顶部当前边界。

`DisableNativeTick` 与蓝图的Tick Frequency=Never配套，避免前面章节遇到的Native Tick警告。Start/Stop由Bootstrap用C++调用，不提供蓝图第二套业务订阅入口。

## 4. 新建 ETHealthHUDWidget.cpp

当前源码：[ETHealthHUDWidget.cpp](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETHealthHUDWidget.cpp)。

Ready数值使用私有格式函数；其他状态、日志、比例与解绑时序保持。

数字暂时只显示整数，原始float与比例没有改写，后续产品规则若需要小数再调整展示。`SetPercent` 使用Presenter提供的0–1比例：[UE5.5 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/Components/UProgressBar/SetPercent?application_version=5.5)。只移除本Widget自己的委托，不对Presenter执行Clear或Shutdown。

## 5. Bootstrap 当前实现

### 5.1 ETUIBootstrapComponent.h

当前声明保留三类Widget配置及生命 / 体力观察接口，见下方Source。

当前源码：[ETUIBootstrapComponent.h](../EmberTrial/Source/EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h)。

### 5.2 ETUIBootstrapComponent.cpp

当前实现保留生命HUD层级4、体力3、状态5；层级在Bootstrap内命名，暂停 / 失败流程另有自己的层级。HUD不抢焦点、不设置输入模式。

当前源码：[ETUIBootstrapComponent.cpp](../EmberTrial/Source/EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp)。

移除顺序为“View主动解绑 → RemoveFromParent → Bootstrap释放引用”；即使临时变量保留旧Widget，它也不应收到后续数据。EndPlay先解绑View，再停止Presenter数据观察；NativeDestruct是幂等兜底。Presenter重启没有创建新Presenter / Widget，也没有重置生命值。

## 6. 正常构建与索引

仍保持UE关闭。PowerShell逐项执行，不使用Live Coding：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后CLion打开现有根工程，Reload CMake Project。确认新cpp属于 `EmberTrialUICodeModel`，对ProgressBar.h / ETHealthPresenter.h执行导航。刷新脚本含Editor Build，不用再重复构建Editor。

接着正常Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标成功后重开UE。若Build失败，先停在这里，把**首个error前后20行文字**与文件路径 / 行号发来；不要继续创建基于旧类的Widget。目录 / 窗口 / CMake操作仍见[CLion已有指南](CLion_UEClassCreation.md)。

## 7. Designer 与 Controller 配置

### 7.1 创建与父类

1. Content Browser创建文件夹 `EmberTrial/UI/HUD`。
2. 右键 → User Interface → Widget Blueprint，选择父类 **ETHealthHUDWidget**（展开All Classes搜索）。命名 **WBP_ETHealthHUD**。若先创建普通Widget，Class Settings → Parent Class改为ETHealthHUDWidget。
3. 确认Class Settings的Parent Class正确后进入Designer。先完成四个必须绑定控件，再Compile；创建到一半的缺失BindWidget错误不算最终结果。

### 7.2 控件层级与精确名称

按以下层级拖入，名称用Hierarchy选中控件后修改。四个标有 `Is Variable=True` 的控件都勾选变量；不是给文字内容起名。

```text
HUDCanvas                       Canvas Panel（根）
└─ HealthBorder                 Border
   └─ HealthBox                 Vertical Box
      ├─ HealthStatusText       Text Block，Is Variable=True
      └─ HealthContent          Vertical Box，Is Variable=True
         ├─ HealthValuesText    Text Block，Is Variable=True
         └─ BarSize             Size Box
            └─ HealthBar        Progress Bar，Is Variable=True
```

`HealthContent` 必须包含数值与条，不能包含标题。这样NoPawn / Waiting仅隐藏数值区，标题仍显示原因。每个Border / SizeBox只放一个子控件。

### 7.3 逐项属性

| 选中对象 | 属性与数值 |
| --- | --- |
| HealthBorder → Slot(Canvas Panel Slot) | Anchors Min=(0,0), Max=(0,0)；Alignment=(0,0)；Position X=24,Y=24；Size X=360,Y=124；Auto Size=False |
| HealthBorder | Padding左/上/右/下均12；Brush Color RGBA=(0.03,0.03,0.04,0.80)；Horizontal / Vertical Alignment=Fill |
| HealthStatusText | Text预览“生命值”；Font Size=18；Color白色；Auto Wrap Text=True；Slot(Vertical Box) Size=Auto、Horizontal Alignment=Fill、Padding Bottom=6 |
| HealthContent | Slot(Vertical Box) Size=Auto、Horizontal Alignment=Fill；Visibility默认Visible，运行时C++设置 |
| HealthValuesText | Text预览“100 / 100”；Font Size=24；Color白色；Slot Size=Auto、Horizontal Alignment=Fill |
| BarSize | Height Override勾选=20；Slot Size=Auto、Horizontal Alignment=Fill、Padding Top=6 |
| HealthBar | Percent=1.0；Bar Fill Type=Left to Right；Bar Fill Style=Scale；Is Marquee=False；Fill Color RGBA=(0.70,0.16,0.08,1)；无需图片 |
| Widget Class Defaults | Is Focusable=False；Tick Frequency=Never；Visibility=Not Hit-Testable(Self & All Children) |

字体复用旧状态面板当前能显示中文的字体。各控件保持默认Render Transform，不手动添加Scale修正；先在常用视口尺寸预览左上角位置。Border变大 / 小只影响排版，不改变生命数据。

**逐项检查：**四个精确名称与类型 / Is Variable；HealthContent层级；Text与Percent右侧没有Bind；Class父类；Never / 不可聚焦。Widget Graph不添加Event Tick、Construct业务逻辑、Set Percent / Set Text或输入模式节点。Compile应无Missing BindWidget和Native Tick警告；保存。原生构造 / 析构生命周期参考[UE5.5 UUserWidget](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/UMG/Blueprint/UUserWidget?application_version=5.5)。

### 7.4 装配

1. 打开 **BP_ETPlayerController**，Components选中已有的唯一 **UIBootstrap**。
2. Details → EmberTrial / UI：**Status Widget Class**仍为WBP_ETLifecycleStatus；新增 **Health Widget Class**选 **WBP_ETHealthHUD**。
3. 不再Add Component，不在Event BeginPlay创建Widget；原生Bootstrap负责创建和加入屏幕。
4. Compile / Save。打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`，确认沿用已验收的GameMode Override。先Play观察左右两个面板，Stop后再添加临时验收入口。

首次构造可出现一条Unavailable / ConstructSnapshot，随后同帧订阅快照或Ready事件给出实际状态。这条初始化日志不属于第二份UI；持久显示Unavailable才需要排查。

## 8. 临时蓝图：逐节点连接

### 8.1 共用约定与变量

**全部操作在BP_ETPlayerController**，打开Event Graph和My Blueprint。`Self` 指Controller；UIBootstrap的方法Target接组件引用；角色的方法Target接Cast成功输出。这三个对象不可混用。

添加两项临时变量：

| 名称 | 类型 | 默认值 / 用途 |
| --- | --- | --- |
| ETTempHealthPawn | Pawn **Object Reference**，蓝色对象 | None；保存UnPossess前的身体 |
| ETTempHeldHealthWidget | ETHealthHUDWidget **Object Reference**，蓝色对象 | None；强持有最近一次移除的旧Widget |

类型下拉选择Object Reference，不是Class Reference；不要勾Instance Editable / Expose on Spawn。变量创建后Compile一次再看默认值。

Event Graph右键搜索顶部键盘 **4、5、6、7、8、9、J、K** 的键事件（不是数字小键盘）。每个事件选中后Details：Consume Input=True，Execute When Paused=False。仅连 **Released**；Pressed不接。测试时完整按下并松开，长按不反复触发。

从Components将 **UIBootstrap拖入Graph → Get**，蓝色输出供8 / 9使用。它是组件实例引用，不是Get Class / Construct Object。保留原1/2/3、E/P。图里有旧4–9节点时先确认是先前已要求删除的临时节点，避免同键两套入口。

所有Print String：Print to Log=True、Print to Screen=True、Duration=2。下面的“结束”指该Print的白色输出不再连接其他业务节点；函数无返回值可留空退出，不需要接回成功分支。带执行引脚的 **Is Valid** 有Is Valid / Is Not Valid两个出口，本章不要选只有布尔返回值的纯版本。

### 8.2 临时函数 ETTempApplyHealthEffect

My Blueprint → Functions点`+`，命名 **ETTempApplyHealthEffect**。函数Details输入添加：

| 输入 | 类型 |
| --- | --- |
| EffectClass | GameplayEffect **Class Reference**，紫色类引脚 |
| CaseLabel | String |

不开Pure，无输出。进入函数图，按以下顺序放节点并可加注释A1–A8：

| 编号 | 节点 | 放置 / 搜索方式 |
| --- | --- | --- |
| A1 | Branch（暂停检查） | 右键Branch；另放Is Game Paused纯查询 |
| A2 | Is Valid（当前Pawn） | 另放Get Controlled Pawn，Target=Self；从其输出创建带执行线Is Valid |
| A3 | Cast To BP_ThirdPersonCharacter | 从当前Pawn输出拖线搜索Cast，保留执行引脚，不Convert to Pure Cast |
| A4 | Branch（Ready检查） | 从As BP Third Person Character拖线找Is Gameplay Ready，再接Branch |
| A5 | Get Ability System Component | 从Cast成功对象拖线搜索，选Ability System Blueprint Library的Actor查询版本 |
| A6 | Is Valid（ASC） | 从A5返回对象拖线建立带执行线版本 |
| A7 | Make Effect Context | 从A5返回ASC拖线搜索；Target为Ability System Component |
| A8 | Apply Gameplay Effect to Self | 从同一ASC拖线搜索，选择带Gameplay Effect Class / Level / Effect Context的ASC版本 |

**先连白色执行线：**

| 从 | 到 |
| --- | --- |
| 函数Entry执行输出 | A1执行输入 |
| A1 True | Print String：`[M1-03B2] Rejected paused`，然后结束 |
| A1 False | A2执行输入 |
| A2 Is Not Valid | Print String：`[M1-03B2] Rejected no Pawn`，然后结束 |
| A2 Is Valid | A3执行输入 |
| A3 Cast Failed | Print String：`[M1-03B2] Rejected unsupported Pawn`，然后结束 |
| A3成功白色出口 | A4执行输入 |
| A4 False | Print String：`[M1-03B2] Rejected not ready`，然后结束 |
| A4 True | A6执行输入 |
| A6 Is Not Valid | Print String：`[M1-03B2] Rejected missing ASC`，然后结束 |
| A6 Is Valid | A8执行输入 |
| A8执行输出 | Print String（下文格式化请求），然后结束 |

**再逐条连数据引脚：**

| 从 | 到 |
| --- | --- |
| Is Game Paused.Return Value | A1.Condition（暂停True拒绝） |
| Get Controlled Pawn.Return Value，Target=Self | A2.Input Object **和** A3.Object |
| A3.As BP Third Person Character | Is Gameplay Ready.Target **和** A5.Actor |
| Is Gameplay Ready.Return Value | A4.Condition（Ready True接受） |
| A5.Return Value（ASC） | A6.Input Object、A7.Target、A8.Target，三个都接同一个ASC |
| 函数Entry.EffectClass | A8.Gameplay Effect Class |
| A7.Return Value | A8.Effect Context |
| A8.Level | 直接输入1.0 |

A5是纯查询，没有白线。A7在本机通常也是纯节点；若实际显示白线，则把“A6有效 → A8”改为“A6有效 → A7 → A8”，其余数据连接不变。ASC查询的Actor**必须显式接Cast角色**，不留为Controller Self。

最后放Format Text，格式填写：

```text
[M1-03B2] Effect requested {Case}
```

函数输入CaseLabel接Format Text的Case；Format Text.Result接最后的Print String.In String，Text→String自动转换。Apply返回的Active Gameplay Effect Handle不接成功判断；Instant GE不保留持续效果句柄。本Print仅证明发起请求，实际成功以HUD数字和原生日志为准。

Compile函数 / 蓝图，确认拒绝分支都只Print结束，未通向Apply。没有Set Health / Clamp / Ensure / Tick / Delay / 更新Widget节点。

### 8.3 数字6 / 7 与 J / K：四个Effect入口

回Event Graph，拖My Blueprint的ETTempApplyHealthEffect进图选择Call，共四个调用。**每个调用Target=Self**，各键Released白线接对应调用执行输入，调用白色输出不再连接其他业务。

| 键事件 | EffectClass下拉选择 | CaseLabel输入 |
| --- | --- | --- |
| 6 Released | `/Game/EmberTrial/Tests/Effects/GE_ET_Test_Damage25` | D25 |
| 7 Released | `/Game/EmberTrial/Tests/Effects/GE_ET_Test_Heal40` | H40 |
| J Released | `/Game/EmberTrial/Tests/Effects/GE_ET_Test_Max60` | M60 |
| K Released | `/Game/EmberTrial/Tests/Effects/GE_ET_Test_Max120` | M120 |

Class选资源的类，不选None / GameplayEffect基类。四个Class分别为Health Add -25 / Health Add +40 / MaxHealth Override60 / MaxHealth Override120；本批不改GE资源。新的J/K仅为临时测试，不是最终游戏操作。

### 8.4 数字4：先保存当前Pawn，再UnPossess

放Get Controlled Pawn（Target=Self）、带白线Is Valid、Set ETTempHealthPawn、Un Possess、一个拒绝Print。

**白线：**4.Released → Is Valid执行输入；Is Valid有效出口 → Set ETTempHealthPawn → Un Possess；Is Not Valid → Print `[M1-03B2] No current Pawn; saved Pawn kept`，结束。

**数据线：**

| 引脚 | 设置 |
| --- | --- |
| Is Valid.Input Object | **Get Controlled Pawn.Return Value**，不要接Self或已保存变量 |
| Set ETTempHealthPawn.变量值输入 | 同一个Get Controlled Pawn.Return Value |
| Un Possess.Target | Self（Controller） |

无效分支不执行Set，所以重复按4不会把已保存身体覆盖成None。Set完成后才UnPossess，不能调换白线顺序。该操作不会销毁身体。

### 8.5 数字5：检查保存的Pawn，再防重复Possess

放Get ETTempHealthPawn、带白线Is Valid、Get Controlled Pawn（Target=Self）、Equal(Object)、Branch、Possess、两个Print。Equal(Object)可从两个Pawn对象引脚拖线搜索`==`。

**白线：**5.Released → Is Valid；无效 → Print `[M1-03B2] No saved Pawn`结束；有效 → Branch；Branch.True → Print `[M1-03B2] Already possessed`结束；Branch.False → Possess，结束。

**数据线：**

| 引脚 | 设置 |
| --- | --- |
| Is Valid.Input Object | **Get ETTempHealthPawn**，这次检查保存的身体 |
| Equal(Object).A | Get Controlled Pawn.Return Value |
| Equal(Object).B | Get ETTempHealthPawn |
| Branch.Condition | Equal(Object).Return Value |
| Possess.Target | Self（Controller） |
| Possess.In Pawn | Get ETTempHealthPawn |

相等True只Print；不相等False才Possess。不会创建新Pawn，不调用Ensure / 初始化生命值。5不能接4的“当前Pawn有效”条件，因为UnPossess后当前Pawn本来就是None。

### 8.6 数字8：保留旧Widget，再重建

目标是证明旧Widget**仍被强引用、没有被GC时也已解绑**。本节不要在移除后重新Get Health Widget来充当旧对象。

1. Components拖UIBootstrap进Graph选Get。
2. 从该组件输出拖出 **Get Health Widget**（纯节点）；其Target接UIBootstrap。
3. 拖ETTempHeldHealthWidget进图选Set，把第2步返回值接Set的变量值输入。
4. 从UIBootstrap输出拖出 **Rebuild Health Widget**，Target接组件。
5. 放Get ETTempHeldHealthWidget，用它创建带白线Is Valid。
6. 放Get Display Name，Object接**保存的ETTempHeldHealthWidget**。用Format Text格式 `[M1-03B2] Held old widget valid: {Name}`，Name接Display Name返回值，Result接有效分支Print.In String。
7. 无效分支另放Print，文字 `[M1-03B2] No held old widget`。

**全部白线顺序：**

```text
8.Released
  → Set ETTempHeldHealthWidget（先保存当时的Widget）
  → Rebuild Health Widget（Target=UIBootstrap）
  → Is Valid（Input Object=Get ETTempHeldHealthWidget）
      Is Valid     → Print“Held old widget valid: 名称” → 结束
      Is Not Valid → Print“No held old widget” → 结束
```

数据检查：Get Health Widget.Target与Rebuild Health Widget.Target均是**UIBootstrap**；Is Valid.Input Object与Get Display Name.Object均是**保存的旧Widget变量**。不在这个蓝图调用RemoveFromParent / StopObserving / CreateWidget，新原生逻辑负责它们。

每次8覆盖一个旧引用，不会累积保留十份Widget。前一份释放不影响本次最新旧Widget仍有效的证明。正式实现不保留此临时强引用。

### 8.7 数字9：保持同一Presenter与View，重启数据观察

Components拖UIBootstrap进Graph选Get，从输出创建 **Restart Health Observation**，Target接UIBootstrap。白线 **9.Released → Restart Health Observation → Print String**，Print文字 `[M1-03B2] Restart observation requested`，之后结束。

不接Rebuild Health Widget / NewObject / Ensure。这个入口只刷新Presenter对数据源的订阅；它发布StartSnapshot，原Widget仍应通过OnViewDataChanged接收。若该节点Target显示Self并报类型不兼容，删掉节点后从组件输出重新创建。

### 8.8 开始验收前的连接自查

| 检查 | 应当看到 |
| --- | --- |
| 4的Is Valid | Input Object接当前Get Controlled Pawn返回值 |
| 5的Is Valid | Input Object接保存的ETTempHealthPawn |
| 8的Is Valid | Input Object接保存的ETTempHeldHealthWidget |
| 四个GE调用 | Target=Self；EffectClass不是None；只有Released入口 |
| 函数Ready / 暂停 | Ready True去ASC；暂停True拒绝；全部失败出口结束 |
| ASC相关节点 | Actor=Cast身体；Make Context / Apply Target=同一ASC |
| 8 / 9 | 方法Target=UIBootstrap；8保存发生在重建前；9不重建 |

Controller Compile / Save；若报类型错误，先看Target / Object / Class Reference类型，不添加Cast掩盖错误连接。下面每组独立测试，发现失败停在该行反馈，不用反复重做已通过的其他组。

## 9. 分组验收：准备 → 操作 → 预期 → 清理

统一在L_ET_VerticalSlice，单玩家PIE。按键均完整按下 / 松开；每步等HUD更新后再下一步。Output Log过滤 `[M1-03B` 看Presenter / View，必要时清过滤看A2初始化与Warning。原生日志与BP的“Effect requested”不是同一事件。

除9.7特意非法配置外，每组开始前角色配置100/100；写“新PIE”的组必须先Stop再Play，不能带上一组的伤害或上限进入。每组最后Stop，不在PIE内改Class Defaults。

### 9.1 出生与面板装配

**准备：**完成第7节配置，Controller已Compile；新PIE。

**操作：**看左上角生命HUD、右上角原状态面板；移动 / 转向一次。

**画面预期：**只有一份生命HUD，文字生命值、100 / 100，血条全满；与右上角面板无重叠。中文正常，鼠标 / 镜头未被生命HUD抢走。

**日志预期：**一组View subscribed和Health widget created；最终View为Ready100/100、Fraction=1。可以先有Unavailable / ConstructSnapshot，以及Waiting / NoPawn后Ready；也可SubscribeSnapshot直接Ready。最终稳定状态必须正确，没有BindWidget / Native Tick / Accessed None错误。

**清理：**Stop。没有临时配置需要恢复。

### 9.2 扣血、治疗、零生命值与不变去重

**准备：**新PIE，确认100/100。

**操作与画面预期（同一轮按顺序）：**

| 顺序 | 操作 | 数字 / 血条 |
| ---: | --- | --- |
| 1 | 6一次 | 75/100，约3/4 |
| 2 | 7一次 | 100/100，全满 |
| 3 | 7再一次 | 仍100/100，全满 |
| 4 | 6四次，每次等更新 | 75→50→25→0 /100；最终空条，标题仍生命值，数字0/100可见 |
| 5 | 6再一次 | 仍0/100，无负值 |
| 6 | 7一次 | 40/100，约40% |

**日志预期：**每次真正变化由Presenter变化Snapshot和当前Widget的ViewDataChanged推动。步骤3 / 5可以有BP请求Print，但无新数值变化快照 / ViewDataChanged；不把查询或请求Print当重复订阅。0/100仍State=Ready，不是NoPawn / Waiting，没有死亡或重生行为。

**清理：**Stop，下一组新PIE恢复100/100。

### 9.3 最大值变化与比例

**准备：**新PIE100/100。

| 顺序 | 操作 | 数字 / 比例预期 |
| ---: | --- | --- |
| 1 | 6一次 | 75/100，0.750 |
| 2 | J一次 | 60/60，全满，1.000 |
| 3 | K一次 | 60/120，半条，0.500；不会自动治疗 |
| 4 | 7一次 | 100/120，约83%，日志0.833 |
| 5 | 7再一次 | 120/120，全满 |

**日志预期：**当前Widget接收变化，最终View里的Health / MaxHealth / Fraction与表一致。最大值下降涉及两项属性变化，可能有多次原生回调；本组不要求整个过程只有一条日志，以最后一致快照为准。不要用BP重算比例或直接Set Percent补救。

**清理：**Stop。无需修改测试GE默认值，角色配置仍100/100。

### 9.4 晚加入快照与旧Widget存活时解绑

**准备：**新PIE100/100，6一次到75/100；在当前View日志中记下旧Widget名字。

**操作：**按8一次，不额外扣血 / 治疗；随后按6一次；再按8三次，每次等日志输出。

**画面预期：**第一次重建立即显示75/100、3/4条，不必等下一次Effect。随后6显示50/100；后三次重建一直立即显示50/100，屏幕始终一份生命HUD。

**日志预期：**

1. 第一次8出现旧Widget unsubscribed / removed、新Widget subscribed / created；新名字与旧名字不同。
2. 新Widget `Reason=SubscribeSnapshot` 为75/100。没有新的Health initialized / Ready=1，Presenter名字不变。
3. BP输出 `Held old widget valid: 旧名字`，确认最近移除的旧对象仍有效。
4. 此后6只有**当前新Widget**一条Health=50、Reason=ViewDataChanged；该旧名字解绑后没有这条更新。后续8同样核对最新被移除的名字。

ConstructSnapshot初始化日志、重建时的SubscribeSnapshot均不能算属性重复更新。关键是在**最后一次重建完成后的一次真正扣血**中只当前Widget接收；若旧对象仍有ViewDataChanged，解绑失败，停止反馈。

**清理：**Stop；临时变量随该Controller结束释放。后面第10节还要删除该变量。

### 9.5 重启Presenter观察不丢失View订阅

**准备：**新PIE100/100，6一次到75；记当前Presenter / Widget名字。

**操作：**9三次，再6一次。

**画面预期：**9期间始终75/100；随后6立即50/100，不需要重建Widget。

**日志预期：**三次StartSnapshot仍同一个Presenter；当前Widget同一个名字，通过ViewDataChanged收到75/100。没有新的Widget created / Health initialized / Ready=1。最后6只有一条实际变化Snapshot及该Widget一条50/100 ViewDataChanged。

**清理：**Stop。

### 9.6 无Pawn、无Pawn下重建、恢复同一身体

**准备：**新PIE，6一次到75/100。

**操作：**4一次 → 8一次 → 5一次 → 5重复一次；再做三轮4→5；最后确认移动 / 转向 / 跳跃。

**画面预期：**4后标题“暂无受控角色”，数值与血条隐藏，不残留75/100，也不显示0/0。无Pawn时8重建同样显示该标题；5恢复同一身体，立即Ready75/100、3/4条。三轮往返始终保持75，无重置；重复5不干扰控制。

**日志预期：**NoPawn View快照；8的新Widget.SubscribeSnapshot为NoPawn；5恢复Ready75/100。同一身体名字保持；重复5只有Already possessed Print，无额外Possess / Ready变化。NoPawn下8的旧对象依然主动解绑。

**清理：**必须先5恢复原身体再Stop；不销毁角色、不添加新的Pawn。

### 9.7 未就绪状态与测试后恢复

**准备：**Stop，打开BP_ThirdPersonCharacter Class Defaults，临时 **Initial Health=100 / Initial Max Health=60**；Compile / Save；新PIE。

**操作：**各按一次6、7、J、K；9三次；8一次。

**画面预期：**标题“角色准备中”，数值与条隐藏，不把未就绪呈现为Ready的0血量。四个Effect入口拒绝，9不初始化；8重建仍从快照立即显示角色准备中。

**日志预期：**A2非法配置Warning是预期；Presenter / Widget为WaitingForGameplayReady；四次Rejected not ready；没有Effect applied / Health initialized / Ready=1。重建SubscribeSnapshot仍Waiting；无Accessed None。本文BP只Print请求，所以判断是否应用应同时看数值 / A2原生记录。

**清理必须完成：**Stop，把两项配置恢复 **100 / 100**，Compile / Save，再Play确认出生Ready100/100后Stop。没恢复不要进入下一组。

### 9.8 暂停焦点与临时输入不补执行

**准备：**新PIE100/100，6一次到75/100。

**操作：**分三轮开关菜单：

1. P打开；菜单中完整按下 / 松开6、7、8、9、J、K；鼠标点继续。
2. P再打开；重复这些临时键；用Enter或Space继续。
3. P再打开；点击菜单背景；重复这些临时键；P关闭。

**画面预期：**原菜单在HUD上层，鼠标与键盘继续有效；三轮恢复都75/100，没有后台治疗、重建或补执行。关闭后无需点视口即可移动 / 转向；按住移动键暂停、菜单中松开再继续不卡键，E不补交互。

**日志预期：**菜单内和恢复后无这些按键触发的新Effect requested / Health widget created / Restart observation requested。仅由Pause原逻辑造成的日志不算失败。不要为了让Controller临时键在菜单内触发而开启Execute When Paused。

**清理：**关闭菜单再Stop。

### 9.9 重建十次与三轮PIE清理

**准备：**最终配置100/100；新PIE。

**操作：**8完整按下 / 松开十次，每次等面板显示；6一次；Stop。再Play / Stop两轮，合计三轮。

**画面预期：**十次重建始终一份HUD、100/100；6后75/100。后两轮出生各100/100，原状态面板仍一份，无旧值残留。

**日志预期：**每次移除旧Widget先unsubscribed，然后removed；新Widget立即快照。最后6只当前Widget收到一次75变化，不是十一个Widget。结束清理后该轮旧Widget不再有Ready / ViewDataChanged；下一轮使用新对象。正常Controller EndPlay路径中View先解绑再Presenter Shutdown；即使World结束的其他数据源通知更早到达，也不能在**View已解绑之后**继续更新。重复StopObserving不重复产生解绑日志，无Accessed None / 脚本错误。

**清理：**Stop，准备第10节删除临时入口。本组已覆盖三轮PIE，不额外要求重复完整测试。

## 10. 删除临时入口与原功能回归

停止PIE后在BP_ETPlayerController删除：

- 临时 **4–9、J、K** 键事件以及对应临时Cast / Print / Get / Set连线组。
- 函数 **ETTempApplyHealthEffect**。
- 变量 **ETTempHealthPawn、ETTempHeldHealthWidget**；若删除提示仍被使用，先找到并删除临时引用节点。

**保留：**唯一UIBootstrap、StatusWidgetClass / HealthWidgetClass配置、两个Widget资产、Presenter、本文四份C++文件、五个测试GE、已有Probe测试资产。不要删除旧E/P或1/2/3。

确认角色最终100/100、MaxFloor测试GE仍为0、没有把测试GE加进出生初始化。Controller / 玩家 / Widget Compile，Save All。新地图再做一轮短回归：移动 / 转向 / 跳跃，1/2/3与2→3，E-A/B及提示，P鼠标 / 键盘继续；HUD出生100/100，暂停后仍正确。结束后Save All。

本批不重新打包、不加镜像布局的自动化测试；需要的是双目标编译与上述真实委托 / 存活旧Widget / UI输入边界验证。正式受击伤害、死亡 / 重生与性能Trace在后续阶段，不将练习3键直接当真实伤害。

## 11. 反馈与报错材料

完成后可复制填写，**保留失败项及实际值**，无需整理整份日志，我按完成时间读取本地文件：

```text
M1-03B2 完成 / 卡在第 __ 节
Widget两文件 / Bootstrap两文件，结束保护已补：是
关闭UE后Editor Build / 索引刷新 / Game Build：成功
UI目标 / ProgressBar与Presenter头导航：通过
WBP父类 / 四BindWidget名称类型 / 无Bind / Never / Compile：正确
单个UIBootstrap，两个Widget Class配置：正确
9.1 出生100/100、左右面板与中文：通过
9.2 扣血 / 治疗 / 零值 / 不变去重：通过
9.3 最大值下降与上升 / 比例：通过
9.4 晚加入75快照；旧Widget仍有效但无后续事件：通过
9.5 重启观察三次对象不换 / 属性不重置 / 单次变化：通过
9.6 NoPawn隐藏 / 空Pawn重建 / 三轮恢复同一身体：通过
9.7 非法配置Waiting / Effect拒绝 / 已恢复100/100：通过
9.8 暂停鼠标 / 键盘 / 背景继续、临时键无补执行：通过
9.9 十次重建 / 三轮PIE / 清理后无旧View事件：通过
临时按键 / 函数 / 两变量已删除；Compile / Save：是
最终原1/2/3、2→3、E/P与基础输入：正常
源码 / 资产已保存，临时配置恢复：是
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
```

| 问题 | 发给我的材料 |
| --- | --- |
| C++ Build / UHT失败 | 首个error前后20行**文字**、文件完整路径 / 行号、是Editor还是Game目标；不只发最后exit code 6 |
| Blueprint Compile错误 | Compiler Results完整文字；涉及控件时另附Hierarchy与Parent Class截图 |
| 找不到节点 / 类型不符 / Is Valid疑问 | 所在章节、节点名；整段蓝图截图包含白线、数据线与Target / Input Object，不只截节点标题 |
| HUD不显示 / 不更新 | 当前Class配置、最小复现顺序、预期 / 实际数字；Output Log中同一时段B1 Snapshot与B2 View文字；必要时Designer层级截图 |
| 旧Widget重复更新 | 最后一次8与随后一次6附近日志；指出旧 / 新Widget名字及Held old widget valid记录 |
| 暂停 / 时序问题 | 精确按键顺序与是否松开，10–20秒视频可补充；不强制录整组 |

先描述失败发生在哪一步即可，不必先判断原因。我可读取本地文件定位，给出修正与复验；只有你明确要求直接操作，才接管C++或编辑器。下一批B3继续验证真实新身体、未支持Pawn与旧数据源隔离。
