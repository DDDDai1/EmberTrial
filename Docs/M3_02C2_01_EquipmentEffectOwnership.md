# M3-02C2-01：身体装备GE的句柄所有权

2026-10-29复核，固定UE **5.5.4**。本批已手动实施并[验收通过](Reports/M3_02C2_01_Verification.md)，三相关Success、双Build与玩家GE清理已核对，无待补。实际代码配置40 / 检查2 / 等待5分钟；当前进入[C2-02真实提交与新身体恢复](M3_02C2_02_EquipmentTransactions.md)。以下保留本批历史实现供复查，不用重新覆盖后续修改或重跑验收。

## 1. 本批效果与范围

身体侧新增原生`UETRelicEquipmentComponent`，只负责自有装备GE的创建、读回、更新与清理；Character构造一份继承组件。Player身体Ready后创建一个原生Infinite零幅度GE，Neutral / Enemy虽继承组件但不创建效果。空槽保留句柄、值为0；换加成更新同一句柄，不先Remove旧GE再Apply新GE。

- 三新Source：组件h / cpp、新长期EquipmentOwnership自动化。
- 两旧Source：Character.h / cpp，只前置声明 / 成员 / 测试friend、include与默认子对象创建。原Ready / 能力 / 输入 / EndPlay保持。
- 复用B原生双SetByCaller GE与现有属性，不新增GE蓝图、IA、UI、Config或临时验收图。
- 正向20 / 5等效果只由自动化临时写入；正式Play仍空槽 / 原100 / 25，本批尚无真实换装按钮或命令。

已验收实际**175 Source / 97cpp、Core71 / UI23 / Editor3、28测试**。Content仍296 / 来源77；旧身体如合法重存，只记录实际字节差异，不新增资产。

## 2. 为什么先做句柄，再接库存事务

1. **一份自有句柄。**用EffectContext的SourceObject标出组件，再用保存的精确Handle查找 / 更新。另一系统应用同一个GE类仍是别人的效果，不能按类名Remove全部。
2. **更新避免临时上限回落。**例：旧加成20，上限120、生命110；新加成40应直接到140 / 110。先移除旧GE会暂时回100并夹掉10生命，再应用新GE也补不回来。
3. **提高不治疗、降低只夹最终越界值。**现有HealthAttributeSet负责真实夹限，CombatLife已有伤害来源分类；不靠“把生命恢复到旧数值”掩盖夹限或真实伤害，也不把上限夹限当命中受击。
4. **有限范围的批处理。**5.5.4的`FScopedAggregatorOnDirtyBatch`把两个聚合器的脏通知延迟到当前作用域结束，当前写入重入得到Busy。它不等于锁住任意回调 / World，更不等于库存与GAS数据库事务。不要跨帧保存它，或在作用域中主动销毁ASC / 聚合器。
5. **基础适配层与业务策略分开。**本批私有TrySetModifiers只接受已建模的加法管线；完整槽位 / 最终基伤、世界 / 暂停 / 祭坛资格、库存版本和提交点留C2-02。Applied只表示本次自有GE更新已发出，不表示库存ID提交成功；同步观察者仍可能使身体结束。
6. **生命周期。**基础Ready撤销、组件失活 / 结束都先清本地Handle再精确移除。显式重新激活只建空槽，不恢复旧加成或治疗；C2-02会从库存重建。外部移除 / 篡改GE后查询不可用，不静默补建而掩盖所有权错误。

本批只创建玩家一个无周期 / 无Tick / 无Timer的活动GE；代价是一个常驻效果条目，收益是后续换装不反复重建 / 临时掉上限。敌人 / 训练目标不增加活动效果。`GetAppliedModifiers`仅原生只读，不是未来EquipmentReady / 祭坛可操作状态。

## 3. 创建三个文件

