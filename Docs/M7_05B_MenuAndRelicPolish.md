# M7-05B 菜单、暂停与遗物界面打磨

统一主菜单、暂停、祭坛、四类设置和胜败页的字体、背景与按钮状态，让它们与左下战斗 HUD 共用深色和暖琥珀的视觉风格。遗物详情继续使用可滚动正文和固定操作区，设置列继续支持焦点滚入。预计 Designer **80–110 分钟**，实机检查 **10–15 分钟**。

本章使用十份已有 WBP。开始前在 CLion 确认 M7-05A 已提交，然后在 UE 停止 PIE。下面的名称来自存盘蓝图实际层级，读数保存在本地 `Artifacts/M7_05A/MenuTreesBefore.json`。这些操作直接在当前编辑器的 Designer 中完成，改完一组就 Compile → Save。

## 1. 共用色彩和按钮样式

所有颜色都填颜色选择器的 **Hex sRGB**，输入含 Alpha 的八位值。Font 保留现有支持中文的字体资产，Render Transform 采用默认值。

| 用途 | Hex sRGB |
| --- | --- |
| 主菜单全屏底色 | `10141CFF` |
| 暂停、失败与胜利页遮罩 | `10141CD9` |
| 遗物和设置卡片底色 | `171B22F2` |
| 主要文字 | `F5F1E8FF` |
| 次要文字和状态说明 | `B9C2CCFF` |
| 标题强调色 | `D9AA63FF` |
| 按钮 Normal / Hovered / Pressed Tint | `29323FFF` / `455269FF` / `705837FF` |

在 Designer 的 Hierarchy 选中一个 **Button**，Details 搜索 **Style**，分别展开 **Normal、Hovered、Pressed**，修改各 Brush 的 **Tint**。保留 Brush 原来的 Draw As、Image 和 Margin。再清空 Details 搜索，找到 **Background Color** 和 **Color and Opacity**，两者均设白色 `FFFFFFFF`，使 Style 中的颜色按原值显示。设置 **Is Focusable=True**。

按钮内的 Text Block 用主要文字色。主操作按钮的字号 **22**，卡片内按钮的字号 **14**。主操作按钮的 **Normal Padding / Pressed Padding** 均设左/上/右/下 **16 / 10 / 16 / 10**，卡片内按钮用 **8 / 6 / 8 / 6**，避免按下时文字位移。保留 Disabled 样式和引擎键盘焦点提示，运行时启用状态由已有逻辑控制。

可先做好一个主按钮，再选另一个同类按钮，在 Style 属性行使用右键 **Copy / Paste** 复制结构值。只粘贴 Style，随后分别设置 Padding 与子文本字号。每份 WBP 的既有对象名、父类和 BindWidget 对照第 7 节。

## 2. 主菜单

1. 打开 `/Game/EmberTrial/ui/Menu/WBP_ETMainMenu`，进入 Designer。选 `Border_Background`，Brush Color 用全屏底色，Padding 四边 **24**。它的 Canvas Panel Slot 为铺满四边的 Anchors，四个 Offset 为 **0**。
2. 找到 `Border_Background → MainMenuScale → MainMenuColumns`。选 `MainMenuScale`，Stretch 为 **Scale To Fit**，Stretch Direction 为 **Down Only**。`MainMenuColumns` 的 Scale Box Slot 为水平和垂直 **Center**。
3. 在 `MainMenuColumns` 下核对两个 Size Box。`MainMenuActionsSize` 的 Width Override 为 **480**，Height Override 不勾选。其 Horizontal Box Slot 为 **Auto / Vertical Center**。`MainMenuSettingsSize` 为 **320 × 680**，同样 Auto / Vertical Center，Slot 左边距 **32**。
4. 展开 `MainMenuActionsSize → VerticalBox_Menu`。按表设置。Text Block 的 Slot 采用 **Auto / Horizontal Fill**，表中列出的边距以外都用 0。

   | 控件 | 配置 |
   | --- | --- |
   | `Text_Title` | 显示“灰烬试炼”，Font Size **36**，强调色，Auto Wrap Text 开，Wrap Text At **480**，Bottom Padding **12** |
   | `Text_Subtitle` | 显示“三场战斗试炼”，字号 **16**，次要文字色，Auto Wrap 开，Bottom Padding **24** |
   | `StartButton`、`ContinueButton`、`QuitButton` | 第 1 节主按钮样式，Slot Bottom Padding **12**。内部 Text 分别沿用“开始新试炼”“继续本次试炼”“退出游戏” |
   | `StatusText` | Designer 的 Content → Text 清空，字号 **16**，次要文字色，Auto Wrap 开，Wrap Text At **480**，Slot Top Padding **8** |

