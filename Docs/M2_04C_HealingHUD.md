# M2-04C：事件驱动治疗次数HUD

2026-10-20，UE5.5.4。**本章与M2-04治疗整章均已验收，无待补项。**用户21:13全项完成，实现60 / 检查10 / 等待3分钟；证据与边界见[C验收报告](Reports/M2_04C_Verification.md)。下文保留结构 / Designer / 检查说明供复查，完整实现以链接的当前Source为准，无需重新录入或重做已通过检查。下一入口[M2-05A：韧性属性基础](M2_05A_PoiseFoundation.md)待手动实施。

## 1. 已交付行为与边界

左上角生命 / 体力面板下方新增“治疗次数（Q）”和整数`3 / 3`。正式Q在Notify成功治疗时更新为`2 / 3`；满血拒绝、提交前取消均不减次数。耗尽仍明确显示`0 / 3`与“治疗次数已用尽（Q）”。次数取自现有资源组件，UI不再保存另一份业务数值。

只增加展示链：`LocalPlayer当前Pawn → HealingResource事件 → HealingPresenter快照 → HealingHUDWidget`。不改治疗事务、能力、输入、动画、Tag或角色组件。现有恢复、连段、闪避与菜单继续保留。

| 工作 | 预计手动用时 | 说明 |
| --- | ---: | --- |
| 四个新文件、Bootstrap接入与理解 | 35–60分钟 | 保持清理后的命名、分区与所有权风格 |
| 新WBP与已有Controller配置 | 15–25分钟 | 无蓝图业务接线或新美术资产 |
| 相关检查 | 8–12分钟 | 两项现有自动化，四组短查；失败排查另计 |
| 合计安排 | 约70–110分钟 | 包含阅读 / 记录余量，构建等待另列，不计为实耗 |

实现后应能解释三点：资源和显示谁拥有数据；已从屏幕移除但未GC的Widget为何也要解绑；重新订阅为何不能重新初始化次数。当前Development包`Cleanup/Attempt01`不包含本章新HUD，完成后先在编辑器验证，阶段打包再更新包体。

### 1.1 显示状态与边界

| Presenter状态 | 显示 | 数据意义 |
| --- | --- | --- |
| Ready，有剩余 | 治疗次数（Q），整数当前 / 上限 | 读到当前身体的有效资源快照 |
| Ready，剩余0 | 治疗次数已用尽（Q），0 / 上限 | 有效耗尽，不是Waiting，也不补满 |
| NoPawn | 暂无受控角色；隐藏数值容器 | 当前没有有效受控身体 |
| UnsupportedPawn | 当前角色不提供治疗次数；隐藏数值 | 非EmberTrialCharacter或非Player阵营 |
| WaitingForGameplayReady | 角色准备中；隐藏数值 | 玩家身体存在但尚未Ready |
| Unavailable | 治疗次数暂不可用；隐藏数值 | 数据源不可用、组件失活或快照非法 |

Ready不等于“现在按Q一定能治疗”：满血、忙碌、死亡等仍由既有能力检查。死亡时次数本身可以仍有效，失败页负责输入与遮挡；Retry / Continue重建世界后，新身体重新3/3。Pause、重建View、重启观察与组件Deactivate→Activate均保留同一身体次数。

### 1.2 当前与预计规模

本章完成后实际**120份Source / 63cpp**，CodeModel Core43 / UI17 / Editor3，来源64项。下文代码路径对应当前已实现文件；M2-05A预计123 / 65仅为后续计划，不能混入C的实物计数。

## 2. 文件与操作顺序

1. Stop PIE → Save All，关闭UE编辑器。CLion保存现有文件；新反射类型使用正常Editor构建，不用Live Coding热补。
2. 手动创建以下四个文件，`Public` / `Private`及复数`Presenters` / `Widgets`拼写保持一致。可以在CLion文件树创建普通文件，无需让UE向导另建错误模块的类。
3. 按第3、4节输入四份完整代码，再按第5节只增补既有Bootstrap。不要把四份代码都粘到同一个文件。
4. 第6节Build与索引通过后，再打开UE完成第7、8节资产配置，最后执行第9节短查。

| 类型 | 相对于仓库根目录的精确路径 |
| --- | --- |
| 新 | `EmberTrial/Source/EmberTrialUI/Public/Presenters/ETHealingPresenter.h` |
| 新 | `EmberTrial/Source/EmberTrialUI/Private/Presenters/ETHealingPresenter.cpp` |
| 新 | `EmberTrial/Source/EmberTrialUI/Public/Widgets/ETHealingHUDWidget.h` |
| 新 | `EmberTrial/Source/EmberTrialUI/Private/Widgets/ETHealingHUDWidget.cpp` |
| 修改既有 | `EmberTrial/Source/EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h` |
| 修改既有 | `EmberTrial/Source/EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp` |

