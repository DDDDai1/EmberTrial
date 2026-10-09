#include "Widgets/ETLifecycleStatusWidget.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETLifecycleWidget, Log, All);

#define LOCTEXT_NAMESPACE "ETLifecycleStatusWidget"

void UETLifecycleStatusWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    StopObserving();

    if (IsDesignTime())
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    UETUILocalPlayerSubsystem* Subsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>()
        : nullptr;

    if (Subsystem)
    {
        BoundSubsystem = Subsystem;
        Subsystem->OnObservedPawnChanged.AddUniqueDynamic(
            this, &UETLifecycleStatusWidget::HandleObservedPawnChanged);
        bObserving = true;

        UE_LOG(LogETLifecycleWidget, Display,
            TEXT("[M1-02C2] Widget subscribed Widget=%s LocalPlayer=%s"),
            *GetNameSafe(this), *GetNameSafe(LocalPlayer));
    }
    else
    {
        UE_LOG(LogETLifecycleWidget, Warning,
            TEXT("[M1-02C2] Missing local subsystem Widget=%s"),
            *GetNameSafe(this));
    }

    // 订阅后立即读取快照，覆盖 Widget 晚于 Possess 创建的情况。
    RefreshStatus(TEXT("ConstructSnapshot"));
}

void UETLifecycleStatusWidget::NativeDestruct()
{
    StopObserving();
    Super::NativeDestruct();
}

void UETLifecycleStatusWidget::StopObserving()
{
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(
            this, &UETLifecycleStatusWidget::HandleObservedPawnChanged);
    }

    if (bObserving)
    {
        UE_LOG(LogETLifecycleWidget, Display,
            TEXT("[M1-02C2] Widget unsubscribed Widget=%s"),
            *GetNameSafe(this));
    }

    BoundSubsystem.Reset();
    bObserving = false;
}

void UETLifecycleStatusWidget::HandleObservedPawnChanged(
    APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;

    if (bObserving)
    {
        RefreshStatus(TEXT("PawnEvent"));
    }
}

void UETLifecycleStatusWidget::RefreshStatus(const TCHAR* Reason)
{
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    APawn* Pawn = Subsystem ? Subsystem->GetObservedPawn() : nullptr;

    if (!StatusText || !PawnText)
    {
        UE_LOG(LogETLifecycleWidget, Warning,
            TEXT("[M1-02C2] Missing BindWidget fields Widget=%s"),
            *GetNameSafe(this));
        return;
    }

    const FText Status = !Subsystem
        ? LOCTEXT("Unavailable", "本地 UI 入口不可用")
        : (Pawn ? LOCTEXT("Controlled", "控制状态：已观察到 Pawn")
                : LOCTEXT("NoPawn", "控制状态：暂无 Pawn"));

    StatusText->SetText(Status);
    PawnText->SetText(FText::Format(
        LOCTEXT("PawnFormat", "当前 Pawn：{0}"),
        FText::FromString(GetNameSafe(Pawn))));

    UE_LOG(LogETLifecycleWidget, Display,
        TEXT("[M1-02C2] Widget snapshot Widget=%s Pawn=%s Reason=%s"),
        *GetNameSafe(this), *GetNameSafe(Pawn), Reason);
}

#undef LOCTEXT_NAMESPACE