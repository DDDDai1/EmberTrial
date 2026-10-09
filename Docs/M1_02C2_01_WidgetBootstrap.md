# M1-02C2-01：生命周期状态 Widget 与 UI Bootstrap

生成日期：2026-10-03；验收更新：2026-10-03。前置：[M1-02C1 已验收](Reports/M1_02C1_Verification.md)。**C2-01 已通过，用户约 19:02 完成；手动 40 分钟、构建 / 索引等待 5 分钟。** DisableNativeTick 已手动补上，正常双目标 Build / Widget Compile 警告消失，完整实机矩阵通过。证据见 [验收报告](Reports/M1_02C2_01_Verification.md)；下一批按 [C2-02 新 Pawn 替换验证](M1_02C2_02_PawnReplacement.md) 执行。下文实现 / 补修步骤保留供复查，无需重做已通过项。

## 1. 本批目标

预计手动 **70–110 分钟**，构建 / 索引等待另计。C2 分两批：本页先完成一个只读状态面板、Controller 上的 UI Bootstrap、失去 / 恢复控制及 Widget 重建测试；C2-02 再验证真正的新 Pawn 替换，完成 M1-02 总验收。

运行效果：屏幕右上角固定显示“本地 UI 生命周期”、控制状态及当前 Pawn 实例名。按临时 4 / 5 时状态立即改变；按临时 7 重建面板，仍显示当前状态，且始终只有一份面板。原 E 提示与 P 菜单继续工作。它是开发期验证面板，正式 HUD / 战斗属性在 M1-03 接入，不能把“有 Pawn”称为 GameplayReady。

~~~mermaid
flowchart LR
    BP[BP_ETPlayerController 装配] --> B[UETUIBootstrapComponent]
    B -->|创建 / 清理| W[UETLifecycleStatusWidget]
    W -->|订阅 + 立即快照| S[C1 LocalPlayerSubsystem]
    S -->|Pawn 变化| W
    W -->|SetText| Text[UMG 两个 TextBlock]
~~~

分工：

- Bootstrap 负责本地条件、Widget 类配置、创建 / 移除；不存储 Pawn，也不参与控制 / 暂停。
- Widget 负责订阅 / 解绑 C1、读取当前 Pawn、事件驱动 SetText；不修改玩法。
- C1 继续独立观察 Controller / Pawn。核心模块不依赖 UI。
- Blueprint 负责布局、配色及在 Controller 上选择类；不用 Tick、Text 的 Bind 函数、Delay 重试、全局 Get Player Controller 0 或 Cast 到具体角色。

## 2. 操作前准备与文件布局

1. UE Save All，退出 PIE，关闭 UE；CLion 保存全部。
2. 打开完整 `E:\UE_Project\Demo\EmberTrial\CMakeLists.txt` 工程。
3. 使用 CLion **New → File** 创建四个文件。若用 UE 新建类覆盖了 CMake，按 [现有 CLion 指南](CLion_UEClassCreation.md) 恢复；不要清缓存。
4. C1 的两个文件、模块入口、核心 GameMode / Controller 保留，不修改。

~~~text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\
├─ Public\
│  ├─ Lifecycle\ETUILocalPlayerSubsystem.h       保留
│  ├─ Bootstrap\ETUIBootstrapComponent.h        新建
│  └─ Widgets\ETLifecycleStatusWidget.h          新建
├─ Private\
│  ├─ EmberTrialUIModule.cpp                    保留
│  ├─ Lifecycle\ETUILocalPlayerSubsystem.cpp     保留
│  ├─ Bootstrap\ETUIBootstrapComponent.cpp      新建
│  └─ Widgets\ETLifecycleStatusWidget.cpp        新建
└─ EmberTrialUI.Build.cs                       修改
~~~

## 3. 修改 UI Build.cs

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
            "Engine",
            "UMG"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EmberTrial"
        });
    }
}
~~~

公开 Widget 头文件继承 UUserWidget，所以 UMG 是 Public 依赖。本批只用 UMG 包装类，不直接写 Slate 类型；不额外加入 Slate / SlateCore，也不在核心 Build.cs 加入 UI / UMG。

## 4. Widget 完整代码

### 4.1 ETLifecycleStatusWidget.h

路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETLifecycleStatusWidget.h`。

~~~cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETLifecycleStatusWidget.generated.h"

class APawn;
class UTextBlock;
class UETUILocalPlayerSubsystem;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETLifecycleStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // 主动解绑，不等待 Slate 销毁或 GC。
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void RefreshStatus(const TCHAR* Reason);

    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PawnText;

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    bool bObserving = false;
};
~~~

两个 BindWidget 字段要求 Blueprint 中存在**同名、同类型**控件。它们由 UHT / UMG 装配，不用在 C++ 创建 TextBlock。

DisableNativeTick 告诉 UE 这个原生 Widget 不需要 NativeTick；本类仅在 Construct 与 Pawn 事件发生时更新，不实现 NativeTick。原稿遗漏此标记，已于 2026-10-03 补正；已有源码需按第 7.2.1 节手动补上。

### 4.2 ETLifecycleStatusWidget.cpp

路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETLifecycleStatusWidget.cpp`。

~~~cpp
#include "Widgets/ETLifecycleStatusWidget.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETLifecycleWidget, Log, All);

#define LOCTEXT_NAMESPACE "ETLifecycleStatusWidget"

void UETLifecycleStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    StopObserving();

    if (IsDesignTime())
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    UETUILocalPlayerSubsystem* Subsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>()
        : nullptr;

    if (Subsystem)
    {
        BoundSubsystem = Subsystem;
        Subsystem->OnObservedPawnChanged.AddUniqueDynamic(
            this, &UETLifecycleStatusWidget::HandleObservedPawnChanged);
        bObserving = true;

        UE_LOG(LogETLifecycleWidget, Display,
            TEXT("[M1-02C2] Widget subscribed Widget=%s LocalPlayer=%s"),
            *GetNameSafe(this), *GetNameSafe(LocalPlayer));
    }
    else
    {
        UE_LOG(LogETLifecycleWidget, Warning,
            TEXT("[M1-02C2] Missing local subsystem Widget=%s"),
            *GetNameSafe(this));
    }

    // 订阅后立即读取快照，覆盖 Widget 晚于 Possess 创建的情况。
    RefreshStatus(TEXT("ConstructSnapshot"));
}

void UETLifecycleStatusWidget::NativeDestruct()
{
    StopObserving();
    Super::NativeDestruct();
}

void UETLifecycleStatusWidget::StopObserving()
{
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(
            this, &UETLifecycleStatusWidget::HandleObservedPawnChanged);
    }

    if (bObserving)
    {
        UE_LOG(LogETLifecycleWidget, Display,
            TEXT("[M1-02C2] Widget unsubscribed Widget=%s"),
            *GetNameSafe(this));
    }

    BoundSubsystem.Reset();
    bObserving = false;
}

void UETLifecycleStatusWidget::HandleObservedPawnChanged(
    APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;

    if (bObserving)
    {
        RefreshStatus(TEXT("PawnEvent"));
    }
}

void UETLifecycleStatusWidget::RefreshStatus(const TCHAR* Reason)
{
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    APawn* Pawn = Subsystem ? Subsystem->GetObservedPawn() : nullptr;

    if (!StatusText || !PawnText)
    {
        UE_LOG(LogETLifecycleWidget, Warning,
            TEXT("[M1-02C2] Missing BindWidget fields Widget=%s"),
            *GetNameSafe(this));
        return;
    }

    const FText Status = !Subsystem
        ? LOCTEXT("Unavailable", "本地 UI 入口不可用")
        : (Pawn ? LOCTEXT("Controlled", "控制状态：已观察到 Pawn")
                : LOCTEXT("NoPawn", "控制状态：暂无 Pawn"));

    StatusText->SetText(Status);
    PawnText->SetText(FText::Format(
        LOCTEXT("PawnFormat", "当前 Pawn：{0}"),
        FText::FromString(GetNameSafe(Pawn))));

    UE_LOG(LogETLifecycleWidget, Display,
        TEXT("[M1-02C2] Widget snapshot Widget=%s Pawn=%s Reason=%s"),
        *GetNameSafe(this), *GetNameSafe(Pawn), Reason);
}

#undef LOCTEXT_NAMESPACE
~~~

“暂无 Pawn”是合法显示状态，不是错误。Widget 不缓存 Pawn，也不向角色添加第二份控制状态。NativeConstruct 可以多次发生，所以每次先解绑，再注册一次；NativeDestruct 也做幂等清理。

## 5. Bootstrap 完整代码

### 5.1 ETUIBootstrapComponent.h

路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Bootstrap\ETUIBootstrapComponent.h`。

~~~cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETUIBootstrapComponent.generated.h"

class UETLifecycleStatusWidget;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETUIBootstrapComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETUIBootstrapComponent();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildStatusWidget();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void RemoveStatusWidget();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETLifecycleStatusWidget> StatusWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UETLifecycleStatusWidget> StatusWidget;
};
~~~

### 5.2 ETUIBootstrapComponent.cpp

路径：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Bootstrap\ETUIBootstrapComponent.cpp`。

~~~cpp
#include "Bootstrap/ETUIBootstrapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"
#include "Widgets/ETLifecycleStatusWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUIBootstrap, Log, All);

UETUIBootstrapComponent::UETUIBootstrapComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UETUIBootstrapComponent::BeginPlay()
{
    Super::BeginPlay();
    RebuildStatusWidget();
}

void UETUIBootstrapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemoveStatusWidget();
    Super::EndPlay(EndPlayReason);
}

void UETUIBootstrapComponent::RebuildStatusWidget()
{
    if (!HasBegunPlay())
    {
        return;
    }

    RemoveStatusWidget();

    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController()
        || !Controller->GetLocalPlayer())
    {
        return;
    }

    if (!StatusWidgetClass)
    {
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M1-02C2] Missing StatusWidgetClass Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }

    StatusWidget = CreateWidget<UETLifecycleStatusWidget>(
        Controller, StatusWidgetClass);

    if (!StatusWidget)
    {
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M1-02C2] Widget creation failed Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }

    if (!StatusWidget->AddToPlayerScreen(5))
    {
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M1-02C2] AddToPlayerScreen failed Owner=%s"),
            *GetNameSafe(Controller));
        RemoveStatusWidget();
        return;
    }

    UE_LOG(LogETUIBootstrap, Display,
        TEXT("[M1-02C2] Widget created Owner=%s Widget=%s"),
        *GetNameSafe(Controller), *GetNameSafe(StatusWidget.Get()));
}

void UETUIBootstrapComponent::RemoveStatusWidget()
{
    if (!StatusWidget)
    {
        return;
    }

    UE_LOG(LogETUIBootstrap, Display,
        TEXT("[M1-02C2] Widget removed Owner=%s Widget=%s"),
        *GetNameSafe(GetOwner()), *GetNameSafe(StatusWidget.Get()));

    StatusWidget->StopObserving();
    StatusWidget->RemoveFromParent();
    StatusWidget = nullptr;
}
~~~

先 StopObserving，再 RemoveFromParent，避免被移除但还未 GC 的旧 Widget 继续响应 Pawn 事件。重建时始终先清旧实例。TObjectPtr + UPROPERTY 保存 Widget；World Actor 仍由 C1 的弱引用观察。这里的强引用只负责面板存活，EndPlay 必须主动释放。

本批采用 APlayerController 作为通用宿主，不要求核心 Controller 引用任何 UI 类型；通过 BP_ETPlayerController 添加组件完成装配。

本机接口已核对：[CreateWidget / UUserWidget](D:/UE5/UE_5.5/Engine/Source/Runtime/UMG/Public/Blueprint/UserWidget.h:1703)、[NativeConstruct / NativeDestruct](D:/UE5/UE_5.5/Engine/Source/Runtime/UMG/Private/UserWidget.cpp:1767)、[AddToPlayerScreen](D:/UE5/UE_5.5/Engine/Source/Runtime/UMG/Private/UserWidget.cpp:1283)。[Epic API 说明](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/UUserWidget/AddToPlayerScreen?lang=en-US)解释它按所属本地玩家加入屏幕；具体版本以本机 UE 5.5.4 源码为准。代码尚未编译，API 核对不等于 Build 通过。

## 6. 正常构建与索引

源码保存完毕，确认 UE 关闭，执行：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

**不加 -SkipBuild。** 脚本完成 Editor Build / 模型刷新后，CLion Ctrl+Shift+A → Reload CMake Project。检查四个新文件属于 EmberTrialUICodeModel，UserWidget.h / TextBlock.h 可以导航。

再保持 UE 关闭，执行 Game Build：

~~~powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
~~~

本批新增反射类与 UMG 依赖，不能用 Live Coding 代替正常 Build。成功后再打开 UE。

## 7. 创建状态面板 Blueprint

### 7.1 创建与父类

1. 在 Content Browser 建立 `/Game/EmberTrial/UI/Prototype` 文件夹。
2. 在该目录右键 → User Interface → Widget Blueprint。父类选择 **ETLifecycleStatusWidget**（若弹窗先显示 UserWidget，用 All Classes 搜索）。
3. 命名 **WBP_ETLifecycleStatus**。完整资产路径 `/Game/EmberTrial/UI/Prototype/WBP_ETLifecycleStatus`。
4. 若已经创建成普通 UserWidget，在 Graph → Class Settings → Parent Class 改为 ETLifecycleStatusWidget。
5. **先完成下面控件命名，再 Compile**，否则严格 BindWidget 会提示缺少字段；出现该提示先检查名字和类型，不改成 Optional 隐藏布局错误。

### 7.2 Designer 布局

根节点使用 **Canvas Panel**。已有其他根容器时按下面层级调整：

~~~text
Canvas Panel
└─ Border
   └─ Vertical Box
      ├─ Text Block：TitleText（标题）
      ├─ Text Block：StatusText（必须同名，Is Variable=True）
      └─ Text Block：PawnText（必须同名，Is Variable=True）
~~~

Border 在 Canvas Panel 下，因此才会显示 Canvas Slot 的 Anchors / Position / Size。

| 项目 | 配置 |
|---|---|
| Border → Canvas Slot Anchors | 右上角单点：Min=(1,0)，Max=(1,0) |
| Border → Alignment | X=1，Y=0 |
| Border → Position | X=-24，Y=24 |
| Border → Size | X=440，Y=112；Auto Size=False |
| Border → Padding | 四边 12 |
| Border → Brush Color | R=0.03，G=0.03，B=0.04，A=0.8 |
| TitleText → Text / Font Size | 本地 UI 生命周期 / 18 |
| StatusText → 设计时 Text / Font Size | 控制状态：等待预览 / 16 |
| PawnText → 设计时 Text / Font Size | 当前 Pawn：预览 / 14；Auto Wrap Text=True |
| 三个文字的 Color | 白色；Vertical Box Slot Size=Auto |
| 状态与 Pawn 文字的 Slot Padding | 上边 4，其余 0 |
| 标题的 Slot Padding | 四边 0；只影响排版 |
| Class Defaults → Is Focusable | False |
| Class Defaults → Tick Frequency | Never |

UE 字段名可能带空格。Class Defaults 找不到 Tick Frequency 时在 Details 搜索 Tick；**不要通过 C++ 访问 TickFrequency，它在本机 UUserWidget 是 private**。

