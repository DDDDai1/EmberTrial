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

namespace
{
// 菜单相对HUD的既有层级，不与Pause / Failure的独立装配混用。
constexpr int32 MainMenuWidgetZOrder = 100;
} // namespace

// ---- 默认配置与本地菜单创建 ----

AETMainMenuController::AETMainMenuController()
{
    bAutoManageActiveCameraTarget = false;
    GameplayMap = TSoftObjectPtr<UWorld>(
        FSoftObjectPath(TEXT("/Game/EmberTrial/Maps/L_ET_VerticalSlice.L_ET_VerticalSlice")));
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
    SessionFailureHandle =
        Session->OnTravelFailed.AddUObject(this, &ThisClass::HandleSessionFailure);
    MenuWidget = CreateWidget<UETMainMenuWidget>(this, MainMenuWidgetClass);
    if (!IsValid(MenuWidget.Get()) || !MenuWidget->HasRequiredLayout())
    {
        UE_LOG(LogETMainMenu, Warning,
               TEXT("[M1-06C] Menu creation or layout failed Controller=%s"), *GetNameSafe(this));
        RemoveMenuWidget();
        return;
    }
    MenuWidget->OnStartRequested.AddUniqueDynamic(this, &ThisClass::HandleStartRequested);
    MenuWidget->OnContinueRequested.AddUniqueDynamic(this, &ThisClass::HandleContinueRequested);
    MenuWidget->OnQuitRequested.AddUniqueDynamic(this, &ThisClass::HandleQuitRequested);
    const bool bCanContinue = Session->HasContinueSnapshot();
    MenuWidget->ShowReady(bCanContinue);
    if (!MenuWidget->AddToPlayerScreen(MainMenuWidgetZOrder))
    {
        RemoveMenuWidget();
        return;
    }
    ApplyMenuInputMode();
    UE_LOG(LogETMainMenu, Display,
           TEXT("[M1-06C] Menu ready Controller=%s Widget=%s Pawn=%s Continue=%d"),
           *GetNameSafe(this), *GetNameSafe(MenuWidget.Get()), *GetNameSafe(GetPawn()),
           bCanContinue ? 1 : 0);
}

// ---- 共用当前状态检查；开始 / 继续额外检查World，退出保留原边界 ----

bool AETMainMenuController::CanHandleMenuAction() const
{
    return !bEndingPlay && !bTransitionPending && IsLocalController() && IsValid(MenuWidget.Get());
}

void AETMainMenuController::ApplyMenuInputMode()
{
    // 两个调用点已确认View有效；UIOnly仍清键，避免菜单输入穿入新世界。
    bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(this, MenuWidget.Get(),
                                                   EMouseLockMode::DoNotLock, true);
    MenuWidget->FocusStartButton();
}

// ---- 三种菜单意图；先pending，再调用可能同步失败 / 回调的会话服务 ----

void AETMainMenuController::HandleStartRequested()
{
    if (!CanHandleMenuAction() || !GetWorld())
    {
        return;
    }
    bTransitionPending = true;
    MenuWidget->ShowPending(FText::FromString(TEXT("正在开始新试炼…")));
    FText Error;
    if (!Session.IsValid() ||
        !Session->RequestNewRun(GetWorld(), GameplayMap.ToSoftObjectPath(), Error))
    {
        RecoverAfterFailure(Error.IsEmpty() ? FText::FromString(TEXT("试炼会话不可用，请重试。"))
                                            : Error);
    }
}

void AETMainMenuController::HandleContinueRequested()
{
    if (!CanHandleMenuAction() || !GetWorld())
    {
        return;
    }
    bTransitionPending = true;
    MenuWidget->ShowPending(FText::FromString(TEXT("正在继续本次试炼…")));
    FText Error;
    if (!Session.IsValid() || !Session->RequestContinue(GetWorld(), Error))
    {
        RecoverAfterFailure(
            Error.IsEmpty() ? FText::FromString(TEXT("试炼会话不可用，请重新开始。")) : Error);
    }
}

void AETMainMenuController::HandleQuitRequested()
{
    if (!CanHandleMenuAction())
    {
        return;
    }
    bTransitionPending = true;
    MenuWidget->ShowPending(FText::FromString(TEXT("正在退出…")));
    UE_LOG(LogETMainMenu, Display, TEXT("[M1-06C] Quit requested Controller=%s"),
           *GetNameSafe(this));
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

// ---- 只接受当前世界失败；解除pending后恢复同一View的输入 ----

void AETMainMenuController::HandleSessionFailure(UWorld* FailedWorld, const FText& Message)
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
    MenuWidget->ShowError(Message, Session.IsValid() && Session->HasContinueSnapshot());
    ApplyMenuInputMode();
}

// ---- 主动解绑与结束清理；不清除Session上其他对象的订阅 ----

void AETMainMenuController::RemoveMenuWidget()
{
    if (IsValid(MenuWidget.Get()))
    {
        const FString PreviousName = GetNameSafe(MenuWidget.Get());
        MenuWidget->OnStartRequested.RemoveDynamic(this, &ThisClass::HandleStartRequested);
        MenuWidget->OnContinueRequested.RemoveDynamic(this, &ThisClass::HandleContinueRequested);
        MenuWidget->OnQuitRequested.RemoveDynamic(this, &ThisClass::HandleQuitRequested);
        MenuWidget->RemoveFromParent();
        UE_LOG(LogETMainMenu, Display, TEXT("[M1-06C] Menu removed Controller=%s Widget=%s"),
               *GetNameSafe(this), *PreviousName);
    }
    MenuWidget = nullptr;
}

void AETMainMenuController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 先封住迟到请求，再撤销失败回调与View；父类清理最后执行。
    bEndingPlay = true;
    if (Session.IsValid())
    {
        Session->OnTravelFailed.Remove(SessionFailureHandle);
    }
    SessionFailureHandle.Reset();
    Session.Reset();
    RemoveMenuWidget();
    UE_LOG(LogETMainMenu, Display, TEXT("[M1-06C] Menu controller end Controller=%s Reason=%d"),
           *GetNameSafe(this), static_cast<int32>(EndPlayReason));
    Super::EndPlay(EndPlayReason);
}
