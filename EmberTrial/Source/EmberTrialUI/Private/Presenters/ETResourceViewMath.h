#pragma once

#include "Math/UnrealMathUtility.h"

// Presenter共享的纯数值约束；不读取Actor / ASC，也不把合法区间等同于GameplayReady。
namespace ETResourceViewMath
{
inline bool IsValidRange(float Current, float Maximum)
{
    // 先排除非有限值；零资源有效，只有最大值必须为正。
    return FMath::IsFinite(Current) && FMath::IsFinite(Maximum) && Maximum > 0.0f &&
           Current >= 0.0f && Current <= Maximum;
}
} // namespace ETResourceViewMath
