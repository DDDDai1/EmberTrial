#include "Flow/ETFailureFlowComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "Widgets/ETFailureWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogETFailureFlow, Log, All);

UETFailureFlowComponent::UETFailureFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
    MainMenuMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(
        TEXT("/Game/EmberTrial/Maps/L_ET_MainMenu.L_ET_MainMenu")));
}

void UETFailureFlowComponent::BeginPlay()
{
    bEndingPlay = false;
    Super::BeginPlay();
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() ||
        !Controller->GetLocalPlayer() || !GetWorld() ||
        GetWorld()->GetNetMode() != NM_Standalone)
    {
        return;
    }
    ControllerOwner = Controller;
    UIContext = Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>();
    UGameInstance* Instance = Controller->GetGameInstance();
    Session = Instance ? Instance->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!UIContext.IsValid() || !Session.IsValid())
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06B] Missing UI context or session Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }
    UIContext->OnObservedPawnChanged.AddUniqueDynamic(
        this, &ThisClass::HandleObservedPawnChanged);
    SessionFailureHandle = Session->OnTravelFailed.AddUObject(
        this, &ThisClass::HandleSessionFailure);
    BindPawn(UIContext->GetObservedController() == Controller
        ? UIContext->GetObservedPawn() : nullptr);
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure flow ready Owner=%s Session=%s"),
        *GetNameSafe(Controller), *GetNameSafe(Session.Get()));
}

void UETFailureFlowComponent::HandleObservedPawnChanged(
    APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    if (!bEndingPlay && UIContext.IsValid())
    {
        BindPawn(UIContext->GetObservedController() == ControllerOwner.Get()
            ? CurrentPawn : nullptr);
    }
}

void UETFailureFlowComponent::BindPawn(APawn* CurrentPawn)
{
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CurrentPawn);
    UETCombatLifeComponent* NextLife = IsValid(Body)
        ? Body->FindComponentByClass<UETCombatLifeComponent>() : nullptr;
    if (IsValid(NextLife) && BoundLife.Get() == NextLife && DeathHandle.IsValid())
    {
        if (HasCurrentDeadBody())
        {
            QueueFailure();
        }
        return;
    }
    CancelPresentation();
    StopObservingLife();
    RemoveFailureWidget();
    RestoreOwnedInput();
    bCommandPending = false;
    if (bEndingPlay || !IsValid(NextLife))
    {
        return;
    }
    BoundLife = NextLife;
    DeathHandle = NextLife->OnDeathSealed.AddUObject(
        this, &ThisClass::HandleDeathSealed);
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Life bound Owner=%s Body=%s Dead=%d"),
        *GetNameSafe(GetOwner()), *GetNameSafe(CurrentPawn),
        NextLife->IsDead() ? 1 : 0);
    if (HasCurrentDeadBody())
    {
        QueueFailure();
    }
}

void UETFailureFlowComponent::StopObservingLife()
{
    if (UETCombatLifeComponent* Life = BoundLife.Get())
    {
        Life->OnDeathSealed.Remove(DeathHandle);
        UE_LOG(LogETFailureFlow, Display,
            TEXT("[M1-06B] Life unbound Owner=%s Body=%s"),
            *GetNameSafe(GetOwner()), *GetNameSafe(Life->GetOwner()));
    }
    DeathHandle.Reset();
    BoundLife.Reset();
}

bool UETFailureFlowComponent::HasCurrentDeadBody() const
{
    APlayerController* Controller = ControllerOwner.Get();
    UETCombatLifeComponent* Life = BoundLife.Get();
    UWorld* World = GetWorld();
    AEmberTrialCharacter* Body = IsValid(Controller)
        ? Cast<AEmberTrialCharacter>(Controller->GetPawn()) : nullptr;
    return !bEndingPlay && IsValid(World) && !World->bIsTearingDown &&
        IsValid(Controller) && Controller->IsLocalController() &&
        UIContext.IsValid() && UIContext->GetObservedController() == Controller &&
        UIContext->GetObservedPawn() == Body && IsValid(Body) &&
        Body->IsGameplayReady() && IsValid(Life) && Life->GetOwner() == Body &&
        Life->IsDead();
}

