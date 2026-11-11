# M4-01D1 远程守卫的能力授予与攻击请求

本章已验收，手动30分钟，构建等待5分钟。[验收报告](Reports/M4_01D1_Verification.md) · [当前D2操作指南](M4_01D2_RangedGuardAI.md)

本批把已验收的远程GA接入角色的原生生命周期。预计手动25–40分钟，构建等待另记。修改三个既有C++文件，沿用C3的远程资产。下一批由AI行为树选目标、保持距离并调用本章入口。[M4-01C3验收](Reports/M4_01C3_Verification.md)

## 1. 本批行为

| 环节 | 预期 |
| --- | --- |
| 授予 | 敌方身体BeginPlay、属性就绪且配置有效后，授予一份远程能力并持有Spec句柄 |
| 请求 | `RequestRangedShot(Target)`使用已持有的Spec发送目标。GA继续处理动作互斥、暂停、动画发射与冷却 |
| 结束 | EndPlay撤销就绪，取消现有动作，再清除本身体持有的远程Spec |
| 验收 | 既有能力专项通过角色BeginPlay获得GA并从原生请求入口发射，覆盖非敌方授予拒绝和结束清理 |

## 2. 修改角色头文件

在CLion打开`EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h`。保存后在UE关闭时构建。

### 2.1 前置声明

在`class UETGA_BreakPulse;`后一行插入。

```cpp
class UETGA_RangedShot;
```

### 2.2 私有配置和句柄

在`void GrantLightAttackAbility();`后插入。

```cpp

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Abilities")
    TSubclassOf<UETGA_RangedShot> RangedShotAbilityClass;

    FGameplayAbilitySpecHandle RangedShotAbilityHandle;
    void GrantRangedShotAbility();
```

### 2.3 公共入口

在`bool RequestLightAttack();`后插入。

```cpp

    bool RequestRangedShot(AEmberTrialCharacter* Target);
    FGameplayAbilitySpecHandle GetRangedShotSpecHandle() const;
```

## 3. 修改角色实现

打开`EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

### 3.1 Include和BeginPlay

在`#include "Abilities/ETGA_BreakPulse.h"`后增加一行。

```cpp
#include "Abilities/ETGA_RangedShot.h"
```

在`BeginPlay`的`GrantLightAttackAbility();`调用后加一行。

```cpp
    GrantRangedShotAbility();
```

### 3.2 授予、句柄查询和请求

在`// ---- 轻攻击：玩家连段与敌人单段共用授予入口 ----`这条注释**上方**粘贴。保留注释与原近战实现。

```cpp
// ---- 敌方远程能力的授予与请求 ----
void AEmberTrialCharacter::GrantRangedShotAbility()
{
    if (CombatTeam != EETCombatTeam::Enemy || !HasAuthority() || !IsGameplayReady() ||
        IsCombatDead() || RangedShotAbilityHandle.IsValid() || !RangedShotAbilityClass)
    {
        return;
    }

    const UETGA_RangedShot* Defaults = RangedShotAbilityClass.GetDefaultObject();
    if (RangedShotAbilityClass->HasAnyClassFlags(CLASS_Abstract) ||
        !IsValid(Defaults) || !Defaults->HasValidConfiguration())
    {
        UE_LOG(LogETAbilityFoundation, Warning,
               TEXT("[M4-01D1] Ranged grant rejected Body=%s Class=%s"),
               *GetNameSafe(this), *GetNameSafe(RangedShotAbilityClass.Get()));
        return;
    }

    RangedShotAbilityHandle = AbilitySystemComponent->GiveAbility(
        FGameplayAbilitySpec(RangedShotAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M4-01D1] Ranged grant Body=%s Granted=%d"),
           *GetNameSafe(this), RangedShotAbilityHandle.IsValid() ? 1 : 0);
}

FGameplayAbilitySpecHandle AEmberTrialCharacter::GetRangedShotSpecHandle() const
{
    return IsGameplayReady() && !IsCombatDead() ? RangedShotAbilityHandle : FGameplayAbilitySpecHandle();
}

bool AEmberTrialCharacter::RequestRangedShot(AEmberTrialCharacter* Target)
{
    const FGameplayAbilitySpecHandle Handle = GetRangedShotSpecHandle();
    if (!HasAuthority() || CombatTeam != EETCombatTeam::Enemy || !Handle.IsValid() ||
        !IsValid(Target) || !Target->IsGameplayReady() || Target->IsCombatDead() ||
        Target->GetWorld() != GetWorld() || !CanDealCombatDamageTo(Target))
    {
        return false;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
    if (!ASC->AbilityActorInfo.IsValid() || !Spec || Spec->IsActive() ||
        !IsValid(Spec->Ability) || Spec->Ability->GetClass() != RangedShotAbilityClass.Get())
    {
        return false;
    }

    // 只把本次目标交给该身体实际授予的远程能力。
    FGameplayEventData Event;
    Event.EventTag = ETGameplayTags::Event_RangedShot;
    Event.Instigator = this;
    Event.Target = Target;
    return ASC->TriggerAbilityFromGameplayEvent(Handle, ASC->AbilityActorInfo.Get(), Event.EventTag, &Event, *ASC);
}
```

