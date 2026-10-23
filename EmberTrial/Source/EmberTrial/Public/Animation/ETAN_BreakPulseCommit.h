#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ETAN_BreakPulseCommit.generated.h"

// 动画资产上的无状态路由；一次提交、范围目标结算与取消保护由当前GA负责。
UCLASS(meta = (DisplayName = "ET Break Pulse Commit"))
class EMBERTRIAL_API UETAN_BreakPulseCommit : public UAnimNotify
{
    GENERATED_BODY()
public:
    UETAN_BreakPulseCommit();
    // 传递Mesh / 动画 / Montage实例 / Notify身份，不在共享Notify中缓存身体。
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};