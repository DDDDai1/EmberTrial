# M1-06A：独立主菜单与战斗入口

2026-10-18清理同步：本页保留历史实施 / 配置 / 排障经验，当前整文件实现以Source链接为准，不按旧全文覆盖现有工程。当前已接菜单继续、原生暂停与检查点；这轮清理由助手按用户授权直接修改，普通功能开发的手动实践约定保留。验证与本批Push清单见[Cleanup](Cleanup.md)。

下发与验收：2026-10-10；UE **5.5.4**。前置[M1-05总验收](Reports/M1_05_Verification.md)通过，分批计划见[M1-06路线](M1_06_FlowRoadmap.md)。**A功能验收通过：用户21:13全项反馈、双目标Build / 29cpp索引、菜单切图与清理日志已核对。**六新Source / 两旧cpp、四新资产与启动 / Cook配置完成，清单44；实际手动45 / 等待5分钟。Unity补修已关闭；菜单空HUD初始化Warning已定位、非阻塞，已在后续B原生构建收口，见第11.1节与[验收报告](Reports/M1_06A_Verification.md)。以上是原A实施记录；当前清理的源码修改与验证以Cleanup为准。

原实施约定：C++由你手动写入；当前清理按本轮授权由助手直接修改。本文没有临时验收按键 / 函数，主要操作是新建文件、Designer和Class / 地图配置；Blueprint Event Graph本批留空。最后一节列出本批Commit / Push内容。

## 当前主菜单实现与职责

Controller拥有唯一菜单View和自己的Session失败订阅。CanHandleMenuAction集中结束 / pending / 本地 / 当前View判断；开始 / 继续仍额外要求World，退出保持原范围。ApplyMenuInputMode复用UIOnly、清键、鼠标与初始焦点，仍只在原两个合法调用点执行，不缓存身份。

层级100是原装配契约的内部命名常量；GameplayMap仍是可配置软引用，默认正式地图保留；显式设为None仍预检拒绝，不因C++默认值自动改图。Widget只呈现busy / Continue快照与发意图，按钮名、中文、焦点、反射字段和会话规则不改。当前具备“继续本次试炼”，A阶段的“继续禁用”只描述历史阶段。

## 1. 完成后看到什么、预计多久

启动菜单地图后，全屏显示“EmberTrial / 灰烬试炼”，下面有**开始试炼 / 继续试炼 / 退出游戏**。继续暂时禁用，说明当前尚无存档 / 继续功能；按开始载入已有正式战斗房间，角色 / 敌人初始100/100，HUD为新世界的玩家数据，鼠标隐藏，输入和视口焦点恢复。退出在PIE中结束当前Play，在独立游戏中退出进程。

预计手动**60–100分钟**：代码25–40、资产25–45、相关检查8–12分钟；构建 / 索引等待另计。菜单用UMG纯色与现有字体，不增加美术支出。失败界面、检查点、暂停菜单迁移在后续批次；A沿用当前P菜单和战斗实现。

| 分工 | 本批职责 |
| --- | --- |
| Core Menu GameMode | 当前菜单世界不生成Pawn / Spectator / HUD；BP装配UI Controller Class |
| UI MainMenu Controller | 创建 / 移除唯一菜单实例，按钮请求、切图门控、UI Only / 焦点和失败恢复 |
| UI MainMenu Widget | 绑定按钮、渲染状态、发送意图；不OpenLevel、不查ASC、不保存游戏 |
| 既有Game Controller | 新世界BeginPlay显式Game Only、隐藏鼠标、Flush；修复跨地图继承的UI输入状态 |
| Blueprint / 地图 / 配置 | 布局、类 / 软地图引用、GameMode Override、启动地图和Cook列表 |

## 2. 准备与完整文件范围

1. Stop PIE，确认当前正式身体100/100、伤害25 / 10、slomo1、Debug False已保持正常；不用重跑战斗矩阵。
2. UE中Save All并**关闭编辑器**。本批新增UCLASS与反射属性，用正常UHT / UBT构建，不靠Live Coding补类。
3. 在CLion当前EmberTrial工程窗口中创建下面六个文件，输入第3–5节。文件名不带A / U前缀，不手工创建`.generated.h`。
4. 第6节只替换既有游戏Controller.cpp，保留原.h与Blueprint父类；第7节双目标Build / 索引通过后再打开UE。
5. 第8–9节完成Designer、Class和地图 / 项目配置，再按第10节三组检查。完成后按第12节反馈，最后Commit / Push。

| 操作 | 完整路径 |
| --- | --- |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\ETMenuGameMode.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETMenuGameMode.cpp` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETMainMenuWidget.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETMainMenuWidget.cpp` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\FrontEnd\ETMainMenuController.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\FrontEnd\ETMainMenuController.cpp` |
| 替换.cpp全文 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETPlayerController.cpp` |
| 第7.1节Unity补修，四处标识符改名 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.cpp` |

**注意模块：**GameMode在`EmberTrial`，Menu Widget / Controller在`EmberTrialUI`，API宏分别为`EMBERTRIAL_API` / `EMBERTRIALUI_API`。本批直接在CLion对应文件夹手动建文件，避免UE类向导默认写进核心模块。现有两份Build.cs依赖足够，不修改Build.cs、Target或uproject。

## 3. Core：不生成身体的菜单GameMode

### 3.1 ETMenuGameMode.h全文

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ETMenuGameMode.generated.h"

class APlayerController;

UCLASS()
class EMBERTRIAL_API AETMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AETMenuGameMode();

protected:
    virtual void HandleStartingNewPlayer_Implementation(
        APlayerController* NewPlayer) override;
};
```

### 3.2 ETMenuGameMode.cpp全文

```cpp
#include "Framework/ETMenuGameMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMenuGameMode, Log, All);

AETMenuGameMode::AETMenuGameMode()
{
    DefaultPawnClass = nullptr;
    SpectatorClass = nullptr;
    HUDClass = nullptr;
}

void AETMenuGameMode::HandleStartingNewPlayer_Implementation(
    APlayerController* NewPlayer)
{
    // Menu players have no Pawn. Do not call the default RestartPlayer path.
    UE_LOG(LogETMenuGameMode, Display,
        TEXT("[M1-06A] Menu player ready Controller=%s Pawn=%s"),
        *GetNameSafe(NewPlayer),
        *GetNameSafe(IsValid(NewPlayer) ? NewPlayer->GetPawn() : nullptr));
}
```

不调用此函数的Super是有意设计：引擎默认实现会RestartPlayer并尝试生成身体。菜单只需要Controller与UMG。核心GameMode没有include UI类，UI Controller Class在菜单GameMode BP中指定。

## 4. UI：按钮与状态Widget

### 4.1 ETMainMenuWidget.h全文

当前源码：[ETMainMenuWidget.h](../EmberTrial/Source/EmberTrialUI/Public/Widgets/ETMainMenuWidget.h)。

### 4.2 ETMainMenuWidget.cpp全文

当前源码：[ETMainMenuWidget.cpp](../EmberTrial/Source/EmberTrialUI/Private/Widgets/ETMainMenuWidget.cpp)。

事件只在NativeOnInitialized绑定一次，ShowPending控制按钮可用性，Controller还会再次检查请求状态。Continue在A始终禁用，没有假存档值或空点击行为。C接入会话检查点后再更新资格与文案；UI结构不需要重建。

## 5. UI：独立主菜单Controller

### 5.1 ETMainMenuController.h全文

当前源码：[ETMainMenuController.h](../EmberTrial/Source/EmberTrialUI/Public/FrontEnd/ETMainMenuController.h)。

### 5.2 ETMainMenuController.cpp全文

当前源码：[ETMainMenuController.cpp](../EmberTrial/Source/EmberTrialUI/Private/FrontEnd/ETMainMenuController.cpp)。

地图是软引用，只用PackageName预检并发起OpenLevel，没有LoadSynchronous。pending先置True再切图，Widget也立即禁用按钮，防止同世界连续请求。EndPlay先设结束标记、解绑全局失败事件和Widget通知、再移除View。切图失败恢复只处理旧菜单Controller仍有效且收到本世界TravelFailure的情况；世界已被销毁的加载故障不能由这个旧Controller恢复，M1-07打包专项再验证实际包与失败路径。

不在菜单EndPlay对全局输入盲目复位：输入模式属于当前进入的页面 / Controller。战斗世界在下一节主动建立自己的Game Only状态；Stop PIE的视口交还由编辑器处理。

## 6. 游戏Controller：新世界主动恢复输入

只把`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETPlayerController.cpp`替换为下面全文，**原ETPlayerController.h不改**：

当前源码：[ETPlayerController.cpp](../EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp)。

新增代码只在本地游戏Controller BeginPlay建立输入策略。Game Only把焦点交给游戏视口、恢复鼠标捕获；`SetConsumeCaptureMouseDown(false)`让首次捕获的点击也能送入游戏，`FlushPressedKeys()`清掉控制器已有按键状态。这不取代现有暂停开 / 关的Flush，也不修改攻击 / 闪避 / 交互规则。

## 7. 正常Editor / Game Build与CLion索引（你执行）

1. 保存全部八个改动文件（包含第7.1节Unity补修），UE保持关闭。PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先正常构建Editor，再生成两个模块的CodeModel。CLion提示CMake变化时Reload；若没有提示，用Tools → CMake → Reload CMake Project。新的三个cpp纳入实际索引后预计**29工程cpp**（原26 + GameMode / Widget / Controller各1），不把头文件数算入cpp。

