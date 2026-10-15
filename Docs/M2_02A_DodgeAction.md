# M2-02A：GAS闪避动作与一次体力成本

2026-10-15，固定 **UE 5.5.4**。前置：[M2-01已收口](Reports/M2_01_Verification.md)。**本批已按用户15:45反馈与本地证据验收通过，无待补项。**实际90Source / 46cpp / 来源51项；手动50 / 构建等待5分钟，详见[A验收](Reports/M2_02A_Verification.md)。下方保留原操作代码供复查，无需重复执行；后续按[B方向位移 / 输入指南](M2_02B_DirectionalDodge.md)实施。

原下发预算（非当前待办）：手动 **90–150分钟**：代码55–95、资产15–25、相关检查10–15、保存记录5–10；构建等待另计。可以分两次完成，代码部分完整保存后再正常Build；不要求做满预算。涉及成本与共用动作门控，本批比单纯体力UI范围更大。

## 1. 本批结果与关键顺序

新GAS闪避先通过已有ke路线调用角色RequestDodge，播放现有原地Montage，成功一次扣25体力；动作用State.Dodging门控移动 / 跳跃 / 攻击 / E及旧Practice，恢复组件在正式Dodging期间停表。3受击可取消，真实死亡 / EndPlay沿用共同清理。**本批没有新位移或无敌**；原2仍保持M0原型，只能在与正式GAS动作互斥的空闲状态运行。B接方向位移并迁移2键，C再接短无敌。

成本使用新 **GE_ET_DodgeCost**：Instant、Stamina Add(Base)、固定Scalable Float=-25，本Demo允许单次1–100点。这里调整上一页路线的“复用增量GE”细节：标准GAS CheckCost能够预先计算固定负值；原生SetByCaller增量GE继续专职恢复 / 开发注入，不把需要额外参数的资源增量Spec当默认Cost GE。

调用顺序：`CanActivate / CheckCost → 创建并启动Montage Task → 核对实际播放实例 → CommitAbility → 检查真实扣量 → Begin`。启动前配置 / 资源 / 地面 / 忙碌拒绝不扣；Task没成功启动不提交成本；提交后的受击或死亡取消不退款。仅看到Accepted不等于证明支付，查看Begin的Before / After / Cost。

原生ApplyCost为const，mutable布尔只保存本次提交幂等标记，并在调用父类GE应用前置位，防同步回调再次扣费；不存第二份体力。每次真正Activate重置标记，End不退款。序号保护Task启动 / Commit期间同步取消后的旧收尾，结束时解绑Task回调，再由GAS结束任务 / 移除Owned Tag。

标准成本接口参考[Epic CheckCost](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/Abilities/UGameplayAbility/CheckCost?application_version=5.5) / [ApplyCost](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/Abilities/UGameplayAbility/ApplyCost?application_version=5.5)；实际签名与Montage Task失败 / 清理按本机5.5.4源码核对。默认只支持当前单机、Instant资源规则，不承诺未来Duration Buff或网络预测契约。

## 2. 准备与文件

Stop PIE → Save All → 关闭UE，CLion保存。**三新Source / 十旧Source、两个新GE / GA资产**；不改Build.cs / Target / uproject / UI Source / 输入资产。原计划完成后90Source / 46cpp；15:45验收已达到。

路径根目录 `E:\UE_Project\Demo\EmberTrial\Source`：

| 操作 | 相对路径 |
| --- | --- |
| 新建 | EmberTrial/Public/Abilities/ETGA_Dodge.h |
| 新建 | EmberTrial/Private/Abilities/ETGA_Dodge.cpp |
| 新建 | EmberTrial/Tests/ETDodgeCostTests.cpp |
| 修改两文件 | EmberTrial/Public/Abilities/ETGameplayTags.h 与 Private/Abilities/ETGameplayTags.cpp |
| 修改两文件 | EmberTrial/Public/Characters/EmberTrialCharacter.h 与 Private/Characters/EmberTrialCharacter.cpp |
| 修改 | EmberTrial/Private/Abilities/ETGA_LightAttack.cpp |
| 修改 | EmberTrial/Private/Abilities/ETGA_HitReact.cpp |
| 修改 | EmberTrial/ETPracticeActionComponent.cpp |
| 修改 | EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp |
| 修改 | EmberTrial/Tests/ETStaminaRecoveryTests.cpp |
| 修改 | EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp |

保留IsCombatDead、初始化 / Ready、所有既有方法和默认子对象身份。不要创建第二个ASC / Recovery / Practice；新h的generated.h仍是最后include。

## 3. 三个新文件：各自完整代码


### 3.1 EmberTrial/Public/Abilities/ETGA_Dodge.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Dodge : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Dodge();
    static bool ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost);

