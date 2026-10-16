# M2-03C：恢复阶段闪避取消与战斗集成

2026-10-16，固定 **UE5.5.4**。前置[B已验收](Reports/M2_03B_Verification.md)，当前99Source / 52cpp / 来源58项；1→2→3已成立，攻击期间2仍拒绝。**本批只生成手动操作指南，尚未写入Source / 修改资产或运行Build / 测试。**一新 / 七旧Source，实施后预计100Source / 53cpp，来源仍58，无新UE资产。

全部C++仍由你手动录入。预计手动 **90–150分钟**：代码 / 理解60–100、资产 / 输入核对10–20、相关检查10–15、记录5–10，构建等待另列，可分两次完成。原路线C的60–100分钟上调为90–150，原因是本批补齐预检失败保留原攻击、保存标签默认值校验和可复用真实交接测试；不是新增旧功能验收矩阵。整章10h基础预算仍保持，未计本批实耗。

## 1. 交付规则与实现分工

| 情况 | 结果 |
| --- | --- |
| 任一段前摇 / 命中期间按2 | 拒绝，不扣体力、不缓存闪避，原攻击继续；需到恢复段重新按 |
| 任一段命中结束后、自动淡出开始前按2 | 通过Dodge完整激活预检后取消当前轻攻击，再启动原正式闪避 |
| 第1 / 2段命中8–14帧 | 恢复从14/30约.466667秒开始；到各自Montage自动淡出前结束 |
| 第3段命中6–12帧 | 恢复从12/30=.400000秒开始；同样到自动淡出前结束 |
| 体力不足 / Montage None / 位移参数非法 / 非地面 / 暂停 / 受击 / 死亡 | 激活预检拒绝，不先取消原攻击、不扣成本 |
| 2与已缓存的下一段同处一个恢复时段 | 2若在待切段Timer调度前被接受，清旧输入 / Timer；若下一段已实际开始，则按新段的恢复状态重新判定 |
| 交接成功 | 旧Attacking / Trace / 输入 / Task清理，Dodge只扣一次25，沿用150cm / .30秒与.15秒无敌 |
| Dodge再被3 / 死亡 / Stop取消 | 沿用原清理，不退款、不补播旧连击；下一次左键从1开始 |

恢复时段直接读取**当前Montage唯一命中NotifyState的结束时间**，不另建Notify类、Delay或配置数组。若未来修改命中时间，恢复起点随真实通知移动；自动淡出阶段只等原能力完成，不缓存2。窗口依据Montage位置，不能把渲染帧率或墙钟秒数当动画帧号。

LightAttack只提供无副作用的`CanCancelIntoDodge()`：当前玩家 / 控制权 / Montage实例必须匹配、能力可取消、位置处于恢复时段。Dodge的CanActivate保留成本 / 冷却 / 地面 / 资源 / 输入与配置检查，只在该查询为真时允许原Attacking共存。通过检查后，GAS自身`CancelAbilitiesWithTag=Ability.Attack.Light`在PreActivate里取消原攻击；不在Character或蓝图里先Cancel再“试试看”。

因此Dodge构造中移除Attacking这一条硬阻断，**不等于任意攻击阶段都允许闪避**：原生CanActivate仍有恢复门控；Dodging / HitReact / Dead继续阻断。Activate开头再核对原攻击确实结束、没有其他Montage所有者，之后复用原Montage / RMS准备、一次付款与保护逻辑。普通空闲闪避也继续使用同一入口。

明确边界：预检拒绝不取消原攻击；若预检已通过、交接已开始后发生底层播放失败或同步取消，清理本次准备资源，不自动复活旧攻击。尚未提交则不扣费，成功付款后取消仍不退款；不能将这段流程说成支持所有异常回滚的事务。

接口与顺序已按本机5.5.4的GameplayAbility.cpp `PreActivate` / `CallActivateAbility`、ASC `InternalTryActivateAbility`及AnimInstance.cpp核对；[Epic 5.5能力生命周期与标签说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine?application_version=5.5)提供公开契约。静态检查不代替本批实际编译 / 测试。

## 2. 准备与文件清单

Stop PIE → Save All → 关闭UE → CLion保存。完成第3–6节全部八文件，再第7节正常Build；不使用Live Coding处理反射类头文件变更。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Abilities\ETGA_LightAttack.h                   旧，加查询 / friend
├─ EmberTrial\Private\Abilities\ETGA_LightAttack.cpp                旧，加查询函数
├─ EmberTrial\Public\Abilities\ETGA_Dodge.h                         旧，加策略查询 / friend
├─ EmberTrial\Private\Abilities\ETGA_Dodge.cpp                      旧，五处修改
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h               旧，一行friend
├─ EmberTrial\Public\Combat\ETCombatLifeComponent.h                 旧，一行friend
├─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp 旧，加策略规则
└─ EmberTrial\Tests\ETLightComboCancelTests.cpp                       新，完整非UCLASS测试
```

不创建新C++类 / 组件 / GE / Notify / 输入资产；Character.cpp、Trace、位移Task、UI、Build.cs / Target / uproject不改。旧三段动画 / Notify时值保持，唯一需重点核对的是既有GA_ET_Dodge的标签默认值。所有旧friend、函数与属性保留。

## 3. LightAttack：公开恢复查询，不改原连段与伤害

### 3.1 ETGA_LightAttack.h 两处

路径见第2节Public/Abilities。第一处在public区：

**找到原内容：**

```cpp
bool TryBufferComboInput();
```

**替换为：**

```cpp
bool TryBufferComboInput();
    bool CanCancelIntoDodge() const;
```

第二处在private区，保留旧Flow friend：

**找到原内容：**

```cpp
friend class FETLightComboFlowTest;
```

**替换为：**

```cpp
friend class FETLightComboFlowTest;
    friend class FETLightComboCancelTest;
```

### 3.2 ETGA_LightAttack.cpp 插入完整查询

在该文件找到`void UETGA_LightAttack::ClearBufferedInput()`，将下面完整函数插在它**前面**。原ClearBufferedInput及其后函数保持；不要把新函数写进旧函数花括号里。本文件已有所需include，不再重复加。

```cpp
bool UETGA_LightAttack::CanCancelIntoDodge() const
{
    if (!IsActive() || !CanBeCanceled() || !bEnableCombo || !CurrentActorInfo ||
        bStartingStage || bEndingAttack || !RuntimeStages.IsValidIndex(ActiveStageIndex)) { return false; }
    const AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    const APlayerController* PC=IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UAbilitySystemComponent* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    const UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    const FAnimMontageInstance* Instance=IsValid(Anim) && IsValid(ActiveMontage)
        ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    if (!IsValid(Body) || Body->IsActorBeingDestroyed() || !Body->IsGameplayReady() ||
        Body->IsCombatDead() || !Body->HasAuthority() || Body->GetCombatTeam()!=EETCombatTeam::Player ||
        !IsValid(PC) || !PC->IsLocalController() || PC->GetPawn()!=Body || PC->IsMoveInputIgnored() ||
        CurrentActorInfo->OwnerActor.Get()!=Body || Body->GetAbilitySystemComponent()!=ASC ||
        !IsValid(ASC) || UGameplayStatics::IsGamePaused(Body) || ASC->GetAnimatingAbility()!=this ||
        ASC->GetCurrentMontage()!=ActiveMontage || !Instance ||
        Instance->GetInstanceID()!=ActiveMontageInstanceID ||
        Anim->GetCurrentActiveMontage()!=ActiveMontage || !Anim->Montage_IsPlaying(ActiveMontage)) { return false; }
    float HitEnd=0.0f;int32 Hits=0;
    for (const FAnimNotifyEvent& Event : ActiveMontage->Notifies)
    {
        if (IsValid(Event.NotifyStateClass) && Event.NotifyStateClass->IsA<UETANS_AttackWindow>())
        { ++Hits;HitEnd=Event.GetTime()+Event.GetDuration(); }
    }
    const float Position=Instance->GetPosition();
    const float RecoveryEnd=ActiveMontage->GetPlayLength()-ActiveMontage->BlendOut.GetBlendTime();
    return Hits==1 && FMath::IsFinite(HitEnd) && FMath::IsFinite(Position) &&
        FMath::IsFinite(RecoveryEnd) && HitEnd>0.0f && Position>=HitEnd && Position<RecoveryEnd;
}
```

它不复用`HasNextStage()`，所以第3段恢复也允许闪避；不清输入、不停Montage、不修改Tag。B的连段窗口.60–.80不改变，本查询的起点是命中结束，而非连段窗开启。

## 4. Dodge：预检后由GAS取消原攻击

### 4.1 ETGA_Dodge.h 两处

在public区：

**找到原内容：**

```cpp
bool HasValidInvulnerabilityConfiguration() const;
```

**替换为：**

```cpp
bool HasValidInvulnerabilityConfiguration() const;
    bool HasValidRecoveryCancelPolicy() const;
```

在private区：

**找到原内容：**

```cpp
friend class FETDodgeInvulnerabilityTest;
```

**替换为：**

```cpp
friend class FETDodgeInvulnerabilityTest;
    friend class FETLightComboCancelTest;
