# M1-03A1：在现有 Character 上接入 ASC

生成 / 验收日期：2026-10-03。**A1 已通过，用户约20:43完成，手动45 / 等待3分钟。** 证据见 [验收报告](Reports/M1_03A1_Verification.md)，下一批按 [A2-01生命属性 / GameplayReady](M1_03A2_01_HealthReady.md) 操作。下文保留原交接步骤供复查，不重做已通过项；当前实施以实际源码和验收报告为准。

## 1. 范围、效果与后续批次

本批在 **现有 AEmberTrialCharacter / BP_ThirdPersonCharacter** 上增加一个原生 AbilitySystemComponent，提供 IAbilitySystemInterface，并处理 BeginPlay、控制变化和 EndPlay 的 ActorInfo。

最终表现：原角色多一个继承的 AbilitySystem；每次出生记录正确 Owner / Avatar；临时失去 / 恢复控制刷新 Controller，ASC 实例保持相同；原动作、交互 / 暂停与生命周期面板继续正常。

M1-03 依次推进：

| 批次 | 内容 |
|---|---|
| **A1，本页** | GAS 插件 / 依赖、ASC 所有权与 ActorInfo；不新增原生类 |
| A2，后续下发 | Health / MaxHealth AttributeSet、基础属性初始化 / GameplayReady、最小 Effect 验证 |
| B，后续下发 | Presenter、HUD 立即快照 / 属性事件 / 解绑与对象替换 |
| C，后续下发 | 属性边界和完整数据流回归，形成 M1-03 总验收 |

本页还没有生命值、伤害能力或血条。ActorInfo ready 日志表示运行时关联正确，**不是 OnGameplayReady**。取舍见 [ADR-002](ADR/ADR_002_ASCOnCharacter.md)。GAS 中 ASC、属性与 Gameplay Effect 的职责见 [Epic 概览](https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system-for-unreal-engine)。

## 2. 开始前与修改清单

1. UE 退出 PIE / Save All，**关闭 UE 编辑器**；CLion 保存所有文件。
2. 保留现有角色父类、PracticeAction、UIBootstrap、Widget 和全部资产。
3. 本批只改以下四个文件；不要通过 UE 创建新的 Character 或 ASC 类：

| 文件 | 修改 |
|---|---|
| [EmberTrial.uproject](E:/UE_Project/Demo/EmberTrial/EmberTrial.uproject) | 启用 GameplayAbilities 插件，保留两个项目模块 |
| [EmberTrial.Build.cs](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/EmberTrial.Build.cs) | GAS 依赖，保留原输入依赖 / 私有路径 |
| [EmberTrialCharacter.h](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h) | 接口 / ASC 字段 / 生命周期声明 |
| [EmberTrialCharacter.cpp](E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp) | 创建 / 关联刷新 / 结束清理 |

**UI Build.cs、两个 Target、C1 / C2 UI 源码均不改。** 先做完第 3–5 节再正常构建，不用 Live Coding / Hot Reload 处理新的反射属性与原生子对象。

## 3. 插件与模块依赖

### 3.1 修改 .uproject 的 Plugins

只替换原文件的 `Plugins` 数组为以下内容；`FileVersion`、`EngineAssociation`、`Modules` 等保持原样：

~~~json
"Plugins": [
    {
        "Name": "ModelingToolsEditorMode",
        "Enabled": true,
        "TargetAllowList": ["Editor"]
    },
    {
        "Name": "GameplayAbilities",
        "Enabled": true
    }
]
~~~

这是 JSON 的属性片段，不是完整文件。两个插件对象之间有逗号；不要把 GameplayAbilities 放进项目 Modules，不给它加 Editor 限制。GameplayTags / GameplayTasks 是模块依赖，不需要在这里增加同名插件。

### 3.2 修改核心 Build.cs

当前核心文件较短，可替换为下面全文；不要覆盖 UI 模块文件：

~~~csharp
using UnrealBuildTool;

public class EmberTrial : ModuleRules
{
    public EmberTrial(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "GameplayAbilities"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "GameplayTags", "GameplayTasks"
        });

        PrivateIncludePaths.Add(ModuleDirectory);
    }
}
~~~

GameplayAbilities 为公开依赖，因为公开角色头文件继承它的接口；Tags / Tasks 在核心内部使用。核心没有加入 UI / UMG 依赖。

## 4. 角色头文件：准确插入 / 替换

编辑 `Public/Characters/EmberTrialCharacter.h`，以下修改都保留原字段和函数。

### 4.1 include / 前向声明

在 `#include "CoreMinimal.h"` 后添加：

~~~cpp
#include "AbilitySystemInterface.h"
~~~

`#include "EmberTrialCharacter.generated.h"` 仍必须是最后一条 include。在现有 `class UETPracticeActionComponent;` 附近添加：

~~~cpp
class UAbilitySystemComponent;
~~~

### 4.2 类继承

把原 `class ... : public ACharacter` 一行替换为：

~~~cpp
class EMBERTRIAL_API AEmberTrialCharacter : public ACharacter, public IAbilitySystemInterface
~~~

UCLASS / GENERATED_BODY 不变，不修改类名 / API 宏，不改 Blueprint Parent。

### 4.3 私有字段与函数

**紧接原 PracticeActionComponent 字段之后、现有第一个 public: 之前**插入：

~~~cpp
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Abilities",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    bool bAbilitySystemShuttingDown = false;

    void RefreshEmberAbilityActorInfo(const TCHAR* Reason);
~~~

此处原本就是 class 的私有区域。ASC 通过 UPROPERTY 持有；普通 bool 是结束阶段保护，不是玩法状态 / 配置，也不需要 Blueprint 访问。

### 4.4 public 接口

在现有 `AEmberTrialCharacter();` 后添加：

~~~cpp
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
~~~

这里实现 C++ 接口，不加 UFUNCTION。第 8 节用引擎的 Blueprint Library 节点读取 ASC。

### 4.5 protected 生命周期

在现有 `virtual void BeginPlay() override;` 后添加：

~~~cpp
    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
~~~

保留原 SetupPlayerInputComponent、Move / Look / Jump 等声明；不要重复写 BeginPlay。

## 5. 角色 .cpp

### 5.1 include 与日志

在现有角色头文件 include 后添加下面三条；保留所有原 include：

~~~cpp
#include "AbilitySystemComponent.h"
#include "GameplayAbilityTypes.h"
#include "GameFramework/PlayerController.h"
~~~

在现有 `DEFINE_LOG_CATEGORY(LogTemplateCharacter);` 之后添加：

~~~cpp
DEFINE_LOG_CATEGORY_STATIC(LogETAbilityFoundation, Log, All);
~~~

### 5.2 构造函数增加原生子对象

在 `AEmberTrialCharacter::AEmberTrialCharacter()` 的左大括号后、原 Capsule 设置前插入：

~~~cpp
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
    AbilitySystemComponent->SetIsReplicated(false);
~~~

其余构造代码保留。单人路线不启用 ASC 网络复制；不关闭 ASC 的 Tick，不设置复制模式，不在 BeginPlay 再 NewObject 一个组件。

### 5.3 替换 BeginPlay

原 BeginPlay 只有 Super 与 PracticeAction 查找；替换该函数全文为：

~~~cpp
void AEmberTrialCharacter::BeginPlay()
{
    bAbilitySystemShuttingDown = false;
    Super::BeginPlay();
    PracticeActionComponent = FindComponentByClass<UETPracticeActionComponent>();
    RefreshEmberAbilityActorInfo(TEXT("BeginPlay"));
}
~~~

保留 FindComponentByClass，避免破坏既有输入门控。

### 5.4 新增接口、刷新与结束函数

把下面整段加在 BeginPlay 后、原 Input 区域前；不要放进另一个函数的括号里：

~~~cpp
UAbilitySystemComponent* AEmberTrialCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent.Get();
}

void AEmberTrialCharacter::RefreshEmberAbilityActorInfo(const TCHAR* Reason)
{
    if (bAbilitySystemShuttingDown || IsActorBeingDestroyed())
    {
        return;
    }

    if (!IsValid(AbilitySystemComponent) || !AbilitySystemComponent->AbilityActorInfo.IsValid())
    {
        UE_LOG(LogETAbilityFoundation, Error,
            TEXT("[M1-03A1] Missing ASC or ActorInfo Character=%s Reason=%s"),
            *GetNameSafe(this), Reason);
        return;
    }

    const bool bNeedsInit =
        AbilitySystemComponent->AbilityActorInfo->OwnerActor.Get() != this ||
        AbilitySystemComponent->AbilityActorInfo->AvatarActor.Get() != this;

    if (bNeedsInit)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }
    else
    {
        AbilitySystemComponent->RefreshAbilityActorInfo();
    }

    UE_LOG(LogETAbilityFoundation, Display,
        TEXT("[M1-03A1] ActorInfo ready Character=%s Reason=%s Mode=%s ASC=%s Owner=%s Avatar=%s Controller=%s"),
        *GetNameSafe(this), Reason, bNeedsInit ? TEXT("Init") : TEXT("Refresh"),
        *GetNameSafe(AbilitySystemComponent.Get()),
        *GetNameSafe(AbilitySystemComponent->GetOwnerActor()),
        *GetNameSafe(AbilitySystemComponent->GetAvatarActor()),
        *GetNameSafe(AbilitySystemComponent->AbilityActorInfo->PlayerController.Get()));
}

void AEmberTrialCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (HasActorBegunPlay())
    {
        RefreshEmberAbilityActorInfo(TEXT("PossessedBy"));
    }
}

void AEmberTrialCharacter::UnPossessed()
{
    Super::UnPossessed();
    if (HasActorBegunPlay())
    {
        RefreshEmberAbilityActorInfo(TEXT("UnPossessed"));
    }
}

void AEmberTrialCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bAbilitySystemShuttingDown = true;

    if (IsValid(AbilitySystemComponent) && AbilitySystemComponent->AbilityActorInfo.IsValid())
    {
        AbilitySystemComponent->CancelAllAbilities();
        AbilitySystemComponent->RemoveAllGameplayCues();
        AbilitySystemComponent->ClearActorInfo();

        UE_LOG(LogETAbilityFoundation, Display,
            TEXT("[M1-03A1] ActorInfo cleared Character=%s Reason=%d"),
            *GetNameSafe(this), static_cast<int32>(EndPlayReason));
    }

    Super::EndPlay(EndPlayReason);
}
~~~

理解这段代码：

- UE 5.5.4 的 ASC InitializeComponent 已默认绑定 Owner / Avatar，所以首次项目日志通常是 **Mode=Refresh**；不要求一定出现 Init。
- 首次 Possess 可能在 BeginPlay 前发生：此时不提前操作 ASC，BeginPlay 会补齐。运行中控制变化在 Super 之后刷新 Controller。
- UnPossess 只更新 Controller 关联，Owner / Avatar 仍是同一个 Character。属性 / 能力后续不会在这里重置或重新授予。
- EndPlay 先设结束保护，再清理；Super 期间可能发生 UnPossess，保护阻止它重新建立 ActorInfo。本批还没有授予能力，Cancel 当前为空。
- 没有 Delay、逐帧初始化或新的 Blueprint BeginPlay 接线；没有创建 GameplayReady 假事件。

核对依据：[接口](D:/UE5/UE_5.5/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Public/AbilitySystemInterface.h:28)、[ASC 默认初始化](D:/UE5/UE_5.5/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:57)、[关联刷新 / 清理](D:/UE5/UE_5.5/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent_Abilities.cpp:234)、[Controller 查找](D:/UE5/UE_5.5/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/GameplayAbilityTypes.cpp:21)。API 阅读不代表本批 Build 已通过。

## 6. 保存、正常 Build 与索引

确认四份文件保存、UE 关闭，在 PowerShell 执行：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

**不加 -SkipBuild。** Editor Build 成功后，CLion Reload CMake Project；现有角色仍属于 EmberTrialCodeModel，AbilitySystemInterface.h / AbilitySystemComponent.h 能导航。

再保持 UE 关闭，执行正常 Game Build：

~~~powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
~~~

首次启用 GAS 可能增加编译 / 链接等待，单独记录。失败时先复制第一个具体 error 及上下文，不只贴最终 exit code；不要删除缓存或更换引擎来处理普通代码错误。

## 7. 编辑器检查与首次运行

1. 两个 Build 成功后打开 UE，Plugins 搜索 Gameplay Abilities，确认 Enabled；仅检查，不再次切换开关。
2. 打开 BP_ThirdPersonCharacter：父类仍为 EmberTrialCharacter，Components 中应有**一个继承的 AbilitySystem**。不要通过 Add Component 手动增加第二个 ASC。
3. 原 PracticeAction 仍只有一个；BP_ETPlayerController 的 UIBootstrap / WidgetClass 保持正确。Compile / Save 角色 Blueprint。
4. 打开 L_ET_VerticalSlice，Play：搜索 `M1-03A1`，每个原角色实例有一条 Reason=BeginPlay 的 ActorInfo ready。
5. Owner / Avatar 均为当前 BP_ThirdPersonCharacter 实例；Controller 为 BP_ETPlayerController，ASC=AbilitySystem。没有 Missing ASC or ActorInfo。
6. 状态面板仍一份，原移动 / 转向 / 跳跃、1/2/3 / 打断、E/P 简单回归。Stop 时有 ActorInfo cleared；结束保护后不应因清理再次出现 ready。

UI 初始快照可能早于 ActorInfo 日志，这是正常时序。本页面板只显示 Pawn；等 A2/B 接入明确就绪与数据，不用人为 Delay 对齐日志。

## 8. 临时验证失去 / 恢复控制