void UETFailureFlowComponent::HandleDeathSealed(UETCombatLifeComponent* SourceLife)
{
    if (SourceLife == BoundLife.Get() && HasCurrentDeadBody())
    {
        QueueFailure();
    }
}

void UETFailureFlowComponent::QueueFailure()
{
    if (!HasCurrentDeadBody() || bPresentationQueued ||
        IsValid(FailureWidget.Get()) || bCommandPending)
    {
        return;
    }
    bPresentationQueued = true;
    // Run after this frame's death callbacks and Controller BeginPlay input setup.
    PresentationHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(
        this, &ThisClass::PresentFailure);
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure queued Owner=%s Body=%s"),
        *GetNameSafe(GetOwner()), *GetNameSafe(BoundLife->GetOwner()));
}

void UETFailureFlowComponent::CancelPresentation()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PresentationHandle);
    }
    PresentationHandle.Invalidate();
    bPresentationQueued = false;
}

void UETFailureFlowComponent::PresentFailure()
{
    bPresentationQueued = false;
    PresentationHandle.Invalidate();
    if (!HasCurrentDeadBody() || IsValid(FailureWidget.Get()))
    {
        return;
    }
    APlayerController* Controller = ControllerOwner.Get();
    if (!FailureWidgetClass || FailureWidgetClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06B] Missing concrete FailureWidgetClass Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }
    FailureWidget = CreateWidget<UETFailureWidget>(Controller, FailureWidgetClass);
    if (!IsValid(FailureWidget.Get()) || !FailureWidget->HasRequiredLayout())
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06B] Failure layout invalid Owner=%s"), *GetNameSafe(Controller));
        RemoveFailureWidget();
        return;
    }
    FailureWidget->OnRetryRequested.AddUniqueDynamic(
        this, &ThisClass::HandleRetryRequested);
    FailureWidget->OnMainMenuRequested.AddUniqueDynamic(
        this, &ThisClass::HandleMainMenuRequested);
    FailureWidget->ShowReady();
    if (!FailureWidget->AddToPlayerScreen(200))
    {
        RemoveFailureWidget();
        return;
    }
    const bool bWasPaused = UGameplayStatics::IsGamePaused(this);
    if (!bWasPaused && !UGameplayStatics::SetGamePaused(this, true))
    {
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06B] Failure pause rejected"));
        RemoveFailureWidget();
        return;
    }
    bOwnsPause = !bWasPaused;
    bCommandPending = false;
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        Controller, FailureWidget.Get(), EMouseLockMode::DoNotLock, true);
    FailureWidget->FocusRetryButton();
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure shown Owner=%s Body=%s Widget=%s Paused=%d"),
        *GetNameSafe(Controller), *GetNameSafe(BoundLife->GetOwner()),
        *GetNameSafe(FailureWidget.Get()), UGameplayStatics::IsGamePaused(this) ? 1 : 0);
}

void UETFailureFlowComponent::HandleRetryRequested()
{
    if (bCommandPending || !HasCurrentDeadBody() || !IsValid(FailureWidget.Get()))
    {
        return;
    }
    bCommandPending = true;
    FailureWidget->ShowPending(FText::FromString(TEXT("正在重新进入试炼…")));
    FText Error;
    if (!Session.IsValid() || !Session->RequestRetry(GetWorld(), Error))
    {
        RecoverFailure(Error.IsEmpty()
            ? FText::FromString(TEXT("试炼会话不可用，请重新开始。")) : Error);
    }
}

void UETFailureFlowComponent::HandleMainMenuRequested()
{
    if (bCommandPending || !HasCurrentDeadBody() || !IsValid(FailureWidget.Get()))
    {
        return;
    }
    bCommandPending = true;
    FailureWidget->ShowPending(FText::FromString(TEXT("正在返回主菜单…")));
    FText Error;
    if (!Session.IsValid() ||
        !Session->RequestMainMenu(GetWorld(), MainMenuMap.ToSoftObjectPath(), Error))
    {
        RecoverFailure(Error.IsEmpty()
            ? FText::FromString(TEXT("试炼会话不可用，请重试。")) : Error);
    }
}

void UETFailureFlowComponent::HandleSessionFailure(
    UWorld* FailedWorld, const FText& Message)
{
    if (!bEndingPlay && FailedWorld == GetWorld() &&
        bCommandPending && IsValid(FailureWidget.Get()))
    {
        RecoverFailure(Message);
    }
}

void UETFailureFlowComponent::RecoverFailure(const FText& Message)
{
    bCommandPending = false;
    if (!HasCurrentDeadBody() || !IsValid(FailureWidget.Get()))
    {
        return;
    }
    if (!UGameplayStatics::IsGamePaused(this))
    {
        bOwnsPause = UGameplayStatics::SetGamePaused(this, true);
    }
    FailureWidget->ShowError(Message);
    APlayerController* Controller = ControllerOwner.Get();
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        Controller, FailureWidget.Get(), EMouseLockMode::DoNotLock, true);
    FailureWidget->FocusRetryButton();
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure recovered Owner=%s Message=%s"),
        *GetNameSafe(Controller), *Message.ToString());
}

void UETFailureFlowComponent::RemoveFailureWidget()
{
    if (IsValid(FailureWidget.Get()))
    {
        const FString WidgetName = GetNameSafe(FailureWidget.Get());
        FailureWidget->OnRetryRequested.RemoveDynamic(
            this, &ThisClass::HandleRetryRequested);
        FailureWidget->OnMainMenuRequested.RemoveDynamic(
            this, &ThisClass::HandleMainMenuRequested);
        FailureWidget->RemoveFromParent();
        UE_LOG(LogETFailureFlow, Display,
            TEXT("[M1-06B] Failure removed Owner=%s Widget=%s"),
            *GetNameSafe(GetOwner()), *WidgetName);
    }
    FailureWidget = nullptr;
}

void UETFailureFlowComponent::RestoreOwnedInput()
{
    const bool bReleasePause = bOwnsPause;
    bOwnsPause = false;
    APlayerController* Controller = ControllerOwner.Get();
    UWorld* World = GetWorld();
    if (!bReleasePause || bEndingPlay || !IsValid(World) || World->bIsTearingDown ||
        !IsValid(Controller) || !UIContext.IsValid() ||
        UIContext->GetObservedController() != Controller ||
        (Session.IsValid() && Session->IsTravelPending()))
    {
        return;
    }
    UGameplayStatics::SetGamePaused(this, false);
    FInputModeGameOnly GameInput;
    GameInput.SetConsumeCaptureMouseDown(false);
    Controller->SetInputMode(GameInput);
    Controller->bShowMouseCursor = false;
    Controller->FlushPressedKeys();
}

void UETFailureFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    CancelPresentation();
    if (UIContext.IsValid())
    {
        UIContext->OnObservedPawnChanged.RemoveDynamic(
            this, &ThisClass::HandleObservedPawnChanged);
    }
    if (Session.IsValid())
    {
        Session->OnTravelFailed.Remove(SessionFailureHandle);
    }
    SessionFailureHandle.Reset();
    StopObservingLife();
    RemoveFailureWidget();
    bOwnsPause = false;
    bCommandPending = false;
    UIContext.Reset();
    Session.Reset();
    ControllerOwner.Reset();
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure flow end Owner=%s"), *GetNameSafe(GetOwner()));
    // Do not apply input or pause changes to an ending world.
    Super::EndPlay(EndPlayReason);
}