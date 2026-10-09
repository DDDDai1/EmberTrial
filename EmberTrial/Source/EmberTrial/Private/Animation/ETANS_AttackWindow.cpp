#include "Animation/ETANS_AttackWindow.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

UETANS_AttackWindow::UETANS_AttackWindow()
{
    bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor = false;
    NotifyColor = FColor::Orange;
#endif
}

void UETANS_AttackWindow::BranchingPointNotifyBegin(
    FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
    AActor* Owner = IsValid(Mesh) ? Mesh->GetOwner() : nullptr;
    UETCombatTraceComponent* Trace = IsValid(Owner)
        ? Owner->FindComponentByClass<UETCombatTraceComponent>() : nullptr;
    if (IsValid(Trace))
    {
        Trace->SetAttackWindow(Mesh, Payload.SequenceAsset,
            Payload.MontageInstanceID, this, true);
    }
}

void UETANS_AttackWindow::BranchingPointNotifyEnd(
    FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh = Payload.SkelMeshComponent;
    AActor* Owner = IsValid(Mesh) ? Mesh->GetOwner() : nullptr;
    UETCombatTraceComponent* Trace = IsValid(Owner)
        ? Owner->FindComponentByClass<UETCombatTraceComponent>() : nullptr;
    if (IsValid(Trace))
    {
        Trace->SetAttackWindow(Mesh, Payload.SequenceAsset,
            Payload.MontageInstanceID, this, false);
    }
}