在 BP_ETPlayerController 添加 **ETTempASCPawn，Pawn Object Reference，默认 None**。本批顶部数字 **4 / 5 / 8** 均使用 Released；不改 Enhanced Input 资产。

### 8.1 数字 4：缓存再 UnPossess

`4 Released → Is Valid → Set ETTempASCPawn → UnPossess`。

- Is Valid 的 **Input Object 接 Get Controlled Pawn，Target=Self 的 Return Value**。
- Is Valid 分支才执行；Is Not Valid 不接后续。
- Set ETTempASCPawn 的值接同一个 Get Controlled Pawn 返回值。
- UnPossess 的 Target=Self。先 Set 再 UnPossess；重复 4 当前为空，不覆盖缓存。

### 8.2 数字 5：不同才恢复

`5 Released → Is Valid → Branch → Possess`。

- Is Valid 的 Input Object=Get ETTempASCPawn；无效分支不执行。
- Condition=Equal (Object)：Get Controlled Pawn（Target=Self）与 Get ETTempASCPawn。
- **True 不接后续；False 才 Possess。** Possess Target=Self，In Pawn=Get ETTempASCPawn。

### 8.3 数字 8：读取 ASC

`8 Released → Is Valid（当前 Pawn）→ Is Valid（ASC）→ Print String`。

- 第一个 Input Object=Get Controlled Pawn，Target=Self。
- 搜索引擎的 **Get Ability System Component**，选带 **Actor** 引脚的纯节点；Actor 接当前 Pawn。
- Return Value 接第二个 Is Valid 的 Input Object，并接 Get Object Name 的 Object。
- 第二个有效分支 Print String：Append A=`[M1-03A1] Blueprint ASC=`，B=ASC 的 Get Object Name；Print to Log=True。
- 任一无效分支可以 Print String `[M1-03A1] Blueprint no current ASC`，Print to Log=True；不调用创建 / 初始化 / Clear。

这是 Blueprint Library 的查询节点，角色的 C++ 接口 getter 不会直接作为角色 Blueprint 节点出现。Compile / Save 后：

1. Play，按 8：返回有效 AbilitySystem。
2. 按 4：面板 Pawn=None；ActorInfo ready Reason=UnPossessed，**Owner / Avatar 仍是原角色、Controller=None**。空 Pawn 的 8 被拒绝，不能 Accessed None。
3. 按 5：控制 / 面板恢复，Reason=PossessedBy、Controller 恢复；再按 8，仍是原角色的 AbilitySystem。
4. 再做两轮 4→5，共三轮；重复 5 不重绑，没有再次 Reason=BeginPlay，不产生新 ASC。
5. 正常控制时 Stop，再 Play / Stop 两次，总共三次 PIE：每轮出生 / 清理正确，无旧世界 ActorInfo / Widget 事件或错误。

ASC 的名称在不同 World 可以相同，不据此判断跨 PIE 是同一实例；同一轮里的归属和控制变化才是这里的判断依据。无需重做整套 Probe 测试或闪避帧率矩阵。

## 9. 清理与反馈

通过后 Stop；删除 Controller 的 4/5/8 临时事件 / 查询 / Print 节点与 ETTempASCPawn，Compile / Save。**保留原生 AbilitySystem、PracticeAction、UIBootstrap 和 Widget。**

回新地图 Play，一份面板，原 1/2/3、2→3、E-A/B、P 鼠标 / Enter / Space / 背景关闭、无补播 / 卡键、移动 / 转向 / 跳跃正常。Stop / Save All。

~~~text
M1-03A1 完成 / 卡在第 __ 节
.uproject GAS 插件 / 核心 Build.cs / 角色 .h .cpp 已保存：是
关闭 UE 后 Editor Build / 索引刷新 / Game Build：成功
CLion 角色目标 / 两个 GAS 头文件导航：通过
原角色父类保持，单个继承 AbilitySystem / 单个 PracticeAction：是
首次 ActorInfo ready；Owner / Avatar / Controller 正确：通过
8 查询 ASC / 空 Pawn 拒绝无错误：通过
三轮 4→5；Controller None→恢复，ASC 归属不变：通过
重复 5 无重绑 / 无重复 BeginPlay：通过
三次 PIE 清理 / 无结束后重新 ready / 无错误：通过
临时 4/5/8 与 ETTempASCPawn 已删除；Compile / Save：是
最终原 1/2/3、2→3、E/P、移动 / 转向 / 跳跃：正常
源码 / 资产已保存：是
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
~~~

错误优先发文字原文、对应章节与操作顺序；Blueprint 连线 / 属性用包含相关引脚的截图。我会主动读本地日志定位，修正代码继续写进本页供你执行；只有你明确要求时才直接修改源码。
