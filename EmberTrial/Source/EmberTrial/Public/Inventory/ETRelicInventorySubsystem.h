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