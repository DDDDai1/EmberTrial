#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ETCombatProjectile.h"

#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETCombatProjectileDamageTest,
    "EmberTrial.M4.Combat.ProjectileDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETCombatProjectileDamageTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const TSubclassOf<UGameplayEffect> DamageEffect = LoadClass<UGameplayEffect>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    if (!TestNotNull(TEXT("Existing Instant health damage GE"), DamageEffect.Get()))
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
    const auto SpawnBody = [World](double X, EETCombatTeam Team)
    {
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(X, 0.0, 0.0), FRotator::ZeroRotator);
        if (Body)
        {
            Body->CombatTeam = Team;
            Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        }
        return Body;
    };
    AEmberTrialCharacter* Source = SpawnBody(0.0, EETCombatTeam::Enemy);
    AEmberTrialCharacter* Target = SpawnBody(480.0, EETCombatTeam::Player);
    if (!TestNotNull(TEXT("Enemy source"), Source) || !TestNotNull(TEXT("Player target"), Target) ||
        !Fixture.BeginPlayInTestWorld() ||
        !TestTrue(TEXT("Both ASCs ready"), Source->IsGameplayReady() && Target->IsGameplayReady()))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    // BeginPlay may select the default movement mode. Freeze the collision
    // fixtures after world startup so gravity cannot move them between shots.
    for (AEmberTrialCharacter* Body : {Source, Target})
    {
        UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
        Movement->DisableMovement();
        Movement->SetComponentTickEnabled(false);
    }

    constexpr float Damage = 16.0f;
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const auto CheckHealth = [this, TargetASC, Health](const TCHAR* Label, float Expected)
    {
        TestEqual(Label, TargetASC->GetNumericAttribute(Health), Expected);
        TestEqual(TEXT("Target base health matches Current"),
                  TargetASC->GetNumericAttributeBase(Health), Expected);
    };
    const auto Advance = [&Fixture]()
    {
        for (int32 Step = 0; Step < 6; ++Step)
        {
            if (!Fixture.TickTestWorld(0.10f))
            {
                return false;
            }
        }
        return true;
    };

    // 火力、规则和墙体均走真实扫掠与命中回调。
    const auto Fire = [&](const TCHAR* Label, float Before, float After, AActor* ExpectedHit,
                          bool bInvulnerable = false, bool bDestroySource = false)
    {
        AETCombatProjectile* Shot = World->SpawnActor<AETCombatProjectile>();
        if (!TestNotNull(TEXT("Projectile"), Shot))
        {
            return false;
        }
        TestFalse(TEXT("Missing GE rejected"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, nullptr, Damage));
        TestFalse(TEXT("Wrong GE contract rejected"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, UGameplayEffect::StaticClass(), Damage));
        TestFalse(TEXT("Zero damage rejected"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, DamageEffect, 0.0f));
        if (!TestTrue(TEXT("Launch configured shot"),
                      Shot->TryLaunchDamage(Source, FVector::ForwardVector, DamageEffect, Damage)))
        {
            return false;
        }
        CheckHealth(TEXT("No damage at launch"), Before);
        TestFalse(TEXT("Second request cannot replace damage snapshot"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, DamageEffect, 90.0f));

        int32 Impacts = 0;
        Shot->OnImpact.AddLambda([&](AETCombatProjectile*, const FHitResult& Hit)
        {
            ++Impacts;
            TestTrue(TEXT("Expected actor blocked the shot"), Hit.GetActor() == ExpectedHit);
            CheckHealth(TEXT("Listener sees the post-hit health"), After);
        });
        const TWeakObjectPtr<AETCombatProjectile> WeakShot(Shot);
        if (bInvulnerable)
        {
            TargetASC->AddLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (bDestroySource)
        {
            Source->Destroy();
        }
        const bool bAdvanced = Advance();
        if (bInvulnerable)
        {
            TargetASC->RemoveLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (AETCombatProjectile* LiveShot = WeakShot.Get())
        {
            // 回调使用本次Fire的栈变量，提前失败时也要解除绑定。
            LiveShot->OnImpact.Clear();
        }
        TestEqual(FString::Printf(TEXT("%s: one blocking impact callback"), Label), Impacts, 1);
        TestFalse(TEXT("Blocking hit consumes projectile"), WeakShot.IsValid());
        CheckHealth(Label, After);
        return bAdvanced;
    };

    if (!Fire(TEXT("Enemy shot deals 16 once"), 100.0f, 84.0f, Target) ||
        !Fire(TEXT("Invulnerability added in flight blocks damage"), 84.0f, 84.0f, Target, true))
    {
        return false;
    }
    Target->CombatTeam = EETCombatTeam::Enemy;
    if (!Fire(TEXT("Friendly hit consumes flight without damage"), 84.0f, 84.0f, Target))
    {
        return false;
    }
    Target->CombatTeam = EETCombatTeam::Player;

    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Thin wall"), Wall))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->InitBoxExtent(FVector(3.0, 80.0, 80.0));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(240.0, 0.0, 0.0));
    if (!Fire(TEXT("Wall prevents damage to the body behind it"), 84.0f, 84.0f, Wall))
    {
        return false;
    }
    Wall->Destroy();

    if (!Fire(TEXT("Destroyed source cannot submit damage"), 84.0f, 84.0f, Target, false, true) ||
        !Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif