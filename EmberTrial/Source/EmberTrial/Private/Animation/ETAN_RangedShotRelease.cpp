#include "Animation/ETAN_RangedShotRelease.h"

#include "Abilities/ETGA_RangedShot.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETAN_RangedShotRelease::UETAN_RangedShotRelease()
{
    bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;
#endif
}

void UETAN_RangedShotRelease::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body = IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC = IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_RangedShot* Ability = IsValid(ASC) ? Cast<UETGA_RangedShot>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Ability))
    {
        Ability->HandleRelease(Mesh, Payload.SequenceAsset, Payload.MontageInstanceID, this);
    }
}