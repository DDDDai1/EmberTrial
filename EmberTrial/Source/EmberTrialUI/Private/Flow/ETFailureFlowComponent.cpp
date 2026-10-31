#include "Flow/ETFailureFlowComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "Framework/ETPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "Widgets/ETFailureWidget.h"
#include "Widgets/ETPauseWidget.h"
#include "Interaction/ETRelicAltar.h"

DEFINE_LOG_CATEGORY_STATIC(LogETFailureFlow, Log, All);

namespace
{
// 现有呈现层级；Failure覆盖Pause，值属于装配契约而非资源配置。
constexpr int32 PauseWidgetZOrder = 150;
constexpr int32 FailureWidgetZOrder = 200;
} // namespace

// ---- 组件初始化与外部事件注册；保留历史类名以兼容序列化配置 ----

UETFailureFlowComponent::UETFailureFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
    MainMenuMap = TSoftObjectPtr<UWorld>(
        FSoftObjectPath(TEXT("/Game/EmberTrial/Maps/L_ET_MainMenu.L_ET_MainMenu")));
}

void UETFailureFlowComponent::BeginPlay()
{
    bEndingPlay = false;
    Super::BeginPlay();
    AETPlayerController* Controller = Cast<AETPlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() || !Controller->GetLocalPlayer() ||
        !GetWorld() || GetWorld()->GetNetMode() != NM_Standalone)
    {
        return;
    }
    ControllerOwner = Controller;
    UIContext = Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>();
    UGameInstance* Instance = Controller->GetGameInstance();
    Session = Instance ? Instance->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!UIContext.IsValid() || !Session.IsValid())
    {
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Missing UI context or session Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    PauseRequestHandle =
        Controller->OnPauseRequested.AddUObject(this, &ThisClass::HandlePauseRequested);
    AltarRequestHandle = Controller->OnRelicAltarRequested.AddUObject(
        this, &ThisClass::HandleAltarRequested);
    UIContext->OnObservedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandleObservedPawnChanged);
    SessionFailureHandle =
        Session->OnTravelFailed.AddUObject(this, &ThisClass::HandleSessionFailure);
    BindPawn(UIContext->GetObservedController() == Controller ? UIContext->GetObservedPawn()
                                                              : nullptr);
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Game flow ready Owner=%s Session=%s"),
           *GetNameSafe(Controller), *GetNameSafe(Session.Get()));
}

// ---- 实时身份与身体状态查询，不缓存Pawn / Ready / 死亡结论 ----

bool UETFailureFlowComponent::HasCurrentContext() const
{
    AETPlayerController* Controller = ControllerOwner.Get();
    UWorld* World = GetWorld();
    return !bEndingPlay && IsValid(World) && !World->bIsTearingDown && IsValid(Controller) &&
           Controller->IsLocalController() && UIContext.IsValid() && Session.IsValid() &&
           UIContext->GetObservedController() == Controller;
}

bool UETFailureFlowComponent::HasCurrentBody(bool bExpectedDead) const
{
    if (!HasCurrentContext())
    {
        return false;
    }
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(ControllerOwner->GetPawn());
    UETCombatLifeComponent* Life = BoundLife.Get();
    return IsValid(Body) && UIContext->GetObservedPawn() == Body && Body->IsGameplayReady() &&
           IsValid(Life) && Life->GetOwner() == Body && Life->IsDead() == bExpectedDead;
}

bool UETFailureFlowComponent::HasCurrentDeadBody() const
{
    return HasCurrentBody(true);
}

// ---- 旧身体解绑与新Life快照；相同有效订阅去重 ----

void UETFailureFlowComponent::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    if (!bEndingPlay && UIContext.IsValid())
    {
        BindPawn(UIContext->GetObservedController() == ControllerOwner.Get() ? CurrentPawn
                                                                             : nullptr);
    }
}

void UETFailureFlowComponent::BindPawn(APawn* CurrentPawn)
{
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CurrentPawn);
    UETCombatLifeComponent* NextLife =
        IsValid(Body) ? Body->FindComponentByClass<UETCombatLifeComponent>() : nullptr;
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
    RemovePauseWidget();
    RemoveFailureWidget();
    RestoreOwnedInput();
    bCommandPending = false;
    if (bEndingPlay || !IsValid(NextLife))
    {
        return;
    }
    BoundLife = NextLife;
    DeathHandle = NextLife->OnDeathSealed.AddUObject(this, &ThisClass::HandleDeathSealed);
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Life bound Owner=%s Body=%s Dead=%d"),
           *GetNameSafe(GetOwner()), *GetNameSafe(CurrentPawn), NextLife->IsDead() ? 1 : 0);
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
        UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Life unbound Owner=%s Body=%s"),
               *GetNameSafe(GetOwner()), *GetNameSafe(Life->GetOwner()));
    }
    DeathHandle.Reset();
    BoundLife.Reset();
}

// ---- 存活暂停、继续与暂停所有权；不取消当前战斗动作 ----

void UETFailureFlowComponent::HandlePauseRequested()
{
    if (!HasCurrentBody(false) || bCommandPending || bPresentationQueued ||
        IsValid(FailureWidget.Get()) || IsValid(PauseWidget.Get()) || Session->IsTravelPending() ||
        UGameplayStatics::IsGamePaused(this))
    {
        return;
    }
    PresentPause();
}

void UETFailureFlowComponent::PresentPause(AETRelicAltar* SourceAltar)
{
    const bool bFromAltar = SourceAltar != nullptr;
    AETPlayerController* Controller = ControllerOwner.Get();
    AEmberTrialCharacter* OpeningBody = Controller
        ? Cast<AEmberTrialCharacter>(Controller->GetPawn()) : nullptr;
    if (!HasCurrentBody(false) || bCommandPending || bPresentationQueued ||
        IsValid(PauseWidget.Get()) || IsValid(FailureWidget.Get()) || Session->IsTravelPending() ||
        UGameplayStatics::IsGamePaused(this) ||
        (bFromAltar && (!IsValid(SourceAltar) || !SourceAltar->CanUseAltar(OpeningBody))))
    {
        return;
    }
    if (!PauseWidgetClass || PauseWidgetClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06D] Missing concrete PauseWidgetClass Owner=%s"), *GetNameSafe(GetOwner()));
        return;
    }
    PauseWidget = CreateWidget<UETPauseWidget>(Controller, PauseWidgetClass);
    if (!IsValid(PauseWidget.Get()) || !PauseWidget->HasRequiredLayout())
    {
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Pause layout invalid"));
        RemovePauseWidget();
        return;
    }
    PauseWidget->OnResumeIntent.AddUniqueDynamic(this, &ThisClass::HandleResumeRequested);
    PauseWidget->OnMainMenuIntent.AddUniqueDynamic(this, &ThisClass::HandleMainMenuRequested);
    if (bFromAltar)
    {
        PauseWidget->ShowAltarReady();
    }
    else
    {
        PauseWidget->ShowReady();
    }
    if (!PauseWidget->AddToPlayerScreen(PauseWidgetZOrder))
    {
        RemovePauseWidget();
        return;
    }
    // 创建 / 显示Widget可同步运行回调；在取得暂停前重验当前身体与来源。
    if (!IsValid(PauseWidget.Get()) || !HasCurrentBody(false) ||
        Controller->GetPawn() != OpeningBody || Session->IsTravelPending() ||
        UGameplayStatics::IsGamePaused(this) ||
        (bFromAltar && (!IsValid(SourceAltar) || !SourceAltar->CanUseAltar(OpeningBody))) ||
        !UGameplayStatics::SetGamePaused(this, true))
    {
        RemovePauseWidget();
        return;
    }
    bOwnsPause = true;
    if (bFromAltar)
    {
        ActiveAltar = SourceAltar;
        SourceAltar->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleAltarEndPlay);
    }
    OpeningBody->StopJumping();
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, PauseWidget.Get(),
        EMouseLockMode::DoNotLock, true);
    PauseWidget->FocusResume();
    NotifyMenuState();
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Pause shown Owner=%s Widget=%s Paused=1"),
        *GetNameSafe(Controller), *GetNameSafe(PauseWidget.Get()));
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M3-04B] Browse shown Owner=%s Altar=%s Mode=%s"), *GetNameSafe(Controller),
        *GetNameSafe(ActiveAltar.Get()), bFromAltar ? TEXT("Altar") : TEXT("Pause"));
}

void UETFailureFlowComponent::HandleResumeRequested()
{
    if (!HasCurrentContext() || !IsValid(PauseWidget.Get()) || bCommandPending ||
        Session->IsTravelPending())
    {
        return;
    }
    if (HasCurrentDeadBody())
    {
        QueueFailure();
        return;
    }
    if (UGameplayStatics::IsGamePaused(this) && !UGameplayStatics::SetGamePaused(this, false))
    {
        PauseWidget->ShowError(FText::FromString(TEXT("无法继续，请再次尝试。")));
        return;
    }
    RemovePauseWidget();
    RestoreOwnedInput();
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Pause resumed Owner=%s Paused=%d"),
           *GetNameSafe(GetOwner()), UGameplayStatics::IsGamePaused(this) ? 1 : 0);
}

// ---- 死亡封锁之后呈现；普通NextTick与已暂停立即替换两条路径保持 ----

void UETFailureFlowComponent::HandleDeathSealed(UETCombatLifeComponent* SourceLife)
{
    if (SourceLife == BoundLife.Get() && HasCurrentDeadBody())
    {
        QueueFailure();
    }
}

void UETFailureFlowComponent::QueueFailure()
{
    if (!HasCurrentDeadBody() || bPresentationQueued || IsValid(FailureWidget.Get()) ||
        bCommandPending)
    {
        return;
    }
    if (IsValid(PauseWidget.Get()) && bOwnsPause)
    {
        // A paused world cannot advance the normal next-tick timer.
        // Keep pause ownership while replacing this component's active view.
        PresentFailure();
        return;
    }
    bPresentationQueued = true;
    // Retain B's next-tick ordering after death and Controller BeginPlay.
    PresentationHandle =
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::PresentFailure);
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Failure queued Owner=%s Body=%s"),
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
    AETPlayerController* Controller = ControllerOwner.Get();
    if (!FailureWidgetClass || FailureWidgetClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETFailureFlow, Warning,
               TEXT("[M1-06D] Missing concrete FailureWidgetClass Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    FailureWidget = CreateWidget<UETFailureWidget>(Controller, FailureWidgetClass);
    if (!IsValid(FailureWidget.Get()) || !FailureWidget->HasRequiredLayout())
    {
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Failure layout invalid Owner=%s"),
               *GetNameSafe(Controller));
        RemoveFailureWidget();
        return;
    }
    FailureWidget->OnRetryRequested.AddUniqueDynamic(this, &ThisClass::HandleRetryRequested);
    FailureWidget->OnMainMenuRequested.AddUniqueDynamic(this, &ThisClass::HandleMainMenuRequested);
    FailureWidget->ShowReady();
    if (!FailureWidget->AddToPlayerScreen(FailureWidgetZOrder))
    {
        RemoveFailureWidget();
        return;
    }
    const bool bWasPaused = UGameplayStatics::IsGamePaused(this);
    if (!bWasPaused && !UGameplayStatics::SetGamePaused(this, true))
    {
        RemoveFailureWidget();
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Failure pause rejected"));
        return;
    }
    // Preserve ownership if this component had already opened its pause menu.
    bOwnsPause = bOwnsPause || !bWasPaused;
    RemovePauseWidget();
    bCommandPending = false;
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, FailureWidget.Get(),
                                                   EMouseLockMode::DoNotLock, true);
    FailureWidget->FocusRetryButton();
    NotifyMenuState();
    UE_LOG(LogETFailureFlow, Display,
           TEXT("[M1-06D] Failure shown Owner=%s Body=%s Widget=%s Paused=%d"),
           *GetNameSafe(Controller), *GetNameSafe(BoundLife->GetOwner()),
           *GetNameSafe(FailureWidget.Get()), UGameplayStatics::IsGamePaused(this) ? 1 : 0);
}

// ---- 会话命令；先pending，再调用可能同步失败的请求 ----

void UETFailureFlowComponent::HandleRetryRequested()
{
    if (bCommandPending || !HasCurrentDeadBody() || !IsValid(FailureWidget.Get()) ||
        Session->IsTravelPending())
    {
        return;
    }
    bCommandPending = true;
    FailureWidget->ShowPending(FText::FromString(TEXT("正在重新进入试炼…")));
    FText Error;
    if (!Session->RequestRetry(GetWorld(), Error))
    {
        RecoverActivePanel(Error);
    }
}

void UETFailureFlowComponent::HandleMainMenuRequested()
{
    if (!HasCurrentContext() || bCommandPending || Session->IsTravelPending())
    {
        return;
    }
    const bool bFromFailure = IsValid(FailureWidget.Get()) && HasCurrentDeadBody();
    const bool bFromPause = IsValid(PauseWidget.Get());
    if (!bFromFailure && !bFromPause)
    {
        return;
    }
    bCommandPending = true;
    const FText Message = FText::FromString(TEXT("正在返回主菜单…"));
    if (bFromFailure)
    {
        FailureWidget->ShowPending(Message);
    }
    else
    {
        PauseWidget->ShowPending(Message);
    }
    FText Error;
    if (!Session->RequestMainMenu(GetWorld(), MainMenuMap.ToSoftObjectPath(), Error))
    {
        RecoverActivePanel(Error);
    }
}

void UETFailureFlowComponent::HandleSessionFailure(UWorld* FailedWorld, const FText& Message)
{
    if (!bEndingPlay && FailedWorld == GetWorld() && bCommandPending)
    {
        RecoverActivePanel(Message);
    }
}

// ---- 失败恢复当前面板；不复活身体，不缓存过期会话结果 ----

void UETFailureFlowComponent::RecoverActivePanel(const FText& Message)
{
    bCommandPending = false;
    if (!HasCurrentContext())
    {
        return;
    }
    if (!ActiveAltar.IsExplicitlyNull() && !ActiveAltar.IsValid())
    {
        // 请求失败但原祭坛已结束，撤销该页面，不能恢复成一个无来源的祭坛上下文。
        HandleResumeRequested();
        return;
    }
    if (!UGameplayStatics::IsGamePaused(this))
    {
        if (!UGameplayStatics::SetGamePaused(this, true))
        {
            UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Recovery pause rejected"));
            return;
        }
        bOwnsPause = true;
    }
    if (HasCurrentDeadBody() && IsValid(PauseWidget.Get()))
    {
        QueueFailure();
        return;
    }
    const FText Error =
        Message.IsEmpty() ? FText::FromString(TEXT("请求未完成，请重新选择。")) : Message;
    AETPlayerController* Controller = ControllerOwner.Get();
    Controller->bShowMouseCursor = true;
    if (IsValid(FailureWidget.Get()) && HasCurrentDeadBody())
    {
        FailureWidget->ShowError(Error);
        UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, FailureWidget.Get(),
                                                       EMouseLockMode::DoNotLock, true);
        FailureWidget->FocusRetryButton();
    }
    else if (IsValid(PauseWidget.Get()))
    {
        PauseWidget->ShowError(Error);
        UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, PauseWidget.Get(),
                                                       EMouseLockMode::DoNotLock, true);
        PauseWidget->FocusResume();
    }
    NotifyMenuState();
    UE_LOG(LogETFailureFlow, Display,
           TEXT("[M1-06D] Panel recovered Owner=%s Message=%s Paused=%d"), *GetNameSafe(Controller),
           *Error.ToString(), UGameplayStatics::IsGamePaused(this) ? 1 : 0);
}

// ---- View主动解绑；不同意图名称与焦点保持各自契约 ----

void UETFailureFlowComponent::RemovePauseWidget()
{
    ClearAltarContext();
    if (IsValid(PauseWidget.Get()))
    {
        const FString WidgetName = GetNameSafe(PauseWidget.Get());
        PauseWidget->OnResumeIntent.RemoveDynamic(this, &ThisClass::HandleResumeRequested);
        PauseWidget->OnMainMenuIntent.RemoveDynamic(this, &ThisClass::HandleMainMenuRequested);
        PauseWidget->RemoveFromParent();
        UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Pause removed Owner=%s Widget=%s"),
               *GetNameSafe(GetOwner()), *WidgetName);
    }
    PauseWidget = nullptr;
}

