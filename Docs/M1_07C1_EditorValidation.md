# M1-07C1：Editor 模块与 BT / UMG 配置校验

更新2026-10-19，UE **5.5.4**。[C1原验收](Reports/M1_07C1_Verification.md)记录13:24、手动40 / 等待5及断键探针。当前源码已完成第21批清理；本页保留Editor边界、规则与复用检查步骤，首次创建过程为历史记录，无需重复。功能开发暂停在M2-04B2。

## 1. 本批交付与时间

建立一个只在编辑器加载的模块，使用 UE 自带 **Data Validation** 检查追击 BT 的黑板绑定和 UMG 的必需控件。以后修改这两类配置，可在 Content Browser 右键检查，不必反复搭临时 Controller 函数。错误包含资产路径、规则 ID 和修复提示。

预计手动 **60–100分钟**：文件与代码35–60、模块 / 索引配置15–25、专项检查5–10、记录5；正常构建 / 索引等待约5–15分钟，实际另记。无需新增蓝图业务、PIE 临时按键或本轮 Cook。初次工具建设花费较多，后续用同一入口检查；尚未测得节省率。

| 本批检查 | 明确边界 |
| --- | --- |
| ET-AI-001 | 正式 BT 有 RootNode / BlackboardAsset |
| ET-AI-002 | `PursuitStopDistance` 存在且为 Float |
| ET-AI-003 | 树内恰有一个项目追击任务，Acceptable Radius 绑定该键 |
| ET-AI-004 | 两项 Includes Radius 为常量 False，Track Moving Goal 为常量 True |
| ET-UI-001 | Widget 编译 / 控件树可用，全部原生必需 BindWidget 能找到同名控件 |
| ET-UI-002 | 找到的控件类型匹配原生属性，如 Button / TextBlock / ProgressBar |
| ET-SETUP-001 | 校验器需要的原生类 / 反射字段可用；工具自身缺依赖也明确报错 |

静态检查不证明AI真正移动、按钮输入、事件解绑或蓝图白线正确。Controller距离 / 引用 / 输入与共享配置现由[C2](M1_07C2_GameplayValidation.md)负责；[C3](Reports/M1_07C3_Verification.md)保留M1 Attempt02包证据，不能当作M2新版包已验证。

## 2. 文件和模块分工

首次接入时在CLion创建下列Editor模块文件；当前文件均已存在，只需按第3节入口阅读。修改反射类前Save All、关闭UE；Editor模块类不使用Actor / Component新建向导。

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\
├─ EmberTrialEditor.Build.cs
├─ Public\Validation\ETProjectAssetValidator.h
└─ Private\
   ├─ EmberTrialEditorModule.cpp
   └─ Validation\ETProjectAssetValidator.cpp
```

注册边界见第4节：Editor Target包含三个项目模块，Game Target只包含Core / UI；Core / UI不反向依赖Editor。

当前规模与清理结果统一见[Progress](Progress.md)，合法Probe仍保存在专用Validation目录。

依赖方向为 Editor → Core、AIModule、UMG / UMGEditor、DataValidation；Core / UI 不引用 Editor。Validator 只读取资产，不创建角色、Widget 实例或运行世界，不自动修改配置。BT 遍历和控件检查各自独立；规则 ID 便于定位问题。将来扩大规则时先新增小函数，不把运行时玩法写进这个工具。

## 3. 当前源码入口与职责

### 3.1 EmberTrialEditor.Build.cs

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\EmberTrialEditor.Build.cs`。

[EmberTrialEditor.Build.cs 当前源码](../EmberTrial/Source/EmberTrialEditor/EmberTrialEditor.Build.cs)。实现与配置以该文件为准，本文保留职责和排障说明。

DataValidation 在公开依赖中，因为公开头文件继承它的类；AssetRegistry 提供该父类头文件中的 FAssetData。UnrealEd / UMGEditor 只在这个 Editor 模块中，不向 Runtime Build.cs 添加。

### 3.2 EmberTrialEditorModule.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Private\EmberTrialEditorModule.cpp`。

[EmberTrialEditorModule.cpp 当前源码](../EmberTrial/Source/EmberTrialEditor/Private/EmberTrialEditorModule.cpp)。实现与配置以该文件为准，本文保留职责和排障说明。

UE 启动时会发现已加载模块里的原生 Validator；不用手动 AddValidator，也不用做一个 Validator 蓝图。新增反射类后正常关闭 UE 构建 / 重启，不用 Live Coding 验证注册。

