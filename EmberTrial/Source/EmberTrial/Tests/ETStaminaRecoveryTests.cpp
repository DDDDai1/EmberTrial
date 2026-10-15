#if WITH_DEV_AUTOMATION_TESTS

#include "Resources/ETStaminaRecoveryComponent.h"
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETStaminaRecoveryTest,
    "EmberTrial.M2.Resources.StaminaRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETStaminaRecoveryTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>();
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Temporary body"), Body) || !TestNotNull(TEXT("Pauser"), Pauser)) { return false; }
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    Body->GetCharacterMovement()->SetComponentTickEnabled(false);
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UETStaminaRecoveryComponent* Recovery = Body->FindComponentByClass<UETStaminaRecoveryComponent>();
    if (!TestNotNull(TEXT("ASC"), ASC) || !TestNotNull(TEXT("Recovery"), Recovery)) { return false; }
    auto Advance = [&](int32 Steps)
    {
        for (int32 Index = 0; Index < Steps; ++Index)
        {
            if (!TestWorld.TickTestWorld(0.05f))
            {
                TestWorld.ForwardErrorMessages(this);
                return false;
            }
        }
        return true;
    };
    const FGameplayAttribute Attribute = UETStaminaAttributeSet::GetStaminaAttribute();
    auto Value = [&]() { return ASC->GetNumericAttribute(Attribute); };
    auto ApplyDelta = [&](float Delta)
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
            UETStaminaDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
        if (!Spec.IsValid()) { AddError(TEXT("Stamina delta spec unavailable")); return; }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_StaminaDelta, Delta);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    };
    auto Same = [&](const TCHAR* Label, float Expected)
    {
        TestTrue(Label, FMath::IsNearlyEqual(Value(), Expected, 0.001f));
    };
    TestFalse(TEXT("Full has no timer"), Recovery->IsRecoveryScheduled());
    TestFalse(TEXT("No component Tick capability"), Recovery->PrimaryComponentTick.bCanEverTick);
    TestFalse(TEXT("No registered component Tick"), Recovery->PrimaryComponentTick.IsTickFunctionRegistered());
    ApplyDelta(-25.0f);
    Same(TEXT("Real GE consumes 25"), 75.0f);
    TestTrue(TEXT("Deficit schedules timer"), Recovery->IsRecoveryScheduled());
    if (!Advance(14)) { return false; } // 0.70 seconds, before delay.
    Same(TEXT("Delay prevents early recovery"), 75.0f);
    if (!Advance(4)) { return false; } // 0.90 seconds total, with sampling margin.
    TestTrue(TEXT("Recovery starts after delay"), Value() > 75.0f && Value() <= 79.0f);

    const float BeforePause = Value();
    const double TimeBeforePause = World->GetTimeSeconds();
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    const bool bPausedTicksOK = Advance(20);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr); // Always restore before exit.
    if (!bPausedTicksOK) { return false; }
    Same(TEXT("Pause freezes resource"), BeforePause);
    TestTrue(TEXT("Pause freezes world clock"), FMath::IsNearlyEqual(
        World->GetTimeSeconds(), TimeBeforePause, 0.001));
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("Resume recovers without paused catch-up"),
        Value() > BeforePause && Value() <= BeforePause + 6.2f);

    ApplyDelta(-10.0f);
    if (!Advance(6)) { return false; }
    ApplyDelta(-5.0f);
    const float AfterSecondSpend = Value();
    if (!Advance(14)) { return false; }
    Same(TEXT("Second spend resets delay"), AfterSecondSpend);
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("Recovery resumes after reset delay"), Value() > AfterSecondSpend);
    for (const FGameplayTag Tag : {FGameplayTag(ETGameplayTags::State_Attacking),
                                  FGameplayTag(ETGameplayTags::State_Dodging)})
    {
        ASC->AddLooseGameplayTag(Tag);
        TestFalse(TEXT("Busy tag stops timer immediately"), Recovery->IsRecoveryScheduled());
        const float BusyValue = Value();
        if (!Advance(20)) { return false; }
        Same(TEXT("Busy freezes recovery"), BusyValue);
        ASC->RemoveLooseGameplayTag(Tag);
        if (!Advance(4)) { return false; }
        TestTrue(TEXT("Busy end does not pay busy duration"), Value() > BusyValue && Value() <= BusyValue + 6.2f);
    }
    ApplyDelta(1000.0f);
    Same(TEXT("Recovery / restore caps at maximum"), 100.0f);
    TestFalse(TEXT("Full stops timer"), Recovery->IsRecoveryScheduled());

    ASC->SetNumericAttributeBase(UETStaminaAttributeSet::GetMaxStaminaAttribute(), 120.0f);
    Same(TEXT("Maximum increase itself does not refill"), 100.0f);
    TestTrue(TEXT("New deficit wakes recovery"), Recovery->IsRecoveryScheduled());
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("New capacity can recover later"), Value() > 100.0f && Value() <= 106.2f);
    ApplyDelta(1000.0f);
    Recovery->RefreshObservation();
    Recovery->RefreshObservation();
    const uint64 RevisionBeforeSpend = Recovery->DecreaseRevision;
    ApplyDelta(-5.0f);
    TestEqual(TEXT("Reobserve does not duplicate spend callback"),
        Recovery->DecreaseRevision, RevisionBeforeSpend + 1);
    const uint64 OldSerial = Recovery->ObservationSerial;
    Recovery->Deactivate();
    TestFalse(TEXT("Deactivate clears timer"), Recovery->IsRecoveryScheduled());
    ApplyDelta(-5.0f);
    const float DisabledValue = Value();
    if (!Advance(20)) { return false; }
    Same(TEXT("Inactive component does not recover"), DisabledValue);
    Recovery->Activate(true);
    const double NewDeadline = Recovery->RecoveryNotBefore;
    FOnAttributeChangeData StaleChange;
    StaleChange.Attribute = Attribute;
    StaleChange.OldValue = 100.0f;
    StaleChange.NewValue = 50.0f;
    Recovery->HandleAttributeChanged(StaleChange, TWeakObjectPtr<UAbilitySystemComponent>(ASC), OldSerial);
    TestEqual(TEXT("Old observation cannot reset new deadline"), Recovery->RecoveryNotBefore, NewDeadline);

    Recovery->RecoveryRate = 0.0f;
    AddExpectedMessagePlain(TEXT("Invalid recovery settings"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 1);
    Recovery->RefreshObservation();
    TestFalse(TEXT("Invalid rate leaves no timer"), Recovery->IsRecoveryScheduled());
    Recovery->RecoveryRate = 20.0f;
    Recovery->RefreshObservation();
    if (!Advance(18)) { return false; }
    TestTrue(TEXT("Valid settings recover again"), Value() > DisabledValue);
    TestEqual(TEXT("Stamina effect leaves Health alone"),
        ASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);

    ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestTrue(TEXT("Actual zero Health commits death"), Body->IsCombatDead());
    TestFalse(TEXT("Death stops timer immediately"), Recovery->IsRecoveryScheduled());
    const float AtDeath = Value();
    if (!Advance(20)) { return false; }
    Same(TEXT("Dead body does not recover"), AtDeath);
    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay leaves no timer"), Recovery->IsRecoveryScheduled());
    TestFalse(TEXT("EndPlay revokes Ready"), Body->IsGameplayReady());
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif