#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ETANS_ComboWindow.generated.h"

UCLASS(meta=(DisplayName="ET Combo Window"))
class EMBERTRIAL_API UETANS_ComboWindow : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UETANS_ComboWindow();
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;
private:
    void Route(FBranchingPointNotifyPayload& Payload,bool bOpen);
};