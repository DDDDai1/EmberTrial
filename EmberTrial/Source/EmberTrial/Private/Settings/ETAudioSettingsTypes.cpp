#include "Settings/ETAudioSettingsTypes.h"

namespace ETAudioPreferencesPrivate
{
    bool IsValidVolume(const float Value)
    {
        return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
    }
} // namespace ETAudioPreferencesPrivate

bool FETAudioPreferences::IsValid() const
{
    return ETAudioPreferencesPrivate::IsValidVolume(MasterVolume) &&
        ETAudioPreferencesPrivate::IsValidVolume(MusicVolume) &&
        ETAudioPreferencesPrivate::IsValidVolume(EffectsVolume);
}

bool FETAudioPreferences::TryGetEffectiveVolumes(float& OutMusic, float& OutEffects) const
{
    const FETAudioPreferences Copy = *this; // 输出可引用自身字段，先保存输入。
    OutMusic = OutEffects = 0.0f;
    if (!Copy.IsValid())
    {
        return false;
    }
    OutMusic = Copy.MasterVolume * Copy.MusicVolume;
    OutEffects = Copy.MasterVolume * Copy.EffectsVolume;
    return true;
}