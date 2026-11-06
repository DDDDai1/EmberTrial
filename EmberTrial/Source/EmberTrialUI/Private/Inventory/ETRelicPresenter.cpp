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
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicPresenter, Log, All);

bool FETRelicPanelData::operator==(const FETRelicPanelData& Other) const
{
    return State == Other.State && PreviewState == Other.PreviewState && SelectedId == Other.SelectedId &&
           Name.EqualTo(Other.Name) && Description.EqualTo(Other.Description) && bSlotB == Other.bSlotB &&
           BeforeMax == Other.BeforeMax && AfterMax == Other.AfterMax &&
               BeforeAttack == Other.BeforeAttack && AfterAttack == Other.AfterAttack &&
               BeforeHealingBonus == Other.BeforeHealingBonus &&
               AfterHealingBonus == Other.AfterHealingBonus &&
               bHasCommands == Other.bHasCommands && bCanEquip == Other.bCanEquip &&
               bCanUnequip == Other.bCanUnequip && CommandMessage.EqualTo(Other.CommandMessage);
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
    ++ObservationGeneration;
    CanSubmitCommand.Unbind();
    SubmitCommand.Unbind();
    CommandMessage = FText::GetEmpty();
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
    CommandMessage = FText::GetEmpty();
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
    CommandMessage = FText::GetEmpty();
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
    CommandMessage = FText::GetEmpty();
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
    Data.CommandMessage = CommandMessage;
    Data.bHasCommands = CanSubmitCommand.IsBound() && SubmitCommand.IsBound();
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
    if (Data.State != EETRelicPanelState::Ready)
    {
        return Data;
    }
    float CurrentHealth = 0.0f;
    float CurrentMax = 0.0f;
    if (!Body->GetGameplayHealthSnapshot(CurrentHealth, CurrentMax) || CurrentHealth <= 0.0f)
    {
        return Data;
    }
    const bool bAllowed = !bSubmitting && Data.bHasCommands &&
        CanSubmitCommand.Execute(Equipment);
    UETRelicInventorySubsystem* Inventory = BoundInventory.Get();
    const FETRelicInventorySnapshot Current = Inventory ? Inventory->GetSnapshot() : FETRelicInventorySnapshot{};
    Data.bCanUnequip = bAllowed && !(bSlotB ? Current.Equipped.SlotB : Current.Equipped.SlotA).IsNone();
    // 无搜索结果仍可卸下目标槽，但不能把空选择误当成装备请求。
    if (SelectedId.IsNone())
    {
        return Data;
    }
    FETRelicLoadoutPlan Preview;
    const auto Reply = Equipment->TryBuildPreview(bSlotB ? EETRelicSlot::Second : EETRelicSlot::First,
                                                 SelectedId, ObservedRevision, Preview);
    if (Reply.Result == EETRelicInventoryResult::Success)
    {
        Data.bCanEquip = bAllowed;
        Data.PreviewState = EETRelicPreviewState::Available;
        Data.BeforeMax = Preview.BeforePreview.MaxHealth;
        Data.AfterMax = Preview.AfterPreview.MaxHealth;
        Data.BeforeAttack = Preview.BeforePreview.LightAttackBaseDamage;
        Data.AfterAttack = Preview.AfterPreview.LightAttackBaseDamage;
        Data.BeforeHealingBonus = Preview.BeforeModifiers.HealingBonus;
        Data.AfterHealingBonus = Preview.AfterModifiers.HealingBonus;
    }
    else if (Reply.Result == EETRelicInventoryResult::NoChange)
    {
        float Health = 0.0f;
        float Bonus = 0.0f;
        FETRelicModifierTotals Applied;
        const UETGA_LightAttack* Attack = Body->GetLightAttackConfiguration();
        if (Attack && Equipment->GetAppliedModifiers(Applied) &&
            Body->GetGameplayHealthSnapshot(Health, Data.BeforeMax) &&
            Body->GetGameplayLightAttackBonusSnapshot(Bonus) && UETGA_LightAttack::TryCalculateStageDamage(
                Attack->GetBaseAttackDamage(), Bonus, 1.0f, Data.BeforeAttack))
        {
            Data.PreviewState = EETRelicPreviewState::AlreadyEquipped;
            Data.AfterMax = Data.BeforeMax;
            Data.AfterAttack = Data.BeforeAttack;
            Data.BeforeHealingBonus = Applied.HealingBonus;
            Data.AfterHealingBonus = Applied.HealingBonus;
        }
    }
    else if (Reply.Result == EETRelicInventoryResult::RuleRejected)
    {
        Data.PreviewState = EETRelicPreviewState::Rejected;
    }
    return Data;
}

