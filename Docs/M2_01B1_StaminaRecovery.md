# M2-01B1：体力恢复、阻断与复用检查入口

**2026-10-15已验收，手动40 / 等待5分钟，无待补项。**见[B1报告](Reports/M2_01B1_Verification.md)。下一步[B2体力显示](M2_01B2_StaminaHUD.md)。下文保留原下发说明，“仅文档 / 尚未Build”表示下发时状态，不要求重做通过项；当前实际83Source / 42cpp / 清单48。

生成2026-10-14，文档复核2026-10-15，固定 **UE 5.5.4**。前置：[M2-01A已验收](Reports/M2_01A_Verification.md)，当前实际78Source / 39cpp / 来源48项。**本页代码仅在文档，待你手动实现，尚未编译或实测。**原C++手动约定与方案B继续。

M2-01B拆两批：**B1完成恢复规则 / 检查入口，B2接事件驱动的体力观察 / 显示**。本页是B1的完整实施指南；B2沿用LocalPlayer → Presenter → Widget的数据流，在B1通过后下发，不同时扩展两组生命周期。预计手动 **80–130分钟**：代码50–85、装配10–15、检查5–15、保存记录5–10；Build / 索引等待另列。预算不计为实际用时。

## 1. 完成后怎样运行

体力降低后，延迟0.75世界秒；正式动作空闲时，以20点 / 世界秒恢复，默认每0.10秒提交一次真实Instant GE。恢复到上限停止计时器；正式GAS攻击 / 受击时立即停表，结束后按未暂停的时间继续，但不补算被阻断的一整段时间。死亡立即停表，EndPlay解绑与清计时器。

本批没有体力条，先使用一个长期保留的PIE开发命令读数 / 注入一次增量与自动测试检查。无需临时Controller蓝图、临时按键或精确手动掐时间。B2体力条接好后仍能复用这个入口。

| 规则 | 本批决定 |
| --- | --- |
| 恢复所有者 | 身体继承一个 `StaminaRecovery` 原生组件，唯一持有计时器 / ASC订阅；组件不另存实时体力 |
| 资源变化 | 原生 `UETStaminaDeltaEffect` 固定Instant、单Stamina AddBase、SetByCaller.StaminaDelta；调用方传实际增量 |
| 默认配置 | Recovery Rate=20、Recovery Delay=0.75、Recovery Interval=0.10、Auto Activate=True |
| 延迟重置 | **任何实际体力降低**都重置延迟，包括上限下降导致的压低；不根据请求量推断实际消耗 |
| 忙碌门控 | 当前正式Attacking / HitReact / Dead的Tag事件直接停 / 开表；最后一个正式忙碌Tag退出后才恢复 |
| 原型边界 | 按2仍为M0 Practice，没有正式体力成本 / Tag，本批不接管它；其播放期间可按本规则恢复。M2-02迁移正式闪避时加入Dodging门控 |
| 时间 | 用World.GetTimeSeconds和World TimerManager；世界暂停 / slomo都按世界时间，恢复后不按真实墙钟补量 |
| 帧率 | 按真实经过的世界时间计算增量，不能每次回调硬加2点；显示 / 生效有一个采样周期的离散误差 |
| 满体力 / 死亡 | 没有恢复计时器，不开组件Tick；上限增加产生缺额后可重新恢复，增加上限本身不直接补体力 |
| 重绑 / 关闭 | Activate / Ready重观察先解绑；Deactivate / Ready=False / EndPlay清理；旧ASC / 旧观察序号回调不能影响新周期 |
| Ready语义 | A的Ready保证生命 / 体力属性可用，不表示当前存活、资源足够或恢复组件已开启 |

计时器可用Handle控制和清理，GE负责属性变化，概念见[Epic计时器说明](https://dev.epicgames.com/documentation/unreal-engine/gameplay-timers-in-unreal-engine?lang=en-US)与[Epic Gameplay Effects说明](https://dev.epicgames.com/documentation/unreal-engine/gameplay-effects-for-the-gameplay-ability-system-in-unreal-engine)。本页签名与暂停时World Timer不Tick的行为按本机5.5.4源码核对；官网默认版本可能更高。

## 2. 准备与文件

停止PIE → Save All → 关闭UE；CLion保存当前文件。在已有工程内手动创建目录 / 文件，不打开新小工作空间。**五新Source、七旧Source修改**；无新模块 / UE资产，Build.cs / Target / uproject不改。

| 操作 | 路径（根目录 `E:\UE_Project\Demo\EmberTrial\Source`） |
| --- | --- |
| 新建 | `EmberTrial/Public/Abilities/Effects/ETStaminaDeltaEffect.h` |
| 新建 | `EmberTrial/Private/Abilities/Effects/ETStaminaDeltaEffect.cpp` |
| 新建 | `EmberTrial/Public/Resources/ETStaminaRecoveryComponent.h` |
| 新建 | `EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp` |
| 新建 | `EmberTrial/Tests/ETStaminaRecoveryTests.cpp` |
| 修改两文件 | `EmberTrial/Public/Abilities/ETGameplayTags.h` / `Private/Abilities/ETGameplayTags.cpp` |
| 修改两文件 | `EmberTrial/Public/Characters/EmberTrialCharacter.h` / `Private/Characters/EmberTrialCharacter.cpp` |
| 修改两文件 | `EmberTrial/Public/Framework/ETPlayerController.h` / `Private/Framework/ETPlayerController.cpp` |
| 修改一文件 | `EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp` |

A的两个属性文件、StaminaFoundation测试、IsCombatDead与各原函数保留。本批不需要再调整Ready / 初始化函数，不删除旧方法。

## 3. 原生Tag与通用体力Effect

### 3.1 两个既有Tag文件各加一项

ETGameplayTags.h：在 **namespace ETGameplayTags的大括号内部**、原SetByCaller_HealthDelta声明后加：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_StaminaDelta);
```

ETGameplayTags.cpp：同一namespace内部、原HealthDelta定义之后加：

```cpp
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_StaminaDelta,
        "SetByCaller.StaminaDelta", "Signed stamina change carried by an Instant effect spec.");
```

只加这一项，原六个Tag保留，不另建namespace、不在.ini重复登记原生Tag。

### 3.2 新 ETStaminaDeltaEffect.h 全文

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETStaminaDeltaEffect.generated.h"

UCLASS()
class EMBERTRIAL_API UETStaminaDeltaEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETStaminaDeltaEffect(const FObjectInitializer& ObjectInitializer);
};
```

### 3.3 新 ETStaminaDeltaEffect.cpp 全文

```cpp
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"

UETStaminaDeltaEffect::UETStaminaDeltaEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = ETGameplayTags::SetByCaller_StaminaDelta;

    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UETStaminaAttributeSet::GetStaminaAttribute();
    Modifier.ModifierOp = EGameplayModOp::AddBase;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}
```

这是固定的资源数值规则，恢复速率 / 延迟仍可在组件Details配置；不为同一个加减规则创建多个重复资产。后续能力可以构造自己的Spec传不同增量，**是否允许消耗、何时提交与取消事务仍由能力决定**，这个通用GE本身不会替能力校验死亡 / 忙碌或把不足体力消费拒绝掉。

## 4. 恢复组件：两份全文

### 4.1 ETStaminaRecoveryComponent.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETStaminaRecoveryComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;
struct FGameplayTag;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETStaminaRecoveryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETStaminaRecoveryComponent();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;
    bool IsRecoveryScheduled() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETStaminaRecoveryTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
        meta = (ClampMin = "0.01", ClampMax = "1000.0"))
    float RecoveryRate = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
        meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
    float RecoveryDelay = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Stamina Recovery",
        meta = (ClampMin = "0.05", ClampMax = "0.5", Units = "s"))
    float RecoveryInterval = 0.10f;

    void RefreshObservation();
    void StopObserving();
    void SyncRecoveryTimer();
    void ClearRecoveryTimer(const TCHAR* Reason);
    bool ReadBody(AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC,
        float& Stamina, float& MaxStamina) const;
    void HandleRecoveryTimer(uint64 Serial);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Body, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
        TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);
    void HandleActionTagChanged(const FGameplayTag Tag, int32 NewCount,
        TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);

    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle StaminaHandle;
    FDelegateHandle MaxStaminaHandle;
    TArray<FDelegateHandle> ActionTagHandles;
    FTimerHandle RecoveryTimer;
    uint64 ObservationSerial = 0;
    uint64 DecreaseRevision = 0;
    double RecoveryNotBefore = 0.0;
    double LastSampleTime = 0.0;
    bool bRecovering = false;
    bool bEndingPlay = false;
};
```

### 4.2 ETStaminaRecoveryComponent.cpp

```cpp
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStaminaRecovery, Log, All);

UETStaminaRecoveryComponent::UETStaminaRecoveryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UETStaminaRecoveryComponent::BeginPlay()
{
    Super::BeginPlay();
    bEndingPlay = false;
    BodyOwner = Cast<AEmberTrialCharacter>(GetOwner());
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.AddUniqueDynamic(
            this, &ThisClass::HandleGameplayReadyChanged);
    }
    RefreshObservation();
}

void UETStaminaRecoveryComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    if (HasBegunPlay() && !bEndingPlay)
    {
        RefreshObservation();
    }
}

void UETStaminaRecoveryComponent::Deactivate()
{
    Super::Deactivate();
    StopObserving();
}

void UETStaminaRecoveryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(
            this, &ThisClass::HandleGameplayReadyChanged);
    }
    StopObserving();
    BodyOwner.Reset();
    Super::EndPlay(EndPlayReason);
}

bool UETStaminaRecoveryComponent::IsRecoveryScheduled() const
{
    const UWorld* World = GetWorld();
    return World && World->GetTimerManager().TimerExists(RecoveryTimer);
}

void UETStaminaRecoveryComponent::ClearRecoveryTimer(const TCHAR* Reason)
{
    const bool bHadTimer = IsRecoveryScheduled();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RecoveryTimer);
    }
    RecoveryTimer.Invalidate();
    if (bHadTimer || bRecovering)
    {
        UE_LOG(LogETStaminaRecovery, Display,
            TEXT("[M2-01B1] Timer stopped Owner=%s Reason=%s"),
            *GetNameSafe(GetOwner()), Reason);
    }
    bRecovering = false;
}

void UETStaminaRecoveryComponent::StopObserving()
{
    ++ObservationSerial;
    ClearRecoveryTimer(TEXT("Unsubscribe"));
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(
            UETStaminaAttributeSet::GetStaminaAttribute()).Remove(StaminaHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(
            UETStaminaAttributeSet::GetMaxStaminaAttribute()).Remove(MaxStaminaHandle);
        const FGameplayTag Tags[] = {ETGameplayTags::State_Attacking,
            ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
        for (int32 Index = 0; Index < ActionTagHandles.Num(); ++Index)
        {
            ASC->RegisterGameplayTagEvent(Tags[Index], EGameplayTagEventType::NewOrRemoved)
                .Remove(ActionTagHandles[Index]);
        }
        UE_LOG(LogETStaminaRecovery, Display,
            TEXT("[M2-01B1] Observe stopped Owner=%s"), *GetNameSafe(GetOwner()));
    }
    StaminaHandle.Reset();
    MaxStaminaHandle.Reset();
    ActionTagHandles.Reset();
    BoundASC.Reset();
}

bool UETStaminaRecoveryComponent::ReadBody(AEmberTrialCharacter*& Body,
    UAbilitySystemComponent*& ASC, float& Stamina, float& MaxStamina) const
{
    Body = BodyOwner.Get();
    ASC = BoundASC.Get();
    return !bEndingPlay && IsActive() && IsValid(Body) && !Body->IsActorBeingDestroyed() &&
        Body->HasAuthority() && Body->IsGameplayReady() && IsValid(ASC) &&
        Body->GetAbilitySystemComponent() == ASC && ASC->AbilityActorInfo.IsValid() &&
        ASC->AbilityActorInfo->OwnerActor.Get() == Body &&
        ASC->AbilityActorInfo->AvatarActor.Get() == Body &&
        Body->GetGameplayStaminaSnapshot(Stamina, MaxStamina) &&
        FMath::IsFinite(Stamina) && FMath::IsFinite(MaxStamina) &&
        MaxStamina >= 1.0f && Stamina >= 0.0f && Stamina <= MaxStamina;
}

void UETStaminaRecoveryComponent::RefreshObservation()
{
    StopObserving();
    AEmberTrialCharacter* Body = BodyOwner.Get();
    if (bEndingPlay || !IsActive() || !IsValid(Body) ||
        !Body->HasAuthority() || !Body->IsGameplayReady())
    {
        return;
    }
    TArray<UETStaminaRecoveryComponent*> Components;
    Body->GetComponents(Components);
    if (Components.Num() != 1 || !FMath::IsFinite(RecoveryRate) ||
        RecoveryRate <= 0.0f || RecoveryRate > 1000.0f ||
        !FMath::IsFinite(RecoveryDelay) || RecoveryDelay < 0.0f || RecoveryDelay > 10.0f ||
        !FMath::IsFinite(RecoveryInterval) || RecoveryInterval < 0.05f || RecoveryInterval > 0.5f)
    {
        UE_LOG(LogETStaminaRecovery, Warning,
            TEXT("[M2-01B1] Invalid recovery settings Owner=%s Count=%d Rate=%.2f Delay=%.2f Interval=%.2f"),
            *GetNameSafe(Body), Components.Num(), RecoveryRate, RecoveryDelay, RecoveryInterval);
        return;
    }
    BoundASC = Body->GetAbilitySystemComponent();
    AEmberTrialCharacter* CheckedBody = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    if (!ReadBody(CheckedBody, ASC, Stamina, MaxStamina))
    {
        BoundASC.Reset();
        return;
    }
    StaminaHandle = ASC->GetGameplayAttributeValueChangeDelegate(
        UETStaminaAttributeSet::GetStaminaAttribute()).AddUObject(
            this, &ThisClass::HandleAttributeChanged, BoundASC, ObservationSerial);
    MaxStaminaHandle = ASC->GetGameplayAttributeValueChangeDelegate(
        UETStaminaAttributeSet::GetMaxStaminaAttribute()).AddUObject(
            this, &ThisClass::HandleAttributeChanged, BoundASC, ObservationSerial);
    const FGameplayTag Tags[] = {ETGameplayTags::State_Attacking,
        ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
    for (const FGameplayTag Tag : Tags)
    {
        ActionTagHandles.Add(ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
            .AddUObject(this, &ThisClass::HandleActionTagChanged, BoundASC, ObservationSerial));
    }
    LastSampleTime = GetWorld()->GetTimeSeconds();
    RecoveryNotBefore = LastSampleTime + RecoveryDelay;
    UE_LOG(LogETStaminaRecovery, Display,
        TEXT("[M2-01B1] Observe ready Owner=%s Rate=%.1f Delay=%.2f Interval=%.2f Stamina=%.1f/%.1f"),
        *GetNameSafe(Body), RecoveryRate, RecoveryDelay, RecoveryInterval, Stamina, MaxStamina);
    SyncRecoveryTimer();
}

void UETStaminaRecoveryComponent::HandleGameplayReadyChanged(
    AEmberTrialCharacter* Body, bool bReady)
{
    if (bEndingPlay || Body != BodyOwner.Get()) { return; }
    if (bReady) { RefreshObservation(); }
    else { StopObserving(); }
}

void UETStaminaRecoveryComponent::HandleAttributeChanged(const FOnAttributeChangeData& Change,
    TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial)
{
    if (bEndingPlay || Serial != ObservationSerial ||
        !SourceASC.IsValid() || SourceASC.Get() != BoundASC.Get()) { return; }
    if (Change.Attribute == UETStaminaAttributeSet::GetStaminaAttribute() &&
        Change.NewValue < Change.OldValue)
    {
        ++DecreaseRevision;
        LastSampleTime = GetWorld()->GetTimeSeconds();
        RecoveryNotBefore = LastSampleTime + RecoveryDelay;
        bRecovering = false;
        UE_LOG(LogETStaminaRecovery, Display,
            TEXT("[M2-01B1] Delayed Owner=%s Stamina=%.1f NotBefore=%.3f"),
            *GetNameSafe(GetOwner()), Change.NewValue, RecoveryNotBefore);
    }
    SyncRecoveryTimer();
}

void UETStaminaRecoveryComponent::HandleActionTagChanged(const FGameplayTag Tag,
    int32 NewCount, TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial)
{
    (void)Tag;
    (void)NewCount;
    if (bEndingPlay || Serial != ObservationSerial ||
        !SourceASC.IsValid() || SourceASC.Get() != BoundASC.Get()) { return; }
    LastSampleTime = GetWorld()->GetTimeSeconds();
    SyncRecoveryTimer();
}

void UETStaminaRecoveryComponent::SyncRecoveryTimer()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    if (!ReadBody(Body, ASC, Stamina, MaxStamina))
    {
        ClearRecoveryTimer(TEXT("Unavailable"));
        return;
    }
    if (Body->IsCombatDead() || ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
    {
        ClearRecoveryTimer(TEXT("Dead"));
        return;
    }
    if (!Body->IsFormalActionIdle())
    {
        ClearRecoveryTimer(TEXT("Busy"));
        return;
    }
    if (Stamina >= MaxStamina)
    {
        ClearRecoveryTimer(TEXT("Full"));
        return;
    }
    if (!IsRecoveryScheduled())
    {
        LastSampleTime = GetWorld()->GetTimeSeconds();
        GetWorld()->GetTimerManager().SetTimer(RecoveryTimer,
            FTimerDelegate::CreateUObject(this, &ThisClass::HandleRecoveryTimer, ObservationSerial),
            RecoveryInterval, true, RecoveryInterval);
    }
}

void UETStaminaRecoveryComponent::HandleRecoveryTimer(uint64 Serial)
{
    if (Serial != ObservationSerial || bEndingPlay) { return; }
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    if (!ReadBody(Body, ASC, Before, MaxStamina) || Body->IsCombatDead() ||
        !Body->IsFormalActionIdle() || Before >= MaxStamina)
    {
        SyncRecoveryTimer();
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    const double EligibleTime = FMath::Max(0.0, Now - FMath::Max(LastSampleTime, RecoveryNotBefore));
    const float Amount = FMath::Min(MaxStamina - Before,
        static_cast<float>(EligibleTime * RecoveryRate));
    // Keep accumulating time until the float resource can represent the increment.
    if (Amount <= 0.0f || Before + Amount <= Before) { return; }
    LastSampleTime = Now;
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
        UETStaminaDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
    if (!Spec.IsValid())
    {
        ClearRecoveryTimer(TEXT("MissingSpec"));
        return;
    }
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_StaminaDelta, Amount);
    if (!bRecovering)
    {
        bRecovering = true;
        UE_LOG(LogETStaminaRecovery, Display,
            TEXT("[M2-01B1] Recovery begin Owner=%s Time=%.3f Stamina=%.1f/%.1f"),
            *GetNameSafe(Body), Now, Before, MaxStamina);
    }
    const uint64 Revision = DecreaseRevision;
    const TWeakObjectPtr<UAbilitySystemComponent> SourceASC = ASC;
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    // Applying a GE can synchronously trigger callbacks, shutdown, or a new spend.
    if (bEndingPlay || Serial != ObservationSerial || Revision != DecreaseRevision ||
        !SourceASC.IsValid() || SourceASC.Get() != BoundASC.Get()) { return; }
    float After = 0.0f;
    if (!ReadBody(Body, ASC, After, MaxStamina)) { return; }
    if (After <= Before)
    {
        ClearRecoveryTimer(TEXT("NoProgress"));
        return;
    }
    SyncRecoveryTimer();
}
```

LastSampleTime只用于积分，不是第二份体力；两种序号分别拒绝旧观察回调、拒绝Effect应用期间新消耗导致的旧收尾。局部Tags数组在函数内，不增加Unity合编匿名常量重名。恢复只在缺额且正式空闲时计时；Tag事件处理忙碌边界，World暂停保持世界时间不前进。

延迟边界可能只有极小浮点增量，此时不提交GE也不提前消耗积分时间；等可表示的增量再处理，避免把“尚不足以改变float值”误记为NoProgress停表。

## 5. 七个旧文件的准确接入

### 5.1 Character.h：前置声明与私有字段

前置声明区，在已有 `class UETStaminaAttributeSet;` 后加：

```cpp
class UETStaminaRecoveryComponent;
```

私有区，现有 `TObjectPtr<UETStaminaAttributeSet> StaminaAttributes;` 后加：

```cpp
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Stamina",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETStaminaRecoveryComponent> StaminaRecoveryComponent;
```

### 5.2 Character.cpp：一include / 一构造行

在已有体力属性include后加：

```cpp
#include "Resources/ETStaminaRecoveryComponent.h"
```

构造函数中，原 `CombatLifeComponent = CreateDefaultSubobject...` **完整语句后**加：

```cpp
    StaminaRecoveryComponent = CreateDefaultSubobject<UETStaminaRecoveryComponent>(TEXT("StaminaRecovery"));
```

其余所有Character函数保持原样。本批不替换IsGameplayReady / IsCombatDead / EnsureGameplayInitialized，不改原型Practice组件。

### 5.3 PlayerController.h：public增加一个开发命令

在现有 `void OnGameMenuStateChanged();` 后、`protected:` 前添加：

```cpp
    UFUNCTION(Exec, meta = (DevelopmentOnly))
    void ET_Stamina(float Delta);
```

### 5.4 PlayerController.cpp：include与新增完整函数

在现有include区追加以下六条，原include与LogETPlayerController定义保留：

```cpp
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Characters/EmberTrialCharacter.h"
#include "Resources/ETStaminaRecoveryComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
```

在现有EndPlay函数的结束 `}` **之后**添加，不放进EndPlay内：

```cpp
void AETPlayerController::ET_Stamina(float Delta)
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(GetPawn());
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    if (!World || World->WorldType != EWorldType::PIE || !IsLocalController() ||
        !IsValid(Body) || !Body->HasAuthority() || !FMath::IsFinite(Delta) ||
        FMath::Abs(Delta) > 100.0f || !Body->GetGameplayStaminaSnapshot(Before, MaxStamina))
    {
        UE_LOG(LogETPlayerController, Display,
            TEXT("[M2-01B1] Debug rejected Controller=%s Reason=ContextOrValue"), *GetNameSafe(this));
        return;
    }
    if (Delta != 0.0f && (World->IsPaused() || Body->IsCombatDead()))
    {
        UE_LOG(LogETPlayerController, Display,
            TEXT("[M2-01B1] Debug rejected Controller=%s Reason=PausedOrDead"), *GetNameSafe(this));
        return;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (Delta != 0.0f)
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
            UETStaminaDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
        if (!Spec.IsValid()) { return; }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_StaminaDelta, Delta);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
    float After = 0.0f;
    if (!Body->GetGameplayStaminaSnapshot(After, MaxStamina)) { return; }
    const UETStaminaRecoveryComponent* Recovery = Body->FindComponentByClass<UETStaminaRecoveryComponent>();
    UE_LOG(LogETPlayerController, Display,
        TEXT("[M2-01B1] Debug Owner=%s Delta=%.1f Before=%.1f After=%.1f Max=%.1f Time=%.3f Timer=%d Paused=%d"),
        *GetNameSafe(Body), Delta, Before, After, MaxStamina, World->GetTimeSeconds(),
        IsValid(Recovery) && Recovery->IsRecoveryScheduled() ? 1 : 0, World->IsPaused() ? 1 : 0);
#else
    (void)Delta;
#endif
}
```

Delta=0只读，可在暂停 / 已死亡时读当前身体；非零只允许PIE、有限[-100,100]、Ready且存活、世界未暂停。Game构建中的函数体没有修改资源的功能，不依赖DevelopmentOnly标签充当运行保护。这里是**开发注入**，不是正式闪避成本接口；不足时会夹到0，不能用它实现“资源不足拒绝能力”。

### 5.5 Editor校验器：两处精确添加

在既有 `ValidateBody` 的必需引用列表中，将以下旧两行：

```cpp
    for (const TCHAR* Name : {TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"),
        TEXT("StaminaAttributes"), TEXT("CombatTraceComponent"), TEXT("CombatLifeComponent")})
```

仅替换为：

```cpp
    for (const TCHAR* Name : {TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"),
        TEXT("StaminaAttributes"), TEXT("StaminaRecoveryComponent"),
        TEXT("CombatTraceComponent"), TEXT("CombatLifeComponent")})
```

仍在ValidateBody函数内，找到其末尾已有 `if (IsValid(Life)) {...}`，在该if结束后、**ValidateBody的最后一个 `}` 前**添加：

```cpp
    const FObjectPropertyBase* RecoveryProperty = FindFProperty<FObjectPropertyBase>(
        Defaults->GetClass(), TEXT("StaminaRecoveryComponent"));
    UObject* Recovery = RecoveryProperty ? RecoveryProperty->GetObjectPropertyValue_InContainer(Defaults) : nullptr;
    if (IsValid(Recovery))
    {
        const TOptional<double> Rate = ReadNumber(Blueprint, Recovery, TEXT("RecoveryRate"), Issues);
        const TOptional<double> Delay = ReadNumber(Blueprint, Recovery, TEXT("RecoveryDelay"), Issues);
        const TOptional<double> Interval = ReadNumber(Blueprint, Recovery, TEXT("RecoveryInterval"), Issues);
        const FBoolProperty* AutoActivate = FindFProperty<FBoolProperty>(
            Recovery->GetClass(), TEXT("bAutoActivate"));
        if (Rate.IsSet() && Delay.IsSet() && Interval.IsSet() &&
            (!FMath::IsFinite(Rate.GetValue()) || Rate.GetValue() <= 0.0 || Rate.GetValue() > 1000.0 ||
             !FMath::IsFinite(Delay.GetValue()) || Delay.GetValue() < 0.0 || Delay.GetValue() > 10.0 ||
             !FMath::IsFinite(Interval.GetValue()) || Interval.GetValue() < 0.05 || Interval.GetValue() > 0.5 ||
             !AutoActivate || !AutoActivate->GetPropertyValue_InContainer(Recovery)))
        {
            AddIssue(Blueprint, TEXT("ET-STAM-001"),
                TEXT("Recovery requires AutoActivate, finite 0<Rate<=1000, 0<=Delay<=10 and 0.05<=Interval<=0.5."), Issues);
        }
    }
```

已有反射 / ReadNumber帮助函数与UObject/UnrealType include足够，不加新Editor依赖。必需引用列表已经检查None；末尾段用反射取得同一对象，不重复报告缺失引用。两处修改都在ValidateBody内，其余全部规则保留。

## 6. 新测试 ETStaminaRecoveryTests.cpp：全文

长期保留在Tests，使用自己的临时世界 / 原生身体 / 同一个通用GE，不保存资源。自动检查延迟与再次降低重置、暂停 / 忙碌无整段补量、上限增加 / 满值停表、重复观察去重、旧回调拒绝、Deactivate / Activate、非法速率、真实生命归零死亡与EndPlay清理。时序断言留一个采样周期余量，避免依赖某一帧恰好回调。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Resources/ETStaminaRecoveryComponent.h"
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETStaminaRecoveryTest,
    "EmberTrial.M2.Resources.StaminaRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETStaminaRecoveryTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>();
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Temporary body"), Body) || !TestNotNull(TEXT("Pauser"), Pauser)) { return false; }
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    Body->GetCharacterMovement()->SetComponentTickEnabled(false);
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UETStaminaRecoveryComponent* Recovery = Body->FindComponentByClass<UETStaminaRecoveryComponent>();
    if (!TestNotNull(TEXT("ASC"), ASC) || !TestNotNull(TEXT("Recovery"), Recovery)) { return false; }
    auto Advance = [&](int32 Steps)
    {
        for (int32 Index = 0; Index < Steps; ++Index)
        {
            if (!TestWorld.TickTestWorld(0.05f))
            {
                TestWorld.ForwardErrorMessages(this);
                return false;
            }
        }
        return true;
    };
    const FGameplayAttribute Attribute = UETStaminaAttributeSet::GetStaminaAttribute();
    auto Value = [&]() { return ASC->GetNumericAttribute(Attribute); };
    auto ApplyDelta = [&](float Delta)
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
            UETStaminaDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
        if (!Spec.IsValid()) { AddError(TEXT("Stamina delta spec unavailable")); return; }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_StaminaDelta, Delta);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    };
    auto Same = [&](const TCHAR* Label, float Expected)
    {
        TestTrue(Label, FMath::IsNearlyEqual(Value(), Expected, 0.001f));
    };
    TestFalse(TEXT("Full has no timer"), Recovery->IsRecoveryScheduled());
    TestFalse(TEXT("No component Tick capability"), Recovery->PrimaryComponentTick.bCanEverTick);
    TestFalse(TEXT("No registered component Tick"), Recovery->PrimaryComponentTick.IsTickFunctionRegistered());
    ApplyDelta(-25.0f);
    Same(TEXT("Real GE consumes 25"), 75.0f);
    TestTrue(TEXT("Deficit schedules timer"), Recovery->IsRecoveryScheduled());
    if (!Advance(14)) { return false; } // 0.70 seconds, before delay.
    Same(TEXT("Delay prevents early recovery"), 75.0f);
    if (!Advance(4)) { return false; } // 0.90 seconds total, with sampling margin.
    TestTrue(TEXT("Recovery starts after delay"), Value() > 75.0f && Value() <= 79.0f);

    const float BeforePause = Value();
    const double TimeBeforePause = World->GetTimeSeconds();
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    const bool bPausedTicksOK = Advance(20);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr); // Always restore before exit.
    if (!bPausedTicksOK) { return false; }
    Same(TEXT("Pause freezes resource"), BeforePause);
    TestTrue(TEXT("Pause freezes world clock"), FMath::IsNearlyEqual(
        World->GetTimeSeconds(), TimeBeforePause, 0.001));
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("Resume recovers without paused catch-up"),
        Value() > BeforePause && Value() <= BeforePause + 6.2f);

    ApplyDelta(-10.0f);
    if (!Advance(6)) { return false; }
    ApplyDelta(-5.0f);
    const float AfterSecondSpend = Value();
    if (!Advance(14)) { return false; }
    Same(TEXT("Second spend resets delay"), AfterSecondSpend);
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("Recovery resumes after reset delay"), Value() > AfterSecondSpend);
    ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestFalse(TEXT("Busy tag stops timer immediately"), Recovery->IsRecoveryScheduled());
    const float BusyValue = Value();
    if (!Advance(20)) { return false; }
    Same(TEXT("Busy freezes recovery"), BusyValue);
    ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("Busy end does not pay busy duration"), Value() > BusyValue && Value() <= BusyValue + 6.2f);
    ApplyDelta(1000.0f);
    Same(TEXT("Recovery / restore caps at maximum"), 100.0f);
    TestFalse(TEXT("Full stops timer"), Recovery->IsRecoveryScheduled());

    ASC->SetNumericAttributeBase(UETStaminaAttributeSet::GetMaxStaminaAttribute(), 120.0f);
    Same(TEXT("Maximum increase itself does not refill"), 100.0f);
    TestTrue(TEXT("New deficit wakes recovery"), Recovery->IsRecoveryScheduled());
    if (!Advance(4)) { return false; }
    TestTrue(TEXT("New capacity can recover later"), Value() > 100.0f && Value() <= 106.2f);
    ApplyDelta(1000.0f);
    Recovery->RefreshObservation();
    Recovery->RefreshObservation();
    const uint64 RevisionBeforeSpend = Recovery->DecreaseRevision;
    ApplyDelta(-5.0f);
    TestEqual(TEXT("Reobserve does not duplicate spend callback"),
        Recovery->DecreaseRevision, RevisionBeforeSpend + 1);
    const uint64 OldSerial = Recovery->ObservationSerial;
    Recovery->Deactivate();
    TestFalse(TEXT("Deactivate clears timer"), Recovery->IsRecoveryScheduled());
    ApplyDelta(-5.0f);
    const float DisabledValue = Value();
    if (!Advance(20)) { return false; }
    Same(TEXT("Inactive component does not recover"), DisabledValue);
    Recovery->Activate(true);
    const double NewDeadline = Recovery->RecoveryNotBefore;
    FOnAttributeChangeData StaleChange;
    StaleChange.Attribute = Attribute;
    StaleChange.OldValue = 100.0f;
    StaleChange.NewValue = 50.0f;
    Recovery->HandleAttributeChanged(StaleChange, TWeakObjectPtr<UAbilitySystemComponent>(ASC), OldSerial);
    TestEqual(TEXT("Old observation cannot reset new deadline"), Recovery->RecoveryNotBefore, NewDeadline);

    Recovery->RecoveryRate = 0.0f;
    AddExpectedMessagePlain(TEXT("Invalid recovery settings"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 1);
    Recovery->RefreshObservation();
    TestFalse(TEXT("Invalid rate leaves no timer"), Recovery->IsRecoveryScheduled());
    Recovery->RecoveryRate = 20.0f;
    Recovery->RefreshObservation();
    if (!Advance(18)) { return false; }
    TestTrue(TEXT("Valid settings recover again"), Value() > DisabledValue);
    TestEqual(TEXT("Stamina effect leaves Health alone"),
        ASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);

    ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestTrue(TEXT("Actual zero Health commits death"), Body->IsCombatDead());
    TestFalse(TEXT("Death stops timer immediately"), Recovery->IsRecoveryScheduled());
    const float AtDeath = Value();
    if (!Advance(20)) { return false; }
    Same(TEXT("Dead body does not recover"), AtDeath);
    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay leaves no timer"), Recovery->IsRecoveryScheduled());
    TestFalse(TEXT("EndPlay revokes Ready"), Body->IsGameplayReady());
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

忙碌用一个真实ASC Loose Tag模拟状态门控，不能据此说已测试了尚未实现的GA_Dodge；死亡通过现有真实Health / Life提交。暂停前后采用WorldSettings Pauser，与已有位移测试同一临时世界方法；不是用真实睡眠模拟暂停。新测试不替代旧A初始化测试，下一节两项一起跑一次。

无Tick的断言同时检查 `bCanEverTick=False` 与未注册Tick。UE 5.5.4的 `IsComponentTickEnabled()` 只查询内部启用标记，不能单独证明Tick已注册或会执行；这里避免把未注册的默认标记误判成组件开了Tick。

## 7. 正常Build / 索引与三个正式身体

保存全部Source，UE仍关闭，PowerShell：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor成功并刷新索引后，保持UE关闭，再执行：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

先Editor成功与索引刷新，再Game；前一条报错先处理，不继续误记后一条成功。CLion Reload CMake Project后，新Effect / Recovery / Test属于EmberTrialCodeModel；Editor校验器属于EmberTrialEditorCodeModel。generated.h最后include，文件 / 类名一致，不用CMake CodeModel编译游戏。

打开UE，分别打开原玩家 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`、敌人 `/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy`、训练目标 `/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget`：

1. Components应有**单个继承的StaminaRecovery**。这是原生构造创建的，不手动Add第二份。原ASC / Trace / Life / Practice装配保留。
2. 选中继承StaminaRecovery，Details搜索 `Recovery`，确认Rate20 / Delay0.75 / Interval0.10；清空搜索后搜 `Auto Activate`，保持勾选。若编辑属性，编辑的是该蓝图默认组件，不是PIE中的实例。
3. Class Defaults的生命与体力初始值仍100 / 100；Compile → Save。Content Browser缩略图右键 → Asset Actions → Validate Assets，三份均预期M1-07C2 Valid / Issues0。

新规则ET-STAM-001检查恢复参数 / AutoActivate，ET-BODY-001保护引用。无新UE资产，不需要新建GE蓝图；Body恢复配置非法时不启动观察 / 计时器，原属性Ready不因此变成等待。正式资产保持合法；本批不再搭Editor负例Probe，不能宣称该静态错误分支全部实测。

## 8. 分层检查（合计5–15分钟）

### 8.1 两项资源自动测试（约1–3分钟）

不Play，在Output Log底部命令框输入：

```text
Automation RunTests EmberTrial.M2.Resources
```

预期 **StaminaFoundation / StaminaRecovery两项各一次Success**。Foundation确保新组件没有破坏A数值 / 初始化 / 换控契约；Recovery测试临时世界的时序和清理。预期非法配置消息由各测试登记，正式身体不应报Invalid recovery settings。等待两个Test Completed后再Play，不在测试运行中改源码 / 蓝图。

若Failure，发完整路径和对应Test Started至Completed段或Saved日志路径；我读取本机定位。不要删断言或吞掉所有Warning。成功后本批不额外运行M0完整组，因为Practice源码 / 输入未变，B2不重复设计这些自动化。

### 8.2 正式世界恢复与Pause（约2–5分钟）

1. 打开正式纵向地图正常Play。出生初始化100 / 100、Ready后新增 `[M2-01B1] Observe ready`，玩家 / 敌人 / 训练目标各一份，Rate20 / Delay0.75 / Interval0.10。满体力没有Timer，血条仍只显示生命。
2. **PIE保持运行**，打开Output Log命令框，先输入：

   ```text
   ke BP_ETPlayerController_C ET_Stamina 0
   ```

   预期当前玩家Debug Delta0 / Before100 / After100 / Max100 / Timer0。采用已有 `ke` 路线，避免命令被Editor世界或没有焦点的游戏控制台接收；若没有任何项目日志，先确认PIE仍运行且地图使用BP_ETPlayerController。
3. 输入：

   ```text
   ke BP_ETPlayerController_C ET_Stamina -25
   ```

   预期真实75 / 100、Delayed NotBefore、Timer1；约0.75世界秒后Recovery begin，约再1.25秒恢复100，Timer stopped Reason=Full。再用步骤2读数应100 / 100、Timer0。**你不用精确掐秒**，我读日志的Time / NotBefore核对；定量无提前和积分规则由自动测试覆盖。
4. 再注入-25，切回游戏视口按P暂停。暂停期间步骤2只读命令可用，Paused1，前后两次查询间隔约2秒应体力不变；尝试-25应Debug rejected Reason=PausedOrDead。用一种已有方式继续，之后恢复并满值停止。这里检查世界暂停与入口拒绝，不要求鼠标 / Enter / Space全重测。

若切视口耗时使体力已满再暂停，不算代码失败；只重做这一步，注入 **-100** 后暂停，留足约5秒恢复窗口。不要改Recovery Rate来制造验收条件；时序由自动测试兜底。

### 8.3 Stop与一条短基线（约2–4分钟）

在未满体力时Stop，再Play：新身体100 / 100、Timer0、HUD生命100 / 100；旧Observe stopped、原Ready=0 / ActorInfo与UI清理，无Stop后旧Recovery begin。无需精确赶0.75秒，注入-100后Stop即可。

这一轮只做左键一次真实25、E中文 / P一种继续和移动转向跳跃短基线；按2仍是原型且不消费体力，恢复与其没有正式Tag联动符合第1节。保存资产与地图，原设置保持100/100、25/10、slomo1、Debug False。

本批真实死亡计时器清理由自动测试的Health归零覆盖，不再手动按4四次重演整个失败页；B2体力显示 / M2正式消耗接入后再根据变更复用相关真实链路。旧Attempt02没有本批实现，不拿它做检查；不要求新Cook或Trace。

## 9. 最简反馈

```text
M2-01B1 完成 / 卡在第__节第__步
五新 / 七旧Source；关闭UE后Editor Build / 索引 / Game Build：
三正式身体单继承Recovery、20 / 0.75 / 0.10 / AutoActivate、Compile / Save / Valid：
Resources两项一次：Foundation / Recovery Success
满值Timer0；开发-25真实75、延迟 / 恢复满值停止：
Pause前后查询不变；非零拒绝；恢复无暂停整段补量：
未满Stop / 新一轮100 Timer0、旧观察 / Timer清理：
左键25 / E / P / 原2与移动转向跳跃短基线：
配置恢复、Source / 资产Save All；失败或特别说明（无则填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

Build发第一条真实UHT / C++ / linker错误，日志优先；组件Details找不到或重复时发截图 / 资产路径。时序只给操作顺序和结果，不要求你做精确计时记录，我读取现存日志与测试结果。

## 10. 本批 Commit / Push（固定放在最后）

- 五新Source：Effect两文件、Recovery两文件、Tests/ETStaminaRecoveryTests.cpp，按第2节路径；七旧Source：Tag两文件、Character两文件、PlayerController两文件、既有Editor Validator。
- 三正式身体若Compile / Save产生实际.uasset差异才纳入，走既有LFS；不创建重复Recovery组件或GE资产，48项来源清单仍按实际哈希 / 用途更新。
- 本指南、M2路线 / Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow / Introduction实际更新；实施完成后的报告与清单再追加。A验收收尾若未提交先纳入独立文档提交，已提交的实现不重复处理。
- Build.cs / Target / uproject / 共享Config / UI Source本批没有计划改动。生成CMake / IDE文件、Binaries / Intermediate / Saved、Raw / 测试日志、旧包 / Trace留本地，不Force Add。

CLion Save All → 刷新Changes → 最新完整实现与合法配置重新Stage → 检查Staged Diff → Commit → Push。检查通过后可用 `feat: add delayed stamina recovery and lifecycle guards`；目前仅下发指南时用 `docs: prepare stamina recovery`。用户操作Git；B1完成才进入B2体力Presenter / HUD，不提前写M2-01B整章完成。