protected:
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    friend class FETDodgeCostContractTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Dodge")
    TObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    uint32 ActivationSerial = 0;
    // UE declares ApplyCost const. This flag belongs only to the current attempt.
    mutable bool bCostSubmitted = false;

    UFUNCTION()
    void HandleMontageCompleted();
    UFUNCTION()
    void HandleMontageAborted();
};
```


### 3.2 EmberTrial/Private/Abilities/ETGA_Dodge.cpp

```cpp
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETDodge, Log, All);

UETGA_Dodge::UETGA_Dodge()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Dodge);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
}

bool UETGA_Dodge::ReadCostAmount(const UGameplayEffect* Effect, float Level, float& OutCost)
{
    OutCost = 0.0f;
    if (!IsValid(Effect) || !FMath::IsFinite(Level) || Level < 1.0f ||
        Effect->DurationPolicy != EGameplayEffectDurationType::Instant ||
        Effect->Modifiers.Num() != 1 || Effect->Executions.Num() != 0)
    {
        return false;
    }
    const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
    float Magnitude = 0.0f;
    if (Modifier.Attribute != UETStaminaAttributeSet::GetStaminaAttribute() ||
        Modifier.ModifierOp != EGameplayModOp::AddBase ||
        Modifier.ModifierMagnitude.GetMagnitudeCalculationType() !=
            EGameplayEffectMagnitudeCalculation::ScalableFloat ||
        !Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(Level, Magnitude) ||
        !FMath::IsFinite(Magnitude) || Magnitude > -1.0f || Magnitude < -100.0f)
    {
        return false;
    }
    OutCost = -Magnitude;
    return true;
}

bool UETGA_Dodge::CheckCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    if (!IsValid(ASC) || !IsValid(Body) || Body->IsActorBeingDestroyed() ||
        !Body->HasAuthority() || Body->IsCombatDead() ||
        ActorInfo->OwnerActor.Get() != Body || Body->GetAbilitySystemComponent() != ASC ||
        !Body->GetWorld() || Body->GetWorld()->GetNetMode() != NM_Standalone ||
        UGameplayStatics::IsGamePaused(Body) ||
        !Body->GetGameplayStaminaSnapshot(Stamina, MaxStamina) ||
        !FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina) ||
        MaxStamina < 1.0f || Stamina < 0.0f || Stamina > MaxStamina ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost) ||
        Stamina < Cost)
    {
        return false;
    }
    return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UETGA_Dodge::ApplyCost(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo) const
{
    if (bCostSubmitted || !CheckCost(Handle, ActorInfo, nullptr)) { return; }
    bCostSubmitted = true; // Guard BEFORE the GE can synchronously notify observers.
    Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

bool UETGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !CheckCost(Handle, ActorInfo, OptionalRelevantTags) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    APlayerController* PC = IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UCharacterMovementComponent* Movement = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice = IsValid(Body)
        ? Body->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    return IsValid(Body) && Body->GetCombatTeam() == EETCombatTeam::Player &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Body && !PC->IsMoveInputIgnored() &&
        Body->IsFormalActionIdle() && (!Practice || Practice->IsPracticeActionIdle()) &&
        IsValid(Movement) && Movement->IsMovingOnGround() && !Body->GetVelocity().ContainsNaN() &&
        IsValid(DodgeMontage) && FMath::IsFinite(DodgeMontage->GetPlayLength()) &&
        DodgeMontage->GetPlayLength() > 0.0f && IsValid(Anim) && !Anim->GetCurrentActiveMontage();
}

void UETGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    const uint32 Serial = ++ActivationSerial;
    bCostSubmitted = false;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    if (!IsValid(Body) || !IsValid(ASC) || !IsValid(DodgeMontage) || Body->IsCombatDead() ||
        !Body->GetGameplayStaminaSnapshot(Before, MaxStamina) ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    if (UCharacterMovementComponent* Movement = Body->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
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
    // A real montage instance exists. Commit now; any earlier failure spent nothing.
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
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] Begin Body=%s Serial=%u Before=%.1f After=%.1f Cost=%.1f Dodging=%d"),
        *GetNameSafe(Body), Serial, Before, After, Cost, ASC->GetTagCount(ETGameplayTags::State_Dodging));
}

