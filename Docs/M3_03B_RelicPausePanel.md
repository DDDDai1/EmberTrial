# M3-03B：暂停页中的遗物列表、详情与只读比较

2026-10-29，UE **5.5.4**。[M3-03A已验收](Reports/M3_03A_Verification.md)。本章主体C++由你手动输入，Designer按步骤装配；后续助手直接处理的检查与补修见对应章节记录。

## 1. 完成本章后看到什么

按P打开原暂停页，左侧仍是继续 / 返回菜单，右侧新增遗物面板：真实两件拥有项、已装备槽位徽记、选中名称 / 说明、槽A / B比较按钮与数值变化。默认生息余烬预览最大生命100→120，选择砺锋残片预览首段轻击25→30。切换选择与比较槽位不会装备、扣血或治疗。

本页是正式可复用的只读浏览界面，复用原暂停Flow的UIOnly、鼠标、焦点和恢复输入；本章没有第二个暂停所有者。后续安全祭坛接入确认换装，当前按钮只叫“比较槽A / B”，不调用TryEquip。ListView复用条目时解绑旧Item；Presenter按稳定ID保留数据Item身份，不把条目Widget当物品。

范围：**七新UI / 五旧Source**（Core组件h / cpp、Character.h专项friend与Restart声明、Character.cpp换控后恢复、UI Build.cs的Slate依赖）；新建两Widget，修改原WBP_PauseMenu布局。无新输入 / Config / 地图 / 美术下载；字体沿用现有中文链。预计代码90–130分钟、Designer30–45分钟、检查5–10分钟，等待另记，可分两次完成；全部源码保存后统一Build。

当前179 Source / 100cpp（Core72 / UI25 / Editor3）、30测试、296 Content / 来源77。实施后预计**186 Source / 104cpp（Core72 / UI29 / Editor3）、31测试**；两新Widget使Content298，来源预计79，实际SHA由我下次验收更新，不提前编造。

## 2. 生命周期与边界

| 所有者 | 负责内容 |
| --- | --- |
| 原FailureFlow / PauseWidget | 世界暂停、初始继续按钮焦点、P关闭、恢复 / 返回菜单；原业务保持 |
| RelicPanelWidget | 页面期间强持有一份Presenter，Construct订阅 / Destruct清理，渲染和只读选择意图 |
| RelicPresenter | 弱观察当前本地Pawn / 库存 / ASC / 装备事件，按稳定ID复用Item、生成只读比较 |
| ListEntryWidget | 绑定当前Item、立即取快照；EntryReleased / Destruct解绑、清旧文字与选中色 |
| RelicEquipmentComponent | 写保护撤销后发布装备状态完成通知；不反向依赖UI |

装备两属性更新期间，Presenter忽略临时属性事件，等待完成通知再统一刷新，避免短暂Waiting和重复展示。初次零幅度恢复也有完成通知，不能只等属性数值变化。没有新Tick / 轮询Timer；UListView自身的Slate更新照常工作。

## 3. 五个旧源码改动

Save All，关闭UE，在CLion手动操作。以下路径相对`E:\UE_Project\Demo\EmberTrial\Source`。原测试 / Ready / 权限 / 数值逻辑保留。

### 3.1 EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h

事件类型在前置声明区域；事件成员和批更新查询放原public区域。generated.h仍是最后一条include。

**原生装备观察事件类型**。找到原文：

```cpp
class UETRunSessionSubsystem;
```

保留原文，在其后插入：

```cpp
class UETRelicEquipmentComponent;

// 通知本组件控制下的装备状态已变化；消费者重新查询，不使用旧事件值作为权威。
DECLARE_MULTICAST_DELEGATE_OneParam(FETRelicEquipmentChanged, UETRelicEquipmentComponent*);
```

**public事件成员与批更新查询**。找到原文：

```cpp
    void RefreshEquipmentState();
```

保留原文，在其后插入：

```cpp
    bool IsUpdatingEquipment() const { return bChanging; }
    FETRelicEquipmentChanged OnEquipmentChanged;
```

### 3.2 EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp

替换下面四个函数，其他函数不改。广播在本次bChanging作用域结束后；Release先让身份失效，再释放 / 通知，空身份不重复通知。原提交点与零幅度GE仍沿用。

**UETRelicEquipmentComponent::TrySetModifiers**。只替换该函数的完整实现，返回类型至配对的最后`}`；相邻函数保留：

```cpp
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

    {
        TGuardValue<bool> ChangeGuard(bChanging, true);
        WriteModifiersUnchecked(Modifiers);
    }
    OnEquipmentChanged.Broadcast(this);
    // Applied表示已向自有GE发出更新；同步回调可以继续改变世界，不在这里伪造回滚。
    return EETRelicEffectWriteResult::Applied;
}
```

**UETRelicEquipmentComponent::ReleaseOwnedEffect**。只替换该函数的完整实现，返回类型至配对的最后`}`；相邻函数保留：

```cpp
void UETRelicEquipmentComponent::ReleaseOwnedEffect()
{
    const bool bHadIdentity = OwnedEffect.IsValid() || AppliedRevision != 0;
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
    if (bHadIdentity)
    {
        OnEquipmentChanged.Broadcast(this);
    }
}
```

**UETRelicEquipmentComponent::RefreshEquipmentState**。只替换该函数的完整实现，返回类型至配对的最后`}`；相邻函数保留：

```cpp
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

    const bool bRefillNewBody = bInitialRestorePending;
    {
        TGuardValue<bool> ChangeGuard(bChanging, true);
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
    }
    UE_LOG(LogETRelicEquipment, Display,
           TEXT("[M3-02C2-02] Restore Body=%s Revision=%llu A=%s B=%s Initial=%d"),
           *GetNameSafe(GetOwner()), static_cast<unsigned long long>(Snapshot.Revision),
           *Snapshot.Equipped.SlotA.ToString(), *Snapshot.Equipped.SlotB.ToString(), bRefillNewBody ? 1 : 0);
    OnEquipmentChanged.Broadcast(this);
}
```

**UETRelicEquipmentComponent::TryEquip**。只替换该函数的完整实现，返回类型至配对的最后`}`；相邻函数保留：

```cpp
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
            {
                TGuardValue<bool> ChangeGuard(bChanging, true);
                AppliedLoadout = Committed.After;
                AppliedRevision = Revision;
                WriteModifiersUnchecked(Committed.AfterModifiers);
            }
            // 发布时bChanging已撤销，读者可立即读取真正就绪的快照。
            OnEquipmentChanged.Broadcast(this);
        });
}
```

### 3.3 EmberTrial/Public/Characters/EmberTrialCharacter.h

在原private测试friend区域增加一行；换控补修还需在生命周期声明区域添加Restart，见3.5。不把私有属性改public：

**UI专项friend**。找到原文：

```cpp
    friend class FETRelicEquipmentTransactionTest;
```

保留原文，在其后插入：

```cpp
    friend class FETRelicPresenterTest;
```

### 3.4 EmberTrialUI/EmberTrialUI.Build.cs

ListView公开引擎头直接使用Slate列表类型，因此在既有**PrivateDependencyModuleNames**内添加Slate；Core仍不依赖UI。数组其他项与Public依赖不变。

**ListView直接依赖Slate**。找到原文：

```cpp
            "SlateCore"
```

将这行替换为：

```cpp
            "Slate",
            "SlateCore"
```

### 3.5 换控后的装备恢复补修（2026-10-29，助手已直接修改）

