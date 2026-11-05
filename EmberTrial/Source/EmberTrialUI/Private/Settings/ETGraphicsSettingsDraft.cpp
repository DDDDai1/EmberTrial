#include "Settings/ETGraphicsSettingsDraft.h"

// ---- 页面会话与本卡默认；不调用全局SetToDefaults ----

bool FETGraphicsSettingsDraft::Open(const FETGraphicsPreferences& Applied)
{
    const FETGraphicsPreferences Copy = Applied;
    Close();
    if (!Copy.IsValid())
    {
        return false;
    }
    Baseline = Value = Copy;
    bOpen = true;
    return true;
}

void FETGraphicsSettingsDraft::Close()
{
    bOpen = false;
    Baseline = Value = FETGraphicsPreferences{};
}

FETGraphicsPreferences FETGraphicsSettingsDraft::Defaults()
{
    FETGraphicsPreferences Result;
    Result.QualityLevel = 1;
    Result.FrameRateLimit = 60.0f;
    Result.bUseVSync = false;
    return Result;
}

void FETGraphicsSettingsDraft::UseDefaults()
{
    if (bOpen)
    {
        Value = Defaults();
    }
}

// ---- 选择目录；保留合法的自定义 / 非常用帧率，不伪造混合画质 ----

void FETGraphicsSettingsDraft::GetQualityLevels(TArray<int32>& Out) const
{
    Out.Reset();
    if (!bOpen)
    {
        return;
    }
    if (Baseline.QualityLevel == FETGraphicsPreferences::CustomQuality)
    {
        Out.Add(FETGraphicsPreferences::CustomQuality);
    }
    for (int32 Level = 0; Level <= FETGraphicsPreferences::MaxQuality; ++Level)
    {
        Out.Add(Level);
    }
}

void FETGraphicsSettingsDraft::GetFrameRateLimits(TArray<float>& Out) const
{
    Out.Reset();
    if (!bOpen)
    {
        return;
    }
    Out = {0.0f, 30.0f, 60.0f, 120.0f};
    if (!Out.ContainsByPredicate([this](float Limit)
        { return FMath::IsNearlyEqual(Limit, Baseline.FrameRateLimit); }))
    {
        Out.Add(Baseline.FrameRateLimit);
        Out.Sort();
    }
}

bool FETGraphicsSettingsDraft::SetQuality(const int32 Level)
{
    TArray<int32> Choices;
    GetQualityLevels(Choices);
    if (!bOpen || !Choices.Contains(Level))
    {
        return false;
    }
    Value.QualityLevel = Level;
    return true;
}

bool FETGraphicsSettingsDraft::SetFrameRateLimit(const float Limit)
{
    TArray<float> Choices;
    GetFrameRateLimits(Choices);
    const int32 Index = Choices.IndexOfByPredicate([Limit](float Choice)
        { return FMath::IsNearlyEqual(Choice, Limit); });
    if (!bOpen || !FMath::IsFinite(Limit) || Index == INDEX_NONE)
    {
        return false;
    }
    Value.FrameRateLimit = Choices[Index];
    return true;
}

bool FETGraphicsSettingsDraft::SetVSync(const bool bEnabled)
{
    if (!bOpen)
    {
        return false;
    }
    Value.bUseVSync = bEnabled;
    return true;
}

// ---- 最终候选；过期草稿拒绝，不覆盖其他入口刚应用的偏好 ----

bool FETGraphicsSettingsDraft::Same(
    const FETGraphicsPreferences& A, const FETGraphicsPreferences& B)
{
    return A.IsValid() && B.IsValid() && A.QualityLevel == B.QualityLevel &&
        FMath::IsNearlyEqual(A.FrameRateLimit, B.FrameRateLimit) &&
        A.bUseVSync == B.bUseVSync;
}

bool FETGraphicsSettingsDraft::TryMakeApply(
    const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out) const
{
    const FETGraphicsPreferences Copy = Current;
    Out = FETGraphicsPreferences{};
    return bOpen && IsDirty() && Same(Copy, Baseline) &&
        FETGraphicsSettingsRules::TryResolve(Value, Copy, Out);
}