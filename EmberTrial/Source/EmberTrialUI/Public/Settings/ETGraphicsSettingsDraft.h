#pragma once

#include "CoreMinimal.h"
#include "Settings/ETGraphicsSettingsTypes.h"

/** 页面值副本；关闭丢弃，应用前重验可见偏好快照，不持有引擎对象。 */
class EMBERTRIALUI_API FETGraphicsSettingsDraft
{
public:
    bool Open(const FETGraphicsPreferences& Applied);
    void Close();
    bool SetQuality(int32 Level);
    bool SetFrameRateLimit(float Limit);
    bool SetVSync(bool bEnabled);
    void UseDefaults();

    void GetQualityLevels(TArray<int32>& Out) const;
    void GetFrameRateLimits(TArray<float>& Out) const;
    bool TryMakeApply(const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out) const;

    bool IsOpen() const { return bOpen; }
    bool IsDirty() const { return bOpen && !Same(Value, Baseline); }
    bool IsDefault() const { return bOpen && Same(Value, Defaults()); }
    const FETGraphicsPreferences& GetValue() const { return Value; }
    static FETGraphicsPreferences Defaults();

private:
    static bool Same(const FETGraphicsPreferences& A, const FETGraphicsPreferences& B);
    FETGraphicsPreferences Baseline;
    FETGraphicsPreferences Value;
    bool bOpen = false;
};