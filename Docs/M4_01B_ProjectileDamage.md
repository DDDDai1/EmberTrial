# M4-01B 投射物接入GAS生命伤害

2026-11-10已验收。三新四旧Core文件已提交。实际手动30分钟，构建等待5分钟。以下保留实现步骤。[B验收](Reports/M4_01B_Verification.md)

## 1. 本批行为

| 环节 | 预期 |
| --- | --- |
| 发射 | `TryLaunchDamage`快照GE类、正值伤害和能力等级。已有`TryLaunch`继续用于纯飞行 |
| 结算 | 飞行第一次阻挡时用来源ASC提交现有的Instant Health AddBase／SetByCaller GE。可用来源、目标ASC、阵营、死亡和无敌按命中时状态校验 |
| 物理阻挡 | 友方、无敌目标和墙仍可挡住投射物。伤害被拒绝时也完成本次飞行。发射者失效后，命中按`SourceUnavailable`拒绝伤害 |
| 同步回调 | 先关闭碰撞并封住重复命中，再提交伤害。投射物仍有效时，外部`OnImpact`观察到结算后的生命 |
| 兼容 | 近战沿用原连段快照、伤害资产、日志格式和测试。M4后续再实现远程守卫的施放、冷却、动画与AI |

测试伤害16仅用于夹具。正式项目的玩家25和近战敌人10沿用已有配置。

## 2. 新增共享结算

关闭UE编辑器。在`EmberTrial/Source/EmberTrial`下按文件名所列路径创建。共享头位于`Public/Combat`，实现位于`Private/Combat`。调用者目前都在Core。无需修改Build.cs或创建新GE。

### 2.1 `Public/Combat/ETCombatDamage.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;

namespace ETCombatDamage
{
    struct FResult
    {
        // 拒绝原因使用静态文本。空值表示已尝试提交GE。
        const TCHAR* Rejection = nullptr;
        bool bApplied = false;
        bool bActiveHandle = false;
        bool bHasPostHitSnapshot = false;
        float HealthBefore = 0.0f;
        float HealthAfter = 0.0f;
        float BaseAfter = 0.0f;
        float MaxAfter = 0.0f;
    };

    bool IsValidConfiguration(TSubclassOf<UGameplayEffect> EffectClass, float Damage);

    FResult ApplyHit(AEmberTrialCharacter* Source, UAbilitySystemComponent* SourceASC,
                     const FHitResult& Hit, TSubclassOf<UGameplayEffect> EffectClass,
                     float Damage, float Level, const UObject* SourceObject,
                     AActor* EffectCauser = nullptr);
}
```

### 2.2 `Private/Combat/ETCombatDamage.cpp`

```cpp
#include "Combat/ETCombatDamage.h"

#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/HitResult.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

bool ETCombatDamage::IsValidConfiguration(TSubclassOf<UGameplayEffect> EffectClass, float Damage)
{
    if (!FMath::IsFinite(Damage) || Damage <= 0.0f || !IsValid(EffectClass.Get()))
    {
        return false;
    }

    const UGameplayEffect* Effect = EffectClass.GetDefaultObject();
    if (!IsValid(Effect) || Effect->DurationPolicy != EGameplayEffectDurationType::Instant ||
        Effect->Modifiers.Num() != 1 || Effect->Executions.Num() != 0)
    {
        return false;
    }

    const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
    return Modifier.Attribute == UETHealthAttributeSet::GetHealthAttribute() &&
           Modifier.ModifierOp == EGameplayModOp::AddBase &&
           Modifier.ModifierMagnitude.GetMagnitudeCalculationType() ==
               EGameplayEffectMagnitudeCalculation::SetByCaller &&
           Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag ==
               ETGameplayTags::SetByCaller_HealthDelta;
}

