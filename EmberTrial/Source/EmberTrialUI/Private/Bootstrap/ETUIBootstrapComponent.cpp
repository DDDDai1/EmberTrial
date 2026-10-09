#include "Bootstrap/ETUIBootstrapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"
#include "Widgets/ETLifecycleStatusWidget.h"
#include "Widgets/ETHealthHUDWidget.h"
#include "Engine/LocalPlayer.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Presenters/ETHealthPresenter.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUIBootstrap, Log, All);

UETUIBootstrapComponent::UETUIBootstrapComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UETUIBootstrapComponent::BeginPlay()
{
    bUIShuttingDown = false;
    Super::BeginPlay();
    RestartHealthObservation();
    RebuildStatusWidget();
    RebuildHealthWidget();
}

void UETUIBootstrapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bUIShuttingDown = true;
    RemoveHealthWidget();
    RemoveStatusWidget();
    if (HealthPresenter)
    {
        HealthPresenter->Shutdown();
        HealthPresenter = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void UETUIBootstrapComponent::RebuildStatusWidget()
{
    if (!HasBegunPlay() || bUIShuttingDown)
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
    StatusWidget = CreateWidget<UETLifecycleStatusWidget>(Controller, StatusWidgetClass);
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
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() || !Controller->GetLocalPlayer())
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
    RemoveHealthWidget();
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController()
        || !Controller->GetLocalPlayer())
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
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M1-03B2] Health widget creation failed Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }
    if (!HealthWidget->AddToPlayerScreen(4))
    {
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M1-03B2] Health AddToPlayerScreen failed Owner=%s"),
            *GetNameSafe(Controller));
        RemoveHealthWidget();
        return;
    }
    HealthWidget->StartObserving(HealthPresenter.Get());
    UE_LOG(LogETUIBootstrap, Display,
        TEXT("[M1-03B2] Health widget created Owner=%s Widget=%s"),
        *GetNameSafe(Controller), *GetNameSafe(HealthWidget.Get()));
}

void UETUIBootstrapComponent::RemoveHealthWidget()
{
    if (!HealthWidget)
    {
        return;
    }
    HealthWidget->StopObserving();
    UE_LOG(LogETUIBootstrap, Display,
        TEXT("[M1-03B2] Health widget removed Owner=%s Widget=%s"),
        *GetNameSafe(GetOwner()), *GetNameSafe(HealthWidget.Get()));
    HealthWidget->RemoveFromParent();
    HealthWidget = nullptr;
}