# M2-06B1：锁定目标屏幕标记

2026-10-24，UE5.5.4。前置[A验收通过](Reports/M2_06A_Verification.md)，无待补项；[锁定路线](M2_06_TargetLockRoadmap.md)。**本章待用户手动实施**：六新UI文件、零旧Source修改、一新WBP与原Controller添加一份UI组件。助手只准备忽略目录中的候选与文档，未改实际Source / UE资产，未代Build / 自动化 / PIE。

预算手动80–120分钟：代码45–70、Designer / 配置25–35、人工检查8–12，构建等待另记。原B将UI与镜头合在一次粗估；现在分B1标记、B2镜头，避免同时排查投影、输入和相机插值，新增预算使用M2既有缓冲，不叠加全项目总预算。

## 1. 最终画面、职责与刷新边界

Tab锁定真正敌人后，在目标胶囊顶部再向上24cm的位置显示一枚橙黄色小菱形。移动鼠标或敌人移动时跟随目标；解锁、目标死亡 / 玩家死亡 / 换身体 / Stop后消失。镜头背后或安全屏幕范围外隐藏，转回后重新出现；**不贴屏幕边缘、不做离屏箭头、不自动转镜头**。B2再处理镜头，伤害 / 闪避 / 破势规则沿A基线。A的遮挡宽限尚未解除锁定时，标记可短暂保留；本批不另外做UI遮挡射线。

- **Core TargetLock**继续拥有唯一目标与1500cm / 遮挡等脱锁规则，B1不改这六份A源码、不写GAS、不重新扫描候选。
- **新UI Marker组件**加在原游戏Controller，弱观察Core目标，持有一份Widget。只在本地目标有效、Widget加入画面且根可见时启用投影Tick；失去资格即停。Tick位于相机更新后的PostUpdateWork，不在暂停期间执行。
- **Widget**只接收UMG坐标 / 显示资格，设置一个Border的Canvas Slot，位置相同不重复写Slot；无目标引用、Tick、属性Binding或输入业务。
- **坐标**调用UE5.5的ProjectWorldLocationToWidgetPosition并使用玩家界面几何尺寸；质量 / DPI换算由引擎完成。不要再除GetViewportScale，也不要用桌面鼠标或编辑器窗口尺寸混算。

屏幕外只隐藏菱形子控件，根Widget仍可见、投影更新继续，才能在镜头转回时重新出现；根Widget隐藏 / 脱离Viewport或组件失活则停止更新。只跟踪一名玩家视口，当前离线单人，不宣称已验证分屏 / VR或操作系统窗口遮挡策略。

投影需要随相机变化逐帧更新，这不等同每帧扫描Actor或GAS；本批没有新性能采样，不宣称帧率提升。菱形使用UMG两层Border，无图片 / 字体 / 模型或付费资产。

## 2. 保存关闭与六个新文件

Stop PIE → Save All → 关闭UE → CLion保存。以下路径相对`E:\UE_Project\Demo\EmberTrial\Source`；**全部属于EmberTrialUI模块**，不能建到Core/Tests或Core/Public下。可分段创建、保存，六份齐全再正常Build。

| 新文件 | 作用 | 完整代码 |
| --- | --- | --- |
| EmberTrialUI/Public/Widgets/ETTargetLockMarkerWidget.h | 单Border只读View接口 | 3.1 |
| EmberTrialUI/Private/Widgets/ETTargetLockMarkerWidget.cpp | 布局检查 / Canvas位置 / 隐藏 | 3.2 |
| EmberTrialUI/Private/TargetLock/ETTargetMarkerRules.h | 无UObject的边界纯规则，普通头文件，无generated.h | 第4节 |
| EmberTrialUI/Public/TargetLock/ETTargetLockMarkerComponent.h | Controller的UI表现组件 | 5.1 |
| EmberTrialUI/Private/TargetLock/ETTargetLockMarkerComponent.cpp | 来源 / View生命周期与投影 | 5.2 |
| EmberTrialUI/Private/Tests/ETTargetMarkerRulesTests.cpp | 纯规则专项，普通.cpp | 第6节 |

公开头与实现分别放Public / Private对应目录。测试完整路径为`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Tests\ETTargetMarkerRulesTests.cpp`；Private/TargetLock同时包含普通规则头和组件实现，普通头不要放generated.h。

