# M3-04C2：祭坛页面确认装备与卸装

2026-11-04，UE5.5.4。[C1已验收](Reports/M3_04C1_Verification.md)。本章是手动实施指南：十旧UI模块文件、两个既有Widget资产；正式Source / 资产尚未改动，没有本章Build或Success结论。

## 1. 完成后的体验与分工

祭坛旁按E，原遗物面板出现“确认装备”“卸下该槽”和结果提示。比较槽A / B、选择遗物仍只预览，点击确认才提交；装备徽记、比较、生命HUD与真实攻击同步。普通P页只读，命令区域整体隐藏。无搜索结果仍可卸下已装备目标槽，已装备同件与空槽按钮按状态禁用。

Flow独占C1的Access；在非暂停时创建，取得暂停并成功呈现同一页后绑定命令。每次确认核对当前View身份、IsInViewport、暂停归属、身体 / 世界 / 来源与会话，再由Core按看过的库存版本提交。Presenter只拿弱UObject绑定委托，不持有Access，不直接Apply GE。页面移除时先撤销Access与命令，旧View / 旧Presenter不能继续写。

切图请求Pending期间按钮与最终接收端均拒绝；请求预检失败可沿原页面恢复，成功切图或页面关闭撤销上下文。不会为提交暂时解暂停，也不增加新的Flow、输入Action、每帧扫描或临时验收蓝图。

| 工作 | 预计手动时间 |
| --- | --- |
| 十旧UI模块文件，以下按文件逐处修改 | 70–100分钟 |
| 两个既有Widget的Designer配置 | 15–25分钟 |
| 三项相关专项与真实分组短查 | 5–10分钟 |

当前193 Source / 108cpp（Core76 / UI29 / Editor3）、32测试、299 Content / 来源80；本批不增加Source文件 / 测试注册或资产，数量保持。等待另记，预计不计实耗。

## 2. 实施前

Save All并关闭UE。修改顺序与下节一致；头文件新增绑定后，旧Widget在Designer补齐前可能出现required binding信息，不先Play或改成Optional逃避装配。本章不Reparent、不删除旧Graph、不改C1 Core / 资源默认值。

所有文件都在`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI`。以下每小节先说明定位点，再给该处替换后的完整片段：原文查找块只是定位，**不要把原文和新文同时粘贴**。未列出的逻辑保留。

## 3. 按文件逐处修改

### 3.1 EmberTrialUI.Build.cs

**修改 1：** 在PublicDependencyModuleNames中加入EmberTrial（注意前一项逗号）。

查找原文：
```cpp
            "SlateCore"
```

替换为：
```cpp
            "SlateCore",
            "EmberTrial"
```

**修改 2：** 从PrivateDependencyModuleNames删除EmberTrial；模块只保留一处此依赖。

查找原文：
```cpp
            "EmberTrial",
            "GameplayAbilities",
```

替换为：
```cpp
            "GameplayAbilities",
```

**修改 3：** 同步这条旧依赖注释。

查找原文：
```cpp
// UI通过私有Core依赖读取玩法接口；Slate / Input只服务实现，Core不能反向依赖UI。
```

替换为：
```cpp
// 公开UI委托使用Core槽位 / 回复类型，Core已列为公开依赖；Core仍不能反向依赖UI。
```

### 3.2 Public/Inventory/ETRelicPresenter.h

**修改 4：** 在原ViewModels include后加入Core库存头；自身generated.h仍是最后一条include。

查找原文：
```cpp
#include "Inventory/ETRelicViewModels.h"
```

替换为：
```cpp
#include "Inventory/ETRelicViewModels.h"
#include "Inventory/ETRelicInventorySubsystem.h"
```

**修改 5：** 在FETRelicPanelData内扩展命令显示状态；不是Presenter私有bSlotB的位置。

查找原文：
```cpp
    bool bSlotB = false;
    float BeforeMax
```

替换为：
```cpp
    bool bSlotB = false;
    bool bHasCommands = false;
    bool bCanEquip = false;
    bool bCanUnequip = false;
    FText CommandMessage;
    float BeforeMax
```

**修改 6：** 在UCLASS之前加入两个原生委托类型并替换旧只读注释。

查找原文：
```cpp
// 页面作用域的只读Presenter：观察数据源、复用稳定ID的Item、保存页面选择。
```

替换为：
```cpp
// 页面只拿到弱绑定的查询 / 提交委托，不持有Core Access本体。
DECLARE_DELEGATE_RetVal_OneParam(bool, FETRelicCanSubmit,
    const UETRelicEquipmentComponent*);
DECLARE_DELEGATE_RetVal_FourParams(FETRelicInventoryReply, FETRelicSubmit,
    UETRelicEquipmentComponent*, EETRelicSlot, FName, uint64);

// 页面作用域Presenter：观察数据，保存选择，只通过当前Flow的命令桥提交。
```

**修改 7：** 在Presenter public的Shutdown后新增四个原生接口。

查找原文：
```cpp
    void Shutdown();
```

替换为：
```cpp
    void Shutdown();
    void SetCommands(FETRelicCanSubmit CanSubmit, FETRelicSubmit Submit);
    void RefreshCommandAvailability();
    FETRelicInventoryReply ConfirmSelected();
    FETRelicInventoryReply UnequipPreviewSlot();
```

**修改 8：** 在private的BuildData声明后增加共享提交方法。

查找原文：
```cpp
    FETRelicPanelData BuildData() const;
```

替换为：
```cpp
    FETRelicPanelData BuildData() const;
    FETRelicInventoryReply SubmitChange(bool bUnequip);
```

**修改 9：** 在Presenter末尾增加命令与观察代次；不要放进Data结构。

查找原文：
```cpp
    bool bRefreshing = false;
```

替换为：
```cpp
    bool bRefreshing = false;
    bool bSubmitting = false;
    uint64 ObservationGeneration = 0;
    FETRelicCanSubmit CanSubmitCommand;
    FETRelicSubmit SubmitCommand;
    FText CommandMessage;
```

### 3.3 Private/Inventory/ETRelicPresenter.cpp

**修改 10：** 增加同步提交期间的自身GC保活头文件。

查找原文：
```cpp
#include "Templates/UnrealTemplate.h"
```

替换为：
```cpp
#include "Templates/UnrealTemplate.h"
#include "UObject/StrongObjectPtr.h"
```

**修改 11：** 扩FETRelicPanelData::operator==末行，命令状态变化也要驱动View。

查找原文：
```cpp
           BeforeAttack == Other.BeforeAttack && AfterAttack == Other.AfterAttack;
```

替换为：
```cpp
           BeforeAttack == Other.BeforeAttack && AfterAttack == Other.AfterAttack &&
           bHasCommands == Other.bHasCommands && bCanEquip == Other.bCanEquip &&
           bCanUnequip == Other.bCanUnequip && CommandMessage.EqualTo(Other.CommandMessage);
```

**修改 12：** StopObserving入口先撤销当前页面命令；原订阅清理保留。

查找原文：
```cpp
void UETRelicPresenter::StopObserving(const bool bNotify)
{
```

替换为：
```cpp
void UETRelicPresenter::StopObserving(const bool bNotify)
{
    ++ObservationGeneration;
    CanSubmitCommand.Unbind();
    SubmitCommand.Unbind();
    CommandMessage = FText::GetEmpty();
```

**修改 13：** 切换选择时清除上一次操作提示。

查找原文：
```cpp
    SelectedId = Id;
```

替换为：
```cpp
    CommandMessage = FText::GetEmpty();
    SelectedId = Id;
```

**修改 14：** 切换目标槽时清除旧提示。

查找原文：
```cpp
    bSlotB = bSecondSlot;
```

替换为：
```cpp
    CommandMessage = FText::GetEmpty();
    bSlotB = bSecondSlot;
```

**修改 15：** 修改查询条件时清除旧提示。

查找原文：
```cpp
    Query = NewQuery;
```

替换为：
```cpp
    CommandMessage = FText::GetEmpty();
    Query = NewQuery;
```

**修改 16：** 完整替换BuildData函数；只移除这里的“SelectedId为空就提前返回”旧逻辑，使空搜索结果仍可卸装。

定位原函数签名 `FETRelicPanelData UETRelicPresenter::BuildData() const`，选中该函数直到配对的最后一个右大括号。

替换为：
```cpp
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
```

**修改 17：** 在“回调核对当前来源”分区之前插入五个完整命令方法。

查找原文：
```cpp
// ---- 回调核对当前来源，忽略旧身体 / 旧库存的事件 ----
```

替换为：
```cpp
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
```

### 3.4 Public/Widgets/ETRelicPanelWidget.h

**修改 18：** 增加命令委托类型；自身generated.h保持最后。

查找原文：
```cpp
#include "Components/ComboBoxString.h"
```

替换为：
```cpp
#include "Components/ComboBoxString.h"
#include "Inventory/ETRelicPresenter.h"
```

**修改 19：** 增加VerticalBox前置声明。

查找原文：
```cpp
class UTextBlock;
```

替换为：
```cpp
class UTextBlock;
class UVerticalBox;
```

**修改 20：** 新增两个public原生方法，不是蓝图事件。

查找原文：
```cpp
protected:
    virtual void NativeConstruct()
```

替换为：
```cpp
public:
    void SetCommands(FETRelicCanSubmit CanSubmit, FETRelicSubmit Submit);
    void SetCommandBusy(bool bBusy);

protected:
    virtual void NativeConstruct()
```

**修改 21：** HandleSlotB后新增两个按钮处理函数，各带UFUNCTION。

查找原文：
```cpp
    void HandleSlotB();
```

替换为：
```cpp
    void HandleSlotB();

    UFUNCTION()
    void HandleConfirm();

    UFUNCTION()
    void HandleUnequip();
```

**修改 22：** 在原PresenterHandle之前加入四个必需绑定及本View命令缓存；11个旧绑定保留。

查找原文：
```cpp
    FDelegateHandle PresenterHandle;
```

替换为：
```cpp
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> CommandContent;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> ConfirmEquipButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> UnequipButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> CommandResultText;

    FETRelicCanSubmit CanSubmitCommand;
    FETRelicSubmit SubmitCommand;
    bool bCommandBusy = false;

    FDelegateHandle PresenterHandle;
```

**修改 23：** 同步绑定数量注释。

查找原文：
```cpp
以下11个BindWidget
```

替换为：
```cpp
以下15个BindWidget
```

### 3.5 Private/Widgets/ETRelicPanelWidget.cpp

**修改 24：** 增加VerticalBox头文件。

查找原文：
```cpp
#include "Components/TextBlock.h"
```

替换为：
```cpp
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
```

**修改 25：** HasRequiredLayout增加四项，不能改成Optional规避Designer装配。

查找原文：
```cpp
           SlotText && SlotAButton && SlotBButton && SearchBox && FilterCombo && SortCombo;
```

替换为：
```cpp
           SlotText && SlotAButton && SlotBButton && SearchBox && FilterCombo && SortCombo &&
           CommandContent && ConfirmEquipButton && UnequipButton && CommandResultText;
```

**修改 26：** 在NativeConstruct原SlotA绑定前添加两个新按钮绑定。

查找原文：
```cpp
    SlotAButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotA);
```

替换为：
```cpp
    ConfirmEquipButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleConfirm);
    UnequipButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleUnequip);
    SlotAButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSlotA);
```

**修改 27：** 观察启动后再设置命令；StartObserving会清理前一次命令。

查找原文：
```cpp
    Presenter->StartObserving(LocalPlayer ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>() : nullptr);
```

替换为：
```cpp
    Presenter->StartObserving(LocalPlayer ? LocalPlayer->GetSubsystem<UETUILocalPlayerSubsystem>() : nullptr);
    Presenter->SetCommands(CanSubmitCommand, SubmitCommand);
```

**修改 28：** NativeDestruct最开头解除新按钮与命令缓存，后续旧清理保留。

查找原文：
```cpp
void UETRelicPanelWidget::NativeDestruct()
{
```

替换为：
```cpp
void UETRelicPanelWidget::NativeDestruct()
{
    CanSubmitCommand.Unbind();
    SubmitCommand.Unbind();
    if (ConfirmEquipButton)
    {
        ConfirmEquipButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleConfirm);
    }
    if (UnequipButton)
    {
        UnequipButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleUnequip);
    }
```

**修改 29：** 先解绑View事件，再清命令 / Shutdown，关闭时不渲染一次旧View。

查找原文：
```cpp
        Presenter->OnChanged.Remove(PresenterHandle);
        Presenter->Shutdown();
```

替换为：
```cpp
        Presenter->OnChanged.Remove(PresenterHandle);
        Presenter->SetCommands({}, {});
        Presenter->Shutdown();
```

**修改 30：** Render中ReadyCount分支区分祭坛可提交上下文与普通只读页面。

查找原文：
```cpp
? NSLOCTEXT("ETRelics", "ReadyCount", "遗物 · 只读查看（{0} 件）")
```

替换为：
```cpp
? (Data.bHasCommands ? NSLOCTEXT("ETRelics", "AltarCount", "遗物祭坛（{0} 件）")
                             : NSLOCTEXT("ETRelics", "ReadyCount", "遗物 · 只读查看（{0} 件）"))
```

**修改 31：** 在Render原SlotText设置后增加按钮 / 提示显示；空搜索也不阻止卸下已装备槽。

查找原文：
```cpp
    SlotText->SetText(FText::FromString(Data.bSlotB ? TEXT("比较目标：槽 B") : TEXT("比较目标：槽 A")));
```

替换为：
```cpp
    SlotText->SetText(FText::FromString(Data.bSlotB ? TEXT("比较目标：槽 B") : TEXT("比较目标：槽 A")));
    CommandContent->SetVisibility(Data.bHasCommands ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ConfirmEquipButton->SetIsEnabled(!bCommandBusy && Data.bCanEquip);
    UnequipButton->SetIsEnabled(!bCommandBusy && Data.bCanUnequip);
    CommandResultText->SetText(Data.CommandMessage.IsEmpty()
        ? FText::FromString(TEXT("选择目标槽位与遗物后确认。")) : Data.CommandMessage);
```

**修改 32：** 在原“查询控件”分区前插入四个方法；保留所有搜索 / P / Esc逻辑。

查找原文：
```cpp
// ---- 查询控件：
```

替换为：
```cpp
// ---- 原生页面命令桥；View只发意图，不能自己Apply GE ----
void UETRelicPanelWidget::SetCommands(FETRelicCanSubmit CanSubmit, FETRelicSubmit Submit)
{
    CanSubmitCommand = MoveTemp(CanSubmit);
    SubmitCommand = MoveTemp(Submit);
    if (Presenter)
    {
        Presenter->SetCommands(CanSubmitCommand, SubmitCommand);
    }
}

void UETRelicPanelWidget::SetCommandBusy(const bool bBusy)
{
    bCommandBusy = bBusy;
    if (Presenter)
    {
        Render(Presenter->GetSnapshot());
    }
}

void UETRelicPanelWidget::HandleConfirm()
{
    if (!bCommandBusy && Presenter)
    {
        Presenter->ConfirmSelected();
    }
}

void UETRelicPanelWidget::HandleUnequip()
{
    if (!bCommandBusy && Presenter)
    {
        Presenter->UnequipPreviewSlot();
    }
}

// ---- 查询控件：
```

### 3.6 Public/Widgets/ETPauseWidget.h

**修改 33：** 增加原生命令委托类型；自身generated.h仍最后。

查找原文：
```cpp
#include "Blueprint/UserWidget.h"
```

替换为：
```cpp
#include "Blueprint/UserWidget.h"
#include "Inventory/ETRelicPresenter.h"
```

**修改 34：** 增加嵌入面板前置声明。

查找原文：
```cpp
class UTextBlock;
```

替换为：
```cpp
class UTextBlock;
class UETRelicPanelWidget;
```

**修改 35：** public增加向嵌入Panel转发命令的方法。

查找原文：
```cpp
    void FocusResume();
```

替换为：
```cpp
    void FocusResume();
    void ConfigureRelicCommands(FETRelicCanSubmit CanSubmit, FETRelicSubmit Submit);
```

**修改 36：** 在bBusy注释前增加名为RelicPanel的必需子Widget绑定；旧三绑定保留。

查找原文：
```cpp
    // busy只保护本View请求中的输入
```

替换为：
```cpp
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UETRelicPanelWidget> RelicPanel;

    // busy只保护本View请求中的输入
```

### 3.7 Private/Widgets/ETPauseWidget.cpp

**修改 37：** 增加嵌入Panel头。

查找原文：
```cpp
#include "Widgets/ETPauseWidget.h"
```

替换为：
```cpp
#include "Widgets/ETPauseWidget.h"
#include "Widgets/ETRelicPanelWidget.h"
```

**修改 38：** HasRequiredLayout增加第四个绑定RelicPanel。

查找原文：
```cpp
return IsValid(BTN_Resume.Get()) && IsValid(MainMenuButton.Get()) && IsValid(StatusText.Get());
```

替换为：
```cpp
return IsValid(BTN_Resume.Get()) && IsValid(MainMenuButton.Get()) && IsValid(StatusText.Get()) && IsValid(RelicPanel.Get());
```

**修改 39：** ApplyState同步暂停页Pending / Ready到新确认按钮。

查找原文：
```cpp
    bBusy = bPending;
```

替换为：
```cpp
    bBusy = bPending;
    if (RelicPanel)
    {
        RelicPanel->SetCommandBusy(bBusy);
    }
```

**修改 40：** 更新ShowAltarReady展示文字。

查找原文：
```cpp
遗物祭坛 · 选择遗物比较属性；松开 P 或选择继续游戏返回试炼。
```

替换为：
```cpp
遗物祭坛 · 比较后确认装备或卸下目标槽；松开 P 或选择继续游戏返回。
```

**修改 41：** 在ShowAltarReady之前插入转发方法。

查找原文：
```cpp
void UETPauseWidget::ShowAltarReady()
```

替换为：
```cpp
void UETPauseWidget::ConfigureRelicCommands(FETRelicCanSubmit CanSubmit, FETRelicSubmit Submit)
{
    if (RelicPanel)
    {
        RelicPanel->SetCommands(MoveTemp(CanSubmit), MoveTemp(Submit));
    }
}

void UETPauseWidget::ShowAltarReady()
```

### 3.8 Public/Flow/ETFailureFlowComponent.h

**修改 42：** Flow独占TUniquePtr需要完整原生Access类型；自身generated.h仍最后。

查找原文：
```cpp
#include "TimerManager.h"
```

替换为：
```cpp
#include "TimerManager.h"
#include "Interaction/ETRelicAltarAccess.h"
```

**修改 43：** 增加原生方法所用类型前置声明。

查找原文：
```cpp
class AETRelicAltar;
```

替换为：
```cpp
class AETRelicAltar;
class UETRelicEquipmentComponent;
struct FETRelicInventoryReply;
enum class EETRelicSlot : uint8;
```

**修改 44：** ClearAltarContext声明后添加两个原生命令接收方法，不加UFUNCTION。

查找原文：
```cpp
    void ClearAltarContext();
```

替换为：
```cpp
    void ClearAltarContext();
    bool CanSubmitRelic(const UETRelicEquipmentComponent* Equipment,
        TWeakObjectPtr<UETPauseWidget> SourceView) const;
    FETRelicInventoryReply SubmitRelic(UETRelicEquipmentComponent* Equipment,
        EETRelicSlot Slot, FName RelicId, uint64 ExpectedRevision,
        TWeakObjectPtr<UETPauseWidget> SourceView);
```

**修改 45：** ActiveAltar之后增加唯一上下文所有权；不加UPROPERTY。

查找原文：
```cpp
    TWeakObjectPtr<AETRelicAltar> ActiveAltar;
```

替换为：
```cpp
    TWeakObjectPtr<AETRelicAltar> ActiveAltar;
    TUniquePtr<FETRelicAltarAccess> AltarAccess;
```

### 3.9 Private/Flow/ETFailureFlowComponent.cpp

**修改 46：** 增加装备服务 / Presenter委托头文件。

查找原文：
```cpp
#include "Interaction/ETRelicAltar.h"
```

替换为：
```cpp
#include "Interaction/ETRelicAltar.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Inventory/ETRelicPresenter.h"
```

**修改 47：** 完整替换PresentPause函数；其余暂停 / Failure / 输入恢复逻辑保留。

定位原函数签名 `void UETFailureFlowComponent::PresentPause(AETRelicAltar* SourceAltar)`，选中该函数直到配对的最后一个右大括号。

