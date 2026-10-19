#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ETGameMode.generated.h"

class UETRunSessionSubsystem;

// 单人试炼按稳定ID选出生点；只向Session报告实际选择，不处理UI或存档。
UCLASS()
class EMBERTRIAL_API AETGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AETGameMode();
    virtual void StartPlay() override;

    UFUNCTION(Exec, meta = (DevelopmentOnly))
    void ETDebugSetCheckpointId(FName TestId);

protected:
    // ---- 在生成Pawn前选择，绕过旧StartSpot / 编辑器临时出生点 ----
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    virtual AActor* FindPlayerStart_Implementation(AController* Player,
                                                   const FString& IncomingName) override;

private:
    // 每次查询当前GI，避免把跨世界对象缓存到规则Actor上。
    UETRunSessionSubsystem* GetRunSession() const;

    // 已实际选择的ID；不是请求ID，回退后由StartPlay回报实际值。
    FName SelectedSpawnId = NAME_None;
};
