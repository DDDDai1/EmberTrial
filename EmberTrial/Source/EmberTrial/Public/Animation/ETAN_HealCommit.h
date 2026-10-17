#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ETAN_HealCommit.generated.h"

UCLASS(meta=(DisplayName="ET Heal Commit"))
class EMBERTRIAL_API UETAN_HealCommit : public UAnimNotify
{
    GENERATED_BODY()
public:
    UETAN_HealCommit();
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};