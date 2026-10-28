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