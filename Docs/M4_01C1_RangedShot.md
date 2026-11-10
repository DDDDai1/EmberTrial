# M4-01C1 远程守卫的原生发射入口

本批把目标检查、视线检查和生成投射物收进一个入口。预计手动35–50分钟，构建等待另记。你在CLion新建三份C++文件，另在角色头中加一行测试声明。[M4-01B验收](Reports/M4_01B_Verification.md)

## 1. 交付边界

| 环节 | 本批行为 |
| --- | --- |
| 请求 | `ETRangedShot::TryFire`接收明确的来源和目标，校验敌方身份、双方存活与就绪、世界暂停以及配置 |
| 发射资格 | 以两个角色中心之间的三维距离判断250–1200 cm区间，沿同一路径检查Visibility阻挡。配置可调，默认12点仅为占位平衡值 |
| 生成 | 从来源中心按当时的目标方向生成一次，来源自身碰撞由投射物忽略。返回非空指针表示发射成功，可供后续能力决定何时开始冷却 |
| 伤害 | 继续走M4-01B的扫掠和GAS入口，目标的无敌与阵营以实际命中时为准 |
| 下一批 | 将此入口接入远程能力的动画提交点和冷却，再接感知、距离决策及正式守卫。C1由专项驱动，无需设置蓝图、动画、AI或地图资源 |

## 2. 新增两份发射源码

保存并关闭UE编辑器。在CLion的`EmberTrial/Source/EmberTrial`下按下面目录新建文件。`ETRangedShot.generated.h`由正常Editor Build生成。

### 2.1 `Public/Combat/ETRangedShot.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "ETRangedShot.generated.h"

class AEmberTrialCharacter;
class AETCombatProjectile;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETRangedShotConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Ranged Shot")
    TSubclassOf<AETCombatProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot", meta = (ClampMin = "0.01"))
    float Damage = 12.0f;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot", meta = (ClampMin = "1.0", Units = "cm"))
    float MinRange = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Ranged Shot", meta = (ClampMin = "1.0", Units = "cm"))
    float MaxRange = 1200.0f;
};

namespace ETRangedShot
{
    // 发射时以角色中心瞄准，不在飞行途中跟踪目标。
    EMBERTRIAL_API AETCombatProjectile* TryFire(
        AEmberTrialCharacter* Source, AEmberTrialCharacter* Target,
        const FETRangedShotConfig& Config, float AbilityLevel = 1.0f);
}
```

### 2.2 `Private/Combat/ETRangedShot.cpp`

```cpp
#include "Combat/ETRangedShot.h"

#include "Characters/EmberTrialCharacter.h"
#include "CollisionQueryParams.h"
#include "Combat/ETCombatDamage.h"
#include "Combat/ETCombatProjectile.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRangedShot, Log, All);

namespace
{
    bool IsReadyRangedShotBody(const AEmberTrialCharacter* Body)
    {
        float Health = 0.0f;
        float MaxHealth = 0.0f;
        return IsValid(Body) && !Body->IsActorBeingDestroyed() && !Body->IsCombatDead() &&
               Body->GetGameplayHealthSnapshot(Health, MaxHealth) &&
               FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) &&
               Health > 0.0f && MaxHealth >= 1.0f && Health <= MaxHealth;
    }
}

AETCombatProjectile* ETRangedShot::TryFire(
    AEmberTrialCharacter* Source, AEmberTrialCharacter* Target,
    const FETRangedShotConfig& Config, float AbilityLevel)
{
    if (!IsReadyRangedShotBody(Source) || !IsReadyRangedShotBody(Target) ||
        !Source->HasAuthority() || Source->GetCombatTeam() != EETCombatTeam::Enemy ||
        !Source->CanDealCombatDamageTo(Target))
    {
        return nullptr;
    }

    UWorld* World = Source->GetWorld();
    UClass* ProjectileClass = Config.ProjectileClass.Get();
    if (!World || World != Target->GetWorld() || !World->HasBegunPlay() || World->IsPaused() ||
        !IsValid(ProjectileClass) || ProjectileClass->HasAnyClassFlags(CLASS_Abstract) ||
        !ETCombatDamage::IsValidConfiguration(Config.DamageEffectClass, Config.Damage) ||
        !FMath::IsFinite(AbilityLevel) || AbilityLevel <= 0.0f ||
        !FMath::IsFinite(Config.MinRange) || !FMath::IsFinite(Config.MaxRange) ||
        Config.MinRange < 1.0f || Config.MaxRange < Config.MinRange)
    {
        return nullptr;
    }

    const FVector Origin = Source->GetActorLocation();
    const FVector Aim = Target->GetActorLocation();
    const FVector Delta = Aim - Origin;
    const double DistanceSquared = Delta.SizeSquared();
    if (Origin.ContainsNaN() || Aim.ContainsNaN() || !FMath::IsFinite(DistanceSquared) ||
        DistanceSquared < FMath::Square(static_cast<double>(Config.MinRange)) ||
        DistanceSquared > FMath::Square(static_cast<double>(Config.MaxRange)))
    {
        return nullptr;
    }

    FCollisionQueryParams SightQuery(SCENE_QUERY_STAT(ETRangedShotSight), false, Source);
    SightQuery.AddIgnoredActor(Target);
    if (World->LineTraceTestByChannel(Origin, Aim, ECC_Visibility, SightQuery))
    {
        return nullptr;
    }

    // SpawnActor会执行BeginPlay。先快照发射参数，避免同步回调修改配置。
    const TSubclassOf<UGameplayEffect> DamageEffectClass = Config.DamageEffectClass;
    const float Damage = Config.Damage;
    const FVector Direction = Delta.GetSafeNormal();
    const TWeakObjectPtr<AEmberTrialCharacter> WeakSource(Source);
    const TWeakObjectPtr<AEmberTrialCharacter> WeakTarget(Target);

    FActorSpawnParameters Spawn;
    Spawn.Owner = Source;
    Spawn.Instigator = Source;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AETCombatProjectile* Shot = World->SpawnActor<AETCombatProjectile>(
        ProjectileClass, Origin, Direction.Rotation(), Spawn);
    if (!IsValid(Shot))
    {
        return nullptr;
    }

    AEmberTrialCharacter* LiveSource = WeakSource.Get();
    AEmberTrialCharacter* LiveTarget = WeakTarget.Get();
    if (!IsReadyRangedShotBody(LiveSource) || !IsReadyRangedShotBody(LiveTarget) ||
        !LiveSource->CanDealCombatDamageTo(LiveTarget) ||
        !Shot->TryLaunchDamage(LiveSource, Direction, DamageEffectClass, Damage, AbilityLevel) ||
        !IsValid(Shot) || Shot->IsActorBeingDestroyed())
    {
        if (IsValid(Shot) && !Shot->IsActorBeingDestroyed())
        {
            Shot->Destroy();
        }
        return nullptr;
    }

    UE_LOG(LogETRangedShot, Display,
           TEXT("[M4-01C1] Fired Source=%s Target=%s Projectile=%s Distance=%.1f Damage=%.1f"),
           *GetNameSafe(LiveSource), *GetNameSafe(LiveTarget), *GetNameSafe(Shot),
           FMath::Sqrt(DistanceSquared), Damage);
    return Shot;
}
```

## 3. 新增专项

在`Public/Characters/EmberTrialCharacter.h`中找到已有的`friend class FETCombatProjectileDamageTest;`，紧接其后加一行。此声明允许夹具配置测试身体的阵营。

```cpp
    friend class FETRangedShotTest;
```

在`Tests/ETRangedShotTests.cpp`新建完整文件。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ETRangedShot.h"

#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRangedShotTest, "EmberTrial.M4.Combat.RangedShot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRangedShotTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FETRangedShotConfig Config;
    Config.ProjectileClass = AETCombatProjectile::StaticClass();
    Config.DamageEffectClass = LoadClass<UGameplayEffect>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    if (!TestNotNull(TEXT("Existing damage GE"), Config.DamageEffectClass.Get()))
    {
        return false;
    }

    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AEmberTrialCharacter* Source = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator);
    AEmberTrialCharacter* Target = World->SpawnActor<AEmberTrialCharacter>(
        FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Shooter"), Source) || !TestNotNull(TEXT("Target"), Target) ||
        !TestNotNull(TEXT("Pause state"), Pauser))
    {
        return false;
    }
    Source->CombatTeam = EETCombatTeam::Enemy;
    Target->CombatTeam = EETCombatTeam::Player;
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    // 世界启动后锁定测试身体，保持每次扫掠的碰撞位置。
    for (AEmberTrialCharacter* Body : {Source, Target})
    {
        Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        Body->GetCharacterMovement()->DisableMovement();
        Body->GetCharacterMovement()->SetComponentTickEnabled(false);
    }
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const auto MoveTarget = [Target](double X)
    {
        Target->SetActorLocation(FVector(X, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
    };
    const auto Reject = [this, World, Source](const TCHAR* Reason, AEmberTrialCharacter* Body,
                                             const FETRangedShotConfig& ShotConfig)
    {
        TestNull(Reason, ETRangedShot::TryFire(Source, Body, ShotConfig));
        const TActorIterator<AETCombatProjectile> Live(World);
        TestFalse(TEXT("Rejected request leaves no live projectile"), static_cast<bool>(Live));
    };

    Reject(TEXT("Missing target"), nullptr, Config);
    Target->CombatTeam = EETCombatTeam::Enemy;
    Reject(TEXT("Friendly target"), Target, Config);
    Target->CombatTeam = EETCombatTeam::Player;

    FETRangedShotConfig Invalid = Config;
    Invalid.MaxRange = Invalid.MinRange - 1.0f;
    Reject(TEXT("Reversed range"), Target, Invalid);
    Invalid = Config;
    Invalid.ProjectileClass = nullptr;
    Reject(TEXT("Missing projectile class"), Target, Invalid);
    Invalid = Config;
    Invalid.DamageEffectClass = nullptr;
    Reject(TEXT("Missing damage effect"), Target, Invalid);

    MoveTarget(Config.MinRange - 1.0f);
    Reject(TEXT("Inside minimum range"), Target, Config);
    MoveTarget(Config.MaxRange + 1.0f);
    Reject(TEXT("Outside maximum range"), Target, Config);
    MoveTarget(600.0);

    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("Pause fixture active"), World->IsPaused());
    Reject(TEXT("World paused"), Target, Config);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);

    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Sight blocker"), Wall))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->InitBoxExtent(FVector(3.0, 80.0, 80.0));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(300.0, 0.0, 0.0));
    Reject(TEXT("Sight blocked"), Target, Config);
    Wall->Destroy();

    TestEqual(TEXT("Rejected requests leave health unchanged"), TargetASC->GetNumericAttribute(Health), 100.0f);
    AETCombatProjectile* Shot = ETRangedShot::TryFire(Source, Target, Config);
    if (!TestNotNull(TEXT("Clear shot launches"), Shot))
    {
        return false;
    }
    TestTrue(TEXT("Projectile owner and instigator are the shooter"),
             Shot->GetOwner() == Source && Shot->GetInstigator() == Source);
    TestEqual(TEXT("Launch does not damage immediately"), TargetASC->GetNumericAttribute(Health), 100.0f);
    const TWeakObjectPtr<AETCombatProjectile> WeakShot(Shot);
    for (int32 Step = 0; Step < 8; ++Step)
    {
        if (!Fixture.TickTestWorld(0.10f))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
    }
    TestFalse(TEXT("Successful impact consumes projectile"), WeakShot.IsValid());
    TestEqual(TEXT("Actual ranged damage"), TargetASC->GetNumericAttribute(Health), 88.0f);
    TestEqual(TEXT("Actual ranged damage updates Base"), TargetASC->GetNumericAttributeBase(Health), 88.0f);
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

## 4. 构建与索引

保存源码，在UE关闭时从仓库根目录依次执行。Editor目标由索引脚本正常构建，第二条单独构建Game Development。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

```powershell
Set-Location 'E:\UE_Project\Demo'
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

在CLion重载CMake。Core索引应从93增加至95个cpp。

## 5. 分组验收

隔离工程的Editor Development构建与46项全组自动化通过。发射专项实际在600 cm发射，并走真实扫掠把目标生命从100降至88。全组结果保存在`Artifacts/M4_01C1/AutomationReport/index.json`。正式工程按以下两组验收。

### 5.1 发射与旧结算

在UE的Output Log命令栏逐条执行，上一条结束后再执行下一条。

```text
Automation RunTests EmberTrial.M4.Combat.RangedShot
Automation RunTests EmberTrial.M4.Combat.Projectile
```

| 结果 | 预期 |
| --- | --- |
| `RangedShot` | 缺目标、友方、非法配置、过近、过远、暂停、遮挡均拒绝，且无残留弹体。移除挡板后600 cm发射成功，生成时生命仍为100，实际命中后Current与Base均为88 |
| `ProjectileDamage` / `ProjectileFlight` | 两项各一条Success，覆盖已有真实伤害、命中规则和纯飞行 |

### 5.2 正式地图短查

1. 打开`L_ET_VerticalSlice`，启动PIE并确认Entry出生、移动与转向正常。
2. 按P打开暂停页，确认页面实际出现。点击继续后移动一次，再Stop。按本批范围，正式地图维持现有近战守卫。

反馈三新文件与friend、双Build和索引、三项自动化、地图短查、保存状态、手动时间和等待。失败时贴首条编译错误，或测试名和首条`Expected`。

## 6. Commit / Push

先Stage本轮验收与指南文档，核对Staged Diff后可单独提交。

```text
Docs/Reports/M4_01B_Verification.md
Docs/M4_01B_ProjectileDamage.md
Docs/M4_01C1_RangedShot.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

- 英文 `docs: verify projectile damage and plan ranged firing`
- 中文 `docs: 完成投射物伤害验收并规划远程发射`

手动实施与验收后，Stage以下四份Core源码。

```text
EmberTrial/Source/EmberTrial/Public/Combat/ETRangedShot.h
EmberTrial/Source/EmberTrial/Private/Combat/ETRangedShot.cpp
EmberTrial/Source/EmberTrial/Tests/ETRangedShotTests.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
```

- 英文 `feat: gate ranged projectile fire by range and line of sight`
- 中文 `feat: 按射程与视线校验远程投射物发射`

候选、构建日志、自动化与PIE证据保留在本地`Artifacts`。
