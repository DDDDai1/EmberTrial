#include "Animation/ETAN_HealCommit.h"
#include "Abilities/ETGA_Heal.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETAN_HealCommit::UETAN_HealCommit()
{
    bIsNativeBranchingPoint=true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor=false;NotifyColor=FColor::Green;
#endif
}
void UETAN_HealCommit::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh=Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body=IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC=IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_Heal* Heal=IsValid(ASC) ? Cast<UETGA_Heal>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Heal)) { Heal->HandleHealCommit(Mesh,Payload.SequenceAsset,Payload.MontageInstanceID,this); }
}