void UETFailureFlowComponent::RemoveFailureWidget()
{
    if (IsValid(FailureWidget.Get()))
    {
        const FString WidgetName = GetNameSafe(FailureWidget.Get());
        FailureWidget->OnRetryRequested.RemoveDynamic(this, &ThisClass::HandleRetryRequested);
        FailureWidget->OnMainMenuRequested.RemoveDynamic(this, &ThisClass::HandleMainMenuRequested);
        FailureWidget->RemoveFromParent();
        UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Failure removed Owner=%s Widget=%s"),
               *GetNameSafe(GetOwner()), *WidgetName);
    }
    FailureWidget = nullptr;
}

void UETFailureFlowComponent::NotifyMenuState()
{
    if (HasCurrentContext())
    {
        ControllerOwner->OnGameMenuStateChanged();
    }
}

// ---- 只释放本组件的暂停；恢复GameOnly、首击、清键和视口焦点 ----

void UETFailureFlowComponent::RestoreOwnedInput()
{
    // 先撤销所有权，再校验当前身份；若解暂停失败才重新取回。
    const bool bReleasePause = bOwnsPause;
    bOwnsPause = false;
    if (!bReleasePause || !HasCurrentContext() || Session->IsTravelPending())
    {
        return;
    }
    if (UGameplayStatics::IsGamePaused(this) && !UGameplayStatics::SetGamePaused(this, false))
    {
        bOwnsPause = true;
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Input restore pause rejected"));
        return;
    }
    AETPlayerController* Controller = ControllerOwner.Get();
    FInputModeGameOnly GameInput;
    GameInput.SetConsumeCaptureMouseDown(false);
    Controller->SetInputMode(GameInput);
    Controller->bShowMouseCursor = false;
    Controller->FlushPressedKeys();
    UWidgetBlueprintLibrary::SetFocusToGameViewport();
    NotifyMenuState();
}

// ---- 结束先封口 / 清Timer / 解绑，不再更改正在结束的世界输入 ----

void UETFailureFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    CancelPresentation();
    if (AETPlayerController* Controller = ControllerOwner.Get())
    {
        Controller->OnPauseRequested.Remove(PauseRequestHandle);
        Controller->OnRelicAltarRequested.Remove(AltarRequestHandle);
    }
    PauseRequestHandle.Reset();
    AltarRequestHandle.Reset();
    if (UIContext.IsValid())
    {
        UIContext->OnObservedPawnChanged.RemoveDynamic(this, &ThisClass::HandleObservedPawnChanged);
    }
    if (Session.IsValid())
    {
        Session->OnTravelFailed.Remove(SessionFailureHandle);
    }
    SessionFailureHandle.Reset();
    StopObservingLife();
    RemovePauseWidget();
    RemoveFailureWidget();
    bOwnsPause = false;
    bCommandPending = false;
    UIContext.Reset();
    Session.Reset();
    ControllerOwner.Reset();
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Game flow end Owner=%s"),
           *GetNameSafe(GetOwner()));
    // No input/focus/pause changes in an ending world.
    Super::EndPlay(EndPlayReason);
}

void UETFailureFlowComponent::HandleAltarRequested(AETRelicAltar* SourceAltar)
{
    if (!HasCurrentBody(false) || !IsValid(SourceAltar) || SourceAltar->GetWorld() != GetWorld())
    {
        return;
    }
    PresentPause(SourceAltar);
}

void UETFailureFlowComponent::ClearAltarContext()
{
    // Get(true)仅用于解绑结束中的对象，不能拿这种对象授权装备或读取玩法状态。
    if (AETRelicAltar* Altar = ActiveAltar.Get(true))
    {
        Altar->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleAltarEndPlay);
    }
    ActiveAltar.Reset();
}

void UETFailureFlowComponent::HandleAltarEndPlay(AActor* SourceActor, EEndPlayReason::Type Reason)
{
    if (bEndingPlay || SourceActor != ActiveAltar.Get(true) || !HasCurrentContext())
    {
        return;
    }
    UE_LOG(LogETFailureFlow, Display, TEXT("[M3-04B] Altar source ended Owner=%s Reason=%d"),
        *GetNameSafe(GetOwner()), static_cast<int32>(Reason));
    // 切图中不恢复旧世界；若切图失败，RecoverActivePanel会处理失效的祭坛来源。
    if (!bCommandPending && !Session->IsTravelPending())
    {
        HandleResumeRequested();
    }
}