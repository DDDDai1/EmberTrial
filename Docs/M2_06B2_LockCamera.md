# M2-06B2：锁定镜头跟随

更新2026-10-25，UE5.5.4。B2基础 / 右肩历史证据见[验收记录](Reports/M2_06B2_Verification.md)；用户在C收尾又反馈贴身侧转，已授权直接补修。下方三份完整代码已与当前工程同步：Pitch Offset=-18°、Max Shoulder Yaw=12°。最新出生 / 镜头数值与四专项记录见[C指南10.1](M2_06C_TargetSwitching.md#101-出生首帧与贴身镜头补修已直接实施)，不重复手动录入或整章验收。

原B2用户手动90 / 等待5已登记，助手本次修改 / 构建 / computer-use不追加为用户手动工时。已存在的三份未提交YawOffset补修在忽略Raw中保留原始备份，再接入右肩实现，不改Git暂存或历史提交。

## 1. 最终取景与输入规则

Tab锁定同一个Core目标后，相机逐渐移到取景方向右侧75cm、抬高55cm，玩家在画面偏左处，敌人靠近中间；原臂长400cm和原FOV保持，能看清全身与敌人动作。目标注视点为敌人身体中心上方45cm。目标朝向根据当前肩位偏移作解析补偿，远景使敌人接近画面中心，近景限制肩位偏航补偿至12°以保持后侧视角，也不使用上一帧相机坐标造成反馈晃动。

相机局部+Y代表右侧；这是镜头取景坐标中的右肩，角色仍按原移动 / 动作规则转身。本章不每帧把Actor扭向目标，攻击 / 闪避方向仍由能力在原时机采样 / 固定。W沿镜头控制Yaw移动，不等同自动走向敌人。

锁定时Yaw / Pitch由跟随接管，鼠标不争抢；主动解除后立即归还鼠标，肩位沿原Controller更新平滑回到取得时的原值，不跳回旧角度。P暂停冻结跟随和回正；相机失活、换身体、结束 / 切图时精确归还旧SpringArm偏移。死亡Failure保持暂停画面，Retry / 新Controller不会继承旧偏移。

保留-45°至15°期望Pitch范围。PitchOffset=-18°、YawOffset=0，肩位与目标高度参与视线求解；MaxShoulderYaw=12°限制贴身侧转，不再叠加早期25°偏航方案。FollowSpeed8是指数收敛速度，进入 / 退出肩位均平滑；没有新增Tick或Timer。

视觉方向参考用户指定的第三人称动作游戏：[游戏科学《黑神话：悟空》产品页](https://store.steampowered.com/app/2358720/Black_Myth_Wukong/)。本项目使用自己的角色比例与参数；未取得该商业游戏内部相机配置，也未导入其素材。75 / 55 / 45为EmberTrial当前取景设定，不标成原作参数。

## 2. 架构与实际修改范围

原三新Core / 两旧Controller已由用户在基础版实施；B2右肩与近景部分只涉及下表三个既有Core文件。C收尾另改GameMode并新增SpawnPlacement测试，完整范围见C11节；本页不重复列入相机实现表：

| 文件（相对EmberTrial/Source） | 完整代码 | 职责 |
| --- | --- | --- |
| EmberTrial/Public/TargetLock/ETTargetLockCameraComponent.h | 3.1 | 肩位配置 / 观察与原值所有权 |
| EmberTrial/Private/TargetLock/ETTargetLockCameraComponent.cpp | 3.2 | 资格 / 视线求解 / 插值 / 恢复 |
| EmberTrial/Tests/ETTargetLockCameraTests.cpp | 3.3 | 原角度边界及肩位几何 / 真实换控清理 |

Controller仍在原UpdateRotation中调用组件，替换合法锁定帧的RotationInput后执行一次Super，让CameraManager继续管理原生旋转约束。组件只缓存弱SpringArm和取得时的SocketOffset，不缓存目标；唯一目标仍由Core TargetLock持有。组件新增一份Pawn更换观察，EndPlay成对解绑；不扫描Actor / GAS、不新增逐帧日志。

右肩通过SpringArm的SocketOffset实现，沿用引擎对该位置的碰撞Sweep。首次取得保存原值，之后向“原值+配置偏移”插值，不在现值上反复加75。解除时不再依赖旧目标，原UpdateRotation完成归还；失活 / 换身体 / 结束立即恢复原值并清弱引用。没有永久改变玩家相机或地图变换。

当前实现已在工程中，勿重复创建同名类或额外Add Component。首次学习可对照第3节；需要以后修改反射字段时仍先Save All / 关闭UE，再正常Build。A锁定、B1标记、UI、GA、Character源码、Build.cs / Target / Config保持。

## 3. 三个文件的完整代码

### 3.1 ETTargetLockCameraComponent.h

```cpp
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
```

### 3.2 ETTargetLockCameraComponent.cpp

```cpp
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
```

### 3.3 ETTargetLockCameraTests.cpp

该专项保留原角度 / 有限值 / 帧率规则，并使用FTestWorldWrapper中的真实Controller / 两个Pawn验证肩位不累加、平滑归还、Deactivate、Possess、Pause与EndPlay。它不创建Viewport或证明Mesh遮挡 / 墙边画面；本次取景另有computer-use与原生截图支持。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetLockCameraComponent.h"
#include "Misc/AutomationTest.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Framework/ETPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETTargetLockCameraRulesTest,
                                 "EmberTrial.M2.TargetLock.CameraRules",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETTargetLockCameraRulesTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UETTargetLockCameraComponent* Camera = NewObject<UETTargetLockCameraComponent>();
    if (!TestNotNull(TEXT("Camera rules object"), Camera))
    {
        return false;
    }
    // 保留原有零肩位角度基线，再单独验证新的右肩几何 / 生命周期。
    Camera->PitchOffset = -25.0f;
    Camera->YawOffset = 25.0f;
    auto Calculate = [Camera](const FRotator& Current, const FVector& Source, const FVector& Target,
                              float Seconds, FRotator& Out)
    {
        return Camera->CalculateRotationDelta(Current, Source, Target, FVector::ZeroVector, Seconds,
                                              Out);
    };
    const FVector Origin = FVector::ZeroVector;
    const FVector Right(0.0, 600.0, 0.0);
    FRotator Delta = FRotator::ZeroRotator;

    // 无World / BeginPlay的组件不能接管输入，失败输出必须清零。
    Delta = FRotator(1.0, 2.0, 3.0);
    TestFalse(TEXT("Unbound context cannot own the camera"),
              Camera->TryMakeRotationDelta(nullptr, .016f, Delta));
    TestTrue(TEXT("Rejected context clears output"), Delta.IsZero());
    TestTrue(TEXT("Valid right target step"),
             Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    TestTrue(TEXT("Yaw approaches the framed angle without snapping"),
             Delta.Yaw > 0.0 && Delta.Yaw < 115.0);
    TestTrue(TEXT("Level target applies negative framing pitch"),
             Delta.Pitch < 0.0 && Delta.Pitch > -25.0);
    TestTrue(TEXT("Camera does not add roll"), Delta.Roll == 0.0);

    const FVector Across = FRotator(0.0, -179.0, 0.0).Vector() * 600.0;
    TestTrue(TEXT("Angle wrap step is valid"),
             Calculate(FRotator(0.0, 179.0, 0.0), Origin, Across, .1f, Delta));
    TestTrue(TEXT("Wrapped yaw plus framing takes the positive short arc"),
             Delta.Yaw > 0.0 && Delta.Yaw < 27.0);

    // 同一静止目标、同一游戏秒；不是用纯规则冒充真实Viewport或运动目标手感。
    auto Simulate = [&](int32 Frames)
    {
        FRotator Current = FRotator::ZeroRotator;
        for (int32 I = 0; I < Frames; ++I)
        {
            FRotator Step = FRotator::ZeroRotator;
            const bool bValid = Calculate(Current, Origin, Right, 1.0f / Frames, Step);
            TestTrue(TEXT("Every simulation step is valid"), bValid);
            Current = (Current + Step).GetNormalized();
        }
        return Current;
    };
    const FRotator At30 = Simulate(30);
    const FRotator At60 = Simulate(60);
    const FRotator At120 = Simulate(120);
    TestTrue(TEXT("30 and 60 subdivisions converge equally"), At30.Equals(At60, .001));
    TestTrue(TEXT("60 and 120 subdivisions converge equally"), At60.Equals(At120, .001));
    TestTrue(TEXT("One second approaches target"), FMath::Abs(At60.Yaw - 115.0) < .05);

    TestTrue(TEXT("High target step"),
             Calculate(FRotator::ZeroRotator, Origin, FVector(600, 0, 10000), 1.0f, Delta));
    TestTrue(TEXT("Desired high pitch is clamped"), Delta.Pitch > 14.9 && Delta.Pitch <= 15.0);
    TestTrue(TEXT("Low target step"),
             Calculate(FRotator::ZeroRotator, Origin, FVector(600, 0, -10000), 1.0f, Delta));
    TestTrue(TEXT("Desired low pitch is clamped"), Delta.Pitch < -44.9 && Delta.Pitch >= -45.0);
    TestTrue(TEXT("Capture from free high pitch"),
             Calculate(FRotator(70, 0, 0), Origin, FVector(600, 0, 0), .016f, Delta));
    TestTrue(TEXT("Capture approaches bounds smoothly"),
             70.0 + Delta.Pitch > 15.0 && Delta.Pitch < 0.0);

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    for (float BadTime : {0.0f, -1.0f, NaN, Infinity})
    {
        Delta = FRotator(1, 2, 3);
        TestFalse(TEXT("Invalid game delta is rejected"),
                  Calculate(FRotator::ZeroRotator, Origin, Right, BadTime, Delta));
        TestTrue(TEXT("Failed step clears output"), Delta.IsZero());
    }
    TestFalse(TEXT("Coincident targets have no yaw"),
              Calculate(FRotator::ZeroRotator, Origin, Origin, .1f, Delta));
    TestFalse(TEXT("Directly vertical targets have no yaw"),
              Calculate(FRotator::ZeroRotator, Origin, FVector(0, 0, 100), .1f, Delta));
    TestFalse(TEXT("Nonfinite current is rejected"),
              Calculate(FRotator(NaN, 0, 0), Origin, Right, .1f, Delta));
    TestFalse(TEXT("Nonfinite source is rejected"),
              Calculate(FRotator::ZeroRotator, FVector(Infinity, 0, 0), Right, .1f, Delta));
    TestFalse(TEXT("Nonfinite target is rejected"),
              Calculate(FRotator::ZeroRotator, Origin, FVector(0, NaN, 0), .1f, Delta));
    Camera->FollowSpeed = 0.0f;
    TestFalse(TEXT("Invalid speed rejected"),
              Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->FollowSpeed = 8.0f;
    Camera->MinPitch = Camera->MaxPitch;
    TestFalse(TEXT("Invalid pitch range rejected"),
              Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->MinPitch = -45.0f;
    Camera->PitchOffset = NaN;
    TestFalse(TEXT("Nonfinite framing offset rejected"),
              Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->PitchOffset = -25.0f;

    // 左右构图均走同一最短角路径；0恢复原同轴算法，不创建额外模式。
    Camera->YawOffset = -25.0f;
    TestTrue(TEXT("Negative framing offset is supported"),
             Calculate(FRotator::ZeroRotator, Origin, Right, 1.0f, Delta));
    TestTrue(TEXT("Negative framing approaches 65 degrees"), Delta.Yaw > 64.9 && Delta.Yaw < 65.0);
    Camera->YawOffset = 0.0f;
    TestTrue(TEXT("Zero offset remains supported"),
             Calculate(FRotator::ZeroRotator, Origin, Right, 1.0f, Delta));
    TestTrue(TEXT("Zero offset approaches original 90 degrees"),
             Delta.Yaw > 89.9 && Delta.Yaw < 90.0);
    for (float BadYaw : {NaN, Infinity, 36.0f, -36.0f})
    {
        Camera->YawOffset = BadYaw;
        Delta = FRotator(1, 2, 3);
        TestFalse(TEXT("Invalid framing offset is rejected"),
                  Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
        TestTrue(TEXT("Invalid framing clears output"), Delta.IsZero());
    }
    Camera->YawOffset = 25.0f;
    // ---- 远景精确注视 / 近景交战轴限幅；近景允许目标略离中心 ----
    Camera->PitchOffset = 0.0f;
    Camera->YawOffset = 0.0f;
    const FVector Focus(600, 0, 45);
    TestTrue(TEXT("Right shoulder geometry solved"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Focus,
                                            Camera->ShoulderOffset, 10.0f, Delta));
    const FRotator View = Delta.GetNormalized();
    const FVector CameraPosition =
        -View.Vector() * 400.0 + FRotationMatrix(View).TransformVector(Camera->ShoulderOffset);
    const FVector FocusInView = View.UnrotateVector(Focus - CameraPosition);
    const FVector PlayerInView = View.UnrotateVector(Origin - CameraPosition);
    TestTrue(TEXT("Right-side camera puts player left of center"), PlayerInView.Y < -70.0);
    TestTrue(TEXT("Focus remains forward and centered horizontally"),
             FocusInView.X > 0.0 && FMath::Abs(FocusInView.Y) < .001);
    TestTrue(TEXT("Height compensation centers the focus"), FMath::Abs(FocusInView.Z) < .001);
    TestTrue(TEXT("Very close target remains finite"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(10, 0, 45),
                                            Camera->ShoulderOffset, .1f, Delta) &&
                 !Delta.ContainsNaN());
    TestFalse(TEXT("Nonfinite socket offset is rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Focus,
                                             FVector(0, NaN, 0), .1f, Delta));

    // 贴脸时保持背后小偏角与俯视；旧算法在70cm可补偿约72度。
    Camera->PitchOffset = -18.0f;
    TestEqual(TEXT("Native downward framing default"),
              GetDefault<UETTargetLockCameraComponent>()->PitchOffset, -18.0f);
    TestEqual(TEXT("Native shoulder yaw cap default"),
              GetDefault<UETTargetLockCameraComponent>()->MaxShoulderYaw, 12.0f);
    for (double Distance : {70.0, 84.0, 100.0, 200.0, 600.0})
    {
        TestTrue(TEXT("Melee framing solves"),
                 Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin,
                                                FVector(Distance, 0, 45), Camera->ShoulderOffset,
                                                10.0f, Delta));
        TestTrue(TEXT("Shoulder compensation stays within combat axis cap"),
                 FMath::Abs(Delta.Yaw) <= 12.001);
        TestTrue(TEXT("Level melee uses moderate downward pitch"),
                 Delta.Pitch >= -35.0 && Delta.Pitch <= -18.0);
        const FVector Position = -Delta.Vector() * 400.0 +
                                 FRotationMatrix(Delta).TransformVector(Camera->ShoulderOffset);
        const FVector InView = Delta.UnrotateVector(FVector(Distance, 0, 45) - Position);
        TestTrue(TEXT("Melee focus remains forward inside a conservative frame"),
                 InView.X > 0.0 && FMath::Abs(InView.Y / InView.X) < .35 &&
                     FMath::Abs(InView.Z / InView.X) < .55);
    }
    for (float InvalidCap : {NaN, Infinity, -1.0f, 31.0f})
    {
        Camera->MaxShoulderYaw = InvalidCap;
        TestFalse(TEXT("Invalid shoulder yaw cap rejected"),
                  Calculate(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
        TestTrue(TEXT("Invalid shoulder cap clears output"), Delta.IsZero());
    }
    Camera->MaxShoulderYaw = 12.0f;

    // ---- 真实Controller / Pawn换控与组件结束；不创建Viewport或冒充视觉验收 ----
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AETPlayerController* PC = World->SpawnActor<AETPlayerController>();
    AEmberTrialCharacter* First =
        World->SpawnActor<AEmberTrialCharacter>(FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    AEmberTrialCharacter* Second =
        World->SpawnActor<AEmberTrialCharacter>(FVector(600, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Controller fixture"), PC) || !TestNotNull(TEXT("First body"), First) ||
        !TestNotNull(TEXT("Second body"), Second))
    {
        return false;
    }
    PC->Possess(First);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UETTargetLockCameraComponent* Rig = PC->FindComponentByClass<UETTargetLockCameraComponent>();
    if (!TestNotNull(TEXT("Single native camera component"), Rig))
    {
        return false;
    }
    USpringArmComponent* FirstBoom = First->GetCameraBoom();
    USpringArmComponent* SecondBoom = Second->GetCameraBoom();
    const FVector FirstBase(11, 12, 13);
    const FVector SecondBase(1, -2, 3);
    FirstBoom->SocketOffset = FirstBase;
    SecondBoom->SocketOffset = SecondBase;
    Rig->UpdateShoulderOffset(FirstBoom, .1f);
    TestTrue(TEXT("Entry smoothly moves toward the right shoulder"),
             FirstBoom->SocketOffset.Y > FirstBase.Y &&
                 FirstBoom->SocketOffset.Y < FirstBase.Y + Rig->ShoulderOffset.Y);
    Rig->UpdateShoulderOffset(FirstBoom, 10.0f);
    Rig->UpdateShoulderOffset(FirstBoom, 10.0f);
    TestTrue(TEXT("Repeated updates never accumulate offset"),
             FirstBoom->SocketOffset.Equals(FirstBase + Rig->ShoulderOffset, .001));
    Rig->ReturnShoulderOffset(.05f);
    TestTrue(TEXT("Unlock return is smooth"), FirstBoom->SocketOffset.Y > FirstBase.Y);
    for (int32 I = 0; I < 30; ++I)
    {
        Rig->ReturnShoulderOffset(.05f);
    }
    TestTrue(TEXT("Unlock restores exact saved base"), FirstBoom->SocketOffset == FirstBase);
    TestFalse(TEXT("Completed return releases old boom"), Rig->ShoulderBoom.IsValid());

    Rig->UpdateShoulderOffset(FirstBoom, .1f);
    Rig->Deactivate();
    TestTrue(TEXT("Deactivate restores exact base immediately"),
             FirstBoom->SocketOffset == FirstBase);
    Rig->Activate(false);
    Rig->UpdateShoulderOffset(FirstBoom, .1f);
    PC->Possess(Second);
    TestTrue(TEXT("Real possession restores old body's boom"),
             FirstBoom->SocketOffset == FirstBase);
    TestFalse(TEXT("New body cannot inherit old shoulder ownership"), Rig->ShoulderBoom.IsValid());

    Rig->UpdateShoulderOffset(SecondBoom, .1f);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pause fixture"), Pauser))
    {
        return false;
    }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    const FVector PausedOffset = SecondBoom->SocketOffset;
    TestFalse(TEXT("Paused camera does not own rotation"),
              Rig->TryMakeRotationDelta(PC->GetTargetLockComponent(), .5f, Delta));
    TestTrue(TEXT("Pause freezes shoulder interpolation"),
             SecondBoom->SocketOffset == PausedOffset);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    Rig->Deactivate();
    Rig->Activate(false);
    Rig->UpdateShoulderOffset(SecondBoom, .1f);
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay releases boom reference"), Rig->ShoulderBoom.IsValid());
    TestTrue(TEXT("EndPlay restores second body's exact base"),
             SecondBoom->SocketOffset == SecondBase);
    return !HasAnyErrors();
}
#endif
```

## 4. 两个既有Controller文件：四处修改

### 4.1 头文件：前置声明

打开`EmberTrial/Public/Framework/ETPlayerController.h`。在现有`class UETTargetLockComponent;`下一行加入：

```cpp
class UETTargetLockCameraComponent;
```

这不是#include，不能放到类大括号内；原有generated.h仍为最后一条#include。

### 4.2 头文件：覆写声明与组件属性

1. 找到`protected:`，在已有`virtual void SetupInputComponent() override;`下一行加入：

```cpp
virtual void UpdateRotation(float DeltaTime) override;
```

2. 找到类末尾`private:`现有`void HandleTargetLockStarted();`，在它之后、类最后的`};`之前加入：

```cpp
// 镜头规则不进入UI，也不让Character的动作状态接管相机所有权。
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Target Lock Camera",
          meta = (AllowPrivateAccess = "true"))
TObjectPtr<UETTargetLockCameraComponent> TargetLockCameraComponent;
```

不要再声明一份TargetLockComponent；带Camera的是本次新组件，原目标身份组件保留。

### 4.3 cpp：一条include与构造函数

打开`EmberTrial/Private/Framework/ETPlayerController.cpp`，现有TargetLock头下一行加入：

```cpp
#include "TargetLock/ETTargetLockCameraComponent.h"
```

找到已有构造函数`AETPlayerController::AETPlayerController()`，**只替换这个函数**为：

```cpp
AETPlayerController::AETPlayerController()
{
    TargetLockComponent = CreateDefaultSubobject<UETTargetLockComponent>(TEXT("TargetLock"));
    TargetLockCameraComponent =
        CreateDefaultSubobject<UETTargetLockCameraComponent>(TEXT("TargetLockCamera"));
}
```

两个子对象名是不同身份；不要在蓝图Add Component里再加一份Camera。

### 4.4 cpp：新增完整UpdateRotation

在构造函数结束后、已有GetTargetLockComponent定义之前插入以下函数。文件原来没有UpdateRotation；若已经加入过，就核对仅一份。

```cpp
void AETPlayerController::UpdateRotation(float DeltaTime)
{
    FRotator LockDelta = FRotator::ZeroRotator;
    if (IsValid(TargetLockCameraComponent.Get()) &&
        TargetLockCameraComponent->TryMakeRotationDelta(TargetLockComponent.Get(), DeltaTime,
                                                         LockDelta))
    {
        // 锁定时接管本帧Yaw / Pitch；解除后自然沿用鼠标输入，不保存旧输入或旧角度。
        RotationInput = LockDelta;
    }
    // 保留引擎CameraManager约束与原生控制流程，只调用一次。
    Super::UpdateRotation(DeltaTime);
}
```

不用改SetupInputComponent、HandleTargetLockStarted、BeginPlay、OnPossess / EndPlay、Pause桥接或ET_Stamina / ET_Healing。Controller里不调用SetActorRotation / SetIgnoreLookInput；不创建另一条Look输入绑定。

## 5. 正常构建与CLion索引

UE保持关闭，PowerShell：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

等待Editor Build成功后，在CLion对原EmberTrial工程执行Reload CMake Project / 重新加载CMake项目，仍选已有CodeModel配置，不单独打开新cpp作为工程。预期Core CodeModel **59cpp**、UI23、Editor3；新增2cpp，不是3cpp。三个新文件含一个头文件；首次B2实施已从153 Source / 83cpp达到**156 / 85**，测试21→22，来源73 / UE资产292。本次构图补修不增加文件或测试数量；只跑更新后的同一专项。

接着在同一PowerShell执行Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

若Error，先停在此处，发第一条具体编译 / 链接错误和文件行号；单独的UBT exit code 6不是根因。不删缓存、不添加Core→UI依赖。

## 6. UE组件与旧相机设置：逐项核对

### 6.1 原Controller新增原生Camera组件

1. 双构建成功后打开工程，进入正式地图`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。
2. Content Browser打开**已有**`/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`。
3. 左侧Components中找到带继承标识的**TargetLockCamera**。这是新C++构造函数带来的原生组件，不在Add里手动添加。如果没有，先确认是正常关闭UE后的Build、当前BP父类仍ETPlayerController，然后关闭重开该资产。
4. 单击TargetLockCamera，在Details清空搜索后，搜索`Follow Speed`。展开Ember Trial / Target Lock Camera，设置或核对：

| 字段 | 值 | 含义 |
| --- | ---: | --- |
| Follow Speed | 8.0 | 角度指数收敛速度 |
| Min Pitch | -45.0 | 期望俯仰下限，度 |
| Max Pitch | 15.0 | 期望俯仰上限，度 |
| Pitch Offset | -18.0 | 解析视线后的额外俯视，度 |
| Max Shoulder Yaw | 12.0 | 肩位补偿偏航上限，度；近景允许目标略离中心 |
| Yaw Offset | 0.0 | 肩位已参与求解，额外偏航微调 |
| Shoulder Offset X / Y / Z | 0 / 75 / 55 | 局部厘米偏移，+Y为右肩 |
| Target Focus Height | 45.0 | 目标中心上方注视点，厘米 |
| Activation → Auto Activate | True | 开始时提供镜头规则 |

5. 该组件没有自己的Tick；不用打开Start with Tick Enabled / Tick Even when Paused。原TargetLock、TargetLockMarker、UIBootstrap、FailureFlow各一份；原六项锁定数值、Marker24 / 24与WBP Class、五HUD引用全部保留。
6. 不加Blueprint Tick / 鼠标事件 / 控制旋转图。对Controller点Compile → Save。

### 6.2 原玩家相机只核对，数值不改

打开`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`；如果浏览器目录大小写不同，按资产名定位原玩家，不新建。

1. Components选择**CameraBoom**。Details搜索`Use Pawn Control Rotation`：True；清空搜索后看Target Arm Length仍400，Do Collision Test保持原默认。不要同时启用新的Camera Rotation Lag / Camera Lag，本批只保留原配置。
2. Components选择**FollowCamera**。Use Pawn Control Rotation=False，组件Active保持True；镜头仍接在CameraBoom上。
3. 点工具栏**Class Defaults**。分别搜索`Use Controller Rotation Pitch`、`Use Controller Rotation Yaw`、`Use Controller Rotation Roll`，三项均False；CharacterMovement的Orient Rotation to Movement保持True。
4. 若原来已全部正确，**不需要改动、重存玩家蓝图**。若发现实物不同，先记录字段和原值，再仅纠正上述契约，Compile / Save，并在反馈说明；这将是额外资产差异，后续清单按实物更新。

## 7. 分组短查：8–12分钟

### 7.1 先一项新规则专项

非PIE状态打开Output Log，在底部命令栏输入：

```text
Automation RunTests EmberTrial.M2.TargetLock.CameraRules
```

看到`Test Completed. Result={Success}`后进入实机。不跑旧21项全组；本批不改A目标选择或B1投影规则。专项覆盖原角度 / 有限值 / 等时长规则、右肩视线、贴身有限值，以及真实世界中的肩位所有权 / 归还 / 换控 / Pause / EndPlay；不写成真实Viewport / Mesh遮挡自动通过。

### 7.2 自由 / 锁定 / 解锁与目标移动（约3分钟）

1. Play，使用原先熟悉的PIE方式，在反馈注明Selected Viewport或New Editor Window。先不锁定，鼠标左右 / 上下移动应和之前一样。
2. 让敌人在镜头前方可见范围内，站在数百cm处按一次Tab。菱形出现，镜头逐渐对准敌人；不要用贴脸重叠作为首个判断样本。
3. 让敌人追来，或按A / D短移动形成相对位置变化。镜头持续跟随目标，菱形无明显一帧拖后；接近正后方向时不出现整圈绕转。靠近旧墙走一小段，SpringArm应仍能缩进，别穿过挡墙。
4. 持续锁定时移动鼠标：Yaw / Pitch不被抢走是预期。Tab解除后立即移动鼠标：从当前镜头恢复自由，没有回到旧角度的跳变，也不需要再点视口。
5. 再锁一次，分别站定左键、2闪避或F任选两种动作；镜头可以转，角色动作方向没有被每帧强拉，闪避仍固定采样方向且只扣一次体力。若背对敌人，先走一步转身再站定，不把本批误验为自动攻击转向。

### 7.3 相机失活不清Core、暂停与恢复（约2分钟）

保持锁定，打开控制台，依次执行，每条后关闭控制台观察：

```text
ke ETTargetLockCameraComponent Deactivate
```

镜头变回自由鼠标，但Core仍持有原目标、B1标记仍存在（目标若被自由镜头甩到屏外会隐藏）。不要按Tab，否则会主动解除Core。再执行：

```text
ke ETTargetLockCameraComponent Activate false
```

镜头从当前位置重新平滑跟随同一目标。此过程不应出现新的Core Locked / Cleared；本组件没有循环日志，不能要求凭一条Camera Active日志确认。

按P暂停，在菜单中移动鼠标约2秒，再鼠标点击继续；镜头未在暂停中追随 / 漂移，继续后无堆积视角。再用P→键盘继续一轮即可，不重做全部Pause矩阵；Tab菜单内不补执行，原鼠标焦点恢复正常。

### 7.4 释放与短基线（约3–5分钟）

1. 保持锁定，按Tab主动解除并自由转镜头；再锁定后击杀敌人一次，目标死亡后菱形消失、镜头回自由。敌人仍使用原100生命与10伤害，不临时改数值。
2. 新一轮锁定后，使用已有4开发伤害造成一次Failure，Retry后新双100、相机自由且无旧目标；不需要重建任何Controller测试图。P返回菜单→Continue后仍是新Controller的自由镜头，重新Tab才锁。
3. 锁定中Stop，重新Play，默认未锁、五HUD与标记各自正常。只做一次左键 / 2 / E / Q或F、P与移动转向跳跃短基线；不重复旧全部自动化 / 伤害矩阵。
4. Stop后核对TargetLockCamera AutoActivate=True、速度 / 角度8 / -45 / 15 / -18 / 0，MaxShoulderYaw12，ShoulderOffset0 / 75 / 55、TargetFocusHeight45；Marker的AutoActivate与24 / 24保持、slomo=1、Debug False。Save All；没有临时配置可写“未改”。

若平地同高的镜头上下抖动，给5–10秒视频和动作顺序；若只在贴墙发生，说明墙位置与是否SpringArm缩短。先记录，不为了消抖关闭碰撞或改全部镜头数值。

## 8. 当前状态与后续试玩

B2基础功能由用户完成，本次右肩补修由助手直接完成：无需再手动录入第3节或第4节代码，也无需重复整章验收。Editor / Game Development、CLion元数据和三项专项均已通过。真实Selected Viewport中看到了玩家偏左 / 敌人清楚、标记 / P冻结与键盘继续、组件正式入口解除后回正；实际取景截图保存在本地Artifacts。

### 8.1 右肩补修最终配置与操作记录

| 项目 | 当前值 / 结果 |
| --- | --- |
| Follow Speed / Min Pitch / Max Pitch | 8 / -45 / 15 |
| Pitch Offset / Yaw Offset | 当前-18 / 0；原右肩历史值0 / 0由C收尾补修替代 |
| Max Shoulder Yaw | 12°，限制贴身绕到侧面 |
| Shoulder Offset | X0、Y75、Z55cm |
| Target Focus Height | 45cm |
| Auto Activate / 原臂长 | True / 400cm |
| Editor / Game / 三模块索引 | 助手正常构建成功 / 已刷新 |
| CameraRules / Selection / MarkerRules | 2026-10-24 15:01:15三项Success |
| 原Controller | computer-use已核对上述继承值，Compile → Save；无额外相机组件 |
| 实机截图 | Artifacts/M2_06B2_RightShoulder/RightShoulder_PIE.png，1359×754 |
| 临时设置 | 本轮用slomo .05与运行时Teleport便于观察，已slomo1并Stop，地图 / 出生点未改 |

当前相机保持动作游戏所需的全身空间；以后觉得过近 / 过远或偏移不足，反馈5–10秒视频和实际距离 / 动作即可，再按单一变量调整。贴墙时原SpringArm碰撞会优先缩短相机距离，不把普通墙边遮挡宣传成完全消除。此次没有新增包或性能采样。

本次电脑工具短按Tab未形成可靠的Enhanced Input Started样本，实机镜头使用已有`ke ETTargetLockComponent ToggleTargetLock`正式组件入口请求 / 解除；没有硬写目标引用或绕过合法性过滤，不另宣称完成一次新的物理Tab验收。原用户B2的Tab / 输入反馈沿用，代码未改输入映射。后续试玩仍按正式Tab操作即可。

本轮不要求新的完整回报模板。若继续微调，只报具体画面问题、PIE方式和额外手动时间；原90分钟不重复登记。右肩补修已交付；随后用户要求进入下一章，[C方向切换指南已实施](M2_06C_TargetSwitching.md)，本轮收尾镜头 / 出生补修与提交范围见C10.1 / 11节，B2源码以本指南为准。

## 9. 本章 Commit / Push（固定放在最后）

基础B2在63abd20，之后右肩 / C / 近景与出生补修已合并在本地`2b62e6a`。完整历史范围见[C末尾](M2_06C_TargetSwitching.md#11-本章-commit--push固定放在最后)，不要重复Stage未变Source或重存资产。

当前新增C / M2-06报告与M2-07A准备材料，范围见[A末尾](M2_07A_IntegratedPackage.md#9-本章-commit--push固定放在最后)。下发态英文`docs: prepare M2 package integration checks`，中文译文`docs: 准备M2阶段包内集成检查`。

用户自行Stage / Review Diff / Commit / Push，助手未操作索引。Raw / 日志 / 截图 / 采样脚本 / Saved / Binaries / Intermediate / CMake索引 / 包体留本地，不Force Add。
