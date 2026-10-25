#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETTargetLockCameraComponent.generated.h"

class UETTargetLockComponent;
class APawn;
class USpringArmComponent;

// Controller拥有锁定镜头与临时肩位偏移；目标身份仍由Core锁定组件负责。
UCLASS(ClassGroup = (EmberTrial))
class EMBERTRIAL_API UETTargetLockCameraComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETTargetLockCameraComponent();

    // 由Controller::UpdateRotation调用；false表示本帧沿用原生自由视角输入。
    bool TryMakeRotationDelta(const UETTargetLockComponent* Lock, float DeltaSeconds,
                              FRotator& OutDelta);

    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETTargetLockCameraRulesTest;

    // 指数收敛速度；不是角速度。不同帧率对静止目标有一致的收敛时间。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "0.1", ClampMax = "30.0"))
    float FollowSpeed = 8.0f;

    // 限制期望俯仰；从自由镜头进入时平滑收敛，不把当前角度瞬移到边界。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "-80.0", ClampMax = "80.0", Units = "deg"))
    float MinPitch = -45.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "-80.0", ClampMax = "80.0", Units = "deg"))
    float MaxPitch = 15.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "-30.0", ClampMax = "30.0", Units = "deg"))
    float PitchOffset = -18.0f;

    // 肩位已经参与取景求解；通常保持0，仅供构图微调。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "-35.0", ClampMax = "35.0", Units = "deg"))
    float YawOffset = 0.0f;

    // 肩位补偿相对交战轴的偏航上限；贴身时不再为了居中绕到侧面。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "0.0", ClampMax = "30.0", Units = "deg"))
    float MaxShoulderYaw = 12.0f;

    // 相机局部+Y为右侧。通过SpringArm SocketOffset参与原有碰撞检测。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera", meta = (Units = "cm"))
    FVector ShoulderOffset = FVector(0.0, 75.0, 55.0);

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock Camera",
              meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "cm"))
    float TargetFocusHeight = 45.0f;

    // 只保存本组件临时取得的臂偏移，换身体 / 失活 / 结束时原值归还。
    TWeakObjectPtr<USpringArmComponent> ShoulderBoom;
    FVector SavedSocketOffset = FVector::ZeroVector;
    bool bEndingPlay = false;

    bool HasValidSettings() const;
    void UpdateShoulderOffset(USpringArmComponent* Boom, float DeltaSeconds);
    void ReturnShoulderOffset(float DeltaSeconds);
    void RestoreShoulderOffset();

    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    bool CalculateRotationDelta(const FRotator& Current, const FVector& SourceLocation,
                                const FVector& TargetLocation, const FVector& SocketOffset,
                                float DeltaSeconds, FRotator& OutDelta) const;
};