**避免C1083：**此测试引用UI模块的Private规则头，必须作为UI模块源码编译。请在CLion的物理目录`Source/EmberTrialUI/Private/Tests`创建普通.cpp，不能沿用上章Core测试的`Source/EmberTrial/Tests`目录，也不要用UE向导默认的Core模块创建它。2026-10-24已按用户授权将误放的本机测试移到上述UI目录，内容未改、旧位置无副本；正常重新运行第7节刷新脚本即可。不通过修改include、给Core添加UI依赖或公开Private规则头解决。

若使用UE C++向导创建组件，父类ActorComponent、模块EmberTrialUI；Widget基类为UserWidget，同属UI。用CLion在正确物理目录直接创建也可以，最后以表中完整代码替换骨架。UI已有UMG / SlateCore和私有Core依赖，不改Build.cs、Target或uproject，更不能给Core加UI依赖。

## 3. 新增纯View

### 3.1 `EmberTrialUI/Public/Widgets/ETTargetLockMarkerWidget.h`

新文件全文：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETTargetLockMarkerWidget.generated.h"

class UBorder;

// 只接收UMG局部坐标与显示资格，不拥有目标或查询玩法状态。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETTargetLockMarkerWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    bool HasValidMarkerLayout() const;
    void ApplyMarkerFrame(bool bShow, const FVector2D& Position);

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> LockMarker;
};
```

### 3.2 `EmberTrialUI/Private/Widgets/ETTargetLockMarkerWidget.cpp`

新文件全文：

```cpp
#include "Widgets/ETTargetLockMarkerWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"

void UETTargetLockMarkerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    ApplyMarkerFrame(false, FVector2D::ZeroVector);
}

bool UETTargetLockMarkerWidget::HasValidMarkerLayout() const
{
    return LockMarker && Cast<UCanvasPanelSlot>(LockMarker->Slot);
}

void UETTargetLockMarkerWidget::ApplyMarkerFrame(bool bShow, const FVector2D& Position)
{
    if (!LockMarker)
    {
        return;
    }
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(LockMarker->Slot);
    if (!bShow || !CanvasSlot || Position.ContainsNaN())
    {
        LockMarker->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    if (CanvasSlot->GetPosition() != Position)
    {
        CanvasSlot->SetPosition(Position);
    }
    LockMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
}
```

## 4. 新增投影显示边界纯规则

这里只判断投影成功标志、有限坐标 / 尺寸和内边距，不做世界投影或DPI换算；名字虽是TargetLock，不属于Core。新文件全文：

```cpp
#pragma once

#include "CoreMinimal.h"

namespace ETTargetMarkerRules
{
// Position和Size均为玩家界面的UMG单位；不在这里重复除DPI或做世界坐标投影。
inline bool ShouldShow(bool bProjected, const FVector2D& Position, const FVector2D& Size,
                       float Inset)
{
    return bProjected && !Position.ContainsNaN() && !Size.ContainsNaN() && FMath::IsFinite(Inset) &&
           Inset >= 0.0f && Size.X > 2.0 * Inset && Size.Y > 2.0 * Inset && Position.X >= Inset &&
           Position.Y >= Inset && Position.X <= Size.X - Inset && Position.Y <= Size.Y - Inset;
}
} // namespace ETTargetMarkerRules
```

## 5. 新增Controller上的UI标记组件

它与UIBootstrap / FailureFlow是独立组件：前者继续装配旧五HUD，新组件只负责一个目标标记，不扩展每个资源Presenter或改变暂停输入。AddToPlayerScreen层级0低于既有状态 / 资源HUD，不获取键鼠焦点。

使用UE已有OnNativeVisibilityChanged / OnNativeDestruct委托，精确持有并释放三个句柄；晚创建 / 重建时直接读取Core当前目标。屏幕外不清Core锁定；重建只换View。

### 5.1 `EmberTrialUI/Public/TargetLock/ETTargetLockMarkerComponent.h`

新文件全文：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETTargetLockMarkerComponent.generated.h"

class APlayerController;
class AEmberTrialCharacter;
class UETTargetLockComponent;
class UETTargetLockMarkerWidget;
class UUserWidget;
enum class ESlateVisibility : uint8;

// UI表现组件：订阅Core唯一目标，拥有一份View及必要的投影更新。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETTargetLockMarkerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UETTargetLockMarkerComponent();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Target Marker")
    void RebuildMarkerWidget();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Marker")
    TSubclassOf<UETTargetLockMarkerWidget> MarkerWidgetClass;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Marker",
              meta = (ClampMin = "0.0", ClampMax = "200.0", Units = "cm"))
    float HeightOffset = 24.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Marker",
              meta = (ClampMin = "12.0", ClampMax = "200.0"))
    float ScreenInset = 24.0f;
    UPROPERTY(Transient)
    TObjectPtr<UETTargetLockMarkerWidget> MarkerWidget;

    TWeakObjectPtr<APlayerController> BoundController;
    TWeakObjectPtr<UETTargetLockComponent> BoundLock;
    FDelegateHandle TargetChangedHandle;
    FDelegateHandle VisibilityChangedHandle;
    FDelegateHandle DestructHandle;
    bool bViewAttached = false;
    bool bEndingPlay = false;

    bool HasValidSettings() const;
    bool CanRefresh() const;
    void RefreshPresentation();
    void SetRefreshEnabled(bool bEnabled);
    void UpdateMarkerFrame();
    void RemoveMarkerWidget();
    void HandleTargetChanged(AEmberTrialCharacter* Previous, AEmberTrialCharacter* Current);
    void HandleWidgetVisibility(ESlateVisibility Visibility);
    void HandleWidgetDestruct(UUserWidget* Widget);
};
```

### 5.2 `EmberTrialUI/Private/TargetLock/ETTargetLockMarkerComponent.cpp`

新文件全文：

```cpp
#include "TargetLock/ETTargetLockMarkerComponent.h"
#include "TargetLock/ETTargetMarkerRules.h"
#include "TargetLock/ETTargetLockComponent.h"
#include "Widgets/ETTargetLockMarkerWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Layout/Geometry.h"

DEFINE_LOG_CATEGORY_STATIC(LogETTargetMarker, Log, All);

UETTargetLockMarkerComponent::UETTargetLockMarkerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bTickEvenWhenPaused = false;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    bAutoActivate = true;
}

// ---- Controller / Core来源绑定；不修改锁定、不订阅输入、不创建第二份目标身份 ----
void UETTargetLockMarkerComponent::BeginPlay()
{
    Super::BeginPlay();
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!IsValid(PC) || !PC->IsLocalController() || !PC->GetLocalPlayer())
    {
        SetRefreshEnabled(false);
        return;
    }
    BoundController = PC;
    UETTargetLockComponent* Lock = PC->FindComponentByClass<UETTargetLockComponent>();
    if (!IsValid(Lock))
    {
        UE_LOG(LogETTargetMarker, Warning, TEXT("[M2-06B1] Missing Core TargetLock Owner=%s"),
               *GetNameSafe(PC));
        return;
    }
    BoundLock = Lock;
    TargetChangedHandle = Lock->OnTargetChanged.AddUObject(this, &ThisClass::HandleTargetChanged);
    RebuildMarkerWidget();
}

void UETTargetLockMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    SetRefreshEnabled(false);
    if (UETTargetLockComponent* Lock = BoundLock.Get())
    {
        Lock->OnTargetChanged.Remove(TargetChangedHandle);
    }
    TargetChangedHandle.Reset();
    BoundLock.Reset();
    RemoveMarkerWidget();
    BoundController.Reset();
    Super::EndPlay(EndPlayReason);
}

void UETTargetLockMarkerComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    RefreshPresentation();
}

void UETTargetLockMarkerComponent::Deactivate()
{
    Super::Deactivate();
    RefreshPresentation();
}

// ---- View重建只释放显示资源；Core目标 / 维持Timer不受影响 ----
void UETTargetLockMarkerComponent::RebuildMarkerWidget()
{
    if (!HasBegunPlay() || bEndingPlay)
    {
        return;
    }
    RemoveMarkerWidget();
    APlayerController* PC = BoundController.Get();
    if (!IsValid(PC) || !PC->GetLocalPlayer() || !BoundLock.IsValid() || !MarkerWidgetClass)
    {
        UE_LOG(LogETTargetMarker, Warning,
               TEXT("[M2-06B1] Cannot create marker: local source or MarkerWidgetClass missing"));
        return;
    }
    MarkerWidget = CreateWidget<UETTargetLockMarkerWidget>(PC, MarkerWidgetClass);
    if (!MarkerWidget || !MarkerWidget->AddToPlayerScreen(0))
    {
        UE_LOG(LogETTargetMarker, Warning, TEXT("[M2-06B1] Marker AddToPlayerScreen failed"));
        RemoveMarkerWidget();
        return;
    }
    if (!MarkerWidget->HasValidMarkerLayout())
    {
        UE_LOG(LogETTargetMarker, Warning,
               TEXT("[M2-06B1] LockMarker must be a Border directly inside the root Canvas"));
        RemoveMarkerWidget();
        return;
    }
    bViewAttached = true;
    VisibilityChangedHandle = MarkerWidget->OnNativeVisibilityChanged.AddUObject(
        this, &ThisClass::HandleWidgetVisibility);
    DestructHandle =
        MarkerWidget->OnNativeDestruct.AddUObject(this, &ThisClass::HandleWidgetDestruct);
    UE_LOG(LogETTargetMarker, Display, TEXT("[M2-06B1] View created Widget=%s"),
           *GetNameSafe(MarkerWidget.Get()));
    RefreshPresentation();
}

void UETTargetLockMarkerComponent::RemoveMarkerWidget()
{
    bViewAttached = false;
    SetRefreshEnabled(false);
    UETTargetLockMarkerWidget* Previous = MarkerWidget.Get();
    MarkerWidget = nullptr;
    if (IsValid(Previous))
    {
        Previous->OnNativeVisibilityChanged.Remove(VisibilityChangedHandle);
        Previous->OnNativeDestruct.Remove(DestructHandle);
        Previous->ApplyMarkerFrame(false, FVector2D::ZeroVector);
        UE_LOG(LogETTargetMarker, Display, TEXT("[M2-06B1] View removed Widget=%s"),
               *GetNameSafe(Previous));
        Previous->RemoveFromParent();
    }
    VisibilityChangedHandle.Reset();
    DestructHandle.Reset();
}

// ---- 投影更新资格；屏幕外只隐藏子控件，仍需更新才能在转回镜头后重新出现 ----
bool UETTargetLockMarkerComponent::HasValidSettings() const
{
    return FMath::IsFinite(HeightOffset) && HeightOffset >= 0.0f && HeightOffset <= 200.0f &&
           FMath::IsFinite(ScreenInset) && ScreenInset >= 12.0f && ScreenInset <= 200.0f;
}

bool UETTargetLockMarkerComponent::CanRefresh() const
{
    APlayerController* PC = BoundController.Get();
    UETTargetLockComponent* Lock = BoundLock.Get();
    UWorld* World = GetWorld();
    return HasBegunPlay() && !bEndingPlay && IsActive() && HasValidSettings() && IsValid(World) &&
           !World->bIsTearingDown && IsValid(PC) && PC->IsLocalController() &&
           PC->GetLocalPlayer() && IsValid(Lock) && Lock->GetOwner() == PC &&
           Lock->HasBegunPlay() && IsValid(Lock->GetLockedTarget()) && bViewAttached &&
           IsValid(MarkerWidget.Get()) && MarkerWidget->IsInViewport() && MarkerWidget->IsVisible();
}

void UETTargetLockMarkerComponent::SetRefreshEnabled(bool bEnabled)
{
    const bool bWasEnabled = IsComponentTickEnabled();
    SetComponentTickEnabled(bEnabled);
    if (bWasEnabled != bEnabled)
    {
        UE_LOG(LogETTargetMarker, Display, TEXT("[M2-06B1] Projection refresh Owner=%s Active=%d"),
               *GetNameSafe(GetOwner()), bEnabled ? 1 : 0);
    }
}

void UETTargetLockMarkerComponent::RefreshPresentation()
{
    const bool bCanRefresh = CanRefresh();
    SetRefreshEnabled(bCanRefresh);
    if (bCanRefresh)
    {
        UpdateMarkerFrame();
    }
    else if (MarkerWidget)
    {
        MarkerWidget->ApplyMarkerFrame(false, FVector2D::ZeroVector);
    }
}

void UETTargetLockMarkerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdateMarkerFrame();
}

void UETTargetLockMarkerComponent::UpdateMarkerFrame()
{
    if (!CanRefresh())
    {
        RefreshPresentation();
        return;
    }
    APlayerController* PC = BoundController.Get();
    AEmberTrialCharacter* Target = BoundLock->GetLockedTarget();
    const UCapsuleComponent* Capsule = Target->GetCapsuleComponent();
    const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.0f;
    const FVector Anchor =
        Target->GetActorLocation() + FVector::UpVector * (HalfHeight + HeightOffset);
    FVector2D Position = FVector2D::ZeroVector;
    const bool bProjected =
        !Anchor.ContainsNaN() &&
        UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Anchor, Position, true);
    const FVector2D Size = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(PC).GetLocalSize();
    const bool bShow = ETTargetMarkerRules::ShouldShow(bProjected, Position, Size, ScreenInset);
    MarkerWidget->ApplyMarkerFrame(bShow, Position);
}

void UETTargetLockMarkerComponent::HandleTargetChanged(AEmberTrialCharacter* Previous,
                                                       AEmberTrialCharacter* Current)
{
    (void)Previous;
    (void)Current;
    RefreshPresentation();
}

void UETTargetLockMarkerComponent::HandleWidgetVisibility(ESlateVisibility Visibility)
{
    (void)Visibility;
    RefreshPresentation();
}

void UETTargetLockMarkerComponent::HandleWidgetDestruct(UUserWidget* Widget)
{
    if (Widget == MarkerWidget.Get())
    {
        bViewAttached = false;
        SetRefreshEnabled(false);
    }
}
```

## 6. 新增一项纯规则自动化

检查投影失败、内边界、四侧越界、零 / 过小画面、非法内边距、NaN / Infinity和不同UMG尺寸。不创建真实Viewport / Widget，不把数学规则Success当成DPI / 相机 / 可见画面已验证；这些由第10节实机补齐。旧20项保留，本次不复制A的世界 / 目标夹具。

新文件全文：

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetMarkerRules.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETTargetMarkerRulesTest, "EmberTrial.UI.TargetLock.MarkerRules",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETTargetMarkerRulesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using ETTargetMarkerRules::ShouldShow;
    const FVector2D Size(1280, 720);
    const FVector2D Center(640, 360);
    TestTrue(TEXT("Valid local point"), ShouldShow(true, Center, Size, 24.0f));
    TestFalse(TEXT("Failed / behind projection"), ShouldShow(false, Center, Size, 24.0f));
    TestTrue(TEXT("Inset boundary is inclusive"), ShouldShow(true, FVector2D(24, 24), Size, 24.0f));
    for (const FVector2D Point : {FVector2D(23, 360), FVector2D(1257, 360), FVector2D(640, 23),
                                  FVector2D(640, 697), FVector2D(-1, -1)})
    {
        TestFalse(TEXT("Outside safe viewport"), ShouldShow(true, Point, Size, 24.0f));
    }
    TestFalse(TEXT("No geometry yet"), ShouldShow(true, Center, FVector2D::ZeroVector, 24.0f));
    TestFalse(TEXT("Viewport smaller than inset"),
              ShouldShow(true, FVector2D(20, 20), FVector2D(40, 40), 24.0f));
    TestFalse(TEXT("Negative inset"), ShouldShow(true, Center, Size, -1.0f));
    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("NaN position"), ShouldShow(true, FVector2D(NaN, 100), Size, 24.0f));
    TestFalse(TEXT("Infinite size"), ShouldShow(true, Center, FVector2D(Infinity, 720), 24.0f));
    TestFalse(TEXT("NaN inset"), ShouldShow(true, Center, Size, NaN));
    TestTrue(TEXT("Another UMG canvas size"),
             ShouldShow(true, FVector2D(960, 540), FVector2D(1920, 1080), 24.0f));
    // DPI转换 / 相机 / 真实Viewport由引擎和实机检查负责，不把纯规则当成投影集成通过。
    return !HasAnyErrors();
}
#endif
```

## 7. 正常Editor / 索引 / Game Build

六新文件保存、UE仍关闭。在PowerShell依次执行，首条Error出现先停并反馈文字 / 路径行号：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

CLion Reload CMake Project；预期**153 Source / 83cpp，Core57 / UI23 / Editor3**，自动化21项（Core17 / UI4）。只增加UI三cpp，A的Core57不变；当前实际仍147 / 80 / 20，以上是手动实施后的预期。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标通过再打开UE创建资产；未配置MarkerWidgetClass前先不要Play。无需清缓存，不Live Coding反射头；本指南目前只做静态文本 / 本机接口核对，助手未代编译或执行新专项。

## 8. 新建WBP_ETTargetLockMarker（详细Designer步骤）

### 8.1 新资产与正确父类

1. Content Browser进入`/Game/EmberTrial/ui/HUD`，右键 → User Interface → Widget Blueprint，命名**WBP_ETTargetLockMarker**，不要Duplicate已有生命 / 冷却WBP。
2. 创建时可选All Classes → **ETTargetLockMarkerWidget**；若只给UserWidget，创建后File → Reparent Blueprint选择ETTargetLockMarkerWidget。
3. Class Settings核对Parent Class。首次尚无LockMarker时，Compile出现required widget binding缺失先完成下节，不改成Optional或退回UserWidget。
4. Graph保持空；不接Tab、Construct中的投影、Event Tick、SetPosition、GetAllActors或TargetLock调用。以下只有布局和外观。

### 8.2 按Hierarchy建三层

```text
Canvas Panel（根，可命名MarkerCanvas）
└─ Border：LockMarker
   └─ Border：MarkerInner
