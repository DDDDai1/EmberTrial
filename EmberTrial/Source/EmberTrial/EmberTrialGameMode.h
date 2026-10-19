// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmberTrialGameMode.generated.h"

// 旧ThirdPerson兼容规则类：共享配置的GlobalDefaultGameMode与类重定向仍引用它。
// 正式试炼 / 菜单分别使用ETGameMode / ETMenuGameMode，不能据类名将此类删除。
UCLASS(minimalapi)
class AEmberTrialGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AEmberTrialGameMode();
};
