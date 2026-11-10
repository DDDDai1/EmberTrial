#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "ETCombatProjectile.generated.h"

class AEmberTrialCharacter;
class AETCombatProjectile;
class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETCombatProjectileImpact, AETCombatProjectile*, const FHitResult&);

// 一次性投射物。可纯飞行，也可按发射时快照结算生命伤害。
UCLASS(Blueprintable)
class EMBERTRIAL_API AETCombatProjectile : public AActor
{
    GENERATED_BODY()

public:
    AETCombatProjectile();

    bool TryLaunch(AActor* InSource, const FVector& WorldDirection);
    bool TryLaunchDamage(AEmberTrialCharacter* InSource, const FVector& WorldDirection,
                         TSubclassOf<UGameplayEffect> InEffectClass, float Damage, float Level = 1.0f);
    AActor* GetSourceActor() const { return SourceActor.Get(); }

    FETCombatProjectileImpact OnImpact;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool TryLaunchInternal(AActor* InSource, const FVector& WorldDirection,
                           TSubclassOf<UGameplayEffect> InEffectClass, float Damage, float Level);
    void ApplyDamageOnImpact(const FHitResult& Hit);

    UFUNCTION()
    void HandleProjectileStopped(const FHitResult& Hit);

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "1.0", Units = "cm/s"))
    float LaunchSpeed = 1600.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0.01", Units = "s"))
    float FlightLifetime = 3.0f;

    // 飞行期间持有GE类，发射者仍是弱引用。
    UPROPERTY(Transient)
    TSubclassOf<UGameplayEffect> FlightDamageEffectClass;

    float FlightDamage = 0.0f;
    float FlightDamageLevel = 1.0f;
    TWeakObjectPtr<AActor> SourceActor;
    bool bLaunched = false;
    bool bResolved = false;
};