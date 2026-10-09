# M1-02C1：本地 UI 生命周期与 Pawn 订阅

生成日期：2026-10-02；验收更新：2026-10-03。前置：[M1-02B 已验收](Reports/M1_02B_Verification.md)。**M1-02C1 已通过**：用户手动 30 min、构建 / 索引等待 4 min；源码与构建 / 生命周期日志已核对，完整测试按用户反馈登记。证据范围见 [验收报告](Reports/M1_02C1_Verification.md)。本页保留实施步骤，下一批为 [M1-02C2-01 Widget / Bootstrap](M1_02C2_01_WidgetBootstrap.md)。所有 C++ 继续由你手动实现。

## 1. 本批目标

原估计手动 **50–80 分钟**，实际 **30 分钟**，等待 **4 分钟**。建立 UETUILocalPlayerSubsystem：每个本地玩家拥有一份 UI 生命周期状态，跟踪当前 Controller / Pawn，切换时解绑旧订阅，EndPlay 时清理。C1 已完成这个数据与生命周期入口；C2 再创建 UI Bootstrap 和测试 Widget；M1-03 才接 ASC / GameplayReady / 正式 HUD。

本批不用创建新的可见 UI，原 E 提示与 P 菜单继续作为玩法基线。日志里的“ready”只说明本地 Subsystem 已初始化，不表示角色战斗属性已经就绪。

~~~mermaid
flowchart LR
    LP[LocalPlayer] -->|自动创建| S[UETUILocalPlayerSubsystem]
    LP -->|PlayerControllerChanged| S
    PC[当前 Controller] -->|OnPossessedPawnChanged| S
    PC -->|OnEndPlay| S
    Pawn[当前 Pawn] -->|OnEndPlay| S
    S -->|弱引用 / 查询 / 变化事件| Future[后续 Widget / Presenter]
~~~

### 为什么使用引擎回调

UE 5.5 已提供 LocalPlayer 的 Controller 变化回调；不需要在核心 Controller 中包含 UI 类。这样保留 **UI → Core** 的依赖方向，并覆盖 Controller 分配早于 / 晚于 Subsystem 初始化的情况：

- Initialize 时主动读取已有 Controller，补上“订阅前已经发生”的状态。
- PlayerControllerChanged 时重新绑定；绑定 Pawn 变化事件后立刻读取现有 Pawn。
- 相同对象不重复注册 / 广播；Controller 或 Pawn 结束时移除委托并清空观察状态。
- 使用弱引用观察 World Actor，无 Tick、轮询和延迟重试。

本机已检查的接口：[LocalPlayerSubsystem.h](D:/UE5/UE_5.5/Engine/Source/Runtime/Engine/Public/Subsystems/LocalPlayerSubsystem.h)、[ULocalPlayer 通知](D:/UE5/UE_5.5/Engine/Source/Runtime/Engine/Private/LocalPlayer.cpp:710)、[Controller 委托](D:/UE5/UE_5.5/Engine/Source/Runtime/Engine/Classes/GameFramework/Controller.h:60)、[Actor EndPlay 委托](D:/UE5/UE_5.5/Engine/Source/Runtime/Engine/Classes/GameFramework/Actor.h:2265)。本批不加入多人 / 手柄范围。

## 2. 操作前准备

1. UE Save All、退出 PIE、关闭 UE；CLion 保存全部文件。
2. 打开完整的 `E:\UE_Project\Demo\EmberTrial\CMakeLists.txt` 工程。
3. 下面用 **New → File** 创建两个普通文件，避免 UE 向导覆盖 CMake；若继续用 UE 创建类，按 [已有 CLion 指南第 4 节](CLion_UEClassCreation.md) 恢复模型。
4. 本批修改反射类和模块依赖，完成后必须正常 UBT Build，不依赖 Live Coding / Hot Reload。

新布局：

~~~text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\
├─ Public\
│  └─ Lifecycle\
│     └─ ETUILocalPlayerSubsystem.h      新建
├─ Private\
│  ├─ EmberTrialUIModule.cpp             保留
│  └─ Lifecycle\
│     └─ ETUILocalPlayerSubsystem.cpp    新建
└─ EmberTrialUI.Build.cs                 修改依赖
~~~

UETUILocalPlayerSubsystem 由引擎自动创建。**不用为它创建 Blueprint，也不把它作为组件添加到 Controller。** C2 的 UI Bootstrap 会另行创建并装配；核心 GameMode / Controller 的四个文件本批不改。

## 3. 修改 UI 模块依赖

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\EmberTrialUI.Build.cs`。替换为：

~~~csharp
using UnrealBuildTool;

public class EmberTrialUI : ModuleRules
{
    public EmberTrialUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EmberTrial"
        });
    }
}
~~~

公开头文件继承 Engine 类型，因此 Engine / CoreUObject 放在 Public 依赖；Core 的玩法契约仍是 UI 的私有依赖。本批不使用 UMG / Slate，C2 按实际类型再加入。核心 `EmberTrial.Build.cs` 不加入 UI 模块。

## 4. 新建头文件

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Lifecycle\ETUILocalPlayerSubsystem.h`。

~~~cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ETUILocalPlayerSubsystem.generated.h"

class AActor;
class APawn;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FETUIObservedPawnChanged,
    APawn*, PreviousPawn,
    APawn*, CurrentPawn);

UCLASS(BlueprintType)
class EMBERTRIALUI_API UETUILocalPlayerSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void PlayerControllerChanged(APlayerController* NewController) override;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    APlayerController* GetObservedController() const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    APawn* GetObservedPawn() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETUIObservedPawnChanged OnObservedPawnChanged;

private:
    void RebindController(APlayerController* NewController);
    void ClearController(const TCHAR* Reason);
    void RefreshObservedPawn(const TCHAR* Reason);
    void SetObservedPawn(APawn* NewPawn, const TCHAR* Reason);

    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    UFUNCTION()
    void HandleControllerEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

    UFUNCTION()
    void HandlePawnEndPlay(AActor* Actor, EEndPlayReason::Type Reason);

    TWeakObjectPtr<APlayerController> ObservedController;
    TWeakObjectPtr<APawn> ObservedPawn;
};
~~~

`generated.h` 是最后一条 include，由 UHT 生成。委托参数允许 nullptr，后续 Widget 必须将空 Pawn 当成正常状态处理。本批不暴露写入角色状态的接口。

## 5. 新建实现文件

完整路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Lifecycle\ETUILocalPlayerSubsystem.cpp`。

~~~cpp
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUILifecycle, Log, All);

void UETUILocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    UE_LOG(LogETUILifecycle, Display,
        TEXT("[M1-02C1] UI local ready LocalPlayer=%s"),
        *GetNameSafe(LocalPlayer));

    APlayerController* InitialController = nullptr;
    if (LocalPlayer && LocalPlayer->GetWorld())
    {
        InitialController = LocalPlayer->GetPlayerController(LocalPlayer->GetWorld());
    }
    RebindController(InitialController);
}

void UETUILocalPlayerSubsystem::Deinitialize()
{
    ClearController(TEXT("SubsystemDeinitialize"));

    UE_LOG(LogETUILifecycle, Display,
        TEXT("[M1-02C1] UI local deinitialized LocalPlayer=%s"),
        *GetNameSafe(GetLocalPlayer()));

    Super::Deinitialize();
}

void UETUILocalPlayerSubsystem::PlayerControllerChanged(APlayerController* NewController)
{
    Super::PlayerControllerChanged(NewController);
    RebindController(NewController);
}

APlayerController* UETUILocalPlayerSubsystem::GetObservedController() const
{
    return ObservedController.Get();
}

APawn* UETUILocalPlayerSubsystem::GetObservedPawn() const
{
    return ObservedPawn.Get();
}

void UETUILocalPlayerSubsystem::RebindController(APlayerController* NewController)
{
    if (!IsValid(NewController))
    {
        NewController = nullptr;
    }
    else if (NewController->GetLocalPlayer() != GetLocalPlayer())
    {
        UE_LOG(LogETUILifecycle, Warning,
            TEXT("[M1-02C1] Reject foreign Controller=%s"),
            *GetNameSafe(NewController));
        return;
    }

    if (ObservedController.Get() == NewController && !ObservedController.IsStale())
    {
        RefreshObservedPawn(TEXT("ControllerSnapshot"));
        return;
    }

    ClearController(TEXT("ControllerChanged"));

    if (!NewController)
    {
        return;
    }

    ObservedController = NewController;
    NewController->OnPossessedPawnChanged.AddUniqueDynamic(
        this, &UETUILocalPlayerSubsystem::HandlePossessedPawnChanged);
    NewController->OnEndPlay.AddUniqueDynamic(
        this, &UETUILocalPlayerSubsystem::HandleControllerEndPlay);

    UE_LOG(LogETUILifecycle, Display,
        TEXT("[M1-02C1] Controller bound Controller=%s"),
        *GetNameSafe(NewController));

    RefreshObservedPawn(TEXT("ControllerSnapshot"));
}

void UETUILocalPlayerSubsystem::ClearController(const TCHAR* Reason)
{
    APlayerController* PreviousController = ObservedController.Get();
    if (PreviousController)
    {
        PreviousController->OnPossessedPawnChanged.RemoveDynamic(
            this, &UETUILocalPlayerSubsystem::HandlePossessedPawnChanged);
        PreviousController->OnEndPlay.RemoveDynamic(
            this, &UETUILocalPlayerSubsystem::HandleControllerEndPlay);
    }

    ObservedController.Reset();
    SetObservedPawn(nullptr, Reason);

    if (PreviousController)
    {
        UE_LOG(LogETUILifecycle, Display,
            TEXT("[M1-02C1] Controller cleared Controller=%s Reason=%s"),
            *GetNameSafe(PreviousController), Reason);
    }
}

void UETUILocalPlayerSubsystem::RefreshObservedPawn(const TCHAR* Reason)
{
    APlayerController* Controller = ObservedController.Get();
    SetObservedPawn(Controller ? Controller->GetPawn() : nullptr, Reason);
}

void UETUILocalPlayerSubsystem::SetObservedPawn(APawn* NewPawn, const TCHAR* Reason)
{
    if (!IsValid(NewPawn))
    {
        NewPawn = nullptr;
    }

    APawn* PreviousPawn = ObservedPawn.Get();
    if (PreviousPawn == NewPawn && !ObservedPawn.IsStale())
    {
        return;
    }

    if (PreviousPawn)
    {
        PreviousPawn->OnEndPlay.RemoveDynamic(
            this, &UETUILocalPlayerSubsystem::HandlePawnEndPlay);
    }

    ObservedPawn = NewPawn;
    if (NewPawn)
    {
        NewPawn->OnEndPlay.AddUniqueDynamic(
            this, &UETUILocalPlayerSubsystem::HandlePawnEndPlay);
    }

    UE_LOG(LogETUILifecycle, Display,
        TEXT("[M1-02C1] Pawn changed Previous=%s Current=%s Reason=%s"),
        *GetNameSafe(PreviousPawn), *GetNameSafe(NewPawn), Reason);

    OnObservedPawnChanged.Broadcast(PreviousPawn, NewPawn);
}

void UETUILocalPlayerSubsystem::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    (void)OldPawn;
    (void)NewPawn;
    RefreshObservedPawn(TEXT("PossessionChanged"));
}

void UETUILocalPlayerSubsystem::HandleControllerEndPlay(
    AActor* Actor, EEndPlayReason::Type Reason)
{
    (void)Reason;
    if (Actor == ObservedController.Get())
    {
        ClearController(TEXT("ControllerEndPlay"));
    }
}

void UETUILocalPlayerSubsystem::HandlePawnEndPlay(
    AActor* Actor, EEndPlayReason::Type Reason)
{
    (void)Reason;
    if (Actor == ObservedPawn.Get())
    {
        SetObservedPawn(nullptr, TEXT("PawnEndPlay"));
    }
}
~~~

### 5.1 几处实现的目的

- 同一 Controller 的再次通知只刷新快照，不重复 Add 委托；同一 Pawn 的快照不重复广播。
- 旧 Actor 的委托先移除，再保存新对象；nullptr 是角色暂时不存在时的合法值。
- Pawn 销毁可能先触发 EndPlay，也可能先发生 UnPossess；两条路径都调用 SetObservedPawn，指针相同会跳过重复广播。
- `IsStale` 用来区别“从未观察对象”与“弱引用曾指向对象但对象已失效”，使失效状态也能被清空。
- Pawn 回调重新读取 Controller 的实际 Pawn；委托携带的 OldPawn / NewPawn 不作为额外长期状态存储。
- 本地玩家拥有 Subsystem；它观察 Actor 的生命周期，不延长 Actor 的存活。暂时只处理 Pawn 是否存在，后续正式属性绑定还需要 GameplayReady 契约。

## 6. 正常 Build / 索引与基础验证

保存源码，确认 UE 关闭，执行：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

本次不加 -SkipBuild。完成后 CLion **Ctrl+Shift+A → Reload CMake Project**，确认两份新文件属于 EmberTrialUICodeModel，能导航到 LocalPlayerSubsystem.h。

随后保持 UE 关闭，构建 Game：

~~~powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
~~~

打开 UE / L_ET_VerticalSlice，Play。Output Log 搜索 **M1-02C1**，应观察到：

~~~text
UI local ready LocalPlayer=...
Controller bound Controller=BP_ETPlayerController_C_...
Pawn changed Previous=None Current=BP_ThirdPersonCharacter_C_... Reason=...
~~~

这些日志的相对顺序与 Reason 允许不同：初始 Pawn 既可能由快照读到，也可能在随后的 Possess 回调读到。**需要的是最终跟踪到正确 Controller / Pawn，且同一状态不重复广播。**

试一次移动 / 转向 / 跳跃、1/2/3、2→3、E-A/B 和 P 鼠标 / 键盘继续。Stop，看到 Pawn 清空、Controller cleared 和 UI local deinitialized；Reason / 先后顺序不作为固定契约。LocalPlayer 名称和实例后缀不用与示例一致。

若日志完全没有 UI local ready，先提供完成时间与 Build 结果；我读取注册 / 模块加载 / 日志定位，不通过额外 Tick 轮询掩盖缺失初始化。

## 7. 临时 Pawn 生命周期测试

仅在 BP_ETPlayerController 添加下述测试节点，验收后删除。不要写到角色 Blueprint；UnPossess 后角色将不能接收按键，但 Controller 仍存在。当前只测试**同一身体失去 / 恢复控制**，真正的新 Pawn 和 Widget 重绑在 C2 / 后续阶段验证。

### 7.1 临时变量

在 BP_ETPlayerController 创建变量 **ETTempPawn**：

- 类型：**Pawn Object Reference**。
- 默认：None。
- 不勾 Instance Editable / Expose on Spawn。

### 7.2 顶部数字 4：保存 Pawn 再 UnPossess

在 Event Graph 右键搜索键盘 **4**（不是 NumPad Four），使用 **Released** 白线。

~~~text
4 Released
  → Is Valid（有白线的执行版本；Input Object 接 Get Controlled Pawn 的 Return Value，Target = Self）
     Is Valid → Set ETTempPawn（值 = Get Controlled Pawn 返回值）
              → Un Possess（Target = Self）
     Is Not Valid → 不连接
~~~

上图表示执行白线。另放一个 **Get Controlled Pawn（Target = Self）**，其 Return Value 同时连到 Is Valid 的 Input Object 和 Set ETTempPawn 的值输入。Get Controlled Pawn 是纯查询节点，没有白色执行引脚；ETTempPawn 的 Get 节点同样只有数据输出。无 Pawn 时再次按 4 不覆盖已保存变量。

### 7.3 顶部数字 5：Possess 保存的 Pawn

~~~text
5 Released
  → Is Valid（Input Object = Get ETTempPawn）
     Is Valid → Branch（当前 Controlled Pawn == ETTempPawn）
                  True → 不连接（已经控制，不重复调用）
                  False → Possess（Target = Self，In Pawn = ETTempPawn）
     Is Not Valid → 不连接
~~~

Branch 的 Condition 数据线这样连接：

1. 放一个 Get Controlled Pawn，Target 保持 Self。
2. 放一个 Get ETTempPawn。
3. 从 Pawn 数据引脚拖出，搜索 `==`，选择比较对象的 **Equal (Object)**；两个输入分别连接上述 Pawn 值，bool 输出连到 Branch 的 Condition。
4. Is Valid 的有效白线连 Branch，Branch 的 False 白线连 Possess；Possess 的 In Pawn 接 ETTempPawn，Target 保持 Self。

这道比较不能省略。本机 UE 5.5 的 [PlayerController::OnPossess](D:/UE5/UE_5.5/Engine/Source/Runtime/Engine/Private/PlayerController.cpp:854) 会让传入 Pawn 的已有 Controller 先 UnPossess；对已经控制的同一个 Pawn 重复调用，会触发额外解绑 / 重启，不能用它来模拟“重复读取同一状态”。本测试通过 Branch 跳过这类重复命令；真正的 4→5 仍会触发失去 / 恢复控制。

### 7.4 顶部数字 6：销毁当前 Pawn

~~~text
6 Released
  → Is Valid（Input Object = Get Controlled Pawn，Target = Self）
     Is Valid → Destroy Actor（Target = Get Controlled Pawn）
     Is Not Valid → 不连接
~~~

Compile / Save，PIE 点击一次游戏视口确保获得输入。**测试时角色站定、动作已结束、菜单关闭**，不要把这些临时按键接进正式输入资产。

### 7.5 验证与清理

1. 正常出生后按 4：Pawn changed 的 Current=None；角色停止接受控制。再次按 4 不新增相同 Pawn 状态记录。
2. 按 5：Current 回到保存的角色；基础移动 / 转向 / 跳跃恢复。再次按 5 走第 7.3 节 Branch 的 True，不再次调用 Possess，不新增 Pawn 状态记录。
3. 再做 **4→5 三轮**。每次失去 / 恢复控制各发生一次状态转换，日志不按轮数累计成多条相同通知。
4. 已恢复控制后按 6：Pawn 销毁，观察状态清空，没有 Accessed None。按 5 不尝试 Possess 已销毁对象；该对象 Is Valid 为 false 属于预期。
5. Stop / 重新 Play，角色重新正常出生。再 Play / Stop 两次，初始化 / 清理可重复，旧对象不出现在下一轮观察状态里。
6. Stop，**删除 4 / 5 / 6 的所有临时事件与调用节点、ETTempPawn 变量**，Compile / Save。
7. 清理后的最终 Play 再做一次 1/2/3、2→3、E / P、移动 / 转向 / 跳跃；Stop / Save All。

没有新增 Pawn changed 日志本身不能说明清理失败：没有 Pawn 时停止，SetObservedPawn 的同值保护会跳过广播。销毁的事件顺序不必固定，最终为空且不重复才是验收依据。

本批无需 Cook、重新录像或重复闪避帧率矩阵。若重夺控制后原 E/P 或输入异常，给出复现步骤，我读取既有角色 Blueprint / 日志，修复方案仍由你手动实现，除非明确要求接管。

## 8. 反馈

~~~text
M1-02C1 完成 / 卡在第 __ 节
UI Build.cs / 两个 Subsystem 文件已手动保存：是
Editor Build / 索引刷新 / Game Build：成功
CLion UI 模块目标 / 引擎头文件导航：通过
首次 UI local ready / Controller bound / 正确 Pawn：通过
4 / 5 失去和恢复控制；重复按键无重复状态：通过
三轮 4→5、恢复移动 / 转向 / 跳跃：通过
6 销毁清空 / 销毁后 5 不执行 / 无 Accessed None：通过
三次 PIE 初始化 / 清理与下一轮对象：通过
临时节点和 ETTempPawn 已删除；Compile / Save：是
最终原 1/2/3、2→3、E/P 与基础输入：正常
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
~~~

无需逐条手抄正常日志；提供完成时间，我读取本地日志。C1 通过后再下发 C2 的 Widget / Bootstrap 配置与重绑显示，M1-02 总任务在这些子批完成后验收。
