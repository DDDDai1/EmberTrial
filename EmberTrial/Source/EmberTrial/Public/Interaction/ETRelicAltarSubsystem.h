#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ETRelicAltarSubsystem.generated.h"

class AEmberTrialCharacter;
class AETRelicAltar;

// 当前World的祭坛索引：只弱持有注册对象，按请求查询，不扫描全世界或每帧轮询。
UCLASS()
class EMBERTRIAL_API UETRelicAltarSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "EmberTrial|Relic Altar")
    AETRelicAltar* FindAvailableAltar(AEmberTrialCharacter* PlayerBody) const;

    // 此处是打开入口前的运行态资格；不表示已获得页面中的换装许可。
    static bool IsEligiblePlayer(const AEmberTrialCharacter* PlayerBody);
    virtual void Deinitialize() override;

protected:
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
    friend class AETRelicAltar;
    void RegisterAltar(AETRelicAltar* Altar);
    void UnregisterAltar(AETRelicAltar* Altar);

    TSet<TWeakObjectPtr<AETRelicAltar>> Altars;
    bool bClosing = false;
};