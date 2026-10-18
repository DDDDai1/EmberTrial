# M1-02B：正式 Framework 对象与角色公开边界

2026-10-18清理同步：本页保留历史接入 / 配置 / 验收经验，相关整文件或完整方法以当前Source链接为准，不覆盖已完成工程。本次格式、入口审计与验证见[Cleanup](Cleanup.md)。

生成日期：2026-10-02；交接更新：2026-10-03。前置：[M1-02A 已验收](Reports/M1_02A_Verification.md)。**M1-02B 已通过**：用户手动 50 min、构建 / 索引等待 15 min；源码迁移、双目标构建、正式对象 / 地图装配及原功能回归验收完成。证据与日志范围见 [验收报告](Reports/M1_02B_Verification.md)。CLion 配置问题已解决，无需另发报告。本页保留实施步骤，下一批为 [M1-02C1](M1_02C1_LocalPlayerLifecycle.md)。

## 当前Controller职责边界

[头文件](../EmberTrial/Source/EmberTrial/Public/Framework/ETPlayerController.h) / [cpp](../EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp)保留Possess日志、原生P释放意图、首次GameOnly输入与清键。Core的OnPauseRequested由UI Flow订阅；OnGameMenuStateChanged是现有蓝图事件桥接，不能因普通C++调用少而删掉反射入口。

ET_Stamina / ET_Healing是复用的PIE诊断Exec，正式玩法由GA / 资源组件处理。0用于只读；非Editor方法体为空，PIE注入范围仍为体力±100、治疗0–1000。范围用内部常量命名，它们不是属性上限、GA成本或正式治疗量。World / 本地身份 / 暂停 / 死亡等原检查不变，命令不作为游戏产品输入。

## 1. 目标与选择

预计手动 **40–60 分钟**，构建 / 索引等待另计。本批让新地图使用正式 GameMode / PlayerController，并将已验证的 Character 纳入公开契约。

角色继续使用 **AEmberTrialCharacter / BP_ThirdPersonCharacter**：保留原生类名和既有蓝图类型，后续在这个身体上接入 GAS。原计划表里的 AETCharacter 名称据此修正；理由见 [ADR-001](ADR/ADR_001_FrameworkReuse.md)。本批不会重做移动、动画或复制另一套角色逻辑。

~~~mermaid
flowchart TD
    Map["L_ET_VerticalSlice 的地图 Override"] --> GM["BP_ETGameMode : AETGameMode"]
    GM --> PC["BP_ETPlayerController : AETPlayerController"]
    GM --> Pawn["BP_ThirdPersonCharacter : AEmberTrialCharacter"]
    PC -->|Possess| Pawn
    Future["当前BP装配的 UI 组件"] -.装配到.-> PC
~~~

GameMode 选择本局对象并承载规则；Controller 承载控制与 Possess 生命周期；Character 是可更换的身体。具体资源和蓝图类在 Blueprint 中配置。[Epic：Gameplay Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-framework-in-unreal-engine?application_version=5.5)

| 本批职责 | 后续接入 |
|---|---|
| AETGameMode：选择默认类、记录本局启动 | 失败 / 通关与重试流程 |
| AETPlayerController：记录控制 / 身体生命周期 | 本地 UI Bootstrap、Pawn 重绑、玩家 Inventory |
| AEmberTrialCharacter：已有身体与输入、公开类型边界 | Character 持有 ASC / 战斗，UI 通过查询 / 委托读取 |

原Framework阶段UI只有入口日志；当前已接HUD、暂停 / 失败流程。以下迁移步骤保留历史依据，当前Controller只发暂停意图并建立GameOnly输入，具体页面由UI模块装配。

## 2. 操作前准备与文件布局

1. UE **Save All**，退出 PIE 并关闭编辑器；CLion 保存所有文件，可以保持打开。
2. 本批涉及头文件位置与反射类，使用正常 UBT Build；不使用 Live Coding。
3. 在完整 EmberTrial 工程的 CLion Project 树或资源管理器中创建 / 移动文件。新文件使用 **New → File**，粘贴下方完整代码；扩展名应是 .h / .cpp。若使用 UE 新建类向导，先按 [本机操作规则](CLion_UEClassCreation.md) 关闭自动编译、创建成功时选 No，再关闭 UE 完成正常 Build / 脚本刷新；向导可能覆盖本项目的 CMake 适配配置。

