#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ETANS_ComboWindow.generated.h"

// 只转发带Montage实例ID和Token的通知；连段状态由当前攻击Ability保存。
UCLASS(meta = (DisplayName = "ET Combo Window"))
class EMBERTRIAL_API UETANS_ComboWindow : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UETANS_ComboWindow();
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;

private:
    void Route(FBranchingPointNotifyPayload& Payload, bool bOpen);
};