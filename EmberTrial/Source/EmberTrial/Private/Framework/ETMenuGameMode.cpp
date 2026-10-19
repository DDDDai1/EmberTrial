#include "Framework/ETMenuGameMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETMenuGameMode, Log, All);

AETMenuGameMode::AETMenuGameMode()
{
    // 三个None分别关闭默认身体、旁观身体和传统HUD；具体UI Controller由BP装配。
    DefaultPawnClass = nullptr;
    SpectatorClass = nullptr;
    HUDClass = nullptr;
}

void AETMenuGameMode::InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer)
{
    // 有意不调用Super：菜单没有AHUD；调用默认实现会尝试以空HUDClass生成Actor。
    (void)NewPlayer;
}

void AETMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    // 有意不调用Super：默认实现会RestartPlayer。这里只记录当前Controller / Pawn。
    UE_LOG(LogETMenuGameMode, Display, TEXT("[M1-06A] Menu player ready Controller=%s Pawn=%s"),
           *GetNameSafe(NewPlayer),
           *GetNameSafe(IsValid(NewPlayer) ? NewPlayer->GetPawn() : nullptr));
}
