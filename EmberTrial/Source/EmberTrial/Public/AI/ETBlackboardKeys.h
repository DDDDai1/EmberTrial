#pragma once

#include "CoreMinimal.h"

// Runtime与Editor共用的黑板名称契约，不保存任何敌人实例状态。
// 字符串必须与现有BB资产一致；C++符号改名不代表可以自动改资产键。
namespace ETBlackboardKeys
{
inline const FName TargetActor(TEXT("TargetActor"));
inline const FName CanPursue(TEXT("CanPursue"));
inline const FName CanMeleeAttack(TEXT("CanMeleeAttack"));
inline const FName PursuitStopDistance(TEXT("PursuitStopDistance"));
} // namespace ETBlackboardKeys