ETCombatDamage::FResult ETCombatDamage::ApplyHit(
    AEmberTrialCharacter* Source, UAbilitySystemComponent* SourceASC, const FHitResult& Hit,
    TSubclassOf<UGameplayEffect> EffectClass, float Damage, float Level,
    const UObject* SourceObject, AActor* EffectCauser)
{
    // 发射时的数值已经快照，身份、生命和防御Tag以命中时为准。
    float SourceHealth = 0.0f;
    float SourceMaxHealth = 0.0f;
    if (!IsValid(Source) || Source->IsActorBeingDestroyed() || Source->IsCombatDead() ||
        !Source->HasAuthority() || !IsValid(SourceASC) ||
        Source->GetAbilitySystemComponent() != SourceASC || !SourceASC->AbilityActorInfo.IsValid() ||
        SourceASC->AbilityActorInfo->OwnerActor.Get() != Source ||
        SourceASC->AbilityActorInfo->AvatarActor.Get() != Source ||
        !Source->GetGameplayHealthSnapshot(SourceHealth, SourceMaxHealth) ||
        !FMath::IsFinite(SourceHealth) || !FMath::IsFinite(SourceMaxHealth) ||
        SourceHealth <= 0.0f || SourceMaxHealth < 1.0f || SourceHealth > SourceMaxHealth ||
        UGameplayStatics::IsGamePaused(Source))
    {
        return {TEXT("SourceUnavailable")};
    }
    if (!IsValidConfiguration(EffectClass, Damage) || !FMath::IsFinite(Level) || Level <= 0.0f)
    {
        return {TEXT("InvalidDamageConfig")};
    }

    AEmberTrialCharacter* Target = Cast<AEmberTrialCharacter>(Hit.GetActor());
    if (!IsValid(Target) || Target == Source || Target->GetWorld() != Source->GetWorld())
    {
        return {TEXT("UnsupportedTarget")};
    }
    if (!Source->CanDealCombatDamageTo(Target))
    {
        return {TEXT("CombatTeam")};
    }
    if (Target->IsCombatDead())
    {
        return {TEXT("TargetDead")};
    }

    FResult Result;
    float MaxHealthBefore = 0.0f;
    if (!Target->GetGameplayHealthSnapshot(Result.HealthBefore, MaxHealthBefore) ||
        !FMath::IsFinite(Result.HealthBefore) || !FMath::IsFinite(MaxHealthBefore) ||
        MaxHealthBefore < 1.0f || Result.HealthBefore < 0.0f || Result.HealthBefore > MaxHealthBefore)
    {
        return {TEXT("TargetNotReady")};
    }
    if (Result.HealthBefore <= 0.0f)
    {
        return {TEXT("TargetZeroHealth")};
    }

    const FGameplayAttribute HealthAttribute = UETHealthAttributeSet::GetHealthAttribute();
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    if (!IsValid(TargetASC) || !TargetASC->AbilityActorInfo.IsValid() ||
        TargetASC->AbilityActorInfo->OwnerActor.Get() != Target ||
        TargetASC->AbilityActorInfo->AvatarActor.Get() != Target ||
        !TargetASC->HasAttributeSetForAttribute(HealthAttribute))
    {
        return {TEXT("TargetASCUnavailable")};
    }

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    if (!Context.IsValid())
    {
        return {TEXT("ContextUnavailable")};
    }
    if (EffectCauser)
    {
        Context.AddInstigator(Source, EffectCauser);
    }
    Context.AddSourceObject(SourceObject);
    Context.AddHitResult(Hit, true);
    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, Level, Context);
    if (!Spec.IsValid())
    {
        return {TEXT("SpecUnavailable")};
    }
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_HealthDelta, -Damage);

    if (TargetASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable))
    {
        return {TEXT("Invulnerable")};
    }

    const TWeakObjectPtr<AEmberTrialCharacter> WeakTarget(Target);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakTargetASC(TargetASC);
    const FActiveGameplayEffectHandle Handle =
        SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
    Result.bApplied = Handle.WasSuccessfullyApplied();
    Result.bActiveHandle = Handle.IsValid();

    // 属性回调可能同步取消能力、销毁目标或更换ASC。
    const AEmberTrialCharacter* LiveTarget = WeakTarget.Get();
    const UAbilitySystemComponent* LiveASC = WeakTargetASC.Get();
    if (LiveTarget && LiveASC && LiveTarget->GetAbilitySystemComponent() == LiveASC &&
        LiveTarget->GetGameplayHealthSnapshot(Result.HealthAfter, Result.MaxAfter))
    {
        Result.BaseAfter = LiveASC->GetNumericAttributeBase(HealthAttribute);
        Result.bHasPostHitSnapshot = true;
    }
    return Result;
}
```

## 3. 投射物接入

替换以下两份既有文件。原`TryLaunch`签名保持一致。发射快照在激活移动组件前写入，来源继续使用弱引用。

### 3.1 `Public/Combat/ETCombatProjectile.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "ETCombatProjectile.generated.h"

