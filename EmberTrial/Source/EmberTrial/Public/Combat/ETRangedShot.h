#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "ETRangedShot.generated.h"

class AEmberTrialCharacter;
class AETCombatProjectile;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETRangedShotConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Ranged Shot")
    TSubclassOf<AETCombatProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot", meta = (ClampMin = "0.01"))
    float Damage = 12.0f;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot", meta = (ClampMin = "1.0", Units = "cm"))
    float MinRange = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot", meta = (ClampMin = "1.0", Units = "cm"))
    float MaxRange = 1200.0f;
};

namespace ETRangedShot
{
    // 发射时以角色中心瞄准，不在飞行途中跟踪目标。
    EMBERTRIAL_API AETCombatProjectile* TryFire(
        AEmberTrialCharacter* Source, AEmberTrialCharacter* Target,
        const FETRangedShotConfig& Config, float AbilityLevel = 1.0f);
}