现有UI模块依赖已包含Core玩法模块、UMG等所需项。本章不新增Build.cs / Target / .uproject或Editor校验器修改；不在Character再挂一个HealingResource。

## 3. 只读Presenter：当前实现

- [ETHealingPresenter.h](../EmberTrial/Source/EmberTrialUI/Public/Presenters/ETHealingPresenter.h)
- [ETHealingPresenter.cpp](../EmberTrial/Source/EmberTrialUI/Private/Presenters/ETHealingPresenter.cpp)

资源OnSnapshotChanged是原生无参事件，Presenter重新读取当前快照，弱来源与观察代次拒绝旧组件 / 旧一轮回调。订阅不以当前快照可用为前提，因此失活时重启观察仍能收到后续Activate通知。0次仍Ready，快照只含整数与名称。

StartObserving先清旧数据源，保留View对同一Presenter的订阅，再绑定当前LocalPlayer / Pawn / Ready / 资源；每次绑定后立即读快照。Shutdown与BeginDestroy精确解绑，不清空别人的共享事件。重复源码已在验收后归并到当前Source，不从历史指南覆盖最新文件。

## 4. 只读Widget：当前实现

- [ETHealingHUDWidget.h](../EmberTrial/Source/EmberTrialUI/Public/Widgets/ETHealingHUDWidget.h)
- [ETHealingHUDWidget.cpp](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETHealingHUDWidget.cpp)

只把DTO转换为整数Text与容器Visibility；不读ASC、不治疗、不管理输入。订阅后即时读缓存，晚加入正确显示2/3；StopObserving先封口再解绑，NativeDestruct兜底。DisableNativeTick与蓝图Never配套。

三个BindWidget为HealingStatusText / HealingValuesText（TextBlock）与HealingContent（VerticalBox）。标题在数值容器之外；Unavailable隐藏数值时保留原因。不可改成Optional压掉结构错误，第7节保留详细Designer步骤。

## 5. UIBootstrap：当前实现与所有权

- [ETUIBootstrapComponent.h](../EmberTrial/Source/EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h)
- [ETUIBootstrapComponent.cpp](../EmberTrial/Source/EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp)

Bootstrap持有HealingPresenter / HealingWidget与可配置HealingWidgetClass，BeginPlay先开始数据观察、再创建View。RebuildHealingWidget先退订 / 移除旧View再创建，不新建Presenter或修改次数；RestartHealingObservation重用同一Presenter重新绑定当前身体。

四类View共用本地Controller / LocalPlayer解析，HUD ZOrder分别状态5 / 生命4 / 体力3 / 次数2。EndPlay先置bUIShuttingDown、移除全部View，最后Shutdown Presenter，保留既有状态 / 生命 / 体力与失败流程。历史增补锚点已完成，后续不要重复添加字段或函数。

## 6. 正常构建、索引与相关自动化

### 6.1 关闭UE后Editor Build与CLion索引

先确认四新 / 两旧文件全部保存，UE已关闭。在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先正常构建Editor，再更新CodeModel。成功后在已有CLion工程Reload CMake，不打开新文件夹工作区、不手改生成CMakeLists。预期UI CodeModel由15变17cpp，Core43 / Editor3保持；新文件属于EmberTrialUICodeModel。Ctrl+B在Presenter中检查资源头，在Widget中检查TextBlock头可导航。

### 6.2 Game Development Build

Editor / 索引成功后，保持UE关闭执行：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两个目标均成功才打开UE。若失败，发第一条真正的C++ / UHT错误及上下约20行；`UBT failed with exit code 6`本身不能定位原因。不要先删缓存、删除Tests或改旧测试断言。

### 6.3 两项相关生产者回归，各跑一次

本章复用已有`EmberTrial.M2.Healing.Resource`与`EmberTrial.M2.Healing.Ability`，确认新接入没有破坏原治疗生产者。在非PIE状态的Output Log逐条输入下列命令，第一项完成后再执行下一项：

```text
Automation RunTests EmberTrial.M2.Healing.Resource
Automation RunTests EmberTrial.M2.Healing.Ability
```

两项应各有一次最终Success。它们覆盖资源与能力，不自动证明新Presenter / UMG / 真实键鼠通过；这些由第9节补齐。本章不新增整数getter镜像测试、不重跑未变Combo / Dodge全矩阵，也不要求重新打包。