class AEmberTrialCharacter;
class AETCombatProjectile;
class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETCombatProjectileImpact, AETCombatProjectile*, const FHitResult&);

// 一次性投射物。可纯飞行，也可按发射时快照结算生命伤害。
UCLASS(Blueprintable)
class EMBERTRIAL_API AETCombatProjectile : public AActor
{
    GENERATED_BODY()

public:
    AETCombatProjectile();

    bool TryLaunch(AActor* InSource, const FVector& WorldDirection);
    bool TryLaunchDamage(AEmberTrialCharacter* InSource, const FVector& WorldDirection,
                         TSubclassOf<UGameplayEffect> InEffectClass, float Damage, float Level = 1.0f);
    AActor* GetSourceActor() const { return SourceActor.Get(); }

    FETCombatProjectileImpact OnImpact;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool TryLaunchInternal(AActor* InSource, const FVector& WorldDirection,
                           TSubclassOf<UGameplayEffect> InEffectClass, float Damage, float Level);
    void ApplyDamageOnImpact(const FHitResult& Hit);

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

    // 飞行期间持有GE类，发射者仍是弱引用。
    UPROPERTY(Transient)
    TSubclassOf<UGameplayEffect> FlightDamageEffectClass;

    float FlightDamage = 0.0f;
    float FlightDamageLevel = 1.0f;
    TWeakObjectPtr<AActor> SourceActor;
    bool bLaunched = false;
    bool bResolved = false;
};
```

### 3.2 `Private/Combat/ETCombatProjectile.cpp`

```cpp
#include "Combat/ETCombatProjectile.h"

#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatDamage.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCombatProjectile, Log, All);

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
    return TryLaunchInternal(InSource, WorldDirection, nullptr, 0.0f, 1.0f);
}

bool AETCombatProjectile::TryLaunchDamage(
    AEmberTrialCharacter* InSource, const FVector& WorldDirection,
    TSubclassOf<UGameplayEffect> InEffectClass, float Damage, float Level)
{
    if (!IsValid(InSource) || !InSource->IsGameplayReady() || InSource->IsCombatDead() ||
        !ETCombatDamage::IsValidConfiguration(InEffectClass, Damage) ||
        !FMath::IsFinite(Level) || Level <= 0.0f)
    {
        return false;
    }
    return TryLaunchInternal(InSource, WorldDirection, InEffectClass, Damage, Level);
}

bool AETCombatProjectile::TryLaunchInternal(
    AActor* InSource, const FVector& WorldDirection, TSubclassOf<UGameplayEffect> InEffectClass,
    float Damage, float Level)
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
    FlightDamageEffectClass = InEffectClass;
    FlightDamage = Damage;
    FlightDamageLevel = Level;
    Collision->IgnoreActorWhenMoving(InSource, true);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ProjectileMovement->Velocity = Direction * LaunchSpeed;
    SetLifeSpan(FlightLifetime);
    ProjectileMovement->Activate();
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
        if (FlightDamageEffectClass)
        {
            ApplyDamageOnImpact(Hit);
        }
        if (!IsActorBeingDestroyed())
        {
            OnImpact.Broadcast(this, Hit);
        }
    }
    if (!IsActorBeingDestroyed())
    {
        Destroy();
    }
}

void AETCombatProjectile::ApplyDamageOnImpact(const FHitResult& Hit)
{
    AEmberTrialCharacter* Source = Cast<AEmberTrialCharacter>(GetSourceActor());
    const FString SourceName = GetNameSafe(Source);
    const FString TargetName = GetNameSafe(Hit.GetActor());
    const float Damage = FlightDamage;
    const ETCombatDamage::FResult Result = ETCombatDamage::ApplyHit(
        Source, Source ? Source->GetAbilitySystemComponent() : nullptr,
        Hit, FlightDamageEffectClass, Damage, FlightDamageLevel, this, this);
    if (Result.Rejection)
    {
        UE_LOG(LogETCombatProjectile, Display,
               TEXT("[M4-01B] Damage skipped Source=%s Target=%s Reason=%s"),
               *SourceName, *TargetName, Result.Rejection);
        return;
    }
    UE_LOG(LogETCombatProjectile, Display,
           TEXT("[M4-01B] Damage Source=%s Target=%s Requested=%.1f Applied=%d Snapshot=%d "
                "Before=%.1f After=%.1f BaseAfter=%.1f"),
           *SourceName, *TargetName, Damage, Result.bApplied ? 1 : 0,
           Result.bHasPostHitSnapshot ? 1 : 0, Result.HealthBefore, Result.HealthAfter,
           Result.BaseAfter);
}

void AETCombatProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ProjectileMovement->OnProjectileStop.RemoveDynamic(this, &ThisClass::HandleProjectileStopped);
    OnImpact.Clear();
    Super::EndPlay(EndPlayReason);
}
```

## 4. 近战改用共享结算

打开`Private/Abilities/ETGA_LightAttack.cpp`，按以下三处增量操作，保留中间的激活、蒙太奇、缓冲和取消代码。

### 4.1 Include

定位原先连续的四条。

```cpp
#include "Combat/ETCombatTraceComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "GameplayEffect.h"
```

替换为以下三条。

```cpp
#include "Combat/ETCombatDamage.h"
#include "Combat/ETCombatTraceComponent.h"
#include "GameplayEffect.h"
```

### 4.2 配置检查

整段替换`HasValidDamageConfiguration`，下方的`HasValidComboConfiguration`保持原样。

```cpp
bool UETGA_LightAttack::HasValidDamageConfiguration() const
{
    return ETCombatDamage::IsValidConfiguration(DamageEffectClass, AttackDamage);
}
```

### 4.3 命中回调

整段替换文件末尾的`HandleTraceHit`。函数上方的结束、Trace Serial与阶段逻辑保留。

```cpp
void UETGA_LightAttack::HandleTraceHit(uint32 Serial, const FHitResult& Hit)
{
    if (!IsActive() || Serial == 0 || Serial != ActiveTraceSerial || !CurrentActorInfo)
    {
        return;
    }

    const FString TargetName = GetNameSafe(Hit.GetActor());
    const auto Reject = [Serial, &TargetName](const TCHAR* Reason)
    {
        UE_LOG(LogETLightAttack, Display,
               TEXT("[M1-04B2] Damage skipped Serial=%u Target=%s Reason=%s"), Serial, *TargetName,
               Reason);
    };

    AEmberTrialCharacter* Source = Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    UAbilitySystemComponent* SourceASC = CurrentActorInfo->AbilitySystemComponent.Get();
    if (!IsValid(Source) || !IsValid(SourceASC) || CurrentActorInfo->OwnerActor.Get() != Source)
    {
        Reject(TEXT("SourceUnavailable"));
        return;
    }
    if (!FMath::IsFinite(AttackDamage) || AttackDamage <= 0.0f)
    {
        Reject(TEXT("InvalidDamageConfig"));
        return;
    }

    const FString SourceName = GetNameSafe(Source);
    const float RequestedDamage = ActiveStageDamage;
    const ETCombatDamage::FResult Result = ETCombatDamage::ApplyHit(
        Source, SourceASC, Hit, DamageEffectClass, RequestedDamage, GetAbilityLevel(), this);
    if (Result.Rejection)
    {
        Reject(Result.Rejection);
        return;
    }
    if (!Result.bHasPostHitSnapshot)
    {
        UE_LOG(LogETLightAttack, Display,
               TEXT("[M1-04B2] Damage Serial=%u Source=%s Target=%s Requested=%.1f Applied=%d "
                    "ActiveHandle=%d Snapshot=Unavailable"),
               Serial, *SourceName, *TargetName, RequestedDamage, Result.bApplied ? 1 : 0,
               Result.bActiveHandle ? 1 : 0);
        return;
    }
    UE_LOG(LogETLightAttack, Display,
           TEXT("[M1-04B2] Damage Serial=%u Source=%s Target=%s Requested=%.1f Applied=%d "
                "ActiveHandle=%d Before=%.1f After=%.1f BaseAfter=%.1f Max=%.1f Actual=%.1f"),
           Serial, *SourceName, *TargetName, RequestedDamage, Result.bApplied ? 1 : 0,
           Result.bActiveHandle ? 1 : 0, Result.HealthBefore, Result.HealthAfter, Result.BaseAfter,
           Result.MaxAfter, FMath::Max(0.0f, Result.HealthBefore - Result.HealthAfter));
}
```

## 5. 新增自动化

在`Public/Characters/EmberTrialCharacter.h`内，找到现有的`friend class FETLightComboFlowTest;`。紧接其后插入一行，保留其余friend。测试借此在独立世界配置阵营。

```cpp
    friend class FETCombatProjectileDamageTest;
