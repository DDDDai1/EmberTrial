#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;

namespace ETCombatDamage
{
    struct FResult
    {
        // 拒绝原因使用静态文本。空值表示已尝试提交GE。
        const TCHAR* Rejection = nullptr;
        bool bApplied = false;
        bool bActiveHandle = false;
        bool bHasPostHitSnapshot = false;
        float HealthBefore = 0.0f;
        float HealthAfter = 0.0f;
        float BaseAfter = 0.0f;
        float MaxAfter = 0.0f;
    };

    bool IsValidConfiguration(TSubclassOf<UGameplayEffect> EffectClass, float Damage);

    FResult ApplyHit(AEmberTrialCharacter* Source, UAbilitySystemComponent* SourceASC,
                     const FHitResult& Hit, TSubclassOf<UGameplayEffect> EffectClass,
                     float Damage, float Level, const UObject* SourceObject,
                     AActor* EffectCauser = nullptr);
}