替换为：
```cpp
void UETFailureFlowComponent::PresentPause(AETRelicAltar* SourceAltar)
{
    const bool bFromAltar = SourceAltar != nullptr;
    AETPlayerController* Controller = ControllerOwner.Get();
    AEmberTrialCharacter* OpeningBody = Controller
        ? Cast<AEmberTrialCharacter>(Controller->GetPawn()) : nullptr;
    if (!HasCurrentBody(false) || bCommandPending || bPresentationQueued ||
        IsValid(PauseWidget.Get()) || IsValid(FailureWidget.Get()) || Session->IsTravelPending() ||
        UGameplayStatics::IsGamePaused(this) ||
        (bFromAltar && (!IsValid(SourceAltar) || !SourceAltar->CanUseAltar(OpeningBody))))
    {
        return;
    }
    TUniquePtr<FETRelicAltarAccess> NextAccess;
    if (bFromAltar)
    {
        NextAccess = FETRelicAltarAccess::TryOpen(Controller, SourceAltar);
        if (!NextAccess)
        {
            return;
        }
    }
    if (!PauseWidgetClass || PauseWidgetClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06D] Missing concrete PauseWidgetClass Owner=%s"), *GetNameSafe(GetOwner()));
        return;
    }
    PauseWidget = CreateWidget<UETPauseWidget>(Controller, PauseWidgetClass);
    if (!IsValid(PauseWidget.Get()) || !PauseWidget->HasRequiredLayout())
    {
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06D] Pause layout invalid"));
        RemovePauseWidget();
        return;
    }
    PauseWidget->OnResumeIntent.AddUniqueDynamic(this, &ThisClass::HandleResumeRequested);
    PauseWidget->OnMainMenuIntent.AddUniqueDynamic(this, &ThisClass::HandleMainMenuRequested);
    if (bFromAltar)
    {
        PauseWidget->ShowAltarReady();
    }
    else
    {
        PauseWidget->ShowReady();
    }
    if (!PauseWidget->AddToPlayerScreen(PauseWidgetZOrder))
    {
        RemovePauseWidget();
        return;
    }
    // 创建 / 显示Widget可同步运行回调；在取得暂停前重验当前身体与来源。
    if (!IsValid(PauseWidget.Get()) || !HasCurrentBody(false) ||
        Controller->GetPawn() != OpeningBody || Session->IsTravelPending() ||
        UGameplayStatics::IsGamePaused(this) ||
        (bFromAltar && (!IsValid(SourceAltar) || !SourceAltar->CanUseAltar(OpeningBody))) ||
        !UGameplayStatics::SetGamePaused(this, true))
    {
        RemovePauseWidget();
        return;
    }
    bOwnsPause = true;
    if (bFromAltar)
    {
        AltarAccess = MoveTemp(NextAccess);
        ActiveAltar = SourceAltar;
        SourceAltar->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleAltarEndPlay);
    }
    if (bFromAltar)
    {
        const TWeakObjectPtr<UETPauseWidget> SourceView(PauseWidget.Get());
        PauseWidget->ConfigureRelicCommands(
            FETRelicCanSubmit::CreateUObject(this, &ThisClass::CanSubmitRelic, SourceView),
            FETRelicSubmit::CreateUObject(this, &ThisClass::SubmitRelic, SourceView));
        if (PauseWidget.Get() != SourceView.Get() || !HasCurrentBody(false))
        {
            RemovePauseWidget();
            RestoreOwnedInput();
            return;
        }
    }
    OpeningBody->StopJumping();
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(Controller, PauseWidget.Get(),
        EMouseLockMode::DoNotLock, true);
    PauseWidget->FocusResume();
    NotifyMenuState();
    UE_LOG(LogETFailureFlow, Display, TEXT("[M1-06D] Pause shown Owner=%s Widget=%s Paused=1"),
        *GetNameSafe(Controller), *GetNameSafe(PauseWidget.Get()));
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M3-04B] Browse shown Owner=%s Altar=%s Mode=%s"), *GetNameSafe(Controller),
        *GetNameSafe(ActiveAltar.Get()), bFromAltar ? TEXT("Altar") : TEXT("Pause"));
}
```

**修改 48：** ClearAltarContext先撤销许可，再清View命令，后面原来源解绑保持。

查找原文：
```cpp
void UETFailureFlowComponent::ClearAltarContext()
{
```

替换为：
```cpp
void UETFailureFlowComponent::ClearAltarContext()
{
    if (AltarAccess)
    {
        AltarAccess->Close();
    }
    AltarAccess.Reset();
    if (IsValid(PauseWidget.Get()))
    {
        PauseWidget->ConfigureRelicCommands({}, {});
    }
```

**修改 49：** 在HandleAltarEndPlay函数前插入两个最终接收方法；弱SourceView是绑定payload，位置必须在参数列表末尾。

查找原文：
```cpp
void UETFailureFlowComponent::HandleAltarEndPlay(
```

替换为：
```cpp
// ---- 最终命令门控；旧View、普通P、挂起切图与外部暂停都不能提供写权限 ----
bool UETFailureFlowComponent::CanSubmitRelic(const UETRelicEquipmentComponent* Equipment,
    TWeakObjectPtr<UETPauseWidget> SourceView) const
{
    return HasCurrentBody(false) && !bCommandPending && !bPresentationQueued && bOwnsPause &&
        SourceView.IsValid() && SourceView.Get() == PauseWidget.Get() && SourceView->IsInViewport() &&
        !IsValid(FailureWidget.Get()) && !Session->IsTravelPending() &&
        UGameplayStatics::IsGamePaused(this) && AltarAccess && AltarAccess->CanCommitFor(Equipment);
}

FETRelicInventoryReply UETFailureFlowComponent::SubmitRelic(
    UETRelicEquipmentComponent* Equipment, const EETRelicSlot Slot, const FName RelicId,
    const uint64 ExpectedRevision, TWeakObjectPtr<UETPauseWidget> SourceView)
{
    if (!CanSubmitRelic(Equipment, SourceView))
    {
        return {EETRelicInventoryResult::NotReady};
    }
    const FETRelicInventoryReply Reply = Equipment->TryEquipAtAltar(
        *AltarAccess, Slot, RelicId, ExpectedRevision);
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M3-04C2] Relic submit Slot=%d Id=%s Result=%d"),
        static_cast<int32>(Slot), *RelicId.ToString(), static_cast<int32>(Reply.Result));
    return Reply;
}

void UETFailureFlowComponent::HandleAltarEndPlay(
```

