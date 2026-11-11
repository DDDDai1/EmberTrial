# M4-01C3 远程守卫的蒙太奇、弹体与能力资产

本批把已验收的远程能力接到三份可保存资产，并把既有专项切到真实资源。预计手动40–60分钟，构建等待另记。[M4-01C2验收](Reports/M4_01C2_Verification.md)

## 1. 本批交付

| 资产 | 路径与职责 |
| --- | --- |
| `AM_ET_RangedShot_01` | `/Game/EmberTrial/Animations/Ranged/Montage`。复用现有破势动作作为占位，仅保留远程发射通知 |
| `BP_ETRangedProjectile` | `/Game/EmberTrial/Combat/Projectiles`。原生投射物加一颗可见球体，碰撞仍由继承的球体负责 |
| `GA_ET_RangedShot` | `/Game/EmberTrial/Abilities`。绑定远程Montage、投射物BP和已有伤害GE |

测试仍在隔离World中授予远程能力。本批验证可保存资产到真实发射的链路。下一批接入守卫的感知、攻击距离和行为树。

## 2. UE编辑器资产配置

先在当前C2构建上完成三份资产。保存并重开资源核对路径后，再关UE修改第3节的测试。若资产已经按要求创建，可直接核对并复用。

### 2.1 远程Montage

1. 在Content Browser创建目录`/Game/EmberTrial/Animations/Ranged/Montage`。找到`/Game/EmberTrial/Animations/BreakPulse/Montage/AM_ET_BreakPulse_01`，右键 **Duplicate**，命名`AM_ET_RangedShot_01`。将新副本拖到Ranged的Montage目录并选择 **Move Here**。若原目录出现该副本的Redirector，在原目录执行 **Fix Up Redirectors in Folder**。
2. 打开远程副本，在Notifies子轨道上选中旧 **ET Break Pulse Commit** 点并删除。随后在轨道空白处右键，选 **Add Notify → ET Ranged Shot Release**。资源类名为`ETAN_RangedShotRelease`。
3. 右键**新通知点本体**，在 **Notify Begin Time** 输入`0.400000`后回车。30 fps对应第12帧。左键选中时若Details提供 **Montage Tick Type**，选 **Branching Point**。原生通知也会强制分支点。保留Trigger Chance 1和No Filtering。
4. 确认只剩一条发射点。保留模板的单段`DefaultSlot`、单Section、1倍速、Auto Blend Out和原混合参数。保存并重开，核对路径和0.40秒。原破势Montage及共享Animation Sequence保留各自的原配置。

### 2.2 可见弹体

1. 在`/Game/EmberTrial/Combat/Projectiles`新建 **Blueprint Class**。从 **All Classes** 搜索`ETCombatProjectile`并选为父类，命名`BP_ETRangedProjectile`。
2. 打开蓝图。在Components中选继承的`Collision`根组件，点 **Add → Static Mesh**，命名`ShotVisual`。确认它在Collision下面。Components树应同时有继承的Collision和ProjectileMovement。
3. 选ShotVisual，按下表配置。若看不到Engine资源，在Content Browser的Settings里开启 **Show Engine Content**，选择`/Engine/BasicShapes/Sphere`。可在Details的Static Mesh选择器中核对路径。

| 属性 | 值 |
| --- | --- |
| Static Mesh | `/Engine/BasicShapes/Sphere` |
| Relative Location / Rotation | 三轴0 |
| Relative Scale | 三轴0.20 |
| Mobility | Movable |
| Collision Presets | NoCollision |
| Can Ever Affect Navigation | False |
| Visible | True |

4. Compile、Save。Class Defaults中的原生Launch Speed 1600 cm/s和Flight Lifetime 3 s保留。

### 2.3 远程能力GA

1. 在`/Game/EmberTrial/Abilities`新建Blueprint Class，从 **All Classes** 搜索`ETGA_RangedShot`并选择。命名`GA_ET_RangedShot`。
2. 打开后点 **Class Defaults**，在`EmberTrial / Ranged Shot`下设置下表。折叠字段 **Shot Config** 需要展开。

