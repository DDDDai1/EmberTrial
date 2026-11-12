#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/ETAttackCharge.h"
#include "Combat/ETAttackSector.h"
#include "ETANS_AttackWindow.generated.h"

// 通知对象可被多个播放实例共用，不在此缓存Mesh、Owner或窗口状态。
UCLASS(meta = (DisplayName = "ET Attack Window"))
class EMBERTRIAL_API UETANS_AttackWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UETANS_AttackWindow();
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;

protected:
    virtual const FETAttackSector* GetSector() const { return nullptr; }
    virtual const FETAttackCharge* GetCharge() const { return nullptr; }

private:
    void Route(FBranchingPointNotifyPayload& Payload, bool bOpen);
};

// This notify uses the same montage identity and lifetime as the hand trace.
UCLASS(meta = (DisplayName = "ET Sector Attack Window"))
class EMBERTRIAL_API UETANS_SectorAttackWindow : public UETANS_AttackWindow
{
    GENERATED_BODY()

protected:
    virtual const FETAttackSector* GetSector() const override { return &Sector; }

private:
    UPROPERTY(EditAnywhere, Category = "EmberTrial|Attack", meta = (ShowOnlyInnerProperties))
    FETAttackSector Sector;
};

UCLASS(meta = (DisplayName = "ET Charge Attack Window"))
class EMBERTRIAL_API UETANS_ChargeAttackWindow : public UETANS_AttackWindow
{
    GENERATED_BODY()

protected:
    virtual const FETAttackCharge* GetCharge() const override { return &Charge; }

private:
    UPROPERTY(EditAnywhere, Category = "EmberTrial|Attack", meta = (ShowOnlyInnerProperties))
    FETAttackCharge Charge;
};
