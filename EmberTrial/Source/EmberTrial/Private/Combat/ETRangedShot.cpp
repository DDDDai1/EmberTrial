#include "Combat/ETRangedShot.h"

#include "Characters/EmberTrialCharacter.h"
#include "CollisionQueryParams.h"
#include "Combat/ETCombatDamage.h"
#include "Combat/ETCombatProjectile.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRangedShot, Log, All);

namespace
{
    bool IsReadyRangedShotBody(const AEmberTrialCharacter* Body)
    {
        float Health = 0.0f;
        float MaxHealth = 0.0f;
        return IsValid(Body) && !Body->IsActorBeingDestroyed() && !Body->IsCombatDead() &&
               Body->GetGameplayHealthSnapshot(Health, MaxHealth) &&
               FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) &&
               Health > 0.0f && MaxHealth >= 1.0f && Health <= MaxHealth;
    }
}

AETCombatProjectile* ETRangedShot::TryFire(
    AEmberTrialCharacter* Source, AEmberTrialCharacter* Target,
    const FETRangedShotConfig& Config, float AbilityLevel)
{
    if (!IsReadyRangedShotBody(Source) || !IsReadyRangedShotBody(Target) ||
        !Source->HasAuthority() || Source->GetCombatTeam() != EETCombatTeam::Enemy ||
        !Source->CanDealCombatDamageTo(Target))
    {
        return nullptr;
    }

    UWorld* World = Source->GetWorld();
    UClass* ProjectileClass = Config.ProjectileClass.Get();
    if (!World || World != Target->GetWorld() || !World->HasBegunPlay() || World->IsPaused() ||
        !IsValid(ProjectileClass) || ProjectileClass->HasAnyClassFlags(CLASS_Abstract) ||
        !ETCombatDamage::IsValidConfiguration(Config.DamageEffectClass, Config.Damage) ||
        !FMath::IsFinite(AbilityLevel) || AbilityLevel <= 0.0f ||
        !FMath::IsFinite(Config.MinRange) || !FMath::IsFinite(Config.MaxRange) ||
        Config.MinRange < 1.0f || Config.MaxRange < Config.MinRange)
    {
        return nullptr;
    }

    const FVector Origin = Source->GetActorLocation();
    const FVector Aim = Target->GetActorLocation();
    const FVector Delta = Aim - Origin;
    const double DistanceSquared = Delta.SizeSquared();
    if (Origin.ContainsNaN() || Aim.ContainsNaN() || !FMath::IsFinite(DistanceSquared) ||
        DistanceSquared < FMath::Square(static_cast<double>(Config.MinRange)) ||
        DistanceSquared > FMath::Square(static_cast<double>(Config.MaxRange)))
    {
        return nullptr;
    }

    FCollisionQueryParams SightQuery(SCENE_QUERY_STAT(ETRangedShotSight), false, Source);
    SightQuery.AddIgnoredActor(Target);
    if (World->LineTraceTestByChannel(Origin, Aim, ECC_Visibility, SightQuery))
    {
        return nullptr;
    }

    // SpawnActor会执行BeginPlay。先快照发射参数，避免同步回调修改配置。
    const TSubclassOf<UGameplayEffect> DamageEffectClass = Config.DamageEffectClass;
    const float Damage = Config.Damage;
    const FVector Direction = Delta.GetSafeNormal();
    const TWeakObjectPtr<AEmberTrialCharacter> WeakSource(Source);
    const TWeakObjectPtr<AEmberTrialCharacter> WeakTarget(Target);

    FActorSpawnParameters Spawn;
    Spawn.Owner = Source;
    Spawn.Instigator = Source;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AETCombatProjectile* Shot = World->SpawnActor<AETCombatProjectile>(
        ProjectileClass, Origin, Direction.Rotation(), Spawn);
    if (!IsValid(Shot))
    {
        return nullptr;
    }

    AEmberTrialCharacter* LiveSource = WeakSource.Get();
    AEmberTrialCharacter* LiveTarget = WeakTarget.Get();
    if (!IsReadyRangedShotBody(LiveSource) || !IsReadyRangedShotBody(LiveTarget) ||
        !LiveSource->CanDealCombatDamageTo(LiveTarget) ||
        !Shot->TryLaunchDamage(LiveSource, Direction, DamageEffectClass, Damage, AbilityLevel) ||
        !IsValid(Shot) || Shot->IsActorBeingDestroyed())
    {
        if (IsValid(Shot) && !Shot->IsActorBeingDestroyed())
        {
            Shot->Destroy();
        }
        return nullptr;
    }

    UE_LOG(LogETRangedShot, Display,
           TEXT("[M4-01C1] Fired Source=%s Target=%s Projectile=%s Distance=%.1f Damage=%.1f"),
           *GetNameSafe(LiveSource), *GetNameSafe(LiveTarget), *GetNameSafe(Shot),
           FMath::Sqrt(DistanceSquared), Damage);
    return Shot;
}