5. `StatusText` 的运行时状态由原生写入。旧 Designer 文案“会话检查点仅在本次运行有效”与现有持久化存档行为不符，清空后可避免载入首帧出现过期说明。
6. 展开右列 `MainMenuSettingsSize → MainMenuSettingsScroll → MainMenuSettingsStack`，四卡顺序为 Input / Video / Graphics / Audio。第一卡 Top Padding **0**，其余 **12**。滚动属性按第 5 节核对。Compile → Save。

## 3. 暂停与祭坛共用页面

1. 打开 `/Game/EmberTrial/ui/Menu/WBP_PauseMenu`。根 Canvas 的子 Border 实际名为 **`Border_122`**，选它并设置遮罩色、Padding 四边 **24**。Canvas Slot 为四边 Stretch，四个 Offset 为 **0**。
2. 展开 `Border_122 → PauseScale → PauseColumns`。`PauseScale` 用 **Scale To Fit / Down Only**，`PauseColumns` 在 Scale Box Slot 中水平与垂直居中。
3. 三列分别是 `PauseActionsSize`、`RelicPanel`、`PauseVideoSettingsSize`。左列宽 **320**、无 Height Override，中间遗物实例的 Horizontal Box Slot 左边距 **24**，右列 **320 × 680**、左边距 **32**。三列均为 Auto / Vertical Center。
4. 展开左列 `PauseActionsSize → VerticalBox_45`。标题 **`TextBlock_50`** 的文字保留“游戏已暂停”，字号 **32**、主要文字色、Auto Wrap 开。标题 Bottom Padding **20**。
5. `BTN_Resume` 和 `MainMenuButton` 使用第 1 节主按钮样式。Slot 用 Auto / Horizontal Fill / Bottom Padding **12**。其子文本分别是 `TextBlock_109` 和 `MainMenuLabel`，文字保留“继续游戏”和“返回主菜单”。
6. `StatusText` 字号 **16**、次要文字色，Auto Wrap 开、Wrap Text At **320**，Top Padding **4**、Bottom Padding **12**。左列的 `InputSettingsPanel` 接着放在其下，Vertical Box Slot 为 Auto / Horizontal Fill。
7. 右列 `PauseVideoSettingsSize → PauseSettingsScroll → PauseSettingsStack` 为 Video / Graphics / Audio，第一卡 Top Padding **0**，后两卡 **12**。本页同时供祭坛使用，运行时的说明和只读 / 可装备状态由现有 Flow 配置。Compile → Save。

## 4. 遗物列表与详情

### 4.1 遗物卡布局

打开 `/Game/EmberTrial/ui/Inventory/WBP_ETRelicPanel`。以下层级已存在。

```text
PanelSize
└─ PanelBorder
   └─ Vertical
      ├─ TitleText / InventoryStatusText / SearchBox / QueryOptionsRow
      └─ BodyColumns
         ├─ RelicList
         └─ DetailsColumn
            ├─ DetailsScroll
            │  └─ 名称、说明、槽位、比较按钮、PreviewText
            └─ CommandContent
               └─ 确认 / 卸装按钮、CommandResultText
```

1. `PanelSize` 用 **720 × 620**，`PanelBorder` 用卡片底色、四边 Padding **16**。`Vertical` 的 Border Slot 为 Fill / Fill。
2. `TitleText` 字号 **24**、强调色，`InventoryStatusText` 字号 **16**、次要文字色。两个 Slot 都用 Auto / Fill / Bottom Padding **8**。
3. `SearchBox` 的输入字体改为 **16**。展开它的 Style，Normal / Hovered / Focused 背景 Brush Tint 分别为 **`202732FF` / `29323FFF` / `455269FF`**。Foreground Color 为主要文字色，Padding **8 / 6 / 8 / 6**。`QueryOptionsRow` Top Padding **8**、Bottom Padding **12**。原下拉框选项继续由 C++ 生成。
4. `BodyColumns` 的 Vertical Box Slot 用 **Fill 1 / Horizontal Fill / Vertical Fill**。`RelicList` 的 Horizontal Box Slot 为 **Fill 0.38**，Right Padding **16**。`DetailsColumn` 的 Slot 为 **Fill 0.62**，其余边距 0，让长说明拥有更宽列。
5. `DetailsScroll` 的 Vertical Box Slot 为 **Fill 1**，`CommandContent` 为 **Auto**。详情滚动视口使用 **Clip to Bounds**。正文滚动时底部确认和卸装区仍可见。

### 4.2 详情文字与条目

在详情中选中以下 Text Block，Font 和颜色按表设，统一 **Auto Wrap Text=True**，清除已有 **Wrap Text At** 的固定值，用 **0** 让其跟随列宽。Vertical Box Slot 用 Auto / Fill。

| 控件 | Font Size / 色彩 | Bottom Padding |
| --- | --- | ---: |
| `DetailNameText` | 22 / 主要文字色 | 8 |
| `DetailDescriptionText` | 16 / 次要文字色 | 12 |
| `SlotText` | 16 / 强调色 | 8 |
| `PreviewText` | 16 / 主要文字色 | 8 |
| `CommandResultText` | 14 / 次要文字色 | 0 |

`SlotAButton`、`SlotBButton`、`ConfirmEquipButton` 和 `UnequipButton` 使用第 1 节卡片按钮样式。`CommandContent` 的 Slot Top Padding 设 **12**，它内部 `CommandButtons` 的 Bottom Padding 设 **8**。布局按当前父容器设置，按钮文字继续由已有状态控制。

打开同目录 `WBP_ETRelicListEntry`。根 `EntrySize` 的 Height Override 用 **64**，Width Override 关闭。选 `SelectionBorder`，Padding **8 / 8 / 8 / 8**。`ItemNameText` 字号 **18**、主要文字色，`EquippedText` 字号 **14**、次要文字色。两个 Text 的 Auto Wrap Text 均开，垂直 Slot 为 Auto / Fill。选择色沿用原生事件提供的金色，条目的 Tick Frequency 用 **Auto**，供 ListView 的条目生命周期使用。

两份资产分别 Compile → Save。列表仍用单选和已有 Entry Widget Class。

## 5. 四类设置卡与滚动焦点

在 `/Game/EmberTrial/ui/Settings` 依次打开四份设置 WBP。根 Size Box 继续自动计算高度，Width / Height Override 均关闭。每张卡的根 Border 用卡片底色、Padding 四边 **12**，其唯一 Vertical Box 子项为 Fill / Fill。

| WBP | Border | 标题 |
| --- | --- | --- |
| `WBP_ETInputSettings` | `InputSettingsBorder` | `InputSettingsTitle` |
| `WBP_ETVideoSettings` | `VideoSettingBorder` | `VideoTitle` |
| `WBP_ETGraphicsSettings` | `GraphicsSettingBorder` | `GraphicsTitle` |
| `WBP_ETAudioSettings` | `AudioSettingsBorder` | `AudioTitle` |

1. 标题字号 **22**、主要文字色，Slot Bottom Padding **10**。卡内字段标签、当前值、Slider 数值和 CheckBox 子标签用字号 **16**，主要文字色。状态说明用 **14**、次要文字色，Slot Top Padding **8**。
2. 所有可能变长的当前配置和状态 Text Block 开 **Auto Wrap Text**，Wrap Text At 用 **0**。例如 `CurrentModeText`、`GraphicsCurrentText`、`AudioCurrentText` 及四卡底部的 StatusText。将 `GraphicsCurrentText`、`GraphicsStatusText` 里的 Designer “Text Block”清空，运行时内容由原生填写。
3. 所有卡片 Button 采用第 1 节卡片按钮样式。其子文本字号 **14**。Slider 的 **Slider Bar Color** 用 `455269FF`，**Slider Handle Color** 用强调色。保留既有灵敏度和音量范围、步进与可聚焦设置。
4. Combo Box String 在 **Appearance → Font** 使用 **16**。其 **Foreground Color** 用主要文字色。在 **Widget Style → Combo Button Style → Button Style** 下，按第 1 节设置 Normal / Hovered / Pressed 的 Brush Tint。展开列表的 **Item Style** 用相同三种底色区分普通、悬停与选中，保留原 Scroll Bar Style。字体和前景色也要在下拉展开后可读。
5. 回到主菜单与暂停宿主，分别选择 **`MainMenuSettingsScroll`** 和 **`PauseSettingsScroll`**，核对 **Orientation Vertical、Scroll When Focus Changes Instant Scroll、Navigation Destination Into View、Consume Mouse Wheel When Scrolling Possible**。各卡的 Vertical Box Slot 使用 **Auto / Horizontal Fill**，滚动列之外的按钮仍能获得键盘焦点。
6. 四个设置 WBP 先各自 Compile → Save，再编译保存 MainMenu 和 Pause，接着在 Content Browser 对本章十个资产执行 **Validate Assets**。

