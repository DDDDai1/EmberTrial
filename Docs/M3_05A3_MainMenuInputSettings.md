# M3-05A3：主菜单复用输入设置卡

2026-11-05，UE **5.5.4**。[A2暂停输入卡已验收](Reports/M3_05A2_Verification.md)，无待补。本章是手动实施指南，正式Source / 资产尚未修改，没有本章Build或Success结论。

## 1. 本章交付

打开主菜单即可在右侧输入设置卡调整偏好，再开始 / 继续试炼。复用A2的同一Widget资产与草稿规则：应用才保存、离开丢弃未应用修改。主菜单没有Pawn / ASC / Gameplay HUD，设置卡仍能独立工作；暂停页继续复用同一卡片。

这是一小批入口复用：**四份旧UI源码、两份旧Widget资产**，不新增类 / 测试 / 资产。只在MainMenuWidget加入第五绑定与既有busy透传，输入卡仅改两处通用提示及注释。MainMenuController、RunSession、Core、Config、Pause和输入映射均无需改动。之后进入M3-05B的显示设置，分辨率实际应用 / 确认回退另外设计。

预算代码15–25 / Designer15–25 / 阅读5 / 检查5–10分钟，构建等待另计。实际201 Source / 113cpp（Core78 / UI32 / Editor3）、34自动化、300 Content / 来源81，本批完成后数量保持。预算不计实耗。

## 2. 实施前与四个文件

UE Save All、停止PIE并关闭编辑器，CLion保留。以下都是已有文件，不创建同名类或新设置组件：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETMainMenuWidget.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETMainMenuWidget.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETInputSettingsWidget.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETInputSettingsWidget.cpp
```

改完C++后，旧主菜单在Designer配第五绑定前可能提示InputSettingsPanel缺失；先完成第5节再Play，不改为Optional或删除原资产。

## 3. 八处精确修改

原文只用来定位；用新文替换一次，未列出代码保留。第7 / 8处只改字符串内部文字，保留外层TEXT / FText和标点括号。

### 3.1 Public/Widgets/ETMainMenuWidget.h

在前置声明组增加共用输入卡类型，不在此处改generated.h顺序。

查找原文：

```cpp
class UTextBlock;
```

替换为：

```cpp
class UTextBlock;
class UETInputSettingsWidget;
```

### 3.2 Public/Widgets/ETMainMenuWidget.h

在原StatusText字段后加入第五个必需绑定；保留原四个名称。

查找原文：

```cpp
    TObjectPtr<UTextBlock> StatusText;
```

替换为：

```cpp
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETInputSettingsWidget> InputSettingsPanel;
```

### 3.3 Private/Widgets/ETMainMenuWidget.cpp

增加新卡头文件，使用已存在的UI公开依赖。

查找原文：

```cpp
#include "Widgets/ETMainMenuWidget.h"
```

替换为：

```cpp
#include "Widgets/ETMainMenuWidget.h"
#include "Widgets/ETInputSettingsWidget.h"
```

### 3.4 Private/Widgets/ETMainMenuWidget.cpp

仅替换HasRequiredLayout内的return，把第五绑定纳入原布局预检。

查找原文：

```cpp
    return IsValid(StartButton.Get()) && IsValid(ContinueButton.Get()) &&
           IsValid(QuitButton.Get()) && IsValid(StatusText.Get());
```

替换为：

```cpp
    return IsValid(StartButton.Get()) && IsValid(ContinueButton.Get()) &&
        IsValid(QuitButton.Get()) && IsValid(StatusText.Get()) && IsValid(InputSettingsPanel.Get());
```

### 3.5 Private/Widgets/ETMainMenuWidget.cpp

在ApplyState保存bBusy后透传给输入卡，切图请求期间禁用卡片；原Continue快照与恢复顺序保留。

查找原文：

```cpp
    bBusy = bPending;
    if (!HasRequiredLayout())
```

替换为：

```cpp
    bBusy = bPending;
    if (InputSettingsPanel)
    {
        InputSettingsPanel->SetHostBusy(bBusy);
    }
    if (!HasRequiredLayout())
```

### 3.6 Public/Widgets/ETInputSettingsWidget.h

只同步类注释，公开接口和七个绑定不改。

查找原文：

```cpp
/** 现有Pause中的输入设置卡；不拥有世界暂停 / 输入模式，不另建Flow。 */
```

替换为：

```cpp
/** 主菜单 / Pause共用输入设置卡；不拥有世界暂停或输入模式。 */
```

### 3.7 Private/Widgets/ETInputSettingsWidget.cpp

NativeConstruct里的初始提示改为跨主菜单 / 暂停通用文字；只替换这段字符串。

查找原文：

```cpp
修改后选择应用；P继续会丢弃未应用的修改。
```

替换为：

```cpp
修改后选择应用；离开页面会丢弃未应用的修改。
```

### 3.8 Private/Widgets/ETInputSettingsWidget.cpp

HandleApply里的成功提示改为通用文字；不要改应用 / 保存 / 焦点判断。

查找原文：

```cpp
已应用；继续游戏后使用新设置。
```

替换为：

```cpp
已应用；进入或继续游戏后使用新设置。
```


## 4. 正常Build与索引

UE保持关闭，保存文件。PowerShell先运行正常Editor构建 / 三模块索引：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion按提示Reload，Core78 / UI32 / Editor3保持。若编译失败，发第一条error及前后10–20行；exit code 6是汇总，不通过删除缓存处理。成功后重开UE，先装配Designer。

## 5. 主菜单布局与第五绑定

### 5.1 保留原菜单，包装成左列

1. 打开`/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`，确认父类仍ETMainMenuWidget。Designer的Hierarchy找到**VerticalBox_Menu**，它包含原标题 / 副标题、StartButton、ContinueButton、QuitButton、StatusText；不要删这整列、改按钮名或重建Graph。
2. 选中整个VerticalBox_Menu，右键**Wrap With → Size Box**，新容器命名**MainMenuActionsSize**。Width Override勾选=**480**，Height Override不勾。选原VerticalBox_Menu，`Slot (Size Box Slot)`对齐Fill / Fill、Padding四向0。
3. 选MainMenuActionsSize，右键Wrap With → **Horizontal Box**，命名**MainMenuColumns**。选MainMenuActionsSize，`Slot (Horizontal Box Slot)` Size=**Auto**、Vertical Alignment=**Center**、Padding全0；不要用Fill1分走右列宽度。
4. 选MainMenuColumns，右键Wrap With → **Scale Box**，命名**MainMenuScale**。Stretch=**Scale To Fit**，Stretch Direction=**Down Only**。选MainMenuColumns，`Slot (Scale Box Slot)` Horizontal / Vertical Alignment=**Center / Center**。
5. MainMenuScale现在应是原**Border_Background的直接子项**。选MainMenuScale，在`Slot (Border Slot)`设Horizontal / Vertical Alignment=**Fill / Fill**、Padding四方向均**24**。包装时继承来的原48 / Center必须在这里替换，不去改Canvas上的位置。
6. 原Border_Background的Canvas Slot仍全屏Anchors Min0,0 / Max1,1、四Offset0，原背景色与Visible保持。标题、三按钮、Continue可用性和StatusText保留；StatusText Auto Wrap / Wrap Text At480仍适配左列。三按钮Focusable、Down and Up及24号中文文字保持。

### 5.2 右列复用原输入卡

1. 选MainMenuColumns，从Palette → User Created搜索已有**WBP_ETInputSettings**；不Duplicate、不新建另一个设置Widget。
2. 先拖**Size Box**到MainMenuColumns，作为MainMenuActionsSize的右侧兄弟，命名**MainMenuSettingsSize**。Width Override勾选=**320**、Height Override不勾；Horizontal Box Slot Size=Auto、Vertical Alignment=Center，Padding **Left32**、其余0。
3. 将已有WBP_ETInputSettings拖入MainMenuSettingsSize成为唯一子项。选**这个UserWidget实例外层**，改名为**InputSettingsPanel**、勾Is Variable。它是第五个BindWidget；别把SizeBox、内部VerticalBox或资产名改成该字段。
4. InputSettingsPanel的`Slot (Size Box Slot)`设Horizontal / Vertical Alignment=Fill / Fill、Padding全0，Visibility=Visible / Is Enabled=True。内部七个绑定、Slider范围与键盘配置沿用A2，不复制或重新接OnClicked。
5. 完整层级应为：

```text
原Canvas Panel
  Border_Background（原全屏）
    MainMenuScale（ScaleBox，Scale To Fit / Down Only）
      MainMenuColumns（HorizontalBox）
        MainMenuActionsSize（SizeBox Width480）
          VerticalBox_Menu（原）
            原标题 / 副标题
            StartButton / ContinueButton / QuitButton / StatusText
        MainMenuSettingsSize（SizeBox Width320）
          InputSettingsPanel（已有WBP_ETInputSettings实例）
```

6. 原主菜单Class Defaults保持Is Focusable=True、Tick Frequency=Never，Graph无新增业务。Compile → Save；五绑定名称 / 类型为三个原Button、原Text与新ETInputSettingsWidget。Validate Assets看到本项目Valid，再Save。

### 5.3 同步共用卡的Designer占位文案

1. 打开已有`/Game/EmberTrial/ui/Settings/WBP_ETInputSettings`，Designer选**SettingsStatusText**（Text Block，非整个Widget）。
2. `Content → Text`改为“**修改后选择应用；离开页面会丢弃未应用的修改。**”，不建Text Binding。其字号14、Auto Wrap和原七绑定保持；运行文案已由第3节同步为跨菜单通用文字。
3. Compile → Save并Validate Assets。主菜单和Pause使用同一份资产，都会得到通用提示；原Pause无额外Designer / C++修改。Save All。

## 6. 最小检查（约5–10分钟）

### 6.1 一个原专项

停止PIE，从Output Log的Cmd执行一次：

```text
Automation RunTests EmberTrial.UI.Settings.InputDraft
```

Success即可。本章没有新规则或测试注册，不新增镜像测试、不跑全34项、A1重启读取或CameraRules；原有内存夹具不冒充主菜单实机。

### 6.2 主菜单进入游戏前的真实设置

1. 在编辑器打开**L_ET_MainMenu**再Play（不要从战斗地图Play后期待自动显示菜单）。可用New Editor Window (PIE)观察。首屏原三按钮与右侧一张输入卡完整，继续按当前Session可用性显示；无Pawn / 原战斗HUD符合菜单隔离。
2. 在卡内改到约0.50 / 反转，**不应用**，点击开始新试炼。进入游戏镜头仍默认1 / 不反转；P打开原卡也应1 / 不反转，说明菜单离开没有自动提交草稿。
3. P返回主菜单。再次改0.50 / 反转，按Tab在滑条 / CheckBox / 按钮之间导航，方向键与Space修改，Enter应用。卡片应提示通用“进入或继续游戏”文字，不把P写成主菜单的继续键。
4. 用原开始或继续进入游戏，实际镜头变慢 / 反转；P卡读到相同已应用值。个人偏好不因NewRun / Continue而重置，也不产生第二套设置实例。

### 6.3 焦点、关闭与最终恢复

1. P返回主菜单，输入卡点默认值但**先取消修改**，应保留已应用0.50 / 反转；再默认值 → 应用，恢复最终**1.00 / 不反转**。Esc在卡内只取消草稿，Enter / Space只执行当前焦点控件，不意外开始 / 退出。
2. 主菜单保留一项未应用草稿，Stop，再Play菜单；卡回已应用1 / 不反转、没有旧草稿或重影。无需重新完整退出编辑器验证磁盘。
3. 最后开始试炼，确认第一轮移动 / 转向正常、P卡仍单份且鼠标继续无补跳 / 卡键；不重跑旧能力与装卸矩阵。Stop，回常用L_ET_VerticalSlice并Save All。原地图默认 / Controller配置不改。

Pending来自原Controller的ShowPending；MainMenu把同一状态传给卡，失败后ShowError恢复，不在卡内等待World Tick。此次不再搭None地图 / 临时蓝图验收；若遇切图失败只保留当轮真实日志，再按现象窄查。

## 7. 定位与后续

- 缺InputSettingsPanel：选的是UserWidget实例，名字 / Is Variable正确；其父类必须是A2原生ETInputSettingsWidget，不能用普通VerticalBox占位。
- 主菜单输入卡灰色：先看是否仍在Pending / 布局未完成；不要删除原Controller门控。新卡确认沿实际Viewport宿主检查，旧页不再写设置。
- 内容过宽 / 裁切：检查左480 / 右320、左间距32，ScaleBox Down Only；MainMenuScale在Border Slot是Fill / Padding24，不能遗留原Center / 48。小窗口应整体缩小，原按钮文字不必为凑尺寸缩成极小字号。
- Pause显示文案也变化属同资产复用结果；算法、草稿模型与其七绑定不变。无每帧配置读取 / 新Timer或第二份暂停状态。

显示设置下一批将处理引擎实际分辨率 / 窗口 / 画质和确认回退；本批不使用旧M2包验证新入口，不做新Cook。面试可解释：同一设置卡只依赖偏好值服务，两个宿主各自负责busy / 焦点 / 生命周期，个人设置不依赖ASC或RunSession。

## 8. 完成后反馈

```text
M3-05A3 完成
四旧UI文件；Editor Build / Core78-UI32-Editor3索引 / Game Build：
MainMenu第五绑定与两列缩放；共用卡文案 / 两Valid / Save：
InputDraft一次：
菜单未应用离开不提交 / 应用后进入游戏实际生效 / Pause同值：
Tab-方向键-Space-Enter / Esc仅取消 / 原开始-继续焦点：
Stop新卡无旧草稿 / 最终1.00不反转 / 最短移动与P检查：
Source / 两资产Save All、已回常用地图；布局或失败问题（无填无）：
完成时分 / 手动代码 / Designer / 阅读配置 / 检查 / 构建等待：
```

长日志由我读取；C++错误用文字与第一条error，布局 / 焦点问题附完整Hierarchy、窗口截图和操作步骤。

## 9. Commit / Push（固定放在最后）

**当前文档收尾**：A2实现在本地`df3a1cd`，起点工作区干净。Stage A2验收 / 指南收尾、来源清单 / 状态文档与本A3指南；A3正式Source / 资产尚未实施。英文`docs: verify pause input settings and prepare main menu reuse`；中文`docs: 完成暂停输入设置验收并准备主菜单复用`。

**本章实施完成后**，CLion检查最终差异再Stage：

- 第2节四份既有UI文件，保留原菜单意图 / Controller门控。
- `EmberTrial/Content/EmberTrial/ui/Menu/WBP_ETMainMenu.uasset`。
- `EmberTrial/Content/EmberTrial/ui/Settings/WBP_ETInputSettings.uasset`（仅通用占位文案）。
- 实际修改的本指南 / Reports / AssetManifest / Progress等共享文档；未生成报告不创建空文件。
- 个人GameUserSettings.ini、Saved / Intermediate / Binaries、.idea / CMake生成、Artifacts日志 / 候选 / 旧包留本地。本批没有新增源码 / 资产，也不强制重存未改Pause。

英文实现建议：`feat: reuse input settings on the main menu`。

专业中文：`feat: 在主菜单复用输入设置与草稿确认`。

Stage后又修改需重新Stage最新版本 → Review → Commit / Push由用户操作；助手未操作索引或远端。