本批最终布局：

~~~text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\
├─ Public\
│  ├─ Characters\
│  │  └─ EmberTrialCharacter.h       从原根目录移动
│  └─ Framework\
│     ├─ ETPlayerController.h        新建
│     └─ ETGameMode.h                新建
├─ Private\
│  ├─ Characters\
│  │  └─ EmberTrialCharacter.cpp     从原根目录移动
│  └─ Framework\
│     ├─ ETPlayerController.cpp      新建
│     └─ ETGameMode.cpp              新建
├─ EmberTrial.Build.cs               修改一行
├─ EmberTrialGameMode.cpp            修改一条 include，M0 类保留
├─ ETPracticeActionComponent.*       保留原位置
├─ ETPracticeActor.*                 保留原位置
└─ Tests\                           保留原位置
~~~

不要留下第二份 EmberTrialCharacter.h / .cpp；同名 UCLASS 的拷贝会导致 UHT / 编译重复定义。只移动本表的两个角色文件，练习组件与测试的相对 include 不改。

## 3. 将现有 Character 作为公开类型

### 3.1 移动两份文件

将：

- `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrialCharacter.h`
- `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrialCharacter.cpp`

分别移动到：

- `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h`
- `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp`

如果 CLion 弹出自动改 include 的选项，可取消自动重构，下面手动修改两处 include 即可。

### 3.2 导出 Character 类

打开移动后的头文件，只将：

~~~cpp
class AEmberTrialCharacter : public ACharacter
~~~

改为：

~~~cpp
class EMBERTRIAL_API AEmberTrialCharacter : public ACharacter
~~~

UCLASS、GENERATED_BODY、属性、函数与文件名全部保留；`EmberTrialCharacter.generated.h` 仍是最后一条 include。API 宏使其他模块可以使用此类的导出符号，不会把 private 字段变成 public。

移动文件不改变类的 `/Script/EmberTrial.EmberTrialCharacter` 身份，因此已有蓝图父类无需重新指定。

### 3.3 更新两处 include

下面两个 .cpp 的原 include `"EmberTrialCharacter.h"` 均改为：

~~~cpp
#include "Characters/EmberTrialCharacter.h"
~~~

两个文件的完整路径：

1. `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp`。
2. `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrialGameMode.cpp`。

其余代码保持原内容；特别是移动 / Jump 门控与 PracticeActionComponent 的查找。

### 3.4 给旧内部头文件保留私有搜索路径

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\EmberTrial.Build.cs`。

在现有构造函数中、PublicDependencyModuleNames.AddRange **之后**加入：

~~~csharp
PrivateIncludePaths.Add(ModuleDirectory);
~~~

参考最终构造函数：

~~~csharp
public EmberTrial(ReadOnlyTargetRules Target) : base(Target)
{
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicDependencyModuleNames.AddRange(new string[]
    {
        "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput"
    });

    PrivateIncludePaths.Add(ModuleDirectory);
}
~~~

移动后的角色 .cpp 仍有 `#include "ETPracticeActionComponent.h"`。该组件暂留核心根目录，所以用**私有**搜索路径解决内部引用。UI 后续使用 Public 契约；不要把根目录加入 PublicIncludePaths。Public / Private 目录与 C++ 访问权限分别处理模块边界和成员访问。[Epic：模块结构与依赖](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-modules?application_version=5.5)

## 4. PlayerController当前源码（历史接入）

### 4.1 ETPlayerController.h

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\ETPlayerController.h`。

当前源码：[ETPlayerController.h](../EmberTrial/Source/EmberTrial/Public/Framework/ETPlayerController.h)。

### 4.2 ETPlayerController.cpp

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETPlayerController.cpp`。

当前源码：[ETPlayerController.cpp](../EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp)。

每个 override 都保留 Super；输入 / 控制与引擎委托仍由父类流程执行。这里不新加 Tick、计时器或另一套 PawnChanged 事件。当前UI通过LocalPlayer观察层订阅UE已有的OnPossessedPawnChanged。

BeginPlay 和 Possess 的先后不作为本批固定契约；BeginPlay 中 Pawn=None 不能单独证明失败，最终以实际 Possess / 操控结果为准。记录旧 Pawn 的名称后再调用 Super，可以观察解绑，不把旧 Pawn 作为长期强引用保存。

## 5. 新建 GameMode

### 5.1 ETGameMode.h

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\ETGameMode.h`。

~~~cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ETGameMode.generated.h"

UCLASS()
class EMBERTRIAL_API AETGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AETGameMode();
    virtual void StartPlay() override;
};
~~~

### 5.2 ETGameMode.cpp

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETGameMode.cpp`。

~~~cpp
#include "Framework/ETGameMode.h"
#include "Framework/ETPlayerController.h"
#include "Characters/EmberTrialCharacter.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGameMode, Log, All);

AETGameMode::AETGameMode()
{
    PlayerControllerClass = AETPlayerController::StaticClass();
    DefaultPawnClass = AEmberTrialCharacter::StaticClass();
}

void AETGameMode::StartPlay()
{
    Super::StartPlay();

    UE_LOG(LogETGameMode, Display,
        TEXT("[M1-02B] Rules ready GameMode=%s ControllerClass=%s PawnClass=%s"),
        *GetNameSafe(this),
        *GetNameSafe(PlayerControllerClass.Get()),
        *GetNameSafe(DefaultPawnClass.Get()));
}
~~~

C++ 默认值是原生类型，下面会在 GameMode 蓝图中选择有 Mesh / 输入 / 动画配置的角色资产。新类没有写死蓝图路径；未来规则归 GameMode，Widget 创建归 UI 装配。Rules ready 日志只说明 StartPlay 已执行，不是后续 ASC 的 OnGameplayReady 事件。

## 6. 正常 Build 与索引刷新

保存全部文件，确认 UE 关闭。执行：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

这次不加 -SkipBuild，因为有新的 UCLASS 和目录。脚本先构建 Development Editor，再读取两个模块各自的元数据。

在 CLion 执行 **Reload CMake Project**，等待分析结束；确认新目录 / 文件可见。在新 GameMode .cpp 中将光标放进 include 文件名或 AEmberTrialCharacter 符号，Ctrl+B 应跳到新的 Public/Characters 头文件。

随后保持 UE 关闭，构建一次 Game：

~~~powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
~~~

本批无需 Cook / 打包或重复已有自动测试。若提示重复 UCLASS，先检查第 3 节是否把移动误做成了复制；若找不到 ETPracticeActionComponent.h，核对 3.4 的私有路径与实际文件位置。

## 7. Blueprint 装配

### 7.1 创建 BP_ETPlayerController

1. 正常打开 UE；Content Browser 创建目录 `/Game/EmberTrial/Blueprints/Framework`。
2. 在该目录空白处右键 → **Blueprint Class** → 展开 **All Classes**。
3. 搜索 **ETPlayerController**，选中新 C++ 父类，点击 Select。
4. 命名 **BP_ETPlayerController**；打开它，Class Settings 中确认父类为 ETPlayerController。
5. 本批不增加事件节点；Compile / Save。

资产路径：`/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`。这里为下一批 UI Bootstrap 组件保留装配位置。

### 7.2 创建 BP_ETGameMode

1. 同一目录新建 Blueprint Class → All Classes → **ETGameMode**，命名 **BP_ETGameMode**。
2. 打开后点击 **Class Defaults**，在 Details 搜索 / 展开 Classes。
3. 设置 **Player Controller Class = BP_ETPlayerController**。
4. 设置 **Default Pawn Class = BP_ThirdPersonCharacter**，确认选中现有 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`。
5. 其他 Classes 沿用父类默认，本批没有新的 GameState / PlayerState / HUD 类。Compile / Save。

资产路径：`/Game/EmberTrial/Blueprints/Framework/BP_ETGameMode`。

**必须完成第 4 项**：原生 AEmberTrialCharacter 默认没有蓝图中的 Mesh / 输入资产配置。角色资源由已有 Blueprint 提供。

### 7.3 让新地图采用新规则

1. 打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`。
2. **Window → World Settings → GameMode → GameMode Override**，从原 None 改为 **BP_ETGameMode**。
3. 检查该 GameMode 的 Default Pawn / Player Controller 值对应上面的两个选择；需要更改时打开 BP_ETGameMode 的 Class Defaults。
4. 打开现有 BP_ThirdPersonCharacter，Compile / Save；确认父类仍为 EmberTrialCharacter、PracticeAction 仍只挂一个。
5. File → Save All。此批只设置新地图的 Override，ThirdPersonMap 继续作为 M0 对照入口。

## 8. 最小验收

### 8.1 对象正确且原功能可用

1. 在新地图 Play，用 Output Log 搜索 `M1-02B`。应看到：
   - Rules ready：GameMode 是 BP_ETGameMode 的实例（例如 BP_ETGameMode_C_0）；ControllerClass 是 BP_ETPlayerController_C；PawnClass 是 BP_ThirdPersonCharacter_C。
   - Controller BeginPlay：Controller 是 BP_ETPlayerController 的实例，Local=1。
   - Possess：Controller 控制 BP_ThirdPersonCharacter 的实例。
2. 在 PIE Outliner 中可分别搜索上述对象名称，确认当前世界只有一个玩家 Controller / 玩家角色；不要额外手工摆放 Controller 或玩家角色。
3. 出生、WASD / 转向 / Space 正常。站定按 1 / 2 / 3，再做一次 2→3；靠墙闪避后能恢复控制。
4. A / B 靠近、E 激活、离开隐藏；P 鼠标继续，再 P 键盘继续；恢复后直接移动 / 跳跃。
5. Stop，确认 Controller EndPlay；再 Play 两次，每轮有一组对应的 Controller BeginPlay / Rules ready，状态能重置，无新增蓝图运行错误。日志顺序与实例后缀不要求完全一致。

停止 PIE 的路径未必产生单独的 UnPossess 日志，本批不强行要求它与 EndPlay 成对；后续会用实际 Pawn 更换场景验证重绑 / 解绑。

### 8.2 旧基线回归与保存

1. 切回 ThirdPersonMap，Play 一次；基础移动与 E/P 正常。
2. Stop，回到 L_ET_VerticalSlice，Save All。日常继续在新地图开发。

无需重复帧率 / 坡面矩阵、视频录制或打包。这里验证对象装配和文件迁移；ASC、HUD、重试尚未实现，不计入本批通过结论。

## 9. 反馈

~~~text
M1-02B 完成 / 卡在第 __ 节
角色 .h/.cpp 已移动，原位置无重复副本；类名与既有功能保留：是
角色 API / 两处 include / Build.cs 私有路径：已完成
新 GameMode / PlayerController 四个源码文件：已保存
Editor Build / 刷新索引 / Game Build：成功
CLion 新文件可见，角色公开头文件导航：通过
两个 Blueprint 父类 / Classes / 新地图 Override：正确
BP_ThirdPersonCharacter Compile，单个 PracticeAction：通过
新地图三次 Play；Framework 日志 / 原有功能：通过
ThirdPersonMap 回归；已回新地图并保存：通过
大致完成时间：今天 __:__，日志由你读取
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
~~~

报错优先提供第一条 Build 错误文字及上下文；Classes / Override 配置不确定时给截图。普通报错仍由我读取与定位、给出修正步骤，不自动直接修改 C++。

B 通过后进入 M1-02C：UI Bootstrap / 本地玩家生命周期与 Pawn 重绑。核心模块继续不依赖 UMG / EmberTrialUI；正式 Gameplay 就绪与 HUD 数据绑定在 M1-03 接入。
