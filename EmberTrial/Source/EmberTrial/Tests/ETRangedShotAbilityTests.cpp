#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_RangedShot.h"

#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/ETAN_RangedShotRelease.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRangedShotAbilityTest, "EmberTrial.M4.Combat.RangedShotAbility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRangedShotAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UAnimMontage* Template = LoadObject<UAnimMontage>(nullptr,
        TEXT("/Game/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01.AM_ET_BreakPulse_01"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    FETRangedShotConfig Config;
    Config.ProjectileClass = AETCombatProjectile::StaticClass();
    Config.DamageEffectClass = LoadClass<UGameplayEffect>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    if (!TestNotNull(TEXT("Saved montage template"), Template) || Template->Notifies.Num() != 1 ||
        !TestNotNull(TEXT("Quinn"), Quinn) || !TestNotNull(TEXT("Hit reaction"), HitClass) ||
        !TestNotNull(TEXT("Health damage GE"), Config.DamageEffectClass.Get()))
    {
        return false;
    }

    // 复用现有动作，临时副本仅换通知。不会保存或修改正式动画。
    TStrongObjectPtr<UAnimMontage> Montage(DuplicateObject<UAnimMontage>(Template, GetTransientPackage()));
    Montage->Notifies[0].Notify = NewObject<UETAN_RangedShotRelease>(Montage.Get());
    Montage->RefreshCacheData();

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
    const FGameplayAbilitySpecHandle Ranged = ASC->GiveAbility(FGameplayAbilitySpec(UETGA_RangedShot::StaticClass()));
    const FGameplayAbilitySpecHandle Hit = ASC->GiveAbility(FGameplayAbilitySpec(HitClass));
    const FGameplayAbilitySpec* RangedSpec = ASC->FindAbilitySpecFromHandle(Ranged);
    UETGA_RangedShot* RangedAbility =
        RangedSpec ? Cast<UETGA_RangedShot>(RangedSpec->GetPrimaryInstance()) : nullptr;
    if (!TestNotNull(TEXT("Granted ranged ability instance"), RangedAbility))
    {
        return false;
    }
    // 配置本世界的实例，不改CDO或磁盘上的能力资产。
    RangedAbility->CastMontage = Montage.Get();
    RangedAbility->ShotConfig = Config;
    if (!TestTrue(TEXT("Can activate with temporary montage"), RangedAbility->CanActivateAbility(
            Ranged, ASC->AbilityActorInfo.Get())))
    {
        return false;
    }

    const auto Request = [ASC, Body](AActor* Aim)
    {
        FGameplayEventData Event;
        Event.EventTag = ETGameplayTags::Event_RangedShot;
        Event.Instigator = Body;
        Event.Target = Aim;
        return ASC->HandleGameplayEvent(Event.EventTag, &Event);
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
    if (!Advance(1.2f))
    {
        return false;
    }
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
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif