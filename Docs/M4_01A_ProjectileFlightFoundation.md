# M4-01A 远程守卫投射物飞行基础

2026-11-10已验收。三份Core文件已提交，双Build、索引、飞行专项和地图短查通过。实际手动40分钟，构建等待5分钟。以下保留实现步骤。[A验收](Reports/M4_01A_Verification.md)

## 1. 交付边界

M4先完成远程守卫，再推进混合遭遇、守关敌人与完整关卡路线。本批交付原生飞行、第一次阻挡命中和生命周期。M4-01B接入真实GAS伤害与阵营／无敌规则，后续再配置远程能力、AI、可视表现和关卡实例。

| 情况 | 本批行为 |
| --- | --- |
| 生成 | 移动组件未激活，投射物保持静止且球体无碰撞 |
| 发射 | `TryLaunch`接收同世界有效来源与非零世界方向，一次成功后封住重复请求。当前默认1600 cm/s、球半径10 cm、寿命3个游戏秒 |
| 移动与阻挡 | 发射后球体执行Sweep，忽略发射者，对WorldStatic、WorldDynamic和Pawn设置阻挡响应。实际是否阻挡取决于双方的碰撞响应 |
| 命中 | 第一次阻挡即关闭碰撞，再广播`OnImpact`。监听者可在回调读取弱引用来源，允许其同步销毁投射物 |
| 清理 | 阻挡命中后销毁，超时静默销毁，EndPlay解绑。移动和寿命沿用世界游戏时间，Actor本身无Tick |

## 2. 创建与粘贴

保存并关闭UE编辑器。在CLion项目树的`EmberTrial/Source/EmberTrial`下，按下列位置直接新建文件。`Public/Combat`、`Private/Combat`、`Tests`都已存在。每个文件粘贴对应的完整代码。`.generated.h`由UHT在正常构建中生成。

### 2.1 `ETCombatProjectile.h`

完整路径 `EmberTrial/Source/EmberTrial/Public/Combat/ETCombatProjectile.h`。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETCombatProjectile.generated.h"

class AETCombatProjectile;
class UProjectileMovementComponent;
class USphereComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETCombatProjectileImpact, AETCombatProjectile*, const FHitResult&);

// 一次性投射物。调用者决定何时发射以及如何结算命中。
UCLASS(Blueprintable)
class EMBERTRIAL_API AETCombatProjectile : public AActor
{
    GENERATED_BODY()

public:
    AETCombatProjectile();

    bool TryLaunch(AActor* InSource, const FVector& WorldDirection);
    AActor* GetSourceActor() const { return SourceActor.Get(); }

    FETCombatProjectileImpact OnImpact;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleProjectileStopped(const FHitResult& Hit);

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "1.0", Units = "cm/s"))
    float LaunchSpeed = 1600.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Projectile", meta = (ClampMin = "0.01", Units = "s"))
    float FlightLifetime = 3.0f;

    TWeakObjectPtr<AActor> SourceActor;
    bool bLaunched = false;
    bool bResolved = false;
};
```

### 2.2 `ETCombatProjectile.cpp`

完整路径 `EmberTrial/Source/EmberTrial/Private/Combat/ETCombatProjectile.cpp`。

```cpp
#include "Combat/ETCombatProjectile.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"

AETCombatProjectile::AETCombatProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(10.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->SetGenerateOverlapEvents(false);
    Collision->SetCanEverAffectNavigation(false);
    SetRootComponent(Collision);

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->SetUpdatedComponent(Collision);
    ProjectileMovement->bAutoActivate = false;
    ProjectileMovement->bInitialVelocityInLocalSpace = false;
    ProjectileMovement->ProjectileGravityScale = 0.0f;
    ProjectileMovement->bSweepCollision = true;
    ProjectileMovement->bShouldBounce = false;
    ProjectileMovement->bRotationFollowsVelocity = true;
}

void AETCombatProjectile::BeginPlay()
{
    Super::BeginPlay();
    ProjectileMovement->OnProjectileStop.AddUniqueDynamic(this, &ThisClass::HandleProjectileStopped);
}

bool AETCombatProjectile::TryLaunch(AActor* InSource, const FVector& WorldDirection)
{
    UWorld* World = GetWorld();
    if (!HasAuthority() || !HasActorBegunPlay() || IsActorBeingDestroyed() || bLaunched || bResolved ||
        !World || World->IsPaused() || !IsValid(InSource) || InSource == this ||
        InSource->IsActorBeingDestroyed() || InSource->GetWorld() != World ||
        WorldDirection.ContainsNaN() || !FMath::IsFinite(LaunchSpeed) || LaunchSpeed < 1.0f ||
        !FMath::IsFinite(FlightLifetime) || FlightLifetime < 0.01f)
    {
        return false;
    }
    const FVector Direction = WorldDirection.GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        return false;
    }

    bLaunched = true;
    SourceActor = InSource;
    Collision->IgnoreActorWhenMoving(InSource, true);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProjectileMovement->Velocity = Direction * LaunchSpeed;
    ProjectileMovement->Activate();
    SetLifeSpan(FlightLifetime);
    return true;
}

void AETCombatProjectile::HandleProjectileStopped(const FHitResult& Hit)
{
    if (!bLaunched || bResolved || IsActorBeingDestroyed())
    {
        return;
    }
    // 先封住本次飞行，再通知可能同步销毁投射物的命中监听者。
    bResolved = true;
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Hit.bBlockingHit)
    {
        OnImpact.Broadcast(this, Hit);
    }
    if (!IsActorBeingDestroyed())
    {
        Destroy();
    }
}

void AETCombatProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ProjectileMovement->OnProjectileStop.RemoveDynamic(this, &ThisClass::HandleProjectileStopped);
    OnImpact.Clear();
    Super::EndPlay(EndPlayReason);
}
```

### 2.3 `ETCombatProjectileTests.cpp`

完整路径 `EmberTrial/Source/EmberTrial/Tests/ETCombatProjectileTests.cpp`。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ETCombatProjectile.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETCombatProjectileFlightTest,
    "EmberTrial.M4.Combat.ProjectileFlight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETCombatProjectileFlightTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    auto SpawnBlocker = [World](const FVector& Position, const FVector& Extent,
                                const ECollisionChannel Channel) -> AActor*
    {
        AActor* Blocker = World->SpawnActor<AActor>();
        if (!IsValid(Blocker))
        {
            return nullptr;
        }
        UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
        Blocker->SetRootComponent(Box);
        Box->InitBoxExtent(Extent);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionObjectType(Channel);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->SetGenerateOverlapEvents(false);
        Box->RegisterComponent();
        Blocker->SetActorLocation(Position);
        return Blocker;
    };
    AActor* Source = SpawnBlocker(FVector::ZeroVector, FVector(40.0), ECC_Pawn);
    AActor* Wall = SpawnBlocker(FVector(400.0, 0.0, 0.0), FVector(3.0, 80.0, 80.0), ECC_WorldStatic);
    AETCombatProjectile* HitProjectile = World->SpawnActor<AETCombatProjectile>();
    AETCombatProjectile* MissProjectile = World->SpawnActor<AETCombatProjectile>(
        FVector(-200.0, 0.0, 0.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Source collision at launch origin"), Source) ||
        !TestNotNull(TEXT("Thin blocking wall"), Wall) ||
        !TestNotNull(TEXT("Wall projectile"), HitProjectile) ||
        !TestNotNull(TEXT("Miss projectile"), MissProjectile) ||
        !Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    const FVector Origin = HitProjectile->GetActorLocation();
    TestFalse(TEXT("Reject null source"), HitProjectile->TryLaunch(nullptr, FVector::ForwardVector));
    TestFalse(TEXT("Reject zero direction"), HitProjectile->TryLaunch(Source, FVector::ZeroVector));
    if (!Fixture.TickTestWorld(0.10f))
    {
        return false;
    }
    TestTrue(TEXT("Unlaunched projectile remains inert"), HitProjectile->GetActorLocation().Equals(Origin));

    int32 WallImpacts = 0;
    int32 MissImpacts = 0;
    AActor* FirstHit = nullptr;
    AActor* ReportedSource = nullptr;
    bool bRelaunchFromImpact = false;
    HitProjectile->OnImpact.AddLambda([&](AETCombatProjectile* Projectile, const FHitResult& Hit)
    {
        ++WallImpacts;
        FirstHit = Hit.GetActor();
        ReportedSource = Projectile->GetSourceActor();
        bRelaunchFromImpact = Projectile->TryLaunch(Source, FVector::RightVector);
    });
    MissProjectile->OnImpact.AddLambda([&](AETCombatProjectile*, const FHitResult&) { ++MissImpacts; });

    const TWeakObjectPtr<AETCombatProjectile> HitRef(HitProjectile);
    const TWeakObjectPtr<AETCombatProjectile> MissRef(MissProjectile);
    if (!TestTrue(TEXT("Launch overlapping source"), HitProjectile->TryLaunch(Source, FVector::ForwardVector)) ||
        !TestTrue(TEXT("Launch down the clear lane"), MissProjectile->TryLaunch(Source, FVector::RightVector)))
    {
        return false;
    }
    TestFalse(TEXT("Reject second launch"), HitProjectile->TryLaunch(Source, FVector::RightVector));
    if (!Fixture.TickTestWorld(0.10f) ||
        !TestTrue(TEXT("First projectile clears its source and is in flight"), HitRef.IsValid()) ||
        !TestTrue(TEXT("Second projectile is in flight"), MissRef.IsValid()))
    {
        return false;
    }
    TestTrue(TEXT("First projectile moved forward"), HitRef->GetActorLocation().X > 40.0);
    TestTrue(TEXT("Second projectile moved along its own direction"), MissRef->GetActorLocation().Y > 40.0);
    TestEqual(TEXT("No impact before the wall"), WallImpacts, 0);

    for (int32 Step = 0; Step < 34; ++Step)
    {
        if (!Fixture.TickTestWorld(0.10f))
        {
            return false;
        }
    }
    TestEqual(TEXT("Only one blocking impact"), WallImpacts, 1);
    TestTrue(TEXT("First hit is the wall, not the source"), FirstHit == Wall);
    TestTrue(TEXT("Impact carries the original source"), ReportedSource == Source);
    TestFalse(TEXT("Impact listener cannot relaunch"), bRelaunchFromImpact);
    TestFalse(TEXT("Wall projectile is removed"), HitRef.IsValid());
    TestFalse(TEXT("Miss projectile expires by lifetime"), MissRef.IsValid());
    TestEqual(TEXT("Lifetime expiry emits no impact"), MissImpacts, 0);

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 3. Editor与Game Development构建

从仓库根目录执行。先通过Editor Build与三模块索引，再单独执行Game Build。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

```powershell
Set-Location 'E:\UE_Project\Demo'
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

构建完成后，在CLion重新加载CMake项目并检查三个新文件进入Core目标。相较上一批，Core应增加两个cpp。

## 4. 验收

隔离工程的UE 5.5.4 Editor Development构建与`ProjectileFlight`专项已通过。正式工程的双Build、索引和地图短查按以下步骤验收。

### 4.1 真实世界飞行专项

打开UE工程，保持退出PIE。在Output Log底部的命令栏输入并回车。

```text
Automation RunTests EmberTrial.M4.Combat.ProjectileFlight
```

等最终结果为`Success`。测试自身在独立世界创建发射者、薄挡板与两枚投射物。第一枚从发射者碰撞内起飞，穿过发射者后击中薄挡板，验证只广播一次命中且回调不能重新发射。第二枚从另一条空路线起飞，验证按独立方向移动、到期销毁且没有命中事件。

### 4.2 正式地图短查

打开`L_ET_VerticalSlice`，直接Play一次。确认生命与体力100，移动转向正常，左键可攻击，按P并继续后输入恢复，再停止PIE。本批的投射物只由自动化夹具生成，正式地图沿用现有敌人和行为。

反馈三新文件、双Build和索引、ProjectileFlight结果、正式地图短查、源码是否保存、实际手动时间与构建等待。失败时贴测试名和第一条`Expected`或第一条编译错误，画面问题补截图。

## 5. Commit / Push（历史）

原两组变更已提交。当前批次的文档与实现清单见[M4-01B操作指南](M4_01B_ProjectileDamage.md)。

M3验收与M4操作指南组。

```text
Docs/Reports/M3_06C3_Verification.md
Docs/M3_06C3_SixRelicIntegratedPackage.md
Docs/M4_01A_ProjectileFlightFoundation.md
Docs/M3_Roadmap.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

- 英文 `docs: verify six-relic package and plan projectile foundation`
- 中文 `docs: 完成六遗物集成包验收并规划投射物飞行基础`

M4-01A实现组，在你完成双Build与专项后提交。

```text
EmberTrial/Source/EmberTrial/Public/Combat/ETCombatProjectile.h
EmberTrial/Source/EmberTrial/Private/Combat/ETCombatProjectile.cpp
EmberTrial/Source/EmberTrial/Tests/ETCombatProjectileTests.cpp
```

- 英文 `feat: add swept projectile flight and impact lifecycle`
- 中文 `feat: 新增投射物扫掠飞行与命中生命周期`

包、Runtime、UAT、候选与隔离测试产物均留在本地`Artifacts`。
