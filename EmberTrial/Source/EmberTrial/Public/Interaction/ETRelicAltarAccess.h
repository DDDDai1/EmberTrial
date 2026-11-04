#pragma once

#include "CoreMinimal.h"
#include "Templates/UniquePtr.h"

class APlayerController;
class AEmberTrialCharacter;
class AETRelicAltar;
class UETRelicEquipmentComponent;
class UETRunSessionSubsystem;
class UWorld;

// 单次页面上下文：Core只验证身份 / 空间，暂停所有权由页面Flow负责。
// C2由Flow在暂停前创建，失败 / 关闭即撤销；普通P页不创建。
class EMBERTRIAL_API FETRelicAltarAccess
{
public:
    static TUniquePtr<FETRelicAltarAccess> TryOpen(
        APlayerController* Controller, AETRelicAltar* Altar);

    bool CanCommitFor(const UETRelicEquipmentComponent* Equipment) const;
    void Close();
    ~FETRelicAltarAccess() = default;

    FETRelicAltarAccess(const FETRelicAltarAccess&) = delete;
    FETRelicAltarAccess& operator=(const FETRelicAltarAccess&) = delete;
    FETRelicAltarAccess(FETRelicAltarAccess&&) = delete;
    FETRelicAltarAccess& operator=(FETRelicAltarAccess&&) = delete;

private:
    FETRelicAltarAccess() = default;

    TWeakObjectPtr<UWorld> BoundWorld;
    TWeakObjectPtr<APlayerController> BoundController;
    TWeakObjectPtr<AEmberTrialCharacter> BoundBody;
    TWeakObjectPtr<AETRelicAltar> BoundAltar;
    TWeakObjectPtr<UETRunSessionSubsystem> BoundSession;
    bool bOpen = false;
};