```

### 4.2 ETGA_Dodge.cpp include与构造策略

第一处，文件开头：

**找到原内容：**

```cpp
#include "Abilities/ETGA_Dodge.h"
```

**替换为：**

```cpp
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_LightAttack.h"
```

第二处，**仅替换构造函数中的这一行**，Dodging / HitReact / Dead三行保留，不把它们清空：

**找到原内容：**

```cpp
ActivationBlockedTags.AddTag(ETGameplayTags::State_Attacking);
```

**替换为：**

```cpp
// The native CanActivateAbility gate permits only our player's recovery window.
    // GAS executes this cancellation only after all activation prechecks pass.
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Attack_Light);
```

`Ability.Attack.Light`是能力身份标签，`State.Attacking`是状态标签，两者用途不同。Cancel容器只填前者，不填State或Ability.Dodge。

### 4.3 插入策略查询

在`bool UETGA_Dodge::CanActivateAbility(`前面插入下面完整函数。ReadCostAmount / CheckCost / ApplyCost / 位移与无敌函数保持。

```cpp
bool UETGA_Dodge::HasValidRecoveryCancelPolicy() const
{
    return CancelAbilitiesWithTag.Num()==1 &&
        CancelAbilitiesWithTag.HasTagExact(ETGameplayTags::Ability_Attack_Light) &&
        !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Attacking) &&
        ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dodging) &&
        ActivationBlockedTags.HasTagExact(ETGameplayTags::State_HitReact) &&
        ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dead);
}
```

### 4.4 整体替换 CanActivateAbility

从该函数签名开始到对应结束`}`，用下面**完整函数**替换。不是替换ActivateAbility，也不重复保留两个同名函数。

```cpp
bool UETGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !HasValidRecoveryCancelPolicy() ||
        !CheckCost(Handle, ActorInfo, OptionalRelevantTags) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    { return false; }
    AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    APlayerController* PC=IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UCharacterMovementComponent* Movement=IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice=IsValid(Body)
        ? Body->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    UAbilitySystemComponent* ASC=ActorInfo->AbilitySystemComponent.Get();
    const FGameplayAbilitySpec* AttackSpec=IsValid(Body) && IsValid(ASC)
        ? ASC->FindAbilitySpecFromHandle(Body->GetLightAttackSpecHandle()) : nullptr;
    const UETGA_LightAttack* Attack=AttackSpec && AttackSpec->IsActive()
        ? Cast<UETGA_LightAttack>(AttackSpec->GetPrimaryInstance()) : nullptr;
    const bool bRecovery=IsValid(Attack) && Attack->CanCancelIntoDodge();
    UAnimInstance* Anim=ActorInfo->GetAnimInstance();
    return IsValid(Body) && Body->GetCombatTeam()==EETCombatTeam::Player &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn()==Body && !PC->IsMoveInputIgnored() &&
        (Body->IsFormalActionIdle() || bRecovery) && (!Practice || Practice->IsPracticeActionIdle()) &&
        IsValid(Movement) && Movement->IsMovingOnGround() && !Movement->HasRootMotionSources() &&
        !Body->GetVelocity().ContainsNaN() && HasValidMovementConfiguration() &&
        HasValidInvulnerabilityConfiguration() && !Body->GetDodgeWorldDirection().IsNearlyZero() &&
        IsValid(Anim) && (!Anim->GetCurrentActiveMontage() || bRecovery);
}
```

### 4.5 整体替换 ActivateAbility

同样只替换下面这个完整函数。旧EndAbility、四个完成 / 取消回调、成本与无敌实现均保留；下面已经包含原Montage / 位移 / 扣费逻辑，不再额外粘一遍。

```cpp
void UETGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    if (!IsActive()) { return; } // PreActivate cancellation may synchronously end us.
    const uint32 Serial = ++ActivationSerial;
    bCostSubmitted = false;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    const FVector Direction = IsValid(Body) ? Body->GetDodgeWorldDirection() : FVector::ZeroVector;
    UCharacterMovementComponent* Movement = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    // GAS PreActivate already attempted cancellation. Do not overwrite an uncancelled action.
    if (!IsValid(Body) || !IsValid(ASC) || !HasValidMovementConfiguration() ||
        !HasValidInvulnerabilityConfiguration() || !HasValidRecoveryCancelPolicy() ||
        Body->IsCombatDead() || UGameplayStatics::IsGamePaused(Body) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead) || ASC->GetAnimatingAbility()!=nullptr ||
        Direction.IsNearlyZero() || !IsValid(Movement) || !Movement->IsMovingOnGround() ||
        Movement->HasRootMotionSources() ||
        !Body->GetGameplayStaminaSnapshot(Before, MaxStamina) ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    Movement->StopMovementImmediately();
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, DodgeMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!MontageTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->ReadyForActivation();
    if (Serial != ActivationSerial || !IsActive()) { return; }
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    if (!IsValid(Anim) || !Anim->GetActiveInstanceForMontage(DodgeMontage) ||
        ASC->GetAnimatingAbility() != this || !Anim->Montage_IsPlaying(DodgeMontage))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    // Prepare our source before charging. No movement tick runs inside this synchronous call.
    MoveTask = UETAbilityTask_DodgeMove::CreateDodgeMove(this, Direction, DodgeDistance, DodgeDuration);
    if (!MoveTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MoveTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMoveCompleted);
    MoveTask->OnAborted.AddDynamic(this, &ThisClass::HandleMoveAborted);
    MoveTask->ReadyForActivation();
    if (Serial != ActivationSerial || !IsActive()) { return; }
    if (!IsValid(MoveTask) || !MoveTask->IsMovementReady())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    // Montage and source exist. Commit once; cancellation cleans the prepared source.
    const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
    if (Serial != ActivationSerial || !IsActive()) { return; }
    float After = 0.0f;
    if (!bCommitted || !bCostSubmitted || Body->IsCombatDead() ||
        !Body->GetGameplayStaminaSnapshot(After, MaxStamina) ||
        !FMath::IsNearlyEqual(Before - After, Cost, 0.001f))
    {
        UE_LOG(LogETDodge, Warning, TEXT("[M2-02A] Commit failed Body=%s Submitted=%d"),
            *GetNameSafe(Body), bCostSubmitted ? 1 : 0);
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    const bool bWindowStarted = StartInvulnerabilityWindow(ASC, Serial);
    if (Serial != ActivationSerial || !IsActive()) { return; }
    if (!bWindowStarted)
    {
        UE_LOG(LogETDodge, Warning, TEXT("[M2-02C] Window application failed after payment Body=%s"),
            *GetNameSafe(Body));
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] Begin Body=%s Serial=%u Before=%.1f After=%.1f Cost=%.1f Dodging=%d"),
        *GetNameSafe(Body), Serial, Before, After, Cost, ASC->GetTagCount(ETGameplayTags::State_Dodging));
}
```

## 5. 两行测试friend与Editor资产规则

### 5.1 Character.h

路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h`。找到已有`friend class FETLightComboFlowTest;`，下一行新增：

```cpp
friend class FETLightComboCancelTest;
```

保留所有旧friend；Character.cpp不改。测试只用它给临时身体选择正式Attack / Dodge类、体力初值，不新增蓝图测试变量。

### 5.2 CombatLife.h

路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Combat\ETCombatLifeComponent.h`。在同一个旧Flow friend下一行新增**同一行**`friend class FETLightComboCancelTest;`，不改cpp；供临时身体选择已有正式受击类。

### 5.3 Validator.cpp：新增ET-DODGE-004

路径见第2节Editor/Private/Validation。在`ValidateAbility`的Dodge分支，找到这一行：

```cpp
if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidInvulnerabilityConfiguration())
```

将下面整段插在该行**前面**；原无敌if、ET-DODGE-001 / 002 / 003和后面的Combo规则全部保留。本文件已经有Dodge头文件，不新增依赖。

```cpp
if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidRecoveryCancelPolicy())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-004"),
                TEXT("Dodge must cancel only Ability.Attack.Light; do not block State.Attacking; keep Dodging/HitReact/Dead blocked."), Issues);
        }
```

## 6. 新建可复用的真实交接自动化

新文件`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETLightComboCancelTests.cpp`，完整内容如下。它不是UCLASS，不加generated.h或Build.cs条目，旧Flow / Dodge测试不删除。

测试加载已保存的GA / Montage，用真实通知、Trace / GE与RMS。目标跟随手部只隔离几何，不伪造HitResult或广播正向命中；旧Task回调是负向注入。自动推进到实际恢复时段，检查三段均可交接、预检失败保留原攻击、待切段被2清除、Pause / 受击 / 死亡 / EndPlay；EndPlay按UE真实协议检查本次Source已标记移除 / Task已释放，不要求世界结束后再Tick才能让容器变空。无需你手赶.15 / .25秒。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/Tasks/ETAbilityTask_DodgeMove.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "UObject/StrongObjectPtr.h"
#include "Animation/ETANS_ComboWindow.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayAbilitySpec.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETLightComboCancelTest,
    "EmberTrial.M2.Combo.RecoveryDodge",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETLightComboCancelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* AttackClass=LoadClass<UETGA_LightAttack>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass=LoadClass<UETGA_Dodge>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    UClass* HitClass=LoadClass<UETGA_HitReact>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    USkeletalMesh* Quinn=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved player GA"),AttackClass) || !TestNotNull(TEXT("Saved dodge GA"),DodgeClass) ||
        !TestNotNull(TEXT("Hit GA"),HitClass) || !TestNotNull(TEXT("Quinn"),Quinn)) { return false; }
    const UETGA_LightAttack* Defaults=AttackClass->GetDefaultObject<UETGA_LightAttack>();
    if (!TestTrue(TEXT("Player combo configured"),Defaults->IsComboEnabled() && Defaults->HasValidComboConfiguration()) ||
        !TestTrue(TEXT("Saved recovery cancellation policy"),DodgeClass->GetDefaultObject<UETGA_Dodge>()->HasValidRecoveryCancelPolicy())) { return false; }

    for (const bool bEndPlayOnly : {false,true})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this);return false; }
        UWorld* World=Fixture.GetTestWorld();
        AActor* Floor=World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Floor"),Floor)) { return false; }
        UBoxComponent* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
        Box->SetMobility(EComponentMobility::Static);Box->SetBoxExtent(FVector(2000,2000,50));
        Box->SetRelativeLocation(FVector(0,0,-50));Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
        auto SpawnBody=[World,Quinn]()
        {
            FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AEmberTrialCharacter* Body=World->SpawnActor<AEmberTrialCharacter>(FVector(0,0,200),FRotator::ZeroRotator,P);
            if (!Body) { return Body; }
            Body->GetMesh()->SetSkeletalMeshAsset(Quinn);Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
            Body->GetMesh()->SetRelativeLocation(FVector(0,0,-90));Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Body->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
            Body->GetCharacterMovement()->bRunPhysicsWithNoController=true;
            return Body;
        };
        AEmberTrialCharacter* Body=SpawnBody();AEmberTrialCharacter* Target=SpawnBody();
        APlayerController* PC=World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Player body"),Body) || !TestNotNull(TEXT("Target"),Target) || !TestNotNull(TEXT("Controller"),PC)) { return false; }
        Body->CombatTeam=EETCombatTeam::Player;Body->LightAttackAbilityClass=AttackClass;Body->DodgeAbilityClass=DodgeClass;
        Body->CombatLifeComponent->HitReactAbilityClass=HitClass;
        Target->CombatTeam=EETCombatTeam::Neutral;Target->InitialMaxHealth=1000;Target->InitialHealth=1000;
        Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Overlap);
        PC->Possess(Body);
        if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        auto Step=[&Fixture,Body,Target]()
        {
            // Geometry fixture: stay in reach of each different right-hand pose.
            // This still uses real montage notifies, world sweeps and the production GE callback.
            const FVector Hand=Body->GetMesh()->GetSocketLocation(FName(TEXT("hand_r")));
            Target->SetActorLocation(FVector(Hand.X,Hand.Y,Target->GetActorLocation().Z),false,nullptr,ETeleportType::TeleportPhysics);
            return Fixture.TickTestWorld(.01f);
        };
        auto Advance=[&Step](float Seconds)
        {
            for (int32 I=0;I<FMath::CeilToInt(Seconds/.01f);++I) { if (!Step()) { return false; } }
            return true;
        };
        auto Until=[&Step](auto Predicate,float Limit)
        {
            for (int32 I=0;I<FMath::CeilToInt(Limit/.01f) && !Predicate();++I) { if (!Step()) { return false; } }
            return Predicate();
        };
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);Target->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) || !TestTrue(TEXT("Ready grounded body"),Body->IsGameplayReady() && Target->IsGameplayReady() &&
            Body->GetCharacterMovement()->IsMovingOnGround())) { return false; }
        UAbilitySystemComponent* ASC=Body->GetAbilitySystemComponent();
        UAbilitySystemComponent* TargetASC=Target->GetAbilitySystemComponent();
        FGameplayAbilitySpec* Spec=ASC->FindAbilitySpecFromHandle(Body->GetLightAttackSpecHandle());
        UETGA_LightAttack* Attack=Spec ? Cast<UETGA_LightAttack>(Spec->GetPrimaryInstance()) : nullptr;
        UETCombatTraceComponent* Trace=Body->FindComponentByClass<UETCombatTraceComponent>();
        if (!TestNotNull(TEXT("Granted attack instance"),Attack) || !TestNotNull(TEXT("Trace"),Trace)) { return false; }
        FGameplayAbilitySpec* DodgeSpec=ASC->FindAbilitySpecFromHandle(Body->DodgeAbilityHandle);
        UETGA_Dodge* Dodge=DodgeSpec ? Cast<UETGA_Dodge>(DodgeSpec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Granted dodge instance"),Dodge)) { return false; }
        const FGameplayAttribute Health=UETHealthAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Stamina=UETStaminaAttributeSet::GetStaminaAttribute();
        TMap<uint32,int32> Candidates;
        const FDelegateHandle Watch=Trace->OnTraceHit.AddLambda([&Candidates,Target](uint32 Serial,const FHitResult& Hit)
            { if (Hit.GetActor()==Target) { ++Candidates.FindOrAdd(Serial); } });
        auto Position=[Attack,Body]()
        {
            UAnimInstance* Anim=Body->GetMesh()->GetAnimInstance();
            FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetMontageInstanceForID(Attack->ActiveMontageInstanceID) : nullptr;
            return Instance ? Instance->GetPosition() : -1.0f;
        };
        auto Start=[this,Body,Attack]()
        { return TestTrue(TEXT("Start fresh attack"),Body->RequestLightAttack()) && TestTrue(TEXT("Starts stage one"),Attack->ActiveStageIndex==0); };
        auto Idle=[&Until,&Advance,Attack]()
        { return Until([Attack](){return !Attack->IsActive();},2.0f) && Advance(.25f); };
        auto Reset=[&Candidates,TargetASC,Health,ASC,Stamina]()
        { Candidates.Reset();TargetASC->SetNumericAttributeBase(Health,1000.0f);ASC->SetNumericAttributeBase(Stamina,100.0f); };
        auto CheckDamage=[this,&Candidates,TargetASC,Health](int32 Count)
        {
            TestEqual(TEXT("Unique serial count"),Candidates.Num(),Count);
            for (const auto& Pair : Candidates) { TestEqual(TEXT("One candidate per serial"),Pair.Value,1); }
            TestEqual(TEXT("Real current health"),TargetASC->GetNumericAttribute(Health),1000.0f-25.0f*Count);
            TestEqual(TEXT("Real base health"),TargetASC->GetNumericAttributeBase(Health),1000.0f-25.0f*Count);
        };
        auto Settle=[&Until,&Advance,Attack,ASC]()
        {
            return Until([Attack,ASC](){return !Attack->IsActive() &&
                !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) &&
                !ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact);},3.0f) && Advance(.25f);
        };
        auto EnterRecovery=[&Start,&Until,&Position,Body,Attack](int32 Stage)
        {
            if (!Start()) { return false; }
            for (int32 I=0;I<Stage;++I)
            {
                if (!Until([&Position](){return Position()>=.48f;},1.0f) || !Body->RequestLightAttack() ||
                    !Until([Attack,I](){return Attack->ActiveStageIndex==I+1;},.6f)) { return false; }
            }
            return Until([Attack](){return Attack->CanCancelIntoDodge();},1.0f);
        };
        auto DodgeNow=[this,Body,Attack,Dodge,ASC]()
        {
            const bool bRequested=Body->RequestDodge();
            TestTrue(TEXT("Recovery dodge accepted"),bRequested);
            TestFalse(TEXT("Attack really cancelled"),Attack->IsActive());
            TestFalse(TEXT("No old Attacking tag"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
            TestFalse(TEXT("Old pending input cleared"),Attack->bHasBufferedInput || Attack->bTransitionPending);
            TestEqual(TEXT("Old trace disarmed"),Attack->ActiveTraceSerial,uint32(0));
            TestTrue(TEXT("Dodge is sole montage owner"),ASC->GetAnimatingAbility()==Dodge && Dodge->IsActive());
            TestEqual(TEXT("Exactly one cost"),ASC->GetNumericAttribute(UETStaminaAttributeSet::GetStaminaAttribute()),75.0f);
            TestTrue(TEXT("Own invulnerability begins"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
            return bRequested && Dodge->IsActive();
        };
        if (bEndPlayOnly)
        {
            Reset();if (!EnterRecovery(0) || !DodgeNow() || !Advance(.05f)) { return false; }
            const TSharedPtr<FRootMotionSource> OwnedSource=Body->GetCharacterMovement()->GetRootMotionSource(FName(TEXT("ETFormalDodge")));
            if (!TestTrue(TEXT("Actual handoff source before EndPlay"),OwnedSource.IsValid())) { return false; }
            if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
            TestFalse(TEXT("EndPlay ends handoff dodge"),Dodge->IsActive() || Attack->IsActive());
            TestFalse(TEXT("EndPlay removes own protection"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
            // UE marks the owned source for removal; no movement tick is required after EndPlay.
            TestTrue(TEXT("EndPlay marks the owned source for removal"),OwnedSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval));
            TestTrue(TEXT("EndPlay drops the movement task"),Dodge->MoveTask==nullptr);
            Trace->OnTraceHit.Remove(Watch);Fixture.ForwardErrorMessages(this);
            if (HasAnyErrors()) { return false; }continue;
        }

        Reset();if (!Start() || !Advance(.10f)) { return false; }
        const uint32 EarlySerial=Attack->ActiveTraceSerial;
        TestFalse(TEXT("Windup is not recovery"),Attack->CanCancelIntoDodge());
        TestFalse(TEXT("Early dodge rejected without buffering"),Body->RequestDodge());
        TestTrue(TEXT("Rejected request preserves attack"),Attack->IsActive());
        TestEqual(TEXT("Rejected request preserves serial"),Attack->ActiveTraceSerial,EarlySerial);
        TestEqual(TEXT("Early refusal costs nothing"),ASC->GetNumericAttribute(Stamina),100.0f);
        if (!Idle()) { return false; }CheckDamage(1);
        TestFalse(TEXT("No delayed dodge after early refusal"),Dodge->IsActive());

        for (int32 Stage=0;Stage<3;++Stage)
        {
            Reset();if (!EnterRecovery(Stage)) { return false; }
            TStrongObjectPtr<UAbilityTask_PlayMontageAndWait> RetiredTask(Attack->MontageTask.Get());
            UAnimMontage* RetiredMontage=Attack->ActiveMontage;
            if (!TestNotNull(TEXT("Old task retained"),RetiredTask.Get()) || !DodgeNow()) { return false; }
            RetiredTask->OnMontageBlendingOut(RetiredMontage,true);
            RetiredTask->OnMontageEnded(RetiredMontage,true);
            TestTrue(TEXT("Stale task cannot clear dodge owner"),ASC->GetAnimatingAbility()==Dodge && Dodge->IsActive());
            if (!Settle()) { return false; }CheckDamage(Stage+1);
            TestFalse(TEXT("No protection after normal completion"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
            TestFalse(TEXT("No remaining movement source"),Body->GetCharacterMovement()->HasRootMotionSources());
        }

        Reset();if (!EnterRecovery(0)) { return false; }
        const uint32 PreservedSerial=Attack->ActiveTraceSerial;
        ASC->SetNumericAttributeBase(Stamina,0.0f);
        TestFalse(TEXT("Unaffordable dodge refused"),Body->RequestDodge());
        TestTrue(TEXT("Insufficient stamina preserves attack"),Attack->IsActive());
        TestEqual(TEXT("Insufficient stamina preserves trace identity"),Attack->ActiveTraceSerial,PreservedSerial);
        ASC->SetNumericAttributeBase(Stamina,100.0f);
        UAnimMontage* SavedMontage=Dodge->DodgeMontage;Dodge->DodgeMontage=nullptr;
        TestFalse(TEXT("Missing dodge montage refused before cancellation"),Body->RequestDodge());
        Dodge->DodgeMontage=SavedMontage;
        const float SavedDuration=Dodge->DodgeDuration;Dodge->DodgeDuration=0.0f;
        TestFalse(TEXT("Invalid movement refused before cancellation"),Body->RequestDodge());
        Dodge->DodgeDuration=SavedDuration;
        TestTrue(TEXT("Configuration refusal preserves original attack"),Attack->IsActive());
        TestEqual(TEXT("Configuration refusal costs nothing"),ASC->GetNumericAttribute(Stamina),100.0f);
        if (!Idle()) { return false; }CheckDamage(1);

        Reset();if (!EnterRecovery(0) || !Until([Attack](){return Attack->bComboWindowOpen;},1.0f) ||
            !Body->RequestLightAttack()) { return false; }
        const FTimerHandle Queued=Attack->TransitionTimer;
        TestTrue(TEXT("Follow-up pending before dodge"),Attack->bTransitionPending);
        if (!DodgeNow()) { return false; }
        TestFalse(TEXT("Handoff removes queued transition timer"),World->GetTimerManager().IsTimerActive(Queued));
        if (!Settle()) { return false; }CheckDamage(1);
        Reset();if (!Start() || !Idle()) { return false; }CheckDamage(1);

        Reset();if (!EnterRecovery(0)) { return false; }
        APlayerState* Pauser=World->SpawnActor<APlayerState>();if (!TestNotNull(TEXT("Pauser"),Pauser)) { return false; }
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const float At=Position();const bool bPaused=Advance(.50f);
        TestFalse(TEXT("Paused recovery query refuses"),Attack->CanCancelIntoDodge());
        TestFalse(TEXT("Paused dodge request refused"),Body->RequestDodge());
        TestTrue(TEXT("Paused attack position frozen"),FMath::IsNearlyEqual(At,Position(),.001f));
        TestEqual(TEXT("Paused request costs nothing"),ASC->GetNumericAttribute(Stamina),100.0f);
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bPaused || !DodgeNow() || !Advance(.05f)) { return false; }
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const FVector FrozenLocation = Body->GetActorLocation();
        const double FrozenTime = World->GetTimeSeconds();
        const bool bDodgePaused = Advance(.50f);
        TestTrue(TEXT("Paused handoff displacement frozen"), Body->GetActorLocation().Equals(FrozenLocation, .01f));
        TestEqual(TEXT("Paused handoff clock frozen"), World->GetTimeSeconds(), FrozenTime);
        TestTrue(TEXT("Paused own protection retained"), ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bDodgePaused || !Settle()) { return false; }
        CheckDamage(1);

        Reset();if (!EnterRecovery(0) || !DodgeNow() || !Advance(.05f)) { return false; }
        if (!TestTrue(TEXT("Formal hit react cancels handoff dodge"),Body->RequestHitReact())) { return false; }
        TestFalse(TEXT("Hit ends dodge and own protection"),Dodge->IsActive() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        TestFalse(TEXT("No dodge during hit reaction"),Body->RequestDodge());
        TestEqual(TEXT("Cancellation does not refund"),ASC->GetNumericAttribute(Stamina),75.0f);
        if (!Advance(.05f)) { return false; }
        TestFalse(TEXT("Hit removes handoff root source"),Body->GetCharacterMovement()->HasRootMotionSources());
        if (!Settle()) { return false; }CheckDamage(1);

        Reset();if (!EnterRecovery(0) || !DodgeNow() || !Advance(.05f)) { return false; }
        ASC->SetNumericAttributeBase(Health,0.0f);
        TestTrue(TEXT("Real death sealed"),Body->IsCombatDead());
        TestFalse(TEXT("Death ends both owners"),Dodge->IsActive() || Attack->IsActive());
        TestFalse(TEXT("Death removes own protection"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        if (!Advance(.10f)) { return false; }
        TestFalse(TEXT("Death releases movement source"),Body->GetCharacterMovement()->HasRootMotionSources());
        TestFalse(TEXT("Dead body refuses dodge"),Body->RequestDodge());
        Trace->OnTraceHit.Remove(Watch);
        if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        Fixture.ForwardErrorMessages(this);if (HasAnyErrors()) { return false; }
    }
    AddInfo(TEXT("[M2-03C] Recovery dodge: stages1/2/3, refusal preserves attack, queued follow-up cleared, Pause/Hit/Death/EndPlay"));
    return !HasAnyErrors();
}
#endif
```

## 7. 正常 Build与索引

1. 确认上述一新 / 七旧Source保存，UE关闭。PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

2. Editor Build成功后CLion Reload CMake Project。三个CodeModel预计**37 + 13 + 3 = 53cpp**；只有Core新加一个测试cpp，生成文件不计入手写数。
3. 再执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

4. 两目标成功再开工程。若失败，贴第一条真实C++ / UHT错误与文件行号；exit6不是根因。不删除全工程缓存，不用旧DLL继续验收。

### 7.1 测试时间类型补修（2026-10-16）

第6节完整测试已将`FrozenTime`更正为`double`。UE5.5.4的`UWorld::GetTimeSeconds()`返回double；旧指南用float保存后再传入TestEqual，使两实参变成double / float，触发C2666重载歧义。这是指南的类型错误，不是手动录入问题。

若已录入旧版，只在ETLightComboCancelTests.cpp约第240行，把`const float FrozenTime=World->GetTimeSeconds();`改为`const double FrozenTime=World->GetTimeSeconds();`。第243行TestEqual保持原样，不降低断言或修改玩法。保存、保持UE关闭后继续第7节Editor / 索引 / Game Build，再按第9节运行相关测试；目前没有补修后的Build / Success，不重做未受影响的资产配置。

### 7.2 暂停变量声明遗漏补修（2026-10-16）

若第246行报`bDodgePaused`未声明，说明局部替换时漏掉了`const bool bDodgePaused = Advance(.50f);`。上次两行示例的替换边界不够明确，现给出完整区块：从**紧接闪避推进.05秒之后**的`SetPauserPlayerState(Pauser)`开始，到后面的`CheckDamage(1);`结束，整体对齐为下列内容；不要改前面第一次暂停攻击的bPaused。

```cpp
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const FVector FrozenLocation = Body->GetActorLocation();
        const double FrozenTime = World->GetTimeSeconds();
        const bool bDodgePaused = Advance(.50f);
        TestTrue(TEXT("Paused handoff displacement frozen"), Body->GetActorLocation().Equals(FrozenLocation, .01f));
        TestEqual(TEXT("Paused handoff clock frozen"), World->GetTimeSeconds(), FrozenTime);
        TestTrue(TEXT("Paused own protection retained"), ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bDodgePaused || !Settle()) { return false; }
        CheckDamage(1);
```

这行不只是变量声明：它实际推进暂停世界.50秒并保存结果，后面的if会检查推进是否成功。不能仅改if变量名、设为true或删除检查。当前源码仍由用户手动补回，保存后继续正常Build；没有补修后Build或自动化Success记录。

## 8. 既有资产与输入核对（不新建蓝图验收图）

