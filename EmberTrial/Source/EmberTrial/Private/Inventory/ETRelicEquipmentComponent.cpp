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
// ---- 只读方案预检：比较与提交共用规则，但读操作不要求Idle / 非暂停 ----
FETRelicInventoryReply UETRelicEquipmentComponent::TryMakePlan(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision,
    FETRelicInventoryPlan& OutPlan) const
{
    OutPlan = {};
    if (bChanging)
    {
        return {EETRelicInventoryResult::Busy};
    }
    UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadBody(Body, ASC) || Body->IsCombatDead() || !IsEquipmentReady() || !IsValid(Inventory))
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
    FETRelicInventoryPlan Candidate;
    const FETRelicInventoryReply Reply = Inventory->TryBuildPlan(
        Slot, RelicId, BaseMax, BaseDamage, Candidate);
    if (Reply.Result != EETRelicInventoryResult::Success)
    {
        return Reply;
    }
    const FETRelicModifierTotals& Totals = Candidate.GetLoadoutPlan().AfterModifiers;
    if (!AreModifiersValid(ASC, Totals) || !IsFinalDamageValid(Totals))
    {
        return {EETRelicInventoryResult::RuleRejected, EETRelicLoadoutResult::InvalidFinalValues};
    }
    OutPlan = Candidate;
    return Reply;
}

FETRelicInventoryReply UETRelicEquipmentComponent::TryBuildPreview(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision,
    FETRelicLoadoutPlan& OutPreview) const
{
    OutPreview = {};
    FETRelicInventoryPlan Plan;
    const FETRelicInventoryReply Reply = TryMakePlan(Slot, RelicId, ExpectedRevision, Plan);
    if (Reply.Result == EETRelicInventoryResult::Success)
    {
        OutPreview = Plan.GetLoadoutPlan();
    }
    return Reply;
}

// ---- 写入仍要求当前身体Idle / 非暂停；只读预览不能绕过确认的版本重验 ----
FETRelicInventoryReply UETRelicEquipmentComponent::TryEquip(
    const EETRelicSlot Slot, const FName RelicId, const uint64 ExpectedRevision)
{
    UETRelicInventorySubsystem* Inventory = InventorySource.Get();
    if (bChanging || (IsValid(Inventory) && Inventory->bChanging))
    {
        return {EETRelicInventoryResult::Busy};
    }
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    if (!IsValid(Body) || !Body->IsFormalActionIdle() || UGameplayStatics::IsGamePaused(this))
    {
        return {EETRelicInventoryResult::NotReady};
    }
    FETRelicInventoryPlan Plan;
    const FETRelicInventoryReply Reply = TryMakePlan(Slot, RelicId, ExpectedRevision, Plan);
    if (Reply.Result != EETRelicInventoryResult::Success)
    {
        return Reply;
    }
    // 所有业务拒绝均已完成。提交回调保持C2-02协议，不先Apply再伪造回滚。
    return Inventory->TryCommitPlan(Plan,
        [this](const FETRelicLoadoutPlan& Committed, const uint64 Revision)
        {
            TGuardValue<bool> ChangeGuard(bChanging, true);
            AppliedLoadout = Committed.After;
            AppliedRevision = Revision;
            WriteModifiersUnchecked(Committed.AfterModifiers);
        });
}