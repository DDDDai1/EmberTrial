#include "TargetLock/ETTargetLockCameraComponent.h"
#include "TargetLock/ETTargetLockComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

UETTargetLockCameraComponent::UETTargetLockCameraComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

// ---- 无Timer / Tick / 订阅 / 目标缓存；每帧只读取Controller已有的Core锁定身份 ----
bool UETTargetLockCameraComponent::TryMakeRotationDelta(const UETTargetLockComponent* Lock,
                                                        float DeltaSeconds,
                                                        FRotator& OutDelta) const
{
    OutDelta = FRotator::ZeroRotator;
    const UWorld* World = GetWorld();
    const APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!HasBegunPlay() || !IsActive() || !IsValid(World) || World->bIsTearingDown ||
        World->IsPaused() || !IsValid(PC) || !PC->IsLocalController() || !PC->GetLocalPlayer() ||
        PC->IsLookInputIgnored() || !IsValid(PC->PlayerCameraManager.Get()) || !IsValid(Lock) ||
        Lock->GetOwner() != PC || !Lock->HasBegunPlay() || !Lock->IsActive())
    {
        return false;
    }

    const AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(PC->GetPawn());
    const AEmberTrialCharacter* Target = Lock->GetLockedTarget();
    if (!IsValid(Body) || !Body->IsGameplayReady() || Body->IsCombatDead() ||
        PC->GetViewTarget() != Body || !IsValid(Target) || Target == Body ||
        Target->GetWorld() != World)
    {
        return false;
    }
    const USpringArmComponent* Boom = Body->GetCameraBoom();
    const UCameraComponent* Camera = Body->GetFollowCamera();
    if (!IsValid(Boom) || !Boom->bUsePawnControlRotation || !IsValid(Camera) || !Camera->IsActive())
    {
        return false;
    }

    // 用身体中心的连线，避免上一帧相机位置反过来影响下一帧的跟随目标角度。
    return CalculateRotationDelta(PC->GetControlRotation(), Body->GetActorLocation(),
                                  Target->GetActorLocation(), DeltaSeconds, OutDelta);
}

// ---- 有限数值 / 最短角路径 / 指数插值；纯规则由专项覆盖 ----
bool UETTargetLockCameraComponent::CalculateRotationDelta(const FRotator& Current,
                                                          const FVector& SourceLocation,
                                                          const FVector& TargetLocation,
                                                          float DeltaSeconds,
                                                          FRotator& OutDelta) const
{
    OutDelta = FRotator::ZeroRotator;
    if (Current.ContainsNaN() || SourceLocation.ContainsNaN() || TargetLocation.ContainsNaN() ||
        !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f || !FMath::IsFinite(FollowSpeed) ||
        FollowSpeed < .1f || FollowSpeed > 30.0f || !FMath::IsFinite(MinPitch) ||
        !FMath::IsFinite(MaxPitch) || MinPitch < -80.0f || MaxPitch > 80.0f ||
        MinPitch >= MaxPitch || !FMath::IsFinite(PitchOffset) || FMath::Abs(PitchOffset) > 30.0f)
    {
        return false;
    }
    const FVector Direction = TargetLocation - SourceLocation;
    if (Direction.ContainsNaN() || Direction.SizeSquared2D() <= UE_SMALL_NUMBER)
    {
        return false;
    }
    const FRotator Aim = Direction.Rotation();
    const double DesiredPitch =
        FMath::Clamp(Aim.Pitch + static_cast<double>(PitchOffset), static_cast<double>(MinPitch),
                     static_cast<double>(MaxPitch));
    const FRotator NormalizedCurrent = Current.GetNormalized();
    const double Alpha = 1.0 - FMath::Exp(-static_cast<double>(FollowSpeed) * DeltaSeconds);
    OutDelta.Pitch = FRotator::NormalizeAxis(DesiredPitch - NormalizedCurrent.Pitch) * Alpha;
    OutDelta.Yaw = FRotator::NormalizeAxis(Aim.Yaw - NormalizedCurrent.Yaw) * Alpha;
    // Roll增量为0；不接管侧倾，也不直接SetControlRotation / SetActorRotation。
    return !OutDelta.ContainsNaN();
}