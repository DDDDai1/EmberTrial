#include "Combat/ETCombatProjectile.h"

#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatDamage.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCombatProjectile, Log, All);

AETCombatProjectile::AETCombatProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(10.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->SetGenerateOverlapEvents(false);
    Collision->SetCanEverAffectNavigation(false);
    SetRootComponent(Collision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->SetUpdatedComponent(Collision);
    ProjectileMovement->bAutoActivate = false;
    ProjectileMovement->bInitialVelocityInLocalSpace = false;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bSweepCollision = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->bRotationFollowsVelocity = true;
}

void AETCombatProjectile::BeginPlay()
{
    Super::BeginPlay();
    ProjectileMovement->OnProjectileStop.AddUniqueDynamic(this, &ThisClass::HandleProjectileStopped);
}

bool AETCombatProjectile::TryLaunch(AActor* InSource, const FVector& WorldDirection)
{
    return TryLaunchInternal(InSource, WorldDirection, nullptr, 0.0f, 1.0f);
}

bool AETCombatProjectile::TryLaunchDamage(
    AEmberTrialCharacter* InSource, const FVector& WorldDirection,
    TSubclassOf<UGameplayEffect> InEffectClass, float Damage, float Level)
{
    if (!IsValid(InSource) || !InSource->IsGameplayReady() || InSource->IsCombatDead() ||
        !ETCombatDamage::IsValidConfiguration(InEffectClass, Damage) ||
        !FMath::IsFinite(Level) || Level <= 0.0f)
    {
        return false;
    }
    return TryLaunchInternal(InSource, WorldDirection, InEffectClass, Damage, Level);
}

bool AETCombatProjectile::TryLaunchInternal(
    AActor* InSource, const FVector& WorldDirection, TSubclassOf<UGameplayEffect> InEffectClass,
    float Damage, float Level)
{
    UWorld* World = GetWorld();
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || bLaunched || bResolved ||
        !World || World->IsPaused() || !IsValid(InSource) || InSource == this ||
        InSource->IsActorBeingDestroyed() || InSource->GetWorld() != World ||
        WorldDirection.ContainsNaN() || !FMath::IsFinite(LaunchSpeed) || LaunchSpeed < 1.0f ||
        !FMath::IsFinite(FlightLifetime) || FlightLifetime < 0.01f)
    {
        return false;
    }
    const FVector Direction = WorldDirection.GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return false;
    }

    bLaunched = true;
    SourceActor = InSource;
    FlightDamageEffectClass = InEffectClass;
    FlightDamage = Damage;
    FlightDamageLevel = Level;
    Collision->IgnoreActorWhenMoving(InSource, true);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProjectileMovement->Velocity = Direction * LaunchSpeed;
    SetLifeSpan(FlightLifetime);
    ProjectileMovement->Activate();
    return true;
}

void AETCombatProjectile::HandleProjectileStopped(const FHitResult& Hit)
{
    if (!bLaunched || bResolved || IsActorBeingDestroyed())
    {
        return;
    }
    // 先封住本次飞行，再通知可能同步销毁投射物的命中监听者。
    bResolved = true;
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Hit.bBlockingHit)
    {
        if (FlightDamageEffectClass)
        {
            ApplyDamageOnImpact(Hit);
        }
        if (!IsActorBeingDestroyed())
        {
            OnImpact.Broadcast(this, Hit);
        }
    }
    if (!IsActorBeingDestroyed())
    {
        Destroy();
    }
}

void AETCombatProjectile::ApplyDamageOnImpact(const FHitResult& Hit)
{
    AEmberTrialCharacter* Source = Cast<AEmberTrialCharacter>(GetSourceActor());
    const FString SourceName = GetNameSafe(Source);
    const FString TargetName = GetNameSafe(Hit.GetActor());
    const float Damage = FlightDamage;
    const ETCombatDamage::FResult Result = ETCombatDamage::ApplyHit(
        Source, Source ? Source->GetAbilitySystemComponent() : nullptr,
        Hit, FlightDamageEffectClass, Damage, FlightDamageLevel, this, this);
    if (Result.Rejection)
    {
        UE_LOG(LogETCombatProjectile, Display,
               TEXT("[M4-01B] Damage skipped Source=%s Target=%s Reason=%s"),
               *SourceName, *TargetName, Result.Rejection);
        return;
    }
    UE_LOG(LogETCombatProjectile, Display,
           TEXT("[M4-01B] Damage Source=%s Target=%s Requested=%.1f Applied=%d Snapshot=%d "
                "Before=%.1f After=%.1f BaseAfter=%.1f"),
           *SourceName, *TargetName, Damage, Result.bApplied ? 1 : 0,
           Result.bHasPostHitSnapshot ? 1 : 0, Result.HealthBefore, Result.HealthAfter,
           Result.BaseAfter);
}

void AETCombatProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ProjectileMovement->OnProjectileStop.RemoveDynamic(this, &ThisClass::HandleProjectileStopped);
    OnImpact.Clear();
    Super::EndPlay(EndPlayReason);
}