#pragma once

#include "CoreMinimal.h"
#include "Settings/ETAudioSettingsTypes.h"

/** 页面纯值草稿；只有显式应用生成请求，不操作混音、World或配置文件。 */
class EMBERTRIALUI_API FETAudioSettingsDraft
{
public:
    bool Open(const FETAudioPreferences& Applied);
    void Close();
    bool SetValues(const FETAudioPreferences& Request);
    void UseDefaults();
    bool TryMakeApply(const FETAudioPreferences& Current, FETAudioPreferences& Out) const;

    bool IsOpen() const { return bOpen; }
    bool IsDirty() const { return bOpen && !Same(Value, Baseline); }
    bool IsDefault() const { return bOpen && Same(Value, FETAudioPreferences{}); }
    const FETAudioPreferences& GetValue() const { return Value; }

private:
    static bool Same(const FETAudioPreferences& A, const FETAudioPreferences& B);
    FETAudioPreferences Baseline;
    FETAudioPreferences Value;
    bool bOpen = false;
};