### 8.1 GA_ET_Dodge标签默认值

1. 打开`/Game/EmberTrial/Abilities/GA_ET_Dodge` → **Class Defaults**。Parent仍ETGA_Dodge，Event Graph空；不要改成新建的GA或替换父类。
2. 清空Details搜索，展开 **Tags**；如果看不到，搜索`Cancel Abilities`（也可搜索`Cancel`），找到 **Cancel Abilities With Tag**。点击容器编辑按钮 / 下拉，勾选**Ability → Attack → Light**，只这一项。不要选State → Attacking。
3. 搜索`Activation Blocked`，找到 **Activation Blocked Tags**：有**State.Dodging / State.HitReact / State.Dead**，没有**State.Attacking**。如果构造默认值已正确继承，不额外改；若仍显示旧Attacking，取消勾选它。只核对这两个容器，不重置整份GA。
4. 对两个容器，若字段旁存在黄色回退箭头且当前与上述不符，可只对该字段Reset to Default；再逐项确认。不要清空整个Tags区域，不改Asset Tags / Activation Owned Tags。
5. Attack / Dodge已有能力标签、Owned Dodging、ServerOnly / InstancedPerActor保留；Dodge Montage仍原闪避AM，Cost仍GE_ET_DodgeCost、150 / .30 / .15保持。
6. Compile → Save。Content Browser右键GA → Asset Actions → Validate Assets，应有本项目`Valid / Issues0`。**ET-DODGE-004**只指这两个标签容器不符合交接契约；按2–4修正。找不到字段时截图完整Class Defaults的Tags与搜索内容，不自行删规则或搭蓝图Cancel节点。

### 8.2 玩家与旧输入

1. BP_ThirdPersonCharacter能力Class仍正式玩家GA / GA_ET_Dodge；单ASC / Trace / Life / Recovery保持，Compile / Save。
2. Event Graph找到既有 **IA_PracticeDodge**（显示名仍沿用M0，不重命名）：**Started**白线经过已验收的暂停 / UI允许门控，最后到 **Request Dodge，Target=Self**；Triggered / Completed不连业务，Return Value留空即可。
3. 若Request前额外串了`Is Formal Action Idle`的Branch，它会吞掉攻击中的2：只把该层前的允许白线直接接Request执行输入，保留更前面的暂停 / UI门控。**E / 移动 / 跳跃自己的Idle检查保留**。不将2连到PracticeAction，不加Cancel Light Attack / Delay / Combo索引变量。
4. 左键 / 数字1仍Started到Request Light Attack(Self)。三份AM与命中 / 连段时间不动，敌人仍原单段10。新版Dodge的精确合法性在原生函数里，不再重复在蓝图拼一套恢复时段。

## 9. 分组检查（10–15分钟）

### 9.1 新交接与相关旧项自动化，约2–4分钟

不Play，Output Log先运行：

```text
Automation RunTests EmberTrial.M2.Combo
```

等 **Flow / RecoveryDodge 两项Success**，再运行：

```text
Automation RunTests EmberTrial.M2.Dodge.Invulnerability
```

等Invulnerability Success。本批新RecoveryDodge覆盖正式位移与一次成本交接；旧Flow确认三段 / 缓冲保持，旧Invulnerability确认伤害保护路径。没有修改位移Task / 成本实现 / Resources / Practice，不重跑它们的全部矩阵，不另加墙坡 / 帧率或UI换Pawn图。

新测试应有`[M2-03C] Recovery dodge: stages1/2/3...`汇总。失败给测试名、第一条Expected与大致时分 / 日志路径；不要删断言、扩大正式恢复范围或把成本改0来凑成功。没有运行就不写Success。

### 9.2 三段恢复取消与预检拒绝，约3–4分钟

- **准备：**正式地图Entry空地Play，生命 / 体力100；可临时控制台`slomo .25`。这是方便看动作，精确窗口已由自动化负责。
- **操作：**先开第一段后马上按2，观察原攻击继续；下一轮等拳头完成命中动作后的收势再按2。再按正常节奏进入第2、3段，各在收势时按2。每轮等体力恢复后再试，不要求手动赶边界。
- **预期：**过早2拒绝且不补执行；恢复2取消当前链、立即原方向闪避、只扣25，旧链不续播，后续左键从1开始。三段都能取消；日志原Attack `Cancelled=1 / AttackingTag=0`后有Dodge Begin / 成本一次、Move与Window清理。
- **拒绝检查：**不足体力 / None Montage / 非法位移保持原攻击由自动化覆盖，不临时改正式GA或搭扣体力函数；若实机错过恢复窗口，观察日志 / 画面，不要求补十轮精确按键。

### 9.3 交战 / 受击 / P，约3–4分钟

- **操作A：**真实目标完成一次三段25或使用一段命中后2取消；让敌人接近挥击，确认普通10与HUD，随后在空地恢复闪避中按3。
- **预期A：**真实命中与敌人单段保持；已命中伤害不撤回，2取消后没有旧Trace额外扣血；3取消闪避，不退款，受击结束恢复输入。
- **操作B：**恢复闪避开始后P暂停约2秒，在菜单按左键 / 2，再以一种支持方式继续。
- **预期B：**位移与保护按世界时间冻结 / 恢复，没有菜单补播、补跳、漂移或卡键；无旧连段重启。暂停期间拒绝2与精确保护清理已由自动化覆盖，不再人工掐.15秒。

### 9.4 世界重建与短基线，约2–3分钟

- **操作：**一次恢复闪避后用现有4完成真实死亡 → Failure Retry；新身体确认双100 / 单HUD，再开始一次连击并Stop，重开。
- **预期：**死亡和EndPlay不遗留旧输入 / RMS / 保护或续播；新左键从1、空闲2、E一次、P一种继续方式与移动转向跳跃正常即可。死亡中P仍由既有Failure控制；不重跑菜单所有None Map / Pawn替换矩阵。
- **恢复：**slomo1，玩家25 / 敌人10 / 双100，Dodge150 / .30 / .15 / Cost25，三段配置 / Notify保持，Debug False，GA标签符合8.1，Source / 资产Save All，回正式地图。当前不Cook / 采Insights；新增包在后续阶段有实际需求时统一。

## 10. 完成反馈与报错材料

```text
M2-03C 完成 / 卡在第__节
一新 / 七旧Source；关闭UE后Editor Build / 索引53cpp / Game Build：
GA_Dodge取消Ability.Attack.Light、Blocked无Attacking / 保留其余三项；Compile / Save / Valid：
既有2的Started到角色Request，无额外Idle拦截；原三段资源保持：
Combo.Flow / Combo.RecoveryDodge / Dodge.Invulnerability：
过早2拒绝不补执行；三段恢复2可取消 / 一次25 / 旧链不续播：
真实命中25、敌人10 / HUD；3取消不退款：
恢复闪避中P冻结恢复 / 菜单无补播不卡键：
死亡Retry新双100 / 动作Stop重开 / E-P与基础输入短基线：
slomo1 / 默认数值 / Debug False、Source资产Save All：
失败或手感问题（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

编译问题文字贴第一条真实错误；标签 / 输入问题截图Class Defaults两个容器或Started到Request的完整局部白线，包含资产名。动画 / 取消问题给输入顺序、当时段落与短视频；日志只给路径 / 大致时间，我自行提取。自动化失败不要求你先判断根因。

## 11. 本批 Commit / Push（固定放在最后）

- **一新Source**：`EmberTrial/Source/EmberTrial/Tests/ETLightComboCancelTests.cpp`。
- **七旧Source**：`EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h`、对应`Private/Abilities/ETGA_LightAttack.cpp`；`Public/Abilities/ETGA_Dodge.h`与对应cpp；`Public/Characters/EmberTrialCharacter.h`、`Public/Combat/ETCombatLifeComponent.h`；`EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。只提交完整保存后的实际修改，Character.cpp / Trace / 位移Task无计划差异。
- **资产**：既有GA_ET_Dodge的标签默认值 / Compile保存真实差异；BP_ThirdPersonCharacter仅有合法输入或编译保存差异时纳入。无新资产 / 输入映射 / Montage；来源清单验收时更新实际哈希，总数仍58，不预填新SHA。
- **文档**：本指南、B报告 / 已验收状态、来源58项与Progress / Backlog / 路线 / Architecture / 验收流程 / Introduction / VersionControl的最新收尾。B主体已在本地HEAD / 缓存origin/main `9a0cf14`，本轮未联网核验；当前C只有指南，完成后再Stage实际实现。

恢复默认值并Save All → CLion刷新Changes → 重新Stage本批完整文件 → 检查Staged Diff → Commit → Push。可用`feat: allow dodge cancel during light attack recovery`；本轮仅文档可用`docs: verify light combo and prepare recovery dodge cancel`。仍由用户操作Git。Raw / 日志 / 生成CMake / IDE缓存 / Binaries / Intermediate / Saved与历史包留本地，不Force Add。
