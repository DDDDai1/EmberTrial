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

// ---- 只观察换身体，不复制锁定目标；退出先归还旧臂，再解除观察 ----
void UETTargetLockCameraComponent::BeginPlay()
{
    Super::BeginPlay();
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnChanged);
    }
}

void UETTargetLockCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    RestoreShoulderOffset();
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        PC->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePossessedPawnChanged);
    }
    Super::EndPlay(EndPlayReason);
}

void UETTargetLockCameraComponent::Deactivate()
{
    Super::Deactivate();
    RestoreShoulderOffset();
}

void UETTargetLockCameraComponent::HandlePossessedPawnChanged(APawn* PreviousPawn,
                                                              APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    RestoreShoulderOffset();
}

bool UETTargetLockCameraComponent::HasValidSettings() const
{
    return FMath::IsFinite(FollowSpeed) && FollowSpeed >= .1f && FollowSpeed <= 30.0f &&
           FMath::IsFinite(MinPitch) && FMath::IsFinite(MaxPitch) && MinPitch >= -80.0f &&
           MaxPitch <= 80.0f && MinPitch < MaxPitch && FMath::IsFinite(PitchOffset) &&
           FMath::Abs(PitchOffset) <= 30.0f && FMath::IsFinite(YawOffset) &&
           FMath::Abs(YawOffset) <= 35.0f && FMath::IsFinite(MaxShoulderYaw) &&
           MaxShoulderYaw >= 0.0f && MaxShoulderYaw <= 30.0f && !ShoulderOffset.ContainsNaN() &&
           ShoulderOffset.GetAbsMax() <= 150.0 && FMath::IsFinite(TargetFocusHeight) &&
           TargetFocusHeight >= 0.0f && TargetFocusHeight <= 100.0f;
}

// ---- 沿Controller已有的UpdateRotation工作，无新增Tick / Timer / 目标缓存 ----
bool UETTargetLockCameraComponent::TryMakeRotationDelta(const UETTargetLockComponent* Lock,
                                                        float DeltaSeconds, FRotator& OutDelta)
{
    OutDelta = FRotator::ZeroRotator;
    const UWorld* World = GetWorld();
    const APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (bEndingPlay || !HasBegunPlay() || !IsActive() || !IsValid(World) || World->bIsTearingDown ||
        !HasValidSettings() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
    {
        RestoreShoulderOffset();
        return false;
    }
    if (World->IsPaused())
    {
        // 暂停保留当前取景，既不追目标，也不推进解锁后的回正。
        return false;
    }
    if (!IsValid(PC) || !PC->IsLocalController() || !PC->GetLocalPlayer() ||
        PC->IsLookInputIgnored() || !IsValid(PC->PlayerCameraManager.Get()))
    {
        RestoreShoulderOffset();
        return false;
    }
    const AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(PC->GetPawn());
    if (!IsValid(Body) || !Body->IsGameplayReady() || Body->IsCombatDead() ||
        PC->GetViewTarget() != Body)
    {
        RestoreShoulderOffset();
        return false;
    }
    USpringArmComponent* Boom = Body->GetCameraBoom();
    const UCameraComponent* Camera = Body->GetFollowCamera();
    if (!IsValid(Boom) || !Boom->bUsePawnControlRotation || !IsValid(Camera) || !Camera->IsActive())
    {
        RestoreShoulderOffset();
        return false;
    }
    const AEmberTrialCharacter* Target =
        IsValid(Lock) && Lock->GetOwner() == PC && Lock->HasBegunPlay() && Lock->IsActive()
            ? Lock->GetLockedTarget()
            : nullptr;
    if (!IsValid(Target) || Target == Body || Target->GetWorld() != World)
    {
        // 立即归还鼠标，肩位沿原更新流程平滑回正；不再依赖旧目标。
        ReturnShoulderOffset(DeltaSeconds);
        return false;
    }

    UpdateShoulderOffset(Boom, DeltaSeconds);
    const FVector Pivot = Boom->GetComponentLocation() + Boom->TargetOffset;
    const FVector Focus = Target->GetActorLocation() + FVector::UpVector * TargetFocusHeight;
    if (!CalculateRotationDelta(PC->GetControlRotation(), Pivot, Focus, Boom->SocketOffset,
                                DeltaSeconds, OutDelta))
    {
        RestoreShoulderOffset();
        return false;
    }
    return true;
}

// ---- 临时Socket所有权：按原值叠加一次，解锁缓回，硬退出精确还原 ----
void UETTargetLockCameraComponent::UpdateShoulderOffset(USpringArmComponent* Boom,
                                                        float DeltaSeconds)
{
    if (ShoulderBoom.Get() != Boom)
    {
        RestoreShoulderOffset();
        ShoulderBoom = Boom;
        SavedSocketOffset = Boom->SocketOffset;
    }
    const double Alpha = 1.0 - FMath::Exp(-static_cast<double>(FollowSpeed) * DeltaSeconds);
    Boom->SocketOffset = FMath::Lerp(Boom->SocketOffset, SavedSocketOffset + ShoulderOffset, Alpha);
}

void UETTargetLockCameraComponent::ReturnShoulderOffset(float DeltaSeconds)
{
    USpringArmComponent* Boom = ShoulderBoom.Get();
    if (!IsValid(Boom))
    {
        RestoreShoulderOffset();
        return;
    }
    const double Alpha = 1.0 - FMath::Exp(-static_cast<double>(FollowSpeed) * DeltaSeconds);
    Boom->SocketOffset = FMath::Lerp(Boom->SocketOffset, SavedSocketOffset, Alpha);
    if (Boom->SocketOffset.Equals(SavedSocketOffset, .05))
    {
        RestoreShoulderOffset();
    }
}

void UETTargetLockCameraComponent::RestoreShoulderOffset()
{
    USpringArmComponent* Boom = ShoulderBoom.Get();
    ShoulderBoom.Reset();
    if (IsValid(Boom))
    {
        Boom->SocketOffset = SavedSocketOffset;
    }
    SavedSocketOffset = FVector::ZeroVector;
}

// ---- 先解侧移 / 抬高后的目标视线，再沿最短角路径插值；不使用上一帧相机坐标 ----
bool UETTargetLockCameraComponent::CalculateRotationDelta(
    const FRotator& Current, const FVector& SourceLocation, const FVector& TargetLocation,
    const FVector& SocketOffset, float DeltaSeconds, FRotator& OutDelta) const
{
    OutDelta = FRotator::ZeroRotator;
    if (!HasValidSettings() || Current.ContainsNaN() || SourceLocation.ContainsNaN() ||
        TargetLocation.ContainsNaN() || SocketOffset.ContainsNaN() ||
        !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
    {
        return false;
    }
    const FVector Direction = TargetLocation - SourceLocation;
    const double Distance2D = Direction.Size2D();
    if (Direction.ContainsNaN() || !FMath::IsFinite(Distance2D) || Distance2D <= UE_SMALL_NUMBER)
    {
        return false;
    }
    // 接近肩位半径时限制反三角输入，防止数值越界及贴身180度翻转。
    constexpr double MaxOffsetRatio = .95;
    const double GeometricSideAngle = FMath::RadiansToDegrees(
        FMath::Asin(FMath::Clamp(SocketOffset.Y / Distance2D, -MaxOffsetRatio, MaxOffsetRatio)));
    // 近战优先保持交战轴与背后视角，允许目标略离中心；不能用完整反正弦补偿追求居中。
    const double SideAngle = FMath::Clamp(GeometricSideAngle, -static_cast<double>(MaxShoulderYaw),
                                          static_cast<double>(MaxShoulderYaw));
    const double DesiredYaw = Direction.Rotation().Yaw - SideAngle + YawOffset;
    const FVector Forward2D = FRotator(0.0, DesiredYaw, 0.0).Vector();
    const double ForwardDistance = FMath::Max(FVector::DotProduct(Direction, Forward2D), 1.0);
    const double FocusDistance =
        FMath::Sqrt(FMath::Square(ForwardDistance) + FMath::Square(Direction.Z));
    const double LiftAngle = FMath::RadiansToDegrees(
        FMath::Asin(FMath::Clamp(SocketOffset.Z / FocusDistance, -MaxOffsetRatio, MaxOffsetRatio)));
    const double DesiredPitch =
        FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Direction.Z, ForwardDistance)) -
                         LiftAngle + PitchOffset,
                     static_cast<double>(MinPitch), static_cast<double>(MaxPitch));
    const FRotator NormalizedCurrent = Current.GetNormalized();
    const double Alpha = 1.0 - FMath::Exp(-static_cast<double>(FollowSpeed) * DeltaSeconds);
    OutDelta.Pitch = FRotator::NormalizeAxis(DesiredPitch - NormalizedCurrent.Pitch) * Alpha;
    OutDelta.Yaw = FRotator::NormalizeAxis(DesiredYaw - NormalizedCurrent.Yaw) * Alpha;
    return !OutDelta.ContainsNaN();
}
