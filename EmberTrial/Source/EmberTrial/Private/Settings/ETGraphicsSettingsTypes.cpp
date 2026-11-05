#include "Settings/ETGraphicsSettingsTypes.h"

bool FETGraphicsPreferences::IsValid() const
{
    return QualityLevel >= CustomQuality && QualityLevel <= MaxQuality &&
        FMath::IsFinite(FrameRateLimit) && FrameRateLimit >= 0.0f;
}

int32 FETGraphicsSettingsRules::FindUniformQuality(const TConstArrayView<int32> Groups)
{
    if (Groups.IsEmpty())
    {
        return FETGraphicsPreferences::InvalidQuality;
    }

    const int32 First = Groups[0];
    bool bUniform = true;
    for (const int32 Value : Groups)
    {
        if (Value < 0 || Value > FETGraphicsPreferences::MaxQuality)
        {
            return FETGraphicsPreferences::InvalidQuality;
        }
        bUniform &= Value == First;
    }
    return bUniform ? First : FETGraphicsPreferences::CustomQuality;
}

bool FETGraphicsSettingsRules::TryResolve(const FETGraphicsPreferences& Request,
    const FETGraphicsPreferences& Current, FETGraphicsPreferences& Out)
{
    // 输入可与输出别名；失败输出保持Invalid，不夹限后伪装为接受。
    const FETGraphicsPreferences RequestCopy = Request;
    const FETGraphicsPreferences CurrentCopy = Current;
    Out = FETGraphicsPreferences{};
    if (!RequestCopy.IsValid() || !CurrentCopy.IsValid())
    {
        return false;
    }

    Out = RequestCopy;
    if (Out.QualityLevel == FETGraphicsPreferences::CustomQuality)
    {
        Out.QualityLevel = CurrentCopy.QualityLevel;
    }
    return true;
}