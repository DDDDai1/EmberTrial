#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETBreakPulseAbilityTest, "EmberTrial.M2.Combat.BreakPulseAbility",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETBreakPulseAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    // 使用正式已保存配置；不能用测试自制GA掩盖实际资产的Tag或Montage漏配。
    UClass* BreakClass = LoadClass<UETGA_BreakPulse>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse.GA_ET_BreakPulse_C"));
    UClass* AttackClass = LoadClass<UETGA_LightAttack>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass = LoadClass<UETGA_Dodge>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    UClass* HealClass =
        LoadClass<UETGA_Heal>(nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal.GA_ET_Heal_C"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    UClass* StaggerClass = LoadClass<UETGA_Stagger>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger.GA_ET_Stagger_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved BreakPulse"), BreakClass) ||
        !TestNotNull(TEXT("Saved Attack"), AttackClass) ||
        !TestNotNull(TEXT("Saved Dodge"), DodgeClass) ||
        !TestNotNull(TEXT("Saved Heal"), HealClass) || !TestNotNull(TEXT("Saved Hit"), HitClass) ||
        !TestNotNull(TEXT("Saved Stagger"), StaggerClass) || !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_BreakPulse* Defaults = BreakClass->GetDefaultObject<UETGA_BreakPulse>();
    if (!TestTrue(TEXT("Saved cast configuration"), Defaults->HasValidConfiguration()) ||
        !TestEqual(TEXT("Default cooldown six seconds"), Defaults->CooldownSeconds, 6.0f))
    {
        return false;
    }
    enum class EScenario : uint8
    {
        Normal,
        Invalid,
        Gates,
        Pause,
        Hit,
        Stagger,
        Death,
        EndPlay
    };
    for (const EScenario Scenario :
         {EScenario::Normal, EScenario::Invalid, EScenario::Gates, EScenario::Pause, EScenario::Hit,
          EScenario::Stagger, EScenario::Death, EScenario::EndPlay})
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
        Floor->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(2000, 2000, 20));
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
        Body->BreakPulseAbilityClass = BreakClass;
        Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Body->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->GetCharacterMovement()->bRunPhysicsWithNoController = true;
        PC->Possess(Body);
        TestFalse(TEXT("Not ready rejects"), Body->RequestBreakPulse());
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
        FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Body->BreakPulseAbilityHandle);
        UETGA_BreakPulse* Ability =
            Spec ? Cast<UETGA_BreakPulse>(Spec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Instanced cast"), Ability))
        {
            return false;
        }
        auto Remaining = [Ability, ASC, Body]()
        {
            float Left = 0.0f;
            float Duration = 0.0f;
            Ability->GetCooldownTimeRemainingAndDuration(
                Body->BreakPulseAbilityHandle, ASC->AbilityActorInfo.Get(), Left, Duration);
            return Left;
        };
        auto HasCooldown = [ASC]()
        { return ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse); };
        if (Scenario == EScenario::Invalid)
        {
            UAnimMontage* Saved = Ability->CastMontage;
            Ability->CastMontage = nullptr;
            TestFalse(TEXT("None montage refuses"), Body->RequestBreakPulse());
            TestFalse(TEXT("None montage does not spend cooldown"), HasCooldown());
            Ability->CastMontage = Saved;
            Ability->CooldownSeconds = 0.0f;
            TestFalse(TEXT("Zero cooldown refuses"), Body->RequestBreakPulse());
            TestFalse(TEXT("Invalid config does not commit"), HasCooldown());
            Ability->CooldownSeconds = 6.0f;
        }
        else if (Scenario == EScenario::Gates)
        {
            Body->GetCharacterMovement()->Velocity = FVector(100, 0, 0);
            TestFalse(TEXT("Moving rejects"), Body->RequestBreakPulse());
            Body->GetCharacterMovement()->StopMovementImmediately();
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
            TestFalse(TEXT("Air rejects"), Body->RequestBreakPulse());
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            TestTrue(TEXT("Attack begins"), Body->RequestLightAttack());
            TestFalse(TEXT("Busy rejects cast"), Body->RequestBreakPulse());
            TestFalse(TEXT("All preflight refusals leave no cooldown"), HasCooldown());
        }
        else
        {
            if (Scenario == EScenario::Pause)
            {
                APlayerState* Pauser = PC->PlayerState;
                if (!IsValid(Pauser))
                {
                    Pauser = World->SpawnActor<APlayerState>();
                }
                if (!TestNotNull(TEXT("Pauser"), Pauser))
                {
                    return false;
                }
                World->GetWorldSettings()->SetPauserPlayerState(Pauser);
                TestFalse(TEXT("Paused input refuses"), Body->RequestBreakPulse());
                TestFalse(TEXT("Paused refusal leaves no cooldown"), HasCooldown());
                World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            }
            if (!TestTrue(TEXT("Cast begins"), Body->RequestBreakPulse()))
            {
                return false;
            }
            TestTrue(TEXT("Cast state active"), Ability->IsActive());
            TestEqual(TEXT("One casting tag"),
                      ASC->GetTagCount(ETGameplayTags::State_CastingBreakPulse), 1);
            TestEqual(TEXT("One cooldown tag"),
                      ASC->GetTagCount(ETGameplayTags::Cooldown_BreakPulse), 1);
            TestTrue(TEXT("Real duration GE starts near six"),
                     FMath::IsNearlyEqual(Remaining(), 6.0f, .05f));
            const uint32 Serial = Ability->ActivationSerial;
            TestFalse(TEXT("Busy repeated request refuses"), Body->RequestBreakPulse());
            TestTrue(TEXT("Repeat does not restart"), Ability->ActivationSerial == Serial);
            TestFalse(TEXT("Cast blocks attack"), Body->RequestLightAttack());
            TestFalse(TEXT("Cast blocks dodge"), Body->RequestDodge());
            TestFalse(TEXT("Cast blocks healing"), Body->RequestHeal());
            TestFalse(TEXT("Cast blocks ordinary actions"), Body->IsFormalActionIdle());
            if (!Advance(.15f))
            {
                return false;
            }
            if (Scenario == EScenario::Pause)
            {
                APlayerState* Pauser = PC->PlayerState;
                if (!IsValid(Pauser))
                {
                    Pauser = World->SpawnActor<APlayerState>();
                }
                if (!TestNotNull(TEXT("Pauser for active cast"), Pauser))
                {
                    return false;
                }
                const float BeforePause = Remaining();
                World->GetWorldSettings()->SetPauserPlayerState(Pauser);
                if (!Advance(1.5f))
                {
                    return false;
                }
                TestTrue(TEXT("Pause freezes cast montage"), Ability->IsActive());
                TestTrue(TEXT("Pause freezes real cooldown"),
                         FMath::IsNearlyEqual(Remaining(), BeforePause, .001f));
                World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            }
            if (Scenario == EScenario::Hit || Scenario == EScenario::Stagger)
            {
                const FGameplayAbilitySpecHandle Interrupt = ASC->GiveAbility(
                    FGameplayAbilitySpec(Scenario == EScenario::Hit ? HitClass : StaggerClass, 1));
                TestTrue(TEXT("Higher priority action accepted"),
                         ASC->TryActivateAbility(Interrupt));
                TestFalse(TEXT("Higher priority action cancels cast"), Ability->IsActive());
                TestFalse(TEXT("Casting tag removed on interrupt"),
                          ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse));
                TestTrue(TEXT("Interrupt does not refund cooldown"), HasCooldown());
            }
            else if (Scenario == EScenario::Death)
            {
                ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
                TestTrue(TEXT("Body really dead"), Body->IsCombatDead());
                TestFalse(TEXT("Death cancels cast"), Ability->IsActive());
                TestFalse(TEXT("Dead input refuses"), Body->RequestBreakPulse());
                TestTrue(TEXT("Death does not refund old body cooldown"), HasCooldown());
                AEmberTrialCharacter* NewBody = World->SpawnActor<AEmberTrialCharacter>(
                    FVector(400, 0, 200), FRotator::ZeroRotator, Spawn);
                if (!TestNotNull(TEXT("New body"), NewBody))
                {
                    return false;
                }
                TestFalse(TEXT("Cooldown belongs only to old ASC"),
                          NewBody->GetAbilitySystemComponent()->HasMatchingGameplayTag(
                              ETGameplayTags::Cooldown_BreakPulse));
            }
            if (Scenario != EScenario::Death && Scenario != EScenario::EndPlay)
            {
                if (!Advance(1.6f))
                {
                    return false;
                }
                TestFalse(TEXT("Cast finishes and clears state"), Ability->IsActive());
                TestTrue(TEXT("Actions unlock before cooldown expires"),
                         Body->IsFormalActionIdle());
                TestTrue(TEXT("Cooldown survives animation end or cancel"), HasCooldown());
                TestFalse(TEXT("Cooling rejects new request"), Body->RequestBreakPulse());
                TestEqual(TEXT("Only one cooldown after repeated requests"),
                          ASC->GetTagCount(ETGameplayTags::Cooldown_BreakPulse), 1);
                // 给GE到期Tick留余量，不在严格6秒边界断言Timer / GE先后顺序。
                if (!Advance(5.0f))
                {
                    return false;
                }
                TestFalse(TEXT("Cooldown naturally expires in game time"), HasCooldown());
                TestTrue(TEXT("After expiry a fresh press is accepted"), Body->RequestBreakPulse());
            }
            if (Scenario != EScenario::Death)
            {
                TestEqual(TEXT("Cast does not damage health"),
                          ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                          40.0f);
                TestEqual(
                    TEXT("Cast does not spend stamina"),
                    ASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
                    100.0f);
                TestEqual(TEXT("C2-01 has no Poise resolution"),
                          ASC->GetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute()),
                          100.0f);
                int32 Charges = 0;
                int32 Maximum = 0;
                UETHealingResourceComponent* Resource =
                    Body->FindComponentByClass<UETHealingResourceComponent>();
                TestTrue(TEXT("Healing resource remains available"),
                         Resource && Resource->GetHealingSnapshot(Charges, Maximum));
                TestEqual(TEXT("No healing charge cost"), Charges, 3);
            }
        }
        // 保留World EndPlay后的对象作只读断言；没有Destroy后推进再访问旧ASC。
        const FGameplayAbilitySpecHandle OwnedHandle = Body->BreakPulseAbilityHandle;
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("EndPlay removes casting tag"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse));
        TestNull(TEXT("EndPlay removes owned spec"), ASC->FindAbilitySpecFromHandle(OwnedHandle));
        TestFalse(TEXT("Ended body cannot start again"), Body->RequestBreakPulse());
    }
    return !HasAnyErrors();
}
#endif