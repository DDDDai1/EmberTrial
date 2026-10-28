# M3-02C1：会话库存、版本方案与切图确认

2026-10-28，固定UE **5.5.4**。[M3-02B已验收](Reports/M3_02B_Verification.md)。**本指南待你手动实现；助手只写文档，未将下列代码写入运行工程，也未编译 / 运行C1。**预计手动代码50–80分钟、检查5–10分钟，Build / 索引等待另记。第3–6节全部完成、保存后再正常Build。

## 1. 本批得到什么

创建由GameInstance管理的进程内库存：启动配置给出两件主资产ID，加载 / 校验后拥有两件、两槽空。预览读取当前快照并产生“来源库存 + 版本 + 前后方案”，不会装备GE。只有未来身体装备组件能调用私有逻辑提交，旧版本 / 别的库存 / 默认空方案都会拒绝；广播期间重入写拒绝，读与预览可用。

RunSession新增**到达确认事件**。NewRun真正到达后恢复初始拥有 / 清空槽位；Retry / Continue / MainMenu保留拥有 / 槽位。四种确认都推进版本，使旧世界方案过期；请求失败不会提前重置库存。关闭进程 / Stop结束GI后清空，不写磁盘。

| 本批 | 边界 |
| --- | --- |
| 三新Source | InventorySubsystem.h / cpp与一个长期自动化 |
| 两旧Source | RunSession.h / cpp，只增加确认事件声明、清理和成功广播 |
| 一Config | DefaultGame.ini新增启动库存主资产ID，不改扫描 / Cook规则 |
| 不新增资产 / Graph | 无组件挂载、输入、Widget或临时Controller验收函数 |
| 实机预期 | 原100 / 25保持；尚无实际换装入口，不期待120 / 30 |

当前169 Source / 93cpp、Core67 / UI23 / Editor3、26测试；实施后预计**172 / 95、Core69 / UI23 / Editor3、27测试**，Content296 / 来源77保持。

## 2. 所有权与提交边界

- **配置**只存`ETRelic:稳定ID`，不硬编码DataAsset文件路径；实际扫描仍由M3-01 AssetManager配置负责。启动时只同步加载这两份小定义，图标仍为软引用、不加载。后续大量掉落资源再按需求做异步加载，不给每次预览重复Load。
- **库存**以UPROPERTY持有定义防GC，只保存稳定ID / 版本 / 两槽；不持有World / Actor / ASC / Widget / GE句柄。定义查询只读，快照是副本。
- **A的纯计算**继续负责资格与数值预览；本批不复制一套算法。成功方案不是提交成功；NoChange / 失败输出清空，不可复用旧Plan。
- **版本**是同一库存内的单调令牌，来源弱引用隔离别的GI；提交前再按当前定义重算。它只解决逻辑前态过期，**不证明当前身体还活着 / 仍在祭坛，也不保证GAS事务原子性**。这些由C2装备服务补齐。
- **事件**由RunSession发布切图已确认事实，Inventory自己订阅；没有反向Inventory依赖。Deinitialize只移除自己持有的Handle，不Clear别人的观察者。
- **C2时序约束**：新身体BeginPlay可能早于PostLoadMap确认。未来恢复服务遇到`IsTravelPending()`必须等确认 / 失败处理后再读取库存，避免NewRun身体误用上轮装备；不能在Actor BeginPlay无条件套用旧两槽。

## 3. 创建三个文件

