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