#include "Animation/ETANS_ComboWindow.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Abilities/ETGA_LightAttack.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETANS_ComboWindow::UETANS_ComboWindow()
{
    bIsNativeBranchingPoint=true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor=false;NotifyColor=FColor::Cyan;
#endif
}
void UETANS_ComboWindow::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) { Route(Payload,true); }
void UETANS_ComboWindow::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) { Route(Payload,false); }
void UETANS_ComboWindow::Route(FBranchingPointNotifyPayload& Payload,bool bOpen)
{
    USkeletalMeshComponent* Mesh=Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body=IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC=IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_LightAttack* Attack=IsValid(ASC) ? Cast<UETGA_LightAttack>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Attack)) { Attack->HandleComboWindow(Mesh,Payload.SequenceAsset,Payload.MontageInstanceID,this,bOpen); }
}