保存资产并关闭UE，CLion继续打开整个EmberTrial工程。新建以下目录 / 文件（已有Inventory目录复用），**不是在UE中创建Actor类**：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Inventory\ETRelicInventorySubsystem.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Inventory\ETRelicInventorySubsystem.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicInventorySessionTests.cpp
```

Public / Private各一份同名h / cpp，测试在Core的Tests。不要生成UI模块测试、把cpp放到Public，或手建generated.h；新头中它必须是最后一个include。无需修改Build.cs / Target / uproject、已有GE / Character或属性默认值。

## 4. 三份完整代码

### 4.1 ETRelicInventorySubsystem.h

第3节Public路径，完整写入：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ETRelicLoadout.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/PrimaryAssetId.h"
#include "ETRelicInventorySubsystem.generated.h"

class UETRelicDefinition;
class UETRelicInventorySubsystem;
class UETRunSessionSubsystem;
enum class EETRunTravelIntent : uint8;

// 领域快照只复制稳定ID与版本；定义对象、身体效果和页面选择不在其中。
struct EMBERTRIAL_API FETRelicInventorySnapshot
{
    bool bReady = false;
    uint64 Revision = 0;
    TArray<FName> OwnedIds;
    FETRelicLoadout Equipped;
};

enum class EETRelicInventoryResult : uint8
{
    Success,
    NoChange,
    NotReady,
    Busy,
    RuleRejected,
    InvalidPlan,
    StalePlan
};

struct EMBERTRIAL_API FETRelicInventoryReply
{
    EETRelicInventoryResult Result = EETRelicInventoryResult::NotReady;
    EETRelicLoadoutResult Rule = EETRelicLoadoutResult::Success;
};

// 只能由库存产生 / 装备服务提交；调用方不能修改其中的前态、版本和数值。
class EMBERTRIAL_API FETRelicInventoryPlan
{
public:
    const FETRelicLoadoutPlan& GetLoadoutPlan() const { return LoadoutPlan; }

private:
    friend class UETRelicInventorySubsystem;
    TWeakObjectPtr<UETRelicInventorySubsystem> Source;
    uint64 ExpectedRevision = 0;
    EETRelicSlot Slot = EETRelicSlot::First;
    FName RequestedId = NAME_None;
    float BaseMaxHealth = 0.0f;
    float BaseLightAttackDamage = 0.0f;
    FETRelicLoadoutPlan LoadoutPlan;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FETRelicInventoryChanged, const FETRelicInventorySnapshot&);

// 当前GI内的逻辑库存；不保存Actor / ASC / Widget / GE句柄，也不写磁盘。
UCLASS(Config = Game, DefaultConfig)
class EMBERTRIAL_API UETRelicInventorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ---- 生命周期与只读入口；预览不改变逻辑槽位或身体属性 ----
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    FETRelicInventorySnapshot GetSnapshot() const;
    const UETRelicDefinition* FindDefinition(FName RelicId) const;
    FETRelicInventoryReply TryBuildPlan(EETRelicSlot Slot, FName RequestedId,
                                      float BaseMaxHealth, float BaseLightAttackDamage,
                                      FETRelicInventoryPlan& OutPlan) const;

    FETRelicInventoryChanged OnChanged;

private:
    // C2的身体装备服务才有提交权限；UI只有读 / 预览，不可直接提交逻辑成功。
    friend class UETRelicEquipmentComponent;
    friend class FETRelicInventorySessionTest;

    bool InitializeDataFromConfig();
    ETRelicLoadoutRules::FDefinitionMap MakeDefinitionMap() const;
    FETRelicInventoryReply TryCommitPlan(const FETRelicInventoryPlan& Plan);
    void HandleTravelConfirmed(UWorld* LoadedWorld, EETRunTravelIntent Intent);
    void ApplyConfirmedIntent(EETRunTravelIntent Intent);
    void AdvanceRevision();
    void PublishSnapshot() const;

    // ---- 静态启动配置与GC所有权；只在GI初始化时同步加载这两个小定义 ----
    UPROPERTY(Config)
    TArray<FPrimaryAssetId> StartingRelics;

    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UETRelicDefinition>> Definitions;

    // ---- 当前进程状态；列表按SortOrder / 稳定ID排序，集合负责资格查询 ----
    TArray<FName> StartingOwnedIds;
    TArray<FName> OwnedIds;
    TSet<FName> OwnedSet;
    FETRelicLoadout Equipped;
    uint64 Revision = 0;
    bool bReady = false;
    bool bChanging = false;
    bool bClosing = false;

    // 只解除本实例的委托；不使RunSession反向依赖Inventory。
    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    FDelegateHandle TravelConfirmedHandle;
};
```

### 4.2 ETRelicInventorySubsystem.cpp

第3节Private路径，完整写入：

```cpp
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
FETRelicInventoryReply UETRelicInventorySubsystem::TryCommitPlan(const FETRelicInventoryPlan& Plan)
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

    // 先提交再同步广播；回调可以读 / 预览，但重入写请求会得到Busy。
    TGuardValue<bool> ChangeGuard(bChanging, true);
    Equipped = A.After;
    AdvanceRevision();
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
```

### 4.3 ETRelicInventorySessionTests.cpp

第3节Tests路径，完整写入。自动化复用真实配置 / 注册的两份定义，只在内存中提交逻辑模型；不改DataAsset、不生成World、不向正式角色施加GE：

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicInventorySubsystem.h"
#include "Engine/GameInstance.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicInventorySessionTest, "EmberTrial.M3.Inventory.Session",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETRelicInventorySessionTest::RunTest(const FString& Parameters)
{
    using EResult = EETRelicInventoryResult;
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    // 只构造内存GI / 模型，不启动World，不向正式身体或磁盘资产写值。
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGameInstance> OtherGI(NewObject<UGameInstance>());
    TStrongObjectPtr<UETRelicInventorySubsystem> Inventory(
        NewObject<UETRelicInventorySubsystem>(GI.Get()));
    TStrongObjectPtr<UETRelicInventorySubsystem> Other(
        NewObject<UETRelicInventorySubsystem>(OtherGI.Get()));
    if (!TestTrue(TEXT("Real configured catalog loads"), Inventory->InitializeDataFromConfig()) ||
        !TestTrue(TEXT("Independent model loads"), Other->InitializeDataFromConfig()))
    {
        return false;
    }
    auto Snapshot = Inventory->GetSnapshot();
    TestTrue(TEXT("Ready version1 empty slots"), Snapshot.bReady && Snapshot.Revision == 1 &&
             Snapshot.Equipped.SlotA.IsNone() && Snapshot.Equipped.SlotB.IsNone());
    TestEqual(TEXT("Two starting owned IDs"), Snapshot.OwnedIds.Num(), 2);
    TestTrue(TEXT("Both configured IDs present"), Snapshot.OwnedIds.Contains(LifeId) &&
             Snapshot.OwnedIds.Contains(EdgeId));
    TestNotNull(TEXT("Definition cached"), Inventory->FindDefinition(LifeId));

    int32 Events = 0;
    const FDelegateHandle Observer = Inventory->OnChanged.AddLambda(
        [&](const FETRelicInventorySnapshot&) { ++Events; });
    auto Build = [&](EETRelicSlot Slot, FName Id, FETRelicInventoryPlan& Out)
    {
        return Inventory->TryBuildPlan(Slot, Id, 100.0f, 25.0f, Out).Result;
    };
    FETRelicInventoryPlan LifePlan;
    FETRelicInventoryPlan CompetingPlan;
    TestTrue(TEXT("Life preview accepted"), Build(EETRelicSlot::First, LifeId, LifePlan) == EResult::Success);
    TestTrue(TEXT("Second plan at same version"),
             Build(EETRelicSlot::Second, EdgeId, CompetingPlan) == EResult::Success);
    TestEqual(TEXT("Preview health120"), LifePlan.GetLoadoutPlan().AfterPreview.MaxHealth, 120.0f);
    TestTrue(TEXT("Preview leaves model unchanged"), Inventory->GetSnapshot().Equipped.SlotA.IsNone());
    TestEqual(TEXT("Preview emits no event"), Events, 0);

    // ---- 来源 / 版本 / NoChange；只提交模型，不宣称已给身体装备GE ----
    TestTrue(TEXT("Foreign model cannot commit"), Other->TryCommitPlan(LifePlan).Result == EResult::InvalidPlan);
    TestTrue(TEXT("Default plan rejected"),
             Inventory->TryCommitPlan(FETRelicInventoryPlan()).Result == EResult::InvalidPlan);
    TestTrue(TEXT("Logical commit accepted"), Inventory->TryCommitPlan(LifePlan).Result == EResult::Success);
    TestEqual(TEXT("One commit one event"), Events, 1);
    TestTrue(TEXT("Other model isolated"), Other->GetSnapshot().Equipped.SlotA.IsNone());
    TestTrue(TEXT("Competing old revision rejected"),
             Inventory->TryCommitPlan(CompetingPlan).Result == EResult::StalePlan);
    FETRelicInventoryPlan Scratch;
    TestTrue(TEXT("Same item is NoChange"), Build(EETRelicSlot::First, LifeId, Scratch) == EResult::NoChange);
    TestTrue(TEXT("NoChange has no reusable plan"),
             Inventory->TryCommitPlan(Scratch).Result == EResult::InvalidPlan);
    const auto Rejected = Inventory->TryBuildPlan(
        EETRelicSlot::Second, FName(TEXT("missing")), 100.0f, 25.0f, Scratch);
    TestTrue(TEXT("Unowned returns domain and rule"), Rejected.Result == EResult::RuleRejected &&
             Rejected.Rule == EETRelicLoadoutResult::NotOwned);
    TestEqual(TEXT("Refusals do not emit changes"), Events, 1);

    // ---- 同步观察者读取提交后快照，重入写得到Busy；不是递归二次提交 ----
    EResult NestedResult = EResult::Success;
    bool bReadCommitted = false;
    const FDelegateHandle Reentrant = Inventory->OnChanged.AddLambda(
        [&](const FETRelicInventorySnapshot& Changed)
        {
            bReadCommitted = Changed.Equipped.SlotB == EdgeId &&
                             Inventory->GetSnapshot().Revision == Changed.Revision;
            FETRelicInventoryPlan Nested;
            if (Build(EETRelicSlot::Second, NAME_None, Nested) == EResult::Success)
            {
                NestedResult = Inventory->TryCommitPlan(Nested).Result;
            }
        });
    TestTrue(TEXT("Fresh edge plan"), Build(EETRelicSlot::Second, EdgeId, Scratch) == EResult::Success);
    TestTrue(TEXT("Second logical commit"), Inventory->TryCommitPlan(Scratch).Result == EResult::Success);
    TestTrue(TEXT("Observer sees committed state"), bReadCommitted);
    TestTrue(TEXT("Nested commit blocked"), NestedResult == EResult::Busy);
    TestEqual(TEXT("No nested extra event"), Events, 2);
    Inventory->OnChanged.Remove(Reentrant);

    // ---- 模拟确认意图测试模型语义；真实PostLoad委托由短实机另查 ----
    for (const EETRunTravelIntent Intent : {EETRunTravelIntent::MainMenu,
                                           EETRunTravelIntent::Continue,
                                           EETRunTravelIntent::Retry})
    {
        TestTrue(TEXT("Plan before world transition"),
                 Build(EETRelicSlot::Second, NAME_None, Scratch) == EResult::Success);
        const uint64 BeforeRevision = Inventory->GetSnapshot().Revision;
        Inventory->ApplyConfirmedIntent(Intent);
        Snapshot = Inventory->GetSnapshot();
        TestTrue(TEXT("Preserved IDs; advanced revision"), Snapshot.Revision > BeforeRevision &&
                 Snapshot.Equipped.SlotA == LifeId && Snapshot.Equipped.SlotB == EdgeId);
        TestTrue(TEXT("Old world plan invalidated"),
                 Inventory->TryCommitPlan(Scratch).Result == EResult::StalePlan);
    }
    const uint64 BeforeNone = Inventory->GetSnapshot().Revision;
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::None);
    TestTrue(TEXT("No confirmed intent changes nothing"), Inventory->GetSnapshot().Revision == BeforeNone);
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::NewRun);
    Snapshot = Inventory->GetSnapshot();
    TestTrue(TEXT("NewRun keeps starting ownership, clears slots"), Snapshot.OwnedIds.Num() == 2 &&
             Snapshot.Equipped.SlotA.IsNone() && Snapshot.Equipped.SlotB.IsNone());

    Inventory->OnChanged.Remove(Observer);
    TestTrue(TEXT("Plan before end"), Build(EETRelicSlot::First, LifeId, Scratch) == EResult::Success);
    Inventory->Deinitialize();
    TestFalse(TEXT("End revokes readiness"), Inventory->GetSnapshot().bReady);
    TestTrue(TEXT("End clears ID snapshot"), Inventory->GetSnapshot().OwnedIds.IsEmpty());
    TestTrue(TEXT("End refuses old plan"), Inventory->TryCommitPlan(Scratch).Result == EResult::NotReady);
    Other->Deinitialize();
    return !HasAnyErrors();
}

#endif
```

## 5. RunSession只增加三个位置

以下均编辑已有文件，**保留全部检查点、Pending、失败回调与旧注释 / 日志**。不要覆盖成一个只包含新代码的RunSession。

### 5.1 Header：新增成功委托类型与成员

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Flow\ETRunSessionSubsystem.h`。

①找到完整的`enum class EETRunTravelIntent : uint8`，在它的结束`};`后、`// 当前进程的地图 / 检查点快照`之前插入：

```cpp
// 只有本GI、目标地图与出生报告确认后才发布；失败请求不发布成功。
DECLARE_MULTICAST_DELEGATE_TwoParams(FETRunTravelConfirmed, UWorld*, EETRunTravelIntent);
```

必须放在enum定义**之后**；保留原generated.h位置，不在include区插入普通声明。②在public原两条委托成员后增加一条，最终这一小组为：

```cpp
    FETRunTravelFailed OnTravelFailed;
    FETRunCheckpointChanged OnCheckpointChanged;
    FETRunTravelConfirmed OnTravelConfirmed;
```

### 5.2 CPP：结束时清理新委托

打开`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Flow\ETRunSessionSubsystem.cpp`。在`Deinitialize()`找到：

```cpp
    OnTravelFailed.Clear();
    OnCheckpointChanged.Clear();
```

在其后增加`OnTravelConfirmed.Clear();`，其余代码保持。Inventory会自行移除自己的Handle；Session结束时Clear自己发布的委托是最后清理，不取代订阅者的解绑。

### 5.3 CPP：确认完成后广播，失败仍走旧分支

找到`HandlePostLoadMap(UWorld* LoadedWorld)`末尾，原代码为：

```cpp
    if (!bConfirmed)
    {
        OnTravelFailed.Broadcast(
            LoadedWorld, FText::FromString(TEXT("未确认试炼地图与出生配置，请检查后重试。")));
    }
```

**只替换这段if**为下方代码，不替换上面的`ResetPendingTravel()`与Travel arrived日志，不改前面的`bConfirmed`计算 / 旧快照提交：

```cpp
    if (bConfirmed)
    {
        OnTravelConfirmed.Broadcast(LoadedWorld, CompletedIntent);
    }
    else
    {
        OnTravelFailed.Broadcast(
            LoadedWorld, FText::FromString(TEXT("未确认试炼地图与出生配置，请检查后重试。")));
    }
```

`CompletedIntent`必须仍是在ResetPendingTravel之前保存的局部变量；**不要改为PendingIntent**，它此时已被清为None。新事件不加到BeginTravel / RequestNewRun，也不从NotifyGameplayWorldReady提前发出。

## 6. 配置两件初始拥有项

打开`E:\UE_Project\Demo\EmberTrial\Config\DefaultGame.ini`，确认UE仍关闭。现有包装 / AssetManager段全部保留，在文件末尾新增以下独立段，**只新增一次**：

