#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETUILifecycle, Log, All);

// ---- LocalPlayer子系统生命周期；初始化日志不代表战斗属性Ready ----

void UETUILocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    UE_LOG(LogETUILifecycle, Display, TEXT("[M1-02C1] UI local ready LocalPlayer=%s"),
           *GetNameSafe(LocalPlayer));

    APlayerController* InitialController = nullptr;
    if (LocalPlayer && LocalPlayer->GetWorld())
    {
        InitialController = LocalPlayer->GetPlayerController(LocalPlayer->GetWorld());
    }
    RebindController(InitialController);
}

void UETUILocalPlayerSubsystem::Deinitialize()
{
    ClearController(TEXT("SubsystemDeinitialize"));

    UE_LOG(LogETUILifecycle, Display, TEXT("[M1-02C1] UI local deinitialized LocalPlayer=%s"),
           *GetNameSafe(GetLocalPlayer()));

    Super::Deinitialize();
}

void UETUILocalPlayerSubsystem::PlayerControllerChanged(APlayerController* NewController)
{
    Super::PlayerControllerChanged(NewController);
    RebindController(NewController);
}

// ---- 只读身份快照 ----

APlayerController* UETUILocalPlayerSubsystem::GetObservedController() const
{
    return ObservedController.Get();
}

APawn* UETUILocalPlayerSubsystem::GetObservedPawn() const
{
    return ObservedPawn.Get();
}

// ---- Controller重绑与去重；外来LocalPlayer的Controller不能替换当前观察 ----

void UETUILocalPlayerSubsystem::RebindController(APlayerController* NewController)
{
    if (!IsValid(NewController))
    {
        NewController = nullptr;
    }
    else if (NewController->GetLocalPlayer() != GetLocalPlayer())
    {
        UE_LOG(LogETUILifecycle, Warning, TEXT("[M1-02C1] Reject foreign Controller=%s"),
               *GetNameSafe(NewController));
        return;
    }

    // 两个Get都为null时，旧弱引用仍可能stale，不能只按指针相等跳过清理。
    if (ObservedController.Get() == NewController && !ObservedController.IsStale())
    {
        RefreshObservedPawn(TEXT("ControllerSnapshot"));
        return;
    }

    ClearController(TEXT("ControllerChanged"));

    if (!NewController)
    {
        return;
    }

    ObservedController = NewController;
    NewController->OnPossessedPawnChanged.AddUniqueDynamic(
        this, &UETUILocalPlayerSubsystem::HandlePossessedPawnChanged);
    NewController->OnEndPlay.AddUniqueDynamic(this,
                                              &UETUILocalPlayerSubsystem::HandleControllerEndPlay);

    UE_LOG(LogETUILifecycle, Display, TEXT("[M1-02C1] Controller bound Controller=%s"),
           *GetNameSafe(NewController));

    RefreshObservedPawn(TEXT("ControllerSnapshot"));
}

void UETUILocalPlayerSubsystem::ClearController(const TCHAR* Reason)
{
    APlayerController* PreviousController = ObservedController.Get();
    if (PreviousController)
    {
        PreviousController->OnPossessedPawnChanged.RemoveDynamic(
            this, &UETUILocalPlayerSubsystem::HandlePossessedPawnChanged);
        PreviousController->OnEndPlay.RemoveDynamic(
            this, &UETUILocalPlayerSubsystem::HandleControllerEndPlay);
    }

    // 先撤销Controller身份，再发布Pawn清空，重入查询也只能看到当前状态。
    ObservedController.Reset();
    SetObservedPawn(nullptr, Reason);

    if (PreviousController)
    {
        UE_LOG(LogETUILifecycle, Display,
               TEXT("[M1-02C1] Controller cleared Controller=%s Reason=%s"),
               *GetNameSafe(PreviousController), Reason);
    }
}

void UETUILocalPlayerSubsystem::RefreshObservedPawn(const TCHAR* Reason)
{
    APlayerController* Controller = ObservedController.Get();
    SetObservedPawn(Controller ? Controller->GetPawn() : nullptr, Reason);
}

// ---- Pawn订阅与变化发布；相同有效对象不重复广播 ----

void UETUILocalPlayerSubsystem::SetObservedPawn(APawn* NewPawn, const TCHAR* Reason)
{
    if (!IsValid(NewPawn))
    {
        NewPawn = nullptr;
    }

    APawn* PreviousPawn = ObservedPawn.Get();
    if (PreviousPawn == NewPawn && !ObservedPawn.IsStale())
    {
        return;
    }

    if (PreviousPawn)
    {
        PreviousPawn->OnEndPlay.RemoveDynamic(this, &UETUILocalPlayerSubsystem::HandlePawnEndPlay);
    }

    ObservedPawn = NewPawn;
    if (NewPawn)
    {
        NewPawn->OnEndPlay.AddUniqueDynamic(this, &UETUILocalPlayerSubsystem::HandlePawnEndPlay);
    }

    UE_LOG(LogETUILifecycle, Display,
           TEXT("[M1-02C1] Pawn changed Previous=%s Current=%s Reason=%s"),
           *GetNameSafe(PreviousPawn), *GetNameSafe(NewPawn), Reason);

    OnObservedPawnChanged.Broadcast(PreviousPawn, NewPawn);
}

// ---- 引擎回调只重查当前Pawn，不把可能过期的事件Payload当作真值 ----

void UETUILocalPlayerSubsystem::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    (void)OldPawn;
    (void)NewPawn;
    RefreshObservedPawn(TEXT("PossessionChanged"));
}

void UETUILocalPlayerSubsystem::HandleControllerEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
    (void)Reason;
    if (Actor == ObservedController.Get())
    {
        ClearController(TEXT("ControllerEndPlay"));
    }
}

void UETUILocalPlayerSubsystem::HandlePawnEndPlay(AActor* Actor, EEndPlayReason::Type Reason)
{
    (void)Reason;
    if (Actor == ObservedPawn.Get())
    {
        SetObservedPawn(nullptr, TEXT("PawnEndPlay"));
    }
}
