#pragma once

#include "Abilities/ETGA_LightAttack.h"
#include "ETGA_GuardianAttack.generated.h"

UCLASS()
class EMBERTRIAL_API UETGA_GuardianAttack : public UETGA_LightAttack
{
    GENERATED_BODY()

protected:
    virtual UAnimMontage* SelectOpeningAttack(const AEmberTrialCharacter& Body, float& OutDamage) const override;
    virtual void OnOpeningAttackStarted(const AEmberTrialCharacter& Body, const UAnimMontage& Montage) override;

private:
    friend class FETGuardianAttackRulesTest;

    enum class EMove : uint8
    {
        Invalid,
        Sweep,
        Charge,
        GroundShock
    };

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian Attack")
    TObjectPtr<UAnimMontage> ChargeMontage;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian Attack")
    TObjectPtr<UAnimMontage> GroundShockMontage;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian Attack", meta = (ClampMin = "1.0"))
    float ChargeDamage = 22.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian Attack", meta = (ClampMin = "1.0"))
    float GroundShockDamage = 26.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Guardian Attack", meta = (ClampMin = "1.0", Units = "cm"))
    float CloseRange = 140.0f;

    // The first close attack in phase two is the shock. Charges leave the close sequence unchanged.
    bool bNextCloseIsGroundShock = true;

    static EMove SelectMove(double Distance, float CloseRange, bool bPhaseTwo, bool bNextCloseIsGroundShock);
};