两个状态 TextBlock 的 Text 右侧保持**没有 Bind 绑定函数**；Event Graph 不添加 Construct / Tick / SetText / 输入逻辑。新建 Widget 自动生成的 Pre Construct / Construct / Tick 事件若没有连接执行线、显示 disabled，可以保留，它们不会执行。运行时文本由 native 事件写入；设计时文字只是编辑器预览。NativeConstruct 将整个 Widget 设置为 **Not Hit-Testable (Self & All Children)**，面板不会拦截 P 菜单背景 / 按钮。

Compile / Save，确认父类和两个绑定字段正确。若中文不显示，先检查是否沿用已通过的 UI 字体资源，不额外下载字体。

### 7.2.1 Native Tick 警告：一行补修

**2026-10-03 复验完成：下列修改已由用户实施，双目标构建通过，Widget Compile 警告消失。** 这里保留根因和修复步骤；当前无需再次修改。

若 Compile 显示 `This widget may require a native tick but the widget is set to never tick...`，这是**编译警告**，本次日志和 Compiler Results 都显示编译成功、1 条 warning。

原因是原稿的原生类没有 DisableNativeTick，UE 会保守认为该类需要 native tick；与蓝图里的 Never 设置相遇就发出警告。**这是助手文档遗漏，不是你把 Never 设置错了。** 已核对本机 [检查逻辑](D:/UE5/UE_5.5/Engine/Source/Editor/UMGEditor/Private/WidgetBlueprint.cpp:1413)、[警告触发](D:/UE5/UE_5.5/Engine/Source/Editor/UMGEditor/Private/WidgetBlueprintCompiler.cpp:746) 和 [Epic UE 5.5 说明](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/Blueprint/EWidgetTickFrequency?application_version=5.5)。

按顺序由你操作：

1. UE Save All、退出 PIE、关闭 UE。CLion 打开 `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETLifecycleStatusWidget.h`。
2. 只将原来的 `UCLASS(Abstract, Blueprintable)` 改成下面这一行，其余代码保留：

~~~cpp
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
~~~

3. 保存头文件，UE 保持关闭，按第 6 节执行 `Tools\Refresh-CLion.ps1`，**不加 -SkipBuild**；完成正常 Editor Build，CLion Reload CMake Project。
4. 按第 6 节再正常 Game Build。这一行改变反射元数据，需让 UHT 重新处理，不只点 Widget Compile，也不用 Live Coding 替代。
5. 重开 UE / WBP_ETLifecycleStatus，**Tick Frequency 保持 Never**，Compile / Save；这条 Native Tick 警告应消失。成功后继续第 7.3 节。

这里只补标记，不添加 NativeTick 或逐帧 SetText。助手没有修改头文件或代执行 Build；用户随后手动完成，双目标 Build / 警告消失已按 19:02 的反馈验收。

### 7.2.2 排障时逐项检查与最终复验

2026-10-03，助手通过 computer-use 读取当前 UE 编辑器。仅选择控件 / 展开属性 / 查看绑定菜单，没有修改布局或 Blueprint 内容；检查后恢复 Designer 的 Border 选择。

| 检查顺序 | 实际设置 / 结论 |
|---|---|
| 1. 父类 | ETLifecycleStatusWidget，正确 |
| 2. 控件层级 | Canvas Panel → Border → Vertical Box → TitleText / StatusText / PawnText，正确 |
| 3. Border 锚点 | Minimum=(1,0)，Maximum=(1,0)，正确 |
| 4. Border 位置 / 尺寸 | Position=(-24,24)，Size=(440,112)，正确 |
| 5. Border 对齐 / Auto Size | Alignment=(1,0)，Size To Content 未勾选，正确 |
| 6. Border Padding | 四边 12，正确 |
| 7. Border 背景 | R/G/B/A=0.03/0.03/0.04/0.8，正确 |
| 8. 控件类型 / 字段 | 三个均为 TextBlock；StatusText / PawnText 名字正确、Is Variable 已勾选 |
| 9. Text 绑定 | 两个状态文字的菜单只有 Create Binding，没有已有绑定，正确 |
| 10. 字号 / 颜色 | Title 18、Status 16、Pawn 14；文字白色，正确 |
| 11. Slot | 三个 Size=Auto；Status / Pawn 上间距 4，其余 0，正确 |
| 12. PawnText 自动换行 | Auto Wrap Text=True，正确 |
| 13. Is Focusable | False，正确 |
| 14. Tick Frequency | Never，符合本面板；第 7.2.1 节原生标记随后由用户补上并构建通过 |
| 15. Event Graph | 默认 Pre Construct / Construct / Tick 均未连线并禁用；未见新增更新逻辑，符合本批 |
| 16. Compile | 排障时 successful 且有 Native Tick warning；最终用户复验 warning 已消失 |

两处排版 / 预览差异允许保留：TitleText 的 Slot 也设置了上间距 4；StatusText 预览文字只写“控制状态”。它们不造成 Tick 警告，运行状态文字会被 C++ 覆盖。如要与表格完全一致，手动将 TitleText 的 Slot 上间距改为 0。

实际资源包目录为 `/Game/EmberTrial/ui/Prototype`（ui 小写），本批通过类选择引用 WBP，当前无需迁移目录。排障时编辑器地图标签为 ThirdPersonMap，因此当时要求切到 L_ET_VerticalSlice 后实测；本次验收日志已确认三轮使用正确的新地图 / GameMode，组件装配及第 8–9 节测试均已通过。

### 7.3 在 Controller 上装配

1. 打开 `/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`。
2. Components → Add，搜索 **ETUIBootstrapComponent**（显示名可能带空格 ET UI Bootstrap）。
3. 添加**一个**，组件实例命名 **UIBootstrap**。
4. 选中该组件，Details 搜索 Status Widget Class，选择 **WBP_ETLifecycleStatus**；Auto Activate 保持 True。
5. Compile / Save。不用在 BeginPlay 再 Create Widget / Add to Viewport，也不把 Bootstrap 加到角色或 GameMode。

## 8. 首轮显示与输入边界

打开 L_ET_VerticalSlice / Play：

- 右上角一份面板，显示“已观察到 Pawn”和真实 BP_ThirdPersonCharacter 实例名；无需手动刷新。
- Output Log 搜索 M1-02C2：Widget subscribed、ConstructSnapshot、Widget created 应出现；具体顺序不固定。
- 做一次原移动 / 转向 / 跳跃、1/2/3、2→3、E-A/B。
- P 打开菜单，分别使用鼠标、Enter / Space 继续；点击背景后 P 关闭，恢复后无需点击视口即可移动 / 转向。
- 菜单期间不触发游戏 E / 1/2/3、不补播、无卡键；状态面板无需主动改输入模式或焦点。
- Stop 时 Widget removed / unsubscribed；C1 的清理日志仍出现。Stop 前控制对象可能已经清空，允许先出现 Pawn=None 的快照。

若 Missing BindWidget fields / Missing local subsystem / Missing StatusWidgetClass / AddToPlayerScreen failed，不继续用延迟轮询绕过；提供文字日志与完成时间，我核对布局 / 装配 / 生命周期。

## 9. 临时验证订阅、销毁与 Widget 重建

测试仅在 **BP_ETPlayerController**。每轮测试时站定、动作结束、菜单关闭。

### 9.1 恢复 C1 的 4 / 5 / 6 临时节点

C1 验收后节点已删除，现在重新创建 ETTempPawn（Pawn Object Reference，默认 None），按 [C1 第 7.2–7.4 节](M1_02C1_LocalPlayerLifecycle.md:385)连接：

- **4 Released**：Get Controlled Pawn（Target=Self）的 Return Value → Is Valid 的 Input Object；有效时保存到 ETTempPawn，再 UnPossess（Target=Self）。
- **5 Released**：Is Valid 的 Input Object=ETTempPawn；有效后比较当前 Controlled Pawn 与 ETTempPawn。相等不执行；不相等才 Possess（Target=Self，In Pawn=ETTempPawn）。
- **6 Released**：Is Valid 的 Input Object=Get Controlled Pawn Return Value；有效时 Destroy Actor，Target 接同一个 Pawn 返回值。

