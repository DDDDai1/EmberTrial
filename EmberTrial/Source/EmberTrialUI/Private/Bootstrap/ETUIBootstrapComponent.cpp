#include "Bootstrap/ETUIBootstrapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"
#include "Widgets/ETLifecycleStatusWidget.h"
#include "Widgets/ETHealthHUDWidget.h"
#include "Engine/LocalPlayer.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Presenters/ETHealthPresenter.h"
#include "Presenters/ETStaminaPresenter.h"
#include "Widgets/ETStaminaHUDWidget.h"
#include "Presenters/ETHealingPresenter.h"
#include "Widgets/ETHealingHUDWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUIBootstrap, Log, All);

namespace
{
    // HUD内部相对层级属于装配契约；保持原值，不改变Pause / Failure的独立层级。
    constexpr int32 StatusWidgetZOrder = 5;
    constexpr int32 HealthWidgetZOrder = 4;
    constexpr int32 StaminaWidgetZOrder = 3;
    constexpr int32 HealingWidgetZOrder = 2;
} // namespace

// ---- Controller组件生命周期 ----

UETUIBootstrapComponent::UETUIBootstrapComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UETUIBootstrapComponent::BeginPlay()
{
    bUIShuttingDown = false;
    Super::BeginPlay();
    RestartHealthObservation();
    RestartStaminaObservation();
    RestartHealingObservation();
    RebuildStatusWidget();
    RebuildHealthWidget();
    RebuildStaminaWidget();
    RebuildHealingWidget();
}

void UETUIBootstrapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 先拒绝重入；让仍被外部引用的旧View立即停止接收事件。
    bUIShuttingDown = true;
    RemoveHealingWidget();
    RemoveStaminaWidget();
    RemoveHealthWidget();
    RemoveStatusWidget();
    if (HealingPresenter)
    {
        HealingPresenter->Shutdown();
        HealingPresenter = nullptr;
    }
    if (StaminaPresenter)
    {
        StaminaPresenter->Shutdown();
        StaminaPresenter = nullptr;
    }
    if (HealthPresenter)
    {
        HealthPresenter->Shutdown();
        HealthPresenter = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

APlayerController* UETUIBootstrapComponent::ResolveLocalUIController() const
{
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() || !Controller->GetLocalPlayer())
    {
        return nullptr;
    }
    return Controller;
}

// ---- 开发状态面板：保留独立的订阅与日志时序 ----

void UETUIBootstrapComponent::RebuildStatusWidget()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    RemoveStatusWidget();
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        return;
    }
    if (!StatusWidgetClass)
    {
        UE_LOG(LogETUIBootstrap, Warning, TEXT("[M1-02C2] Missing StatusWidgetClass Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    StatusWidget = CreateWidget<UETLifecycleStatusWidget>(Controller, StatusWidgetClass);
    if (!StatusWidget)
    {
        UE_LOG(LogETUIBootstrap, Warning, TEXT("[M1-02C2] Widget creation failed Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    if (!StatusWidget->AddToPlayerScreen(StatusWidgetZOrder))
    {
        UE_LOG(LogETUIBootstrap, Warning, TEXT("[M1-02C2] AddToPlayerScreen failed Owner=%s"),
               *GetNameSafe(Controller));
        RemoveStatusWidget();
        return;
    }
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M1-02C2] Widget created Owner=%s Widget=%s"),
           *GetNameSafe(Controller), *GetNameSafe(StatusWidget.Get()));
}

void UETUIBootstrapComponent::RemoveStatusWidget()
{
    if (!StatusWidget)
    {
        return;
    }
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M1-02C2] Widget removed Owner=%s Widget=%s"),
           *GetNameSafe(GetOwner()), *GetNameSafe(StatusWidget.Get()));
    StatusWidget->StopObserving();
    StatusWidget->RemoveFromParent();
    StatusWidget = nullptr;
}

// ---- 生命HUD：重启数据观察与重建View是两个独立操作 ----

UETHealthPresenter* UETUIBootstrapComponent::GetHealthPresenter() const
{
    return HealthPresenter.Get();
}

void UETUIBootstrapComponent::RestartHealthObservation()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        if (HealthPresenter)
        {
            HealthPresenter->Shutdown();
        }
        return;
    }
    if (!HealthPresenter)
    {
        HealthPresenter = NewObject<UETHealthPresenter>(this);
    }
    HealthPresenter->StartObserving(
        Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>());
}

UETHealthHUDWidget* UETUIBootstrapComponent::GetHealthWidget() const
{
    return HealthWidget.Get();
}

void UETUIBootstrapComponent::RebuildHealthWidget()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    // 保持先移除旧View再校验创建条件的顺序，避免失败后遗留旧展示。
    RemoveHealthWidget();
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        return;
    }
    if (!HealthWidgetClass || !IsValid(HealthPresenter.Get()))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M1-03B2] Missing HealthWidgetClass or Presenter Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    HealthWidget = CreateWidget<UETHealthHUDWidget>(Controller, HealthWidgetClass);
    if (!HealthWidget)
    {
        UE_LOG(LogETUIBootstrap, Warning, TEXT("[M1-03B2] Health widget creation failed Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    if (!HealthWidget->AddToPlayerScreen(HealthWidgetZOrder))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M1-03B2] Health AddToPlayerScreen failed Owner=%s"),
               *GetNameSafe(Controller));
        RemoveHealthWidget();
        return;
    }
    HealthWidget->StartObserving(HealthPresenter.Get());
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M1-03B2] Health widget created Owner=%s Widget=%s"),
           *GetNameSafe(Controller), *GetNameSafe(HealthWidget.Get()));
}

void UETUIBootstrapComponent::RemoveHealthWidget()
{
    if (!HealthWidget)
    {
        return;
    }
    HealthWidget->StopObserving();
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M1-03B2] Health widget removed Owner=%s Widget=%s"),
           *GetNameSafe(GetOwner()), *GetNameSafe(HealthWidget.Get()));
    HealthWidget->RemoveFromParent();
    HealthWidget = nullptr;
}

// ---- 体力HUD：保留自己的Presenter / Widget和失败清理路径 ----

UETStaminaPresenter* UETUIBootstrapComponent::GetStaminaPresenter() const
{
    return StaminaPresenter.Get();
}

void UETUIBootstrapComponent::RestartStaminaObservation()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        if (StaminaPresenter)
        {
            StaminaPresenter->Shutdown();
        }
        return;
    }
    if (!StaminaPresenter)
    {
        StaminaPresenter = NewObject<UETStaminaPresenter>(this);
    }
    StaminaPresenter->StartObserving(
        Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>());
}

UETStaminaHUDWidget* UETUIBootstrapComponent::GetStaminaWidget() const
{
    return StaminaWidget.Get();
}

void UETUIBootstrapComponent::RebuildStaminaWidget()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    // 重建失败时也不保留旧View；Presenter及资源数值仍由原对象持有。
    RemoveStaminaWidget();
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        return;
    }
    if (!StaminaWidgetClass || !IsValid(StaminaPresenter.Get()))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M2-01B2] Missing StaminaWidgetClass or Presenter Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    StaminaWidget = CreateWidget<UETStaminaHUDWidget>(Controller, StaminaWidgetClass);
    if (!StaminaWidget)
    {
        UE_LOG(LogETUIBootstrap, Warning, TEXT("[M2-01B2] Stamina widget creation failed Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    if (!StaminaWidget->AddToPlayerScreen(StaminaWidgetZOrder))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M2-01B2] Stamina AddToPlayerScreen failed Owner=%s"),
               *GetNameSafe(Controller));
        RemoveStaminaWidget();
        return;
    }
    StaminaWidget->StartObserving(StaminaPresenter.Get());
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M2-01B2] Stamina widget created Owner=%s Widget=%s"),
           *GetNameSafe(Controller), *GetNameSafe(StaminaWidget.Get()));
}

void UETUIBootstrapComponent::RemoveStaminaWidget()
{
    if (!StaminaWidget)
    {
        return;
    }
    StaminaWidget->StopObserving();
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M2-01B2] Stamina widget removed Owner=%s Widget=%s"),
           *GetNameSafe(GetOwner()), *GetNameSafe(StaminaWidget.Get()));
    StaminaWidget->RemoveFromParent();
    StaminaWidget = nullptr;
}

// ---- 治疗次数HUD：事件快照与界面重建分离 ----

UETHealingPresenter* UETUIBootstrapComponent::GetHealingPresenter() const
{
    return HealingPresenter.Get();
}

void UETUIBootstrapComponent::RestartHealingObservation()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        if (HealingPresenter)
        {
            HealingPresenter->Shutdown();
        }
        return;
    }
    if (!HealingPresenter)
    {
        HealingPresenter = NewObject<UETHealingPresenter>(this);
    }
    HealingPresenter->StartObserving(
        Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>());
}

UETHealingHUDWidget* UETUIBootstrapComponent::GetHealingWidget() const
{
    return HealingWidget.Get();
}

void UETUIBootstrapComponent::RebuildHealingWidget()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    // 先停旧View，再创建新View；Presenter与资源次数保持原对象所有权。
    RemoveHealingWidget();
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        return;
    }
    if (!HealingWidgetClass || !IsValid(HealingPresenter.Get()))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M2-04C] Missing HealingWidgetClass or Presenter Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    HealingWidget = CreateWidget<UETHealingHUDWidget>(Controller, HealingWidgetClass);
    if (!HealingWidget)
    {
        UE_LOG(LogETUIBootstrap, Warning, TEXT("[M2-04C] Healing widget creation failed Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    if (!HealingWidget->AddToPlayerScreen(HealingWidgetZOrder))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M2-04C] Healing AddToPlayerScreen failed Owner=%s"),
               *GetNameSafe(Controller));
        RemoveHealingWidget();
        return;
    }
    HealingWidget->StartObserving(HealingPresenter.Get());
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M2-04C] Healing widget created Owner=%s Widget=%s"),
           *GetNameSafe(Controller), *GetNameSafe(HealingWidget.Get()));
}

void UETUIBootstrapComponent::RemoveHealingWidget()
{
    if (!HealingWidget)
    {
        return;
    }
    HealingWidget->StopObserving();
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M2-04C] Healing widget removed Owner=%s Widget=%s"),
           *GetNameSafe(GetOwner()), *GetNameSafe(HealingWidget.Get()));
    HealingWidget->RemoveFromParent();
    HealingWidget = nullptr;
}