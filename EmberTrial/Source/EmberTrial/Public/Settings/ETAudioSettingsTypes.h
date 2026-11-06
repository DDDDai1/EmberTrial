#pragma once

#include "CoreMinimal.h"
#include "ETAudioSettingsTypes.generated.h"

/** 独立线性音量偏好；不保存SoundClass / World，不代表声音已经应用。 */
USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETAudioPreferences
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float MasterVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float MusicVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio",
        meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
    float EffectsVolume = 1.0f;

    bool IsValid() const;

    // 仅诊断数学组合；后续父子SoundClass分别设置原始值，不能再乘一次Master。
    bool TryGetEffectiveVolumes(float& OutMusic, float& OutEffects) const;
};