#include "Animation/ETAN_BreakPulseCommit.h"
#include "Abilities/ETGA_BreakPulse.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETAN_BreakPulseCommit::UETAN_BreakPulseCommit()
{
    // BranchingPoint保持同步通知；编辑器预览不执行正式破势范围结算业务。
    bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;
    NotifyColor = FColor::Cyan;
#endif
}
// 当前AnimatingAbility负责核对激活与准确实例，Notify不直接操作Poise。
void UETAN_BreakPulseCommit::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body =
        IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC = IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_BreakPulse* Pulse =
        IsValid(ASC) ? Cast<UETGA_BreakPulse>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Pulse))
    {
        Pulse->HandleBreakPulseCommit(Mesh, Payload.SequenceAsset, Payload.MontageInstanceID, this);
    }
}