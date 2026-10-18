#include "Animation/ETAN_HealCommit.h"
#include "Abilities/ETGA_Heal.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETAN_HealCommit::UETAN_HealCommit()
{
    // BranchingPoint保持同步通知；编辑器预览不执行正式治疗业务。
    bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;
    NotifyColor = FColor::Green;
#endif
}
// 当前AnimatingAbility负责核对激活与准确实例，Notify不直接操作Health或次数。
void UETAN_HealCommit::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body =
        IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC = IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_Heal* Heal = IsValid(ASC) ? Cast<UETGA_Heal>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Heal))
    {
        Heal->HandleHealCommit(Mesh, Payload.SequenceAsset, Payload.MontageInstanceID, this);
    }
}