保存资产、关闭UE，CLion保持打开整个工程。新建（已有Inventory目录复用）：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Inventory\ETRelicEquipmentComponent.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Inventory\ETRelicEquipmentComponent.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicEquipmentOwnershipTests.cpp
```

文件名、Public / Private / Tests位置严格按上面填写；不在UE创建第二个Actor或蓝图组件。generated.h最后include，不手建生成文件；Build.cs / Target / uproject保持。

## 4. 三份完整新文件代码

### 4.1 ETRelicEquipmentComponent.h

第3节Public路径，完整写入：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Inventory/ETRelicLoadout.h"
#include "ETRelicEquipmentComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;

enum class EETRelicEffectWriteResult : uint8
{
    Applied,
    Unchanged,
    Unavailable,
    Busy,
    InvalidModifiers
};

// 身体侧自有GE适配层；C2-02再接库存事务 / 世界恢复，不允许Widget直接写效果。
UCLASS(ClassGroup = (EmberTrial))
class EMBERTRIAL_API UETRelicEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETRelicEquipmentComponent();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

    // 只表示本身体的GE槽完整，不代表库存已恢复或当前允许祭坛换装。
    bool GetAppliedModifiers(FETRelicModifierTotals& OutModifiers) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETRelicEquipmentOwnershipTest;

    bool ReadBody(AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const;
    bool HasIntactOwnedEffect(UAbilitySystemComponent* ASC) const;
    bool EnsureOwnedEffect();
    void ReleaseOwnedEffect();

    // 仅效果写入原语，不提交库存ID；C2-02必须在领域事务内调用。
    EETRelicEffectWriteResult TrySetModifiers(const FETRelicModifierTotals& Modifiers);
    bool AreModifiersValid(UAbilitySystemComponent* ASC,
                            const FETRelicModifierTotals& Modifiers) const;

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);

    // 弱身体 / ASC与精确句柄；不保存GI库存、旧World或Widget。
    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> EffectASC;
    FActiveGameplayEffectHandle OwnedEffect;
    FETRelicModifierTotals AppliedModifiers;
    bool bChanging = false;
    bool bEndingPlay = false;
};
```

### 4.2 ETRelicEquipmentComponent.cpp

第3节Private路径，完整写入。ReadBody只接受当前玩家 / Ready / 唯一组件与自己ASC；两件加成始终同时提供，空槽也显式为0：

```cpp
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameplayEffect.h"
#include "GameplayEffectAggregator.h"
#include "Templates/UnrealTemplate.h"
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogETRelicEquipment, Log, All);

// ---- 生命周期：订阅后立即读取，既覆盖晚绑定，也不重复初始化同一身体 ----
UETRelicEquipmentComponent::UETRelicEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
}

void UETRelicEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();
    BodyOwner = Cast<AEmberTrialCharacter>(GetOwner());
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleGameplayReadyChanged);
    }
    EnsureOwnedEffect();
}

void UETRelicEquipmentComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    if (HasBegunPlay() && !bEndingPlay)
    {
        EnsureOwnedEffect();
    }
}

void UETRelicEquipmentComponent::Deactivate()
{
    Super::Deactivate();
    ReleaseOwnedEffect();
}

void UETRelicEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (AEmberTrialCharacter* Body = BodyOwner.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleGameplayReadyChanged);
    }
    ReleaseOwnedEffect();
    BodyOwner.Reset();
    Super::EndPlay(EndPlayReason);
}

void UETRelicEquipmentComponent::HandleGameplayReadyChanged(
    AEmberTrialCharacter* BodyCharacter, const bool bReady)
{
    if (BodyCharacter != BodyOwner.Get())
    {
        return;
    }
    if (bReady && !bEndingPlay)
    {
        EnsureOwnedEffect();
    }
    else
    {
        ReleaseOwnedEffect();
    }
}

// ---- 资格与真实Spec：缓存不是权威，外部移除 / 篡改后不能继续假装装备有效 ----
bool UETRelicEquipmentComponent::ReadBody(AEmberTrialCharacter*& OutBody,
                                         UAbilitySystemComponent*& OutASC) const
{
    OutBody = BodyOwner.Get();
    OutASC = IsValid(OutBody) ? OutBody->GetAbilitySystemComponent() : nullptr;
    if (bEndingPlay || !HasBegunPlay() || !IsActive() || !IsValid(OutBody) ||
        OutBody->IsActorBeingDestroyed() || !OutBody->HasAuthority() ||
        OutBody->GetCombatTeam() != EETCombatTeam::Player || !OutBody->IsGameplayReady() ||
        !IsValid(OutASC) || !OutASC->AbilityActorInfo.IsValid() ||
        OutASC->AbilityActorInfo->OwnerActor.Get() != OutBody ||
        OutASC->AbilityActorInfo->AvatarActor.Get() != OutBody ||
        !OutASC->GetSet<UETHealthAttributeSet>() || !OutASC->GetSet<UETCombatAttributeSet>())
    {
        return false;
    }
    TArray<UETRelicEquipmentComponent*> Components;
    OutBody->GetComponents(Components);
    return Components.Num() == 1;
}

bool UETRelicEquipmentComponent::HasIntactOwnedEffect(UAbilitySystemComponent* ASC) const
{
    if (!IsValid(ASC) || EffectASC.Get() != ASC || !OwnedEffect.IsValid())
    {
        return false;
    }
    const FActiveGameplayEffect* Active = ASC->GetActiveGameplayEffect(OwnedEffect);
    if (!Active || Active->bIsInhibited || !Active->Spec.Def ||
        Active->Spec.Def->GetClass() != UETRelicEquipmentEffect::StaticClass() ||
        Active->Spec.GetContext().GetSourceObject() != this)
    {
        return false;
    }
    const float Missing = std::numeric_limits<float>::quiet_NaN();
    const float Health = Active->Spec.GetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_EquipmentMaxHealth, false, Missing);
    const float Attack = Active->Spec.GetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, false, Missing);
    return FMath::IsFinite(Health) && FMath::IsFinite(Attack) &&
           Health == AppliedModifiers.MaxHealthAdditive && Attack == AppliedModifiers.LightAttackBonus;
}

bool UETRelicEquipmentComponent::GetAppliedModifiers(FETRelicModifierTotals& OutModifiers) const
{
    OutModifiers = {};
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadBody(Body, ASC) || !HasIntactOwnedEffect(ASC))
    {
        return false;
    }
    OutModifiers = AppliedModifiers;
    return true;
}

// ---- 只创建一次零幅度槽；后续更新同一句柄，不先卸旧效果造成临时上限下降 ----
bool UETRelicEquipmentComponent::EnsureOwnedEffect()
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (bChanging || !ReadBody(Body, ASC))
    {
        return false;
    }
    if (OwnedEffect.IsValid())
    {
        return HasIntactOwnedEffect(ASC);
    }

    TGuardValue<bool> ChangeGuard(bChanging, true);
    FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
    Context.AddSourceObject(this);
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
        UETRelicEquipmentEffect::StaticClass(), 1.0f, Context);
    if (!Spec.IsValid())
    {
        return false;
    }
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentMaxHealth, 0.0f);
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 0.0f);
    const FActiveGameplayEffectHandle Created = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

    // Apply可触发同步观察者；期间失活 / 结束时只清刚创建的句柄，不能泄漏。
    AEmberTrialCharacter* CurrentBody = nullptr;
    UAbilitySystemComponent* CurrentASC = nullptr;
    if (!IsValid(ASC) || !Created.IsValid() || !ASC->GetActiveGameplayEffect(Created) ||
        !ReadBody(CurrentBody, CurrentASC) || CurrentASC != ASC)
    {
        if (IsValid(ASC) && Created.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(Created);
        }
        return false;
    }
    EffectASC = ASC;
    OwnedEffect = Created;
    AppliedModifiers = {};
    UE_LOG(LogETRelicEquipment, Display,
           TEXT("[M3-02C2-01] Owned effect ready Body=%s MaxAdd=0.0 AttackAdd=0.0"),
           *GetNameSafe(Body));
    return true;
}

// 这里只检查已建模的加法管线；完整槽位 / 基伤 / 世界 / 祭坛策略由C2-02重验。
bool UETRelicEquipmentComponent::AreModifiersValid(
    UAbilitySystemComponent* ASC, const FETRelicModifierTotals& Modifiers) const
{
    if (!FMath::IsFinite(Modifiers.MaxHealthAdditive) || !FMath::IsFinite(Modifiers.LightAttackBonus))
    {
        return false;
    }
    const double NewMax = static_cast<double>(ASC->GetNumericAttribute(
        UETHealthAttributeSet::GetMaxHealthAttribute())) - AppliedModifiers.MaxHealthAdditive +
        Modifiers.MaxHealthAdditive;
    const double NewBonus = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetLightAttackBonusAttribute())) - AppliedModifiers.LightAttackBonus +
        Modifiers.LightAttackBonus;
    const double Limit = std::numeric_limits<float>::max();
    return FMath::IsFinite(NewMax) && NewMax >= 1.0 && NewMax <= Limit &&
           FMath::IsFinite(NewBonus) && FMath::Abs(NewBonus) <= Limit;
}

EETRelicEffectWriteResult UETRelicEquipmentComponent::TrySetModifiers(
    const FETRelicModifierTotals& Modifiers)
{
    if (bChanging)
    {
        return EETRelicEffectWriteResult::Busy;
    }
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadBody(Body, ASC) || Body->IsCombatDead() || !HasIntactOwnedEffect(ASC))
    {
        return EETRelicEffectWriteResult::Unavailable;
    }
    if (!AreModifiersValid(ASC, Modifiers))
    {
        return EETRelicEffectWriteResult::InvalidModifiers;
    }
    if (Modifiers.MaxHealthAdditive == AppliedModifiers.MaxHealthAdditive &&
        Modifiers.LightAttackBonus == AppliedModifiers.LightAttackBonus)
    {
        return EETRelicEffectWriteResult::Unchanged;
    }

    TGuardValue<bool> ChangeGuard(bChanging, true);
    TMap<FGameplayTag, float> Magnitudes;
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentMaxHealth, Modifiers.MaxHealthAdditive);
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, Modifiers.LightAttackBonus);
    {
        // 两个聚合器都更新后才派发脏通知；这不是任意GAS / 库存事务的原子锁。
        FScopedAggregatorOnDirtyBatch DirtyBatch;
        AppliedModifiers = Modifiers;
        ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(OwnedEffect, Magnitudes);
    }
    // Applied表示已向自有GE发出更新；同步回调可以继续改变世界，不在这里伪造回滚。
    return EETRelicEffectWriteResult::Applied;
}

// ---- 精确释放：先丢弃本地身份，再调用可能重入的GAS移除，不清整张ASC ----
void UETRelicEquipmentComponent::ReleaseOwnedEffect()
{
    UAbilitySystemComponent* ASC = EffectASC.Get();
    const FActiveGameplayEffectHandle Released = OwnedEffect;
    OwnedEffect.Invalidate();
    EffectASC.Reset();
    AppliedModifiers = {};
    if (IsValid(ASC) && Released.IsValid() && ASC->GetActiveGameplayEffect(Released))
    {
        ASC->RemoveActiveGameplayEffect(Released);
        UE_LOG(LogETRelicEquipment, Display,
               TEXT("[M3-02C2-01] Owned effect removed Body=%s"), *GetNameSafe(GetOwner()));
    }
}
```

### 4.3 ETRelicEquipmentOwnershipTests.cpp

第3节Tests路径，完整写入。测试创建自己的World / 身体，不写正式DataAsset、不借用正在运行的PIE或库存模型；测试内暂时改变玩家阵营通过第5.3节friend完成：

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicEquipmentComponent.h"
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicEquipmentOwnershipTest,
    "EmberTrial.M3.Inventory.EquipmentOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicEquipmentOwnershipTest::RunTest(const FString& Parameters)
{
    using EWrite = EETRelicEffectWriteResult;
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    AEmberTrialCharacter* Other = World->SpawnActor<AEmberTrialCharacter>(
        FVector(400, 0, 200), FRotator::ZeroRotator, Spawn);
    AEmberTrialCharacter* Neutral = World->SpawnActor<AEmberTrialCharacter>(
        FVector(800, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Player body"), Body) || !TestNotNull(TEXT("Other player body"), Other) ||
        !TestNotNull(TEXT("Neutral body"), Neutral))
    {
        return false;
    }
    Body->CombatTeam = EETCombatTeam::Player;
    Other->CombatTeam = EETCombatTeam::Player;
    UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* OtherEquipment = Other->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* NeutralEquipment = Neutral->FindComponentByClass<UETRelicEquipmentComponent>();
    if (!TestNotNull(TEXT("Native component"), Equipment) ||
        !TestNotNull(TEXT("Other native component"), OtherEquipment) ||
        !TestNotNull(TEXT("Neutral inherits component"), NeutralEquipment))
    {
        return false;
    }
    FETRelicModifierTotals Snapshot;
    TestFalse(TEXT("Before BeginPlay unavailable"), Equipment->GetAppliedModifiers(Snapshot));
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* OtherASC = Other->GetAbilitySystemComponent();
    const FGameplayAttribute MaxHealth = UETHealthAttributeSet::GetMaxHealthAttribute();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    auto CountOwned = [](UAbilitySystemComponent* SourceASC, UETRelicEquipmentComponent* Source)
    {
        int32 Count = 0;
        for (const FActiveGameplayEffectHandle Handle : SourceASC->GetActiveEffects(FGameplayEffectQuery()))
        {
            const FActiveGameplayEffect* Effect = SourceASC->GetActiveGameplayEffect(Handle);
            Count += Effect && Effect->Spec.GetContext().GetSourceObject() == Source ? 1 : 0;
        }
        return Count;
    };

    // ---- 零幅度槽与重复初始化；非玩家不创建效果 ----
    TestTrue(TEXT("Player effect available"), Equipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("Empty max modifier"), Snapshot.MaxHealthAdditive, 0.0f);
    TestEqual(TEXT("Empty attack modifier"), Snapshot.LightAttackBonus, 0.0f);
    TestFalse(TEXT("Neutral remains dormant"), NeutralEquipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("One owned active effect"), CountOwned(ASC, Equipment), 1);
    const FActiveGameplayEffectHandle InitialHandle = Equipment->OwnedEffect;
    TestTrue(TEXT("Repeated Ensure stays ready"), Body->EnsureGameplayInitialized());
    TestTrue(TEXT("Repeated activation reuses handle"), Equipment->EnsureOwnedEffect());
    TestTrue(TEXT("Stable owned handle"), Equipment->OwnedEffect == InitialHandle);
    TestEqual(TEXT("No duplicate active effect"), CountOwned(ASC, Equipment), 1);

    // ---- 同一句柄更新，不经过基础上限100；提高上限不免费治疗 ----
    TestTrue(TEXT("Apply20 and5"), Equipment->TrySetModifiers({20.0f, 5.0f}) == EWrite::Applied);
    TestEqual(TEXT("Max120"), ASC->GetNumericAttribute(MaxHealth), 120.0f);
    TestEqual(TEXT("Bonus5"), ASC->GetNumericAttribute(Bonus), 5.0f);
    TestEqual(TEXT("Increasing max does not heal"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Max Base stays100"), ASC->GetNumericAttributeBase(MaxHealth), 100.0f);
    TestEqual(TEXT("Bonus Base stays0"), ASC->GetNumericAttributeBase(Bonus), 0.0f);
    ASC->SetNumericAttributeBase(Health, 110.0f);
    int32 MaxEvents = 0;
    bool bTemporaryDrop = false;
    EWrite NestedWrite = EWrite::Applied;
    const FDelegateHandle MaxObserver = ASC->GetGameplayAttributeValueChangeDelegate(MaxHealth).AddLambda(
        [&](const FOnAttributeChangeData& Change)
        {
            ++MaxEvents;
            bTemporaryDrop |= Change.NewValue < 120.0f;
            NestedWrite = Equipment->TrySetModifiers({0.0f, 0.0f});
        });
    TestTrue(TEXT("Update40 and7"), Equipment->TrySetModifiers({40.0f, 7.0f}) == EWrite::Applied);
    TestTrue(TEXT("Same handle after update"), Equipment->OwnedEffect == InitialHandle);
    TestEqual(TEXT("Max140"), ASC->GetNumericAttribute(MaxHealth), 140.0f);
    TestEqual(TEXT("Health110 preserved"), ASC->GetNumericAttribute(Health), 110.0f);
    TestEqual(TEXT("One final max event"), MaxEvents, 1);
    TestFalse(TEXT("No intermediate max drop"), bTemporaryDrop);
    TestTrue(TEXT("Nested write is Busy"), NestedWrite == EWrite::Busy);
    TestTrue(TEXT("Identical totals are unchanged"), Equipment->TrySetModifiers({40.0f, 7.0f}) == EWrite::Unchanged);
    TestEqual(TEXT("Unchanged emits no extra max event"), MaxEvents, 1);
    ASC->GetGameplayAttributeValueChangeDelegate(MaxHealth).Remove(MaxObserver);

    // ---- 降上限只夹最终越界值；夹限没有伤害GE来源 ----
    bool bClampHadDamageSource = false;
    const FDelegateHandle HealthObserver = ASC->GetGameplayAttributeValueChangeDelegate(Health).AddLambda(
        [&](const FOnAttributeChangeData& Change) { bClampHadDamageSource |= Change.GEModData != nullptr; });
    TestTrue(TEXT("Signed attack modifier"), Equipment->TrySetModifiers({5.0f, -3.0f}) == EWrite::Applied);
    TestEqual(TEXT("Final max105"), ASC->GetNumericAttribute(MaxHealth), 105.0f);
    TestEqual(TEXT("Current health clamped105"), ASC->GetNumericAttribute(Health), 105.0f);
    TestEqual(TEXT("Base health clamped105"), ASC->GetNumericAttributeBase(Health), 105.0f);
    TestFalse(TEXT("Clamp is not a damage GE"), bClampHadDamageSource);
    ASC->GetGameplayAttributeValueChangeDelegate(Health).Remove(HealthObserver);
    TestFalse(TEXT("Clamp did not kill body"), Body->IsCombatDead());
    TestTrue(TEXT("NaN rejected"), Equipment->TrySetModifiers(
        {std::numeric_limits<float>::quiet_NaN(), 0.0f}) == EWrite::InvalidModifiers);
    TestTrue(TEXT("Nonpositive final max rejected"),
             Equipment->TrySetModifiers({-101.0f, 0.0f}) == EWrite::InvalidModifiers);
    TestTrue(TEXT("Infinite attack rejected"), Equipment->TrySetModifiers(
        {0.0f, std::numeric_limits<float>::infinity()}) == EWrite::InvalidModifiers);
    TestEqual(TEXT("Refusal leaves current max"), ASC->GetNumericAttribute(MaxHealth), 105.0f);
    TestEqual(TEXT("Refusal leaves signed bonus"), ASC->GetNumericAttribute(Bonus), -3.0f);
    TestEqual(TEXT("Other body max isolated"), OtherASC->GetNumericAttribute(MaxHealth), 100.0f);

    // ---- 同类但非自有的GE必须保留；空槽用0更新，失活才移除句柄 ----
    FGameplayEffectSpecHandle ForeignSpec = ASC->MakeOutgoingSpec(
        UETRelicEquipmentEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
    if (!TestTrue(TEXT("Independent spec"), ForeignSpec.IsValid()))
    {
        return false;
    }
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentMaxHealth, 0.0f);
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 2.0f);
    const FActiveGameplayEffectHandle Foreign = ASC->ApplyGameplayEffectSpecToSelf(*ForeignSpec.Data.Get());
    if (!TestTrue(TEXT("Independent active effect"), Foreign.IsValid()))
    {
        return false;
    }
    TestTrue(TEXT("Empty totals update owned GE"), Equipment->TrySetModifiers({}) == EWrite::Applied);
    TestTrue(TEXT("Empty retains owned handle"), Equipment->OwnedEffect == InitialHandle);
    TestEqual(TEXT("Foreign bonus2 retained"), ASC->GetNumericAttribute(Bonus), 2.0f);
    Equipment->Deactivate();
    TestFalse(TEXT("Inactive view unavailable"), Equipment->GetAppliedModifiers(Snapshot));
    TestTrue(TEXT("Inactive owns no effect"), ASC->GetActiveGameplayEffect(InitialHandle) == nullptr);
    TestTrue(TEXT("Foreign survives deactivation"), ASC->GetActiveGameplayEffect(Foreign) != nullptr);
    Equipment->Activate(false);
    TestTrue(TEXT("Explicit reactivation starts zero slot"), Equipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("Reactivated own attack zero"), Snapshot.LightAttackBonus, 0.0f);
    TestEqual(TEXT("Exactly one rebuilt own effect"), CountOwned(ASC, Equipment), 1);

    // 外部移除不被缓存掩盖，也不静默补建；显式失活 / 激活才能重新建空槽。
    ASC->RemoveActiveGameplayEffect(Equipment->OwnedEffect);
    TestFalse(TEXT("External removal detected"), Equipment->GetAppliedModifiers(Snapshot));
    TestTrue(TEXT("Lost handle refuses write"), Equipment->TrySetModifiers({20.0f, 5.0f}) == EWrite::Unavailable);
    TestFalse(TEXT("Ensure does not hide externally lost handle"), Equipment->EnsureOwnedEffect());
    Equipment->Deactivate();
    Equipment->Activate(false);
    const FActiveGameplayEffectHandle BeforeDestroy = Equipment->OwnedEffect;
    Equipment->DestroyComponent();
    TestTrue(TEXT("Component end removes exact own handle"), ASC->GetActiveGameplayEffect(BeforeDestroy) == nullptr);
    TestTrue(TEXT("Component end preserves foreign GE"), ASC->GetActiveGameplayEffect(Foreign) != nullptr);
    ASC->RemoveActiveGameplayEffect(Foreign);

    const FActiveGameplayEffectHandle OtherHandle = OtherEquipment->OwnedEffect;
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Snapshot.MaxHealthAdditive = 99.0f;
    TestFalse(TEXT("World end revokes snapshot"), OtherEquipment->GetAppliedModifiers(Snapshot));
    TestEqual(TEXT("Unavailable output cleared"), Snapshot.MaxHealthAdditive, 0.0f);
    TestTrue(TEXT("World end removes owned effect"), OtherASC->GetActiveGameplayEffect(OtherHandle) == nullptr);
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 5. Character.h：三个位置

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h`，保留原代码。

### 5.1 在前置声明区增加新组件

找到`class UETHealingResourceComponent;`，其后新增一行：

```cpp
class UETRelicEquipmentComponent;
```

不要把声明插到generated.h之前的include之间，也不要在这里粘贴完整组件类。

### 5.2 在私有子对象区增加成员

找到原代码：

```cpp
    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Combat")
    TObjectPtr<UETCombatAttributeSet> CombatAttributes;
```

在其后、原`// ---- 出生配置`之前插入：

```cpp
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Relics",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETRelicEquipmentComponent> RelicEquipmentComponent;
```

这是**类内部的成员**，缩进与CombatAttributes一致；不要放到类外 / public事件区，也不要覆盖Combat属性集。

### 5.3 保留旧friend，追加新测试

找到`friend class FETTargetLockSelectionTest;`，在其后新增：

```cpp
    friend class FETRelicEquipmentOwnershipTest;
```

此friend仅允许本批自动化在自己的夹具设置私有阵营；不提供生产作弊开关，也不删除其他旧测试friend。

## 6. Character.cpp：两个位置

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp`。

### 6.1 增加include

找到`#include "Resources/ETHealingResourceComponent.h"`，其后增加：

```cpp
#include "Inventory/ETRelicEquipmentComponent.h"
```

### 6.2 构造函数创建一次继承组件

找到`AEmberTrialCharacter::AEmberTrialCharacter()`中的原代码：

```cpp
    HealingResourceComponent =
        CreateDefaultSubobject<UETHealingResourceComponent>(TEXT("HealingResource"));
```

其后、原`// 以下为既有模板配置`之前插入：

```cpp
    RelicEquipmentComponent =
        CreateDefaultSubobject<UETRelicEquipmentComponent>(TEXT("RelicEquipment"));
```

只新增一次。**不要改BeginPlay / EnsureGameplayInitialized / Ready / EndPlay，也不要手动给Blueprint Add Component。**新组件自行订阅基础Ready：组件先BeginPlay时等事件，身体先Ready时立即取快照，两种顺序都只创建一份零幅度GE。

## 7. 正常Build、三身体与三项自动化

### 7.1 正常Editor / Game Build

完成第3–6节并保存，保持UE关闭，PowerShell依次执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本成功后CLion Reload CMake / Refresh，三个模块目标仍在，新组件 / 测试属Core。预期Core71 / UI23 / Editor3只作辅助。继续保持UE关闭：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两个正常Build成功后打开原uproject；不用Live Coding替代新增原生反射组件的编译，不重复生成VS解决方案。

### 7.2 核对三个身体各一份继承组件

依次打开下列已有Blueprint，在Components搜索`RelicEquipment`，每个应有**一份继承的原生组件**：

```text
/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter
/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy
/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget
```

选中该组件，Details的Auto Activate应勾选；无其他新属性要填。保留原Player / Enemy / Neutral阵营、100默认资源、原能力Class / Montage与单ASC，**不手动给敌人 / 训练目标Apply GE**。三身体Compile → Save；本项目Validate Assets各一次应Valid。继承组件虽存在，敌人 / 训练目标因阵营不满足而休眠，不要求它们出现Owned effect ready。

若组件没出现，先核对第5.2 / 6.2是否在正确类 / 构造函数、是否正常关闭UE后Build成功再重开；不要加第二份蓝图组件顶替。

### 7.3 三项相关自动化，各一次

保持非PIE，在Output Log命令栏逐行执行，等每项Success再下一项：

```text
Automation RunTests EmberTrial.M3.Inventory.EquipmentOwnership
Automation RunTests EmberTrial.M3.Inventory.EquipmentEffects
Automation RunTests EmberTrial.M2.Combo.Flow
```

Ownership验证唯一句柄 / 读真实Spec、同句柄更新、提高不治疗、110生命不经临时100上限、最终夹限 / 无伤害来源、拒绝 / 重入、同类外部GE隔离、失活 / 再激活 / 外部移除与结束。EquipmentEffects复用B真实ASC / Base / Current契约；Flow复用原真实三段命中，确认默认零槽未改变攻击链快照。不重跑C1三项 / 所有27项，也不新搭Controller测试图。

出现第一条Error / 失败断言就停下来，贴文字与文件行号给我；不要删断言、放宽预期或反复重跑碰运气。

## 8. 正式Play一次短查（约2分钟）

1. 回到`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，Play。正式玩家应有一条`[M3-02C2-01] Owned effect ready Body=... MaxAdd=0.0 AttackAdd=0.0`；训练目标 / 敌人没有该创建日志是预期。由我读取日志，不用抄对象名或猜Handle整数。
2. HUD仍生命 / 体力100、治疗3与F就绪。左键一次原25、2 / E / P一种继续方式和移动 / 转向 / 跳跃正常；本批不手动施加装备，不期待120 / 30，也不增加开发按键。
3. Stop一次，应有玩家Owned effect removed；再Play，新的玩家重新一份零幅度槽 / 原资源，随后Stop。Ready撤销可能先清理，组件EndPlay不应重复移除；无需为了数日志连续十轮。
4. Save All；没有临时参数 / 场景设置要恢复。真实带装备Retry / Continue、NewRun重置与失败事务留C2-02，当前不重复死亡矩阵、Cook、性能采集或旧多Pawn图。

## 9. 简短反馈

```text
M3-02C2-01 完成 / 卡在第__节第__步
三新 / 两旧Source；Editor Build / 三索引 / Game Build：
三身体单继承RelicEquipment / AutoActivate，Compile-Save / Valid：
EquipmentOwnership / EquipmentEffects / Combo.Flow各一次：
只有玩家零幅度GE / 原100与25、左键-2-E-P与基础输入短查：
Stop精确清理 / 下一轮新零槽，原资源与旧HUD正常：
Source / 资产Save All；失败或不理解处（无填无）：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

成功长日志我读取；报错优先给第一条错误文字 / 路径 / 行号。界面找不到组件或出现重复时才补截图；不要删除旧ASC / 属性集来“消除重复”。本批代码已正常编译，三项Success与正式GE清理见验收报告；此处为历史反馈模板，不重复执行。

## 10. 本批 Commit / Push（固定放在最后）

C2-01实现已在本地5462f49提交，远端未重新核验。当前验收报告 / 下一批指南及入口共13份文档，精确清单与未来C2-02源码范围见[C2-02末尾](M3_02C2_02_EquipmentTransactions.md#11-本批-commit--push固定放在最后)。Source / Config / Content / 来源清单保持用户完成后的状态。

当前建议英文`docs: verify equipment ownership and prepare equipment transactions`，中文译文`docs: 记录装备效果所有权验收并准备换装事务与身体恢复`。下一批实现建议英文`feat: commit relic loadouts and restore equipment on new bodies`，中文译文`feat: 提交遗物配装并在新角色身体上恢复装备效果`。CLion刷新Changes→Stage最新修改→Review Diff→Commit / Push由你操作；Raw / 日志 / 包本地，助手未改索引。
