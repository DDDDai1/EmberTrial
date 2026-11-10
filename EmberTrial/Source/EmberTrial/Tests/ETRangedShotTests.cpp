#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ETRangedShot.h"

#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRangedShotTest, "EmberTrial.M4.Combat.RangedShot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRangedShotTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETRangedShotConfig Config;
    Config.ProjectileClass = AETCombatProjectile::StaticClass();
    Config.DamageEffectClass = LoadClass<UGameplayEffect>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    if (!TestNotNull(TEXT("Existing damage GE"), Config.DamageEffectClass.Get()))
    {
        return false;
    }

    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AEmberTrialCharacter* Source = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator);
    AEmberTrialCharacter* Target = World->SpawnActor<AEmberTrialCharacter>(
        FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Shooter"), Source) || !TestNotNull(TEXT("Target"), Target) ||
        !TestNotNull(TEXT("Pause state"), Pauser))
    {
        return false;
    }
    Source->CombatTeam = EETCombatTeam::Enemy;
    Target->CombatTeam = EETCombatTeam::Player;
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    // 世界启动后锁定测试身体，保持每次扫掠的碰撞位置。
    for (AEmberTrialCharacter* Body : {Source, Target})
    {
        Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        Body->GetCharacterMovement()->DisableMovement();
        Body->GetCharacterMovement()->SetComponentTickEnabled(false);
    }
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const auto MoveTarget = [Target](double X)
    {
        Target->SetActorLocation(FVector(X, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
    };
    const auto Reject = [this, World, Source](const TCHAR* Reason, AEmberTrialCharacter* Body,
                                             const FETRangedShotConfig& ShotConfig)
    {
        TestNull(Reason, ETRangedShot::TryFire(Source, Body, ShotConfig));
        const TActorIterator<AETCombatProjectile> Live(World);
        TestFalse(TEXT("Rejected request leaves no live projectile"), static_cast<bool>(Live));
    };

    Reject(TEXT("Missing target"), nullptr, Config);
    Target->CombatTeam = EETCombatTeam::Enemy;
    Reject(TEXT("Friendly target"), Target, Config);
    Target->CombatTeam = EETCombatTeam::Player;

    FETRangedShotConfig Invalid = Config;
    Invalid.MaxRange = Invalid.MinRange - 1.0f;
    Reject(TEXT("Reversed range"), Target, Invalid);
    Invalid = Config;
    Invalid.ProjectileClass = nullptr;
    Reject(TEXT("Missing projectile class"), Target, Invalid);
    Invalid = Config;
    Invalid.DamageEffectClass = nullptr;
    Reject(TEXT("Missing damage effect"), Target, Invalid);

    MoveTarget(Config.MinRange - 1.0f);
    Reject(TEXT("Inside minimum range"), Target, Config);
    MoveTarget(Config.MaxRange + 1.0f);
    Reject(TEXT("Outside maximum range"), Target, Config);
    MoveTarget(600.0);

    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("Pause fixture active"), World->IsPaused());
    Reject(TEXT("World paused"), Target, Config);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);

    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Sight blocker"), Wall))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->InitBoxExtent(FVector(3.0, 80.0, 80.0));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(300.0, 0.0, 0.0));
    Reject(TEXT("Sight blocked"), Target, Config);
    Wall->Destroy();

    TestEqual(TEXT("Rejected requests leave health unchanged"), TargetASC->GetNumericAttribute(Health), 100.0f);
    AETCombatProjectile* Shot = ETRangedShot::TryFire(Source, Target, Config);
    if (!TestNotNull(TEXT("Clear shot launches"), Shot))
    {
        return false;
    }
    TestTrue(TEXT("Projectile owner and instigator are the shooter"),
             Shot->GetOwner() == Source && Shot->GetInstigator() == Source);
    TestEqual(TEXT("Launch does not damage immediately"), TargetASC->GetNumericAttribute(Health), 100.0f);
    const TWeakObjectPtr<AETCombatProjectile> WeakShot(Shot);
    for (int32 Step = 0; Step < 8; ++Step)
    {
        if (!Fixture.TickTestWorld(0.10f))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
    }
    TestFalse(TEXT("Successful impact consumes projectile"), WeakShot.IsValid());
    TestEqual(TEXT("Actual ranged damage"), TargetASC->GetNumericAttribute(Health), 88.0f);
    TestEqual(TEXT("Actual ranged damage updates Base"), TargetASC->GetNumericAttributeBase(Health), 88.0f);
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif