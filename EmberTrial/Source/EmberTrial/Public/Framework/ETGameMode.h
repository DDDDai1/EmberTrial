#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ETGameMode.generated.h"

class UETRunSessionSubsystem;
class ACharacter;

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

    virtual APawn* SpawnDefaultPawnAtTransform_Implementation(
        AController* NewPlayer, const FTransform& SpawnTransform) override;

private:
    friend class FETSpawnPlacementTest;

    // 只在生成玩家前探测一次可走地面；0关闭，不对高空或非法点强行吸附。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Spawn",
              meta = (ClampMin = "0.0", ClampMax = "500.0", Units = "cm"))
    float SpawnGroundProbeDistance = 100.0f;

    FTransform MakeGroundedSpawnTransform(const FTransform& SpawnTransform,
                                          const ACharacter* DefaultBody) const;

    // 每次查询当前GI，避免把跨世界对象缓存到规则Actor上。
    UETRunSessionSubsystem* GetRunSession() const;

    // 已实际选择的ID；不是请求ID，回退后由StartPlay回报实际值。
    FName SelectedSpawnId = NAME_None;
};
