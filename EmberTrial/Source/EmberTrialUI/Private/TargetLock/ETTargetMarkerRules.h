#pragma once

#include "CoreMinimal.h"

namespace ETTargetMarkerRules
{
    // Position和Size均为玩家界面的UMG单位；不在这里重复除DPI或做世界坐标投影。
    inline bool ShouldShow(bool bProjected, const FVector2D& Position, const FVector2D& Size,
                           float Inset)
    {
        return bProjected && !Position.ContainsNaN() && !Size.ContainsNaN() && FMath::IsFinite(Inset) &&
               Inset >= 0.0f && Size.X > 2.0 * Inset && Size.Y > 2.0 * Inset && Position.X >= Inset &&
               Position.Y >= Inset && Position.X <= Size.X - Inset && Position.Y <= Size.Y - Inset;
    }
} // namespace ETTargetMarkerRules