```

1. Designer若没有根，拖Canvas Panel作为根；有Canvas就复用，只保留一份。
2. Palette搜索Border，拖到根Canvas上，Hierarchy中F2命名**LockMarker**，Details顶部**Is Variable=True**。对象名不是显示文本，类型必须Border。
3. 再拖一个Border到LockMarker，改名MarkerInner；它不是必需绑定控件，可不勾Is Variable。Border只能有一个直接子项，不能把第二层误放到Canvas旁边。
4. LockMarker必须是Canvas直接子项；选中它后Details应显示**Slot (Canvas Panel Slot)**。如果看到Border Slot / Vertical Box Slot，父子关系不对，移回根Canvas；否则原生运行会拒绝该布局。

### 8.3 逐项设置菱形

Designer预览1920×1080，先选Hierarchy中的LockMarker：

| 对象 / Details区域 | 配置 |
| --- | --- |
| LockMarker → Canvas Panel Slot | Anchors Min=(0,0)、Max=(0,0)；Alignment=(.5,.5)；Position X=960、Y=400，仅供预览 |
| 同一Canvas Slot | Size X=16、Y=16；Auto Size=False；ZOrder=0 |
| LockMarker → Appearance | Brush保持默认可绘制Border，Brush Color=(1,.65,.1,1)；Content Color and Opacity保持白色 / Alpha1；不要设No Draw |
| LockMarker → Content | Padding四边3；Horizontal / Vertical Alignment=Fill，让内层填满剩余区域 |
| LockMarker → Render Transform | Angle=45°；Translation=(0,0)、Scale=(1,1)、Shear=(0,0)；Render Transform Pivot=(.5,.5) |
| MarkerInner | Brush默认可绘制，Brush Color=(.02,.02,.02,.85)，Padding=0；Render Transform保持默认，不再转45° |
| 两个Border | Designer Visibility=Not Hit-Testable (Self & All Children)，即HitTestInvisible；不创建Visibility Binding |

外框16×16、内层剩余10×10，整个外框转45°形成小菱形。没有Text / Image，不设置字体或导入图片。运行时Canvas Position会由C++覆盖，不能靠预览坐标判断投影正确；不要用Render Translation代替Slot Position。

Class Defaults：**Is Focusable=False、Tick Frequency=Never、Visibility=Not Hit-Testable (Self & All Children)**。根Canvas铺满玩家界面，不能把整个UserWidget缩成16×16，也不设置Viewport Alignment / Desired Size改变整张画布。

### 8.4 Compile / Save / 校验

1. Compile → Save，预期无required widget binding或Native Tick警告。
2. Content Browser选新WBP → 右键Asset Actions → Validate Assets，应出现本项目**[M1-07C1] Valid**。
3. 绑定问题按Parent Class → LockMarker对象名 / Border类型 / Is Variable → Canvas直接父级查。原生DisableNativeTick已提供，不为消警告开Widget Tick。
4. Designer截图需同时包含WBP标题、Hierarchy和选中对象Details；本章只有一个BindWidget，不添加旧HUD的四个字段。

## 9. 原游戏Controller添加唯一UI标记组件

1. 打开`/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`。Components确认A继承的**TargetLock**仍一份；它是Core目标组件，不是本章新UI组件。
2. Add Component搜索**ETTargetLockMarker**或**Target Lock Marker**，选新原生ETTargetLockMarkerComponent，添加**一份**。组件实例可F2命名TargetLockMarker。
3. 选中新组件，Details搜索Target Marker，配置：

| 字段 | 最终值 |
| --- | --- |
| Marker Widget Class | WBP_ETTargetLockMarker |
| Height Offset | 24cm，叠加在目标胶囊半高之上 |
| Screen Inset | 24个UMG单位 |
| Auto Activate | True |

4. 不手动勾选Start With Tick Enabled / Tick Even When Paused；原生按资格启停并使用PostUpdateWork。**新增组件本身需要投影Tick，WBP仍Never**，两者职责不同。
5. 原唯一UIBootstrap / FailureFlow、五个HUD Class、A的六锁定数值 / Target Lock Action=**IA_ET_LockTarget**、IMC唯一Tab全部保留。本章不改Bootstrap C++、输入、GA、角色或菜单Controller。
6. 不给角色 / 敌人Add本组件，不给主菜单Controller添加。EventGraph不接业务。Controller Compile → Save，WBP也Save，回L_ET_VerticalSlice。

此时应有各一份Core TargetLock、UI TargetLockMarker、UIBootstrap、FailureFlow；“一份”指各自唯一，不是删除旧UI组件来只留本章组件。新WBP存在后尚未验收，来源清单当前72；完成后我核对实物再登记73。

## 10. 方案B最小检查（人工约8–12分钟）

### 10.1 一项新规则专项

Stop PIE，Output Log运行：

```text
Automation RunTests EmberTrial.UI.TargetLock.MarkerRules
```

只需一次Success。Core / A未修改，旧20项沿用，不重跑全部。纯规则不验证实际投影 / DPI，下面画面检查不可省；失败发首条断言与完成时分，不自行删断言。

### 10.2 获取、跟随、边界与暂停（约3分钟）

1. Play，初始无标记，五HUD正常；无锁定不应持续投影刷新。面向真实敌人Tab，出现一枚橙黄菱形，在目标头顶上方，日志[M2-06B1] Projection refresh Active=1。
2. 用鼠标自由转镜头、敌人移动，菱形跟随，始终属于同一锁定目标。**相机不会自动跟随是B1预期**，普通受击 / 破势存活不应让标记随机换人。
3. 在没有墙遮挡、目标仍未被A脱锁的情况下，将目标转到屏幕外 / 镜头背后，菱形隐藏；转回后重新出现，不粘屏幕边缘、不出现在(0,0)。如果A因为遮挡 / 越界已清锁，重新Tab后再看，不能把合法脱锁当作投影错误。
4. P暂停，标记与场景保持，菜单内Tab不补执行；用一种继续方式恢复。没有额外Marker输入处理，不重复A所有Pause组合。

### 10.3 尺寸与DPI口径（约2分钟）

1. 先记录当前PIE方式。可用New Editor Window (PIE) / 新编辑器窗口运行，不必打包；先停止旧PIE再切换。
2. Tab锁定后，拖动游戏窗口边框缩小再放大，或改变嵌入视口大小。只改变窗口尺寸，不改r.ScreenPercentage / 分辨率CVar / 项目DPI曲线。
3. 稳定画面后菱形仍在目标头顶，不因尺寸变化产生持续偏移；窗口变化后一帧布局更新可以接受。小到安全区域不足时隐藏正常，恢复尺寸后再显示。
4. 这是当前机器的一组尺寸样本，不声称已测所有DPI / 分屏。若偏移，发窗口全图与PIE方式，不通过手改Border X/Y抵消；运行时位置由投影覆盖。

### 10.4 锁定中重建View与失活（约2分钟）

保持目标已锁定，PIE游戏控制台（~）逐条执行，不是PowerShell：

```text
ke ETTargetLockMarkerComponent RebuildMarkerWidget
ke ETTargetLockMarkerComponent RebuildMarkerWidget
ke ETTargetLockMarkerComponent Deactivate
ke ETTargetLockMarkerComponent Activate false
```

每次View重建只有一枚标记，Core不应因这些命令产生新的Locked / Cleared。Deactivate只隐藏UI并停止投影，Activate false恢复当前原目标的标记，不重新选择目标；句柄 / View旧对象退出由本组件负责，不搭保留旧Widget变量图。

### 10.5 死亡、重建与最终保存（约2–3分钟）

1. 持锁用原左键 / 2 / F交战，敌人真实死亡时标记消失、投影Active0。A已验收原规则，本次只看UI联动，不重做全部伤害矩阵。
2. 新一轮Tab锁定后让玩家致死，Failure正常、标记消失；Retry新身体无旧标记，重新Tab可显示一份。P返回菜单本来无标记，Continue后同样重新锁定才显示。
3. 锁定中Stop，再Play初始无旧标记 / 投影更新；最后五HUD、左键-2-Q-E-P与移动 / 转向 / 跳跃短查。
4. Stop → Save All，新组件Auto Activate=True、Class正确 / 24 / 24；A默认1200 / 1500 / 60 / .75 / .10 / .35、原输入 / GA数值 / 三身体100/100、slomo1 / Debug False、默认地图保持。没有临时改动填未改，勿保存测试失活为资产默认。

## 11. 完成反馈与问题描述

```text
M2-06B1 完成
六新UI文件 / 零旧Source；关闭UE后Editor Build / Core57-UI23-Editor3索引 / Game Build：
新WBP父类 / 单LockMarker Border / Canvas直接子级 / 16×16与45° / 无Bind / Never / Compile-Save-Valid：
原Controller唯一TargetLockMarker / Class / 24-24 / AutoActivate；原Core与五HUD组件保持：
UI.TargetLock.MarkerRules一次：
Tab单标记 / 相机与敌人移动跟随 / 屏幕外-背后隐藏再出现 / P冻结恢复：
窗口尺寸调整后位置正常（附实际PIE方式，未测试的情况如实写）：
锁定中两次重建 / Deactivate-Activate false；Core目标保持、无重影：
敌人死亡消失 / 玩家Failure-Retry / 菜单Continue / Stop无旧标记与短基线：
Source / 资产Save All；AutoActivate与默认配置恢复或未改：
失败或最明显布局问题（无则填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