```ini
[/Script/EmberTrial.ETRelicInventorySubsystem]
+StartingRelics=ETRelic:life_ember
+StartingRelics=ETRelic:keen_edge
```

段名包含Core模块名`EmberTrial`和类名去掉U前缀；两行前面的`+`是ini数组语法，需要保留。值为M3-01正式两份定义的PrimaryAssetId，不是资产文件名、显示中文或`/Game/...`路径。已核对本机5.5.4的PrimaryAssetId文本导入支持这一形式。

无需在Project Settings里再填一套，也不编辑Saved/Config。若后续日志提示ID未注册，先核对拼写与既有扫描配置 / Definitions结果，不在Inventory中调用AddDynamicAsset / ScanPaths来掩盖配置缺失。本批没有“实际装备”默认值，初始两槽始终空。

## 7. 正常Build与三项自动化

### 7.1 构建与索引

第3–6节全部完成并Save All。UE关闭，在PowerShell依次执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本完成正常Editor Build并刷新索引数据；CLion Reload CMake / Refresh，检查新头 / cpp属于Core目标，UI / Editor目标仍在。预计cpp为Core69 / UI23 / Editor3，数字仅辅助定位，不代替编译。继续保持UE关闭，执行Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

成功后正常启动原uproject。没有新Blueprint需要Reparent或Add Component；Subsystem随GI自动创建，三身体与Controller配置不变。不要用Live Coding替代这次新增反射类 / Config的正常Build。

### 7.2 三项相关自动化，各一次

保持非PIE，Output Log每次只执行一行，等明确Success再执行下一行：

```text
Automation RunTests EmberTrial.M3.Inventory.Session
Automation RunTests EmberTrial.M3.Inventory.Definitions
Automation RunTests EmberTrial.M3.Inventory.LoadoutRules
```

Session查真实配置加载、ID / 快照、预览无副作用、私有模型提交、来源 / 版本 / 重入拒绝、三种保留意图 / NewRun重置和结束撤销。测试直接模拟确认意图，只证明模型语义；实际RunSession成功通知由第8节短实机补查。Definitions查扫描 / 注册，LoadoutRules查纯函数旧契约。

本批不动Character / GA / GE，不重跑B四项或全部26项。配置应为正式两个ID；如果Session开头失败，后面不强行继续，贴第一条错误与失败行给我。若主日志中出现测试内多份Inventory ready属夹具对象，不算正式PIE重复创建；第8节再看同一真实Inventory实例。

## 8. 一次真实菜单往返（5–10分钟内）

### 8.1 初始化与NewRun确认

1. 在Content Browser打开`Content/EmberTrial/Maps`，双击`L_ET_MainMenu`地图；其完整资源路径是`/Game/EmberTrial/Maps/L_ET_MainMenu`。PIE使用当前菜单地图启动，不新建或修改地图配置。
2. Output Log搜索`M3-02C1`，应有本GI一条`Inventory ready`，`Owned=2 A=None B=None Revision=1`。记不住对象名无需抄日志，由我验收时读取。
3. 点击“开始新试炼”（NewRun），进入正式地图；看到原玩家HUD生命 / 体力100、治疗3与F就绪。日志应在原`Travel arrived ... Confirmed=1`之后出现同一Inventory的`Travel confirmed ... Intent=1 ... Revision=2`。本批始终空槽，不期待120 / 30。

### 8.2 返回菜单、Continue与再次新试炼

1. 按P打开暂停，点击返回主菜单。原有流程保持；库存确认日志`Intent=4`，Owned仍2、槽位仍None，版本继续增加。
2. 菜单点“继续本次试炼”，原流程回检查点 / 新身体，库存日志`Intent=3`，仍同一Inventory、版本再增加。短查左键原25、2、E提示、P一种继续方式、移动 / 转向 / 跳跃即可；不用重复多Pawn / 死亡矩阵。
3. 再P返回菜单，点击“开始新试炼”，日志`Intent=1`推进版本，空槽与初始拥有2保持。C1实机没有换装入口，所以**这里只验证事件与版本，不能声称真实身体装备跨图恢复成功**。
4. Stop；新PIE再开一次，日志重新Inventory ready / Revision1、原资源正常；然后Stop，恢复你常用的正式地图并Save All。新GI不继承旧版本 / ID状态；测试没有修改资产，不要求重新保存全部DataAsset。

Intent数字来自既有enum：1=NewRun、2=Retry、3=Continue、4=MainMenu。Retry模型语义已有专项，此批不专门再打一次死亡；真实带装备Retry / Continue与新身体恢复留C2一次集中验证。没有帧时间点 / 控制台掐时要求，也不加临时Blueprint函数。

## 9. 简短反馈

```text
M3-02C1 完成 / 卡在第__节第__步
三新 / 两旧Source与DefaultGame.ini；Editor Build / 三索引 / Game Build：
Inventory.Session / Definitions / LoadoutRules各一次：
真实PIE Owned2 / 空槽 / Revision1；NewRun确认后版本推进：
P返回菜单 / Continue / 再NewRun，同一库存与版本日志：
Stop新GI从1开始 / 原100与25、左键-2-E-P与基础输入短查：
Source / Config保存，恢复常用地图；失败或不理解处（无填无）：
完成时分 / 手动代码 / 检查 / 构建等待：
```

成功长日志不用复制；报错贴第一条Error文字 / 文件行号，节点或界面定位问题再给截图。日志我读取。若只有“Inventory unavailable”，同时说明是否完成第6节、是否正常关闭重开UE；不先删除缓存或修改原资产ID。

## 10. 本批 Commit / Push（固定放在最后）

本轮在B实物完成后新增验收报告 / C1指南并同步入口。**Source是你刚录入的完整实现，助手本轮未改运行代码、Config、资产、索引或重跑构建 / 测试。**本地HEAD `156835b` 的五个B新增文件仍为空壳；请重新Stage下面12份完整源码，再Review Diff，不能仅提交文档。未联网核验远端。

```text
EmberTrial/Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h
EmberTrial/Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h
EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp
EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp
```

当前同批文档共14份（含上次尚未提交的A报告 / B指南与入口）：

```text
Docs/Reports/M3_02A_Verification.md
Docs/Reports/M3_02B_Verification.md
Docs/M3_02A_LoadoutRules.md
Docs/M3_02B_CombatEquipmentEffects.md
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

当前共26份实际待提交文件；建议一并提交完整B实现与交接文档：

- 英文：`feat: add GAS equipment modifiers and light attack snapshots`
- 中文译文：`feat: 新增GAS装备属性修饰与轻攻击快照`

若完整B源码另行提交，后续纯文档建议英文`docs: verify equipment effects and prepare session inventory`，中文译文`docs: 记录装备效果验收并准备会话库存`；不要先留空壳源码、只提交“验收通过”报告。

**C1由你实施后**，再将以下六份实际变化与当时最新验收 / 入口文档一起Stage；这些不是本轮已经实现的文件：

```text
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicInventorySubsystem.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicInventorySessionTests.cpp
EmberTrial/Source/EmberTrial/Public/Flow/ETRunSessionSubsystem.h
EmberTrial/Source/EmberTrial/Private/Flow/ETRunSessionSubsystem.cpp
EmberTrial/Config/DefaultGame.ini
```

- 英文：`feat: add session inventory and versioned loadout plans`
- 中文译文：`feat: 新增会话库存与带版本校验的装备方案`

CLion Refresh Changes → Stage最新内容（已Stage后又改必须再次Stage）→ Review Diff → Commit / Push由你操作。B与C1均无计划资产修改；旧资产无差异不重复Stage，AssetManifest保持77行。Raw候选 / 日志 / 缓存 / 索引生成物 / Trace / 包留本地，不Force Add。
