#include "Framework/ETPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPlayerController, Log, All);

void AETPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (IsLocalController() && GetLocalPlayer())
    {
        FInputModeGameOnly GameInput;
        GameInput.SetConsumeCaptureMouseDown(false);
        SetInputMode(GameInput);
        bShowMouseCursor = false;
        FlushPressedKeys();
        UE_LOG(LogETPlayerController, Display,
            TEXT("[M1-06A] Game input ready Controller=%s"), *GetNameSafe(this));
    }
    UE_LOG(LogETPlayerController, Display,
        TEXT("[M1-02B] Controller BeginPlay Controller=%s Local=%d Pawn=%s"),
        *GetNameSafe(this), IsLocalController() ? 1 : 0, *GetNameSafe(GetPawn()));
}

void AETPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    UE_LOG(LogETPlayerController, Display,
        TEXT("[M1-02B] Possess Controller=%s Pawn=%s"),
        *GetNameSafe(this), *GetNameSafe(GetPawn()));
}

void AETPlayerController::OnUnPossess()
{
    const FString PreviousPawnName = GetNameSafe(GetPawn());
    Super::OnUnPossess();
    UE_LOG(LogETPlayerController, Display,
        TEXT("[M1-02B] UnPossess Controller=%s PreviousPawn=%s CurrentPawn=%s"),
        *GetNameSafe(this), *PreviousPawnName, *GetNameSafe(GetPawn()));
}

void AETPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogETPlayerController, Display,
        TEXT("[M1-02B] Controller EndPlay Controller=%s Reason=%d"),
        *GetNameSafe(this), static_cast<int32>(EndPlayReason));
    Super::EndPlay(EndPlayReason);
}