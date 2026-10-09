#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ETANS_AttackWindow.generated.h"

UCLASS(meta = (DisplayName = "ET Attack Window"))
class EMBERTRIAL_API UETANS_AttackWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UETANS_AttackWindow();
    virtual void BranchingPointNotifyBegin(
        FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyEnd(
        FBranchingPointNotifyPayload& Payload) override;
};