### 3.10 Tests/ETRelicPresenterTests.cpp

**修改 50：** 在既有测试include组加入四个夹具头。

查找原文：
```cpp
#include "Inventory/ETRelicPresenter.h"
```

替换为：
```cpp
#include "Inventory/ETRelicPresenter.h"
#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarAccess.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
```

**修改 51：** 在RunTest末尾原Presenter->Shutdown紧前面插入完整代码块；此前旧断言全部保留。

查找原文：
```cpp
    Presenter->Shutdown();
```

替换为：
```cpp
    // ---- C2原生命令桥；不以无Viewport夹具冒充真实Flow / UMG创建 ----
    {
        using EResult = EETRelicInventoryResult;
        Presenter->SetQuery({});
        Presenter->SetPreviewSlotB(false);
        Presenter->SelectId(LifeId);
        TestFalse(TEXT("Ordinary read-only presenter has no commands"), Presenter->GetSnapshot().bHasCommands);
        TestTrue(TEXT("No command cannot unequip"), Presenter->UnequipPreviewSlot().Result == EResult::NotReady);
        AETRelicAltar* Altar = World->SpawnActor<AETRelicAltar>(
            OtherBody->GetActorLocation() + FVector(100, 0, 0), FRotator::ZeroRotator, Spawn);
        APlayerState* Pauser = World->SpawnActor<APlayerState>();
        auto Access = FETRelicAltarAccess::TryOpen(PC, Altar);
        if (!TestNotNull(TEXT("Command pauser"), Pauser) ||
            !TestTrue(TEXT("Command altar access opens"), Access.IsValid()))
        {
            return false;
        }
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        bool bPageOpen = true;
        int32 Calls = 0;
        EResult Nested = EResult::Success;
        Presenter->SetCommands(
            FETRelicCanSubmit::CreateLambda([&](const UETRelicEquipmentComponent* Target)
            { return bPageOpen && Access->CanCommitFor(Target); }),
            FETRelicSubmit::CreateLambda([&](UETRelicEquipmentComponent* Target,
                EETRelicSlot Slot, FName Id, uint64 Revision)
            {
                ++Calls;
                Nested = Presenter->ConfirmSelected().Result;
                if (!bPageOpen || !Access->CanCommitFor(Target))
                {
                    return FETRelicInventoryReply{EResult::NotReady};
                }
                return Target->TryEquipAtAltar(*Access, Slot, Id, Revision);
            }));
        TestTrue(TEXT("Equipped target slot can unequip"), Presenter->GetSnapshot().bCanUnequip);
        TestTrue(TEXT("Command real unequip"), Presenter->UnequipPreviewSlot().Result == EResult::Success);
        TestTrue(TEXT("Command reentry rejected as Busy"), Nested == EResult::Busy);
        TestEqual(TEXT("One callback per request"), Calls, 1);
        TestTrue(TEXT("Empty slot can equip selected life"), Presenter->GetSnapshot().bCanEquip);
        TestTrue(TEXT("Command real equip"), Presenter->ConfirmSelected().Result == EResult::Success);
        TestFalse(TEXT("Already equipped selection disables confirm"), Presenter->GetSnapshot().bCanEquip);
        TestEqual(TEXT("Live reequip no heal"), OtherASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);
        FETRelicListQuery Empty;
        Empty.SearchText = TEXT("no_such_relic");
        Presenter->SetQuery(Empty);
        TestTrue(TEXT("No result can still unequip equipped slot"), Presenter->GetSnapshot().SelectedId.IsNone() &&
            Presenter->GetSnapshot().bCanUnequip && !Presenter->GetSnapshot().bCanEquip);
        TestTrue(TEXT("Unequip does not require selection"), Presenter->UnequipPreviewSlot().Result == EResult::Success);
        Presenter->SetQuery({});
        Presenter->SelectId(LifeId);
        --Presenter->ObservedRevision;
        TestTrue(TEXT("Observed stale revision reaches real rejection"), Presenter->ConfirmSelected().Result == EResult::StalePlan);
        TestFalse(TEXT("Stale reply produces feedback"), Presenter->GetSnapshot().CommandMessage.IsEmpty());
        bPageOpen = false;
        Presenter->RefreshCommandAvailability();
        const int32 BeforeClosed = Calls;
        TestTrue(TEXT("Closed caller cannot submit"), Presenter->ConfirmSelected().Result == EResult::NotReady);
        TestEqual(TEXT("Closed caller invokes no write callback"), Calls, BeforeClosed);
        bPageOpen = true;
        Presenter->SetCommands(
            FETRelicCanSubmit::CreateLambda([&](const UETRelicEquipmentComponent* Target)
            { return Access->CanCommitFor(Target); }),
            FETRelicSubmit::CreateLambda([&](UETRelicEquipmentComponent*, EETRelicSlot, FName, uint64)
            {
                Presenter->Shutdown();
                return FETRelicInventoryReply{EResult::NotReady};
            }));
        TestTrue(TEXT("Callback may shut down observation safely"),
            Presenter->ConfirmSelected().Result == EResult::NotReady);
        TestTrue(TEXT("Shutdown cannot receive old command feedback"),
            Presenter->GetSnapshot().State == EETRelicPanelState::Unavailable &&
            Presenter->GetSnapshot().CommandMessage.IsEmpty());
        Presenter->SetCommands({}, {});
        Access->Close();
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    }
    Presenter->Shutdown();
```