纯查询只有数据线，Released / Is Valid / Branch / 调用节点使用执行白线。**5 的同 Pawn 比较不能省略**。

### 9.2 新增 7：重建面板

1. 从 Components 面板将 **UIBootstrap** 拖到 Event Graph，选择 Get；输出是组件对象引用。
2. 从这个输出拖出搜索 **Rebuild Status Widget**，放置调用节点，Target 接 UIBootstrap。
3. 放顶部数字 **7** 键盘事件，**Released** 白线接 Rebuild Status Widget。
4. 不添加 Create Widget、Delay 或第二个 Bootstrap。Compile / Save。

### 9.3 验证顺序

1. 正常出生确认一份面板；按 7 三次。每次旧 Widget 解绑 / 移除，新 Widget 订阅 / ConstructSnapshot，当前 Pawn 立即正确，屏幕仍只有一份。
2. 按 4，显示“暂无 Pawn / None”；角色不再接受控制。**此时按 7**，新面板也立即显示 None，证明重建读取当前快照，不依赖下一次事件。
3. 按 5，显示恢复到原角色，移动 / 转向 / 跳跃恢复；重复 5 不产生新的状态变化。
4. 再做三轮 4→5。每次 C1 的 Pawn changed 对应**当前一个 Widget 的一条 Reason=PawnEvent 快照**；旧 Widget 实例名不能继续处理 PawnEvent。ConstructSnapshot 是重建快照，不计为重复 PawnEvent。
5. 恢复控制后按 6，原角色销毁，但 Controller 的面板保留并显示 None。按 7 后仍是 None；按 5 不控制已销毁对象，没有 Accessed None。身体销毁后原角色 E 提示可能消失，属既有清理。
6. Stop / Play 三轮，新的 Widget 初始化 / 清理正确，旧世界对象不能成为当前 Pawn。
7. Stop，删除 4/5/6/7 的所有临时事件与调用节点、ETTempPawn 变量，Compile / Save。**保留 UIBootstrap 组件、Widget 资产及其类配置。**
8. 最终再 Play，原 1/2/3、2→3、E-A/B、P 鼠标 / 键盘继续、移动 / 转向 / 跳跃全部回归；Stop / Save All。

这批验证同一 Pawn、空 Pawn、Widget 重建；**尚不验证真正的新 Pawn 替换**，C2-02 专门覆盖。无需重新 Cook、录像或重复闪避帧率矩阵。

## 10. 反馈

~~~text
M1-02C2-01 完成 / 卡在第 __ 节
UI Build.cs / 四个新文件已保存；C1 文件保留：是
Editor Build / 索引刷新 / Game Build：成功
CLion UI CodeModel / UMG 头文件导航：通过
Widget 父类、StatusText / PawnText、无 Text Bind、Tick Never：正确
DisableNativeTick 已手动补上 / 正常双目标 Build / Native Tick 警告消失：是
Controller 单个 UIBootstrap / StatusWidgetClass：正确
首次一份面板 / 当前 Pawn 与日志：通过
正常状态三次 7 重建 / 空 Pawn 时 7 快照：通过
三轮 4→5 / 无旧 Widget PawnEvent / 重复 5：通过
6 销毁 / 面板显示 None / 7 重建 / 5 不执行：通过
P 鼠标 / 键盘 / 背景输入与无补播不卡键：通过
三次 PIE 清理 / 无 Accessed None：通过
临时节点和 ETTempPawn 已删除；组件和 Widget 保留：是
最终原 1/2/3、2→3、E/P 与基础输入：正常
源码 / 资产已保存：是
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
~~~

正常日志无需逐条手抄，给完成时间我读取本机日志。报错先给完整文字；控件层级 / 引脚连线不清楚时给截图。代码修正仍默认由你手动实现，除非明确要求我接管。
