#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_RangedShot.h"

#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRangedShotAbilityTest, "EmberTrial.M4.Combat.RangedShotAbility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRangedShotAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* RangedClass = LoadClass<UETGA_RangedShot>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_RangedShot.GA_ET_RangedShot_C"));
    UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr,
        TEXT("/Game/EmberTrial/Animations/Ranged/Montage/AM_ET_RangedShot_01.AM_ET_RangedShot_01"));
    UClass* ProjectileClass = LoadClass<AETCombatProjectile>(nullptr,
        TEXT("/Game/EmberTrial/Combat/Projectiles/BP_ETRangedProjectile.BP_ETRangedProjectile_C"));
    UClass* DamageClass = LoadClass<UGameplayEffect>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    if (!TestNotNull(TEXT("Saved ranged GA"), RangedClass) ||
        !TestNotNull(TEXT("Saved ranged montage"), Montage) ||
        !TestNotNull(TEXT("Saved projectile Blueprint"), ProjectileClass) ||
        !TestNotNull(TEXT("Health damage GE"), DamageClass) ||
        !TestNotNull(TEXT("Quinn"), Quinn) || !TestNotNull(TEXT("Hit reaction"), HitClass))
    {
        return false;
    }

    const UETGA_RangedShot* Defaults = RangedClass->GetDefaultObject<UETGA_RangedShot>();
    if (!TestTrue(TEXT("Saved GA and montage satisfy native configuration"), Defaults->HasValidConfiguration()) ||
        !TestTrue(TEXT("Saved GA references the dedicated montage"), Defaults->CastMontage == Montage) ||
        !TestTrue(TEXT("Saved GA references the projectile Blueprint and damage GE"),
                  Defaults->ShotConfig.ProjectileClass.Get() == ProjectileClass &&
                      Defaults->ShotConfig.DamageEffectClass.Get() == DamageClass) ||
        !TestEqual(TEXT("Release is at 0.40 seconds"), Montage->Notifies[0].GetTime(), .40f, .01f) ||
        !TestEqual(TEXT("Saved damage"), Defaults->ShotConfig.Damage, 12.0f) ||
        !TestEqual(TEXT("Saved minimum range"), Defaults->ShotConfig.MinRange, 250.0f) ||
        !TestEqual(TEXT("Saved maximum range"), Defaults->ShotConfig.MaxRange, 1200.0f))
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
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>();
    AEmberTrialCharacter* Target = World->SpawnActor<AEmberTrialCharacter>(
        FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
    AAIController* Controller = World->SpawnActor<AAIController>();
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Shooter"), Body) || !TestNotNull(TEXT("Target"), Target) ||
        !TestNotNull(TEXT("AI controller"), Controller) || !TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    Body->CombatTeam = EETCombatTeam::Enemy;
    Target->CombatTeam = EETCombatTeam::Player;
    Body->RangedShotAbilityClass = RangedClass;
    Target->RangedShotAbilityClass = RangedClass;
    TestFalse(TEXT("Request before BeginPlay is rejected"), Body->RequestRangedShot(Target));
    Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
    Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Body->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Controller->Possess(Body);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    for (AEmberTrialCharacter* Character : {Body, Target})
    {
        Character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Character->GetCharacterMovement()->SetComponentTickEnabled(false);
    }

    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const FGameplayAbilitySpecHandle Ranged = Body->GetRangedShotSpecHandle();
    if (!TestTrue(TEXT("Enemy BeginPlay granted the saved ranged ability"), Ranged.IsValid()))
    {
        return false;
    }
    TestFalse(TEXT("Player is not granted an enemy ranged ability"), Target->GetRangedShotSpecHandle().IsValid());
    const FGameplayAbilitySpecHandle Hit = ASC->GiveAbility(FGameplayAbilitySpec(HitClass));
    const FGameplayAbilitySpec* RangedSpec = ASC->FindAbilitySpecFromHandle(Ranged);
    UETGA_RangedShot* RangedAbility =
        RangedSpec ? Cast<UETGA_RangedShot>(RangedSpec->GetPrimaryInstance()) : nullptr;
    if (!TestNotNull(TEXT("Granted ranged ability instance"), RangedAbility))
    {
        return false;
    }
    if (!TestTrue(TEXT("Granted instance inherited saved asset configuration"),
                  RangedAbility->HasValidConfiguration() &&
                      RangedAbility->CanActivateAbility(Ranged, ASC->AbilityActorInfo.Get())))
    {
        return false;
    }

    const auto Request = [Body](AEmberTrialCharacter* Aim)
    {
        return Body->RequestRangedShot(Aim) ? 1 : 0;
    };
    const auto Advance = [&Fixture](float Seconds)
    {
        for (int32 Index = 0; Index < FMath::CeilToInt(Seconds / .01f); ++Index)
        {
            if (!Fixture.TickTestWorld(.01f))
            {
                return false;
            }
        }
        return true;
    };
    const auto IsCasting = [ASC, Ranged]()
    {
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Ranged);
        return Spec && Spec->IsActive();
    };
    const auto IsCooling = [ASC]()
    {
        return ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_RangedShot);
    };

    TestEqual(TEXT("Missing target event rejects"), Request(nullptr), 0);
    TestFalse(TEXT("Rejected request has no cooldown"), IsCooling());
    if (!TestEqual(TEXT("Accept target event"), Request(Target), 1) ||
        !TestTrue(TEXT("Attacking tag at windup"),
                  IsCasting() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking)) ||
        !Advance(.12f))
    {
        return false;
    }
    TestTrue(TEXT("Real cooldown starts on commit"), IsCooling());
    TestEqual(TEXT("Duplicate event rejects"), Request(Target), 0);
    TestEqual(TEXT("No damage before release"), TargetASC->GetNumericAttribute(Health), 100.0f);
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("Pause fixture active"), World->IsPaused());
    const bool bPausedTick = Advance(.6f);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    if (!bPausedTick)
    {
        return false;
    }
    TestEqual(TEXT("Pause did not release"), TargetASC->GetNumericAttribute(Health), 100.0f);
    if (!Advance(.34f))
    {
        return false;
    }
    TActorIterator<AETCombatProjectile> InFlight(World);
    AETCombatProjectile* Shot = InFlight ? *InFlight : nullptr;
    const UStaticMeshComponent* Visual = IsValid(Shot) ? Shot->FindComponentByClass<UStaticMeshComponent>() : nullptr;
    if (!TestTrue(TEXT("Release spawned the saved projectile Blueprint"),
                  IsValid(Shot) && Shot->GetClass() == ProjectileClass) ||
        !TestTrue(TEXT("Projectile visual exists and has collision disabled"),
                  IsValid(Visual) && IsValid(Visual->GetStaticMesh()) && Visual->IsVisible() &&
                      Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision))
    {
        return false;
    }
    const TWeakObjectPtr<AETCombatProjectile> WeakShot(Shot);
    TestEqual(TEXT("Projectile is in flight before damage"), TargetASC->GetNumericAttribute(Health), 100.0f);
    if (!Advance(.86f))
    {
        return false;
    }
    TestFalse(TEXT("Projectile destroyed after blocking hit"), WeakShot.IsValid());
    TestEqual(TEXT("One release hit after resume"), TargetASC->GetNumericAttribute(Health), 88.0f);
    TestEqual(TEXT("Health base after one release"), TargetASC->GetNumericAttributeBase(Health), 88.0f);
    TestFalse(TEXT("Montage and attacking tag ended"),
              IsCasting() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
    TestTrue(TEXT("Cooldown outlives montage"), IsCooling());
    TestEqual(TEXT("Cooldown rejects after montage"), Request(Target), 0);
    if (!Advance(2.5f))
    {
        return false;
    }
    TestFalse(TEXT("Cooldown expires"), IsCooling());

    if (!TestEqual(TEXT("Next cast accepted"), Request(Target), 1) || !Advance(.12f) ||
        !TestTrue(TEXT("Actual HitReact interrupts ranged windup"), ASC->TryActivateAbility(Hit, false)))
    {
        return false;
    }
    TestFalse(TEXT("Interrupted cast releases attacking tag"),
              IsCasting() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
    TestTrue(TEXT("Interrupted windup keeps cooldown"), IsCooling());
    if (!Advance(1.0f))
    {
        return false;
    }
    TestEqual(TEXT("Interrupted windup never fires"), TargetASC->GetNumericAttribute(Health), 88.0f);
    if (!Advance(2.5f) || !TestEqual(TEXT("Third windup accepted"), Request(Target), 1))
    {
        return false;
    }
    Target->SetActorLocation(FVector(1600.0, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
    if (!Advance(1.2f))
    {
        return false;
    }
    TestEqual(TEXT("Target out of range by release takes no damage"),
              TargetASC->GetNumericAttribute(Health), 88.0f);
    TestTrue(TEXT("Whiff keeps cooldown"), IsCooling());

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay invalidates the body-owned ranged handle"), Body->GetRangedShotSpecHandle().IsValid());
    TestNull(TEXT("EndPlay removes the ranged spec from the ASC"), ASC->FindAbilitySpecFromHandle(Ranged));
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif