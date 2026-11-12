#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/ETGuardianCharacter.h"

#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETGuardianPhaseTest, "EmberTrial.M4.Guardian.Phase",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETGuardianPhaseTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game) || !Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    const FGameplayTag BusyTag = ETGameplayTags::State_Attacking;
    const auto Spawn = [&Fixture](float X)
    {
        return Fixture.GetTestWorld()->SpawnActor<AETGuardianCharacter>(
            FVector(X, 0.0f, 200.0f), FRotator::ZeroRotator);
    };
    const auto SetHealth = [](AETGuardianCharacter& Guardian, float Health)
    {
        Guardian.GetAbilitySystemComponent()->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), Health);
    };
    const auto Tick = [&Fixture]()
    {
        return Fixture.TickTestWorld(0.02f) && Fixture.TickTestWorld(0.02f);
    };

    AETGuardianCharacter* Guardian = Spawn(0.0f);
    AETGuardianCharacter* Lethal = Spawn(400.0f);
    if (!TestTrue(TEXT("Both guardian bodies ready"), Guardian && Lethal &&
                  Guardian->IsGameplayReady() && Lethal->IsGameplayReady()))
    {
        return false;
    }

    TestFalse(TEXT("Fresh body starts in phase one"), Guardian->IsInPhaseTwo());
    TestEqual(TEXT("Phase one walking speed"), Guardian->GetCharacterMovement()->MaxWalkSpeed, 220.0f);
    SetHealth(*Guardian, 51.0f);
    if (!Tick())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("Above half health stays in phase one"), Guardian->IsInPhaseTwo());

    UAbilitySystemComponent* ASC = Guardian->GetAbilitySystemComponent();
    ASC->AddLooseGameplayTag(BusyTag);
    SetHealth(*Guardian, 50.0f);
    if (!Tick())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("Busy at threshold defers transition"), Guardian->IsInPhaseTwo());

    ASC->RemoveLooseGameplayTag(BusyTag);
    if (!Tick())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Recovery at exactly half health enters phase two"), Guardian->IsInPhaseTwo());
    TestEqual(TEXT("Phase two walking speed"), Guardian->GetCharacterMovement()->MaxWalkSpeed, 330.0f);

    SetHealth(*Guardian, 90.0f);
    SetHealth(*Lethal, 50.0f);
    SetHealth(*Lethal, 0.0f);
    if (!Tick())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Healing does not reset phase"), Guardian->IsInPhaseTwo());
    TestTrue(TEXT("Lethal damage wins over queued transition"), Lethal->IsCombatDead() && !Lethal->IsInPhaseTwo());
    const UTextRenderComponent* Marker = Lethal->FindComponentByClass<UTextRenderComponent>();
    TestTrue(TEXT("Death hides the phase marker"), Marker && !Marker->IsVisible());

    AETGuardianCharacter* Fresh = Spawn(800.0f);
    TestTrue(TEXT("New encounter body has independent phase and speed"), Fresh && !Fresh->IsInPhaseTwo() &&
             Fresh->GetCharacterMovement()->MaxWalkSpeed == 220.0f);

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
