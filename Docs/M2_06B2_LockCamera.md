# M2-06B2：锁定镜头跟随

2026-10-24，UE5.5.4。前置[B1标记验收通过](Reports/M2_06B1_Verification.md)，[锁定路线](M2_06_TargetLockRoadmap.md)。**本章待用户手动实施**：三新Core文件、两旧Controller文件；零UI源码、零新资产。候选仅放忽略的Raw目录，实际Source、UE资产与构建由你操作。

手动预计80–130分钟：代码55–90、配置10–20、人工短查8–12；构建等待另记。本批没有新动画 / 美术 / 插件，不增加临时Blueprint测试函数，不进入C的双目标切换。

## 1. 做完后应看到什么

未锁定：鼠标和原来一样自由转镜头。Tab锁定：镜头平滑转向Core已有敌人，敌人移动时继续跟随；Yaw走最短角路径，不绕一整圈。水平同高时期望Pitch=-12°，高低目标加入两身体中心连线的仰角，再把期望Pitch限制在-35°至15°。旧SpringArm仍处理相机碰撞、原距离400cm不改，B1菱形仍在相机更新后投影。

**锁定期间Yaw / Pitch由跟随接管，鼠标转向输入当帧被覆盖，不留队列。**再按Tab解除后从当前位置立即恢复自由鼠标，不跳回锁定前角度。这是本章明确的输入规则，不是“鼠标失灵”；需要自由观察时先Tab解锁。P菜单继续使用现有焦点 / 输入清理；暂停不推进镜头，菜单移动鼠标不会在恢复后积攒一串视角变化。

只跟随相机，**不自动转Character、修改移动模式或替攻击瞄准**。玩家仍按当前控制Yaw进行WASD，角色朝移动方向；攻击 / 闪避方向仍由原能力在既有时机采样 / 固定。动作中相机可以跟随，角色和RootMotion方向不能被镜头每帧拉向敌人。C再统一检查锁定交战与切换，不在此修改四能力。站在背对目标的姿态原地挥空，是本批不带自动起手转向的预期。

进入锁定时从当前自由角度平滑收敛；-35° / 15°约束的是**期望角度**，最初几帧允许当前自由角度仍在范围外，避免突然跳镜头。指数插值参数8是收敛速度，不是8度/秒；静止目标的30 / 60 / 120等时长结果由规则测试比较，真实运动目标 / SpringArm碰撞手感仍需实机。

## 2. 架构、文件与保存关闭

Controller新增原生子对象TargetLockCamera；相机组件不建Tick / Timer / 委托、不缓存目标，每次由Controller原本的UpdateRotation调用一次。它只把Core目标转换成旋转增量；Controller把本帧RotationInput替换为增量后，仍让Super::UpdateRotation执行CameraManager约束、设置ControlRotation和原生FaceRotation流程。原角色三个Use Controller Rotation保持False，因此该流程不额外转身体。

本地来源不合法、没有目标、相机组件失活、换控 / 死亡 / World结束 / 暂停 / 视点不再是当前身体时，不接管输入。没有临时Target缓存或需恢复的Look Ignore计数；解锁没有旧角度快照可回跳。无锁仅做少量资格检查，不查询候选 / 射线 / GAS，本批无新性能采样，不宣称帧率提升。

1. Stop PIE → Save All → 关闭UE编辑器，保存CLion文件。
2. 以下三个**新文件均属于Core EmberTrial**。路径相对`E:\UE_Project\Demo\EmberTrial\Source`；与B1的UI测试不同，这次测试正确位置就是Core/Tests。

| 新文件 | 完整代码 | 职责 |
| --- | --- | --- |
| EmberTrial/Public/TargetLock/ETTargetLockCameraComponent.h | 3.1 | 镜头配置与接口 |
| EmberTrial/Private/TargetLock/ETTargetLockCameraComponent.cpp | 3.2 | 本帧资格、目标角与插值 |
| EmberTrial/Tests/ETTargetLockCameraTests.cpp | 3.3 | 有限值、短角路径、帧率与Pitch规则 |

3. 两个既有文件只按第4节增加内容：`EmberTrial/Public/Framework/ETPlayerController.h`与`EmberTrial/Private/Framework/ETPlayerController.cpp`。不要重建同名Controller，不删现有输入、Pause委托和资源诊断。
4. 不修改Build.cs / Target / uproject、A锁定组件、B1六份UI、Character、GA、IMC或地图。全部保存后再第5节正常Build，不用Live Coding。

## 3. 三个新文件的完整代码

### 3.1 ETTargetLockCameraComponent.h

```cpp
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
```

### 3.3 ETTargetLockCameraTests.cpp

此测试不创建真实相机、World或Viewport；不替代第7节的鼠标、暂停、墙边碰撞与生命周期短查。使用TestTrue比较浮点容差，避免先前float / double的TestEqual重载歧义。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "TargetLock/ETTargetLockCameraComponent.h"
#include "Misc/AutomationTest.h"
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
    const FVector Origin = FVector::ZeroVector;
    const FVector Right(0.0, 600.0, 0.0);
    FRotator Delta = FRotator::ZeroRotator;

    // 无World / BeginPlay的组件不能接管输入，失败输出必须清零。
    Delta = FRotator(1.0, 2.0, 3.0);
    TestFalse(TEXT("Unbound context cannot own the camera"),
              Camera->TryMakeRotationDelta(nullptr, .016f, Delta));
    TestTrue(TEXT("Rejected context clears output"), Delta.IsZero());
    TestTrue(TEXT("Valid right target step"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    TestTrue(TEXT("Yaw turns toward target without snapping"), Delta.Yaw > 0.0 && Delta.Yaw < 90.0);
    TestTrue(TEXT("Level target applies negative framing pitch"),
             Delta.Pitch < 0.0 && Delta.Pitch > -12.0);
    TestTrue(TEXT("Camera does not add roll"), Delta.Roll == 0.0);

    const FVector Across = FRotator(0.0, -179.0, 0.0).Vector() * 600.0;
    TestTrue(TEXT("Angle wrap step is valid"),
             Camera->CalculateRotationDelta(FRotator(0.0, 179.0, 0.0), Origin, Across, .1f, Delta));
    TestTrue(TEXT("179 to -179 takes positive shortest arc"), Delta.Yaw > 0.0 && Delta.Yaw < 2.0);

    // 同一静止目标、同一游戏秒；不是用纯规则冒充真实Viewport或运动目标手感。
    auto Simulate = [&](int32 Frames)
    {
        FRotator Current = FRotator::ZeroRotator;
        for (int32 I = 0; I < Frames; ++I)
        {
            FRotator Step = FRotator::ZeroRotator;
            const bool bValid =
                Camera->CalculateRotationDelta(Current, Origin, Right, 1.0f / Frames, Step);
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
    TestTrue(TEXT("One second approaches target"), FMath::Abs(At60.Yaw - 90.0) < .04);

    TestTrue(TEXT("High target step"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(600, 0, 10000),
                                            1.0f, Delta));
    TestTrue(TEXT("Desired high pitch is clamped"), Delta.Pitch > 14.9 && Delta.Pitch <= 15.0);
    TestTrue(TEXT("Low target step"),
             Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(600, 0, -10000),
                                            1.0f, Delta));
    TestTrue(TEXT("Desired low pitch is clamped"), Delta.Pitch < -34.9 && Delta.Pitch >= -35.0);
    TestTrue(TEXT("Capture from free high pitch"),
             Camera->CalculateRotationDelta(FRotator(70, 0, 0), Origin, FVector(600, 0, 0), .016f,
                                            Delta));
    TestTrue(TEXT("Capture approaches bounds smoothly"),
             70.0 + Delta.Pitch > 15.0 && Delta.Pitch < 0.0);

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();
    for (float BadTime : {0.0f, -1.0f, NaN, Infinity})
    {
        Delta = FRotator(1, 2, 3);
        TestFalse(
            TEXT("Invalid game delta is rejected"),
            Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, BadTime, Delta));
        TestTrue(TEXT("Failed step clears output"), Delta.IsZero());
    }
    TestFalse(TEXT("Coincident targets have no yaw"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Origin, .1f, Delta));
    TestFalse(TEXT("Directly vertical targets have no yaw"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(0, 0, 100), .1f,
                                             Delta));
    TestFalse(TEXT("Nonfinite current is rejected"),
              Camera->CalculateRotationDelta(FRotator(NaN, 0, 0), Origin, Right, .1f, Delta));
    TestFalse(TEXT("Nonfinite source is rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, FVector(Infinity, 0, 0), Right,
                                             .1f, Delta));
    TestFalse(TEXT("Nonfinite target is rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, FVector(0, NaN, 0), .1f,
                                             Delta));
    Camera->FollowSpeed = 0.0f;
    TestFalse(TEXT("Invalid speed rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->FollowSpeed = 8.0f;
    Camera->MinPitch = Camera->MaxPitch;
    TestFalse(TEXT("Invalid pitch range rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    Camera->MinPitch = -35.0f;
    Camera->PitchOffset = NaN;
    TestFalse(TEXT("Nonfinite framing offset rejected"),
              Camera->CalculateRotationDelta(FRotator::ZeroRotator, Origin, Right, .1f, Delta));
    return true;
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

等待Editor Build成功后，在CLion对原EmberTrial工程执行Reload CMake Project / 重新加载CMake项目，仍选已有CodeModel配置，不单独打开新cpp作为工程。预期Core CodeModel **59cpp**、UI23、Editor3；新增2cpp，不是3cpp。三个新文件含一个头文件；整体将从153 Source / 83cpp变为**156 / 85**，测试21→22，来源仍73 / UE资产仍292。这些是实施后的预期，当前未计完成。

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
| Min Pitch | -35.0 | 期望俯仰下限，度 |
| Max Pitch | 15.0 | 期望俯仰上限，度 |
| Pitch Offset | -12.0 | 同高时基础俯视偏移，度 |
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

看到`Test Completed. Result={Success}`后进入实机。不跑旧21项全组；本批不改A目标选择或B1投影规则。专项覆盖默认角度、179°到-179°最短路径、30 / 60 / 120等时长、期望Pitch上下限、入锁时平滑捕获、非法时间 / 数值 / 配置与未绑定拒绝；不写成真实Viewport或全部生命周期自动通过。

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
4. Stop后核对TargetLockCamera AutoActivate=True、四数值8 / -35 / 15 / -12；Marker的AutoActivate与24 / 24保持、slomo=1、Debug False。Save All；没有临时配置可写“未改”。

若平地同高的镜头上下抖动，给5–10秒视频和动作顺序；若只在贴墙发生，说明墙位置与是否SpringArm缩短。先记录，不为了消抖关闭碰撞或改全部镜头数值。

## 8. 回报模板与当前证据边界

```text
M2-06B2 完成
三新 / 两旧Source；关闭UE后Editor Build / Core59-UI23-Editor3索引 / Game Build：
Controller唯一继承TargetLockCamera、8 / -35 / 15 / -12 / AutoActivate；旧组件保持、Compile-Save：
原CameraBoom / FollowCamera / 三Controller Rotation与角色移动设置核对：
TargetLock.CameraRules一次：
自由鼠标 / Tab平滑跟随 / 敌人移动 / 解锁无回跳 / 墙边碰撞：
锁定鼠标不争抢 / 动作方向保持 / Camera失活与恢复时Core目标不变：
P鼠标与键盘继续无积攒 / 敌人死亡释放 / Failure-Retry与Continue新自由：
锁定中Stop再开 / 旧HUD与能力-E-P-移动跳跃短基线：
实际PIE方式 / 最明显镜头问题（无填无）：
Source资产Save All；默认值与临时设置恢复或未改：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

只给完成时分，日志由我提取。编译给首条Error与文件 / 行号及前后20行；组件设置给资产名 / Components / Details完整截图；镜头动态问题优先5–10秒视频并说明锁定、按键顺序与是否贴墙。C的目标切换尚未实现，当前只有单目标镜头；本轮不Cook，也不把旧包当作本功能实机证据。

## 9. 本章 Commit / Push（固定放在最后）

**B2手动实施且验收通过后**Stage三新Source与两旧Controller，前缀均为EmberTrial/Source/：

```text
EmberTrial/Public/TargetLock/ETTargetLockCameraComponent.h
EmberTrial/Private/TargetLock/ETTargetLockCameraComponent.cpp
EmberTrial/Tests/ETTargetLockCameraTests.cpp
EmberTrial/Public/Framework/ETPlayerController.h
EmberTrial/Private/Framework/ETPlayerController.cpp
```

UE资产预计只有原`EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset`保存Camera配置；新组件来自原生，不创建新uasset。如果6.2纠正过玩家相机设置，额外包含实际BP_ThirdPersonCharacter差异并说明，不制造其他重存。B2不改UI源码 / WBP / 输入 / GA / 地图 / Config，来源数量预计仍73，仅实际变化资产更新hash。

文档：本指南、完成后B2报告、Progress / Backlog / Architecture / 方案B / VersionControl / 两份M2路线 / README / Introduction和实际来源更新。尚未提交的B1六新UI、WBP / Controller、B1报告 / 清单与A报告按[B1末尾](M2_06B1_TargetMarker.md#12-本章-commit--push固定放在最后)一并检查：旧Core/Tests误放路径先取消暂存，再Stage正确UI/Private/Tests；已Stage的空骨架需要重新Stage最新完整实现。

建议Commit（英文）：`feat: follow the locked target with a smooth camera`。

中文译文：`feat: 新增锁定目标的平滑镜头跟随`。

英文可作标题，中文作正文；用户操作CLion Refresh Changes → Stage最新内容 → Review Diff → Commit → Push，助手不代Git。Raw / 日志 / Saved / Binaries / Intermediate / CMake索引 / 包体留本地，不Force Add。未实施的候选代码不提前创建空Source / Stage为完成。