## 7. 新建WBP_ETHealingHUD：逐项配置

### 7.1 创建与父类

1. 在Content Browser打开`/Game/EmberTrial/ui/HUD`，从空白新建Widget Blueprint，命名`WBP_ETHealingHUD`。不复制生命 / 体力WBP，避免携带旧ProgressBar或旧BindWidget。
2. 如果创建窗口可选父类，选`ETHealingHUDWidget`；如果先创建为UserWidget，打开后用File → Reparent Blueprint，搜索`ETHealingHUDWidget`并选择原生类。Class Settings中Parent Class最终必须是它。
3. 现在尚未布置三个必需控件时，Compile提示缺BindWidget是中间状态；按7.2完成后再Compile。不要改原生BindWidget来绕过结构检查。

### 7.2 Designer层级与命名

在Designer的Hierarchy建立下列结构；用Palette搜索类型后拖到**指定父节点**。若已有自动Canvas Panel，将它重命名作为根，不再嵌套第二个根Canvas。

```text
HealingHUDCanvas                 Canvas Panel，根
└─ HealingBorder                 Border，拖到根Canvas
   └─ HealingBox                 Vertical Box，Border唯一子控件
      ├─ HealingStatusText       Text，标题
      └─ HealingContent          Vertical Box，数值容器
         └─ HealingValuesText    Text，整数次数
```

选中每个控件，在Details顶部或Hierarchy重命名；名字填上面的标识符，**不是Content → Text中的显示文字**。三个必需字段逐个检查：

| Name，区分拼写 | Palette类型 / 实际类 | Is Variable |
| --- | --- | --- |
| HealingStatusText | Text / TextBlock | 勾选 |
| HealingValuesText | Text / TextBlock | 勾选 |
| HealingContent | Vertical Box / VerticalBox | 勾选 |

不要用Rich Text Block替代Text；不要把HealingContent建成Horizontal Box。标题和HealingContent是HealingBox的两个同级子项；HealingValuesText才位于HealingContent内部。

### 7.3 布局、文字与类属性

逐行点击对应控件后设置；没有看到Canvas Slot时先确认选中的是`HealingBorder`而非根Canvas或它的子项。

| 选中项 | Details具体配置 |
| --- | --- |
| HealingBorder → Slot (Canvas Panel Slot) | Anchors Min=(0,0), Max=(0,0)；Alignment=(0,0)；Position X=24,Y=296；Size X=360,Y=100；Auto Size=False |
| HealingBorder | Padding四边12；背景沿用原HUD深色（可用R/G/B=.03，A=.8）；不要更改Render Transform来代替Slot位置 |
| HealingBox | 默认Vertical Box，顺序标题在上、数值容器在下 |
| HealingStatusText | Content → Text填“治疗次数（Q）”；Font Size18，白色；Vertical Box Slot的Size=Auto、Horizontal Alignment=Fill、Padding Bottom=6 |
| HealingContent | Vertical Box Slot Size=Auto、Horizontal Alignment=Fill；Designer中Visibility=Visible |
| HealingValuesText | Content → Text预览填“3 / 3”；Font Size24，白色；Vertical Box Slot Size=Auto、Horizontal Alignment=Fill |

Y296对应现有体力面板Y160、高124后的12像素间隔。若实际预览仍挤到其他元素，先核对旧面板布局，再只调整本新Border的Canvas位置并在反馈中写实际值，不移动旧生命 / 体力面板。字体使用旧HUD已能显示中文的字体，Render Transform保持默认。

点击Class Defaults：

- Is Focusable=False。
- Tick Frequency=Never。原生已有DisableNativeTick；不应出现“may require a native tick”警告。
- Visibility=Not Hit-Testable (Self & All Children)，即HitTestInvisible；标题与数值显示不抢鼠标输入。

两个Text右侧没有Bind函数；本章不创建Text / Visibility Bind、不写Event Construct / Tick / SetText或输入节点。Designer预览的3/3是占位文字，运行时由C++立即覆盖。

### 7.4 Compile、Save与错误定位

Compile → Save。预期无Missing required widget binding、无Native Tick警告。若缺控件，依次检查Parent Class → Hierarchy Name → 类型 → Is Variable → Compile，不仅检查肉眼显示的文字。必要时截图包含Hierarchy、选中项Details与编译结果即可。

## 8. 在现有Controller装配与只读校验

1. 打开现有`BP_ETPlayerController`，在Components选择已有的、唯一的`UIBootstrap`。不要Add Component再挂一个。
2. Details搜索`Healing Widget Class`，设为新`WBP_ETHealingHUD`。它是组件属性，不是Class Defaults里的角色HealingResource配置。
3. 保持Status / Health / Stamina三个Widget Class原值；FailureFlow组件及Pause / Failure Class / Map配置不动。本章没有新Controller按键或函数。
4. Controller Compile → Save；新WBP也Save。新WBP资产实际路径为`/Game/EmberTrial/ui/HUD/WBP_ETHealingHUD`，原Controller路径保持原位。
5. Content Browser选新WBP → 右键Asset Actions → Validate Assets。预期本项目`[M1-07C1] Valid`、Issues=0。现有通用校验器会检查父类必需控件，无需新Editor代码或制作Probe。
6. 回到`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，不修改地图或GameMode，再开始第9节。

## 9. 四组短检查：约8–12分钟

按下面顺序保留同一轮身体的次数以减少准备。不搭临时蓝图、不重新造测试Pawn矩阵。可站在出生安全位置避开敌人，先测UI；第4组才完成失败流程。日志由我核对，你反馈通过 / 失败与用时即可，不必逐条复制长日志。

### 9.1 出生、正式Q与暂停

**准备：**正式地图Play，新身体生命 / 体力100，治疗默认3/3。

**操作：**
1. 观察左上角只有一份治疗面板，位于体力下方，中文正常，显示3 / 3；旧生命 / 体力正常。
2. 满血按一次Q：应拒绝，次数保持3 / 3。
3. 按开发损伤4一次，生命75；实际按Q一次。动作到既有Notify时生命回100、次数变2 / 3，按键当下不提前扣。长按不连续消费。
4. 按P暂停，Q不执行，次数保持2 / 3。用鼠标或键盘一种方式继续，未补播 / 未补扣；无需重跑旧完整输入矩阵。

**预期日志与画面：**Presenter出生Ready的Charges=3/3，成功消费后2/3；没有另一份Widget叠在原面板上。只要实际数值正确，不要求你抓准确毫秒或截取整段日志。

### 9.2 晚加入View、重启观察与去重

**准备：**保持同一轮2 / 3、退出暂停。以下命令必须在**正在运行的PIE游戏视口内**按`~`打开游戏控制台后执行（不是PowerShell；若键盘需Shift配合，用与以前打开控制台相同按键）。输入一条、Enter，再输入下一条；完成后再按`~`关闭控制台。

```text
ke ETUIBootstrapComponent RebuildHealingWidget
ke ETUIBootstrapComponent RebuildHealingWidget
ke ETUIBootstrapComponent RestartHealingObservation
ke ETUIBootstrapComponent RestartHealingObservation
```

`ke`按当前游戏World中的**类名**调用实例；类名不带U前缀，当前只有一份UIBootstrap。不填Widget递增实例名，不在无PIE的编辑器世界运行。命令若无匹配 / 无作用，先发控制台提示，不额外造蓝图调试入口。

**预期：**每次重建旧View先unsubscribed / removed，新View立即显示2 / 3；Widget后缀递增是新View实例，正常。Presenter不因View重建更换，资源不补满。重启观察可以再次发布一份Ready快照，不能把它误判为重复扣数。

关闭控制台，按4后Q成功一次：应变1 / 3。一次资源变化只有当前Presenter的一份变化快照，旧View已停订阅；`View`应用日志默认Verbose，不要求Display级别逐条显示。检查关键是画面正确、旧View明确退订、没有重复资源变化，不要求捕获GC时点。

### 9.3 暂不可用、失活时重启观察与恢复

**准备：**角色空闲且当前1 / 3；再打开PIE视口的游戏控制台。

**操作第一段：**逐条执行：

```text
ke ETHealingResourceComponent Deactivate
ke ETUIBootstrapComponent RestartHealingObservation
```

预期面板显示“治疗次数暂不可用”，数值容器隐藏；重启观察仍不可用，不能凭缓存假装Ready。这条组件命令会作用于当前World所有该类组件，现有其他身体也暂时失活，仅本地玩家提供本HUD数据；不修改任何资产默认配置。

**恢复：**继续执行，`false`是Activate的bReset参数：

```text
ke ETHealingResourceComponent Activate false
```

预期同一玩家面板恢复1 / 3。若重启观察时只在GetSnapshot成功后才绑定资源事件，这一步将恢复不了；因此这是本章新增的必要边界。恢复命令必须执行，不把组件留在临时失活状态。关闭控制台，其他面板和输入保持正常。

### 9.4 耗尽、世界重建与最终短基线

1. 同一身体按4损伤，再Q成功：显示0 / 3与“治疗次数已用尽（Q）”。再按4后Q应拒绝，生命不增加、仍0 / 3；不隐藏成Waiting，不影响攻击或闪避。
2. 用现有4继续真实扣到死亡（或被敌人击杀）；Failure点重试，**新身体**治疗3 / 3、生命与体力100，只有一份各类HUD。
3. P → 返回主菜单 → 继续本次试炼：新世界、检查点规则沿用，新的治疗3 / 3；不要求保存旧次数。菜单本身无关卡HUD。
4. Stop PIE后再Play一次，出生3 / 3；短试左键、2、E / P与移动 / 转向 / 跳跃正常。无Accessed None或旧View继续收到资源事件。

**恢复 / 保存：**确认失活命令已反向恢复；不改3/3、Amount40、slomo1与原Debug=False，不创建临时地图对象。Stop PIE，Controller与WBP Compile / Save All，回正式地图。开发命令仅改运行实例，不写回资产。

NoPawn / Unsupported / 等待Ready的绑定分支本章做源码检查，沿用既有LocalPlayer / 世界切换基线；没有再人工搭该完整矩阵，不把它写成新实机全覆盖。若这些共享逻辑出现失败，再补对应窄项。

## 10. 完成反馈与交接

```text
M2-04C 完成
四新 / Bootstrap两旧Source；关闭UE后Editor Build / UI17索引 / Game Build：
新WBP父类 / 三BindWidget / 无Binding / Never；Controller唯一组件配置 / Valid：
Healing.Resource / Healing.Ability各一次：
出生3/3 / 正式Q延迟一次减数 / 满血拒绝 / P不补执行：
2/3晚加入重建 / 同Presenter重启观察 / 下一次仅一份变化：
Deactivate→不可用→重启观察→Activate false恢复原次数：
0/3有效显示与拒绝 / Retry与Continue新3/3 / Stop再开：
旧HUD / 左键-2-E-P与移动转向跳跃短基线：
Source / 资产Save All、临时状态已恢复；失败或最明显布局问题（无填无）：
完成时分（如21:18） / 手动代码与配置 / 检查 / 构建等待：
```

代码报错优先贴文字：第一条真实错误、文件行号与上下文；布局 / 控件绑定问题用Hierarchy与Details截图；输入与切图问题写复现顺序、当前状态和实际结果。无需为本章每个按钮录视频或逐条手抄日志。

本章反馈已核对并形成[C报告](Reports/M2_04C_Verification.md)，M2-04已关闭，无需重交本章模板。按既有约定直接下发[M2-05A](M2_05A_PoiseFoundation.md)；A尚未实施，继续由用户手动代码 / 配置 / 构建。

## 11. 本批 Commit / Push（固定放在最后）

当前HEAD仍b1fbb30，尚未Commit本章。已实现六Source、两资产与收尾 / 下一章文档可一起提交；助手未Stage / Commit / Push。

**必须重新Stage四份新Source：暂存区仍是0字节创建骨架，完整代码只在工作区。**在CLion刷新Changes，重新Stage下列文件，再看Staged Diff确认有完整实现，而不只看文件名已勾选：

```text
EmberTrial/Source/EmberTrialUI/Public/Presenters/ETHealingPresenter.h
EmberTrial/Source/EmberTrialUI/Private/Presenters/ETHealingPresenter.cpp
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETHealingHUDWidget.h
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETHealingHUDWidget.cpp
EmberTrial/Source/EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h
EmberTrial/Source/EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp
```

两项LFS资产：

```text
EmberTrial/Content/EmberTrial/ui/HUD/WBP_ETHealingHUD.uasset
EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset
```

本轮文档完整清单（实际已有15项差异；先前下发内容未提交可一起纳入）：

```text
Docs/Reports/M2_04C_Verification.md
Docs/M2_04C_HealingHUD.md
Docs/M2_04_HealingRoadmap.md
Docs/M2_Roadmap.md
Docs/M2_05_BreakPulseRoadmap.md
Docs/M2_05A_PoiseFoundation.md
Docs/AssetManifest.csv
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Docs/Cleanup.md
README.md
Introduction.md
```

M2-05A仅有文档 / 本地候选，实际A Source未写入工程，不存在待提交的A实现。其他Content / Config / Build.cs / Target不强制重存。建议本次Commit：

```text
feat: add event-driven healing charges HUD
```

用户检查Staged Diff → Commit → Push；Raw / 运行日志 / 生成CodeModel / Saved / Intermediate / Binaries与Windows包留本地，不Force Add。A实施后的独立提交清单在[A末尾](M2_05A_PoiseFoundation.md#11-本批-commit--push固定放在最后)。
