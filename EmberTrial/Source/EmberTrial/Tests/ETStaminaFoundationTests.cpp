#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EmberTrialCharacter.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETStaminaFoundationTest,
    "EmberTrial.M2.Resources.StaminaFoundation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETStaminaFoundationTest::RunTest(const FString& Parameters)
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
    auto MakeBody = [&]()
    {
        return World->SpawnActor<AEmberTrialCharacter>(
            FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    };
    AEmberTrialCharacter* Body = MakeBody();
    AEmberTrialCharacter* ZeroBody = MakeBody();
    AEmberTrialCharacter* InvalidBody = MakeBody();
    AEmberTrialCharacter* NonFiniteBody = MakeBody();
    if (!TestNotNull(TEXT("Main body"), Body)
        || !TestNotNull(TEXT("Zero stamina body"), ZeroBody)
        || !TestNotNull(TEXT("Invalid configuration body"), InvalidBody)
        || !TestNotNull(TEXT("Non-finite configuration body"), NonFiniteBody))
    {
        return false;
    }

    Body->InitialStamina = 40.0f;
    Body->InitialMaxStamina = 80.0f;
    ZeroBody->InitialStamina = 0.0f;
    ZeroBody->InitialMaxStamina = 80.0f;
    InvalidBody->InitialHealth = 40.0f;
    InvalidBody->InitialMaxHealth = 80.0f;
    InvalidBody->InitialStamina = 100.0f;
    InvalidBody->InitialMaxStamina = 60.0f;
    NonFiniteBody->InitialStamina = std::numeric_limits<float>::quiet_NaN();

    TestFalse(TEXT("Before BeginPlay is not Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Before BeginPlay cannot initialize"), Body->EnsureGameplayInitialized());
    AddExpectedMessagePlain(TEXT("Invalid startup stamina"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 2);
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* InvalidASC = InvalidBody->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Main ASC"), ASC)
        || !TestNotNull(TEXT("Invalid body's ASC"), InvalidASC)
        || !TestNotNull(TEXT("Registered stamina set"), ASC->GetSet<UETStaminaAttributeSet>()))
    {
        return false;
    }
    TestTrue(TEXT("Registered set is the owned default subobject"),
        ASC->GetSet<UETStaminaAttributeSet>() == Body->StaminaAttributes.Get());

    auto ExpectSnapshot = [&](AEmberTrialCharacter* Subject,
        const TCHAR* Label, float ExpectedStamina, float ExpectedMax)
    {
        float Stamina = -1.0f;
        float MaxStamina = -1.0f;
        TestTrue(FString::Printf(TEXT("%s available"), Label),
            Subject->GetGameplayStaminaSnapshot(Stamina, MaxStamina));
        TestTrue(FString::Printf(TEXT("%s current"), Label),
            FMath::IsNearlyEqual(Stamina, ExpectedStamina, 0.001f));
        TestTrue(FString::Printf(TEXT("%s maximum"), Label),
            FMath::IsNearlyEqual(MaxStamina, ExpectedMax, 0.001f));
        const UAbilitySystemComponent* SubjectASC = Subject->GetAbilitySystemComponent();
        TestTrue(FString::Printf(TEXT("%s base"), Label), FMath::IsNearlyEqual(
            SubjectASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
            ExpectedStamina, 0.001f));
        TestTrue(FString::Printf(TEXT("%s maximum base"), Label), FMath::IsNearlyEqual(
            SubjectASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetMaxStaminaAttribute()),
            ExpectedMax, 0.001f));
    };
    ExpectSnapshot(Body, TEXT("Initial"), 40.0f, 80.0f);
    ExpectSnapshot(ZeroBody, TEXT("Zero is valid"), 0.0f, 80.0f);
    TestTrue(TEXT("Zero stamina does not mean dead"), !ZeroBody->IsCombatDead());
    TestFalse(TEXT("Invalid stamina remains Waiting"), InvalidBody->IsGameplayReady());
    TestFalse(TEXT("NaN startup remains Waiting"), NonFiniteBody->IsGameplayReady());
    TestFalse(TEXT("Rejected initialization is not committed"),
        InvalidBody->bStartupAttributesInitialized);
    TestEqual(TEXT("Rejected stamina did not partially set Health"),
        InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()), 0.0f);
    TestEqual(TEXT("Rejected stamina did not partially set MaxHealth"),
        InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetMaxHealthAttribute()), 100.0f);
    float UnavailableStamina = -1.0f;
    float UnavailableMax = -1.0f;
    TestFalse(TEXT("Waiting snapshot unavailable"),
        InvalidBody->GetGameplayStaminaSnapshot(UnavailableStamina, UnavailableMax));
    TestEqual(TEXT("Unavailable stamina cleared"), UnavailableStamina, 0.0f);
    TestEqual(TEXT("Unavailable maximum cleared"), UnavailableMax, 0.0f);

    InvalidBody->InitialStamina = 30.0f;
    TestTrue(TEXT("Correcting before first successful initialization works"),
        InvalidBody->EnsureGameplayInitialized());
    ExpectSnapshot(InvalidBody, TEXT("Recovered configuration"), 30.0f, 60.0f);
    TestEqual(TEXT("Recovered Health initialized once"),
        InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()), 40.0f);

    // Transient real Instant effects; these do not create or save project assets.
    auto ApplyInstant = [&](const FGameplayAttribute& Attribute,
        EGameplayModOp::Type Operation, float Magnitude)
    {
        UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
        Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
        FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
        Modifier.Attribute = Attribute;
        Modifier.ModifierOp = Operation;
        Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));
        const FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.0f);
        ASC->ApplyGameplayEffectSpecToSelf(Spec);
    };
    const FGameplayAttribute StaminaAttribute = UETStaminaAttributeSet::GetStaminaAttribute();
    const FGameplayAttribute MaxAttribute = UETStaminaAttributeSet::GetMaxStaminaAttribute();
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, -25.0f);
    ExpectSnapshot(Body, TEXT("Consumed"), 15.0f, 80.0f);
    TestTrue(TEXT("Repeat initialization succeeds"), Body->EnsureGameplayInitialized());
    TestTrue(TEXT("Second repeat initialization succeeds"), Body->EnsureGameplayInitialized());
    ExpectSnapshot(Body, TEXT("Repeat preserves consumed value"), 15.0f, 80.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, 200.0f);
    ExpectSnapshot(Body, TEXT("Over recovery"), 80.0f, 80.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, -25.0f);
    ExpectSnapshot(Body, TEXT("No hidden overflow"), 55.0f, 80.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 40.0f);
    ExpectSnapshot(Body, TEXT("Maximum reduced"), 40.0f, 40.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 120.0f);
    ExpectSnapshot(Body, TEXT("Maximum increase does not refill"), 40.0f, 120.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, -200.0f);
    ExpectSnapshot(Body, TEXT("Over consumption"), 0.0f, 120.0f);
    TestTrue(TEXT("Exhausted body stays Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Exhaustion is not death"), Body->IsCombatDead());
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, 30.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 0.0f);
    ExpectSnapshot(Body, TEXT("Zero maximum clamped"), 1.0f, 1.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, -20.0f);
    ExpectSnapshot(Body, TEXT("Negative maximum clamped"), 1.0f, 1.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 80.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, 39.0f);
    ExpectSnapshot(Body, TEXT("Restored test values"), 40.0f, 80.0f);

    ASC->SetNumericAttributeBase(StaminaAttribute, std::numeric_limits<float>::quiet_NaN());
    ASC->SetNumericAttributeBase(MaxAttribute, std::numeric_limits<float>::infinity());
    ExpectSnapshot(Body, TEXT("Non-finite writes preserve safe values"), 40.0f, 80.0f);
    TestEqual(TEXT("Stamina effects leave Health isolated"),
        ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);

    AAIController* FirstController = World->SpawnActor<AAIController>();
    AAIController* SecondController = World->SpawnActor<AAIController>();
    if (!TestNotNull(TEXT("First controller"), FirstController)
        || !TestNotNull(TEXT("Second controller"), SecondController))
    {
        return false;
    }
    FirstController->Possess(Body);
    TestTrue(TEXT("First possession established"), FirstController->GetPawn() == Body);
    FirstController->UnPossess();
    ExpectSnapshot(Body, TEXT("UnPossess preserves body resource"), 40.0f, 80.0f);
    SecondController->Possess(Body);
    TestTrue(TEXT("Second possession established"), SecondController->GetPawn() == Body);
    TestTrue(TEXT("Ensure after possession succeeds"), Body->EnsureGameplayInitialized());
    ExpectSnapshot(Body, TEXT("New controller preserves resource"), 40.0f, 80.0f);

    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay revokes Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("EndPlay cannot reinitialize"), Body->EnsureGameplayInitialized());
    TestFalse(TEXT("EndPlay snapshot unavailable"),
        Body->GetGameplayStaminaSnapshot(UnavailableStamina, UnavailableMax));
    TestEqual(TEXT("EndPlay snapshot clears stamina"), UnavailableStamina, 0.0f);
    TestEqual(TEXT("EndPlay snapshot clears maximum"), UnavailableMax, 0.0f);
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif