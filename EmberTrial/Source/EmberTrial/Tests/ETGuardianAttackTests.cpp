#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_GuardianAttack.h"

#include "Animation/AnimMontage.h"
#include "Characters/ETGuardianCharacter.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETGuardianAttackRulesTest, "EmberTrial.M4.Guardian.AttackRules",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETGuardianAttackRulesTest::RunTest(const FString& Parameters)
{
    using EMove = UETGA_GuardianAttack::EMove;
    const auto Select = [](double Distance, bool bPhaseTwo, bool bNextShock)
    {
        return UETGA_GuardianAttack::SelectMove(Distance, 140.0f, bPhaseTwo, bNextShock);
    };

    TestTrue(TEXT("Phase one uses sweep up close"), Select(90.0, false, true) == EMove::Sweep);
    TestTrue(TEXT("Exact close threshold remains a close attack"), Select(140.0, false, true) == EMove::Sweep);
    TestTrue(TEXT("Far phase-one target gets a charge"), Select(140.1, false, true) == EMove::Charge);
    TestTrue(TEXT("Phase two starts its close sequence with ground shock"), Select(90.0, true, true) == EMove::GroundShock);
    TestTrue(TEXT("Phase two can return to sweep up close"), Select(90.0, true, false) == EMove::Sweep);
    TestTrue(TEXT("Phase-two charge ignores the close sequence"),
             Select(240.0, true, true) == EMove::Charge && Select(240.0, true, false) == EMove::Charge);
    TestTrue(TEXT("Invalid distance cannot choose a move"),
             Select(-1.0, false, false) == EMove::Invalid &&
             Select(std::numeric_limits<double>::quiet_NaN(), true, true) == EMove::Invalid);

    UETGA_GuardianAttack* Attack = NewObject<UETGA_GuardianAttack>();
    Attack->ChargeMontage = NewObject<UAnimMontage>(Attack);
    Attack->GroundShockMontage = NewObject<UAnimMontage>(Attack);
    UAnimMontage* Sweep = NewObject<UAnimMontage>(Attack);
    const AETGuardianCharacter& Guardian = *GetDefault<AETGuardianCharacter>();
    Attack->OnOpeningAttackStarted(Guardian, *Attack->GroundShockMontage);
    TestFalse(TEXT("Starting a shock queues sweep for the next close attack"), Attack->bNextCloseIsGroundShock);
    Attack->OnOpeningAttackStarted(Guardian, *Attack->ChargeMontage);
    TestFalse(TEXT("Starting a charge leaves the close attack pending"), Attack->bNextCloseIsGroundShock);
    Attack->OnOpeningAttackStarted(Guardian, *Sweep);
    TestTrue(TEXT("Starting the sweep queues shock again"), Attack->bNextCloseIsGroundShock);
    return !HasAnyErrors();
}

#endif
