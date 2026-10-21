# M2-05B1：正式破势状态与行动门控

2026-10-21，固定UE5.5.4。**[M2-05A已验收](Reports/M2_05A_Verification.md)，本章为待手动实施指南，尚未Build或实机验证。**原B拆为B1状态 / 门控与B2归零联动 / 恢复 / 表现，控制单批复杂度；完整范围见[破势路线](M2_05_BreakPulseRoadmap.md)。

## 1. 本批交付、暂留边界与用时

新增正式`GA_ET_Stagger`，通过`State.Stunned`表示默认1.2游戏秒的破势状态。接受后取消当前攻击 / 闪避 / 治疗 / 普通受击，阻止其重新激活，清理原Practice动作、移动 / 跳跃输入和活动AI移动；结束后解除自己拥有的状态与阻断关系。死亡优先取消它，EndPlay撤销自己的Spec和计时器，Pause冻结世界时间。

本批复用`Character.RequestStagger`生产反应入口在PIE控制台检查，不新增按键。**尚不自动观察Poise0、不扣 / 回填Poise、不播放新破势动画，也没有F范围技能或冷却。**强制请求时Poise仍100/100是预期；B2再接真实耗尽 / 恢复与占位动作。当前画面是旧动作被取消、身体暂时停住，镜头和P仍可用，不能将它称为最终破势表现。

GAS负责统一动作取消与阻断：新GA的`Cancel Abilities With Tag`与`Block Abilities With Tag`各包含四种既有Ability标签。这样不需要对四份旧GA再保存同一组Stunned阻断配置；Character公共Idle门控负责移动 / 跳跃 / E / AI，攻击入口额外拒绝连段缓冲，普通受击入口与体力恢复同样检查Stunned。

| 条件 | 本批行为 |
| --- | --- |
| 普通攻击 / 闪避 / 治疗 / 受击正在运行 | Stagger接管并取消它们；不返还已消费体力，治疗生效前取消不扣次数 |
| 已Stunned再次请求 | 拒绝，不重启或延长 |
| Stunned期间受真实生命伤害 | Health仍可下降，普通受击不覆盖破势；生命0走死亡最高优先级 |
| Stunned期间输入移动 / 跳跃 / E / 正式能力 | 拒绝，不排队；镜头与P允许 |
| Pause | 游戏时间与活动Timer冻结；恢复后继续剩余时间 |
| 自然结束 / 取消 | 只撤销本GA的Timer、Tag计数与Block关系；不恢复死亡封锁的MovementMode |
| 同身体失去 / 恢复控制 | 反应归身体，控制变化不重新创建Spec或重置时长 |
| EndPlay / 组件Deactivate | 先撤销句柄归属，再取消 / 移除自己的Spec，禁止旧回调结束新激活 |

预计手动约100–160分钟，含代码 / 理解65–110、四资产配置20–35、检查8–12及记录；构建等待另记。当前123 Source / 65cpp、Core45 / UI17 / Editor3、来源64；完成后预计128 Source / 68cpp、Core48 / UI17 / Editor3、新GA加入来源后65项。估计不计实际。

## 2. 准备与精确文件清单

Stop PIE → Save All → 关闭UE，CLion保存。按3–5节创建五文件，再按6节修改七个既有文件；不用Live Coding。新GA / 组件属于`EmberTrial`，只有校验器属于`EmberTrialEditor`；不改UI、Build.cs、Target或uproject。

| 类型 | 相对于仓库根目录的路径 |
| --- | --- |
| 新 | `EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Stagger.h` |
| 新 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Stagger.cpp` |
| 新 | `EmberTrial/Source/EmberTrial/Public/Combat/ETCombatPoiseComponent.h` |
| 新 | `EmberTrial/Source/EmberTrial/Private/Combat/ETCombatPoiseComponent.cpp` |
| 新 | `EmberTrial/Source/EmberTrial/Tests/ETStaggerAbilityTests.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_HitReact.cpp` |
| 旧 | `EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp` |
| 旧 | `EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp` |

## 3. 新GA：状态、世界时间与精确清理

GA拥有DurationTimer与激活代次；只在活动期间存在一个世界Timer。结束先使代次失效、清自己的Timer，再交GAS撤销自己的Tag / Block关系，避免旧回调结束后来的一轮。这里不新增通用计时框架，也不手动把共享Tag计数清零。

### 3.1 ETGA_Stagger.h：完整内容

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "ETGA_Stagger.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UWorld;

// 本批仅拥有破势状态 / 时长；Poise结算与动作表现后续接入。
UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Stagger : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_Stagger();
    bool HasValidConfiguration() const;

protected:
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo* ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                            const FGameplayAbilityActorInfo* ActorInfo,
                            const FGameplayAbilityActivationInfo ActivationInfo,
                            bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    friend class FETStaggerAbilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stagger",
              meta = (ClampMin = "0.05", ClampMax = "10.0"))
    float StaggerDuration = 1.2f;

    FTimerHandle DurationTimer;
    TWeakObjectPtr<UWorld> TimerWorld;
    uint64 ActivationSerial = 0;

    bool ReadContext(const FGameplayAbilityActorInfo* ActorInfo, AEmberTrialCharacter*& Body,
                     UAbilitySystemComponent*& ASC) const;

    void HandleDurationFinished(uint64 Serial);
};
```

### 3.2 ETGA_Stagger.cpp：完整内容

```cpp
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGameplayTags.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStagger, Log, All);

// ---- GAS统一拥有Tag、取消与激活阻断；不手工复制一份Stunned布尔值 ----
UETGA_Stagger::UETGA_Stagger()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Stagger);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Stunned);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Stunned);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);

    const FGameplayTag Actions[] = {ETGameplayTags::Ability_Attack_Light,
                                    ETGameplayTags::Ability_Dodge, ETGameplayTags::Ability_Heal,
                                    ETGameplayTags::Ability_HitReact};
    for (const FGameplayTag& Tag : Actions)
    {
        CancelAbilitiesWithTag.AddTag(Tag);
        BlockAbilitiesWithTag.AddTag(Tag);
    }
}

bool UETGA_Stagger::HasValidConfiguration() const
{
    if (!FMath::IsFinite(StaggerDuration) || StaggerDuration < .05f || StaggerDuration > 10.0f ||
        InstancingPolicy != EGameplayAbilityInstancingPolicy::InstancedPerActor ||
        NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::ServerOnly ||
        ReplicationPolicy != EGameplayAbilityReplicationPolicy::ReplicateNo ||
        bRetriggerInstancedAbility || GetCostGameplayEffect() || GetCooldownGameplayEffect() ||
        GetAssetTags().Num() != 1 || !GetAssetTags().HasTagExact(ETGameplayTags::Ability_Stagger) ||
        ActivationOwnedTags.Num() != 1 ||
        !ActivationOwnedTags.HasTagExact(ETGameplayTags::State_Stunned) ||
        ActivationBlockedTags.Num() != 2 ||
        !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Stunned) ||
        !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dead) ||
        CancelAbilitiesWithTag.Num() != 4 || BlockAbilitiesWithTag.Num() != 4)
    {
        return false;
    }
    const FGameplayTag Actions[] = {ETGameplayTags::Ability_Attack_Light,
                                    ETGameplayTags::Ability_Dodge, ETGameplayTags::Ability_Heal,
                                    ETGameplayTags::Ability_HitReact};
    for (const FGameplayTag& Tag : Actions)
    {
        if (!CancelAbilitiesWithTag.HasTagExact(Tag) || !BlockAbilitiesWithTag.HasTagExact(Tag))
        {
            return false;
        }
    }
    return true;
}

bool UETGA_Stagger::ReadContext(const FGameplayAbilityActorInfo* ActorInfo,
                                AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC) const
{
    Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    return IsValid(Body) && Body->HasAuthority() && !Body->IsCombatDead() && IsValid(ASC) &&
           ActorInfo->OwnerActor.Get() == Body && Body->GetAbilitySystemComponent() == ASC &&
           !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead) &&
           Body->GetGameplayHealthSnapshot(Health, MaxHealth) && FMath::IsFinite(Health) &&
           FMath::IsFinite(MaxHealth) && MaxHealth >= 1.0f && Health > 0.0f && Health <= MaxHealth;
}

bool UETGA_Stagger::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayTagContainer* SourceTags,
                                       const FGameplayTagContainer* TargetTags,
                                       FGameplayTagContainer* OptionalRelevantTags) const
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    return HasValidConfiguration() && ReadContext(ActorInfo, Body, ASC) &&
           !UGameplayStatics::IsGamePaused(Body) &&
           Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags,
                                     OptionalRelevantTags);
}

// ---- 接管旧动作后只设一次世界时间Timer；不禁用移动模式，不接管P / 镜头 ----
void UETGA_Stagger::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadContext(ActorInfo, Body, ASC) || !HasValidConfiguration() ||
        !CommitAbility(Handle, ActorInfo, ActivationInfo) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact))
    {
        // ScopeLock仍延期旧动作取消时拒绝叠加，归零事件批次再统一延后派发。
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    if (UETPracticeActionComponent* Practice =
            Body->FindComponentByClass<UETPracticeActionComponent>())
    {
        Practice->CancelPracticeAction();
    }
    if (!IsActive())
    {
        return;
    }
    if (!ReadContext(ActorInfo, Body, ASC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    if (AController* Controller = Body->GetController())
    {
        Controller->StopMovement();
    }
    if (!IsActive())
    {
        return;
    }
    if (!ReadContext(ActorInfo, Body, ASC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    if (UCharacterMovementComponent* Movement = Body->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
    UWorld* World = Body->GetWorld();
    if (!IsValid(World))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    TimerWorld = World;
    const uint64 Serial = ++ActivationSerial;
    World->GetTimerManager().SetTimer(
        DurationTimer,
        FTimerDelegate::CreateUObject(this, &ThisClass::HandleDurationFinished, Serial),
        StaggerDuration, false);
    UE_LOG(LogETStagger, Display, TEXT("[M2-05B1] Stagger begin Body=%s Duration=%.2f"),
           *GetNameSafe(Body), StaggerDuration);
}

void UETGA_Stagger::HandleDurationFinished(uint64 Serial)
{
    if (!IsActive() || Serial != ActivationSerial)
    {
        return;
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_Stagger::EndAbility(const FGameplayAbilitySpecHandle Handle,
                               const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo,
                               bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo))
    {
        return;
    }
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }
    const FString BodyName = GetNameSafe(ActorInfo->AvatarActor.Get());
    const TWeakObjectPtr<UAbilitySystemComponent> ASC = ActorInfo->AbilitySystemComponent;
    ++ActivationSerial;
    if (UWorld* World = TimerWorld.Get())
    {
        World->GetTimerManager().ClearTimer(DurationTimer);
    }
    DurationTimer.Invalidate();
    TimerWorld.Reset();
    // 先作废回调 / 清Timer，再由GAS撤销自己的Tag计数和Block关系。
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogETStagger, Display, TEXT("[M2-05B1] Stagger end Body=%s Cancelled=%d Stunned=%d"),
           *BodyName, bWasCancelled ? 1 : 0,
           ASC.IsValid() ? ASC->GetTagCount(ETGameplayTags::State_Stunned) : 0);
}
```

Stagger不使用DisableMovement再切回Walking，因此结束回调不会把已死亡身体“恢复”成能移动。普通输入由公共Idle门控阻断，AI活动路径通过Controller.StopMovement立即取消，之后AI既有Service读取同一个Idle条件。若GAS ScopeLock尚未完成旧动作取消，本次拒绝叠加；B2从属性回调派发时会在统一边界延后一次处理，避免递归取消。

## 4. 新身体组件：只拥有授予句柄与反应入口

与CombatLife的死亡所有权分开，Component保存自己的Stagger Spec、观察Ready并负责撤销；本批还不订阅Poise变化。未配置Class的原生自动化身体仍可使用属性，不会因新组件令旧夹具失败；正式三个身体由Validator要求配置。

### 4.1 ETCombatPoiseComponent.h：完整内容

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "ETCombatPoiseComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETGA_Stagger;

// 每身体一份反应入口；本批只拥有Spec，下一批再观察Poise归零。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETCombatPoiseComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETCombatPoiseComponent();
    bool RequestStagger();
    bool HasValidConfiguration() const;
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETStaggerAbilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Poise")
    TSubclassOf<UETGA_Stagger> StaggerAbilityClass;

    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FGameplayAbilitySpecHandle StaggerHandle;
    bool bEndingPlay = false;

    bool ReadBody(AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC) const;
    void StartObserving();
    void StopObserving();

    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* Body, bool bReady);
};
```

### 4.2 ETCombatPoiseComponent.cpp：完整内容

```cpp
#include "Combat/ETCombatPoiseComponent.h"
#include "Abilities/ETGA_Stagger.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameplayAbilitySpec.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCombatPoise, Log, All);

UETCombatPoiseComponent::UETCombatPoiseComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UETCombatPoiseComponent::BeginPlay()
{
    Super::BeginPlay();
    bEndingPlay = false;
    BodyOwner = Cast<AEmberTrialCharacter>(GetOwner());
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleReadyChanged);
    }
    StartObserving();
}

void UETCombatPoiseComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    if (HasBegunPlay())
    {
        StartObserving();
    }
}

void UETCombatPoiseComponent::Deactivate()
{
    Super::Deactivate();
    StopObserving();
}

bool UETCombatPoiseComponent::HasValidConfiguration() const
{
    return StaggerAbilityClass && !StaggerAbilityClass->HasAnyClassFlags(CLASS_Abstract) &&
           StaggerAbilityClass->GetDefaultObject<UETGA_Stagger>()->HasValidConfiguration();
}

bool UETCombatPoiseComponent::ReadBody(AEmberTrialCharacter*& Body,
                                       UAbilitySystemComponent*& ASC) const
{
    Body = BodyOwner.Get();
    ASC = IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (bEndingPlay || !HasBegunPlay() || !IsActive() || !IsValid(Body) ||
        Body->IsActorBeingDestroyed() || !Body->HasAuthority() || Body->IsCombatDead() ||
        !IsValid(ASC) || !ASC->AbilityActorInfo.IsValid() ||
        ASC->AbilityActorInfo->OwnerActor.Get() != Body ||
        ASC->AbilityActorInfo->AvatarActor.Get() != Body ||
        !Body->GetGameplayHealthSnapshot(Health, MaxHealth) || Health <= 0.0f)
    {
        return false;
    }
    TInlineComponentArray<UETCombatPoiseComponent*> Components;
    Body->GetComponents(Components);
    return Components.Num() == 1 && Components[0] == this;
}

void UETCombatPoiseComponent::StartObserving()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    // 无资源配置的原生测试身体可保留属性；正式资产由Validator要求配置。
    if (!HasValidConfiguration() || !ReadBody(Body, ASC))
    {
        return;
    }
    if (BoundASC.Get() == ASC && StaggerHandle.IsValid())
    {
        return;
    }
    StopObserving();
    BoundASC = ASC;
    StaggerHandle =
        ASC->GiveAbility(FGameplayAbilitySpec(StaggerAbilityClass.Get(), 1, INDEX_NONE, this));
    UE_LOG(LogETCombatPoise, Display, TEXT("[M2-05B1] Reaction ready Body=%s Granted=%d"),
           *GetNameSafe(Body), StaggerHandle.IsValid() ? 1 : 0);
}

bool UETCombatPoiseComponent::RequestStagger()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    bool bAccepted = false;
    if (HasValidConfiguration() && ReadBody(Body, ASC) && BoundASC.Get() == ASC &&
        StaggerHandle.IsValid() && !UGameplayStatics::IsGamePaused(Body))
    {
        const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(StaggerHandle);
        if (Spec && Spec->Ability && Spec->Ability->GetClass() == StaggerAbilityClass.Get())
        {
            bAccepted = ASC->TryActivateAbility(StaggerHandle);
        }
    }
    UE_LOG(LogETCombatPoise, Display, TEXT("[M2-05B1] Stagger request Body=%s Accepted=%d"),
           *GetNameSafe(BodyOwner.Get()), bAccepted ? 1 : 0);
    return bAccepted;
}

void UETCombatPoiseComponent::StopObserving()
{
    UAbilitySystemComponent* ASC = BoundASC.Get();
    const FGameplayAbilitySpecHandle OldHandle = StaggerHandle;
    BoundASC.Reset();
    StaggerHandle = FGameplayAbilitySpecHandle();
    if (IsValid(ASC) && OldHandle.IsValid())
    {
        ASC->CancelAbilityHandle(OldHandle);
        ASC->ClearAbility(OldHandle);
    }
}

void UETCombatPoiseComponent::HandleReadyChanged(AEmberTrialCharacter* Body, bool bReady)
{
    if (Body != BodyOwner.Get() || bEndingPlay)
    {
        return;
    }
    if (bReady)
    {
        StartObserving();
    }
    else
    {
        StopObserving();
    }
}

void UETCombatPoiseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleReadyChanged);
    }
    StopObserving();
    BodyOwner.Reset();
    Super::EndPlay(EndPlayReason);
}
```

StartObserving对同ASC / 有效Handle不重复授予；StopObserving先清成员归属，再取消并移除旧Handle。运行时还检查身体只有一个本组件。Blueprint只配置Ability Class与Auto Activate，不能再Add第二个组件或手动Give Ability。

## 5. 新自动化：真实能力取消、暂停与结束

创建`Tests/ETStaggerAbilityTests.cpp`，完整内容如下。它加载保存的正式GA和Quinn，在独立瞬态世界检查七组场景，不修改旧测试或保存测试资产。时长按真实条件等到结束并留余量，避免恰好卡在Timer边界。Scope / 资源 / 生命周期逻辑由代码检查，你不用手掐攻击中断时刻。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatPoiseComponent.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETStaggerAbilityTest, "EmberTrial.M2.Combat.StaggerAbility",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETStaggerAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* StaggerClass = LoadClass<UETGA_Stagger>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger.GA_ET_Stagger_C"));
    UClass* AttackClass = LoadClass<UETGA_LightAttack>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass = LoadClass<UETGA_Dodge>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    UClass* HealClass =
        LoadClass<UETGA_Heal>(nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal.GA_ET_Heal_C"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved Stagger GA"), StaggerClass) ||
        !TestNotNull(TEXT("Saved Attack GA"), AttackClass) ||
        !TestNotNull(TEXT("Saved Dodge GA"), DodgeClass) ||
        !TestNotNull(TEXT("Saved Heal GA"), HealClass) ||
        !TestNotNull(TEXT("Saved Hit GA"), HitClass) || !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_Stagger* Defaults = StaggerClass->GetDefaultObject<UETGA_Stagger>();
    if (!TestTrue(TEXT("Saved reaction policy"), Defaults->HasValidConfiguration()) ||
        !TestEqual(TEXT("Saved duration contract"), Defaults->StaggerDuration, 1.2f))
    {
        return false;
    }
    enum class EScenario : uint8
    {
        GatesAndPause,
        Attack,
        Dodge,
        Heal,
        Hit,
        Death,
        EndPlay
    };
    for (const EScenario Scenario :
         {EScenario::GatesAndPause, EScenario::Attack, EScenario::Dodge, EScenario::Heal,
          EScenario::Hit, EScenario::Death, EScenario::EndPlay})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = Fixture.GetTestWorld();
        AActor* Floor = World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Floor"), Floor))
        {
            return false;
        }
        UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(Box);
        Floor->SetRootComponent(Box);
        Box->SetMobility(EComponentMobility::Static);
        Box->SetBoxExtent(FVector(2000, 2000, 50));
        Box->SetRelativeLocation(FVector(0, 0, -50));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Controller"), PC))
        {
            return false;
        }
        Body->CombatTeam = EETCombatTeam::Player;
        Body->InitialHealth = 40.0f;
        Body->LightAttackAbilityClass = AttackClass;
        Body->DodgeAbilityClass = DodgeClass;
        Body->HealAbilityClass = HealClass;
        UETCombatPoiseComponent* Poise = Body->FindComponentByClass<UETCombatPoiseComponent>();
        if (!TestNotNull(TEXT("Reaction component"), Poise))
        {
            return false;
        }
        Poise->StaggerAbilityClass = StaggerClass;
        Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Body->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->GetCharacterMovement()->bRunPhysicsWithNoController = true;
        PC->Possess(Body);
        TestFalse(TEXT("Before BeginPlay rejects reaction"), Body->RequestStagger());
        if (!Fixture.BeginPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        auto Advance = [&Fixture](float Seconds)
        {
            for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
            {
                if (!Fixture.TickTestWorld(.01f))
                {
                    return false;
                }
            }
            return true;
        };
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) ||
            !TestTrue(TEXT("Grounded"), Body->GetCharacterMovement()->IsMovingOnGround()))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        const FGameplayAbilitySpecHandle ReactionHandle = Poise->StaggerHandle;
        FGameplayAbilitySpec* ReactionSpec = ASC->FindAbilitySpecFromHandle(ReactionHandle);
        UETGA_Stagger* Reaction =
            ReactionSpec ? Cast<UETGA_Stagger>(ReactionSpec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Reaction instance"), Reaction))
        {
            return false;
        }
        // 直接授予真实受击GA；不修改CombatLife的私有配置或旧测试。
        const FGameplayAbilitySpecHandle HitHandle =
            ASC->GiveAbility(FGameplayAbilitySpec(HitClass, 1));
        if (Scenario == EScenario::Attack)
        {
            TestTrue(TEXT("Attack starts"), Body->RequestLightAttack());
            if (!Advance(.1f))
            {
                return false;
            }
        }
        else if (Scenario == EScenario::Dodge)
        {
            TestTrue(TEXT("Dodge starts"), Body->RequestDodge());
            if (!Advance(.05f))
            {
                return false;
            }
        }
        else if (Scenario == EScenario::Heal)
        {
            TestTrue(TEXT("Heal starts"), Body->RequestHeal());
            if (!Advance(.1f))
            {
                return false;
            }
        }
        else if (Scenario == EScenario::Hit)
        {
            TestTrue(TEXT("Hit starts"), ASC->TryActivateAbility(HitHandle));
            if (!Advance(.05f))
            {
                return false;
            }
        }
        TestTrue(TEXT("Reaction accepted"), Body->RequestStagger());
        TestEqual(TEXT("One owned Stunned tag"), ASC->GetTagCount(ETGameplayTags::State_Stunned),
                  1);
        TestFalse(TEXT("Reaction does not restart itself"), Body->RequestStagger());
        TestFalse(TEXT("Formal actions blocked"), Body->IsFormalActionIdle());
        TestFalse(TEXT("Attack blocked"), Body->RequestLightAttack());
        TestFalse(TEXT("Dodge blocked"), Body->RequestDodge());
        TestFalse(TEXT("Heal blocked"), Body->RequestHeal());
        TestFalse(TEXT("Hit blocked"), ASC->TryActivateAbility(HitHandle));
        TestFalse(TEXT("Old attack cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
        TestFalse(TEXT("Old dodge cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging));
        TestFalse(TEXT("Old heal cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing));
        TestFalse(TEXT("Old hit cleared"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact));
        const uint64 OldSerial = Reaction->ActivationSerial;
        const FTimerHandle TimerAtStart = Reaction->DurationTimer;

        if (Scenario == EScenario::GatesAndPause)
        {
            UGameplayEffect* Damage = NewObject<UGameplayEffect>(GetTransientPackage());
            Damage->DurationPolicy = EGameplayEffectDurationType::Instant;
            FGameplayModifierInfo& Mod = Damage->Modifiers.AddDefaulted_GetRef();
            Mod.Attribute = UETHealthAttributeSet::GetHealthAttribute();
            Mod.ModifierOp = EGameplayModOp::AddBase;
            Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.0f));
            ASC->ApplyGameplayEffectSpecToSelf(
                FGameplayEffectSpec(Damage, ASC->MakeEffectContext(), 1.0f));
            TestEqual(TEXT("Damage still applies while Stunned"),
                      ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                      30.0f);
            TestTrue(TEXT("Damage does not restart reaction"),
                     Reaction->ActivationSerial == OldSerial);
            ASC->SetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute(), 50.0f);
            if (!Advance(.3f))
            {
                return false;
            }
            TestEqual(TEXT("Recovery suspended by Stunned"),
                      ASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
                      50.0f);
            APlayerState* Pauser = World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Pauser"), Pauser))
            {
                return false;
            }
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            const double PausedAt = World->GetTimeSeconds();
            if (!Advance(1.0f))
            {
                return false;
            }
            TestTrue(
                TEXT("Pause keeps game time"),
                FMath::IsNearlyEqual(static_cast<double>(World->GetTimeSeconds()), PausedAt, .001));
            TestTrue(TEXT("Pause keeps reaction"),
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            TestFalse(TEXT("Pause refuses another request"), Body->RequestStagger());
            World->GetWorldSettings()->SetPauserPlayerState(nullptr);
            // 按实际条件等到结束，保留计时余量，不断言恰好落在某个Timer边界。
            for (int32 I = 0; I < 150 && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned);
                 ++I)
            {
                if (!Advance(.01f))
                {
                    return false;
                }
            }
            TestFalse(TEXT("Natural duration expires"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            TestTrue(TEXT("No early expiry"), World->GetTimeSeconds() - PausedAt >= .88);
            TestTrue(TEXT("Actions restored"), Body->IsFormalActionIdle());
            TestTrue(TEXT("A later reaction starts"), Body->RequestStagger());
            Reaction->HandleDurationFinished(OldSerial);
            TestTrue(TEXT("Old callback cannot end new reaction"),
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            const FTimerHandle NewTimer = Reaction->DurationTimer;
            ASC->AddLooseGameplayTag(ETGameplayTags::State_Stunned);
            ASC->CancelAbilityHandle(ReactionHandle);
            TestEqual(TEXT("Only own tag count removed"),
                      ASC->GetTagCount(ETGameplayTags::State_Stunned), 1);
            TestFalse(TEXT("Cancel clears own duration timer"),
                      World->GetTimerManager().TimerExists(NewTimer));
            ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Stunned);
        }
        else if (Scenario == EScenario::Dodge)
        {
            const FVector StoppedAt = Body->GetActorLocation();
            if (!Advance(.35f))
            {
                return false;
            }
            TestTrue(TEXT("Cancelled dodge leaves no horizontal drift"),
                     FVector::Dist2D(StoppedAt, Body->GetActorLocation()) < .1f);
            TestEqual(TEXT("Dodge does not refund cost"),
                      ASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
                      75.0f);
            TestFalse(TEXT("Dodge invulnerability cleared"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        }
        else if (Scenario == EScenario::Heal)
        {
            if (!Advance(.65f))
            {
                return false;
            }
            int32 Charges = 0;
            int32 Maximum = 0;
            UETHealingResourceComponent* Resource =
                Body->FindComponentByClass<UETHealingResourceComponent>();
            TestTrue(TEXT("Healing resource available"),
                     Resource && Resource->GetHealingSnapshot(Charges, Maximum));
            TestEqual(TEXT("Before-commit cancel keeps charges"), Charges, 3);
            TestEqual(TEXT("Before-commit cancel keeps health"),
                      ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                      40.0f);
        }
        else if (Scenario == EScenario::Attack)
        {
            TestFalse(
                TEXT("Cancelled attack trace is off"),
                Body->FindComponentByClass<UETCombatTraceComponent>()->IsComponentTickEnabled());
        }
        else if (Scenario == EScenario::Death)
        {
            ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
            TestTrue(TEXT("Death wins"), Body->IsCombatDead());
            TestFalse(TEXT("Death removes Stunned"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            if (!Advance(1.5f))
            {
                return false;
            }
            TestTrue(TEXT("Death movement remains sealed"),
                     Body->GetCharacterMovement()->MovementMode == MOVE_None);
            TestFalse(TEXT("Dead body refuses reaction"), Body->RequestStagger());
        }
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("EndPlay releases reaction tag"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
        TestFalse(TEXT("EndPlay clears duration timer"),
                  World->GetTimerManager().TimerExists(TimerAtStart));
        TestTrue(TEXT("EndPlay removes owned Spec"),
                 ASC->FindAbilitySpecFromHandle(ReactionHandle) == nullptr);
        TestFalse(TEXT("EndPlay refuses requests"), Body->RequestStagger());
        Fixture.ForwardErrorMessages(this);
    }
    return !HasAnyErrors();
}
#endif
```

新测试依赖第8节的新GA资产，必须创建 / 保存后才运行。最终时长契约为1.2；第9节若临时用4秒观察，要恢复1.2再跑测试。测试会真实检查旧序号回调不会结束新一轮、只移除自己一份Tag、取消 / EndPlay清Timer，不能删断言来取得Success。

## 6. 七份既有源码逐处接入

每处只替换列出的片段，其余代码保留；h中的generated.h仍为最后一个include。下面19个锚点已针对A最终实物定位，不用旧完整Character覆盖当前文件。

### 6.1 ETGameplayTags.h / .cpp：两个Tag

在h中：
找到：

```cpp
// ---- 治疗动作 ----
```
替换为：

```cpp
// ---- 破势反应 ----
EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Stagger);
EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned);
// ---- 治疗动作 ----
```

在.cpp中：
找到：

```cpp
// ---- 治疗动作 ----
```
替换为：

```cpp
// ---- 破势反应 ----
UE_DEFINE_GAMEPLAY_TAG(Ability_Stagger, "Ability.Stagger");
UE_DEFINE_GAMEPLAY_TAG(State_Stunned, "State.Stunned");
// ---- 治疗动作 ----
```

不在DefaultGameplayTags.ini再注册同名Tag；状态只由GA拥有，UI / Component不AddLooseGameplayTag。

### 6.2 Character.h：四处增补

**前置声明：**
找到：

```cpp
class UETCombatLifeComponent;
```
替换为：

```cpp
class UETCombatLifeComponent;
class UETCombatPoiseComponent;
```

**新测试friend：**
找到：

```cpp
    friend class FETPoiseFoundationTest;
```
替换为：

```cpp
    friend class FETPoiseFoundationTest;
    friend class FETStaggerAbilityTest;
```

**同一私有区域添加原生组件引用：**原CombatLife字段的UPROPERTY仍保留在前面。
找到：

```cpp
    TObjectPtr<UETCombatLifeComponent> CombatLifeComponent;
```
替换为：

```cpp
    TObjectPtr<UETCombatLifeComponent> CombatLifeComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Poise",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETCombatPoiseComponent> CombatPoiseComponent;
```

**public区新增反应接口：**原Idle查询继续保留。
找到：

```cpp
    UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
    bool IsFormalActionIdle() const;
```
替换为：

```cpp
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Combat")
    bool RequestStagger();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
    bool IsFormalActionIdle() const;
```

### 6.3 Character.cpp：六处接入

**include：**
找到：

```cpp
#include "Combat/ETCombatLifeComponent.h"
```
替换为：

```cpp
#include "Combat/ETCombatLifeComponent.h"
#include "Combat/ETCombatPoiseComponent.h"
```

**构造函数单次创建：**
找到：

```cpp
    CombatLifeComponent = CreateDefaultSubobject<UETCombatLifeComponent>(TEXT("CombatLife"));
```
替换为：

```cpp
    CombatLifeComponent = CreateDefaultSubobject<UETCombatLifeComponent>(TEXT("CombatLife"));
    CombatPoiseComponent = CreateDefaultSubobject<UETCombatPoiseComponent>(TEXT("CombatPoise"));
```

**攻击请求先拒绝Stunned，避免把新点击送进旧连段缓冲：**
找到：

```cpp
bool AEmberTrialCharacter::RequestLightAttack()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !LightAttackAbilityHandle.IsValid())
```
替换为：

```cpp
bool AEmberTrialCharacter::RequestLightAttack()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !LightAttackAbilityHandle.IsValid() ||
        AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned))
```

**在Idle函数之前添加RequestStagger定义：**最后的Idle函数签名保留，后面原来的`{`与函数体仍在。
找到：

```cpp
bool AEmberTrialCharacter::IsFormalActionIdle() const
```
替换为：

```cpp
bool AEmberTrialCharacter::RequestStagger()
{
    return IsValid(CombatPoiseComponent) && CombatPoiseComponent->RequestStagger();
}

bool AEmberTrialCharacter::IsFormalActionIdle() const
```

**Idle返回条件补Stunned：**原移动 / 跳跃 / E与AI读取此公共条件，无需再复制状态到蓝图布尔变量。
找到：

```cpp
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Healing);
```
替换为：

```cpp
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Healing) &&
           !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Stunned);
```

**同步Ready注释：**属性可用与行动许可分开。
找到：

```cpp
// Ready表示初始化完成；零生命、零体力或失去控制不会单独撤销它。
```
替换为：

```cpp
// Ready表示三组属性初始化完成；零资源、失去控制或Stunned不单独撤销它。
```

### 6.4 ETGA_HitReact.cpp：普通受击不能覆盖破势

在CanActivateAbility的已有拒绝条件中增加检查，不更改HitReact的取消攻击 / 闪避 / 治疗策略：
找到：

```cpp
        UGameplayStatics::IsGamePaused(Character))
```
替换为：

```cpp
        Character->GetAbilitySystemComponent()->HasMatchingGameplayTag(ETGameplayTags::State_Stunned) ||
        UGameplayStatics::IsGamePaused(Character))
```

### 6.5 ETStaminaRecoveryComponent.cpp：共用阻断名单加Stunned

只改GetStaminaRecoveryBlockingTags中的数组尾部。注册 / 解绑与ShouldRecover共用同一名单，原Handle数组配对规则保留：
找到：

```cpp
        ETGameplayTags::State_Dodging, ETGameplayTags::State_Healing};
```
替换为：

```cpp
        ETGameplayTags::State_Dodging, ETGameplayTags::State_Healing, ETGameplayTags::State_Stunned};
```

### 6.6 ETGameplayAssetValidator.cpp：五处

**include：**
找到：

```cpp
#include "Abilities/ETGA_Heal.h"
```
替换为：

```cpp
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_Stagger.h"
#include "Combat/ETCombatPoiseComponent.h"
```

**GetGameplayValidationParent的Profiles表加入新GA：**
找到：

```cpp
        {ETGameplayValidationPaths::PlayerAttackPackage, UETGA_LightAttack::StaticClass()},
```
替换为：

```cpp
        {TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger"), UETGA_Stagger::StaticClass()},
        {ETGameplayValidationPaths::PlayerAttackPackage, UETGA_LightAttack::StaticClass()},
```

**ValidateBlueprint在原普通GA分派前处理Stagger：**最后的旧`else if`续行、括号与原函数体全部保留。
找到：

```cpp
    else if (Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>() ||
```
替换为：

```cpp
    else if (const UETGA_Stagger* Stagger = Cast<UETGA_Stagger>(Defaults))
    {
        if (!Stagger->HasValidConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-STAGGER-001"),
                     TEXT("Stagger requires valid duration / owned state / cancel-block policy, "
                          "ServerOnly, and no Cost or Cooldown GE."), Issues);
        }
    }
    else if (Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>() ||
```

**ValidateBody的必需引用列表：**
找到：

```cpp
TEXT("CombatTraceComponent"), TEXT("CombatLifeComponent")})
```
替换为：

```cpp
TEXT("CombatTraceComponent"), TEXT("CombatLifeComponent"), TEXT("CombatPoiseComponent")})
```

**ValidateBody在原训练目标分区注释前验证组件Class：**
找到：

```cpp
    // 训练目标无需攻击；玩家才要求正式输入、闪避和治疗引用。
```
替换为：

```cpp
    const FObjectPropertyBase* ReactionProperty = FindFProperty<FObjectPropertyBase>(
        Defaults->GetClass(), TEXT("CombatPoiseComponent"));
    const UETCombatPoiseComponent* Reaction = ReactionProperty
        ? Cast<UETCombatPoiseComponent>(ReactionProperty->GetObjectPropertyValue_InContainer(Defaults))
        : nullptr;
    if (IsValid(Reaction) && !Reaction->HasValidConfiguration())
    {
        AddIssue(Blueprint, TEXT("ET-STAGGER-002"),
                 TEXT("CombatPoise requires a configured, valid Stagger ability class."), Issues);
    }

    // 训练目标无需攻击；玩家才要求正式输入、闪避和治疗引用。
```

校验规则：ET-STAGGER-001对应新GA参数 / GAS策略，ET-STAGGER-002对应身体组件配置。仍只读CDO，不授予能力或播放动作。本批不新增故意非法Probe；新GA与三个身体Valid即可。

## 7. 关闭UE后正常双Build与索引

保存十二份Source，确认UE关闭，PowerShell先运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor成功后在已有CLion工程Reload CMake。预期Core48 / UI17 / Editor3，共68cpp，五个新文件有正确目标；不要新开仅包含两个文件的目录或手改生成CMake。再保持UE关闭运行Game：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

新Native Tag / 类反射使用正常构建；Build失败发首条真正C++ / UHT / 链接错误及文件行号，不只发UBT exit code 6。不要先删Tests或改断言。两个目标通过再打开UE。

## 8. 新GA与三身体装配

### 8.1 新建GA_ET_Stagger

1. Content Browser到`/Game/EmberTrial/Abilities`，空白右键 → Blueprint Class → All Classes，搜索`ETGA_Stagger`原生类，创建`GA_ET_Stagger`。不要复制旧HitReact / Heal GA，也不创建Gameplay Effect代替能力。
2. 打开Class Settings确认Parent Class=ETGA_Stagger。Class Defaults搜索`Stagger Duration`设1.2，暂不配Montage；本类目前没有动画属性。
3. Event Graph保持空，不写Activate Ability、Delay、Add Tag或End Ability蓝图链。计时与清理由C++完成。
4. Cost Gameplay Effect Class / Cooldown Gameplay Effect Class=None。Instancing=Instanced Per Actor，Net Execution=Server Only，Replication=Replicate No，Retrigger=False。原生已给默认，逐项确认。
5. 标签由原生提供，核对下表，不给其他GA另加一组新配置。若UE面板标签名与你看到的显示不同，先用本项目校验确认，不按猜测改旧GA；缺失时发截图。

| 新GA属性 | 精确内容 |
| --- | --- |
| Asset / Ability Tags | Ability.Stagger，仅一项 |
| Activation Owned Tags | State.Stunned，仅一项 |
| Activation Blocked Tags | State.Stunned、State.Dead |
| Cancel Abilities With Tag | Ability.Attack.Light、Ability.Dodge、Ability.Heal、Ability.HitReact |
| Block Abilities With Tag | 与Cancel相同四项，注意不要误填State标签 |

Compile → Save → Content Browser右键Asset Actions → Validate Assets，预期项目[M1-07C2] Valid / Issues0。新资源路径`/Game/EmberTrial/Abilities/GA_ET_Stagger`。

### 8.2 逐个配置三正式身体

依次打开玩家`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`、训练目标`/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget`、敌人`/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy`。

每个身体按相同步骤：

1. Components选原生继承的`CombatPoise`，应只有一份；不要Add Component。若未出现，先确认关闭UE后的Editor正常Build，以及打开的父类仍为EmberTrialCharacter。
2. Details搜索`Stagger Ability Class`，设`GA_ET_Stagger`；Auto Activate=True。此处是新组件属性，不是旧CombatLife的Hit React Ability Class。
3. 保持CombatLife原HitReact Class、三个属性集 / ASC、Initial Health / Stamina / Poise100/100、玩家Healing3/3、三正式能力Class与旧输入原值。
4. Compile → Save，再逐个Validate Assets。三身体均应项目[M1-07C2] Valid / Issues0，不只看UE总计Validated。

旧攻击 / 闪避 / 治疗GA本批无计划变更，不重做标签列表；GAS阻断由新Stagger统一拥有。NoPawn与故意非法Waiting测试资产不新增 / 不改，不放入地图。

## 9. 三组短查与相关自动化：约8–12分钟

不搭临时Controller图，复用`ke`调用正式反应接口。游戏内控制台命令在PIE视口按`~`打开后输入，Enter，随后关控制台。必须在当前游戏World，不是PowerShell或未Play的编辑器世界。`Called succeeded`只表示函数分派成功，真正接受看`[M2-05B1] Stagger request Accepted=1`。

### 9.1 玩家状态、镜头与P

1. 为方便观察，可先在GA_ET_Stagger Class Defaults把Duration临时改4.0，Compile / Save，再Play；仅这一项临时值，不修改C++。最终必须恢复1.2，第9.4的测试会检查。
2. 从正式地图新Play，先靠近尚未激活的A/B使提示可见，并避开敌人；出生三资源仍正常，Poise100/100。安全位置游戏控制台执行：

```text
ke BP_ThirdPersonCharacter_C RequestStagger
```

3. 立即关控制台，尝试移动 / 跳跃 / 左键 / 2 / Q / E，应拒绝；镜头仍可转动。本批没有新动画，身体暂时停住是预期，不要求最终动作视觉。你不必在4秒里试完全部键，精确门控由自动化覆盖。
4. 同一段中P暂停，菜单正常；继续后剩余状态结束可行动，无补播 / 卡键。若操作慢错过该段，再请求一次即可，不把精确按键时点作为验收任务。
5. 日志应有一次begin / end；反应中的再次请求Accepted0，不增加时长。Poise和生命不因强制反应发生变化，资源不会被本GA消费或回填。

### 9.2 敌人移动与新旧流程

1. 让既有MeleeEnemy_A正常发现玩家并追击。游戏控制台执行：

```text
ke BP_ETMeleeEnemy_C RequestStagger
```

2. 关控制台，敌人应暂时停走，状态结束后重新判断目标 / 继续追击或攻击。无需手掐它挥击到哪一帧；活动攻击 / 闪避位移 / 治疗取消由真实自动化另查。
3. 玩家左键25、4后Q、正式2与原HUD短基线正常。一次Stop再Play检查新身体 / 单HUD和清理；若便于操作，可在临时4秒反应中Stop，但不要求复做全部菜单矩阵。

### 9.3 恢复正式配置并保存

Stop PIE，把GA_ET_Stagger Duration恢复**1.2** → Compile → Save → Validate Assets。新GA与三身体的Class保持正确，Poise / Health / Stamina默认100/100、Healing3/3，slomo1、Debug False不变，回正式地图Save All。不能把4秒临时值Push。

### 9.4 三项自动化，各一次

非PIE的Output Log逐条执行，等上一项最终完成再输入下一项：

```text
Automation RunTests EmberTrial.M2.Combat.StaggerAbility
Automation RunTests EmberTrial.M2.Combo.RecoveryDodge
Automation RunTests EmberTrial.M2.Healing.Ability
```

新测试七组实际检查：时长 / 暂停 / 旧回调 / Tag计数 / 清Timer，攻击、闪避、治疗、受击被取消，死亡及EndPlay优先级；两旧项覆盖公共门控接入后的连段恢复取消与治疗。三项应各Success；不新增模仿getter的测试，不重跑未变14项全矩阵。若新测试失败，贴第一条断言与上下文，先定位原因，不放宽时长 / 漂移断言。

本批不证明Poise0会自动触发、范围命中 / 冷却或新破势动画，因为这些尚未接入；也未新跑完整导航 / Unsupported Pawn矩阵，同身体换控以生命周期结构检查为主。记录范围，不为填表造临时验收代码。

## 10. 完成反馈

```text
M2-05B1 完成
五新 / 七旧Source；关闭UE后Editor Build / Core48-UI17-Editor3索引 / Game Build：
新GA父类 / 1.2秒 / Tags与Cancel-Block四项 / 无Cost-Cooldown / 空Graph：
三身体单CombatPoise / Class与AutoActivate / 四资产Compile-Save-Valid：
玩家强制反应 / 门控 / 镜头与P / 结束可行动无补播：
敌人活动追击停止、结束重判恢复 / 原能力与HUD短基线：
StaggerAbility / RecoveryDodge / Healing.Ability各一次：
Stop再开与清理 / 正式Duration1.2与原默认值恢复 / Save All：
失败或最明显表现问题（无填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

日志文字优先；Blueprint装配问题截图Components / Details / 父类，输入与追击问题写操作顺序和结果。无需逐条手抄日志或精确记录测试时点，我读取日志核对。无待补项时保存B1报告并下发B2，继续由你手动C++与UE配置。

## 11. 本批 Commit / Push（固定放在最后）

### 先提交已验收A及本轮文档

C已在本机HEAD `5a340f5`，不用重复Stage。本次A三新Source暂存仍是0字节骨架，重新Stage工作区完整代码；其余三旧Source / 来源 / 报告与下一章文档一起提交，详见[A末尾](M2_05A_PoiseFoundation.md#11-本批-commit--push固定放在最后)。B1 Source仍只在指南和本地Raw候选，不存在已实施的B1源码。

```text
feat: establish poise attributes and initialization contract
```

### B1实施、验收收尾后

- 第2节十二份Source最终完整修改；新文件创建时自动Stage的空骨架需再次Stage最新实现。
- 新`EmberTrial/Content/EmberTrial/Abilities/GA_ET_Stagger.uasset`，最终Duration1.2。
- 实际变动的三个身体：`EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`、`EmberTrial/Content/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget.uasset`、`EmberTrial/Content/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy.uasset`。
- 届时的`Docs/Reports/M2_05B1_Verification.md`、来源清单与本指南 / 破势路线 / Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / M2路线 / README / Introduction最新差异；B1报告本轮尚未生成。
- 旧GA / 输入 / 地图 / UI / Config / Build.cs / Target本批无计划变化，有额外差异先核对原因，不强制重存。

```text
feat: add GAS stagger state and action gating
```

用户刷新CLion Changes → 重新Stage最新完整修改 → 检查Staged Diff与资产LFS → Commit → Push。Raw / 日志 / 生成CodeModel / Saved / Binaries / Intermediate及包体留本地，不Force Add；助手不操作Git索引。
