#include "Settings/ETAudioSettingsDraft.h"

// ---- 页面会话：复制基线，取消 / 重开取新快照，默认只影响当前卡 ----

bool FETAudioSettingsDraft::Open(const FETAudioPreferences& Applied)
{
    const FETAudioPreferences Copy = Applied;
    Close();
    if (!Copy.IsValid())
    {
        return false;
    }
    Baseline = Value = Copy;
    bOpen = true;
    return true;
}

void FETAudioSettingsDraft::Close()
{
    bOpen = false;
    Baseline = Value = FETAudioPreferences{};
}

bool FETAudioSettingsDraft::SetValues(const FETAudioPreferences& Request)
{
    if (!bOpen || !Request.IsValid())
    {
        return false;
    }
    Value = Request;
    return true;
}

void FETAudioSettingsDraft::UseDefaults()
{
    if (bOpen)
    {
        Value = FETAudioPreferences{};
    }
}

// ---- 最终提交：旧页面不能覆盖其他入口刚应用的值 ----

bool FETAudioSettingsDraft::Same(const FETAudioPreferences& A, const FETAudioPreferences& B)
{
    constexpr float Tolerance = 0.0001f; // 小于百分比显示精度，不把滑条浮点误差视为修改。
    return A.IsValid() && B.IsValid() &&
        FMath::IsNearlyEqual(A.MasterVolume, B.MasterVolume, Tolerance) &&
        FMath::IsNearlyEqual(A.MusicVolume, B.MusicVolume, Tolerance) &&
        FMath::IsNearlyEqual(A.EffectsVolume, B.EffectsVolume, Tolerance);
}

bool FETAudioSettingsDraft::TryMakeApply(
    const FETAudioPreferences& Current, FETAudioPreferences& Out) const
{
    const FETAudioPreferences Copy = Current;
    Out = FETAudioPreferences{};
    if (!bOpen || !IsDirty() || !Same(Copy, Baseline) || !Value.IsValid())
    {
        return false;
    }
    Out = Value;
    return true;
}