## 4. 正常构建与索引

先保存全部十份文件，UE保持关闭。PowerShell逐条执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

脚本完成Editor Build / 三索引，Game Build也通过后Reload CMake。仍Core76 / UI29 / Editor3；公开UI委托使用Core类型，所以EmberTrial移至UI的PublicDependency，只保留一份，没有Core反向依赖UI。失败发第一条error及前后约10行，不使用Live Coding加载这些新增反射绑定。

## 5. WBP_ETRelicPanel Designer（逐项）

打开`/Game/EmberTrial/ui/Inventory/WBP_ETRelicPanel`，父类仍ETRelicPanelWidget，Graph不添加OnClicked业务。

### 5.1 留出布局空间

1. Hierarchy选根部**PanelSize（Size Box）**，Details搜索`Width Override`，勾选并填**720**；`Height Override`勾选填**620**。原PauseScale仍会按小窗口整体缩小。
2. 找到`BodyColumns → DetailsColumn`。原DetailNameText / Description / SlotText / SlotButtons / PreviewText保留。PreviewText仍Fill1 / 自动换行；新命令区域是它的下方兄弟，**不能成为PreviewText或Button的子级**。

### 5.2 加入四个绑定控件

1. Palette搜索**Vertical Box**，拖到Hierarchy的DetailsColumn内最下方、PreviewText后面。命名**CommandContent**，勾**Is Variable**。Vertical Box Slot：Size=Auto、Horizontal Alignment=Fill、Padding Top=12，其余0；Designer Visibility先Visible，运行时C++控制整区显示。
2. 在CommandContent内放**Horizontal Box**，命名CommandButtons（不绑定、可不勾变量）。其Vertical Box Slot Auto / Fill，Padding0。
3. 在CommandButtons内放两个**Button**，顺序与名称精确为：

| 外层控件名 | 类型与Slot | 子Text（只显示、不绑定） |
| --- | --- | --- |
| ConfirmEquipButton | Button，Is Variable；Horizontal Box Slot Fill1，Right Padding4 | “确认装备”，字号16，Button Slot Padding左右8 / 上下10 |
| UnequipButton | Button，Is Variable；Horizontal Box Slot Fill1，Left Padding4 | “卸下该槽”，字号16，同样Padding |

4. 两Button均Is Focusable=True，Click Method / Press Method均**Down and Up**。不要给按钮创建蓝图点击事件；子Text不使用外层Button名字。
5. 在CommandContent内、CommandButtons下方放**Text Block**，命名**CommandResultText**，勾Is Variable。字号16，沿用中文字体，Auto Wrap Text=True；Vertical Box Slot Auto / Fill、Top Padding8，其余0；占位“选择目标槽位与遗物后确认。”，无Text Binding。

```text
DetailsColumn（原）
  原名称 / 说明 / SlotText / SlotButtons
  PreviewText（原，Fill1）
  CommandContent（VerticalBox，新必需绑定）
    CommandButtons（HorizontalBox）
      ConfirmEquipButton → Text“确认装备”
      UnequipButton      → Text“卸下该槽”
    CommandResultText（TextBlock）
```

6. 原11绑定保留，新增4绑定，总**15**。Class Defaults保持Is Focusable=False、Tick Frequency=Never；父类DisableNativeTick保持。Compile → Save，required widget binding应消失，资产本项目Valid；不清缓存，不删除历史条目资产。

## 6. WBP_PauseMenu嵌入面板绑定

1. 打开`/Game/EmberTrial/ui/Menu/WBP_PauseMenu`。Hierarchy展开原PauseScale → PauseColumns，选中右侧那份**WBP_ETRelicPanel实例**，不是左侧原按钮列。
2. 将该实例命名为精确的**RelicPanel**并勾**Is Variable**。它的类型必须是WBP_ETRelicPanel / ETRelicPanelWidget，仍是PauseActionsSize右侧兄弟。名称原本正确也需要核对Is Variable；不创建第二份面板。
3. 原BTN_Resume / MainMenuButton / StatusText三绑定保留，现在第四绑定为RelicPanel。父类仍ETPauseWidget、Is Focusable=True / Tick Never；PauseScale的Scale To Fit / Down Only与原列宽 / 边距保持。
4. Compile → Save，两份Widget都无绑定错误；Save All。Controller仍单UIBootstrap / 单FailureFlow，Class与地图配置不改。若有“树未就绪”旧校验消息，先Compile-Save再校验，不以旧消息判定本轮失败。

## 7. 三项相关自动化

非PIE的Output Log / Cmd，依次运行，每项完成后再下一项：

```text
Automation RunTests EmberTrial.UI.Relics.Presenter
Automation RunTests EmberTrial.M3.Interaction.RelicAltar
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
```

应都Success。Presenter扩原专项，使用真实World / Access / 库存与GE，验证只读拒绝、确认 / 卸装、空搜索、重入 / 过期和结束后的拒绝；夹具中的lambda不是实际Flow，不把它宣称为创建了UMG或验证了IsInViewport。原Altar检查C1上下文，原事务检查库存 / 身体协议；不跑全32项。

## 8. 真实分组检查（5–10分钟）

不搭测试蓝图，不抄长日志，不改正式数值 / AI。只用现有场景、页面与既有PIE命令；日期时分与结果反馈即可。

### 8.1 确认与真实结果

1. 正式地图Play，新GI应空槽、生命100。到祭坛旁E打开，选“比较槽 A”并选择生息余烬；先观察预览120，真实HUD仍100/100。
2. 点击**确认装备**：页面保持暂停，出现成功提示 / 槽A徽记，HUD**100/120**，没有免费治疗；同件确认按钮禁用。切“比较槽 B”、选砺锋残片确认，攻击比较30、槽B徽记出现。
3. 关页，对仍存活训练目标左键一次应真实30。再E开页，槽A点击**卸下该槽**，当前生命只夹限；再装备生息余烬不治疗。槽B保持，空槽卸装按钮禁用。原不允许的跨槽重复方案不能确认。

### 8.2 查询与焦点

1. 在祭坛页搜索一个无结果字符串，已装备目标槽仍可卸装，确认按钮禁用；清空搜索恢复列表。一次鼠标确认，再用Tab聚焦确认或卸装按钮，Enter / Space执行一次，页面不退出。
2. 搜索框内P为字符，Esc离开编辑后P继续；关闭后移动 / 转向直接恢复，无补跳 / 攻击。普通P页**整个命令区隐藏**，仍可搜索比较；反复开关3轮，保持一份页面。

### 8.3 来源与世界重建

1. **保持祭坛页打开**，从编辑器Output Log / Cmd执行一次`DestroyAll ETRelicAltar`（仅PIE运行中）。应沿B自动关页 / 恢复。Stop → Play祭坛恢复；不在Outliner删除持久实例。
2. 装回两件，返回游戏，原4伤害或敌人致死后Retry：新身体**120/120、Bonus5**，治疗3、体力100；再从祭坛页返回菜单→Continue，装备保持、新身体按最终上限恢复；新一份页面可确认 / 卸装，旧页面无提交日志。
3. 最后Stop → Play新GI空槽100 / 原25、E-P与移动短查，Stop；两Widget Save All。没有临时按键 / 默认属性要清理，不重复完整战斗矩阵。

Core负责“条件变化后再次确认时重验”，本章没有承诺页面持续Tick追踪编辑器瞬移 / Disabled / 遮挡变化；这些拒绝由C1专项覆盖。真实来源销毁、切图 / 关闭则主动撤销上下文；普通P的开放行为始终只读。

## 9. 简短反馈

```text
M3-04C2 完成 / 卡在第__节第__步
十旧UI模块文件；Editor Build / Core76-UI29-Editor3索引 / Game Build：
Panel15绑定 / 新按钮与提示 / Pause第四RelicPanel绑定；Compile-Save-Valid：
Presenter / RelicAltar / EquipmentTransactions各一次：
比较无副作用 / 真实100-120与30 / 重复拒绝 / 装卸不治疗：
空搜索仍卸装 / Tab-Enter-Space / P焦点与无补播 / 普通P只读：
来源销毁关闭 / 带装备Retry与Continue新身体 / Stop新GI：
Source / 两资产Save All，默认值保持；失败或布局问题（无填无）：
完成时分 / 手动代码 / Designer / 检查 / 构建等待：
```

## 10. 本批 Commit / Push（固定放在最后）

C1实现已在本地e659e8c，当前先提交C1验收收尾、本章新指南与状态文档；C2正式Source / 资产尚未实施，忽略Artifacts里的候选不Stage。

- 当前文档英文：`docs: verify altar access and prepare relic confirmation UI`
- 中文译文：`docs: 完成祭坛换装上下文验收并准备遗物确认界面`

实施并检查通过后，本章实际范围为：

```text
EmberTrial/Source/EmberTrialUI/EmberTrialUI.Build.cs
EmberTrial/Source/EmberTrialUI/Public/Inventory/ETRelicPresenter.h
EmberTrial/Source/EmberTrialUI/Private/Inventory/ETRelicPresenter.cpp
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETRelicPanelWidget.h
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETRelicPanelWidget.cpp
EmberTrial/Source/EmberTrialUI/Public/Widgets/ETPauseWidget.h
EmberTrial/Source/EmberTrialUI/Private/Widgets/ETPauseWidget.cpp
EmberTrial/Source/EmberTrialUI/Public/Flow/ETFailureFlowComponent.h
EmberTrial/Source/EmberTrialUI/Private/Flow/ETFailureFlowComponent.cpp
EmberTrial/Source/EmberTrialUI/Tests/ETRelicPresenterTests.cpp
EmberTrial/Content/EmberTrial/ui/Inventory/WBP_ETRelicPanel.uasset
EmberTrial/Content/EmberTrial/ui/Menu/WBP_PauseMenu.uasset
Docs/M3_04C2_RelicConfirmationUI.md
当次Reports / AssetManifest / Progress / Backlog / 路线等实际差异
```

- 实现英文：`feat: confirm relic equipment through the altar UI`
- 中文译文：`feat: 通过祭坛界面确认遗物装备与卸装`

Stage最新内容 → Review → Commit / Push由你操作；源码已Stage后继续修改需再次Stage。日志 / Raw / 候选 / 缓存 / 包留本地，未变化资产不强制重存。
