#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ETAN_RangedShotRelease.generated.h"

UCLASS(meta = (DisplayName = "ET Ranged Shot Release"))
class EMBERTRIAL_API UETAN_RangedShotRelease : public UAnimNotify
{
    GENERATED_BODY()

public:
    UETAN_RangedShotRelease();
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};