#include "FrontEnd/ETMainMenuController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/LogMacros.h"
#include "Widgets/ETMainMenuWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMainMenu, Log, All);

AETMainMenuController::AETMainMenuController()
{
    bAutoManageActiveCameraTarget = false;
    GameplayMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(
        TEXT("/Game/EmberTrial/Maps/L_ET_VerticalSlice.L_ET_VerticalSlice")));
}

void AETMainMenuController::BeginPlay()
{
    Super::BeginPlay();
    bEndingPlay = false;
    bTransitionPending = false;
    if (!IsLocalController() || !GetLocalPlayer() || GetNetMode() != NM_Standalone)
    {
        return;
    }
    UGameInstance* Instance = GetGameInstance();
    Session = Instance ? Instance->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!Session.IsValid() || !MainMenuWidgetClass)
    {
        UE_LOG(LogETMainMenu, Warning,
            TEXT("[M1-06C] Missing session or MainMenuWidgetClass Controller=%s"),
            *GetNameSafe(this));
        return;
    }
    SessionFailureHandle = Session->OnTravelFailed.AddUObject(
        this, &ThisClass::HandleSessionFailure);
    MenuWidget = CreateWidget<UETMainMenuWidget>(this, MainMenuWidgetClass);
    if (!IsValid(MenuWidget.Get()) || !MenuWidget->HasRequiredLayout())
    {
        UE_LOG(LogETMainMenu, Warning,
            TEXT("[M1-06C] Menu creation or layout failed Controller=%s"),
            *GetNameSafe(this));
        RemoveMenuWidget();
        return;
    }
    MenuWidget->OnStartRequested.AddUniqueDynamic(
        this, &ThisClass::HandleStartRequested);
    MenuWidget->OnContinueRequested.AddUniqueDynamic(
        this, &ThisClass::HandleContinueRequested);
    MenuWidget->OnQuitRequested.AddUniqueDynamic(
        this, &ThisClass::HandleQuitRequested);
    const bool bCanContinue = Session->HasContinueSnapshot();
    MenuWidget->ShowReady(bCanContinue);
    if (!MenuWidget->AddToPlayerScreen(100))
    {
        RemoveMenuWidget();
        return;
    }
    bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        this, MenuWidget.Get(), EMouseLockMode::DoNotLock, true);
    MenuWidget->FocusStartButton();
    UE_LOG(LogETMainMenu, Display,
        TEXT("[M1-06C] Menu ready Controller=%s Widget=%s Pawn=%s Continue=%d"),
        *GetNameSafe(this), *GetNameSafe(MenuWidget.Get()), *GetNameSafe(GetPawn()),
        bCanContinue ? 1 : 0);
}

void AETMainMenuController::HandleStartRequested()
{
    if (bEndingPlay || bTransitionPending || !IsLocalController() ||
        !IsValid(MenuWidget.Get()) || !GetWorld())
    {
        return;
    }
    bTransitionPending = true;
    MenuWidget->ShowPending(FText::FromString(TEXT("正在开始新试炼…")));
    FText Error;
    if (!Session.IsValid() ||
        !Session->RequestNewRun(GetWorld(), GameplayMap.ToSoftObjectPath(), Error))
    {
        RecoverAfterFailure(Error.IsEmpty()
            ? FText::FromString(TEXT("试炼会话不可用，请重试。")) : Error);
    }
}

void AETMainMenuController::HandleContinueRequested()
{
    if (bEndingPlay || bTransitionPending || !IsLocalController() ||
        !IsValid(MenuWidget.Get()) || !GetWorld())
    {
        return;
    }
    bTransitionPending = true;
    MenuWidget->ShowPending(FText::FromString(TEXT("正在继续本次试炼…")));
    FText Error;
    if (!Session.IsValid() || !Session->RequestContinue(GetWorld(), Error))
    {
        RecoverAfterFailure(Error.IsEmpty()
            ? FText::FromString(TEXT("试炼会话不可用，请重新开始。")) : Error);
    }
}

void AETMainMenuController::HandleQuitRequested()
{
    if (bEndingPlay || bTransitionPending || !IsLocalController() ||
        !IsValid(MenuWidget.Get()))
    {
        return;
    }
    bTransitionPending = true;
    MenuWidget->ShowPending(FText::FromString(TEXT("正在退出…")));
    UE_LOG(LogETMainMenu, Display,
        TEXT("[M1-06C] Quit requested Controller=%s"), *GetNameSafe(this));
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AETMainMenuController::HandleSessionFailure(
    UWorld* FailedWorld, const FText& Message)
{
    if (!bEndingPlay && FailedWorld == GetWorld() && IsValid(MenuWidget.Get()))
    {
        RecoverAfterFailure(Message);
    }
}

void AETMainMenuController::RecoverAfterFailure(const FText& Message)
{
    bTransitionPending = false;
    if (bEndingPlay || !IsValid(MenuWidget.Get()) || !IsLocalController())
    {
        return;
    }
    MenuWidget->ShowError(Message,
        Session.IsValid() && Session->HasContinueSnapshot());
    bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        this, MenuWidget.Get(), EMouseLockMode::DoNotLock, true);
    MenuWidget->FocusStartButton();
}

void AETMainMenuController::RemoveMenuWidget()
{
    if (IsValid(MenuWidget.Get()))
    {
        const FString PreviousName = GetNameSafe(MenuWidget.Get());
        MenuWidget->OnStartRequested.RemoveDynamic(
            this, &ThisClass::HandleStartRequested);
        MenuWidget->OnContinueRequested.RemoveDynamic(
            this, &ThisClass::HandleContinueRequested);
        MenuWidget->OnQuitRequested.RemoveDynamic(
            this, &ThisClass::HandleQuitRequested);
        MenuWidget->RemoveFromParent();
        UE_LOG(LogETMainMenu, Display,
            TEXT("[M1-06C] Menu removed Controller=%s Widget=%s"),
            *GetNameSafe(this), *PreviousName);
    }
    MenuWidget = nullptr;
}

void AETMainMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (Session.IsValid())
    {
        Session->OnTravelFailed.Remove(SessionFailureHandle);
    }
    SessionFailureHandle.Reset();
    Session.Reset();
    RemoveMenuWidget();
    UE_LOG(LogETMainMenu, Display,
        TEXT("[M1-06C] Menu controller end Controller=%s Reason=%d"),
        *GetNameSafe(this), static_cast<int32>(EndPlayReason));
    Super::EndPlay(EndPlayReason);
}