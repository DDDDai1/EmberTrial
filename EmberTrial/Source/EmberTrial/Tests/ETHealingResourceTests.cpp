#if WITH_DEV_AUTOMATION_TESTS

#include "Resources/ETHealingResourceComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETHealingResourceTest,
    "EmberTrial.M2.Healing.Resource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETHealingResourceTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto MakeBody = [&](EETCombatTeam Team, float Health)
    {
        AEmberTrialCharacter* NewBody = World->SpawnActor<AEmberTrialCharacter>(
            FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        if (NewBody)
        {
            NewBody->CombatTeam = Team;
            NewBody->InitialHealth = Health;
        }
        return NewBody;
    };
    AEmberTrialCharacter* Body = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Empty = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Invalid = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Enemy = MakeBody(EETCombatTeam::Enemy, 20.0f);
    AEmberTrialCharacter* Dead = MakeBody(EETCombatTeam::Player, 0.0f);
    AEmberTrialCharacter* Reentrant = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Deactivated = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Fresh = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Bodies[] = {Body, Empty, Invalid, Enemy, Dead, Reentrant, Deactivated, Fresh};
    for (AEmberTrialCharacter* Subject : Bodies)
    {
        if (!TestNotNull(TEXT("Spawned body"), Subject) ||
            !TestNotNull(TEXT("Native healing component"), Subject->HealingResourceComponent.Get()))
        {
            return false;
        }
    }
    UETHealingResourceComponent* Healing = Body->HealingResourceComponent.Get();
    Empty->HealingResourceComponent->InitialCharges = 0;
    Invalid->HealingResourceComponent->InitialCharges = 4;
    int32 Charges = -1;
    int32 MaxCharges = -1;
    TestFalse(TEXT("Before BeginPlay unavailable"), Healing->GetHealingSnapshot(Charges, MaxCharges));
    AddExpectedMessagePlain(TEXT("Invalid healing settings"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 1);
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Main ASC"), ASC) ||
        !TestTrue(TEXT("Main body Ready"), Body->IsGameplayReady()))
    {
        return false;
    }
    const FGameplayAttribute HealthAttribute = UETHealthAttributeSet::GetHealthAttribute();
    auto ExpectCharges = [&](UETHealingResourceComponent* Resource, const TCHAR* Label,
        bool bExpectedAvailable, int32 ExpectedCharges, int32 ExpectedMax = 3)
    {
        int32 Current = -1;
        int32 Maximum = -1;
        TestEqual(FString::Printf(TEXT("%s available"), Label),
            Resource->GetHealingSnapshot(Current, Maximum), bExpectedAvailable);
        TestEqual(FString::Printf(TEXT("%s charges"), Label), Current, bExpectedAvailable ? ExpectedCharges : 0);
        TestEqual(FString::Printf(TEXT("%s maximum"), Label), Maximum, bExpectedAvailable ? ExpectedMax : 0);
    };
    auto ExpectHealth = [&](const TCHAR* Label, float Expected)
    {
        TestEqual(FString::Printf(TEXT("%s Current"), Label), ASC->GetNumericAttribute(HealthAttribute), Expected);
        TestEqual(FString::Printf(TEXT("%s Base"), Label), ASC->GetNumericAttributeBase(HealthAttribute), Expected);
    };
    ExpectCharges(Healing, TEXT("Initial"), true, 3);
    ExpectCharges(Empty->HealingResourceComponent.Get(), TEXT("Zero initial valid"), true, 0);
    ExpectCharges(Invalid->HealingResourceComponent.Get(), TEXT("Invalid settings"), false, 0);
    TestTrue(TEXT("Healing config does not revoke shared Health/Stamina Ready"), Invalid->IsGameplayReady());
    ExpectCharges(Enemy->HealingResourceComponent.Get(), TEXT("Enemy dormant"), false, 0);
    TestFalse(TEXT("Empty rejects"), Empty->HealingResourceComponent->TryApplyHealing(40.0f));
    TestFalse(TEXT("Dead rejects"), Dead->HealingResourceComponent->TryApplyHealing(40.0f));
    TestFalse(TEXT("Enemy rejects"), Enemy->HealingResourceComponent->TryApplyHealing(40.0f));

    int32 ChargeEvents = 0;
    const FDelegateHandle CountHandle = Healing->OnSnapshotChanged.AddLambda([&]() { ++ChargeEvents; });
    for (const float Amount : {0.0f, -1.0f, 1001.0f, std::numeric_limits<float>::quiet_NaN()})
    {
        TestFalse(TEXT("Invalid amount rejects"), Healing->TryApplyHealing(Amount));
    }
    TestEqual(TEXT("Rejects do not publish charge changes"), ChargeEvents, 0);
    ASC->GameplayEffectApplicationQueries.Add(FGameplayEffectApplicationQuery::CreateLambda(
        [](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&) { return false; }));
    TestFalse(TEXT("ASC rejected GE does not spend"), Healing->TryApplyHealing(40.0f));
    ASC->GameplayEffectApplicationQueries.Reset();
    ExpectCharges(Healing, TEXT("Rejected GE preserves charges"), true, 3);
    ExpectHealth(TEXT("Rejected GE preserves Health"), 20.0f);
    TestEqual(TEXT("Rejected GE no notification"), ChargeEvents, 0);

    TestTrue(TEXT("Native Instant heal succeeds"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("20 plus 40"), 60.0f);
    ExpectCharges(Healing, TEXT("One success one charge"), true, 2);
    TestEqual(TEXT("One change notification"), ChargeEvents, 1);
    TestTrue(TEXT("Repeat Ensure succeeds"), Body->EnsureGameplayInitialized());
    Healing->Deactivate();
    ExpectCharges(Healing, TEXT("Deactivated unavailable"), false, 0);
    Healing->Activate(true);
    ExpectCharges(Healing, TEXT("Reactivation preserves count"), true, 2);

    AAIController* Controller = World->SpawnActor<AAIController>();
    if (!TestNotNull(TEXT("Temporary controller"), Controller)) { return false; }
    Controller->Possess(Body);
    Controller->UnPossess();
    Controller->Possess(Body);
    ExpectCharges(Healing, TEXT("Same body Possess preserves count"), true, 2);
    Healing->OnSnapshotChanged.Remove(CountHandle);

    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pauser"), Pauser)) { return false; }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestFalse(TEXT("Paused rejects mutation"), Healing->TryApplyHealing(40.0f));
    ExpectCharges(Healing, TEXT("Paused query works"), true, 2);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    TestTrue(TEXT("Second success reaches full"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("Full"), 100.0f);
    TestFalse(TEXT("Full Health rejects without spending"), Healing->TryApplyHealing(40.0f));
    ExpectCharges(Healing, TEXT("Full rejection"), true, 1);
    ASC->SetNumericAttributeBase(HealthAttribute, 80.0f); // Test setup, not the healing implementation.
    TestTrue(TEXT("Partial effective heal still consumes one"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("Overheal clamps Base and Current"), 100.0f);
    ExpectCharges(Healing, TEXT("Exhausted"), true, 0);
    ASC->SetNumericAttributeBase(HealthAttribute, 20.0f);
    TestFalse(TEXT("Exhausted missing Health rejects"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("No charge no heal"), 20.0f);
    ExpectCharges(Fresh->HealingResourceComponent.Get(), TEXT("Another body independent"), true, 3);

    UETHealingResourceComponent* NestedHealing = Reentrant->HealingResourceComponent.Get();
    UAbilitySystemComponent* NestedASC = Reentrant->GetAbilitySystemComponent();
    int32 RecursiveAttempts = 0;
    bool bRecursiveAccepted = false;
    const FDelegateHandle HealthHandle = NestedASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute)
        .AddLambda([&](const FOnAttributeChangeData& Change)
        {
            if (Change.NewValue > Change.OldValue)
            {
                ++RecursiveAttempts;
                bRecursiveAccepted |= NestedHealing->TryApplyHealing(40.0f);
            }
        });
    const FDelegateHandle SnapshotHandle = NestedHealing->OnSnapshotChanged.AddLambda([&]()
    {
        ++RecursiveAttempts;
        bRecursiveAccepted |= NestedHealing->TryApplyHealing(40.0f);
    });
    TestTrue(TEXT("Outer healing succeeds"), NestedHealing->TryApplyHealing(40.0f));
    TestEqual(TEXT("Health and charge callbacks each attempted once"), RecursiveAttempts, 2);
    TestFalse(TEXT("Recursive heal rejected in both callbacks"), bRecursiveAccepted);
    TestEqual(TEXT("Recursive callbacks do not double heal"), NestedASC->GetNumericAttribute(HealthAttribute), 60.0f);
    ExpectCharges(NestedHealing, TEXT("No recursive double spend"), true, 2);
    NestedASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).Remove(HealthHandle);
    NestedHealing->OnSnapshotChanged.Remove(SnapshotHandle);

    UETHealingResourceComponent* DisabledHealing = Deactivated->HealingResourceComponent.Get();
    UAbilitySystemComponent* DisabledASC = Deactivated->GetAbilitySystemComponent();
    const FDelegateHandle DisableHandle = DisabledASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute)
        .AddLambda([&](const FOnAttributeChangeData& Change)
        {
            if (Change.NewValue > Change.OldValue) { DisabledHealing->Deactivate(); }
        });
    TestTrue(TEXT("Heal remains successful if callback deactivates resource"), DisabledHealing->TryApplyHealing(40.0f));
    DisabledASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).Remove(DisableHandle);
    ExpectCharges(DisabledHealing, TEXT("Disabled query unavailable"), false, 0);
    DisabledHealing->Activate();
    ExpectCharges(DisabledHealing, TEXT("Successful heal was not refunded"), true, 2);

    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    ExpectCharges(Healing, TEXT("EndPlay unavailable"), false, 0);
    TestFalse(TEXT("EndPlay cannot heal"), Healing->TryApplyHealing(40.0f));
    TestFalse(TEXT("EndPlay ready binding removed"),
        Body->OnGameplayReadyChanged.IsAlreadyBound(Healing,
            &UETHealingResourceComponent::HandleGameplayReadyChanged));
    TestFalse(TEXT("EndPlay count listeners cleared"), Healing->OnSnapshotChanged.IsBound());
    AddInfo(TEXT("[M2-04A] Healing.Resource: native GE / one charge / rejection / snapshot / possession / reentrancy / EndPlay checked."));
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif