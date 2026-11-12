#include "Abilities/ETGA_GuardianAttack.h"

#include "AI/ETAIController.h"
#include "Animation/AnimMontage.h"
#include "Characters/ETGuardianCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGuardianAttack, Log, All);

UETGA_GuardianAttack::EMove UETGA_GuardianAttack::SelectMove(double Distance, float Range, bool bPhaseTwo,
                                                          bool bNextCloseIsGroundShock)
{
    if (!FMath::IsFinite(Distance) || Distance < 0.0 || !FMath::IsFinite(Range) || Range < 1.0f)
    {
        return EMove::Invalid;
    }
    if (Distance > Range)
    {
        return EMove::Charge;
    }
    return bPhaseTwo && bNextCloseIsGroundShock ? EMove::GroundShock : EMove::Sweep;
}

UAnimMontage* UETGA_GuardianAttack::SelectOpeningAttack(const AEmberTrialCharacter& Body, float& OutDamage) const
{
    const AETGuardianCharacter* Guardian = Cast<AETGuardianCharacter>(&Body);
    const AETAIController* AI = Cast<AETAIController>(Body.GetController());
    const AEmberTrialCharacter* Target = IsValid(AI) ? AI->GetSensedPlayer() : nullptr;
    UAnimMontage* Sweep = Super::SelectOpeningAttack(Body, OutDamage);
    if (!IsValid(Guardian) || !IsValid(Target) || !Body.CanDealCombatDamageTo(Target) ||
        !IsValid(Sweep) || !IsValid(ChargeMontage) || !IsValid(GroundShockMontage) ||
        Sweep == ChargeMontage.Get() || Sweep == GroundShockMontage.Get() || ChargeMontage == GroundShockMontage)
    {
        return nullptr;
    }

    const double Distance = FVector::Dist2D(Body.GetActorLocation(), Target->GetActorLocation());
    switch (SelectMove(Distance, CloseRange, Guardian->IsInPhaseTwo(), bNextCloseIsGroundShock))
    {
        case EMove::Sweep:
            return Sweep;
        case EMove::Charge:
            OutDamage = ChargeDamage;
            return ChargeMontage;
        case EMove::GroundShock:
            OutDamage = GroundShockDamage;
            return GroundShockMontage;
        default:
            return nullptr;
    }
}

void UETGA_GuardianAttack::OnOpeningAttackStarted(const AEmberTrialCharacter& Body, const UAnimMontage& Montage)
{
    const AETGuardianCharacter* Guardian = CastChecked<AETGuardianCharacter>(&Body);
    const TCHAR* Move = TEXT("Charge");
    if (&Montage != ChargeMontage.Get())
    {
        const bool bGroundShock = &Montage == GroundShockMontage.Get();
        bNextCloseIsGroundShock = !bGroundShock;
        Move = bGroundShock ? TEXT("GroundShock") : TEXT("Sweep");
    }
    UE_LOG(LogETGuardianAttack, Display, TEXT("Guardian attack Body=%s Phase=%d Move=%s Montage=%s"),
           *GetNameSafe(&Body), Guardian->IsInPhaseTwo() ? 2 : 1, Move, *GetNameSafe(&Montage));
}
