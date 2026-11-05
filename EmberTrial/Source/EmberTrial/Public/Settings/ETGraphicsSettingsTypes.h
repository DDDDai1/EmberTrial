#pragma once

#include "CoreMinimal.h"

/** 非分辨率设置的值快照；Custom表示保留当前混合画质，默认对象不可提交。 */
struct EMBERTRIAL_API FETGraphicsPreferences
{
    static constexpr int32 InvalidQuality = -2;
    static constexpr int32 CustomQuality = -1;
    static constexpr int32 MaxQuality = 4; // UE5.5：低 / 中 / 高 / 极高 / 影视级。

    int32 QualityLevel = InvalidQuality;
    float FrameRateLimit = 0.0f; // 0表示取消本设置的上限，仍可能受VSync / 平台限制。
    bool bUseVSync = false;

    bool IsValid() const;
};

/** 只检查值，不访问引擎实例 / 窗口 / 磁盘；不把分辨率比例混入整体画质标签。 */
struct EMBERTRIAL_API FETGraphicsSettingsRules
{
    static int32 FindUniformQuality(TConstArrayView<int32> Groups);
    static bool TryResolve(const FETGraphicsPreferences& Request,
        const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out);
};