运行中的身体已完成BeginPlay时，UE 5.5先执行`Pawn::PossessedBy`，再执行`Controller::SetPawn`。原角色在PossessedBy中刷新装备，当前Controller的Pawn尚未更新，因此`IsCurrentPlayerBody()`拒绝恢复。基础GameplayReady不会再次广播，Presenter会停在Waiting。不能在测试中手动Refresh或放宽身份检查来掩盖这个顺序问题。

在`EmberTrial/Public/Characters/EmberTrialCharacter.h`中，紧跟原`PossessedBy`声明增加：

```cpp
    virtual void Restart() override;
```

在`EmberTrial/Private/Characters/EmberTrialCharacter.cpp`中增加以下实现。Restart由引擎换控链调用，此时Controller已指向新Pawn；初次出生仍走原BeginPlay恢复，同一身体重复换控不会重新初始化属性或免费治疗。

```cpp
void AEmberTrialCharacter::Restart()
{
    Super::Restart();

    // PossessedBy早于Controller::SetPawn；Restart时双方身份才完整。
    // 初次出生仍由BeginPlay恢复；运行中换控在这里补查，不重新初始化属性。
    if (HasActorBegunPlay() && !bAbilitySystemShuttingDown && IsValid(RelicEquipmentComponent))
    {
        RelicEquipmentComponent->RefreshEquipmentState();
    }
}
```

## 4. 七个新UI源码文件

都在EmberTrialUI模块。用CLion普通文件创建，不用UE类向导；没有文件夹就创建文件夹，不手动创建.generated.h。

### 4.1 Presenter头

完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Inventory\ETRelicPresenter.h
```


```cpp
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ETRelicViewModels.h"
#include "UObject/Object.h"
#include "ETRelicPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETRelicEquipmentComponent;
class UETRelicInventorySubsystem;
class UETUILocalPlayerSubsystem;
struct FETRelicInventorySnapshot;
struct FOnAttributeChangeData;

enum class EETRelicPanelState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    Waiting,
    Ready
};
enum class EETRelicPreviewState : uint8
{
    Unavailable,
    Available,
    AlreadyEquipped,
    Rejected
};

// 原生View快照；不含世界对象或确认权限，不把库存版本转成Blueprint不支持的uint64属性。
struct EMBERTRIALUI_API FETRelicPanelData
{
    EETRelicPanelState State = EETRelicPanelState::Unavailable;
    EETRelicPreviewState PreviewState = EETRelicPreviewState::Unavailable;
    FName SelectedId = NAME_None;
    FText Name;
    FText Description;
    bool bSlotB = false;
    float BeforeMax = 0.0f;
    float AfterMax = 0.0f;
    float BeforeAttack = 0.0f;
    float AfterAttack = 0.0f;
    bool operator==(const FETRelicPanelData& Other) const;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FETRelicPanelChanged, const FETRelicPanelData&);

// 页面作用域的只读Presenter：观察数据源、复用稳定ID的Item、保存页面选择。
UCLASS()
class EMBERTRIALUI_API UETRelicPresenter : public UObject
{
    GENERATED_BODY()

public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();
    bool SelectId(FName Id);
    bool SetPreviewSlotB(bool bSecondSlot);
    bool SetQuery(const FETRelicListQuery& NewQuery);
    const FETRelicPanelData& GetSnapshot() const { return Snapshot; }
    const TArray<TObjectPtr<UETRelicListItem>>& GetItems() const { return VisibleItems; }
    UETRelicListItem* FindItem(FName Id) const;
    FETRelicPanelChanged OnChanged;

protected:
    virtual void BeginDestroy() override;

private:
    friend class FETRelicPresenterTest;
    void StopObserving(bool bNotify);
    void ClearBodyBindings();
    void BindCurrentPawn();
    void Refresh();
    void ResolveInventory();
    FETRelicPanelData BuildData() const;

    UFUNCTION()
    void HandlePawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);
    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);
    void HandleInventoryChanged(const FETRelicInventorySnapshot& Data,
                                TWeakObjectPtr<UETRelicInventorySubsystem> Source);
    void HandleEquipmentChanged(UETRelicEquipmentComponent* Source);
    void HandleAttributeChanged(const FOnAttributeChangeData& Data,
                               TWeakObjectPtr<UAbilitySystemComponent> Source);

    // 页面强持有Item；底层世界 / 库存只有弱引用和本实例的订阅Handle。
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UETRelicListItem>> ItemsById;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UETRelicListItem>> VisibleItems;
    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<UETRelicInventorySubsystem> BoundInventory;
    TWeakObjectPtr<AEmberTrialCharacter> BoundBody;
    TWeakObjectPtr<UETRelicEquipmentComponent> BoundEquipment;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle InventoryHandle;
    FDelegateHandle EquipmentHandle;
    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHandle;
    FDelegateHandle BonusHandle;
    FETRelicListQuery Query;
    FETRelicPanelData Snapshot;
    FName SelectedId = NAME_None;
    uint64 ObservedRevision = 0;
    bool bSlotB = false;
    bool bObserving = false;
    bool bRefreshing = false;
};
```

### 4.2 Presenter实现

完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Inventory\ETRelicPresenter.cpp
```


```cpp
#include "Inventory/ETRelicPresenter.h"
#include "Abilities/ETGA_LightAttack.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Templates/UnrealTemplate.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicPresenter, Log, All);

bool FETRelicPanelData::operator==(const FETRelicPanelData& Other) const
{
    return State == Other.State && PreviewState == Other.PreviewState && SelectedId == Other.SelectedId &&
           Name.EqualTo(Other.Name) && Description.EqualTo(Other.Description) && bSlotB == Other.bSlotB &&
           BeforeMax == Other.BeforeMax && AfterMax == Other.AfterMax &&
           BeforeAttack == Other.BeforeAttack && AfterAttack == Other.AfterAttack;
}

// ---- 先订阅再取快照；重启保留页面Item身份，Shutdown释放页面持有的数据 ----
void UETRelicPresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
{
    StopObserving(false);
    BoundSubsystem = Subsystem;
    bObserving = IsValid(Subsystem);
    if (bObserving)
    {
        Subsystem->OnObservedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandlePawnChanged);
        BindCurrentPawn();
    }
    else
    {
        Refresh();
    }
}

void UETRelicPresenter::Shutdown()
{
    StopObserving(true);
}

void UETRelicPresenter::BeginDestroy()
{
    StopObserving(false);
    Super::BeginDestroy();
}

void UETRelicPresenter::StopObserving(const bool bNotify)
{
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(this, &ThisClass::HandlePawnChanged);
    }
    if (UETRelicInventorySubsystem* Inventory = BoundInventory.Get())
    {
        Inventory->OnChanged.Remove(InventoryHandle);
    }
    InventoryHandle.Reset();
    BoundInventory.Reset();
    ClearBodyBindings();
    BoundSubsystem.Reset();
    ObservedRevision = 0;
    if (bNotify)
    {
        ItemsById.Reset();
        VisibleItems.Reset();
        SelectedId = NAME_None;
        const FETRelicPanelData Empty;
        const bool bChanged = !(Snapshot == Empty);
        Snapshot = Empty;
        if (bChanged)
        {
            const FETRelicPanelData Published = Snapshot;
            OnChanged.Broadcast(Published);
        }
    }
    else
    {
        Snapshot = {};
    }
}

void UETRelicPresenter::ClearBodyBindings()
{
    if (AEmberTrialCharacter* Body = BoundBody.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleReadyChanged);
    }
    if (UETRelicEquipmentComponent* Equipment = BoundEquipment.Get())
    {
        Equipment->OnEquipmentChanged.Remove(EquipmentHandle);
    }
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetMaxHealthAttribute()).Remove(MaxHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(UETCombatAttributeSet::GetLightAttackBonusAttribute()).Remove(BonusHandle);
    }
    HealthHandle.Reset();
    MaxHandle.Reset();
    BonusHandle.Reset();
    EquipmentHandle.Reset();
    BoundASC.Reset();
    BoundEquipment.Reset();
    BoundBody.Reset();
}

void UETRelicPresenter::ResolveInventory()
{
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    APlayerController* PC = Subsystem ? Subsystem->GetObservedController() : nullptr;
    UGameInstance* GI = IsValid(PC) && PC->GetWorld() ? PC->GetWorld()->GetGameInstance() : nullptr;
    UETRelicInventorySubsystem* Inventory = IsValid(GI) ? GI->GetSubsystem<UETRelicInventorySubsystem>() : nullptr;
    if (BoundInventory.Get() == Inventory && InventoryHandle.IsValid())
    {
        return;
    }
    if (UETRelicInventorySubsystem* Old = BoundInventory.Get())
    {
        Old->OnChanged.Remove(InventoryHandle);
    }
    BoundInventory = Inventory;
    InventoryHandle.Reset();
    if (Inventory)
    {
        InventoryHandle = Inventory->OnChanged.AddUObject(this, &ThisClass::HandleInventoryChanged, BoundInventory);
    }
}

void UETRelicPresenter::BindCurrentPawn()
{
    ClearBodyBindings();
    ResolveInventory();
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    AEmberTrialCharacter* Body = Subsystem ? Cast<AEmberTrialCharacter>(Subsystem->GetObservedPawn()) : nullptr;
    if (IsValid(Body))
    {
        BoundBody = Body;
        Body->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleReadyChanged);
        BoundEquipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
        if (UETRelicEquipmentComponent* Equipment = BoundEquipment.Get())
        {
            EquipmentHandle = Equipment->OnEquipmentChanged.AddUObject(this, &ThisClass::HandleEquipmentChanged);
        }
        BoundASC = Body->GetAbilitySystemComponent();
        if (UAbilitySystemComponent* ASC = BoundASC.Get())
        {
            HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetHealthAttribute())
                .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC);
            MaxHandle = ASC->GetGameplayAttributeValueChangeDelegate(UETHealthAttributeSet::GetMaxHealthAttribute())
                .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC);
            BonusHandle = ASC->GetGameplayAttributeValueChangeDelegate(UETCombatAttributeSet::GetLightAttackBonusAttribute())
                .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC);
        }
    }
    Refresh();
}

// ---- 行数据 / 页面选择；不创建Widget，不调用TryEquip，不初始化玩法资源 ----
UETRelicListItem* UETRelicPresenter::FindItem(const FName Id) const
{
    const TObjectPtr<UETRelicListItem>* Item = ItemsById.Find(Id);
    return Item ? Item->Get() : nullptr;
}

bool UETRelicPresenter::SelectId(const FName Id)
{
    if (bRefreshing || Id == SelectedId || !VisibleItems.Contains(FindItem(Id)))
    {
        return false;
    }
    SelectedId = Id;
    Refresh();
    return true;
}

bool UETRelicPresenter::SetPreviewSlotB(const bool bSecondSlot)
{
    if (bRefreshing || bSlotB == bSecondSlot)
    {
        return false;
    }
    bSlotB = bSecondSlot;
    Refresh();
    return true;
}

bool UETRelicPresenter::SetQuery(const FETRelicListQuery& NewQuery)
{
    if (bRefreshing || (Query.Filter == NewQuery.Filter && Query.Sort == NewQuery.Sort &&
                       Query.SearchText == NewQuery.SearchText))
    {
        return false;
    }
    Query = NewQuery;
    Refresh();
    return true;
}

void UETRelicPresenter::Refresh()
{
    if (bRefreshing)
    {
        return;
    }
    bool bItemsChanged = false;
    FETRelicPanelData Data;
    {
        TGuardValue<bool> Guard(bRefreshing, true);
        const TArray<TObjectPtr<UETRelicListItem>> PreviousItems = VisibleItems;
        UETRelicInventorySubsystem* Inventory = BoundInventory.Get();
        TArray<FETRelicListRow> AllRows;
        TArray<FETRelicListRow> VisibleRows;
        const FETRelicInventorySnapshot Captured = Inventory ? Inventory->GetSnapshot() : FETRelicInventorySnapshot{};
        if (bObserving && BoundSubsystem.IsValid() &&
            ETRelicPresentation::BuildRows(Captured, Inventory, {}, AllRows) == EETRelicRowsResult::Success &&
            ETRelicPresentation::BuildRows(Captured, Inventory, Query, VisibleRows) == EETRelicRowsResult::Success)
        {
            TMap<FName, TObjectPtr<UETRelicListItem>> Updated;
            for (const FETRelicListRow& Row : AllRows)
            {
                UETRelicListItem* Item = FindItem(Row.RelicId);
                if (!Item)
                {
                    Item = NewObject<UETRelicListItem>(this);
                }
                Updated.Add(Row.RelicId, Item);
                Item->AssignRow(Row);
            }
            ItemsById = MoveTemp(Updated);
            VisibleItems.Reset();
            for (const FETRelicListRow& Row : VisibleRows)
            {
                VisibleItems.Add(FindItem(Row.RelicId));
            }
            if (!VisibleItems.Contains(FindItem(SelectedId)))
            {
                SelectedId = VisibleItems.IsEmpty() ? NAME_None : VisibleItems[0]->GetSnapshot().RelicId;
            }
            ObservedRevision = Captured.Revision;
            Data = BuildData();
        }
        else
        {
            VisibleItems.Reset();
            ItemsById.Reset();
            SelectedId = NAME_None;
            ObservedRevision = 0;
        }
        bItemsChanged = PreviousItems != VisibleItems;
    }
    if (bItemsChanged || !(Snapshot == Data))
    {
        Snapshot = Data;
        UE_LOG(LogETRelicPresenter, Display, TEXT("[M3-03B] View State=%d Items=%d Selected=%s SlotB=%d Preview=%d"),
            static_cast<int32>(Data.State), VisibleItems.Num(), *Data.SelectedId.ToString(), Data.bSlotB ? 1 : 0,
            static_cast<int32>(Data.PreviewState));
        // 已结束组装，View可读稳定集合；渲染不向此处同步递归发选择意图。
        const FETRelicPanelData Published = Snapshot;
        OnChanged.Broadcast(Published);
    }
}

FETRelicPanelData UETRelicPresenter::BuildData() const
{
    FETRelicPanelData Data;
    Data.bSlotB = bSlotB;
    Data.SelectedId = SelectedId;
    if (UETRelicListItem* Item = FindItem(SelectedId))
    {
        const FETRelicListRow Row = Item->GetSnapshot();
        Data.Name = Row.DisplayName;
        Data.Description = Row.Description;
    }
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    APawn* Pawn = Subsystem ? Subsystem->GetObservedPawn() : nullptr;
    if (!IsValid(Pawn))
    {
        Data.State = EETRelicPanelState::NoPawn;
        return Data;
    }
    AEmberTrialCharacter* Body = BoundBody.Get();
    if (!Body || Pawn != Body)
    {
        Data.State = EETRelicPanelState::UnsupportedPawn;
        return Data;
    }
    UETRelicEquipmentComponent* Equipment = BoundEquipment.Get();
    Data.State = Body->IsGameplayReady() && Equipment && Equipment->IsEquipmentReady()
        ? EETRelicPanelState::Ready : EETRelicPanelState::Waiting;
    if (Data.State != EETRelicPanelState::Ready || SelectedId.IsNone())
    {
        return Data;
    }
    float CurrentHealth = 0.0f;
    float CurrentMax = 0.0f;
    if (!Body->GetGameplayHealthSnapshot(CurrentHealth, CurrentMax) || CurrentHealth <= 0.0f)
    {
        return Data;
    }
    FETRelicLoadoutPlan Preview;
    const auto Reply = Equipment->TryBuildPreview(bSlotB ? EETRelicSlot::Second : EETRelicSlot::First,
                                                 SelectedId, ObservedRevision, Preview);
    if (Reply.Result == EETRelicInventoryResult::Success)
    {
        Data.PreviewState = EETRelicPreviewState::Available;
        Data.BeforeMax = Preview.BeforePreview.MaxHealth;
        Data.AfterMax = Preview.AfterPreview.MaxHealth;
        Data.BeforeAttack = Preview.BeforePreview.LightAttackBaseDamage;
        Data.AfterAttack = Preview.AfterPreview.LightAttackBaseDamage;
    }
    else if (Reply.Result == EETRelicInventoryResult::NoChange)
    {
        float Health = 0.0f;
        float Bonus = 0.0f;
        const UETGA_LightAttack* Attack = Body->GetLightAttackConfiguration();
        if (Attack && Body->GetGameplayHealthSnapshot(Health, Data.BeforeMax) &&
            Body->GetGameplayLightAttackBonusSnapshot(Bonus) && UETGA_LightAttack::TryCalculateStageDamage(
                Attack->GetBaseAttackDamage(), Bonus, 1.0f, Data.BeforeAttack))
        {
            Data.PreviewState = EETRelicPreviewState::AlreadyEquipped;
            Data.AfterMax = Data.BeforeMax;
            Data.AfterAttack = Data.BeforeAttack;
        }
    }
    else if (Reply.Result == EETRelicInventoryResult::RuleRejected)
    {
        Data.PreviewState = EETRelicPreviewState::Rejected;
    }
    return Data;
}

// ---- 回调核对当前来源，忽略旧身体 / 旧库存的事件 ----
void UETRelicPresenter::HandlePawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    if (bObserving)
    {
        BindCurrentPawn();
    }
}

void UETRelicPresenter::HandleReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady)
{
    if (bObserving && BodyCharacter == BoundBody.Get())
    {
        Refresh();
    }
}

void UETRelicPresenter::HandleInventoryChanged(const FETRelicInventorySnapshot& Data,
    TWeakObjectPtr<UETRelicInventorySubsystem> Source)
{
    if (bObserving && Source.IsValid() && Source == BoundInventory)
    {
        Refresh();
    }
}

void UETRelicPresenter::HandleEquipmentChanged(UETRelicEquipmentComponent* Source)
{
    if (bObserving && Source == BoundEquipment.Get())
    {
        Refresh();
    }
}

void UETRelicPresenter::HandleAttributeChanged(const FOnAttributeChangeData& Data,
    TWeakObjectPtr<UAbilitySystemComponent> Source)
{
    // 自有GE更新期间不发布临时Waiting；完成通知会在写入保护撤销后统一刷新。
    const UETRelicEquipmentComponent* Equipment = BoundEquipment.Get();
    if (bObserving && Source.IsValid() && Source == BoundASC &&
        (!Equipment || !Equipment->IsUpdatingEquipment()))
    {
        Refresh();
    }
}
```

### 4.3 条目Widget头

完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETRelicListEntryWidget.h
```


```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "ETRelicListEntryWidget.generated.h"

class UBorder;
class UTextBlock;
class UETRelicListItem;
struct FETRelicListRow;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETRelicListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnEntryReleased() override;
    virtual void NativeDestruct() override;

private:
    void UnbindItem();
    void RenderRow(const FETRelicListRow& Row);
    void HandleRowChanged(const FETRelicListRow& Row, TWeakObjectPtr<UETRelicListItem> Source);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> SelectionBorder;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> EquippedText;
    TWeakObjectPtr<UETRelicListItem> BoundItem;
    FDelegateHandle ItemHandle;
};
```

### 4.4 条目Widget实现

完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETRelicListEntryWidget.cpp
```


```cpp
#include "Widgets/ETRelicListEntryWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Inventory/ETRelicViewModels.h"

void UETRelicListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    UnbindItem();
    BoundItem = Cast<UETRelicListItem>(ListItemObject);
    if (UETRelicListItem* Item = BoundItem.Get())
    {
        ItemHandle = Item->OnChanged.AddUObject(this, &ThisClass::HandleRowChanged, BoundItem);
        RenderRow(Item->GetSnapshot());
    }
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
}

void UETRelicListEntryWidget::NativeOnItemSelectionChanged(const bool bIsSelected)
{
    if (SelectionBorder)
    {
        SelectionBorder->SetBrushColor(bIsSelected ? FLinearColor(.22f, .16f, .07f, 1.0f)
                                                  : FLinearColor(.055f, .055f, .055f, 1.0f));
    }
    IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
}

void UETRelicListEntryWidget::NativeOnEntryReleased()
{
    UnbindItem();
    NativeOnItemSelectionChanged(false);
    IUserObjectListEntry::NativeOnEntryReleased();
}

void UETRelicListEntryWidget::NativeDestruct()
{
    UnbindItem();
    Super::NativeDestruct();
}

void UETRelicListEntryWidget::UnbindItem()
{
    if (UETRelicListItem* Item = BoundItem.Get())
    {
        Item->OnChanged.Remove(ItemHandle);
    }
    ItemHandle.Reset();
    BoundItem.Reset();
    RenderRow({});
}

void UETRelicListEntryWidget::HandleRowChanged(const FETRelicListRow& Row,
    TWeakObjectPtr<UETRelicListItem> Source)
{
    if (Source.IsValid() && Source == BoundItem)
    {
        RenderRow(Row);
    }
}

void UETRelicListEntryWidget::RenderRow(const FETRelicListRow& Row)
{
    if (ItemNameText)
    {
        ItemNameText->SetText(Row.DisplayName);
    }
    if (EquippedText)
    {
        EquippedText->SetText(FText::FromString(Row.bEquippedInSlotA ? TEXT("槽 A 已装备") :
            Row.bEquippedInSlotB ? TEXT("槽 B 已装备") : TEXT("未装备")));
        if (Row.RelicId.IsNone())
        {
            EquippedText->SetText(FText::GetEmpty());
        }
    }
}
```

### 4.5 面板Widget头

完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETRelicPanelWidget.h
```


```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETRelicPanelWidget.generated.h"

class UButton;
class UListView;
class UTextBlock;
class UETRelicPresenter;
struct FETRelicPanelData;

// 嵌入现有暂停页的只读浏览页，不拥有世界暂停 / 输入模式，也没有换装确认接口。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETRelicPanelWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    bool HasRequiredLayout() const;
    void Render(const FETRelicPanelData& Data);
    void HandleSelectionChanged(UObject* Item);
    UFUNCTION()
    void HandleSlotA();
    UFUNCTION()
    void HandleSlotB();

    UPROPERTY(Transient)
    TObjectPtr<UETRelicPresenter> Presenter;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UListView> RelicList;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> InventoryStatusText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailNameText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DetailDescriptionText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> PreviewText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SlotText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotAButton;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotBButton;
    FDelegateHandle PresenterHandle;
    FDelegateHandle SelectionHandle;
    bool bRendering = false;
};
```

### 4.6 面板Widget实现

完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETRelicPanelWidget.cpp
```


```cpp
#include "Widgets/ETRelicPanelWidget.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Inventory/ETRelicPresenter.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Templates/UnrealTemplate.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicPanel, Log, All);

bool UETRelicPanelWidget::HasRequiredLayout() const
{
    return RelicList && InventoryStatusText && DetailNameText && DetailDescriptionText && PreviewText &&
           SlotText && SlotAButton && SlotBButton;
}

void UETRelicPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!HasRequiredLayout())
    {
        UE_LOG(LogETRelicPanel, Warning, TEXT("[M3-03B] Missing required panel layout Widget=%s"), *GetName());
        return;
    }
    SlotAButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotA);
    SlotBButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotB);
    RelicList->OnItemSelectionChanged().Remove(SelectionHandle);
    SelectionHandle = RelicList->OnItemSelectionChanged().AddUObject(this, &ThisClass::HandleSelectionChanged);
    if (!Presenter)
    {
        Presenter = NewObject<UETRelicPresenter>(this);
    }
    Presenter->OnChanged.Remove(PresenterHandle);
    PresenterHandle = Presenter->OnChanged.AddUObject(this, &ThisClass::Render);
    ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    Presenter->StartObserving(LocalPlayer ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>() : nullptr);
    Render(Presenter->GetSnapshot());
    UE_LOG(LogETRelicPanel, Display, TEXT("[M3-03B] Panel subscribed Widget=%s"), *GetName());
}

void UETRelicPanelWidget::NativeDestruct()
{
    if (Presenter)
    {
        Presenter->OnChanged.Remove(PresenterHandle);
        Presenter->Shutdown();
    }
    PresenterHandle.Reset();
    if (RelicList)
    {
        RelicList->OnItemSelectionChanged().Remove(SelectionHandle);
        RelicList->ClearListItems();
    }
    SelectionHandle.Reset();
    if (SlotAButton)
    {
        SlotAButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlotA);
    }
    if (SlotBButton)
    {
        SlotBButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSlotB);
    }
    UE_LOG(LogETRelicPanel, Display, TEXT("[M3-03B] Panel unsubscribed Widget=%s"), *GetName());
    Super::NativeDestruct();
}

void UETRelicPanelWidget::HandleSelectionChanged(UObject* Item)
{
    if (!bRendering && Presenter)
    {
        if (const UETRelicListItem* Relic = Cast<UETRelicListItem>(Item))
        {
            Presenter->SelectId(Relic->GetSnapshot().RelicId);
        }
    }
}

void UETRelicPanelWidget::HandleSlotA()
{
    if (Presenter)
    {
        Presenter->SetPreviewSlotB(false);
    }
}

void UETRelicPanelWidget::HandleSlotB()
{
    if (Presenter)
    {
        Presenter->SetPreviewSlotB(true);
    }
}

// ---- 渲染不回发选择意图；只有集合 / 顺序改变才重建ListView的数据集合 ----
void UETRelicPanelWidget::Render(const FETRelicPanelData& Data)
{
    if (!HasRequiredLayout() || !Presenter || bRendering)
    {
        return;
    }
    TGuardValue<bool> Guard(bRendering, true);
    TArray<UObject*> Objects;
    for (UETRelicListItem* Item : Presenter->GetItems())
    {
        Objects.Add(Item);
    }
    if (RelicList->GetListItems() != Objects)
    {
        RelicList->SetListItems(Objects);
    }
    UETRelicListItem* Selected = Presenter->FindItem(Data.SelectedId);
    if (RelicList->GetSelectedItem() != Selected)
    {
        if (Selected)
        {
            RelicList->SetSelectedItem(Selected);
        }
        else
        {
            RelicList->ClearSelection();
        }
    }
    InventoryStatusText->SetText(FText::FromString(Data.State == EETRelicPanelState::Ready
        ? TEXT("遗物 · 只读查看") : TEXT("当前身体或装备尚不可预览")));
    DetailNameText->SetText(Data.SelectedId.IsNone() ? FText::FromString(TEXT("没有可显示的遗物")) : Data.Name);
    DetailDescriptionText->SetText(Data.Description);
    SlotText->SetText(FText::FromString(Data.bSlotB ? TEXT("比较目标：槽 B") : TEXT("比较目标：槽 A")));
    if (Data.PreviewState == EETRelicPreviewState::Available ||
        Data.PreviewState == EETRelicPreviewState::AlreadyEquipped)
    {
        const FNumberFormattingOptions NumberFormat = FNumberFormattingOptions().SetMaximumFractionalDigits(1);
        PreviewText->SetText(FText::Format(FText::FromString(TEXT("最大生命 {0} → {1}\n首段轻击 {2} → {3}{4}")),
            FText::AsNumber(Data.BeforeMax, &NumberFormat), FText::AsNumber(Data.AfterMax, &NumberFormat),
            FText::AsNumber(Data.BeforeAttack, &NumberFormat), FText::AsNumber(Data.AfterAttack, &NumberFormat),
            Data.PreviewState == EETRelicPreviewState::AlreadyEquipped
                ? FText::FromString(TEXT("\n该槽已装备此遗物")) : FText::GetEmpty()));
    }
    else
    {
        PreviewText->SetText(FText::FromString(Data.PreviewState == EETRelicPreviewState::Rejected
            ? TEXT("此方案不符合装备规则，例如同一遗物不能占两槽。") : TEXT("当前没有可用的属性比较。")));
    }
}
```

### 4.7 新Presenter自动化

完整路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Tests\ETRelicPresenterTests.cpp
```


```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicPresenter.h"
#include "Abilities/ETGA_LightAttack.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicPresenterTest, "EmberTrial.UI.Relics.Presenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicPresenterTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto SpawnBody = [&](float X)
    {
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(X, 0, 200), FRotator::ZeroRotator, Spawn);
        if (Body)
        {
            Body->CombatTeam = EETCombatTeam::Player;
            Body->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
        }
        return Body;
    };
    AEmberTrialCharacter* Body = SpawnBody(0);
    AEmberTrialCharacter* OtherBody = SpawnBody(400);
    APlayerController* PC = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Other body"), OtherBody) ||
        !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("Engine"), GEngine))
    {
        return false;
    }
    // 跟随已验证的UI身份夹具，不创建Viewport或额外初始化第二份GI。
    TStrongObjectPtr<ULocalPlayer> LocalPlayer(NewObject<ULocalPlayer>(GEngine));
    LocalPlayer->SwitchController(PC);
    PC->Possess(Body);
    TStrongObjectPtr<UETUILocalPlayerSubsystem> Subsystem(
        NewObject<UETUILocalPlayerSubsystem>(LocalPlayer.Get()));
    Subsystem->PlayerControllerChanged(PC);
    TStrongObjectPtr<UETRelicPresenter> Presenter(NewObject<UETRelicPresenter>());
    Presenter->StartObserving(Subsystem.Get());
    TestTrue(TEXT("Before body BeginPlay waits"), Presenter->GetSnapshot().State == EETRelicPanelState::Waiting);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Zero modifier restore still publishes final readiness"),
        Presenter->GetSnapshot().State == EETRelicPanelState::Ready &&
        Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::Available);
    TestEqual(TEXT("First life comparison120"), Presenter->GetSnapshot().AfterMax, 120.0f);
    TestEqual(TEXT("Two stable items"), Presenter->GetItems().Num(), 2);
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    UETRelicListItem* LifeItem = Presenter->FindItem(LifeId);
    UETRelicListItem* EdgeItem = Presenter->FindItem(EdgeId);
    UETRelicInventorySubsystem* Inventory = World->GetGameInstance()->GetSubsystem<UETRelicInventorySubsystem>();
    UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
    if (!TestNotNull(TEXT("Life item"), LifeItem) || !TestNotNull(TEXT("Edge item"), EdgeItem) ||
        !TestNotNull(TEXT("Inventory"), Inventory) || !TestNotNull(TEXT("Equipment"), Equipment))
    {
        return false;
    }
    int32 ViewEvents = 0;
    const FDelegateHandle ViewHandle = Presenter->OnChanged.AddLambda([&](const FETRelicPanelData&) { ++ViewEvents; });
    auto Equip = [&](EETRelicSlot Slot, FName Id)
    {
        return Equipment->TryEquip(Slot, Id, Inventory->GetSnapshot().Revision).Result;
    };
    TestTrue(TEXT("Real life commit"), Equip(EETRelicSlot::First, LifeId) == EETRelicInventoryResult::Success);
    TestEqual(TEXT("One settled view no temporary Waiting"), ViewEvents, 1);
    TestTrue(TEXT("Badge changes on same item"), Presenter->FindItem(LifeId) == LifeItem &&
        LifeItem->GetSnapshot().bEquippedInSlotA);
    TestTrue(TEXT("Same slot now already equipped"),
        Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::AlreadyEquipped);
    TestTrue(TEXT("Select edge"), Presenter->SelectId(EdgeId));
    TestTrue(TEXT("Select slotB"), Presenter->SetPreviewSlotB(true));
    TestEqual(TEXT("Edge comparison30"), Presenter->GetSnapshot().AfterAttack, 30.0f);
    FETRelicListQuery Query;
    Query.Sort = EETRelicListSort::StableId;
    Presenter->SetQuery(Query);
    TestTrue(TEXT("Resort preserves selected ID and both Item objects"),
        Presenter->GetSnapshot().SelectedId == EdgeId && Presenter->FindItem(LifeId) == LifeItem &&
        Presenter->FindItem(EdgeId) == EdgeItem);
    Presenter->StartObserving(Subsystem.Get());
    TestTrue(TEXT("Restart same presenter retains item identity"), Presenter->FindItem(EdgeId) == EdgeItem);
    TestTrue(TEXT("Real edge commit"), Equip(EETRelicSlot::Second, EdgeId) == EETRelicInventoryResult::Success);
    TestEqual(TEXT("Committed preview current30"), Presenter->GetSnapshot().BeforeAttack, 30.0f);

    Equipment->Deactivate();
    TestTrue(TEXT("Deactivate unavailable comparison"),
        Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::Unavailable);
    Equipment->Activate(false);
    TestTrue(TEXT("Reactivate final readiness"), Presenter->GetSnapshot().State == EETRelicPanelState::Ready);
    UAbilitySystemComponent* OldASC = Body->GetAbilitySystemComponent();
    PC->UnPossess();
    TestTrue(TEXT("NoPawn clears comparison"), Presenter->GetSnapshot().State == EETRelicPanelState::NoPawn &&
        Presenter->GetSnapshot().AfterMax == 0.0f);
    const int32 BeforeOldBody = ViewEvents;
    OldASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 75.0f);
    TestEqual(TEXT("Old body cannot drive current presenter"), ViewEvents, BeforeOldBody);
    PC->Possess(OtherBody);
    TestTrue(TEXT("New body restored and observed"), Presenter->GetSnapshot().State == EETRelicPanelState::Ready &&
        Presenter->BoundBody.Get() == OtherBody);
    // 只走真实Possess链，不让测试手动Refresh掩盖换控后的装备恢复遗漏。
    UAbilitySystemComponent* OtherASC = OtherBody->GetAbilitySystemComponent();
    TestEqual(TEXT("New body restores equipped maximum"),
        OtherASC->GetNumericAttribute(UETHealthAttributeSet::GetMaxHealthAttribute()), 120.0f);
    TestEqual(TEXT("New body receives initial equipped refill"),
        OtherASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 120.0f);
    TestEqual(TEXT("New body exposes equipped attack preview"), Presenter->GetSnapshot().BeforeAttack, 30.0f);
    PC->Possess(Body);
    TestTrue(TEXT("Returning body observed without manual refresh"),
        Presenter->GetSnapshot().State == EETRelicPanelState::Ready && Presenter->BoundBody.Get() == Body);
    TestEqual(TEXT("Returning body does not refill health"),
        OldASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 75.0f);
    PC->Possess(OtherBody);
    TestTrue(TEXT("Second switch remains ready"),
        Presenter->GetSnapshot().State == EETRelicPanelState::Ready && Presenter->BoundBody.Get() == OtherBody);
    Presenter->Shutdown();
    TestTrue(TEXT("Shutdown releases displayed items"), Presenter->GetItems().IsEmpty() &&
        Presenter->GetSnapshot().State == EETRelicPanelState::Unavailable);
    const int32 BeforeStopped = ViewEvents;
    OtherBody->GetAbilitySystemComponent()->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 80.0f);
    TestEqual(TEXT("Stopped presenter receives no source events"), ViewEvents, BeforeStopped);
    Presenter->OnChanged.Remove(ViewHandle);
    TestFalse(TEXT("Inventory handle removed"), Presenter->InventoryHandle.IsValid());
    TestFalse(TEXT("Equipment handle removed"), Presenter->EquipmentHandle.IsValid());
    Subsystem->Deinitialize();
    Fixture.EndPlayInTestWorld();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

Presenter的新测试不创建Viewport / 屏幕Widget：复用已验证的LocalPlayer身份夹具、真实GI库存和ASC，覆盖先观察后Ready、零幅度恢复通知、单次最终发布、Item身份、重启 / 换Pawn / 失活与结束。实际Slate条目和键鼠在第9节短查。

## 5. 正常Editor / Game Build与索引

1. 第3–4节完成保存，UE保持关闭，PowerShell：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

2. CLion Reload CMake / Refresh。新七文件属于UI模块，预计Core72 / UI29 / Editor3；实际UBT成功为准。继续保持UE关闭：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

3. 两目标成功后打开原uproject。新增反射Widget / Presenter用正常Build，不用Live Coding替代；错误发第一条error / 路径 / 行号。不要为UI头找不到而把UI加入Core依赖，新测试必须在UI模块。

## 6. 创建列表条目WBP_ETRelicListEntry

### 6.1 先完成Designer结构与名称

1. Content Browser到`/Game/EmberTrial/ui`，新建Inventory文件夹。右键→User Interface→Widget Blueprint，先用默认UserWidget，命名**WBP_ETRelicListEntry**。
2. 打开Designer。若默认有Canvas Panel，删除这个空根控件（当前是新资产，没有旧业务），从Palette放**Size Box**作为根，命名EntrySize；Width Override不勾，Height Override勾选并填**64**。
3. 拖**Border**到EntrySize作为唯一子项，命名**SelectionBorder**，勾Is Variable。Details中将**Content → Padding**四个方向设为8；**Slot (Size Box Slot) → Padding**四个方向保持0，避免把内边距填到外边距。Horizontal / Vertical Alignment=Fill；Brush Color初始R/G/B=.055、A=1，Content Color and Opacity保持白色。
4. 拖**Vertical Box**到SelectionBorder，再拖两个**Text Block**到Vertical Box。上方命名**ItemNameText**，下方**EquippedText**，两者勾Is Variable。字体Font Family沿用原暂停页StatusText能显示中文的同一设置，Size分别18 / 14；文字颜色白色 / 浅灰，Auto Wrap Text=True。
5. 两个Text的Vertical Box Slot：Size=Auto、Horizontal Alignment=Fill；下方文本Top Padding=4。Designer占位文字分别“遗物名称”“未装备”。Text右侧不要创建Binding；Graph不加OnListItemObjectSet业务。

```text
EntrySize (SizeBox，Height64)
  SelectionBorder (Border)
    VerticalBox
      ItemNameText (TextBlock)
      EquippedText (TextBlock)
```

**6.1 检查记录（2026-10-29）：**已在UE 5.5.4中核对控件层级、命名、变量勾选、EntrySize高度、边框颜色和两段文字自动换行。已将SelectionBorder的Slot Padding改为0、Content Padding改为8；将EquippedText的Top Padding改为4，文字从白色调整为浅灰（线性RGB约0.749、Alpha 1）。两个文字当前使用Roboto Bold、字号18 / 14；本次未对照暂停页字体资产，运行时中文显示仍按后续验收检查。该Widget已编译并保存；列表中的实际布局和交互仍需在面板完成后验收。

### 6.2 设置正确原生父类

1. 三个命名控件完成后，Class Settings→Reparent Blueprint，搜索并选择**ETRelicListEntryWidget**。核对Parent Class指向`/Script/EmberTrialUI.ETRelicListEntryWidget`，不要选择Panel父类。
2. Class Defaults：Is Focusable=False，Tick Frequency=Never。继承的UserObjectListEntry接口由原生父类提供，不再手工添加另一份接口 / 蓝图事件。
3. Compile→Save。三个BindWidget名称 / 类型必须准确，不能用Text代替Border；required widget binding与Native Tick警告均应消失。若有残留，优先查Is Variable和父类，不靠改控件显示文字修复绑定。

## 7. 创建WBP_ETRelicPanel

### 7.1 根容器与上方标题

1. 同一目录新建默认UserWidget，命名**WBP_ETRelicPanel**。Designer删除空Canvas根，放SizeBox作根，命名PanelSize，勾Width Override=**620**、Height Override=**460**。
2. PanelSize唯一子项Border，命名PanelBorder；Details中**Content → Padding**四方向均为16，**Slot (Size Box Slot) → Padding**四方向保持0。Brush Color=(.035,.035,.035,1)，Content Color保持白色，Horizontal / Vertical Alignment=Fill。
3. Border内放VerticalBox。依次放静态TitleText（“遗物”，Font24）、**InventoryStatusText**（勾Is Variable，Font16 / Auto Wrap True）、HorizontalBox，命名BodyColumns。
4. Title / Status的Vertical Box Slot均Auto / Fill；Status的Top Padding=6、Bottom=12。BodyColumns的Vertical Box Slot设**Fill 1** / Fill，使列表获得确定可用高度。

**7.1 检查记录（2026-10-29）：**已核对PanelSize的620×460、控件层级、PanelBorder颜色、TitleText字号24、Status的自动换行与上下间距。修正PanelBorder外边距为0、内部Padding为16；BodyColumns由Auto改为Fill 1；为InventoryStatusText勾选Is Variable，并将字号从12改为16。当前UserWidget父类下编译通过并保存；7.2的列表与详情尚未创建，7.3再设置原生父类并验证全部绑定。运行时整体布局和中文显示需在后续验收中检查。

### 7.2 左列表和右详情

1. 从Palette搜索**List View**，拖到BodyColumns，命名**RelicList**并勾Is Variable；Horizontal Box Slot Size=**Fill .44**，Vertical Alignment=Fill，Right Padding=12。
2. 选RelicList，Details的List Entries / List View相关分类（可搜索属性名）：**Entry Widget Class=WBP_ETRelicListEntry**、Selection Mode=Single、Orientation=Vertical、Is Focusable=True。Entry Spacing若可见填4，其余沿用默认。不要往Items手填字符串、Widget或假物品；运行时由Presenter提供UObject Item。
3. BodyColumns中，RelicList右侧放VerticalBox，命名DetailsColumn。其Horizontal Box Slot Size=**Fill .56**、Vertical Alignment=Fill。
4. DetailsColumn内按顺序放下表控件。每个参与绑定的控件勾Is Variable；名称大小写完全一致：

| 控件名 | UMG类型 | 字体 / 文本与布局 |
| --- | --- | --- |
| DetailNameText | Text Block | Font22，Auto Wrap True，Vertical Slot Auto / Fill；占位“选择遗物” |
| DetailDescriptionText | Text Block | Font16，Auto Wrap True，Auto / Fill、Top Padding8；占位“遗物说明” |
| SlotText | Text Block | Font16，Auto / Fill、Top Padding12；占位“比较目标：槽 A” |
| SlotButtons（不绑定） | Horizontal Box | Auto / Fill、Top Padding8；内放下面两个Button |
| SlotAButton | Button | Horizontal Slot Fill1、Right Padding4；子Text写“比较槽 A”、Font16 |
| SlotBButton | Button | Horizontal Slot Fill1、Left Padding4；子Text写“比较槽 B”、Font16 |
| PreviewText | Text Block | 在SlotButtons下方；Font16、Auto Wrap True、Vertical Slot Fill1 / Fill、Top Padding12；占位“属性比较” |

5. 两个外层Button的Is Focusable=True，Click / Press Method=Down and Up。按钮子Text的Button Slot Padding左右8 / 上下10；按钮子Text不叫SlotAButton / SlotBButton，也不参与BindWidget。所有Text Font Family沿用现有中文字体；没有Text Binding，没有OnClicked蓝图连线。

```text
PanelSize (SizeBox 620×460)
  PanelBorder
    VerticalBox
      TitleText
      InventoryStatusText
      BodyColumns (HorizontalBox)
        RelicList (ListView，.44)
        DetailsColumn (VerticalBox，.56)
          DetailNameText
          DetailDescriptionText
          SlotText
          SlotButtons (HorizontalBox)
            SlotAButton → Text
            SlotBButton → Text
          PreviewText
```

**7.2 检查记录（2026-10-29）：**已核对列表与详情的控件层级、名称、左右列比例0.44 / 0.56和列表右边距12。RelicList为Single且可获得焦点；两个按钮均可获得焦点，Click / Press Method均为Down and Up，按钮各为Fill 1且内侧间距各4。名称、说明和预览文字均开启自动换行；详情名称字号22，说明、槽位、预览及两个按钮文字字号16；说明 / 槽位 / 按钮组 / 预览的上边距依次为8 / 12 / 8 / 12，预览为Fill 1，按钮文字左右8、上下10。发现PreviewText未勾选Is Variable，已补勾；当前UserWidget父类下编译通过并保存。7.3原生绑定和运行时验收尚未完成。

### 7.3 原生父类与最后核对

1. 八个绑定控件全部完成后，Class Settings→Reparent Blueprint→**ETRelicPanelWidget**，核对`/Script/EmberTrialUI.ETRelicPanelWidget`。
2. Class Defaults：Is Focusable=False、Tick Frequency=Never。Compile→Save。必需绑定为**RelicList / InventoryStatusText / DetailNameText / DetailDescriptionText / PreviewText / SlotText / SlotAButton / SlotBButton**，正好八个。
3. Designer中的列表可能只有空预览行；实际名称来自Play中的真实目录，不需要PreConstruct假数据。父UserWidget的Never不关闭ListView自身Slate虚拟化更新，不要停用列表来消除Tick字样。

## 8. 嵌入原暂停页（只改Designer，不改旧Graph / 父类）

打开**`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`**。原ETPauseWidget父类、BTN_Resume / MainMenuButton / StatusText和原Flow配置保持。本节只把已有菜单容器与新面板排成两列。

### 8.1 包装现有菜单列

1. Designer的Hierarchy找到原Border下的**现有VerticalBox**；它包含原标题、BTN_Resume、MainMenuButton、StatusText。确认选的是整个VerticalBox，右键→**Wrap With→Size Box**，新SizeBox命名PauseActionsSize，Width Override勾选=**320**；Height Override不勾。
2. 选新PauseActionsSize，右键Wrap With→**Horizontal Box**，命名PauseColumns。选PauseActionsSize，在Horizontal Box Slot设Size=Auto、Vertical Alignment=Center。
3. 选PauseColumns，右键Wrap With→**Scale Box**，命名PauseScale；Stretch=**Scale To Fit**，Stretch Direction=**Down Only**。原菜单内容的名字和父类绑定不因外层包装改变，别删除原三个绑定控件。
4. 选原全屏Border，Content的Horizontal / Vertical Alignment改为**Fill / Fill**，Padding=24；原全屏锚点 / 四Offset0、背景色与Visible保持。这样ScaleBox获得屏幕范围，小窗口可整体缩小。

### 8.2 加入右列

1. 先确认新WBP_ETRelicPanel已Compile / Save。回WBP_PauseMenu，在Palette搜索**WBP_ETRelicPanel**（通常在User Created分类），拖入Hierarchy的**PauseColumns**，作为PauseActionsSize的**右侧兄弟**。
2. 选这个面板实例，Horizontal Box Slot Size=Auto、Vertical Alignment=Center、Padding Left=24，其余0。实例名可为RelicPanel；它不新增到旧Pause父类的BindWidget字段。
3. 最终Hierarchy核对如下。不要把新面板放进BTN_Resume、StatusText或原VerticalBox里：

```text
Canvas Panel（原）
  Border（原全屏背景）
    PauseScale (ScaleBox)
      PauseColumns (HorizontalBox)
        PauseActionsSize (SizeBox Width320)
          原VerticalBox
            原标题 / BTN_Resume / MainMenuButton / StatusText
        WBP_ETRelicPanel（新右列）
```

4. 原两个Button的Is Focusable、原Widget自身Is Focusable=True / Never、Graph无旧暂停业务保持。Compile→Save All，原三绑定及新两个Widget均无required binding / Native Tick警告。
5. 这次不加IA、Controller组件、CreateWidget蓝图或另一组SetInputMode / SetGamePaused。原暂停页创建时嵌入的面板自己启动一份页面Presenter；关闭时释放订阅。

## 9. 两项自动化与最小真实检查

### 9.1 两项各一次

非PIE的Output Log按顺序执行：

```text
Automation RunTests EmberTrial.UI.Relics.Presenter
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
```

两项应Success。新专项测Presenter事件 / 只读比较 / 重绑，旧事务检查后端行为未被通知改动破坏；不重跑31项全组。若失败，保留第一条Expected / 实际值和文件行号，不删断言。

**自动化补修记录（2026-10-29）：**原`New body restored and observed`失败由运行中换控的装备恢复时机造成，详见3.5。助手已直接修改Character.h / cpp与本节Presenter测试，保留原断言，新增新身体120生命上限 / 首次满血 / 30攻击预览，以及切回原身体仍为75生命、再次切换仍Ready的检查。正常Editor / Game Development Build均通过；使用UE 5.5.4无界面NullRHI进程执行上述两项，报告为**2 Success / 0 Fail / 0 NotRun**。原始日志与JSON位于`Artifacts/M3_03B/PossessionFix/Automation.log`及`Automation/index.json`；这次自动化不代替9.2的真实UI键鼠检查。

### 9.2 实际列表与比较（约5–10分钟）

1. 正式地图Play，先保持空槽100/100。P打开：左原菜单 / 右遗物页各一份；两行中文名称 / 未装备徽记，默认生息余烬显示最大生命**100→120**、轻击**25→25**。
2. 点击砺锋残片行：名称 / 描述更新，最大生命**100→100**、轻击**25→30**。点比较槽B，SlotText变槽B；点回槽A。当前HUD生命 / 真正伤害不因浏览改变。
3. 列表取得焦点后按P，应关闭并可立即移动 / 转向，不补攻击。再P初始继续按钮聚焦时，Enter或Space仍可继续；点击列表后Enter / Space可以由列表或按钮使用，需要继续时使用原继续按钮或P。
4. 在**未暂停**的游戏控制台执行`ET_Relic 1 life_ember`，关闭控制台再P：生息余烬有槽A徽记；选它比较槽A显示“该槽已装备” / 120→120，改比较槽B显示重复遗物方案不符合规则；选砺锋残片比较槽B应120→120 / 25→30。关闭后生命仍100/120，说明浏览没有治疗 / 换装。
5. 再开关P两次，单份列表 / 中文 / 选择正常。日志应各有Panel subscribed / unsubscribed，无关闭后旧面板继续发布；长日志由我读取。
6. 用P返回主菜单，再Continue，打开P只做一次快照检查：仍槽A生命遗物，身体120/120；关闭后左键 / 2 / E与基础输入短查。Stop后Save All，旧配置 / slomo / Debug沿用原值。

本批真实只检查两件列表 / 详情 / 订阅和原焦点边界；EntryReleased代码已接入，但不把两行显示宣传为完成500条池化压力测试。大列表 / 图标异步和正式确认入口后续集中验证，不让你手动创建几百件假遗物。

## 10. 简短反馈

```text
M3-03B 完成 / 卡在第__节第__步
七新UI / 五旧Source；Editor Build / 三索引 / Game Build：
两新WBP父类 / 3与8绑定 / List Entry Class / 无Binding / Never / Compile-Save：
原Pause两列 / 原三绑定与Flow保留：
UI.Relics.Presenter / Inventory.EquipmentTransactions各一次：
真实两行 / 中文 / 选中详情 / 槽A-B比较 / 浏览无真实属性变化：
真实生命遗物徽记 / 已装备与重复拒绝比较 / 单列表与P焦点：
返回菜单Continue快照 / Stop清理 / 原能力与HUD短基线：
Source / 三资产Save All；失败或最明显布局问题（无填无）：
完成时分 / 手动代码 / Designer / 检查 / 构建等待：
```

成功日志由我读取；C++第一条error用文字，Designer父类 / 层级 / 布局不符再补截图。界面工作量可以分次，不为凑本批报告额外重做旧验收图。

## 11. 本批 Commit / Push（固定放在最后）

初稿基于本地HEAD `4ed704b`的03A完成状态整理，下列13份Markdown属于该次文档准备范围；后续本章已进入实现，助手直接执行的Designer检查与源码补修见对应章节，提交前按当前Changes重新Stage。未核验远端：


```text
Docs/Reports/M3_03A_Verification.md
Docs/M3_03B_RelicPausePanel.md
Docs/M3_03A_RelicViewModels.md
Docs/M3_02C2_02_EquipmentTransactions.md
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

- 当前文档建议英文：`docs: verify relic view models and prepare pause menu inventory`
- 中文译文：`docs: 记录遗物展示模型验收并准备暂停页遗物面板`

完成本章后，Stage以下七新 / 四旧Source和最新文档：


```text
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h
EmberTrial/Source/EmberTrialUI/EmberTrialUI.Build.cs
EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicListEntryWidget.cpp
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp
EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETRelicListEntryWidget.h
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETRelicPanelWidget.h
EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp
```

本章资产 / 来源范围：

```text
EmberTrial/Content/EmberTrial/ui/Inventory/WBP_ETRelicListEntry.uasset（新）
EmberTrial/Content/EmberTrial/ui/Inventory/WBP_ETRelicPanel.uasset（新）
EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset（布局修改）
Docs/AssetManifest.csv（由我验收时补两份来源和实际SHA，更新旧Pause SHA）
```

- 实现建议英文：`feat: add event driven relic browsing to the pause menu`
- 中文译文：`feat: 为暂停菜单新增事件驱动的遗物浏览与属性比较`

CLion刷新Changes→Stage最新修改（已Stage后再改需再次Stage）→Review Diff→Commit / Push由你操作。Raw候选 / 日志 / 缓存 / Trace / 包留本地，不Force Add；Source / UE资产 / 文档与清单按实际本章差异提交。