// ---- 当前页面命令；Reply来自真正业务提交，View不能自行制造成功 ----
void UETRelicPresenter::SetCommands(FETRelicCanSubmit CanSubmit, FETRelicSubmit Submit)
{
    CanSubmitCommand = MoveTemp(CanSubmit);
    SubmitCommand = MoveTemp(Submit);
    CommandMessage = FText::GetEmpty();
    Refresh();
}

void UETRelicPresenter::RefreshCommandAvailability()
{
    Refresh();
}

FETRelicInventoryReply UETRelicPresenter::ConfirmSelected()
{
    return SubmitChange(false);
}

FETRelicInventoryReply UETRelicPresenter::UnequipPreviewSlot()
{
    return SubmitChange(true);
}

FETRelicInventoryReply UETRelicPresenter::SubmitChange(const bool bUnequip)
{
    if (bSubmitting || bRefreshing)
    {
        return {EETRelicInventoryResult::Busy};
    }
    UETRelicEquipmentComponent* Equipment = BoundEquipment.Get();
    if (!bObserving || !IsValid(Equipment) || !CanSubmitCommand.IsBound() ||
        !SubmitCommand.IsBound() || !CanSubmitCommand.Execute(Equipment) ||
        (bUnequip ? !Snapshot.bCanUnequip : !Snapshot.bCanEquip))
    {
        return {EETRelicInventoryResult::NotReady};
    }

    TStrongObjectPtr<UETRelicPresenter> KeepAlive(this);
    const uint64 Generation = ObservationGeneration;
    const EETRelicSlot Slot = bSlotB ? EETRelicSlot::Second : EETRelicSlot::First;
    const FName RequestedId = bUnequip ? NAME_None : SelectedId;
    const uint64 ExpectedRevision = ObservedRevision;
    const FETRelicSubmit Command = SubmitCommand;
    FETRelicInventoryReply Reply;
    {
        TGuardValue<bool> Guard(bSubmitting, true);
        // 副本保证回调中Shutdown / Unbind不会销毁正在执行的委托实例。
        Reply = Command.Execute(Equipment, Slot, RequestedId, ExpectedRevision);
    }
    // GE同步回调可关闭页面或换身体；不向已结束 / 重启的观察写入旧结果。
    if (!bObserving || Generation != ObservationGeneration || BoundEquipment.Get() != Equipment)
    {
        return Reply;
    }
    switch (Reply.Result)
    {
    case EETRelicInventoryResult::Success:
        CommandMessage = FText::FromString(bUnequip ? TEXT("已卸下该槽遗物。") : TEXT("装备已生效。"));
        break;
    case EETRelicInventoryResult::NoChange:
        CommandMessage = FText::FromString(TEXT("该槽装备未变化。"));
        break;
    case EETRelicInventoryResult::StalePlan:
        CommandMessage = FText::FromString(TEXT("装备数据已变化，请查看最新比较后再次确认。"));
        break;
    case EETRelicInventoryResult::Busy:
        CommandMessage = FText::FromString(TEXT("正在处理，请稍后再试。"));
        break;
    case EETRelicInventoryResult::RuleRejected:
        CommandMessage = FText::FromString(TEXT("此方案不符合装备规则，请重新选择遗物与槽位。"));
        break;
    default:
        CommandMessage = FText::FromString(TEXT("当前祭坛或身体不可用，请关闭页面后重新进入。"));
        break;
    }
    Refresh();
    return Reply;
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