2. 再执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

3. 两次成功后再打开UE。CLion中点开新Widget / Controller cpp，对`Components/Button.h`、`Misc/PackageName.h`或`Engine/Engine.h`导航，确认文件属于正确模块目标。
4. 报错时优先给第一条真实编译 / UHT错误及前后10–20行，不只给“exit code 6”。不要删Intermediate或改全局CMake版本试错。新.h的generated include必须最后，cpp include必须使用表中目录路径。

本批不重复Cook / 打包。实施后预计Source59份 / cpp29份：六份新增、ETPlayerController.cpp输入初始化，以及下方旧追击Task的四处标识符补修；编译 / 索引数量以成功构建后核查为准。

### 7.1 本次C2374：Unity Build匿名命名空间冲突（用户手动补修）

已核对两个旧cpp的匿名命名空间都定义了`TargetActorKey` / `StopDistanceKey`，且本轮Editor生成的`Module.EmberTrial.cpp`包含两个文件。Unity Build把多个cpp放进同一个编译单元，此时匿名命名空间也属于同一单元，同名常量发生重定义。之前能编译并不保证它们一直合并在一起；分组 / Adaptive Unity工作集变化可能让这个旧命名问题暴露。

只改`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.cpp`，**ETAIController.cpp不改**：

1. 文件顶部namespace内两项定义改为：

```cpp
    const FName PursuitMoveTargetActorKey(TEXT("TargetActor"));
    const FName PursuitMoveStopDistanceKey(TEXT("PursuitStopDistance"));
```

2. 构造函数里引用改名后的完整相邻顺序必须是（2026-10-13补充防止默认黑板绑定丢失）：

```cpp
    BlackboardKey.SelectedKeyName = PursuitMoveTargetActorKey;
    AcceptableRadius = 150.0f;
    AcceptableRadius.SetKey(PursuitMoveStopDistanceKey);
```

