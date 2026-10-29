# M3-02C2-02：真实换装提交与新身体装备恢复

2026-10-29，UE **5.5.4**。[C2-01验收通过](Reports/M3_02C2_01_Verification.md)。本章仍由你手动输入代码；助手只整理文档和本地候选，**运行工程尚未实施、未Build、未宣称新测试Success**。按第3→6节完成全部源码后再统一Build，中途缺少声明出现红线先完成配套修改。

## 1. 目标、范围与时间

本章第一次让正式玩家真实装备两件遗物：生息余烬带来最大生命+20，砺锋残片使当前三段每段基础伤害25→30。GI持有已提交ID，身体只拥有自己的同一份GE；活体换装不补血，Retry / Continue新身体恢复最终装备上限，NewRun确认后回初始两件拥有 / 空槽。菜单UI、祭坛权限、磁盘存档和额外四件遗物仍在后续。

一新 / 九旧Source，没有新资产、输入Action、蓝图图表或配置。预计手动代码 **80–120分钟**，阅读 / 配置 **10–15分钟**，相关自动化与真实检查 **10–15分钟**，构建等待另记。不要把估计登记为已用时间；可以分两次完成源码，但只在全部配套完成后正常Build。

当前实际175 Source / 97cpp、Core71 / UI23 / Editor3、28测试；实施后预计**176 / 98、Core72 / UI23 / Editor3、29测试**。Content296 / 来源77；最新独立包仍是M2_07A/Attempt01，本章不Cook。

| 所有者 | 本章职责 |
| --- | --- |
| InventorySubsystem | 拥有 / 两槽 / 版本、重验方案、提交点、最终领域事件 |
| RelicEquipmentComponent | 当前身体资格、数值预检、精确句柄更新、会话恢复与EquipmentReady |
| Character / LightAttack | 读取真实攻击配置；输入等待装备就绪；保留整链伤害快照 |
| RunSessionSubsystem | 既有Pending / 成功确认 / 失败通知沿用；本章仅增加专项friend |

## 2. 先理解三条执行规则

### 2.1 提交点前后

`TryEquip(槽位, ID, 看到的版本)`先检查当前本地身体、存活 / Idle、暂停 / Pending、版本、拥有 / 槽位规则和最终数值。预检和库存重算都没有属性副作用。通过后，库存先写ID并推进版本，紧接着调用私有同步写入器更新同一句柄，最后广播OnChanged。属性通知发生时已能读到新ID；OnChanged发生时身体同步已结束。

写入器是内部原生函数，不能换成任意蓝图 / 异步操作，也不能在其中增加“先做副作用再返回失败”。GE通知仍可同步造成死亡或世界结束，这属于提交之后的世界变化；不能把它包装成失败退款。`FScopedAggregatorOnDirtyBatch`只合并本次属性脏通知，不提供任意GAS / 领域数据的数据库原子事务。过期版本 / 缺句柄等拒绝都必须发生在提交点之前。

### 2.2 资源语义

- 活体100/100装备生命遗物变为**100/120**；卸下后变100/100；110/120卸下会将Current和Base均夹到100。提高上限、组件重新激活、重复观察均不免费治疗。
- 新身体第一次成功恢复按最终最大生命回满；Retry / Continue带生命遗物应是**120/120**。体力、治疗次数、冷却仍由新身体既有默认初始化处理，不额外重置旧身体资源。
- 新身体BeginPlay可能早于成功切图通知；Pending期间不恢复旧装备，也不接受正式玩法输入。确认库存后再恢复。原`GameplayReady`含义保留，新`EquipmentReady`只负责装备同步。
- 失去控制的旧身体保留自身物理效果，但停止观察共享库存且不能换装。结束 / 失活仍只清自己句柄。无Tick / Timer轮询。

### 2.3 本章开发入口

`ET_Relic`是PIE控制台开发命令；`0`查询、`1 / 2`为槽A / B，`None`卸下。ID来自现有定义，不硬编码资源路径。写入只在Editor编译的PIE执行，普通Game / 打包版无写入逻辑。本章的原生`TryEquip`是后续页面共用的业务入口，正式安全祭坛权限在M3-04统一接入，不能提前当成已完成的祭坛系统。

## 3. 库存提交与攻击只读接口（四个旧文件）

先Save All并关闭UE。CLion保持打开，源码路径根为`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial`。保留旧测试与原功能。

### 3.1 Public/Inventory/ETRelicInventorySubsystem.h

所有include仍在generated.h之前；下面的新声明均在**原有private区域**，不要放到类外。

**同步函数引用include**。找到下面这处原文：

```cpp
#include "CoreMinimal.h"
```

紧接其后插入以下内容，保留原文：

```cpp
#include "Templates/Function.h"
```

**专项friend**。找到下面这处原文：

```cpp
    friend class FETRelicInventorySessionTest;
```

紧接其后插入以下内容，保留原文：

```cpp
    friend class FETRelicEquipmentTransactionTest;
```

**提交重载声明**。找到下面这处原文：

```cpp
    FETRelicInventoryReply TryCommitPlan(const FETRelicInventoryPlan& Plan);
```

紧接其后插入以下内容，保留原文：

```cpp
    // 身体预检完成后使用；回调只执行不会再业务拒绝的原生提交写入。
    FETRelicInventoryReply TryCommitPlan(const FETRelicInventoryPlan& Plan,
        TFunctionRef<void(const FETRelicLoadoutPlan&, uint64)> CommitEffects);
```

### 3.2 Private/Inventory/ETRelicInventorySubsystem.cpp

原先只有一份`TryCommitPlan`实现，整份替换为下面两个重载；后面的HandleTravelConfirmed等函数保留。旧模型专项使用无副作用的重载，真实身体使用带写入器的重载。

**TryCommitPlan原函数替换为两个重载**。从该函数的返回类型开始，直到配对的最后一个`}`，整段替换为下面代码；下一函数保留：

```cpp
FETRelicInventoryReply UETRelicInventorySubsystem::TryCommitPlan(
    const FETRelicInventoryPlan& Plan)
{
    // 仅供既有纯模型专项；生产身体使用下方带写入器的重载。
    return TryCommitPlan(Plan, [](const FETRelicLoadoutPlan&, uint64) {});
}

FETRelicInventoryReply UETRelicInventorySubsystem::TryCommitPlan(const FETRelicInventoryPlan& Plan,
    TFunctionRef<void(const FETRelicLoadoutPlan&, uint64)> CommitEffects)
{
    if (!bReady || bClosing)
    {
        return {EETRelicInventoryResult::NotReady};
    }
    if (bChanging)
    {
        return {EETRelicInventoryResult::Busy};
    }
    if (Plan.Source.Get() != this || Plan.ExpectedRevision == 0)
    {
        return {EETRelicInventoryResult::InvalidPlan};
    }
    if (Plan.ExpectedRevision != Revision || !(Plan.LoadoutPlan.Before == Equipped))
    {
        return {EETRelicInventoryResult::StalePlan};
    }

    // 即使ID和版本相同，也重算静态参数，拒绝预览后被改动的数据定义。
    FETRelicInventoryPlan Fresh;
    const FETRelicInventoryReply Reply = TryBuildPlan(
        Plan.Slot, Plan.RequestedId, Plan.BaseMaxHealth, Plan.BaseLightAttackDamage, Fresh);
    if (Reply.Result != EETRelicInventoryResult::Success)
    {
        return {EETRelicInventoryResult::StalePlan, Reply.Rule};
    }
    const FETRelicLoadoutPlan& A = Plan.LoadoutPlan;
    const FETRelicLoadoutPlan& B = Fresh.LoadoutPlan;
    auto SameTotals = [](const FETRelicModifierTotals& X, const FETRelicModifierTotals& Y)
    {
        return X.MaxHealthAdditive == Y.MaxHealthAdditive && X.LightAttackBonus == Y.LightAttackBonus;
    };
    auto SamePreview = [](const FETRelicLoadoutPreview& X, const FETRelicLoadoutPreview& Y)
    {
        return X.MaxHealth == Y.MaxHealth && X.LightAttackBaseDamage == Y.LightAttackBaseDamage;
    };
    if (!(A.After == B.After) || !SameTotals(A.BeforeModifiers, B.BeforeModifiers) ||
        !SameTotals(A.AfterModifiers, B.AfterModifiers) ||
        !SamePreview(A.BeforePreview, B.BeforePreview) || !SamePreview(A.AfterPreview, B.AfterPreview))
    {
        return {EETRelicInventoryResult::StalePlan};
    }

    // 此处之前均可拒绝且没有属性副作用。此处之后是提交点：先记录ID，
    // 再执行已经预检的同句柄更新，最后发布完整领域事件；不伪造资源回滚。
    TGuardValue<bool> ChangeGuard(bChanging, true);
    Equipped = A.After;
    AdvanceRevision();
    CommitEffects(A, Revision);
    PublishSnapshot();
    return {EETRelicInventoryResult::Success};
}
```

### 3.3 Public/Abilities/ETGA_LightAttack.h

注意本头文件包含`FETLightAttackStage`结构体。下面`public:`是**UETGA_LightAttack类中、构造函数上方**的public区域，不是结构体内，也不是其他类。

**只读伤害配置接口**。找到下面这处原文：

```cpp
public:
```

紧接其后插入以下内容，保留原文：

```cpp
    // 只读配置供装备预检；不授予能力、不改变当前连段的已捕获快照。
    float GetBaseAttackDamage() const { return AttackDamage; }
    bool IsDamageBonusValid(float Bonus) const;
```

### 3.4 Private/Abilities/ETGA_LightAttack.cpp

只替换原`TryCaptureDamageBonus`，并在其后增加`IsDamageBonusValid`；原`TryCalculateStageDamage`完整保留。装备和能力使用同一数值规则，当前连段已经捕获的Bonus仍不会中途变更。

**UETGA_LightAttack::TryCaptureDamageBonus**。从该函数的返回类型开始，直到配对的最后一个`}`，整段替换为下面代码；下一函数保留：

```cpp
bool UETGA_LightAttack::TryCaptureDamageBonus(
    const AEmberTrialCharacter* Body, float& OutBonus) const
{
    OutBonus = 0.0f;
    float Bonus = 0.0f;
    if (!IsValid(Body) || !Body->GetGameplayLightAttackBonusSnapshot(Bonus) ||
        !IsDamageBonusValid(Bonus))
    {
        return false;
    }
    OutBonus = Bonus;
    return true;
}

bool UETGA_LightAttack::IsDamageBonusValid(const float Bonus) const
{
    float Damage = 0.0f;
    if (!TryCalculateStageDamage(AttackDamage, Bonus, 1.0f, Damage))
    {
        return false;
    }
    if (bEnableCombo)
    {
        for (const FETLightAttackStage& Stage : ComboFollowUps)
        {
            if (!TryCalculateStageDamage(AttackDamage, Bonus, Stage.DamageMultiplier, Damage))
            {
                return false;
            }
        }
    }
    return true;
}
```

## 4. 装备组件（两个旧文件完整替换）

这两个文件逻辑关联较多，直接提供完整最终内容，避免漏改BeginPlay / Ready / EndPlay。已有的Ownership低层测试保持原样；其中TrySetModifiers仍为private，只供底层和专项，不作为UI接口。

### 4.1 Public/Inventory/ETRelicEquipmentComponent.h

从`#pragma once`到最后`};`全部替换为：


```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "ETRelicEquipmentComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETRunSessionSubsystem;

enum class EETRelicEffectWriteResult : uint8
{
    Applied,
    Unchanged,
    Unavailable,
    Busy,
    InvalidModifiers
};

// 身体装备适配层：独占自身GE句柄；逻辑库存由GI持有。UI不能直接写ASC。
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

    // 基础GameplayReady不等于装备已恢复；正式就绪还要求当前控制与会话一致。
    bool IsEquipmentReady() const;
    void RefreshEquipmentState();

    // 调用方必须带看到的库存版本；正式祭坛权限由后续入口层接入。
    // 本章唯一交互调用者是受限PIE开发命令，不暴露BlueprintCallable。
    FETRelicInventoryReply TryEquip(EETRelicSlot Slot, FName RelicId, uint64 ExpectedRevision);


protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETRelicEquipmentOwnershipTest;
    friend class FETRelicEquipmentTransactionTest;

    bool IsCurrentPlayerBody() const;
    bool ResolveSessionSources();
    bool ObserveSession(UETRelicInventorySubsystem* Inventory, UETRunSessionSubsystem* Session);
    void StopObservingSession();
    void HandleInventoryChanged(const FETRelicInventorySnapshot& Snapshot);
    void HandleTravelFailed(UWorld* FailedWorld, const FText& Error);
    bool ReadPreviewBases(float& OutMaxHealth, float& OutAttackDamage) const;
    bool IsFinalDamageValid(const FETRelicModifierTotals& Modifiers) const;
    void WriteModifiersUnchecked(const FETRelicModifierTotals& Modifiers);


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

    // 只缓存本身体已同步的身份 / 版本；不拥有另一个库存或旧世界。
    TWeakObjectPtr<UETRelicInventorySubsystem> InventorySource;
    TWeakObjectPtr<UETRunSessionSubsystem> SessionSource;
    FDelegateHandle InventoryChangedHandle;
    FDelegateHandle TravelFailedHandle;
    FETRelicLoadout AppliedLoadout;
    uint64 AppliedRevision = 0;
    bool bInitialRestorePending = true;

    bool bChanging = false;
    bool bEndingPlay = false;
};
```

### 4.2 Private/Inventory/ETRelicEquipmentComponent.cpp

整个文件替换为以下完整内容。日志中的C2-01所有权标记保留，新增C2-02恢复标记；不要求你抄Handle或对象递增后缀。


```cpp
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
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
    RefreshEquipmentState();
}

void UETRelicEquipmentComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    if (HasBegunPlay() && !bEndingPlay)
    {
        RefreshEquipmentState();
    }
}

void UETRelicEquipmentComponent::Deactivate()
{
    Super::Deactivate();
    StopObservingSession();
    ReleaseOwnedEffect();
}

void UETRelicEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    StopObservingSession();
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
        RefreshEquipmentState();
    }
    else
    {
        StopObservingSession();
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
    WriteModifiersUnchecked(Modifiers);
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
    AppliedLoadout = {};
    AppliedRevision = 0;
    if (IsValid(ASC) && Released.IsValid() && ASC->GetActiveGameplayEffect(Released))
    {
        ASC->RemoveActiveGameplayEffect(Released);
        UE_LOG(LogETRelicEquipment, Display,
               TEXT("[M3-02C2-01] Owned effect removed Body=%s"), *GetNameSafe(GetOwner()));
    }
}

// ---- 预检后的写入原语：不再执行可能拒绝的业务判断；只写本身体的同一句柄 ----
void UETRelicEquipmentComponent::WriteModifiersUnchecked(const FETRelicModifierTotals& Modifiers)
{
    UAbilitySystemComponent* ASC = EffectASC.Get();
    TMap<FGameplayTag, float> Magnitudes;
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentMaxHealth, Modifiers.MaxHealthAdditive);
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, Modifiers.LightAttackBonus);
    {
        // 两个聚合器都更新后才派发脏通知；这不是任意GAS / 库存事务的原子锁。
        FScopedAggregatorOnDirtyBatch DirtyBatch;
        AppliedModifiers = Modifiers;
        ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(OwnedEffect, Magnitudes);
    }
}

// ---- 当前身体与会话来源；只订阅自己使用的两个委托，不轮询 ----
bool UETRelicEquipmentComponent::IsCurrentPlayerBody() const
{
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    const APlayerController* PC = IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    return IsValid(PC) && PC->IsLocalController() && PC->GetPawn() == Body &&
           GetWorld() && GetWorld()->GetNetMode() == NM_Standalone;
}

bool UETRelicEquipmentComponent::ObserveSession(
    UETRelicInventorySubsystem* Inventory, UETRunSessionSubsystem* Session)
{
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!IsValid(GI) || !IsValid(Inventory) || !IsValid(Session) ||
        Inventory->GetGameInstance() != GI || Session->GetGameInstance() != GI)
    {
        return false;
    }
    if (InventorySource.Get() == Inventory && SessionSource.Get() == Session &&
        InventoryChangedHandle.IsValid() && TravelFailedHandle.IsValid())
    {
        return true;
    }
    StopObservingSession();
    InventorySource = Inventory;
    SessionSource = Session;
    InventoryChangedHandle = Inventory->OnChanged.AddUObject(this, &ThisClass::HandleInventoryChanged);
    TravelFailedHandle = Session->OnTravelFailed.AddUObject(this, &ThisClass::HandleTravelFailed);
    return true;
}

bool UETRelicEquipmentComponent::ResolveSessionSources()
{
    UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    if (!IsValid(GI))
    {
        return false;
    }
    if (InventorySource.IsValid() && SessionSource.IsValid() &&
        InventorySource->GetGameInstance() == GI && SessionSource->GetGameInstance() == GI)
    {
        return true;
    }
    return ObserveSession(GI->GetSubsystem<UETRelicInventorySubsystem>(),
                          GI->GetSubsystem<UETRunSessionSubsystem>());
}

void UETRelicEquipmentComponent::StopObservingSession()
{
    if (UETRelicInventorySubsystem* Inventory = InventorySource.Get())
    {
        Inventory->OnChanged.Remove(InventoryChangedHandle);
    }
    if (UETRunSessionSubsystem* Session = SessionSource.Get())
    {
        Session->OnTravelFailed.Remove(TravelFailedHandle);
    }
    InventoryChangedHandle.Reset();
    TravelFailedHandle.Reset();
    InventorySource.Reset();
    SessionSource.Reset();
}

void UETRelicEquipmentComponent::HandleInventoryChanged(const FETRelicInventorySnapshot& Snapshot)
{
    RefreshEquipmentState();
}

void UETRelicEquipmentComponent::HandleTravelFailed(UWorld* FailedWorld, const FText& Error)
{
    if (FailedWorld == GetWorld())
    {
        RefreshEquipmentState();
    }
}

bool UETRelicEquipmentComponent::IsEquipmentReady() const
{
    FETRelicModifierTotals Modifiers;
    if (bChanging || !GetAppliedModifiers(Modifiers))
    {
        return false;
    }
    // 低层GE可独立存在；正式装备就绪不能用缓存替代当前会话 / 控制身份。
    if (!IsCurrentPlayerBody() || !InventorySource.IsValid() || !SessionSource.IsValid() ||
        SessionSource->IsTravelPending())
    {
        return false;
    }
    // 输入路径只比较小型身份字段，不为每次移动复制拥有列表。
    const UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    return Inventory->GetGameInstance() == GetWorld()->GetGameInstance() &&
           SessionSource->GetGameInstance() == GetWorld()->GetGameInstance() &&
           Inventory->bReady && !Inventory->bClosing && AppliedRevision == Inventory->Revision &&
           AppliedLoadout == Inventory->Equipped;
}

// ---- 恢复：先等最终切图库存，再写新身体；同一身体刷新 / 重激活不免费治疗 ----
void UETRelicEquipmentComponent::RefreshEquipmentState()
{
    if (bChanging || !EnsureOwnedEffect())
    {
        return;
    }
    if (!IsCurrentPlayerBody())
    {
        // 已离开控制的旧身体保留物理效果，但不继续观察共享库存。
        StopObservingSession();
        return;
    }
    if (!ResolveSessionSources() || SessionSource->IsTravelPending())
    {
        return;
    }
    UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    const FETRelicInventorySnapshot Snapshot = Inventory->GetSnapshot();
    AEmberTrialCharacter* Body = BodyOwner.Get();
    UAbilitySystemComponent* ASC = EffectASC.Get();
    if (!Snapshot.bReady || !IsValid(Body) || Body->IsCombatDead() || !IsValid(ASC))
    {
        return;
    }
    if (AppliedRevision == Snapshot.Revision && AppliedLoadout == Snapshot.Equipped)
    {
        return;
    }
    FETRelicModifierTotals Totals;
    if (ETRelicLoadoutRules::Evaluate(Snapshot.Equipped, Inventory->OwnedSet,
        Inventory->MakeDefinitionMap(), Totals) != EETRelicLoadoutResult::Success ||
        !AreModifiersValid(ASC, Totals) || !IsFinalDamageValid(Totals))
    {
        UE_LOG(LogETRelicEquipment, Warning,
               TEXT("[M3-02C2-02] Restore rejected Body=%s Revision=%llu"),
               *GetNameSafe(Body), static_cast<unsigned long long>(Snapshot.Revision));
        return;
    }

    TGuardValue<bool> ChangeGuard(bChanging, true);
    const bool bRefillNewBody = bInitialRestorePending;
    bInitialRestorePending = false;
    AppliedLoadout = Snapshot.Equipped;
    AppliedRevision = Snapshot.Revision;
    WriteModifiersUnchecked(Totals);

    // GE通知可以同步结束 / 失活身体。发生后不再向失效ASC补写生命。
    AEmberTrialCharacter* CurrentBody = nullptr;
    UAbilitySystemComponent* CurrentASC = nullptr;
    if (bRefillNewBody && ReadBody(CurrentBody, CurrentASC) && CurrentASC == ASC &&
        !CurrentBody->IsCombatDead() && HasIntactOwnedEffect(ASC) && IsCurrentPlayerBody())
    {
        ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(),
                                    ASC->GetNumericAttribute(UETHealthAttributeSet::GetMaxHealthAttribute()));
    }
    UE_LOG(LogETRelicEquipment, Display,
           TEXT("[M3-02C2-02] Restore Body=%s Revision=%llu A=%s B=%s Initial=%d"),
           *GetNameSafe(GetOwner()), static_cast<unsigned long long>(Snapshot.Revision),
           *Snapshot.Equipped.SlotA.ToString(), *Snapshot.Equipped.SlotB.ToString(), bRefillNewBody ? 1 : 0);
}

bool UETRelicEquipmentComponent::IsFinalDamageValid(const FETRelicModifierTotals& Modifiers) const
{
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    UAbilitySystemComponent* ASC = EffectASC.Get();
    const UETGA_LightAttack* Attack = IsValid(Body) ? Body->GetLightAttackConfiguration() : nullptr;
    // 未配置攻击的隔离身体只允许零攻击加成；正式玩家的配置在资产校验中保证。
    if (!IsValid(Attack))
    {
        return Modifiers.LightAttackBonus == 0.0f;
    }
    const double Bonus = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetLightAttackBonusAttribute())) - AppliedModifiers.LightAttackBonus +
        Modifiers.LightAttackBonus;
    return FMath::IsFinite(Bonus) && FMath::Abs(Bonus) <= std::numeric_limits<float>::max() &&
           Attack->IsDamageBonusValid(static_cast<float>(Bonus));
}

bool UETRelicEquipmentComponent::ReadPreviewBases(float& OutMaxHealth, float& OutAttackDamage) const
{
    OutMaxHealth = 0.0f;
    OutAttackDamage = 0.0f;
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    UAbilitySystemComponent* ASC = EffectASC.Get();
    const UETGA_LightAttack* Attack = IsValid(Body) ? Body->GetLightAttackConfiguration() : nullptr;
    if (!IsValid(ASC) || !IsValid(Attack))
    {
        return false;
    }
    // 已建模的加法管线中排除本装备贡献；其他来源的加法仍计入预览。
    const double Max = static_cast<double>(ASC->GetNumericAttribute(
        UETHealthAttributeSet::GetMaxHealthAttribute())) - AppliedModifiers.MaxHealthAdditive;
    const double Damage = static_cast<double>(Attack->GetBaseAttackDamage()) +
        ASC->GetNumericAttribute(UETCombatAttributeSet::GetLightAttackBonusAttribute()) -
        AppliedModifiers.LightAttackBonus;
    const double Limit = std::numeric_limits<float>::max();
    if (!FMath::IsFinite(Max) || Max < 1.0 || Max > Limit ||
        !FMath::IsFinite(Damage) || Damage <= 0.0 || Damage > Limit)
    {
        return false;
    }
    OutMaxHealth = static_cast<float>(Max);
    OutAttackDamage = static_cast<float>(Damage);
    return true;
}

// ---- 同步提交：先完成所有拒绝判断；提交点之后只执行预检过的原生写入 ----
FETRelicInventoryReply UETRelicEquipmentComponent::TryEquip(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision)
{
    UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    if (bChanging || (IsValid(Inventory) && Inventory->bChanging))
    {
        return {EETRelicInventoryResult::Busy};
    }
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadBody(Body, ASC) || !IsEquipmentReady() || !IsCurrentPlayerBody() ||
        !IsValid(Inventory) || !Body->IsFormalActionIdle() || UGameplayStatics::IsGamePaused(this))
    {
        return {EETRelicInventoryResult::NotReady};
    }
    if (ExpectedRevision == 0 || ExpectedRevision != Inventory->GetSnapshot().Revision)
    {
        return {EETRelicInventoryResult::StalePlan};
    }
    float BaseMax = 0.0f;
    float BaseDamage = 0.0f;
    if (!ReadPreviewBases(BaseMax, BaseDamage))
    {
        return {EETRelicInventoryResult::RuleRejected, EETRelicLoadoutResult::InvalidBaseValues};
    }
    FETRelicInventoryPlan Plan;
    const FETRelicInventoryReply Reply = Inventory->TryBuildPlan(Slot, RelicId, BaseMax, BaseDamage, Plan);
    if (Reply.Result != EETRelicInventoryResult::Success)
    {
        return Reply;
    }
    const FETRelicModifierTotals& Totals = Plan.GetLoadoutPlan().AfterModifiers;
    if (!AreModifiersValid(ASC, Totals) || !IsFinalDamageValid(Totals))
    {
        return {EETRelicInventoryResult::RuleRejected, EETRelicLoadoutResult::InvalidFinalValues};
    }
    // 此前均为无副作用读取；无异步间隙。库存内部还会重验来源 / 版本 / 定义。
    return Inventory->TryCommitPlan(Plan,
        [this](const FETRelicLoadoutPlan& Committed, const uint64 Revision)
        {
            TGuardValue<bool> ChangeGuard(bChanging, true);
            AppliedLoadout = Committed.After;
            AppliedRevision = Revision;
            WriteModifiersUnchecked(Committed.AfterModifiers);
        });
}
```

## 5. Character接入与RunSession专项声明（三个旧文件）

### 5.1 Public/Characters/EmberTrialCharacter.h

第一个friend仍在既有private区域；查询 / 命令跟随原`IsFormalActionIdle`放在原public区域。声明和.cpp实现要配套，原有IsCombatDead等函数保持。

**专项friend**。找到下面这处原文：

```cpp
    friend class FETRelicEquipmentOwnershipTest;
```

紧接其后插入以下内容，保留原文：

```cpp
    friend class FETRelicEquipmentTransactionTest;
```

**装备查询与开发命令**。找到下面这处原文：

```cpp
    bool IsFormalActionIdle() const;
```

紧接其后插入以下内容，保留原文：

```cpp
    const UETGA_LightAttack* GetLightAttackConfiguration() const;
    bool IsRelicEquipmentReadyForInput() const;

    // 0仅查询，1 / 2为槽位；None表示卸下。写入仅允许Editor的PIE。
    UFUNCTION(Exec)
    void ET_Relic(int32 Slot = 0, FName RelicId = NAME_None);
```

### 5.2 Private/Characters/EmberTrialCharacter.cpp

四个Request只增加装备门控，不改成统一Idle判断。这样才能保留活动轻击连段缓冲和恢复阶段闪避取消。

**RequestLightAttack首个if增加一项**。在标题指定的函数中，把其首个`if`的下面这一行：

```cpp
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

替换为：

```cpp
    if (!IsRelicEquipmentReadyForInput() || !IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

后面的条件、函数体和返回值全部保留；不要把整函数删成这一行。

**RequestDodge首个if增加一项**。在标题指定的函数中，把其首个`if`的下面这一行：

```cpp
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

替换为：

```cpp
    if (!IsRelicEquipmentReadyForInput() || !IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

后面的条件、函数体和返回值全部保留；不要把整函数删成这一行。

**RequestHeal首个if增加一项**。在标题指定的函数中，把其首个`if`的下面这一行：

```cpp
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

替换为：

```cpp
    if (!IsRelicEquipmentReadyForInput() || !IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

后面的条件、函数体和返回值全部保留；不要把整函数删成这一行。

**RequestBreakPulse首个if增加一项**。在标题指定的函数中，把其首个`if`的下面这一行：

```cpp
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

替换为：

```cpp
    if (!IsRelicEquipmentReadyForInput() || !IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
```

后面的条件、函数体和返回值全部保留；不要把整函数删成这一行。

**IsFormalActionIdle增加条件**。找到下面这处原文：

```cpp
bool AEmberTrialCharacter::IsFormalActionIdle() const
{
    return !bAbilitySystemShuttingDown && IsGameplayReady() && !IsCombatDead() &&
```

紧接其后插入以下内容，保留原文：

```cpp
           IsRelicEquipmentReadyForInput() &&
```

**RefreshEmberAbilityActorInfo末尾刷新装备**。找到下面这处原文：

```cpp
           *GetNameSafe(AbilitySystemComponent->AbilityActorInfo->PlayerController.Get()));
```

紧接其后插入以下内容，保留原文：

```cpp
    if (IsValid(RelicEquipmentComponent))
    {
        RelicEquipmentComponent->RefreshEquipmentState();
    }
```

**补全GI类型include**。找到下面这处原文：

```cpp
#include "Inventory/ETRelicEquipmentComponent.h"
```

紧接其后插入以下内容，保留原文：

```cpp
#include "Engine/GameInstance.h"
```

**末尾新增三个完整函数**。添加到此.cpp现有最后一个函数之后，保留所有既有函数：

```cpp
// ---- 装备读取与开发命令：正式输入只读Ready，换装策略不散落到Widget ----
const UETGA_LightAttack* AEmberTrialCharacter::GetLightAttackConfiguration() const
{
    return !bAbilitySystemShuttingDown && IsValid(LightAttackAbilityClass.Get())
        ? LightAttackAbilityClass->GetDefaultObject<UETGA_LightAttack>() : nullptr;
}

bool AEmberTrialCharacter::IsRelicEquipmentReadyForInput() const
{
    // 这里只门控受玩家控制的输入；无人控制身体的原生能力专项仍可独立运行。
    // 真正换装入口另外严格检查当前本地PlayerController与库存身份。
    return CombatTeam != EETCombatTeam::Player || !IsPlayerControlled() ||
           (IsValid(RelicEquipmentComponent) && RelicEquipmentComponent->IsEquipmentReady());
}

void AEmberTrialCharacter::ET_Relic(const int32 Slot, const FName RelicId)
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    UETRelicInventorySubsystem* Inventory = IsValid(World) && World->GetGameInstance()
        ? World->GetGameInstance()->GetSubsystem<UETRelicInventorySubsystem>() : nullptr;
    if (!IsValid(World) || World->WorldType != EWorldType::PIE || !IsValid(Inventory) ||
        !IsValid(RelicEquipmentComponent) || Slot < 0 || Slot > 2)
    {
        return;
    }
    FETRelicInventoryReply Reply{EETRelicInventoryResult::NoChange};
    if (Slot != 0)
    {
        Reply = RelicEquipmentComponent->TryEquip(
            Slot == 1 ? EETRelicSlot::First : EETRelicSlot::Second, RelicId,
            Inventory->GetSnapshot().Revision);
    }
    const FETRelicInventorySnapshot Snapshot = Inventory->GetSnapshot();
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    float Bonus = 0.0f;
    GetGameplayHealthSnapshot(Health, MaxHealth);
    GetGameplayLightAttackBonusSnapshot(Bonus);
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M3-02C2-02] Relic dev Slot=%d Result=%d Rule=%d Ready=%d Revision=%llu "
                "A=%s B=%s Health=%.1f/%.1f Bonus=%.1f"),
           Slot, static_cast<int32>(Reply.Result), static_cast<int32>(Reply.Rule),
           RelicEquipmentComponent->IsEquipmentReady() ? 1 : 0,
           static_cast<unsigned long long>(Snapshot.Revision), *Snapshot.Equipped.SlotA.ToString(),
           *Snapshot.Equipped.SlotB.ToString(), Health, MaxHealth, Bonus);
#else
    (void)Slot;
    (void)RelicId;
#endif
}
```

`IsRelicEquipmentReadyForInput`约束受玩家控制的身体输入；无人控制身体的原生能力调用仍可独立运行。真正换装另行严格检查当前PlayerController / Pawn与会话，不因为这条输入兼容规则允许旧身体提交。

### 5.3 Public/Flow/ETRunSessionSubsystem.h

只在原有private区域增加一条friend。切图.cpp、确认广播与Pending逻辑完全沿用。

**仅专项friend，不改运行切图逻辑**。找到下面这处原文：

```cpp
private:
```

紧接其后插入以下内容，保留原文：

```cpp
    friend class FETRelicEquipmentTransactionTest;
```

## 6. 一个新自动化文件

在CLion创建普通C++源文件，不使用UE“创建C++类”向导。完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicEquipmentTransactionTests.cpp
```

粘贴下面完整内容。保留旧测试，不添加无效的.generated.h。夹具直接使用FTestWorldWrapper(Game)创建的真实GI子系统，不另行Init / Shutdown一份GI。模拟确认意图用于专项覆盖恢复时序，不能代替第9节真实OpenLevel菜单往返。


```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicEquipmentComponent.h"
#include "Abilities/ETGA_LightAttack.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicEquipmentTransactionTest,
    "EmberTrial.M3.Inventory.EquipmentTransactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicEquipmentTransactionTest::RunTest(const FString& Parameters)
{
    using EResult = EETRelicInventoryResult;
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    // FTestWorldWrapper(Game)会创建并初始化自己的GI，直接使用其真实子系统。
    // 不替换World的GI，不另外Init / Shutdown第二份实例。
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    UGameInstance* GI = World->GetGameInstance();
    UETRelicInventorySubsystem* Inventory = IsValid(GI) ? GI->GetSubsystem<UETRelicInventorySubsystem>() : nullptr;
    UETRunSessionSubsystem* Session = IsValid(GI) ? GI->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Inventory subsystem"), Inventory) ||
        !TestNotNull(TEXT("Session subsystem"), Session) ||
        !TestTrue(TEXT("Configured inventory ready"), Inventory->GetSnapshot().bReady))
    {
        return false;
    }
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APlayerController* PC = World->SpawnActor<APlayerController>();
    auto SpawnPlayerBody = [&](const float X)
    {
        AEmberTrialCharacter* Result = World->SpawnActor<AEmberTrialCharacter>(
            FVector(X, 0, 200), FRotator::ZeroRotator, Spawn);
        if (Result)
        {
            Result->CombatTeam = EETCombatTeam::Player;
            Result->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
        }
        return Result;
    };
    AEmberTrialCharacter* Body = SpawnPlayerBody(0);
    AEmberTrialCharacter* RetryBody = SpawnPlayerBody(400);
    AEmberTrialCharacter* NewRunBody = SpawnPlayerBody(800);
    if (!TestNotNull(TEXT("Controller"), PC) || !TestNotNull(TEXT("Body"), Body) ||
        !TestNotNull(TEXT("Retry body"), RetryBody) || !TestNotNull(TEXT("NewRun body"), NewRunBody))
    {
        return false;
    }
    PC->Possess(Body);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* RetryEquipment = RetryBody->FindComponentByClass<UETRelicEquipmentComponent>();
    UETRelicEquipmentComponent* NewEquipment = NewRunBody->FindComponentByClass<UETRelicEquipmentComponent>();
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    const FGameplayAttribute Max = UETHealthAttributeSet::GetMaxHealthAttribute();
    const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    if (!TestNotNull(TEXT("Equipment"), Equipment) || !TestNotNull(TEXT("Retry equipment"), RetryEquipment) ||
        !TestNotNull(TEXT("New equipment"), NewEquipment) || !TestNotNull(TEXT("ASC"), ASC))
    {
        return false;
    }
    TestTrue(TEXT("Uses real same-GI domain objects"), Equipment->InventorySource.Get() == Inventory &&
             Equipment->SessionSource.Get() == Session);
    Equipment->RefreshEquipmentState();
    TestTrue(TEXT("Restored empty inventory ready"), Equipment->IsEquipmentReady());
    const FActiveGameplayEffectHandle Handle = Equipment->OwnedEffect;
    auto Equip = [&](EETRelicSlot Slot, FName Id)
    {
        return Equipment->TryEquip(Slot, Id, Inventory->GetSnapshot().Revision).Result;
    };

    // ---- 一次真实提交；属性回调已能读到新ID，重入写在提交点期间被阻止 ----
    int32 DomainEvents = 0;
    bool bDomainSeesSettled = false;
    const FDelegateHandle DomainHandle = Inventory->OnChanged.AddLambda(
        [&](const FETRelicInventorySnapshot& Changed)
        {
            ++DomainEvents;
            bDomainSeesSettled = Equipment->IsEquipmentReady() &&
                ASC->GetNumericAttribute(Max) == 120.0f && Changed.Equipped.SlotA == LifeId;
        });
    bool bAttributeSeesNewIds = false;
    EResult Nested = EResult::Success;
    const FDelegateHandle AttributeHandle = ASC->GetGameplayAttributeValueChangeDelegate(Max).AddLambda(
        [&](const FOnAttributeChangeData& Change)
        {
            bAttributeSeesNewIds = Inventory->GetSnapshot().Equipped.SlotA == LifeId;
            Nested = Equip(EETRelicSlot::Second, EdgeId);
        });
    const uint64 OldRevision = Inventory->GetSnapshot().Revision;
    TestTrue(TEXT("Equip life"), Equip(EETRelicSlot::First, LifeId) == EResult::Success);
    TestTrue(TEXT("Attribute sees committed IDs and nested write is Busy"),
             bAttributeSeesNewIds && Nested == EResult::Busy);
    TestTrue(TEXT("Domain event after settled body"), bDomainSeesSettled);
    TestEqual(TEXT("One domain event"), DomainEvents, 1);
    ASC->GetGameplayAttributeValueChangeDelegate(Max).Remove(AttributeHandle);
    Inventory->OnChanged.Remove(DomainHandle);
    TestEqual(TEXT("Live equip does not heal"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Max120"), ASC->GetNumericAttribute(Max), 120.0f);
    TestEqual(TEXT("Max Base100"), ASC->GetNumericAttributeBase(Max), 100.0f);
    TestTrue(TEXT("Same exact GE handle"), Equipment->OwnedEffect == Handle);
    TestTrue(TEXT("Stale UI version rejected"),
        Equipment->TryEquip(EETRelicSlot::Second, EdgeId, OldRevision).Result == EResult::StalePlan);
    TestTrue(TEXT("Duplicate rejected"), Equip(EETRelicSlot::Second, LifeId) == EResult::RuleRejected);
    TestTrue(TEXT("Unowned rejected"), Equip(EETRelicSlot::Second, FName(TEXT("missing"))) == EResult::RuleRejected);
    TestTrue(TEXT("Same item NoChange"), Equip(EETRelicSlot::First, LifeId) == EResult::NoChange);
    TestEqual(TEXT("Rejections preserve health"), ASC->GetNumericAttribute(Health), 100.0f);
    TestTrue(TEXT("Equip edge"), Equip(EETRelicSlot::Second, EdgeId) == EResult::Success);
    TestEqual(TEXT("Real attack bonus5"), ASC->GetNumericAttribute(Bonus), 5.0f);
    float Damage = 0.0f;
    TestTrue(TEXT("Attack formula resolves30"), UETGA_LightAttack::TryCalculateStageDamage(
        Body->GetLightAttackConfiguration()->GetBaseAttackDamage(), ASC->GetNumericAttribute(Bonus), 1.0f, Damage));
    TestEqual(TEXT("Resolved30"), Damage, 30.0f);

    // ---- 活体卸下只夹限；失活 / 重激活与重复观察不恢复生命 ----
    ASC->SetNumericAttributeBase(Health, 110.0f);
    TestTrue(TEXT("Unequip life"), Equip(EETRelicSlot::First, NAME_None) == EResult::Success);
    TestEqual(TEXT("Current clamps100"), ASC->GetNumericAttribute(Health), 100.0f);
    TestEqual(TEXT("Base clamps100"), ASC->GetNumericAttributeBase(Health), 100.0f);
    TestTrue(TEXT("Equip life again"), Equip(EETRelicSlot::First, LifeId) == EResult::Success);
    ASC->SetNumericAttributeBase(Health, 70.0f);
    Equipment->Deactivate();
    Equipment->Activate(false);
    Equipment->RefreshEquipmentState();
    Equipment->RefreshEquipmentState();
    TestEqual(TEXT("Reactivation retains70"), ASC->GetNumericAttribute(Health), 70.0f);
    TestEqual(TEXT("Reactivation restores max120"), ASC->GetNumericAttribute(Max), 120.0f);
    Session->bTravelPending = true;
    TestFalse(TEXT("Pending locks input readiness"), Equipment->IsEquipmentReady());
    TestTrue(TEXT("Pending rejects mutation"), Equip(EETRelicSlot::First, NAME_None) == EResult::NotReady);
    const uint64 BeforeFailedTravel = Inventory->GetSnapshot().Revision;
    Session->bTravelPending = false;
    Session->OnTravelFailed.Broadcast(World, FText::GetEmpty());
    TestTrue(TEXT("Failed travel resumes unchanged inventory"), Equipment->IsEquipmentReady() &&
        Inventory->GetSnapshot().Revision == BeforeFailedTravel);
    TestEqual(TEXT("Failed travel never refills living body"), ASC->GetNumericAttribute(Health), 70.0f);

    // ---- 模拟确认意图 / 新身体，真实OpenLevel留实机；不是旧身体迁移ASC ----
    Session->bTravelPending = true;
    PC->Possess(RetryBody);
    RetryEquipment->RefreshEquipmentState();
    TestFalse(TEXT("New body waits pending"), RetryEquipment->IsEquipmentReady());
    TestTrue(TEXT("Old body cannot write inventory"), Equip(EETRelicSlot::First, NAME_None) == EResult::NotReady);
    TestEqual(TEXT("Old body keeps its physical max"), ASC->GetNumericAttribute(Max), 120.0f);
    Session->bTravelPending = false;
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::Retry);
    UAbilitySystemComponent* RetryASC = RetryBody->GetAbilitySystemComponent();
    TestTrue(TEXT("Retry body ready"), RetryEquipment->IsEquipmentReady());
    TestEqual(TEXT("Retry fills final max120"), RetryASC->GetNumericAttribute(Health), 120.0f);
    TestEqual(TEXT("Retry attack bonus restored"), RetryASC->GetNumericAttribute(Bonus), 5.0f);
    TestEqual(TEXT("Old body health still70"), ASC->GetNumericAttribute(Health), 70.0f);
    RetryASC->SetNumericAttributeBase(Health, 90.0f);
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::Continue);
    TestEqual(TEXT("Same body revision refresh does not refill"), RetryASC->GetNumericAttribute(Health), 90.0f);
    Session->bTravelPending = true;
    PC->Possess(NewRunBody);
    NewEquipment->RefreshEquipmentState();
    TestEqual(TEXT("Pending NewRun does not load previous gear"),
              NewRunBody->GetAbilitySystemComponent()->GetNumericAttribute(Max), 100.0f);
    Session->bTravelPending = false;
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::NewRun);
    TestTrue(TEXT("NewRun empty ready"), NewEquipment->IsEquipmentReady() &&
        Inventory->GetSnapshot().Equipped.SlotA.IsNone() && Inventory->GetSnapshot().Equipped.SlotB.IsNone());
    TestEqual(TEXT("NewRun final100"), NewRunBody->GetAbilitySystemComponent()->GetNumericAttribute(Health), 100.0f);

    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
    UAbilitySystemComponent* NewASC = NewRunBody->GetAbilitySystemComponent();
    NewASC->RemoveActiveGameplayEffect(NewEquipment->OwnedEffect);
    TestFalse(TEXT("Missing owned effect unavailable"), NewEquipment->IsEquipmentReady());
    const uint64 BeforeUnavailable = Inventory->GetSnapshot().Revision;
    TestTrue(TEXT("Missing handle refuses before model write"), NewEquipment->TryEquip(
        EETRelicSlot::First, LifeId, BeforeUnavailable).Result == EResult::NotReady);
    TestTrue(TEXT("Refusal preserves revision"), Inventory->GetSnapshot().Revision == BeforeUnavailable);
    Fixture.EndPlayInTestWorld();
    TestFalse(TEXT("End unsubscribes inventory"), NewEquipment->InventoryChangedHandle.IsValid());
    TestFalse(TEXT("End unsubscribes travel failure"), NewEquipment->TravelFailedHandle.IsValid());
    TestFalse(TEXT("End invalidates body handle"), RetryEquipment->OwnedEffect.IsValid());
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 7. 正常Build与索引

1. 确认一新 / 九旧文件全部保存，UE保持关闭。先在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

2. 脚本成功后在CLion Reload CMake / Refresh，Core新测试属于正确目标。预计Core72 / UI23 / Editor3仅作辅助；实际UBT结果为准。继续保持UE关闭，执行Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

3. 两个Build成功再打开原uproject。不要使用Live Coding代替这次Exec反射声明的正常构建。若失败，给我**第一条真正的error及文件 / 行号**；末尾UBT exit6不够定位。不要清缓存、删旧功能或反复生成VS文件来碰运气。

4. 三正式身体继承组件和旧能力配置都沿用C2-01，不加新组件、不改GE / GA资产。若UE提示资产待重存，Compile→Save并只纳入实际差异；不要求无差异资产再做一轮Validate。

## 8. 自动化：四项各一次

保持非PIE状态，打开Output Log，逐条执行，一项完成再下一项：

```text
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
Automation RunTests EmberTrial.M3.Inventory.EquipmentOwnership
Automation RunTests EmberTrial.M3.Inventory.Session
Automation RunTests EmberTrial.M2.Combo.Flow
```

四项应Success。新专项覆盖真实GE / 库存提交、属性与领域事件顺序、Busy / 过期 / 重复 / 未拥有拒绝、活体夹限、失活不回血、Pending / 失败恢复、旧身体隔离、Retry / NewRun语义和结束清理。旧Ownership检查句柄、外部同类效果隔离；Session检查模型；Combo.Flow检查整链真实伤害与快照。前一章刚过、实现未改的其他测试沿用证据，不跑29项全矩阵。

失败保留日志并停止该批后续验收。不要删失败断言或把Expected改成当前错误值；把第一条Expected及源码行给我。四项Success后再进入下节。

## 9. 分组真实检查（约10–15分钟）

只使用既有场景 / 菜单与PIE控制台，不创建临时蓝图函数或按键。成功日志由我读取，你只记录结果和大致完整时分。

### 9.1 存活装备、真实伤害与拒绝

1. 打开正式`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，Play并确认生命100/100。避开敌人攻击，到训练目标附近；不要改AI、默认生命或遗物数据。
2. 点击PIE视口获得输入，按反引号`~`打开**游戏控制台**。输入下列一行后Enter，再按`~`关闭控制台。后面的命令均用同样方式逐条执行；不要把四行一次粘进输入栏。若键位不起作用，用现有打开游戏控制台的方法；Output Log是查看结果的位置，不保证在非PIE上下文执行Exec能路由到当前Pawn。

```text
ET_Relic 0
```

应有`Ready=1 A=None B=None Health=100.0/100.0 Bonus=0.0`。查询不改变Revision；Result=1只是查询沿用NoChange，不表示失败。

3. 逐条输入：

```text
ET_Relic 1 life_ember
ET_Relic 2 keen_edge
```

应分别Success（Result=0），生命保持100、上限120，最终Bonus=5、A=life_ember / B=keen_edge。正式HUD显示100/120；左键单次命中**尚存活的训练目标**应真实扣30，例如100→70，三段倍率仍1。敌人原伤害10不变；不要把其他敌人同时受击的日志当成本次目标读数。

4. 输入下面两条拒绝命令，再查询；两槽、版本、生命 / Bonus均不改变：

```text
ET_Relic 2 life_ember
ET_Relic 2 missing
ET_Relic 0
```

Result=4为RuleRejected；具体Rule由日志识别即可，不要求背枚举数字。槽B不能重复槽A的同一件，也不能装备未拥有ID。

5. 依次卸下 / 装回生命遗物，观察100/120→100/100→100/120，无免费治疗：

```text
ET_Relic 1 None
ET_Relic 1 life_ember
```

若期间被敌人打伤，当前生命应保持实际伤后值，仅超过新上限才夹限；不用为了凑100反复重跑。Current与Base精确夹限由自动化负责。

6. P暂停，在游戏控制台尝试`ET_Relic 1 None`后关闭控制台并用原菜单继续；应拒绝、恢复后不补执行。查询仍为原两件。控制台关闭时可能先归还控制台焦点，再使用P / 菜单既有路径；不要把控制台未关闭造成的输入不动当作装备卡键。

### 9.2 新身体Retry、菜单Continue与NewRun

1. 保持两件已装备，使用原数字4真实伤害入口或敌人致死。失败页Retry后，查询应仍两件、**新身体120/120、Bonus5**；体力100、治疗3、F就绪，旧HUD / 锁定不残留。无需刻意等敌人完成整场战斗。
2. 此时卸下生命遗物再装回。因为是同一身体，应120/120→100/100→100/120。再通过P返回主菜单→**继续本次试炼**；新身体应回**120/120**且Bonus5。读取装备的是本进程GI，不是旧Pawn或旧GE迁移。
3. P返回菜单→**新试炼**。进入后查询应两槽None、生命100/100、Bonus0；本进程拥有两件仍在，但未装备。NewRun应等确认后才清逻辑装备，不允许新身体先加载旧装备再短暂裁血。
4. 左键原25、2 / E / P一种继续方式与移动 / 转向 / 跳跃做一次短查；保留原100默认值，不修改Blueprint启动属性来模拟装备。
5. Stop，再Play查询一次新GI空槽 / 100 / Bonus0；Stop。Save All，恢复常用正式地图。没有临时节点 / 资源参数需要清理；如果自己改了测试配置，先恢复再报告。

## 10. 简短反馈

```text
M3-02C2-02 完成 / 卡在第__节第__步
一新 / 九旧Source；Editor Build / 三索引 / Game Build：
EquipmentTransactions / EquipmentOwnership / Session / Combo.Flow各一次：
真实装备100/120与Bonus5 / 左键30；拒绝无变化 / 活体卸装不回血：
P内拒绝无补执行；Retry新120 / Continue新120与资源默认：
NewRun与Stop新GI空槽100 / 原25与短基线：
Source / 必要重存资产Save All；失败或不理解处（无填无）：
完成时分（完整时分）/ 手动代码 / 阅读配置 / 检查 / 构建等待：
```

成功日志我读取；错误优先文字，界面 / HUD位置或控件不明才补截图。未实施阶段的完整候选只做静态审查，实际Build / Success以你的执行为准。

## 11. 本批 Commit / Push（固定放在最后）

本地HEAD `5462f49`已有C2-01实现；本轮起点工作区干净，远端未重新核验。当前助手只更新下列13份Markdown，没有改Source / Config / Content / AssetManifest / Tools / Git索引：


```text
Docs/Reports/M3_02C2_01_Verification.md
Docs/M3_02C2_02_EquipmentTransactions.md
Docs/M3_02C2_01_EquipmentEffectOwnership.md
Docs/M3_02C1_SessionInventory.md
Docs/M3_Roadmap.md
Docs/Progress.md
Docs/Backlog.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
Docs/M2_Roadmap.md
README.md
Introduction.md
```

- 当前文档建议英文：`docs: verify equipment ownership and prepare equipment transactions`
- 中文译文：`docs: 记录装备效果所有权验收并准备换装事务与身体恢复`

**你完成本章后**，Stage下面一新 / 九旧Source和当时最新文档；无差异文件无需重复加入：


```text
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Public/Flow/ETRunSessionSubsystem.h
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicInventorySubsystem.h
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp
```

本章没有新资产 / Config。UE确实产生合法重存差异时，只纳入实际变化的原三身体和关联来源记录，不强制重存：

```text
EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset
EmberTrial/Content/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy.uasset
EmberTrial/Content/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget.uasset
Docs/AssetManifest.csv（仅资产字节确有变化且已同步哈希时）
```

- 实现建议英文：`feat: commit relic loadouts and restore equipment on new bodies`
- 中文译文：`feat: 提交遗物配装并在新角色身体上恢复装备效果`

CLion Refresh Changes→Stage最新修改（Stage后又改要再次Stage）→Review Diff→Commit / Push由你操作。Raw候选、日志、缓存、Trace和包留本地，不Force Add。确认差异是本章文件，再提交；助手不替你Stage / Commit / Push。