```

在Core的`Tests`目录新建`ETCombatProjectileDamageTests.cpp`，粘贴完整代码。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/ETCombatProjectile.h"

#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETCombatProjectileDamageTest,
    "EmberTrial.M4.Combat.ProjectileDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETCombatProjectileDamageTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const TSubclassOf<UGameplayEffect> DamageEffect = LoadClass<UGameplayEffect>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    if (!TestNotNull(TEXT("Existing Instant health damage GE"), DamageEffect.Get()))
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
    const auto SpawnBody = [World](double X, EETCombatTeam Team)
    {
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(X, 0.0, 0.0), FRotator::ZeroRotator);
        if (Body)
        {
            Body->CombatTeam = Team;
            Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        }
        return Body;
    };
    AEmberTrialCharacter* Source = SpawnBody(0.0, EETCombatTeam::Enemy);
    AEmberTrialCharacter* Target = SpawnBody(480.0, EETCombatTeam::Player);
    if (!TestNotNull(TEXT("Enemy source"), Source) || !TestNotNull(TEXT("Player target"), Target) ||
        !Fixture.BeginPlayInTestWorld() ||
        !TestTrue(TEXT("Both ASCs ready"), Source->IsGameplayReady() && Target->IsGameplayReady()))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }

    // BeginPlay may select the default movement mode. Freeze the collision
    // fixtures after world startup so gravity cannot move them between shots.
    for (AEmberTrialCharacter* Body : {Source, Target})
    {
        UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
        Movement->DisableMovement();
        Movement->SetComponentTickEnabled(false);
    }

    constexpr float Damage = 16.0f;
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const auto CheckHealth = [this, TargetASC, Health](const TCHAR* Label, float Expected)
    {
        TestEqual(Label, TargetASC->GetNumericAttribute(Health), Expected);
        TestEqual(TEXT("Target base health matches Current"),
                  TargetASC->GetNumericAttributeBase(Health), Expected);
    };
    const auto Advance = [&Fixture]()
    {
        for (int32 Step = 0; Step < 6; ++Step)
        {
            if (!Fixture.TickTestWorld(0.10f))
            {
                return false;
            }
        }
        return true;
    };

    // 火力、规则和墙体均走真实扫掠与命中回调。
    const auto Fire = [&](const TCHAR* Label, float Before, float After, AActor* ExpectedHit,
                          bool bInvulnerable = false, bool bDestroySource = false)
    {
        AETCombatProjectile* Shot = World->SpawnActor<AETCombatProjectile>();
        if (!TestNotNull(TEXT("Projectile"), Shot))
        {
            return false;
        }
        TestFalse(TEXT("Missing GE rejected"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, nullptr, Damage));
        TestFalse(TEXT("Wrong GE contract rejected"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, UGameplayEffect::StaticClass(), Damage));
        TestFalse(TEXT("Zero damage rejected"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, DamageEffect, 0.0f));
        if (!TestTrue(TEXT("Launch configured shot"),
                      Shot->TryLaunchDamage(Source, FVector::ForwardVector, DamageEffect, Damage)))
        {
            return false;
        }
        CheckHealth(TEXT("No damage at launch"), Before);
        TestFalse(TEXT("Second request cannot replace damage snapshot"),
                  Shot->TryLaunchDamage(Source, FVector::ForwardVector, DamageEffect, 90.0f));

        int32 Impacts = 0;
        Shot->OnImpact.AddLambda([&](AETCombatProjectile*, const FHitResult& Hit)
        {
            ++Impacts;
            TestTrue(TEXT("Expected actor blocked the shot"), Hit.GetActor() == ExpectedHit);
            CheckHealth(TEXT("Listener sees the post-hit health"), After);
        });
        const TWeakObjectPtr<AETCombatProjectile> WeakShot(Shot);
        if (bInvulnerable)
        {
            TargetASC->AddLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (bDestroySource)
        {
            Source->Destroy();
        }
        const bool bAdvanced = Advance();
        if (bInvulnerable)
        {
            TargetASC->RemoveLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (AETCombatProjectile* LiveShot = WeakShot.Get())
        {
            // 回调使用本次Fire的栈变量，提前失败时也要解除绑定。
            LiveShot->OnImpact.Clear();
        }
        TestEqual(FString::Printf(TEXT("%s: one blocking impact callback"), Label), Impacts, 1);
        TestFalse(TEXT("Blocking hit consumes projectile"), WeakShot.IsValid());
        CheckHealth(Label, After);
        return bAdvanced;
    };

    if (!Fire(TEXT("Enemy shot deals 16 once"), 100.0f, 84.0f, Target) ||
        !Fire(TEXT("Invulnerability added in flight blocks damage"), 84.0f, 84.0f, Target, true))
    {
        return false;
    }
    Target->CombatTeam = EETCombatTeam::Enemy;
    if (!Fire(TEXT("Friendly hit consumes flight without damage"), 84.0f, 84.0f, Target))
    {
        return false;
    }
    Target->CombatTeam = EETCombatTeam::Player;

    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Thin wall"), Wall))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->InitBoxExtent(FVector(3.0, 80.0, 80.0));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(240.0, 0.0, 0.0));
    if (!Fire(TEXT("Wall prevents damage to the body behind it"), 84.0f, 84.0f, Wall))
    {
        return false;
    }
    Wall->Destroy();

    if (!Fire(TEXT("Destroyed source cannot submit damage"), 84.0f, 84.0f, Target, false, true) ||
        !Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 6. 构建与索引

保存所有源码并关闭UE编辑器。从仓库根目录运行两个构建。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

```powershell
Set-Location 'E:\UE_Project\Demo'
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

