#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ETMenuGameMode.generated.h"

class APlayerController;

// 菜单世界只装配Controller；UMG由UI模块负责，核心规则类不依赖UI类型。
UCLASS()
class EMBERTRIAL_API AETMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AETMenuGameMode();

protected:
    // 不走默认RestartPlayer路径，避免菜单生成游戏身体。
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    // 跳过传统AHUD初始化；HUDClass=None本身不会阻止ClientSetHUD尝试Spawn。
    virtual void InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer) override;
};