| 属性 | 值 |
| --- | --- |
| Cast Montage | `AM_ET_RangedShot_01` |
| Shot Config / Projectile Class | `BP_ETRangedProjectile` |
| Shot Config / Damage Effect Class | `GE_ET_LightAttackDamage` |
| Shot Config / Damage | 12 |
| Shot Config / Min Range | 250 |
| Shot Config / Max Range | 1200 |

3. 保留原生Cooldown Gameplay Effect Class为`ETRangedShotCooldownEffect`，Cost为None，默认触发事件为`Event.Combat.RangedShot`。EventGraph保持空。Compile、Save，再执行Save All。重开Class Defaults核对引用均已持久化。

## 3. 替换既有专项

保存并关闭UE编辑器。在CLion打开`EmberTrial/Source/EmberTrial/Tests/ETRangedShotAbilityTests.cpp`，整份替换为下面代码。此专项现在直接加载三份已保存资产，不需要新建自动化入口。

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
    const FGameplayAbilitySpecHandle Ranged = ASC->GiveAbility(FGameplayAbilitySpec(RangedClass));
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

    const auto Request = [ASC, Body](AActor* Aim)
    {
        FGameplayEventData Event;
        Event.EventTag = ETGameplayTags::Event_RangedShot;
        Event.Instigator = Body;
        Event.Target = Aim;
        return ASC->HandleGameplayEvent(Event.EventTag, &Event);
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
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 4. 正常构建与验收

隔离工程已构建Editor Development，并创建三份测试资产。另启UE进程从磁盘加载后，47项自动化全部Success。正式工程的资产由你按第2节配置。隔离报告保存在`Artifacts/M4_01C3/AutomationReport/index.json`。

在UE关闭时，从仓库根目录依次执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

```powershell
Set-Location 'E:\UE_Project\Demo'
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

重新打开UE和三个资产，核对引用。先在Output Log依次执行下列两条，分别等到两项Success。

```text
Automation RunTests EmberTrial.M4.Combat.RangedShot
Automation RunTests EmberTrial.M4.Combat.Projectile
```

| 检查 | 预期 |
| --- | --- |
| 保存与实例 | GA CDO读取正式Montage、弹体类和伤害GE。授予后的实例直接继承配置，通知时刻0.40秒 |
| 飞行与命中 | 出弹后尚未命中时目标仍100。测试世界中弹体是保存的BP类，具有可见静态网格且网格碰撞关闭。命中后Current和Base均88，弹体销毁 |
| 起手与回归 | 暂停、冷却、受击中断、超出射程及三个既有飞行／发射专项通过 |
| 正式地图 | 打开`L_ET_VerticalSlice`，Play后确认移动和P暂停恢复，再Stop。本批仍以近战地图为日常短查 |

反馈三资产路径、重开引用、双Build与索引、四项自动化、地图短查、保存状态、手动工时与等待。若报错贴首条编译错误，或专项名和首条`Expected`。

## 5. Commit / Push

先Stage本轮验收和指南文档。

```text
Docs/Reports/M4_01C2_Verification.md
Docs/M4_01C2_RangedShotAbility.md
Docs/M4_01C3_RangedShotAssets.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Introduction.md
README.md
```

- 英文 `docs: verify ranged casting and plan saved assets`
- 中文 `docs: 完成远程施放验收并规划正式资产`

三资产保存并验收后，再Stage以下文件。三份`.uasset`由现有Git LFS规则跟踪。

```text
EmberTrial/Source/EmberTrial/Tests/ETRangedShotAbilityTests.cpp
EmberTrial/Content/EmberTrial/Animations/Ranged/Montage/AM_ET_RangedShot_01.uasset
EmberTrial/Content/EmberTrial/Combat/Projectiles/BP_ETRangedProjectile.uasset
EmberTrial/Content/EmberTrial/Abilities/GA_ET_RangedShot.uasset
```

- 英文 `feat: author ranged shot assets and verify saved defaults`
- 中文 `feat: 配置远程发射资产并验证已保存默认值`

占位动作和引擎球体的来源、复用关系及实际资产指纹在下一轮验收时登记。隔离工程及其夹具留在本地`Artifacts`。
