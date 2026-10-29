#include "Inventory/ETRelicInventorySubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "Inventory/ETRelicDefinition.h"
#include "Logging/LogMacros.h"
#include "Subsystems/SubsystemCollection.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicInventory, Log, All);

// ---- GI初始化与精确解绑；依赖声明保证RunSession先初始化、后结束 ----
void UETRelicInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UETRunSessionSubsystem* RunSession = Collection.InitializeDependency<UETRunSessionSubsystem>();
    Session = RunSession;
    if (IsValid(RunSession))
    {
        TravelConfirmedHandle = RunSession->OnTravelConfirmed.AddUObject(
            this, &ThisClass::HandleTravelConfirmed);
    }
    InitializeDataFromConfig();
}

void UETRelicInventorySubsystem::Deinitialize()
{
    bClosing = true;
    bReady = false;
    AdvanceRevision();
    if (UETRunSessionSubsystem* RunSession = Session.Get())
    {
        RunSession->OnTravelConfirmed.Remove(TravelConfirmedHandle);
    }
    TravelConfirmedHandle.Reset();
    Session.Reset();
    OnChanged.Clear();
    Definitions.Reset();
    StartingOwnedIds.Reset();
    OwnedIds.Reset();
    OwnedSet.Reset();
    Equipped = {};
    UE_LOG(LogETRelicInventory, Display, TEXT("[M3-02C1] Inventory end Inventory=%s"),
           *GetNameSafe(this));
    Super::Deinitialize();
}

// 配置无效时保持整体不可用，不留下半份拥有集合，也不在此补注册主资产。
bool UETRelicInventorySubsystem::InitializeDataFromConfig()
{
    UAssetManager* Manager = UAssetManager::GetIfInitialized();
    if (bReady || bClosing)
    {
        return false;
    }
    if (!IsValid(Manager) || StartingRelics.IsEmpty())
    {
        UE_LOG(LogETRelicInventory, Warning,
               TEXT("[M3-02C1] Inventory unavailable: manager / starting config missing"));
        return false;
    }

    TArray<TStrongObjectPtr<UETRelicDefinition>> Loaded;
    TArray<const UETRelicDefinition*> Catalog;
    for (const FPrimaryAssetId& Id : StartingRelics)
    {
        if (!Id.IsValid() || Id.PrimaryAssetType != UETRelicDefinition::AssetType)
        {
            UE_LOG(LogETRelicInventory, Warning,
                   TEXT("[M3-02C1] Invalid starting primary ID=%s"), *Id.ToString());
            return false;
        }
        const FSoftObjectPath Path = Manager->GetPrimaryAssetPath(Id);
        UETRelicDefinition* Definition = Cast<UETRelicDefinition>(Path.TryLoad());
        if (!IsValid(Definition) || Definition->GetPrimaryAssetId() != Id)
        {
            UE_LOG(LogETRelicInventory, Warning,
                   TEXT("[M3-02C1] Starting relic not registered / loadable ID=%s Path=%s"),
                   *Id.ToString(), *Path.ToString());
            return false;
        }
        // 加载下一项前就取得GC所有权；最终再交给UPROPERTY Map。
        Loaded.Emplace(Definition);
        Catalog.Add(Definition);
    }
    TArray<FText> Errors;
    if (!UETRelicDefinition::CollectCatalogErrors(Catalog, Errors))
    {
        for (const FText& Error : Errors)
        {
            UE_LOG(LogETRelicInventory, Warning, TEXT("[M3-02C1] Invalid starting catalog: %s"),
                   *Error.ToString());
        }
        return false;
    }

    Catalog.Sort([](const UETRelicDefinition& A, const UETRelicDefinition& B)
    {
        return A.GetSortOrder() != B.GetSortOrder()
                   ? A.GetSortOrder() < B.GetSortOrder()
                   : A.GetRelicId().LexicalLess(B.GetRelicId());
    });
    for (const auto& Definition : Loaded)
    {
        Definitions.Add(Definition->GetRelicId(), Definition.Get());
    }
    for (const UETRelicDefinition* Definition : Catalog)
    {
        const FName Id = Definition->GetRelicId();
        StartingOwnedIds.Add(Id);
        OwnedSet.Add(Id);
    }
    OwnedIds = StartingOwnedIds;
    Equipped = {};
    AdvanceRevision();
    bReady = true;
    UE_LOG(LogETRelicInventory, Display,
           TEXT("[M3-02C1] Inventory ready Inventory=%s Owned=%d A=None B=None Revision=%llu"),
           *GetNameSafe(this), OwnedIds.Num(), static_cast<unsigned long long>(Revision));
    return true;
}

