#include "FrontEnd/ETMainMenuController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logging/LogMacros.h"
#include "Misc/PackageName.h"
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
    if (!MainMenuWidgetClass)
    {
        UE_LOG(LogETMainMenu, Warning,
            TEXT("[M1-06A] Missing MainMenuWidgetClass Controller=%s"),
            *GetNameSafe(this));
        return;
    }
    MenuWidget = CreateWidget<UETMainMenuWidget>(this, MainMenuWidgetClass);
    if (!IsValid(MenuWidget.Get()) || !MenuWidget->HasRequiredLayout())
    {
        UE_LOG(LogETMainMenu, Warning,
            TEXT("[M1-06A] Menu creation or layout failed Controller=%s"),
            *GetNameSafe(this));
        RemoveMenuWidget();
        return;
    }
    MenuWidget->OnStartRequested.AddUniqueDynamic(
        this, &AETMainMenuController::HandleStartRequested);
    MenuWidget->OnQuitRequested.AddUniqueDynamic(
        this, &AETMainMenuController::HandleQuitRequested);
    MenuWidget->ShowReady();
    if (!MenuWidget->AddToPlayerScreen(100))
    {
        UE_LOG(LogETMainMenu, Warning,
            TEXT("[M1-06A] Menu AddToPlayerScreen failed Controller=%s"),
            *GetNameSafe(this));
        RemoveMenuWidget();
        return;
    }
    bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        this, MenuWidget.Get(), EMouseLockMode::DoNotLock, true);
    MenuWidget->FocusStartButton();
    if (GEngine)
    {
        TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
            this, &AETMainMenuController::HandleTravelFailure);
    }
    UE_LOG(LogETMainMenu, Display,
        TEXT("[M1-06A] Menu ready Controller=%s Widget=%s Pawn=%s"),
        *GetNameSafe(this), *GetNameSafe(MenuWidget.Get()), *GetNameSafe(GetPawn()));
}

void AETMainMenuController::HandleStartRequested()
{
    if (bEndingPlay || bTransitionPending || !IsLocalController() ||
        !IsValid(MenuWidget.Get()) || !GetWorld())
    {
        return;
    }
    const FString PackageName = GameplayMap.ToSoftObjectPath().GetLongPackageName();
    const bool bValidMap = !GameplayMap.IsNull() &&
        FPackageName::IsValidLongPackageName(PackageName) &&
        FPackageName::DoesPackageExist(PackageName) &&
        FPackageName::GetShortName(PackageName) !=
            UGameplayStatics::GetCurrentLevelName(this, true);
    if (!bValidMap)
    {
        UE_LOG(LogETMainMenu, Warning,
            TEXT("[M1-06A] Game map unavailable Path=%s"), *PackageName);
        RecoverAfterFailure(FText::FromString(
            TEXT("游戏地图不可用，请检查主菜单配置。")));
        return;
    }
    bTransitionPending = true;
    MenuWidget->ShowPending(FText::FromString(TEXT("正在进入试炼…")));
    UE_LOG(LogETMainMenu, Display,
        TEXT("[M1-06A] Start requested Controller=%s Map=%s"),
        *GetNameSafe(this), *PackageName);
    UGameplayStatics::OpenLevel(this, FName(*PackageName), true);
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
        TEXT("[M1-06A] Quit requested Controller=%s"), *GetNameSafe(this));
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AETMainMenuController::HandleTravelFailure(UWorld* FailedWorld,
    ETravelFailure::Type FailureType, const FString& ErrorString)
{
    if (bEndingPlay || !bTransitionPending || FailedWorld != GetWorld())
    {
        return;
    }
    UE_LOG(LogETMainMenu, Warning,
        TEXT("[M1-06A] Travel failed Type=%d Error=%s"),
        static_cast<int32>(FailureType), *ErrorString);
    RecoverAfterFailure(FText::FromString(TEXT("进入试炼失败，请重试。")));
}

void AETMainMenuController::RecoverAfterFailure(const FText& Message)
{
    bTransitionPending = false;
    if (bEndingPlay || !IsValid(MenuWidget.Get()) || !IsLocalController())
    {
        return;
    }
    MenuWidget->ShowError(Message);
    bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        this, MenuWidget.Get(), EMouseLockMode::DoNotLock, true);
    MenuWidget->FocusStartButton();
}

void AETMainMenuController::RemoveMenuWidget()
{
    if (!IsValid(MenuWidget.Get()))
    {
        MenuWidget = nullptr;
        return;
    }
    const FString PreviousName = GetNameSafe(MenuWidget.Get());
    MenuWidget->OnStartRequested.RemoveDynamic(
        this, &AETMainMenuController::HandleStartRequested);
    MenuWidget->OnQuitRequested.RemoveDynamic(
        this, &AETMainMenuController::HandleQuitRequested);
    MenuWidget->RemoveFromParent();
    MenuWidget = nullptr;
    UE_LOG(LogETMainMenu, Display,
        TEXT("[M1-06A] Menu removed Controller=%s Widget=%s"),
        *GetNameSafe(this), *PreviousName);
}

void AETMainMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (GEngine && TravelFailureHandle.IsValid())
    {
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    }
    TravelFailureHandle.Reset();
    RemoveMenuWidget();
    UE_LOG(LogETMainMenu, Display,
        TEXT("[M1-06A] Menu controller end Controller=%s Reason=%d"),
        *GetNameSafe(this), static_cast<int32>(EndPlayReason));
    Super::EndPlay(EndPlayReason);
}