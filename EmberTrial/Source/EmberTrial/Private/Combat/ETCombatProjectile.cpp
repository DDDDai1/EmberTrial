#include "Combat/ETCombatProjectile.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"

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
    Collision->IgnoreActorWhenMoving(InSource, true);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProjectileMovement->Velocity = Direction * LaunchSpeed;
    ProjectileMovement->Activate();
    SetLifeSpan(FlightLifetime);
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
        OnImpact.Broadcast(this, Hit);
    }
    if (!IsActorBeingDestroyed())
    {
        Destroy();
    }
}

void AETCombatProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ProjectileMovement->OnProjectileStop.RemoveDynamic(this, &ThisClass::HandleProjectileStopped);
    OnImpact.Clear();
    Super::EndPlay(EndPlayReason);
}