## 6. 胜败页统一样式

打开 `/Game/EmberTrial/ui/Flow/WBP_ETFailure` 与 `WBP_ETVictory`。二者都使用 `ETFailureWidget` 父类，已有根树是 **Canvas_Root → Border_Background → VBox_Failure**。

1. `Border_Background` 使用遮罩色、Padding 四边 **48**，Canvas Slot 四边铺满、Offset 0。`VBox_Failure` 在 Border Slot 中水平与垂直居中。
2. 标题 `Text_Title` 字号 **36**。失败标题用赤陶色 **`D57460FF`**，胜利标题用强调色。标题显示内容分别保留“试炼失败”和“试炼完成”。
3. `Text_Subtitle` 字号 **18**、次要文字色，Auto Wrap 开、Wrap Text At **480**。`StatusText` 用 **16**、次要文字色和同样换行设置，内容由运行时写入。
4. `RetryButton` 与 `MainMenuButton` 使用主按钮样式，Slot Auto / Fill / 上下 Padding **8**。失败页重试文案保持原值，胜利页的 `RetryButton` 文案是“再玩一次”。Compile → Save。

## 7. 分组验收与反馈

先编译保存被编辑的子 WBP，再编译宿主。父类、绑定与焦点行为按下表核对。普通 UserWidget 的 Tick Frequency 沿用 **Never**，遗物 ListEntry 采用 **Auto**。

| 页面 | 必需契约 |
| --- | --- |
| MainMenu | 原八项绑定，三按钮可聚焦 |
| Pause / Altar | 原八项绑定，`RelicPanel` 指向原生 `ETRelicPanelWidget` 的 WBP |
| RelicPanel / Entry | 原十五项 / 三项绑定，ListView 的 Entry Class、选择模式和复用事件完整 |
| Input / Video / Graphics / Audio | 原七 / 八 / 八 / 十一项绑定，可编辑控件支持键盘 |
| Failure / Victory | `RetryButton`、`MainMenuButton`、`StatusText` 三项绑定 |

| 组别 | 操作与通过条件 |
| --- | --- |
| 首屏和小窗 | PIE 看菜单样式及 Continue 的真实可用状态，再用 **1280 × 720** 独立小窗看一次菜单与 P 页。标题、中文和按键标签可读，设置列能滚到音量卡。记录实际窗口尺寸，留菜单、P 或祭坛两张截图。 |
| 焦点和暂停 | Tab / Shift+Tab 能进入设置列，焦点移到屏幕外时自动滚入，方向键、Space、Enter 能操作对应控件。回到游戏后移动无卡键。各设置草稿内 Esc 的原取消行为正常。 |
| 祭坛 | E 打开，依次选最长说明的逆焰刻印和裂势印，详情能滚动，确认 / 卸装始终可见。选择槽位并完成一次装备和卸装。普通 P 页保持只读，关闭后继续游戏。 |
| 胜败页 | 一次真实死亡，检查失败页与键盘 Retry。胜利页的样式在 Designer 预览确认，本阶段集成通关时再验证“再玩一次”和返回菜单。 |

图形、显示和音频卡在 PIE 中的应用限制沿用原行为。本章可用既有独立运行入口检查它们的真实控件状态。样式验收优先检查展开后的下拉文字、禁用按钮和键盘焦点对比。

```text
M7-05B 完成
十份WBP实际路径 / 原父类与绑定 / Compile Save / Validate：
主菜单与Pause三列 / 原ScaleBox / 两ScrollBox / 1280×720实际尺寸：
中文、按钮悬停与按下 / 下拉展开可读 / Tab焦点自动滚动：
最长遗物说明 / 五行比较 / 固定操作区 / 祭坛装卸与普通P只读：
P恢复与无卡键 / 一次失败Retry / 胜利Designer预览：
两张截图路径：
最明显的布局或交互问题（无填无）：
完成时分 / Designer分钟 / 检查分钟：
```

## 8. Commit / Push

下发本章文档时，暂存本指南、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。M7-05A 验收报告和来源清单可随同一批文档提交。

- 英文 `docs(ui): verify combat HUD and plan menu polish`
- 中文 `docs(ui): 记录战斗 HUD 验收与菜单界面打磨方案`

完成后暂存上述十份实际改动的 WBP，连同更新实物哈希的 `Docs/AssetManifest.csv` 和本指南的结果记录。截图和日志保留在本地 `Artifacts/M7_05B`。

- 英文 `feat(ui): unify menu relic and settings presentation`
- 中文 `feat(ui): 统一菜单、遗物与设置界面的视觉样式`