### 3.3 EndPlay清理

找到`EndPlay`里清除LightAttack句柄的`if`块。在完整块后、Dodge句柄块前插入。

```cpp
        if (RangedShotAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(RangedShotAbilityHandle);
        }
```

在`EndPlay`末尾`LightAttackAbilityHandle = FGameplayAbilitySpecHandle();`后增加一行。

```cpp
    RangedShotAbilityHandle = FGameplayAbilitySpecHandle();
```

## 4. 更新既有能力专项

整份替换`EmberTrial/Source/EmberTrial/Tests/ETRangedShotAbilityTests.cpp`。测试配置C3保存的GA类，然后使用实际授予与请求路径。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_RangedShot.h"

#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRangedShotAbilityTest, "EmberTrial.M4.Combat.RangedShotAbility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRangedShotAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* RangedClass = LoadClass<UETGA_RangedShot>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_RangedShot.GA_ET_RangedShot_C"));
    UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr,
        TEXT("/Game/EmberTrial/Animations/Ranged/Montage/AM_ET_RangedShot_01.AM_ET_RangedShot_01"));
    UClass* ProjectileClass = LoadClass<AETCombatProjectile>(nullptr,
        TEXT("/Game/EmberTrial/Combat/Projectiles/BP_ETRangedProjectile.BP_ETRangedProjectile_C"));
    UClass* DamageClass = LoadClass<UGameplayEffect>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_LightAttackDamage.GE_ET_LightAttackDamage_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    if (!TestNotNull(TEXT("Saved ranged GA"), RangedClass) ||
        !TestNotNull(TEXT("Saved ranged montage"), Montage) ||
        !TestNotNull(TEXT("Saved projectile Blueprint"), ProjectileClass) ||
        !TestNotNull(TEXT("Health damage GE"), DamageClass) ||
        !TestNotNull(TEXT("Quinn"), Quinn) || !TestNotNull(TEXT("Hit reaction"), HitClass))
    {
        return false;
    }

    const UETGA_RangedShot* Defaults = RangedClass->GetDefaultObject<UETGA_RangedShot>();
    if (!TestTrue(TEXT("Saved GA and montage satisfy native configuration"), Defaults->HasValidConfiguration()) ||
        !TestTrue(TEXT("Saved GA references the dedicated montage"), Defaults->CastMontage == Montage) ||
        !TestTrue(TEXT("Saved GA references the projectile Blueprint and damage GE"),
                  Defaults->ShotConfig.ProjectileClass.Get() == ProjectileClass &&
                      Defaults->ShotConfig.DamageEffectClass.Get() == DamageClass) ||
        !TestEqual(TEXT("Release is at 0.40 seconds"), Montage->Notifies[0].GetTime(), .40f, .01f) ||
        !TestEqual(TEXT("Saved damage"), Defaults->ShotConfig.Damage, 12.0f) ||
        !TestEqual(TEXT("Saved minimum range"), Defaults->ShotConfig.MinRange, 250.0f) ||
        !TestEqual(TEXT("Saved maximum range"), Defaults->ShotConfig.MaxRange, 1200.0f))
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
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>();
    AEmberTrialCharacter* Target = World->SpawnActor<AEmberTrialCharacter>(
        FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
    AAIController* Controller = World->SpawnActor<AAIController>();
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Shooter"), Body) || !TestNotNull(TEXT("Target"), Target) ||
        !TestNotNull(TEXT("AI controller"), Controller) || !TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    Body->CombatTeam = EETCombatTeam::Enemy;
    Target->CombatTeam = EETCombatTeam::Player;
    Body->RangedShotAbilityClass = RangedClass;
    Target->RangedShotAbilityClass = RangedClass;
    TestFalse(TEXT("Request before BeginPlay is rejected"), Body->RequestRangedShot(Target));
    Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
    Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Body->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Controller->Possess(Body);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    for (AEmberTrialCharacter* Character : {Body, Target})
    {
        Character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Character->GetCharacterMovement()->SetComponentTickEnabled(false);
    }

    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const FGameplayAbilitySpecHandle Ranged = Body->GetRangedShotSpecHandle();
    if (!TestTrue(TEXT("Enemy BeginPlay granted the saved ranged ability"), Ranged.IsValid()))
    {
        return false;
    }
    TestFalse(TEXT("Player is not granted an enemy ranged ability"), Target->GetRangedShotSpecHandle().IsValid());
    const FGameplayAbilitySpecHandle Hit = ASC->GiveAbility(FGameplayAbilitySpec(HitClass));
    const FGameplayAbilitySpec* RangedSpec = ASC->FindAbilitySpecFromHandle(Ranged);
    UETGA_RangedShot* RangedAbility =
        RangedSpec ? Cast<UETGA_RangedShot>(RangedSpec->GetPrimaryInstance()) : nullptr;
    if (!TestNotNull(TEXT("Granted ranged ability instance"), RangedAbility))
    {
        return false;
    }
    if (!TestTrue(TEXT("Granted instance inherited saved asset configuration"),
                  RangedAbility->HasValidConfiguration() &&
                      RangedAbility->CanActivateAbility(Ranged, ASC->AbilityActorInfo.Get())))
    {
        return false;
    }

    const auto Request = [Body](AEmberTrialCharacter* Aim)
    {
        return Body->RequestRangedShot(Aim) ? 1 : 0;
    };
    const auto Advance = [&Fixture](float Seconds)
    {
        for (int32 Index = 0; Index < FMath::CeilToInt(Seconds / .01f); ++Index)
        {
            if (!Fixture.TickTestWorld(.01f))
            {
                return false;
            }
        }
        return true;
    };
    const auto IsCasting = [ASC, Ranged]()
    {
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Ranged);
        return Spec && Spec->IsActive();
    };
    const auto IsCooling = [ASC]()
    {
        return ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_RangedShot);
    };

    TestEqual(TEXT("Missing target event rejects"), Request(nullptr), 0);
    TestFalse(TEXT("Rejected request has no cooldown"), IsCooling());
    if (!TestEqual(TEXT("Accept target event"), Request(Target), 1) ||
        !TestTrue(TEXT("Attacking tag at windup"),
                  IsCasting() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking)) ||
        !Advance(.12f))
    {
        return false;
    }
    TestTrue(TEXT("Real cooldown starts on commit"), IsCooling());
    TestEqual(TEXT("Duplicate event rejects"), Request(Target), 0);
    TestEqual(TEXT("No damage before release"), TargetASC->GetNumericAttribute(Health), 100.0f);
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestTrue(TEXT("Pause fixture active"), World->IsPaused());
    const bool bPausedTick = Advance(.6f);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    if (!bPausedTick)
    {
        return false;
    }
    TestEqual(TEXT("Pause did not release"), TargetASC->GetNumericAttribute(Health), 100.0f);
    if (!Advance(.34f))
    {
        return false;
    }
    TActorIterator<AETCombatProjectile> InFlight(World);
    AETCombatProjectile* Shot = InFlight ? *InFlight : nullptr;
    const UStaticMeshComponent* Visual = IsValid(Shot) ? Shot->FindComponentByClass<UStaticMeshComponent>() : nullptr;
    if (!TestTrue(TEXT("Release spawned the saved projectile Blueprint"),
                  IsValid(Shot) && Shot->GetClass() == ProjectileClass) ||
        !TestTrue(TEXT("Projectile visual exists and has collision disabled"),
                  IsValid(Visual) && IsValid(Visual->GetStaticMesh()) && Visual->IsVisible() &&
                      Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision))
    {
        return false;
    }
    const TWeakObjectPtr<AETCombatProjectile> WeakShot(Shot);
    TestEqual(TEXT("Projectile is in flight before damage"), TargetASC->GetNumericAttribute(Health), 100.0f);
    if (!Advance(.86f))
    {
        return false;
    }
    TestFalse(TEXT("Projectile destroyed after blocking hit"), WeakShot.IsValid());
    TestEqual(TEXT("One release hit after resume"), TargetASC->GetNumericAttribute(Health), 88.0f);
    TestEqual(TEXT("Health base after one release"), TargetASC->GetNumericAttributeBase(Health), 88.0f);
    TestFalse(TEXT("Montage and attacking tag ended"),
              IsCasting() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
    TestTrue(TEXT("Cooldown outlives montage"), IsCooling());
    TestEqual(TEXT("Cooldown rejects after montage"), Request(Target), 0);
    if (!Advance(2.5f))
    {
        return false;
    }
    TestFalse(TEXT("Cooldown expires"), IsCooling());

    if (!TestEqual(TEXT("Next cast accepted"), Request(Target), 1) || !Advance(.12f) ||
        !TestTrue(TEXT("Actual HitReact interrupts ranged windup"), ASC->TryActivateAbility(Hit, false)))
    {
        return false;
    }
    TestFalse(TEXT("Interrupted cast releases attacking tag"),
              IsCasting() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
    TestTrue(TEXT("Interrupted windup keeps cooldown"), IsCooling());
    if (!Advance(1.0f))
    {
        return false;
    }
    TestEqual(TEXT("Interrupted windup never fires"), TargetASC->GetNumericAttribute(Health), 88.0f);
    if (!Advance(2.5f) || !TestEqual(TEXT("Third windup accepted"), Request(Target), 1))
    {
        return false;
    }
    Target->SetActorLocation(FVector(1600.0, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
    if (!Advance(1.2f))
    {
        return false;
    }
    TestEqual(TEXT("Target out of range by release takes no damage"),
              TargetASC->GetNumericAttribute(Health), 88.0f);
    TestTrue(TEXT("Whiff keeps cooldown"), IsCooling());

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay invalidates the body-owned ranged handle"), Body->GetRangedShotSpecHandle().IsValid());
    TestNull(TEXT("EndPlay removes the ranged spec from the ASC"), ASC->FindAbilitySpecFromHandle(Ranged));
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 5. 构建与验收

隔离工程已用C3实际提交的三份资产通过Editor Development构建及47项全组自动化。验收报告保存在`Artifacts/M4_01D1/AutomationReport/index.json`。正式工程按以下步骤验收。

在UE关闭时，从仓库根目录依次执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

```powershell
Set-Location 'E:\UE_Project\Demo'
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

在CLion重载CMake。打开UE后，在Output Log依次执行。

```text
Automation RunTests EmberTrial.M4.Combat.RangedShot
Automation RunTests EmberTrial.M4.Combat.Projectile
```

| 检查 | 预期 |
| --- | --- |
| 授予与请求 | BeginPlay前请求拒绝。敌方身体自动获得远程GA。测试玩家即使填了同一GA也无远程Spec |
| 发射与结束 | 角色原生请求从保存的Montage发射，命中后生命100→88。冷却、受击打断及超距拒绝继续通过。EndPlay清除Spec和句柄 |
| 地图短查 | `L_ET_VerticalSlice`中移动、近战和P暂停恢复通过，随后停止PIE |

反馈三个文件、双Build与索引、四项自动化、地图短查、保存状态、手动工时与构建等待。失败时贴首条编译错误，或专项名称和首条`Expected`。

## 6. Commit / Push

先Stage本轮验收、资产来源和指南文档。

```text
Docs/Reports/M4_01C3_Verification.md
Docs/M4_01C3_RangedShotAssets.md
Docs/M4_01D1_RangedGuardAbilityGrant.md
Docs/AssetManifest.csv
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

- 英文 `docs: verify ranged assets and plan guard ability grant`
- 中文 `docs: 完成远程资产验收并规划守卫能力授予`

手动实现和验收后，再Stage三份源码。

```text
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Tests/ETRangedShotAbilityTests.cpp
```

- 英文 `feat: grant ranged shot through the enemy character lifecycle`
- 中文 `feat: 在敌方角色生命周期内授予和请求远程攻击`