编译发首条Error、文件行号与前后约20行；绑定 / 槽位问题截图含资产名、Hierarchy、Details和Compiler Results。投影偏移给一张包含游戏窗口边界的全图、PIE方式与目标位置，别只截小菱形；动态抖动给5–10秒视频和按键顺序。给完成时分即可，我读日志，不要求手抄成功段。

## 12. 本章 Commit / Push（固定放在最后）

**手动实施且验收通过后**Stage六新Source（均加前缀EmberTrial/Source/）：

```text
EmberTrialUI/Public/Widgets/ETTargetLockMarkerWidget.h
EmberTrialUI/Private/Widgets/ETTargetLockMarkerWidget.cpp
EmberTrialUI/Private/TargetLock/ETTargetMarkerRules.h
EmberTrialUI/Public/TargetLock/ETTargetLockMarkerComponent.h
EmberTrialUI/Private/TargetLock/ETTargetLockMarkerComponent.cpp
EmberTrialUI/Private/Tests/ETTargetMarkerRulesTests.cpp
```

一新 / 一旧UE资产：

```text
EmberTrial/Content/EmberTrial/ui/HUD/WBP_ETTargetLockMarker.uasset
EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset
```

文档：本指南、完成后生成的B1报告与实际来源（当前72，实物核对新增WBP后才73）、Progress / Backlog / Architecture / 方案B / VersionControl / 两份M2路线 / README / Introduction最新收尾和下一章指南；A报告尚未提交时一起包含。本章不改旧六份A Source / UI Bootstrap / Build.cs / 输入资产 / GA / 角色 / 地图 / Config，不制造无关重存差异。

建议Commit（英文）：

```text
feat: display the locked target with a viewport-aware marker
```

中文译文：

```text
feat: 新增适配视口坐标的锁定目标标记
```

英文可作标题，中文供理解或作正文，不必拼成重复标题。CLion刷新Changes → Stage最新完整内容（已Stage后又改需要再Stage）→ 检查新文件非空 / UI路径正确、Controller只有一份新组件 / 正确Class、WBP与LFS → Commit → Push。Raw / 日志 / Saved / Binaries / Intermediate / 索引 / 包体留本地，不Force Add；Git由用户操作，助手未改索引、Commit、Push或联网核验远端。

本次修复前错误Core路径已暂存为新增；提交前先取消旧路径的暂存新增，再Stage正确UI路径，确认最终Staged只有UI目录的一份完整测试。助手已移动工作区文件并更新本文，未代改暂存区。
