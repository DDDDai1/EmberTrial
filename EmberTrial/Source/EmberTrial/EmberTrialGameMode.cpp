// Copyright Epic Games, Inc. All Rights Reserved.

#include "EmberTrialGameMode.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AEmberTrialGameMode::AEmberTrialGameMode()
{
    // 旧模板 / 默认规则兼容路径：仅需要APawn类型，不依赖具体Character实现。
    // 资源路径仍是现有默认出生契约；构造时查找方式与找到后才赋值的行为保留。
    static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
        TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
    if (PlayerPawnBPClass.Class != nullptr)
    {
        DefaultPawnClass = PlayerPawnBPClass.Class;
    }
}