void UETGA_Dodge::HandleMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_Dodge::HandleMontageAborted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UETGA_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }
    ++ActivationSerial;
    if (IsValid(MontageTask))
    {
        MontageTask->OnCompleted.RemoveAll(this);
        MontageTask->OnInterrupted.RemoveAll(this);
        MontageTask->OnCancelled.RemoveAll(this);
    }
    MontageTask = nullptr;
    const FString BodyName = GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    const TWeakObjectPtr<UAbilitySystemComponent> ASC = ActorInfo
        ? ActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    const bool bSubmitted = bCostSubmitted;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] End Body=%s Cancelled=%d CostSubmitted=%d Dodging=%d"),
        *BodyName, bWasCancelled ? 1 : 0, bSubmitted ? 1 : 0,
        ASC.IsValid() ? ASC->GetTagCount(ETGameplayTags::State_Dodging) : 0);
}
```


### 3.3 EmberTrial/Tests/ETDodgeCostTests.cpp

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_Dodge.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpec.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETDodgeCostContractTest,
    "EmberTrial.M2.Dodge.CostContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETDodgeCostContractTest::RunTest(const FString& Parameters)
{
    UClass* CostClass = LoadClass<UGameplayEffect>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost.GE_ET_DodgeCost_C"));
    if (!TestNotNull(TEXT("Create and save the formal Cost GE before running this test"), CostClass)) { return false; }
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    AEmberTrialCharacter* Body = TestWorld.GetTestWorld()->SpawnActor<AEmberTrialCharacter>();
    if (!TestNotNull(TEXT("Temporary native body"), Body)) { return false; }
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Temporary ASC"), ASC) || !ASC->AbilityActorInfo.IsValid()) { return false; }
    const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(UETGA_Dodge::StaticClass(), 1));
    FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
    UETGA_Dodge* Ability = Spec ? Cast<UETGA_Dodge>(Spec->GetPrimaryInstance()) : nullptr;
    if (!TestNotNull(TEXT("Granted primary instance"), Ability)) { return false; }
    Ability->CostGameplayEffectClass = CostClass; // Transient instance only, never the asset CDO.
    const FGameplayAbilityActorInfo* Info = ASC->AbilityActorInfo.Get();
    const FGameplayAbilityActivationInfo Activation;
    const FGameplayAttribute Stamina = UETStaminaAttributeSet::GetStaminaAttribute();
    auto Value = [&]() { return ASC->GetNumericAttribute(Stamina); };
    float Cost = 0.0f;
    TestTrue(TEXT("Formal fixed cost schema"), UETGA_Dodge::ReadCostAmount(Ability->GetCostGameplayEffect(), 1.0f, Cost));
    TestEqual(TEXT("Formal default cost"), Cost, 25.0f);
    TestTrue(TEXT("100 can pay"), Ability->CheckCost(Handle, Info, nullptr));
    int32 Changes = 0;
    const FDelegateHandle Observer = ASC->GetGameplayAttributeValueChangeDelegate(Stamina).AddLambda(
        [&](const FOnAttributeChangeData& Change)
        {
            ++Changes;
            Ability->ApplyCost(Handle, Info, Activation); // A synchronous recursive attempt must not charge twice.
        });
    Ability->ApplyCost(Handle, Info, Activation);
    Ability->ApplyCost(Handle, Info, Activation);
    ASC->GetGameplayAttributeValueChangeDelegate(Stamina).Remove(Observer);
    TestEqual(TEXT("One real payment"), Value(), 75.0f);
    TestEqual(TEXT("Duplicate and recursive payment produce one change"), Changes, 1);
    TestEqual(TEXT("Base also paid once"), ASC->GetNumericAttributeBase(Stamina), 75.0f);

    ASC->SetNumericAttributeBase(Stamina, 24.0f);
    Ability->bCostSubmitted = false; // Simulate the next attempt, without starting any montage.
    TestFalse(TEXT("24 cannot pay 25"), Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Rejected payment preserves 24"), Value(), 24.0f);
    TestFalse(TEXT("Rejected payment is not marked submitted"), Ability->bCostSubmitted);
    ASC->SetNumericAttributeBase(Stamina, 25.0f);
    TestTrue(TEXT("Exactly 25 can pay"), Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Exact payment leaves zero"), Value(), 0.0f);
    TestEqual(TEXT("Exact payment Base zero"), ASC->GetNumericAttributeBase(Stamina), 0.0f);
    TestTrue(TEXT("Zero stamina remains Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Zero stamina cannot pay again"), Ability->CheckCost(Handle, Info, nullptr));
    TestFalse(TEXT("Zero stamina is not death"), Body->IsCombatDead());

    ASC->SetNumericAttributeBase(Stamina, 100.0f);
    Ability->bCostSubmitted = false;
    Ability->CostGameplayEffectClass = nullptr;
    TestFalse(TEXT("Missing cost cannot become free dodge"), Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Missing cost writes nothing"), Value(), 100.0f);
    Ability->CostGameplayEffectClass = CostClass;
    TestEqual(TEXT("Cost never changes Health"), ASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);
    ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestFalse(TEXT("Dead body cannot pay"), Ability->CheckCost(Handle, Info, nullptr));
    Ability->ApplyCost(Handle, Info, Activation);
    TestEqual(TEXT("Dead body resource unchanged"), Value(), 100.0f);
    ASC->ClearAbility(Handle);
    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```


CostContract使用自己的原生身体 / 真实ASC、真实正式Cost GE和已授予的主Ability实例，只在临时实例上设置Class / 标记；不改资产CDO、不启动Montage。它证明成本检查、真实Base / Current扣除、重复 / 同步重入保护、24拒绝 / 25可付、None成本 / 死亡拒绝及生命隔离，**不能当作整次Ability激活 / Montage / 取消的端到端测试**。后者由第8节短PIE与后续位移集成覆盖。

## 4. 十个旧文件：逐处精确修改

每小节都标明实际文件；只将“旧片段”替换成“新片段”，其余内容保留。多次出现的概念有不同位置，不按搜索结果随便替换全部。若你的缩进不同，可按相同函数与语句定位，括号 / 分号以新片段为准。


### 4.1 原Tag头文件：namespace内部加两个声明

文件：`EmberTrial/Public/Abilities/ETGameplayTags.h`。

旧片段：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
```

新片段：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dodging);
```


### 4.2 原Tag实现：同一namespace内部加两个定义

文件：`EmberTrial/Private/Abilities/ETGameplayTags.cpp`。不在ini重复登记，不添加尚未使用的Invulnerable Tag。

旧片段：

```cpp
    UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
```

新片段：

```cpp
    UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Dodge, "Ability.Dodge");
    UE_DEFINE_GAMEPLAY_TAG(State_Dodging, "State.Dodging");
```


### 4.3 Character.h：前置声明

文件：`EmberTrial/Public/Characters/EmberTrialCharacter.h`。

旧片段：

```cpp
class UETGA_LightAttack;
```

新片段：

```cpp
class UETGA_LightAttack;
class UETGA_Dodge;
```


### 4.4 Character.h：private区增加Class、Handle与授予方法

文件：`EmberTrial/Public/Characters/EmberTrialCharacter.h`。

旧片段：

```cpp
	void GrantLightAttackAbility();
```

新片段：

```cpp
	void GrantLightAttackAbility();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Abilities")
    TSubclassOf<UETGA_Dodge> DodgeAbilityClass;

    FGameplayAbilitySpecHandle DodgeAbilityHandle;
    void GrantDodgeAbility();
```


### 4.5 Character.h：public区增加长期角色请求入口

文件：`EmberTrial/Public/Characters/EmberTrialCharacter.h`。不添加临时Controller函数或新的键位。

旧片段：

```cpp
	bool RequestLightAttack();
```

新片段：

```cpp
	bool RequestLightAttack();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestDodge();
```


### 4.6 Character.cpp：新增include

文件：`EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

旧片段：

```cpp
#include "Abilities/ETGA_LightAttack.h"
```

新片段：

```cpp
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Dodge.h"
```


### 4.7 Character.cpp：BeginPlay只追加一行

文件：`EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

旧片段：

```cpp
	GrantLightAttackAbility();
```

新片段：

```cpp
	GrantLightAttackAbility();
    GrantDodgeAbility();
```


### 4.8 Character.cpp：在GrantLightAttackAbility函数前增加两个完整函数

文件：`EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。在旧函数外添加，所有旧函数保留；敌人 / 训练目标不授予正式Dodge。

旧片段：

```cpp
void AEmberTrialCharacter::GrantLightAttackAbility()
```

新片段：

```cpp
void AEmberTrialCharacter::GrantDodgeAbility()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !HasAuthority() ||
        CombatTeam != EETCombatTeam::Player || !DodgeAbilityClass || DodgeAbilityHandle.IsValid())
    {
        return;
    }
    if (DodgeAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETAbilityFoundation, Warning, TEXT("[M2-02A] Abstract Dodge class Body=%s"), *GetNameSafe(this));
        return;
    }
    DodgeAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(DodgeAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-02A] Dodge granted Body=%s Valid=%d"),
        *GetNameSafe(this), DodgeAbilityHandle.IsValid() ? 1 : 0);
}

bool AEmberTrialCharacter::RequestDodge()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !DodgeAbilityHandle.IsValid())
    {
        UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-02A] Dodge request Body=%s Accepted=0 Reason=NotReadyOrNotGranted"), *GetNameSafe(this));
        return false;
    }
    const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(DodgeAbilityHandle);
    if (!Spec || Spec->IsActive())
    {
        UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-02A] Dodge request Body=%s Accepted=0 Reason=MissingOrBusy"), *GetNameSafe(this));
        return false;
    }
    const bool bAccepted = AbilitySystemComponent->TryActivateAbility(DodgeAbilityHandle, false);
    UE_LOG(LogETAbilityFoundation, Display, TEXT("[M2-02A] Dodge request Body=%s Accepted=%d"), *GetNameSafe(this), bAccepted ? 1 : 0);
    return bAccepted;
}

void AEmberTrialCharacter::GrantLightAttackAbility()
```


### 4.9 Character.cpp：IsFormalActionIdle增加Dodging

文件：`EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

旧片段：

```cpp
!AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_HitReact);
```

新片段：

```cpp
!AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_HitReact) &&
        !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Dodging);
```


### 4.10 Character.cpp：EndPlay清理授予Handle

文件：`EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。位置在CancelAllAbilities / 旧LightAttack Clear之后、RemoveAllGameplayCues之前。

旧片段：

```cpp
        AbilitySystemComponent->RemoveAllGameplayCues();
```

新片段：

```cpp
        if (DodgeAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(DodgeAbilityHandle);
        }
        AbilitySystemComponent->RemoveAllGameplayCues();
```


### 4.11 Character.cpp：EndPlay重置新Handle

文件：`EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

旧片段：

```cpp
    LightAttackAbilityHandle = FGameplayAbilitySpecHandle();
```

新片段：

```cpp
    LightAttackAbilityHandle = FGameplayAbilitySpecHandle();
    DodgeAbilityHandle = FGameplayAbilitySpecHandle();
```


### 4.12 LightAttack.cpp：构造函数阻断正式Dodging

文件：`EmberTrial/Private/Abilities/ETGA_LightAttack.cpp`。玩家和敌人共用该原生类，两个资产后续都Compile / Save。

旧片段：

```cpp
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
```

新片段：

```cpp
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dodging);
```


### 4.13 HitReact.cpp：受击能取消Dodge

文件：`EmberTrial/Private/Abilities/ETGA_HitReact.cpp`。

旧片段：

```cpp
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Attack_Light);
```

新片段：

```cpp
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Attack_Light);
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Dodge);
```


### 4.14 HitReact.cpp：ActivateAbility兼容取消尚未完成

文件：`EmberTrial/Private/Abilities/ETGA_HitReact.cpp`。仍不能与未完成取消的正式动作重叠，原CommitAbility与其他条件不删。

旧片段：

```cpp
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking))
```

新片段：

```cpp
        (ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
         ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging)))
```


### 4.15 Practice.cpp：加Character include

文件：`EmberTrial/ETPracticeActionComponent.cpp`。

旧片段：

```cpp
#include "ETPracticeActionComponent.h"
```

新片段：

```cpp
#include "ETPracticeActionComponent.h"
#include "Characters/EmberTrialCharacter.h"
```


### 4.16 Practice.cpp：在RequestPracticeAction的Paused判断之后增加互斥

文件：`EmberTrial/ETPracticeActionComponent.cpp`。原生测试ACharacter不具本项目GAS身体类型，仍走原行为；正式身体才查正式忙碌。旧2继续保留，但不能叠加在新Dodge上。

旧片段：

```cpp
    if (RequestedState != EETPracticeActionState::Attacking
```

新片段：

```cpp
    if (const AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CharacterOwner.Get()))
    {
        if (!Body->IsFormalActionIdle()) { return RejectRequest(RequestedState, TEXT("FormalBusy")); }
    }

    if (RequestedState != EETPracticeActionState::Attacking
```


### 4.17 Recovery.cpp：StopObserving中的Tag数组增加第四项

文件：`EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp`。

旧片段：

```cpp
const FGameplayTag Tags[] = {ETGameplayTags::State_Attacking,
            ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
```

新片段：

```cpp
const FGameplayTag Tags[] = {ETGameplayTags::State_Attacking,
            ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead, ETGameplayTags::State_Dodging};
```


### 4.18 Recovery.cpp：RefreshObservation中的Tag数组同样增加第四项

文件：`EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp`。顺序必须一致，其余观察序号 / Timer / 积分全部保留。不能只改其中一处。

旧片段：

```cpp
const FGameplayTag Tags[] = {ETGameplayTags::State_Attacking,
        ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
```

新片段：

```cpp
const FGameplayTag Tags[] = {ETGameplayTags::State_Attacking,
        ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead, ETGameplayTags::State_Dodging};
```


### 4.19 既有Recovery测试：把单个Attacking段替换为两个Tag的同一组检查

文件：`EmberTrial/Tests/ETStaminaRecoveryTests.cpp`。替换范围到下方ApplyDelta(1000.0f)之前，不包含该行；原测试全部其他部分保留。

旧片段：

```cpp
    ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestFalse(TEXT("Busy tag stops timer immediately"), Recovery->IsRecoveryScheduled());
    const float BusyValue = Value();
    if (!Advance(20)) { return false; }
    Same(TEXT("Busy freezes recovery"), BusyValue);
    ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("Busy end does not pay busy duration"), Value() > BusyValue && Value() <= BusyValue + 6.2f);

```

新片段：

```cpp
    for (const FGameplayTag Tag : {FGameplayTag(ETGameplayTags::State_Attacking),
                                  FGameplayTag(ETGameplayTags::State_Dodging)})
    {
        ASC->AddLooseGameplayTag(Tag);
        TestFalse(TEXT("Busy tag stops timer immediately"), Recovery->IsRecoveryScheduled());
        const float BusyValue = Value();
        if (!Advance(20)) { return false; }
        Same(TEXT("Busy freezes recovery"), BusyValue);
        ASC->RemoveLooseGameplayTag(Tag);
        if (!Advance(4)) { return false; }
        TestTrue(TEXT("Busy end does not pay busy duration"), Value() > BusyValue && Value() <= BusyValue + 6.2f);
    }

```


### 4.20 Editor Validator：新增Dodge include

文件：`EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。

旧片段：

```cpp
#include "Abilities/ETGA_HitReact.h"
```

新片段：

```cpp
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_Dodge.h"
```


### 4.21 Editor Validator：Profiles增加两个精确资产路径

文件：`EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。

旧片段：

```cpp
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact"), UETGA_HitReact::StaticClass()},
```

新片段：

```cpp
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact"), UETGA_HitReact::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge"), UETGA_Dodge::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost"), UGameplayEffect::StaticClass()},
```


### 4.22 Editor Validator：ValidateBlueprint能力分支纳入Dodge

文件：`EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。

旧片段：

```cpp
else if (Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>())
```

新片段：

```cpp
else if (Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>() || Defaults->IsA<UETGA_Dodge>())
```


### 4.23 Editor Validator：ValidateBlueprint中GE分支区分Cost与Damage

文件：`EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。成本GE不能误走Health伤害规则；原伤害GE继续走原规则。

旧片段：

```cpp
        ValidateDamageEffect(Blueprint, Effect, Issues);
```

新片段：

```cpp
        if (Blueprint->GetOutermost()->GetName() == TEXT("/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost"))
        {
            float Cost = 0.0f;
            if (!UETGA_Dodge::ReadCostAmount(Effect, 1.0f, Cost))
            {
                AddIssue(Blueprint, TEXT("ET-DODGE-001"),
                    TEXT("Cost GE requires Instant, one Stamina AddBase ScalableFloat in [-100,-1], no executions."), Issues);
            }
        }
        else
        {
            ValidateDamageEffect(Blueprint, Effect, Issues);
        }
```


### 4.24 Editor Validator：ValidateBody仅玩家要求DodgeAbilityClass

文件：`EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。紧接if(bPlayer)的大括号内第一行加；敌人和训练目标保持None，不强行授予。

旧片段：

```cpp
    if (bPlayer)
    {
```

新片段：

```cpp
    if (bPlayer)
    {
        RequireObject(Blueprint, Defaults, TEXT("DodgeAbilityClass"), TEXT("ET-DODGE-001"), Issues);
```


### 4.25 Editor Validator：ValidateAbility在bAttack判断前加Dodge专用分支

文件：`EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。通用InstancedPerActor / ServerOnly检查仍在该分支之前；return只结束Dodge校验，旧Attack / HitReact规则保留。

旧片段：

```cpp
    const bool bAttack = Defaults->IsA<UETGA_LightAttack>();
```

新片段：

```cpp
    if (Defaults->IsA<UETGA_Dodge>())
    {
        ValidateMontage(Blueprint, Cast<UAnimMontage>(RequireObject(Blueprint, Defaults,
            TEXT("DodgeMontage"), TEXT("ET-GA-001"), Issues)), Issues);
        UClass* CostClass = Cast<UClass>(RequireObject(Blueprint, Defaults,
            TEXT("CostGameplayEffectClass"), TEXT("ET-DODGE-001"), Issues));
        float Cost = 0.0f;
        if (CostClass && !UETGA_Dodge::ReadCostAmount(CostClass->GetDefaultObject<UGameplayEffect>(), 1.0f, Cost))
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-001"), TEXT("Invalid Dodge Cost GE; use one negative fixed Stamina modifier."), Issues);
        }
        return;
    }
    const bool bAttack = Defaults->IsA<UETGA_LightAttack>();
```


## 5. 正常Build与CLion索引

保存所有Source，保持UE关闭，PowerShell先运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor与索引成功后，再执行Game：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

前一项失败先处理，不继续误记成功。CLion Reload CMake Project：新Dodge.cpp / CostTests.cpp在EmberTrialCodeModel，Validator仍在Editor CodeModel，全项目预计46cpp。不通过CMake CodeModel编译UE。不要删除缓存，新增反射函数 / 属性不以Live Coding代替正常Build。

此时先不Play，下面的两个新资产与玩家Class尚未配置。Native Cost测试也必须等GE资产保存后再跑。

## 6. 两个新资产：按顺序创建

### 6.1 GE_ET_DodgeCost：一次真实成本

1. Content Browser进入 `/Game/EmberTrial/Abilities/Effects`。
2. 右键 → Blueprint Class → All Classes，搜索 **GameplayEffect**，创建名 **GE_ET_DodgeCost**。不是Data Asset，也不是StaminaDeltaEffect的子类。
3. 打开新GE → Class Defaults；Duration Policy设 **Instant**。
4. Modifiers数组仅保留一个元素；展开Index0：Attribute选 **ETStaminaAttributeSet.Stamina**；Modifier Op选 **Add (Base)**。
5. Modifier Magnitude类型选 **Scalable Float**，数值填 **-25**；不设Data Table / Curve，不选Set By Caller。注意是负25，正25会治疗体力。
6. Executions保持空，不添加Period、Duration、Tag要求、免疫 / 自定义GE组件、Cue或其他Modifier。本批只用固定单次成本。
7. Compile → Save。缩略图路径应 `/Game/EmberTrial/Abilities/Effects/GE_ET_DodgeCost`。

### 6.2 GA_ET_Dodge：配置动作与继承成本字段

1. Content Browser进入 `/Game/EmberTrial/Abilities`，右键Blueprint Class → All Classes搜索 **ETGA_Dodge**，创建 **GA_ET_Dodge**。
2. 打开 → Class Defaults，搜索 `Dodge Montage`，选择已有 **AM_M0_Dodge_01_Montage**，路径 `/Game/EmberTrial/Animations/Practice/Montages/AM_M0_Dodge_01_Montage`。不重做动画 / Notify，不新增位移轨道。
3. 清空搜索，搜索 `Cost Gameplay Effect Class`（或搜索`Cost`、展开继承的Costs分类），选 **GE_ET_DodgeCost**。这是UGameplayAbility已有字段，不另外造一个Dodge Cost浮点字段，也不在蓝图Event Graph手动扣25。
4. Cooldown Gameplay Effect Class保持None；不添加Ability Triggers。原生Instancing Policy为InstancedPerActor、Net Execution Policy为ServerOnly，保留默认；Tags中Owned含State.Dodging，Blocked含Dodging / Attacking / HitReact / Dead。
5. Event Graph保持空，不添加ActivateAbility / CommitAbility / ApplyGameplayEffect / Montage业务。Compile → Save。

## 7. 角色装配与静态检查

1. 打开原 **BP_ThirdPersonCharacter**，Class Defaults搜索 `Dodge Ability Class`，设为 **GA_ET_Dodge**。不是把GA挂成组件；仍只有原ASC / Trace / Life / Recovery / Practice。
2. 原LightAttack Class、100/100生命 / 体力、输入四资源与Recovery20 / 0.75 / 0.10保持。Compile → Save。
3. GA_ET_LightAttack、GA_ET_EnemyLightAttack、GA_ET_HitReact各打开Compile → Save，确认继承原生Tag：两攻击Blocked含State.Dodging，HitReact Cancel含Ability.Dodge及原Ability.Attack.Light。不改25 / 10伤害、原Montage / GE。
4. 两个新GE / GA、原玩家BP、上述三个GA共六份资产，Content Browser多选 → Asset Actions → Validate Assets。每份应有项目 `[M1-07C2] Valid / Issues0`。新成本规则ET-DODGE-001正常通过；本批不额外做Editor负例Probe，不能将运行None测试当成Editor负例实测。
5. 敌人 / 训练目标的Dodge Ability Class保持None；不修改原2事件 / IMC，旧Practice动画与150cm / 0.30配置保持。原1 / 3 / 4、E / P业务继续保留。

## 8. 相关检查（10–15分钟，不新建临时蓝图）

### 8.1 四项自动测试（约2–3分钟）

不Play，在Output Log命令框逐条执行，前一组完成再下一组：

```text
Automation RunTests EmberTrial.M2.Resources
```

等待Foundation / Recovery两项Success；Recovery现在同组检查Attacking / Dodging两种忙碌。

```text
Automation RunTests EmberTrial.M2.Dodge.CostContract
```

等待CostContract一项Success。

```text
Automation RunTests EmberTrial.M0.PracticeAction.Lifecycle
```

等待Lifecycle一项Success。因为本批修改了Practice公共入口，相关旧Lifecycle需运行一次；位移实现未改，不重做旧30 / 60 / 120 FPS墙坡矩阵。若Failure，发Test Started至Completed或日志路径，不删除断言 / 吞Warning。

### 8.2 正常启动 / 互斥 / 受击取消（约3–5分钟）

1. 正式地图Play，在Entry附近先确认两面板100。出生新增一次 `[M2-02A] Dodge granted ... Valid=1`，敌人 / 训练目标不授予。
2. Output Log输入以下长期角色请求：

   ```text
   ke BP_ThirdPersonCharacter_C RequestDodge
   ```

   预期播放原地闪避、Begin Before100 / After75 / Cost25 / Dodging1，结束End Cancelled0 / Dodging0，之后恢复满体力。**本批原地是预期，原2仍有M0位移且不消费体力。**不用赶截图75，数值以Begin日志为准。
3. 为稳定观察取消，可先控制台输入 `slomo 0.1`，再请求一次新Dodge。动作期间重复请求应拒绝、不再Begin或扣第二次；切回视口尝试左键 / 旧2 / 跳跃 / E，应不叠加动作 / 位移或执行交互，镜头仍可转。3触发正式受击，Dodge End Cancelled1 / Dodging0，受击后恢复；成本已提交不退款。
4. 恢复 `slomo 1`。暂停用既有P，菜单期间RequestDodge应拒绝、不扣费 / 不补播。只测一种继续方式，恢复后可再次请求，不重演全部焦点矩阵。

不要在两次正常请求之间硬记“第二次仍应100”：资源可能尚未回满。一次默认消耗量始终25，按Begin真实Before / After核对。slomo仅用于放宽观察窗口，不要求你掌握精确帧。

### 8.3 死亡 / Stop清理与配置拒绝（约4–6分钟）

1. 再用 `slomo 0.1` 请求Dodge，切回视口用原4键真实致死。应取消当前Dodge，End清Tag、无结束后旧Begin，Failure只一页；Retry新身体两资源100、重新授予一次。恢复 `slomo 1`。
2. 新一次Dodge中Stop（可仍先用slomo0.1放慢），随后再Play新100 / 新授予；结束无旧Tag / Task继续回调。原体力HUD创建 / 清理沿用现有Bootstrap，不重做十次重建。
3. Stop，临时将GA_ET_Dodge的Dodge Montage设None，Compile / Save；Play请求应Rejected / 无Begin，开发查询 `ke BP_ETPlayerController_C ET_Stamina 0` 为100 / Timer0。Stop立即恢复原Montage，Compile / Save → 项目Valid。**这个负例验证启动前配置拒绝；不冒充实际Montage_Play引擎失败路径已经动态测过。**成本None已由自动测试覆盖，不再手工破坏正式Cost GE。
4. 最后正常Play，左键真实25 / 原2 / E / P一种继续与移动转向跳跃短基线；不靠近敌人时两资源最终100，4仍Damage25。恢复slomo1、Debug False，Source / 资产Save All。

本批无新Cook / Insights；现有包仍M1。真正Montage启动失败、同步结束再激活等极端路径做代码审查，不写成全部动态覆盖；位移与无敌验收留B / C。

## 9. 最简反馈

```text
M2-02A 完成 / 卡在第__节第__步
三新 / 十旧Source；关闭UE后Editor Build / 索引46cpp / Game Build：
Cost GE Instant / Stamina Add(Base) -25；GA父类 / Montage / Cost Class / 空Graph：
玩家Dodge Class；旧2未迁移；六资产Compile / Save / 本项目Valid：
Resources两项 / CostContract / Lifecycle各一次Success：
新请求原地播放 / 一次扣25 / 正常结束 / Dodging阻断恢复：
忙碌重复拒绝 / 攻击-旧2-E-跳跃互斥 / 3取消不退款：
Pause拒绝无补播 / 死亡Retry / 动作中Stop与新100：
None Montage拒绝未扣；原Montage恢复 / Valid：
slomo1 / 原配置恢复 / Save All / 最后短基线：
失败或特别说明（无则填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

Build发第一条真实UHT / C++ / linker错误；资产字段问题发对应Class Defaults截图 / 路径；测试只给结果和日志位置，我读取本机详细时序。无需手工摘几十条日志。

## 10. 本批 Commit / Push（固定放在最后）

- 三新Source：Dodge.h / .cpp与ETDodgeCostTests.cpp；十旧Source按第2节，包含已有Recovery测试与Editor Validator，不能遗漏恢复两处Tag数组或旧Practice互斥。
- 新GA_ET_Dodge / GE_ET_DodgeCost、玩家Dodge Class与既有三GA实际.uasset差异；原Montage复用，不为本批重新烘焙。来源清单在实物验收后更新，不提前声称51项。
- 本指南、闪避路线 / 当前状态与后续实际验收报告；本地8b725e5已包含资源实现，不重复提交无差异文件。Build.cs / Target / uproject / UI / IMC本批没有计划修改。
- 临时None Montage必须恢复原资源，slomo1；Raw / 日志 / Binaries / Intermediate / Saved、生成CMake / IDE状态与旧包留本地，不Force Add。

CLion刷新Changes → 重新Stage最新完整实现 / 合法配置 / 文档 → 查看Staged Diff → Commit → Push。通过后可用 `feat: add GAS dodge action and checked stamina cost`；仅下发方案用 `docs: prepare GAS dodge action`。用户操作Git；B方向位移与C无敌仍未完成，不把本批写成正式闪避整章交付。