仅上述**四处C++标识符**变化；`TEXT("TargetActor")`和`TEXT("PursuitStopDistance")`字符串保持原样，Blackboard资产 / BT配置不改。`AcceptableRadius = 150.0f`保留，且必须在SetKey之前：后赋float会复制None键、清掉绑定。不要因为示例只展示两项引用而把常量移到SetKey后；其余构造代码保持。实际半径读取Controller写入的黑板键，当前C2决策应为90。B补采已发现旧顺序回归，当前补修见[M1-07B第8.2节](M1_07B_InsightsBaseline.md#moveradius-fix)。

保存后、UE仍关闭，重新执行本节的`.\Tools\Refresh-CLion.ps1`；Editor / 索引成功再继续Game Build。不要修改生成的Module.EmberTrial.cpp、改成static、关闭Unity或清缓存：这次修复常量名字即可。补修尚未由用户执行 / Build通过，不提前登记为验收；不为纯标识符改名重做M1-05矩阵，A已有短基线覆盖正常追击。

## 8. UE：Widget布局和三个Class装配

### 8.1 创建WBP_ETMainMenu并配齐四个BindWidget

1. 内容浏览器进入已有`/Game/EmberTrial/ui/Menu`（Menu单数、ui小写），创建**User Widget Blueprint**，父类选`ETMainMenuWidget`，命名`WBP_ETMainMenu`。
2. 打开后先做完整Designer再最后Compile。父类有必需BindWidget；刚创建的空Widget自动编译可能暂报缺控件，四项配齐后最终Compile必须没有该警告。不要改成BindWidgetOptional隐藏装配错误。
3. 删除自动生成且不使用的多余根控件，按下面层级搭建；只有一个Canvas根。标题 / 按钮内文字名可自定，四个加粗控件名必须一致：

```text
Canvas Panel
└─ Border_Background
   └─ VerticalBox_Menu
      ├─ Text_Title：EmberTrial / 灰烬试炼
      ├─ Text_Subtitle：一段近战试炼
      ├─ StartButton（Button）
      │  └─ Text：开始试炼
      ├─ ContinueButton（Button）
      │  └─ Text：继续试炼
      ├─ QuitButton（Button）
      │  └─ Text：退出游戏
      └─ StatusText（Text Block）
```

4. 选Border_Background，在**Slot (Canvas Panel Slot)**设置：先设Anchors Min=(0,0)、Max=(1,1)，再把Offset Left / Top / Right / Bottom逐项设为0。切换锚点会为保持原尺寸自动改写Offsets，因此不能只改Anchors；最后必须核对四个Offset均为0。Brush Color=(0.025,0.030,0.040,1)，Visibility=Visible。背景不透出世界。
5. 选VerticalBox_Menu，在**Slot (Border Slot)**设置Horizontal Alignment=Center、Vertical Alignment=Center、Padding=48。不要在这里找Canvas位置或Anchors，它现在是Border的子控件。
6. Text_Title字号40，居中；Text_Subtitle字号18，居中，上下Padding各10。沿用现有能显示中文的字体设置 / 模板字体链，不从网页拷未经确认许可的字体。
7. StartButton / ContinueButton / QuitButton：按以下顺序设置，外边距和内容内边距分别选择不同控件：
   - 选**Button本体**，精确改名，**Is Variable=True、Is Focusable=True**；在**Slot (Vertical Box Slot)**中设**Size=Auto、Horizontal Alignment=Fill、Padding=(0,8,0,8)**。此处Size不要选Fill，菜单高度应由各行内容决定。
   - 选该Button里面的**Text子控件**，在**Slot (Button Slot) → Padding**设**Left=60、Top=14、Right=60、Bottom=14**，Slot的Horizontal / Vertical Alignment均选Center。UE 5.5.4这里不在Button本体下显示独立的“Button Content Padding”项。四值简写框可能显示为`60.0,14.0`，表示左右60、上下14；展开Padding可核对四项。
   - 仍选Text子控件，**Content → Text**分别填“开始试炼”“继续试炼”“退出游戏”；**Appearance → Font → Size=24、Justification=Center**。
   - 回到Button本体，Interaction的**Click Method / Press Method=Down and Up**；Touch Method保持Down and Up。
8. StatusText：精确改名，控件类型为**Text Block**，Is Variable=True；字号16、Justification=Center、Auto Wrap Text=True、Wrap Text At=480。在**Slot (Vertical Box Slot)**设Size=Auto、Horizontal Alignment=Fill、Padding=(0,20,0,0)，即只有上边距20。设计时可填“尚无存档，继续功能暂不可用。”；运行由C++更新，**不添加Text Binding**。
9. ContinueButton的Is Enabled在Designer设False（C++也会强制False）；Start / Quit保持True。按钮与背景Visibility保持Visible，不能设成Not Hit-Testable。
10. 进入Graph，点击顶部**Class Defaults**，在Details搜索`focusable`，勾选**Is Focusable=True**；再搜索`tick`，设**Tick Frequency=Never**。父类已有DisableNativeTick，不应出现“may require a native tick”警告。确认右上角Parent Class=ETMainMenuWidget，**本批Event Graph不添加Construct / OnClicked / Dispatcher连线**，C++已负责。UE自动生成的灰色、无连线Pre Construct / Construct / Tick事件不构成业务逻辑，可保持未连接。
11. Compile → 确认四个BindWidget均找到且无Native Tick警告 → Save。出错优先逐项看名字 / 类型 / Is Variable / 父类。

| 必须名称 | 控件类型 | 由谁写入 / 操作 |
| --- | --- | --- |
| StartButton | Button | C++绑定点击、启用状态与焦点 |
| ContinueButton | Button | A禁用；C再根据真实会话状态开放 |
| QuitButton | Button | C++绑定点击与启用状态 |
| StatusText | Text Block | C++SetText；无Binding |

#### 8.1.1 接管设置与问题记录（2026-10-10）

用户授权助手直接完成第7–11点。本次已在UE 5.5.4设置并正常保存`/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`；未执行菜单PIE、跨地图运行或本章整体验收。

| 发现的问题 | 原因 / 处理 | 当前状态 |
| --- | --- | --- |
| 文档要求找“Button Content Padding”，实际界面找不到 | 内容内边距位于Button内部Text的`Slot (Button Slot) → Padding`。已按60 / 14 / 60 / 14配置三个Text，并修正第7点 | 已解决 |
| Start / Continue行高度被压缩 | 两个Button的Vertical Box Slot原为Size=Fill；已改Auto，Quit原已是Auto。三者最终均Auto、H=Fill、外边距0 / 8 / 0 / 8 | 已解决 |
| Quit行缺少上下间距 | Quit的Vertical Box Slot Padding原为0；已改0 / 8 / 0 / 8 | 已解决 |
| 背景未铺满、菜单挤窄并停在左上方 | 前置第4点的Anchors正确，但Offset Right=1821.921875、Bottom=1051.081055，背景约100×30；已将右 / 下归零，四个Offset最终均0。菜单恢复居中，VBox的48边距和双向Center已核对 | 已解决；第4点补充先锚点后Offset的顺序 |
| 接管时标题 / 副标题仍是“Text Block” | 当时第6点文字尚未填写；不影响第7–11点编译，正式菜单展示需补齐 | 当时交回用户按下方步骤补填；随后Widget21:03:54.867再次保存，最终配置按用户完成反馈登记，本轮未重新查看标题画面 |

第7–11点结果：三个Button名称 / Is Variable / Is Focusable正确，文字24号且居中；Continue禁用，Start / Quit启用，三按钮和背景Visible。StatusText已设变量、16号、居中、自动换行480、上边距20及中文提示，无Text Binding。Class Defaults已设Focusable=True、Tick=Never；Graph只有UE默认的三个未连接事件，无点击业务连线。Compiler Results只有`Compile of WBP_ETMainMenu successful!`一条成功结果，未出现BindWidget缺失或Native Tick警告；21:00正常Save后页签无星号，编辑器显示All Saved。

**你接下来先补这两项文字（约1分钟），再进入8.2：**

1. Designer左侧Hierarchy选`Text_Title` → 右侧Details搜索`text` → 在**Content → Text**输入`EmberTrial / 灰烬试炼`。注意修改显示内容，不改控件名。
2. 选`Text_Subtitle` → 同一个**Content → Text**输入`一段近战试炼`。
3. Compile → Save。第7–11点已完成，不需要再重复配置。本次Widget和本文件的修正属于第13节本章Commit / Push清单。

### 8.2 创建独立菜单Controller BP

1. 进入`/Game/EmberTrial/Blueprints/Framework`，新建Blueprint Class → All Classes，搜索并选择原生**ETMainMenuController**，命名`BP_ETMainMenuController`。不Duplicate BP_ETPlayerController，不从玩家Character继承。
2. Class Defaults → EmberTrial / Menu：**Main Menu Widget Class=WBP_ETMainMenu**；**Gameplay Map=L_ET_VerticalSlice**（地图资源，不是Blueprint / Streaming Level）。软引用自动显示真实资产路径。
3. Components只保留原生默认组件，**不添加UIBootstrap / PracticeAction**；菜单不创建游戏HUD，也不观察ASC。
4. Event Graph留空，Compile → Save。保留既有BP_ETPlayerController与其单个UIBootstrap / 两个Widget Class。

### 8.3 创建独立菜单GameMode BP

1. 同目录新建Blueprint Class → All Classes → 原生**ETMenuGameMode**，命名`BP_ETMenuGameMode`。不Duplicate正式BP_ETGameMode。
2. Class Defaults → Classes：**Player Controller Class=BP_ETMainMenuController**。
3. **Default Pawn Class=None、Spectator Class=None、HUD Class=None**；保持Start Players As Spectators=False。其余GameState / PlayerState / GameSession默认不改。
4. Event Graph不实现Handle Starting New Player或Spawn Pawn；原生覆盖已负责不生成身体。Compile → Save。

**三个父类需要分别正确：**Widget→ETMainMenuWidget，Menu Controller→ETMainMenuController，Menu GameMode→ETMenuGameMode。菜单GameMode通过BP引用UI Controller，核心Build.cs不添加EmberTrialUI依赖。

## 9. UE：菜单地图、启动设置和Cook列表

### 9.1 创建空菜单地图

1. File → New Level，选择**Empty Level**，不选Open World / Empty Open World。Save Current Level As到`/Game/EmberTrial/Maps/L_ET_MainMenu`。
2. 在Place Actors中搜索并放一个**Player Start**，位置(0,0,0)、Rotation=(0,0,0)，可命名`PS_MainMenu`。虽然不生成Pawn，GameMode登录仍查询出生点，保留一个可避免无PlayerStart提示。
3. World Settings → GameMode Override=**BP_ETMenuGameMode**。展开Selected GameMode核对Menu Controller、Default Pawn None；这些值来自菜单BP，不填正式BP_ETGameMode。
4. 不放玩家、敌人、训练目标、导航体或Level Blueprint创建Widget节点。空世界由全屏UMG覆盖，不需要灯光 / 天空 / 美术资源。
5. Save。World Outliner只需PlayerStart及引擎必要对象，不必通过Light Build增加无用内容。

### 9.2 正式地图保留原装配

1. 打开`L_ET_VerticalSlice`，World Settings确认Override仍为**BP_ETGameMode**，其Controller仍BP_ETPlayerController、Default Pawn仍BP_ThirdPersonCharacter。
2. 不把菜单Controller设成战斗地图Controller，不删除原单敌人 / 训练目标 / NavMesh / PlayerStart。
3. 如没有实际改动无需重新保存制造差异；当前导航与战斗设置沿用M1-05。检查完成回`L_ET_MainMenu`。

### 9.3 Project Settings → Maps & Modes

1. Edit → Project Settings → Maps & Modes：**Game Default Map=L_ET_MainMenu**。
2. **Editor Startup Map=L_ET_VerticalSlice**，便于日常开发；它决定编辑器下次打开哪个地图，不决定独立游戏开局。
3. Global Default GameMode保持现有值；两张地图各自的Override决定当前规则。不为菜单修改整个项目的默认玩家身体。
4. UE写入共享`EmberTrial/Config/DefaultEngine.ini`。设置保存后可在CLion查看GameDefaultMap / EditorStartupMap分别指向这两个地图。

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/EmberTrial/Maps/L_ET_MainMenu.L_ET_MainMenu
EditorStartupMap=/Game/EmberTrial/Maps/L_ET_VerticalSlice.L_ET_VerticalSlice
```

上面只说明相关两项，不替换整个DefaultEngine.ini，也不删除已有渲染设置 / GlobalDefaultGameMode。

### 9.4 Project Settings → Packaging

1. 搜索**List of maps to include in a packaged build / Maps to Cook**。
2. 保留已存在`/Game/ThirdPerson/Maps/ThirdPersonMap`，再增加下面两项（若已经有就不重复）：
   - `/Game/EmberTrial/Maps/L_ET_MainMenu`
   - `/Game/EmberTrial/Maps/L_ET_VerticalSlice`
3. 两个条目是地图Package路径，**不带`.L_ET_MainMenu`对象后缀**，不填Windows磁盘路径。
4. 其他Cook、语言、构建参数不改。共享`DefaultGame.ini`会出现相应MapsToCook项；本批只配好列表，M1-07再实际Cook验证。

```ini
+MapsToCook=(FilePath="/Game/EmberTrial/Maps/L_ET_MainMenu")
+MapsToCook=(FilePath="/Game/EmberTrial/Maps/L_ET_VerticalSlice")
```

选择器里找不到地图时先确认地图已正常保存。仅Game Default Map并不能证明软引用战斗地图一定已Cook，两个配置都有用途。

## 10. 三组分层检查（约8–12分钟）

**使用Play而不是Simulate，Net Mode=Play Standalone、单玩家。**首次跨地图，建议Play菜单选择New Editor Window (PIE)，鼠标 / 焦点更容易观察。PIE载入编辑器当前地图；因此先打开`L_ET_MainMenu`，不能只设置Game Default Map后在战斗地图按Play就期待菜单出现。

不要求你逐条复制完整日志。每组观察画面结果，完成后给完整时分，我读取日志确认对象 / 请求 / 清理。这里只验新增菜单、切图输入与直接受影响的暂停 / 战斗短基线。

### 10.1 菜单隔离、鼠标开始与新世界输入（约3–4分钟）

**准备：**正常菜单Controller配置、打开L_ET_MainMenu，全部Compile / Save后Play。

**按顺序操作：**

1. 看菜单标题、三按钮与中文说明，鼠标光标可见；点击禁用继续不执行任何动作。
2. 先在菜单按W / E / P / 1 / 2 / 3，确认没有角色、战斗HUD或暂停页出现。
3. 鼠标点击开始；可快速补点两次，观察只能接受一次请求。加载太快无法补点也正常，不降低slomo或人为制造卡顿。
4. 进入房间后，**先不额外点击视口**，直接W移动、鼠标转向；释放W后站定左键攻击一次，确认首次新点击能攻击。观察HUD100/100（敌人实际命中后下降属于正常）。
5. 按2闪避一次，P打开 → 鼠标继续；恢复后移动。此处只做短基线，不重做取消 / 双目标 / 全部UI矩阵。

**画面 / 日志预期：**

- Menu ready只有一份，`Pawn=None`；菜单世界没有Health widget created或敌人自动交战。
- 此轮一条`[M1-06A] Start requested`；旧菜单`Menu removed` / `Menu controller end`，新游戏Controller有`Game input ready`，LocalPlayer改绑新Controller / Pawn，新HUD进入Ready。
- 菜单消失、鼠标隐藏、输入恢复；没有因为开始点击或菜单期间按键而无意补攻击 / 跳跃 / E。
- 首次新左键与P输入可用，结束无Accessed None / Blueprint Runtime Error。

**清理：**Stop PIE，编辑器仍打开L_ET_MainMenu；不改地图出生位置 / 敌人参数。

### 10.2 键盘开始、第二轮新实例与退出（约3–4分钟）

**准备：**配置保持正常，仍从菜单地图开始新的PIE。

**按顺序操作：**

1. 开局开始按钮应已获得用户焦点。按Tab / Shift+Tab，在开始与退出之间切换，禁用继续不执行命令；回到开始，按Enter并松开开始游戏。
2. 在新房间按Space一次确认正常跳跃；站定后左键一次，P打开后Enter或Space继续，移动 / 转向仍正常。
3. Stop PIE，再从菜单启动一轮。这次用Space并松开激活开始，进入后确认不自动跳跃；随即Stop，无需重复战斗检查。
4. 从菜单再启动一轮，用Tab选择退出，按Enter并松开；**当前PIE结束，UE编辑器仍打开**。

**画面 / 日志预期：**

- Enter / Space都能经Button正常触发，单轮开始只一条请求；新一轮Menu / Game Controller名字可递增，代表新实例，不是重复叠页。
- 上一轮Controller / Widget结束后没有后续旧回调，新HUD正确观察新的玩家身体。
- 退出有`Quit requested`与菜单清理，PIE停止；本次未用新独立exe，不把PIE退出写成已验证包内进程退出。

**清理：**PIE已停；配置不变。本组多次PIE用正式菜单即可，不添加旧Widget强持有或测试函数。

### 10.3 None地图预检拒绝与最终恢复（约2–3分钟）

**准备：**Stop PIE。打开BP_ETMainMenuController，Class Defaults把Gameplay Map临时清成**None**，Compile / Save。只改这一项，不删除实际地图文件或改GameMode。

**按顺序操作：**

1. 在菜单地图Play，点击开始一次；菜单应保留，StatusText变为“游戏地图不可用，请检查主菜单配置。”，开始 / 退出恢复可用。
2. 不再连续点无效开始，选择退出，确认仍能退出PIE。
3. **恢复Gameplay Map=L_ET_VerticalSlice，Compile / Save。**从菜单Play并正常开始一次，确认房间 / HUD / 输入恢复；Stop。

**画面 / 日志预期：**

- None样例一条`Game map unavailable` Warning，没有Start requested、Browse进入游戏地图或新Pawn；Warning为故意负例。
- 无崩溃、按钮可继续操作；恢复后正常一次切图。
- 这个用例只验证软地图预检拒绝；**不触发真正加载失败，不据此声称TravelFailure回调或包内所有故障已亲测**。

**清理：**确认地图引用已恢复、三个Class与Widget / 菜单地图Save。最后编辑器回`L_ET_VerticalSlice`，日常开发从正式地图进行；Game Default Map仍是菜单。

## 11. 问题定位与本批保留项

| 现象 | 优先给我的材料 / 检查点 |
| --- | --- |
| Build exit6 | 第一条真实UHT / C++错误、文件 / 行号与前后10–20行；新文件所在模块 / cpp include路径 |
| 菜单空白或仍有角色 / HUD | 菜单World Settings Override、菜单GM Classes与Controller Class Defaults同框截图；Menu ready前后的日志 |
| BindWidget / Native Tick警告 | WBP父类、Hierarchy四名称类型、Is Variable / Tick Frequency截图；最后Compile的原始文本 |
| 点击开始仍在菜单 | StatusText文字、`Game map unavailable`或Travel failed原始日志、Gameplay Map实际资源路径 |
| 进入后不能移动 / 鼠标仍出现 | 是否看到Game input ready、正式地图Override与Controller Class；是否修改了既有BeginPlay蓝图输入逻辑 |
| 跨地图旧对象错误 / 重复HUD | 完整操作顺序与时分、文字日志；画面如确有叠页附截图。不先搭额外持有旧Widget的测试图 |

本批保留六个新类文件、菜单四资产、两个共享配置变化与游戏Controller输入初始化。原BP_ETPlayerController / UIBootstrap / Health Presenter / HUD、原玩家角色 / Combat / AI / 1–4 / E / P保持当前业务；没有临时测试节点需要删除。

加载提示可能因同步切图太快不明显，是状态反馈，不要求它播放一段动画。恢复Map None后不留错误配置；菜单暂未提供返回战斗或失败重试，后续B / C / D再扩展。

### 11.1 菜单空HUD初始化Warning（本轮发现，非阻塞）

验收日志五次菜单开局各有一条`SpawnActor failed because no class was specified`。UE5.5.4默认`AGameModeBase::InitializeHUDForPlayer_Implementation`仍调用`ClientSetHUD(HUDClass)`，PlayerController会尝试生成AHUD；本菜单HUDClass=None，因此出现提示。菜单与三个正常切图、HUD / 输入清理都通过，不要求立即重跑本批。

**随下一批原生代码构建做这个局部补修，当前源码尚未修改：**

1. 在`EmberTrial/Source/EmberTrial/Public/Framework/ETMenuGameMode.h`的`protected`中，与已有HandleStartingNewPlayer声明并列加入：

```cpp
    virtual void InitializeHUDForPlayer_Implementation(
        APlayerController* NewPlayer) override;
```

2. 在同类`.cpp`中、构造函数之后加入：

```cpp
void AETMenuGameMode::InitializeHUDForPlayer_Implementation(
    APlayerController* NewPlayer)
{
    // The menu world intentionally has no AHUD actor.
    (void)NewPlayer;
}
```

不调用Super，不改游戏GameMode / 正式HUD / HUDClass=None配置。此修改仅跳过菜单世界不需要的AHUD生成路径；下一批关闭UE后正常双目标Build，一次菜单Play应没有此Warning，开始仍正常即可。真正TravelFailure与独立包验证继续留在对应开发批次。

## 12. 完成反馈（复制填写；我负责读日志）

```text
M1-06A 完成
六个新文件 / 游戏Controller.cpp；Editor Build / 索引29cpp / Game Build：
7.1 追击Task四处改名、黑板字符串保持；重建无C2374：
WBP父类 / 四BindWidget / 无Binding / Never / Compile：
菜单Controller / GameMode父类与Classes；无UIBootstrap / Pawn：
菜单Map Override / Game Default Map / Editor Startup Map / 两新Cook条目：
10.1 菜单隔离 / 鼠标单次开始 / 旧页清理 / 新HUD与首次输入：
10.2 Tab / Enter / Space / 无补跳 / 第二轮新实例 / PIE退出：
10.3 None Map拒绝且可退出 / 原Map恢复后正常进入：
P鼠标 / 键盘继续、左键 / 2与移动转向跳跃短基线：
临时Map=None已恢复、Save All、已回正式地图：
失败或特别说明：
大致完成时间（完整时分，例如21:18）：
实际用时：代码__ / 资产__ / 检查__；构建等待__分钟
```

你不用手动统计对象 / 日志次数或填Source哈希。我复核实际文件、receipt / 索引、菜单→战斗的对象与输入记录；未保留的视觉 / 键盘细节依据你反馈，不把静态API核对当实机验收。**2026-10-10收尾已完成：**[A报告](Reports/M1_06A_Verification.md)接受本批功能，四项新资产已真实登记，来源清单44；原40项哈希不变。菜单空HUD Warning登记为非阻塞后续修正，不宣称它已消失。

## 13. 本批需要Commit / Push的内容（固定放在文档最后）

### 13.1 实现源码

选择第2节**六份新Source**、`EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp`，以及第7.1节补修的`EmberTrial/Source/EmberTrial/Private/AI/ETBTTask_PursuitMove.cpp`。不包含生成的`.generated.h`、Module.EmberTrial.cpp、CMakeLists.txt或compile_commands；Build.cs / Target / uproject本批不应因这份方案产生变化。

推荐Commit标题：`feat: add standalone main menu and game entry`。通过反馈后补验收摘要，可和功能一起提交；若已先提交实现，再追加文档收尾即可，不改写历史。

### 13.2 可编辑UE资产与共享配置

| 内容 | 仓库相对路径 |
| --- | --- |
| 菜单地图 | `EmberTrial/Content/EmberTrial/Maps/L_ET_MainMenu.umap` |
| 菜单GameMode BP | `EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETMenuGameMode.uasset` |
| 菜单Controller BP | `EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETMainMenuController.uasset` |
| 菜单Widget BP | `EmberTrial/Content/EmberTrial/ui/Menu/WBP_ETMainMenu.uasset` |
| 启动地图 | `EmberTrial/Config/DefaultEngine.ini` |
| Cook地图列表 | `EmberTrial/Config/DefaultGame.ini` |

四个资产继续使用现有LFS规则。正式地图 / 旧Blueprint只有你实际修改并保存了必要配置才加入；不要求为了本章重新保存所有旧资产制造差异。

### 13.3 文档与现有待提交项

- `Docs/M1_06_FlowRoadmap.md`、`Docs/M1_06A_MainMenu.md`。
- 本次更新的`Docs/Progress.md`、`Docs/Backlog.md`、`Docs/Architecture.md`、`Introduction.md`、`Docs/VersionControl.md`、`Docs/AcceptanceWorkflow_Options.md`。
- 已完成的`Docs/Reports/M1_06A_Verification.md`、`Docs/AssetManifest.csv`四项真实登记与状态更新。
- 已暂存的`EmberTrial/Config/DefaultEditorPerProjectUserSettings.ini`取消跟踪**删除记录**，本地个人设置保留。上一轮主体 / 文档已提交8843ffa，这条删除尚未提交；若你已在另一提交处理则不重复操作。

### 13.4 留本地与CLion最后操作

Raw证据 / 日志、Artifacts视频 / 包、Binaries / Intermediate / Saved / DDC、`.idea` / `.run`、生成CMake / compile_commands与索引缓存留本地，禁止Force Add绕过现有忽略规则。

**本次已核对六个新Source为AM：此前暂存的是初始类骨架，工作区才有完整代码。**在CLion Staging窗口将这六个文件的最新修改再次Stage / Add，确认Staged diff包含完整实现；若使用非Staging的Commit窗口，选择这些文件的全部改动。不要只提交已有暂存内容。

完成并恢复配置后，在CLion Commit / Staging窗口选择上述**实际变化**，核对四个UE资产及八份源码、两Config与验收文档路径，再Commit → Push已有origin/main。不提交未知额外改动，不盲目丢弃它们；出现Push错误给原始文本，我定位后再处理。助手不会代Commit / Push。
