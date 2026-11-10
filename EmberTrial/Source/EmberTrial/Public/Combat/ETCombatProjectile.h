#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETCombatProjectile.generated.h"

class AETCombatProjectile;
class UProjectileMovementComponent;
class USphereComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETCombatProjectileImpact, AETCombatProjectile*, const FHitResult&);

// 一次性投射物。调用者决定何时发射以及如何结算命中。
UCLASS(Blueprintable)
class EMBERTRIAL_API AETCombatProjectile : public AActor
{
    GENERATED_BODY()

public:
    AETCombatProjectile();

    bool TryLaunch(AActor* InSource, const FVector& WorldDirection);
    AActor* GetSourceActor() const { return SourceActor.Get(); }

    FETCombatProjectileImpact OnImpact;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
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

    TWeakObjectPtr<AActor> SourceActor;
    bool bLaunched = false;
    bool bResolved = false;
};