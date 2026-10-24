#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETTargetLockCameraComponent.generated.h"

class UETTargetLockComponent;

// Controller原生子对象：提供本帧镜头增量，不保存第二份目标、不改Actor朝向。
UCLASS(ClassGroup = (EmberTrial))
class EMBERTRIAL_API UETTargetLockCameraComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETTargetLockCameraComponent();

    // 由Controller::UpdateRotation调用；false表示本帧沿用原生自由视角输入。
    bool TryMakeRotationDelta(const UETTargetLockComponent* Lock, float DeltaSeconds,
                              FRotator& OutDelta) const;

private:
    friend class FETTargetLockCameraRulesTest;

    // 指数收敛速度；不是角速度。不同帧率对静止目标有一致的收敛时间。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "0.1", ClampMax = "30.0"))
    float FollowSpeed = 8.0f;

    // 限制期望俯仰；从自由镜头进入时平滑收敛，不把当前角度瞬移到边界。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "-80.0", ClampMax = "80.0", Units = "deg"))
    float MinPitch = -35.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "-80.0", ClampMax = "80.0", Units = "deg"))
    float MaxPitch = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "-30.0", ClampMax = "30.0", Units = "deg"))
    float PitchOffset = -12.0f;

    bool CalculateRotationDelta(const FRotator& Current, const FVector& SourceLocation,
                                const FVector& TargetLocation, float DeltaSeconds,
                                FRotator& OutDelta) const;
};