在CLion重载CMake，确认Core从91增至93个cpp，新工具和测试都进入目标。

## 7. 分组验收

隔离工程的Editor Development构建通过，45项自动化全部Success。最终报告保存在`Artifacts/M4_01B/AutomationReport_Final/index.json`。正式工程按下面三组做窄验收。

### 7.1 投射物两项

在UE的Output Log输入下面一条并回车。前缀会匹配两个专项，等`ProjectileDamage`与`ProjectileFlight`各出一条Success。

```text
Automation RunTests EmberTrial.M4.Combat.Projectile
```

| 检查组 | 自动化覆盖 |
| --- | --- |
| 发射与结算 | 无效GE、零伤害和重复发射拒绝。实际扫掠命中一次，生命Current与Base从100降至84，命中监听者读到84 |
| 命中时规则 | 发射后开启无敌、同阵营、薄墙和发射者销毁四组保持84。每次阻挡仍消耗投射物 |
| 飞行回归 | 纯飞行忽略来源碰撞、扫掠、到期销毁及单次广播 |

### 7.2 近战两个回归

在同一Output Log逐条执行，上一条结束后再运行下一条。

```text
Automation RunTests EmberTrial.M2.Combo.Flow
Automation RunTests EmberTrial.M2.Dodge.Invulnerability
```

前者查三段命中及原配置，后者查近战真实伤害与闪避窗口。两条均应为Success。

### 7.3 正式地图短查

1. 打开`L_ET_VerticalSlice`并Play。靠近训练目标并面向它，站定单击一次左键。空槽预期日志`[M1-04B2] Damage`显示`Actual=25.0`。在地图实例使用默认值且GI是新一轮时检查。
2. 按P打开暂停，用鼠标或键盘继续。确认移动转向正常，再Stop。本批投射物由专项生成。正式地图继续用近战敌人。

反馈三新／四旧、双Build和索引、四项自动化、一次真实25与P短查、源码保存、手动时间与等待。失败时贴测试名及首条Expected，编译失败贴首条编译错误。

## 8. Commit / Push（历史）

下列文档与实现已提交。本轮收尾和下一阶段的清单见[M4-01C1操作指南](M4_01C1_RangedShot.md)。

```text
Docs/Reports/M4_01A_Verification.md
Docs/M4_01A_ProjectileFlightFoundation.md
Docs/M4_01B_ProjectileDamage.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

- 英文 `docs: verify projectile flight and plan GAS damage integration`
- 中文 `docs: 完成投射物飞行验收并规划GAS伤害接入`

手动实施并验收后，Stage以下七份Core源码。检查Staged Diff再Commit与Push。

```text
EmberTrial/Source/EmberTrial/Public/Combat/ETCombatDamage.h
EmberTrial/Source/EmberTrial/Private/Combat/ETCombatDamage.cpp
EmberTrial/Source/EmberTrial/Public/Combat/ETCombatProjectile.h
EmberTrial/Source/EmberTrial/Private/Combat/ETCombatProjectile.cpp
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Tests/ETCombatProjectileDamageTests.cpp
```

- 英文 `feat: share GAS hit resolution between melee and projectiles`
- 中文 `feat: 近战与投射物共用GAS命中结算`

候选、构建日志和自动化证据保存在本地`Artifacts`。