### 3.3 ETProjectAssetValidator.h

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Public\Validation\ETProjectAssetValidator.h`。

[ETProjectAssetValidator.h 当前源码](../EmberTrial/Source/EmberTrialEditor/Public/Validation/ETProjectAssetValidator.h)。实现与配置以该文件为准，本文保留职责和排障说明。

`generated.h` 必须是最后一个 include。这两个 override 使用本机5.5.4的新签名；不要再添加同名 UFUNCTION，不要换成旧教程的单 UObject / ValidationErrors 参数版本。

文件名必须为 **ETProjectAssetValidator.h**（结尾是 `Validator`），与 `ETProjectAssetValidator.generated.h` 的前缀一致。若误命名为 `ETProjectAssetValidation.h`，在 CLion 项目树选中该文件 → Shift+F6 → 改为正确文件名；保留上面的 include 和类名，保存后重跑第5节。UHT报要求 `ETProjectAssetValidation.generated.h` 时，先核对文件名，不能把这条 include 移到其他 include 前面。

### 3.4 ETProjectAssetValidator.cpp

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Private\Validation\ETProjectAssetValidator.cpp`。

[ETProjectAssetValidator.cpp 当前源码](../EmberTrial/Source/EmberTrialEditor/Private/Validation/ETProjectAssetValidator.cpp)。实现与配置以该文件为准，本文保留职责和排障说明。

说明：Bool 的默认值在5.5.4中是 protected，工具通过反射读取，不把 `FValueOrBBKey_Bool` 强转 bool，也不实例化 BehaviorTreeComponent 去运行世界。Acceptable Radius **默认值150与绑定键并存是合法的**；实际值由黑板提供，工具不要求把默认常量改成90。原生追击类未导出跨模块 API，所以只按 `/Script/EmberTrial.ETBTTask_PursuitMove` 获取已加载类，不修改它的 .h。需要 Editor→Core 的显式依赖保证模块顺序。

Widget 检查读取设计时控件树，并向父 Widget Blueprint 查找继承控件；不查看尚未构建的 CDO BindWidget 指针。可选控件不当成必需控件。默认 Widget 编译器本身也检查 BindWidget，本工具统一输出项目规则与资产路径，便于以后批量入口复用。


### 3.5 当前遍历与诊断边界

| 入口 | 职责与保护 |
| --- | --- |
| CanValidateAsset | 正式BT、项目Widget与专用Validation副本；其他Tests不纳入 |
| ValidateLoadedAsset / AddIssue | 每资产独立计数；不支持类型返回NotValidated；每条问题保留规则ID / 资产路径 / 修复提示 |
| ValidateTree | 显式栈与Visited遍历；只对项目追击任务计数和检查，要求恰有一份 |
| HasConstant | GetKey为空且DefaultValue匹配；绑定黑板键不能冒充布尔常量 |
| FindRequiredWidget / ValidateWidget | IncludeSuper检查原生必需属性；沿父蓝图设计树找同名同类型控件，Optional跳过，Visited防重复 |

第21批统一排版与职责注释，未改规则编号、提示、判定顺序或范围。Editor入口与依赖文件只审计，未增加手动注册。正常资源通过不等于所有负例分支均重跑；第7节保留历史最小负例步骤供相关改动时复用。

## 4. 注册模块与插件

### 4.1 EmberTrial.uproject

[EmberTrial.uproject](../EmberTrial/EmberTrial.uproject)中，EmberTrialEditor的Type为Editor，DataValidation插件的TargetAllowList只含Editor。现有其他模块 / 插件保持；不要用历史整文件JSON覆盖当前GAS或输入配置。

[EmberTrial.uproject 当前源码](../EmberTrial/EmberTrial.uproject)。实现与配置以该文件为准，本文保留职责和排障说明。

JSON最后一项后不加逗号。修改模块定义后正常构建 / 重启，由UE发现原生校验器。

### 4.2 Editor Target

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor.Target.cs`。

仅将现有 `ExtraModuleNames.AddRange(...)` 这一行改为：

```csharp
ExtraModuleNames.AddRange(new string[] { "EmberTrial", "EmberTrialUI", "EmberTrialEditor" });
```

类名、Type=Editor、V5、Unreal5_5 和其余代码保留。**不要向 `EmberTrial.Target.cs` 增加 Editor 模块**；Game Target 原两模块保持不变。`Tools\Refresh-CLion.ps1` 已遍历 .uproject 的模块，无需再改脚本。

### 4.3 历史B的include排障

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\AI\ETBTTask_PursuitMove.cpp`。

原B收尾曾删除以下无用途include；当前已完成，阅读时以[追击Task当前实现](../EmberTrial/Source/EmberTrial/Private/AI/ETBTTask_PursuitMove.cpp)为准：

```cpp
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
```

在其他 include 后加入：

```cpp
#include "GameFramework/Pawn.h"
```

最终 `OnTaskFinished` 和构造函数的先常量 / 后 SetKey **保持你刚完成的版本**。这只是显式头文件依赖清理，不改追击逻辑。

## 5. 正常构建与索引

四个新文件和三个修改文件 Save All；确认 UE 已关闭。PowerShell 按顺序执行，前一步失败先停止，发完整首个错误，不继续 Game Build：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

预期：UBT / UHT 成功，脚本打印三个模块的 Index metadata，其中新增 `EmberTrialEditor`。CLion 的 CMake 自动重载后应出现 `EmberTrialEditorCodeModel`，两 cpp 可见；必要时点 Reload CMake Project，**不要改 UE 生成的 CMake 或清空缓存**。对 Validator 的 EditorValidatorBase.h / WidgetBlueprint.h 可导航。

然后执行 Game 构建，补齐 B 最终函数并验证 Editor 依赖隔离：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

Game 构建应成功，Game 不编译 `ETProjectAssetValidator.cpp` / Editor 模块，不报“Unable to instantiate UnrealEd module for non-editor targets”。没有编译输出时可能是 Up to date，不能单靠输出少判断隔离失败；我会复核 Target / Build 记录。启动 UE 后 Output Log 搜索 `[M1-07C1] Editor validation module ready` 应有一条。Editor 构建通过不等于校验注册和规则已验收，第6–7节仍需要执行。

## 6. 正常资产检查（约2–3分钟）

### 6.1 正式 BT

1. 打开 Content Drawer / Content Browser，定位 `/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy`，选中**资产缩略图**。这里 `/Game` 对应 Content，不是文件系统路径。
2. 右键缩略图 → **Asset Actions（资产操作）→ Validate Assets（验证资产）**。选择这一项，不选带 Dependencies 的版本，也不点蓝图 Compile 代替数据校验。
3. 打开 Output Log，搜索 `[M1-07C1]`，应看到 `Valid Asset=/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy...`。这是本项目 Validator 真正执行的标记；仅看到“验证完成”而没有项目标记，不能当注册通过。
4. 如果正式 BT 报 ET-AI-003，检查所选追击节点的 Acceptable Radius 绑定键确为 PursuitStopDistance，保存后重查；不要为了过校验直接改工具的规则。ET-AI-004 则对应三项常量开关。

### 6.2 正式 Health HUD

1. 在 Content Browser 定位 `/Game/EmberTrial/ui/HUD/WBP_ETHealthHUD`。打开它，Compile → Save，保留当前四个控件和原生父类。
2. 回资产缩略图，右键 → Asset Actions → Validate Assets。Output Log 应有该 HUD 路径的 `[M1-07C1] Valid`。
3. 不需要改控件名字制造错误，也不需要重跑 HUD 旧测试矩阵。若失败，先按规则给出的控件名称 / 类型局部核查。不要把 Native Tick、UI焦点等运行问题当成工具已覆盖的项目。

消息有错误时 UE 通常打开 Message Log 的 **Data Validation** 页；也可用 Window → Developer Tools → Message Log 查结果。若菜单翻译不同，以 Asset Actions 子菜单内含“Validate Assets”的项和 Output Log 标记为准。本机菜单注册位置及入口已核对；插件项缺失先看模块启动 / Edit → Plugins → Data Validation，别重复创建 Validator 蓝图。

## 7. 一份隔离 BT：失败与恢复（约3–5分钟）

目标是证明规则真的能捕获**这次发生过的键绑定回归**。不更改正式 BT，不给 Controller 换树，不运行 Play。

1. Content Browser 创建 `/Game/EmberTrial/Tests/Validation` 文件夹。对正式 `BT_ETMeleeEnemy` 右键 → Duplicate（复制），在名称框输入 **`BT_ETValidation_Probe`** 并按 Enter。将**副本缩略图**拖到左侧的 Validation 文件夹，弹出选项时选 **Move Here**。最终资源路径必须为 `/Game/EmberTrial/Tests/Validation/BT_ETValidation_Probe`。移动可能在原 AI/BehaviorTrees 留下同名 Redirector；对原文件夹右键 → **Fix Up Redirectors in Folder** 清理副本的旧路径，随后 Save All。正式 BT 名称 / 路径保持原值。
2. 打开副本，选中图内 **ET Pursuit Move To** 任务节点；右侧 Details 找 **Acceptable Radius**。使用你在 B 中已操作过的黑板绑定选择器，将键从 **PursuitStopDistance** 改为 **None / 不绑定**。常量保留150；其他节点 / Blackboard 引用和开关不改。
3. Save 副本。若当前编辑器开启保存时自动校验，这一步已可能报错，这是预期负例。回 Content Browser 对副本执行 Validate Assets，错误应包含：

   ```text
   [ET-AI-003] /Game/EmberTrial/Tests/Validation/BT_ETValidation_Probe...
   ET Pursuit Move To: bind Acceptable Radius to PursuitStopDistance...
   ```

   同时 Output Log 有该副本的 `[M1-07C1] Invalid ... Issues=1`。已有日志可能因保存 / 手动校验各出现一次，不按总行数判断去重；看最近一次校验。如果变成 ET-SETUP-001 或没有项目标记，先发日志，不能当预期失败通过。
4. 回副本同一节点，把 Acceptable Radius 的绑定键恢复 **PursuitStopDistance**，Save。再对副本执行 Validate Assets，应变为 `[M1-07C1] Valid`，本次没有新的 ET-AI 错误。旧 Message Log 页中的历史错误仍可能保留，按新一轮 / 时间区分。
5. 这份**已恢复为合法配置**的 Probe 保留，供以后复用。它不放地图、不设为 Controller 的树、不加 Cook 地图列表；不要留下故意错误的最终资产，也不要修改它引用的共享黑板。

本批只实测 ET-AI-003 的负例，其余规则检查实现和正常资产；不声称每条规则都已有独立错误样例。未来修改某条规则或出现相关失败时，再增加对应最小负例。既有 Tests 下的非法健康配置示例不在本工具范围内，不阻止正常批次开发。

## 8. 完成边界与日常用法

只需第5节双构建、第6节两个正常资产、第7节副本失败 / 恢复。**不要求本批重跑 PIE、旧战斗 / UI矩阵或 Insights**；模块只增加编辑器静态检查，B的行为正常已有反馈。正常检查目标5–10分钟，遇到失败只针对该项排查。

以后改正式BT / 项目Widget后，Compile（如适用）→ Save → Validate Assets，多选资源可一次验证。命令行可按BehaviorTree或Blueprint类型执行DataValidation，检查本项目Valid标记及命令汇总；不默认每次重做负例或旧玩法矩阵。真正改变运行逻辑时仍做对应窄实机检查。

原C1签名与行为依据本机EditorValidatorBase、ValueOrBBKey、BTTask_MoveTo、BaseWidgetBlueprint等源码核对；历史接入由用户手动完成。第21批源码清理与实际验证记录见[Cleanup](Cleanup.md)，不混入历史手动工时。[官方Data Validation，UE5.5](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-validation-in-unreal-engine?application_version=5.5)保留作入口参考，方法签名以本机头文件为准。

## 9. 最简反馈

```text
M1-07C1 完成 / 卡在第__节第__步
四新文件 / uproject / Editor Target / B include清理，已保存：
Editor Build / 三模块索引 / Game Build：
Editor模块启动一条；正式BT与HealthHUD均有本项目Valid标记：
隔离Probe None绑定→ET-AI-003，恢复绑定→Valid：
Probe已恢复 / 未用于正式Controller；源码与资产Save All：
失败或特别说明（无则填无）：
完成时分 / 实际手动用时 / 构建等待：
```

编译问题优先发送**从首个实际 error 起的文字日志**、文件 / 行号和刚改的内容；不要只发脚本最后的 ExitCode6。Details / 资产配置看不清则发截到资产名、选中节点和右侧字段的截图。已有这份 Probe，后续无需重新设计验收图。

## 10. 本批需要 Commit / Push（固定放在文档最后）

第21批清理的四份Validator Source与五份同步Markdown，共9项，逐文件清单见[Cleanup末尾](Cleanup.md#本批-commit--push固定放在最后)。模块入口 / Build.cs / 两Target / uproject与资产未改，不重复提交原C1 / C2实现。CLion刷新Changes → 重新Stage最新差异 → 检查Staged Diff → Commit → Push；由用户操作Git。Raw / 日志 / 生成文件留本地。
