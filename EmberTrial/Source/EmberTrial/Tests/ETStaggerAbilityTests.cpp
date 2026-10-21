#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatPoiseComponent.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETStaggerAbilityTest, "EmberTrial.M2.Combat.StaggerAbility",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETStaggerAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* StaggerClass = LoadClass<UETGA_Stagger>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger.GA_ET_Stagger_C"));
    UClass* AttackClass = LoadClass<UETGA_LightAttack>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass = LoadClass<UETGA_Dodge>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    UClass* HealClass =
        LoadClass<UETGA_Heal>(nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal.GA_ET_Heal_C"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved Stagger GA"), StaggerClass) ||
        !TestNotNull(TEXT("Saved Attack GA"), AttackClass) ||
        !TestNotNull(TEXT("Saved Dodge GA"), DodgeClass) ||
        !TestNotNull(TEXT("Saved Heal GA"), HealClass) ||
        !TestNotNull(TEXT("Saved Hit GA"), HitClass) || !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_Stagger* Defaults = StaggerClass->GetDefaultObject<UETGA_Stagger>();
    if (!TestTrue(TEXT("Saved reaction policy"), Defaults->HasValidConfiguration()) ||
        !TestEqual(TEXT("Saved duration contract"), Defaults->StaggerDuration, 1.2f))
    {
        return false;
    }
    enum class EScenario : uint8
    {
        GatesAndPause,
        Attack,
        Dodge,
        Heal,
        Hit,
        Death,
        EndPlay
    };
    for (const EScenario Scenario :
         {EScenario::GatesAndPause, EScenario::Attack, EScenario::Dodge, EScenario::Heal,
          EScenario::Hit, EScenario::Death, EScenario::EndPlay})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = Fixture.GetTestWorld();
        AActor* Floor = World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Floor"), Floor))
        {
            return false;
        }
        UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(Box);
        Floor->SetRootComponent(Box);
        Box->SetMobility(EComponentMobility::Static);
        Box->SetBoxExtent(FVector(2000, 2000, 50));
        Box->SetRelativeLocation(FVector(0, 0, -50));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Controller"), PC))
        {
            return false;
        }
        Body->CombatTeam = EETCombatTeam::Player;
        Body->InitialHealth = 40.0f;
        Body->LightAttackAbilityClass = AttackClass;
        Body->DodgeAbilityClass = DodgeClass;
        Body->HealAbilityClass = HealClass;
        UETCombatPoiseComponent* Poise = Body->FindComponentByClass<UETCombatPoiseComponent>();
        if (!TestNotNull(TEXT("Reaction component"), Poise))
        {
            return false;
        }
        Poise->StaggerAbilityClass = StaggerClass;
        Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Body->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->GetCharacterMovement()->bRunPhysicsWithNoController = true;
        PC->Possess(Body);
        TestFalse(TEXT("Before BeginPlay rejects reaction"), Body->RequestStagger());
        if (!Fixture.BeginPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        auto Advance = [&Fixture](float Seconds)
        {
            for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
            {
                if (!Fixture.TickTestWorld(.01f))
                {
                    return false;
                }
            }
            return true;
        };
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) ||
            !TestTrue(TEXT("Grounded"), Body->GetCharacterMovement()->IsMovingOnGround()))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        const FGameplayAbilitySpecHandle ReactionHandle = Poise->StaggerHandle;
        FGameplayAbilitySpec* ReactionSpec = ASC->FindAbilitySpecFromHandle(ReactionHandle);
        UETGA_Stagger* Reaction =
            ReactionSpec ? Cast<UETGA_Stagger>(ReactionSpec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Reaction instance"), Reaction))
        {
            return false;
        }
        // 直接授予真实受击GA；不修改CombatLife的私有配置或旧测试。
        const FGameplayAbilitySpecHandle HitHandle =
            ASC->GiveAbility(FGameplayAbilitySpec(HitClass, 1));
        if (Scenario == EScenario::Attack)
        {
            TestTrue(TEXT("Attack starts"), Body->RequestLightAttack());
            if (!Advance(.1f))
            {
                return false;
            }
        }
        else if (Scenario == EScenario::Dodge)
        {
            TestTrue(TEXT("Dodge starts"), Body->RequestDodge());
            if (!Advance(.05f))
            {
                return false;
            }
        }
        else if (Scenario == EScenario::Heal)
        {
            TestTrue(TEXT("Heal starts"), Body->RequestHeal());
            if (!Advance(.1f))
            {
                return false;
            }
        }
        else if (Scenario == EScenario::Hit)
        {
            TestTrue(TEXT("Hit starts"), ASC->TryActivateAbility(HitHandle));
            if (!Advance(.05f))
            {
                return false;
            }
        }
        TestTrue(TEXT("Reaction accepted"), Body->RequestStagger());
        TestEqual(TEXT("One owned Stunned tag"), ASC->GetTagCount(ETGameplayTags::State_Stunned),
                  1);
        TestFalse(TEXT("Reaction does not restart itself"), Body->RequestStagger());
        TestFalse(TEXT("Formal actions blocked"), Body->IsFormalActionIdle());
        TestFalse(TEXT("Attack blocked"), Body->RequestLightAttack());
        TestFalse(TEXT("Dodge blocked"), Body->RequestDodge());
        TestFalse(TEXT("Heal blocked"), Body->RequestHeal());
        TestFalse(TEXT("Hit blocked"), ASC->TryActivateAbility(HitHandle));
        TestFalse(TEXT("Old attack cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
        TestFalse(TEXT("Old dodge cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        TestFalse(TEXT("Old heal cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing));
        TestFalse(TEXT("Old hit cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact));
        const uint64 OldSerial = Reaction->ActivationSerial;
        const FTimerHandle TimerAtStart = Reaction->DurationTimer;

        if (Scenario == EScenario::GatesAndPause)
        {
            UGameplayEffect* Damage = NewObject<UGameplayEffect>(GetTransientPackage());
            Damage->DurationPolicy = EGameplayEffectDurationType::Instant;
            FGameplayModifierInfo& Mod = Damage->Modifiers.AddDefaulted_GetRef();
            Mod.Attribute = UETHealthAttributeSet::GetHealthAttribute();
            Mod.ModifierOp = EGameplayModOp::AddBase;
            Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.0f));
            ASC->ApplyGameplayEffectSpecToSelf(
                FGameplayEffectSpec(Damage, ASC->MakeEffectContext(), 1.0f));
            TestEqual(TEXT("Damage still applies while Stunned"),
                      ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                      30.0f);
            TestTrue(TEXT("Damage does not restart reaction"),
                     Reaction->ActivationSerial == OldSerial);
            ASC->SetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute(), 50.0f);
            if (!Advance(.3f))
            {
                return false;
            }
            TestEqual(TEXT("Recovery suspended by Stunned"),
                      ASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
                      50.0f);
            APlayerState* Pauser = World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Pauser"), Pauser))
            {
                return false;
            }
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            const double PausedAt = World->GetTimeSeconds();
            if (!Advance(1.0f))
            {
                return false;
            }
            TestTrue(
                TEXT("Pause keeps game time"),
                FMath::IsNearlyEqual(static_cast<double>(World->GetTimeSeconds()), PausedAt, .001));
            TestTrue(TEXT("Pause keeps reaction"),
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            TestFalse(TEXT("Pause refuses another request"), Body->RequestStagger());
            World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            // 按实际条件等到结束，保留计时余量，不断言恰好落在某个Timer边界。
            for (int32 I = 0; I < 150 && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned);
                 ++I)
            {
                if (!Advance(.01f))
                {
                    return false;
                }
            }
            TestFalse(TEXT("Natural duration expires"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            TestTrue(TEXT("No early expiry"), World->GetTimeSeconds() - PausedAt >= .88);
            TestTrue(TEXT("Actions restored"), Body->IsFormalActionIdle());
            TestTrue(TEXT("A later reaction starts"), Body->RequestStagger());
            Reaction->HandleDurationFinished(OldSerial);
            TestTrue(TEXT("Old callback cannot end new reaction"),
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            const FTimerHandle NewTimer = Reaction->DurationTimer;
            ASC->AddLooseGameplayTag(ETGameplayTags::State_Stunned);
            ASC->CancelAbilityHandle(ReactionHandle);
            TestEqual(TEXT("Only own tag count removed"),
                      ASC->GetTagCount(ETGameplayTags::State_Stunned), 1);
            TestFalse(TEXT("Cancel clears own duration timer"),
                      World->GetTimerManager().TimerExists(NewTimer));
            ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Stunned);
        }
        else if (Scenario == EScenario::Dodge)
        {
            const FVector StoppedAt = Body->GetActorLocation();
            if (!Advance(.35f))
            {
                return false;
            }
            TestTrue(TEXT("Cancelled dodge leaves no horizontal drift"),
                     FVector::Dist2D(StoppedAt, Body->GetActorLocation()) < .1f);
            TestEqual(TEXT("Dodge does not refund cost"),
                      ASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
                      75.0f);
            TestFalse(TEXT("Dodge invulnerability cleared"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        }
        else if (Scenario == EScenario::Heal)
        {
            if (!Advance(.65f))
            {
                return false;
            }
            int32 Charges = 0;
            int32 Maximum = 0;
            UETHealingResourceComponent* Resource =
                Body->FindComponentByClass<UETHealingResourceComponent>();
            TestTrue(TEXT("Healing resource available"),
                     Resource && Resource->GetHealingSnapshot(Charges, Maximum));
            TestEqual(TEXT("Before-commit cancel keeps charges"), Charges, 3);
            TestEqual(TEXT("Before-commit cancel keeps health"),
                      ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                      40.0f);
        }
        else if (Scenario == EScenario::Attack)
        {
            TestFalse(
                TEXT("Cancelled attack trace is off"),
                Body->FindComponentByClass<UETCombatTraceComponent>()->IsComponentTickEnabled());
        }
        else if (Scenario == EScenario::Death)
        {
            ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
            TestTrue(TEXT("Death wins"), Body->IsCombatDead());
            TestFalse(TEXT("Death removes Stunned"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            if (!Advance(1.5f))
            {
                return false;
            }
            TestTrue(TEXT("Death movement remains sealed"),
                     Body->GetCharacterMovement()->MovementMode == MOVE_None);
            TestFalse(TEXT("Dead body refuses reaction"), Body->RequestStagger());
        }
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("EndPlay releases reaction tag"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
        TestFalse(TEXT("EndPlay clears duration timer"),
                  World->GetTimerManager().TimerExists(TimerAtStart));
        TestTrue(TEXT("EndPlay removes owned Spec"),
                 ASC->FindAbilitySpecFromHandle(ReactionHandle) == nullptr);
        TestFalse(TEXT("EndPlay refuses requests"), Body->RequestStagger());
        Fixture.ForwardErrorMessages(this);
    }
    return !HasAnyErrors();
}
#endif