// ---- 快照与预览；所有权与定义只借给本次纯计算，不逐次加载资源 ----
FETRelicInventorySnapshot UETRelicInventorySubsystem::GetSnapshot() const
{
    FETRelicInventorySnapshot Snapshot;
    Snapshot.bReady = bReady && !bClosing;
    Snapshot.Revision = Revision;
    if (Snapshot.bReady)
    {
        Snapshot.OwnedIds = OwnedIds;
        Snapshot.Equipped = Equipped;
    }
    return Snapshot;
}

const UETRelicDefinition* UETRelicInventorySubsystem::FindDefinition(const FName RelicId) const
{
    const TObjectPtr<UETRelicDefinition>* Found = Definitions.Find(RelicId);
    return bReady && !bClosing && Found ? Found->Get() : nullptr;
}

ETRelicLoadoutRules::FDefinitionMap UETRelicInventorySubsystem::MakeDefinitionMap() const
{
    ETRelicLoadoutRules::FDefinitionMap Borrowed;
    for (const auto& Pair : Definitions)
    {
        Borrowed.Add(Pair.Key, Pair.Value.Get());
    }
    return Borrowed;
}

FETRelicInventoryReply UETRelicInventorySubsystem::TryBuildPlan(
    const EETRelicSlot Slot, const FName RequestedId, const float BaseMaxHealth,
    const float BaseLightAttackDamage, FETRelicInventoryPlan& OutPlan) const
{
    OutPlan = {};
    if (!bReady || bClosing)
    {
        return {EETRelicInventoryResult::NotReady};
    }
    FETRelicLoadoutPlan Computed;
    const EETRelicLoadoutResult Rule = ETRelicLoadoutRules::BuildReplacementPlan(
        Equipped, OwnedSet, MakeDefinitionMap(), Slot, RequestedId,
        BaseMaxHealth, BaseLightAttackDamage, Computed);
    if (Rule != EETRelicLoadoutResult::Success)
    {
        return {Rule == EETRelicLoadoutResult::NoChange ? EETRelicInventoryResult::NoChange
                                                     : EETRelicInventoryResult::RuleRejected,
                Rule};
    }
    OutPlan.Source = const_cast<UETRelicInventorySubsystem*>(this);
    OutPlan.ExpectedRevision = Revision;
    OutPlan.Slot = Slot;
    OutPlan.RequestedId = RequestedId;
    OutPlan.BaseMaxHealth = BaseMaxHealth;
    OutPlan.BaseLightAttackDamage = BaseLightAttackDamage;
    OutPlan.LoadoutPlan = Computed;
    return {EETRelicInventoryResult::Success};
}

// ---- 私有逻辑提交；C2还必须先核对身体 / 世界与效果，不能把它直接暴露给UI ----
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

// ---- 确认切图后推进会话；请求 / 失败不提前重置，新世界不能沿用旧方案 ----
void UETRelicInventorySubsystem::HandleTravelConfirmed(UWorld* LoadedWorld,
                                                       const EETRunTravelIntent Intent)
{
    if (IsValid(LoadedWorld) && LoadedWorld->GetGameInstance() == GetGameInstance())
    {
        ApplyConfirmedIntent(Intent);
    }
}

void UETRelicInventorySubsystem::ApplyConfirmedIntent(const EETRunTravelIntent Intent)
{
    if (!bReady || bClosing || Intent == EETRunTravelIntent::None)
    {
        return;
    }
    TGuardValue<bool> ChangeGuard(bChanging, true);
    if (Intent == EETRunTravelIntent::NewRun)
    {
        OwnedIds = StartingOwnedIds;
        OwnedSet.Reset();
        for (const FName Id : OwnedIds)
        {
            OwnedSet.Add(Id);
        }
        Equipped = {};
    }
    AdvanceRevision();
    UE_LOG(LogETRelicInventory, Display,
           TEXT("[M3-02C1] Travel confirmed Inventory=%s Intent=%d Owned=%d A=%s B=%s Revision=%llu"),
           *GetNameSafe(this), static_cast<int32>(Intent), OwnedIds.Num(),
           *Equipped.SlotA.ToString(), *Equipped.SlotB.ToString(),
           static_cast<unsigned long long>(Revision));
    PublishSnapshot();
}

void UETRelicInventorySubsystem::AdvanceRevision()
{
    if (++Revision == 0)
    {
        ++Revision;
    }
}

void UETRelicInventorySubsystem::PublishSnapshot() const
{
    const FETRelicInventorySnapshot Snapshot = GetSnapshot();
    OnChanged.Broadcast(Snapshot);
}