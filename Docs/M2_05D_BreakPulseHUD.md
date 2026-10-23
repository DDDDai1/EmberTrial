# M2-05D：破势冷却HUD与模块收口

2026-10-23，固定UE5.5.4。前置[C2-02范围破势已验收](Reports/M2_05C2_02_Verification.md)，无阻断项。本章为**待手动实施**：五新 / 五旧共10份Source，一份新WBP及原Controller装配。完整代码与精确替换见下文；助手仅在忽略目录准备候选，未改实际Source / UE资产，未代Build / 自动化 / PIE或操作Git。

预计手动90–140分钟：代码50–80、Designer / 装配25–45、检查8–12；构建等待另列。原D60–90为粗估，现纳入只读快照、事件 / 可见性Timer和真实生命周期专项后增加30–50分钟，使用M2既有缓冲，不再叠加全项目预算。不要求做满预算，按实耗反馈。

## 1. 做完后的面板、数据归属与边界

左上方旧生命 / 体力 / 治疗次数下方新增一张破势卡：

| 状态 | 标题 / 数值 / 条 |
| --- | --- |
| 出生、能力已授予、无冷却 | “破势（F）· 冷却就绪”；“地面站定后按 F”；条满 |
| 正式F施放中 | “施放中”；真实GAS剩余秒数；条逐渐回满 |
| 动画结束或受击取消但仍冷却 | “冷却中”；一位小数的剩余时间；不退款 / 不重置 |
| 冷却GE自然结束 | 下一游戏Tick回“冷却就绪”，周期刷新Timer停止 |
| 角色死亡 | 标题“角色已死亡”，内容隐藏，刷新停止 |
| 无Pawn / 不支持 / 未Ready / 无能力Spec | 各自明确状态，不凭缓存假装冷却就绪 |

**冷却就绪只描述冷却，不等于CanActivate成功**：移动、空中、其他动作 / 暂停仍可能拒绝F。本面板不把所有施放条件再复制成另一套规则，也不新增按钮 / 点击施放、圆形图标素材、VFX或音效。

- **GAS / GA**继续拥有6秒Duration GE与准确生效通知；本批不改范围300 / Poise60 / Cost或原Tags。
- **Character公开只读函数**核对自己授予的Spec / ASC归属，读取真实剩余 / 总时长；不授予、不播放、不修改GE。
- **Presenter**观察当前本地Pawn、Ready与3个GAS Tag；事件延后一次NextTick合并，避免读GE / 取消过程的中间状态。Ready广播早于BeginPlay尾部能力授予，也通过这一次延后解决，不永久轮询未授予状态。
- **周期Timer**仅面板可见、当前身体Ready且冷却活动时运行，每.10游戏秒读一次真实GAS；隐藏 / 死亡 / NoPawn / EndPlay / Shutdown停止。Pause冻结世界Timer，不用现实时间减数。隐藏时允许事件快照更新，但不周期刷新。
- **Widget**只渲染快照与报告根可见性，无Tick / Text Bind / 自有倒计时；Bootstrap先停旧View再建新View，重建不更换Presenter或重置冷却。

当前一个Presenter对应Bootstrap的一份正式View；根Widget隐藏 / 移除会停周期刷新。窗口被别的应用遮住等桌面可见性不在本策略范围。当前能力在BeginPlay同步授予，本批不承诺任意未来异步授予都会自动通知；若以后改为异步授予，需要同时扩展能力就绪事件。

## 2. 准备与10份Source

1. Stop PIE → Save All → **关闭UE**，CLion保存。反射头与模块依赖有变，用正常UBT / UHT，不Live Coding。
2. 以下路径均相对`E:\UE_Project\Demo\EmberTrial\Source`。新文件只有5份；头文件generated.h必须最后include，旧文件只按第5节改，保留原代码。

| 类型 | 相对路径 | 操作 |
| --- | --- | --- |
| 新 | EmberTrialUI/Public/Presenters/ETBreakPulsePresenter.h | 第3.1全文 |
| 新 | EmberTrialUI/Private/Presenters/ETBreakPulsePresenter.cpp | 第3.2全文 |
| 新 | EmberTrialUI/Public/Widgets/ETBreakPulseHUDWidget.h | 第4.1全文 |
| 新 | EmberTrialUI/Private/Widgets/ETBreakPulseHUDWidget.cpp | 第4.2全文 |
| 新 | EmberTrialUI/Private/Tests/ETBreakPulseHUDTests.cpp | 第6全文 |
| 旧 | EmberTrial/Public/Characters/EmberTrialCharacter.h | 只读接口 / 最小friend |
| 旧 | EmberTrial/Private/Characters/EmberTrialCharacter.cpp | 只读接口实现 |
| 旧 | EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h | 新View / Presenter接口与所有权 |
| 旧 | EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp | 装配 / 退出 / 层级 |
| 旧 | EmberTrialUI/EmberTrialUI.Build.cs | 私有GameplayTags直接依赖 |

Core不依赖UI；UI公开头不暴露GAS类型定义，GameplayTags仅为私有直接依赖。Editor通用BindWidget校验已能检查新面板，不改Editor代码、Target、uproject或旧18项测试。当前真实规模仍139 / 75cpp / 18测试，后文144 / 78 / 19仅实施后的预期。

## 3. 新增破势Presenter

### 3.1 `EmberTrialUI/Public/Presenters/ETBreakPulsePresenter.h`

新文件全文：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TimerManager.h"
#include "ETBreakPulsePresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETUILocalPlayerSubsystem;
class UWorld;
struct FGameplayTag;

UENUM(BlueprintType)
enum class EETBreakPulseViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Dead,
    Ready
};

USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETBreakPulseViewData
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    EETBreakPulseViewState State = EETBreakPulseViewState::Unavailable;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    FString PawnName;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    bool bCasting = false;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    bool bCoolingDown = false;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Remaining = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Duration = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float ReadyFraction = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FETBreakPulseViewChanged, const FETBreakPulseViewData&,
                                            Data);

// 一个本地身体 / 一个HUD View；只观察GAS，不决定施放或修改冷却。
UCLASS(BlueprintType)
class EMBERTRIALUI_API UETBreakPulsePresenter : public UObject
{
    GENERATED_BODY()
public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();
    void SetPresentationEnabled(bool bEnabled);
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETBreakPulseViewData GetSnapshot() const;
    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETBreakPulseViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    friend class FETBreakPulseHUDTest;
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void StopTimers();
    void RebindObservedPawn(const TCHAR* Reason);
    void RequestDeferredRefresh();
    void HandleDeferredRefresh(uint64 Serial);
    void HandlePeriodicRefresh(uint64 Serial);
    void RefreshSnapshot(const TCHAR* Reason);
    void SyncPeriodicRefresh(const FETBreakPulseViewData& Data);
    FETBreakPulseViewData BuildSnapshot() const;
    void HandleTagChanged(FGameplayTag Tag, int32 Count,
                          TWeakObjectPtr<UAbilitySystemComponent> Source, uint64 Serial);
    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);
    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    TWeakObjectPtr<UWorld> TimerWorld;
    FDelegateHandle CooldownChangedHandle;
    FDelegateHandle CastingChangedHandle;
    FDelegateHandle DeathChangedHandle;
    FTimerHandle DeferredRefreshTimer;
    FTimerHandle PeriodicRefreshTimer;
    FETBreakPulseViewData Snapshot;
    uint64 ObservationSerial = 0;
    bool bObserving = false;
    bool bPresentationEnabled = false;
};
```

### 3.2 `EmberTrialUI/Private/Presenters/ETBreakPulsePresenter.cpp`

新文件全文：

```cpp
#include "Presenters/ETBreakPulsePresenter.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogETBreakPulsePresenter, Log, All);

void UETBreakPulsePresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
{
    StopObservingInternal(false);
    if (!IsValid(Subsystem))
    {
        RefreshSnapshot(TEXT("MissingSubsystem"));
        return;
    }
    bObserving = true;
    BoundSubsystem = Subsystem;
    Subsystem->OnObservedPawnChanged.AddUniqueDynamic(this, &ThisClass::HandleObservedPawnChanged);
    RebindObservedPawn(TEXT("StartSnapshot"));
}

void UETBreakPulsePresenter::Shutdown()
{
    StopObservingInternal(true);
}

void UETBreakPulsePresenter::BeginDestroy()
{
    StopObservingInternal(false);
    Super::BeginDestroy();
}

FETBreakPulseViewData UETBreakPulsePresenter::GetSnapshot() const
{
    return Snapshot;
}

void UETBreakPulsePresenter::SetPresentationEnabled(bool bEnabled)
{
    bPresentationEnabled = bEnabled;
    RefreshSnapshot(TEXT("PresentationChanged"));
}

// ---- 身份先失效，再释放旧来源的三个Tag委托与自己的Timer ----
void UETBreakPulsePresenter::StopObservingInternal(bool bNotify)
{
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(this, &ThisClass::HandleObservedPawnChanged);
    }
    ClearPawnBindings();
    BoundSubsystem.Reset();
    // 保留旧快照到下一次Refresh比较，失败重绑也能发布Ready→Unavailable；不清View订阅。
    if (bNotify)
    {
        RefreshSnapshot(TEXT("Shutdown"));
    }
}

void UETBreakPulsePresenter::StopTimers()
{
    const bool bHadPeriodic = PeriodicRefreshTimer.IsValid();
    if (UWorld* World = TimerWorld.Get())
    {
        World->GetTimerManager().ClearTimer(DeferredRefreshTimer);
        World->GetTimerManager().ClearTimer(PeriodicRefreshTimer);
    }
    DeferredRefreshTimer.Invalidate();
    PeriodicRefreshTimer.Invalidate();
    if (bHadPeriodic)
    {
        UE_LOG(LogETBreakPulsePresenter, Display,
               TEXT("[M2-05D] RefreshTimer Presenter=%s Active=0"), *GetName());
    }
}

void UETBreakPulsePresenter::ClearPawnBindings()
{
    ++ObservationSerial;
    StopTimers();
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->UnregisterGameplayTagEvent(CooldownChangedHandle, ETGameplayTags::Cooldown_BreakPulse);
        ASC->UnregisterGameplayTagEvent(CastingChangedHandle,
                                        ETGameplayTags::State_CastingBreakPulse);
        ASC->UnregisterGameplayTagEvent(DeathChangedHandle, ETGameplayTags::State_Dead);
    }
    CooldownChangedHandle.Reset();
    CastingChangedHandle.Reset();
    DeathChangedHandle.Reset();
    if (AEmberTrialCharacter* Body = BoundCharacter.Get())
    {
        Body->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleGameplayReadyChanged);
    }
    BoundCharacter.Reset();
    BoundASC.Reset();
    TimerWorld.Reset();
}

void UETBreakPulsePresenter::RebindObservedPawn(const TCHAR* Reason)
{
    ClearPawnBindings();
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    AEmberTrialCharacter* Body =
        Subsystem ? Cast<AEmberTrialCharacter>(Subsystem->GetObservedPawn()) : nullptr;
    if (IsValid(Body) && Body->GetCombatTeam() == EETCombatTeam::Player)
    {
        BoundCharacter = Body;
        TimerWorld = Body->GetWorld();
        Body->OnGameplayReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleGameplayReadyChanged);
        if (UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent())
        {
            BoundASC = ASC;
            CooldownChangedHandle =
                ASC->RegisterGameplayTagEvent(ETGameplayTags::Cooldown_BreakPulse)
                    .AddUObject(this, &ThisClass::HandleTagChanged, BoundASC, ObservationSerial);
            CastingChangedHandle =
                ASC->RegisterGameplayTagEvent(ETGameplayTags::State_CastingBreakPulse)
                    .AddUObject(this, &ThisClass::HandleTagChanged, BoundASC, ObservationSerial);
            DeathChangedHandle =
                ASC->RegisterGameplayTagEvent(ETGameplayTags::State_Dead)
                    .AddUObject(this, &ThisClass::HandleTagChanged, BoundASC, ObservationSerial);
        }
    }
    const uint64 Serial = ObservationSerial;
    RefreshSnapshot(Reason);
    if (Serial == ObservationSerial && bObserving)
    {
        RequestDeferredRefresh();
    }
}

void UETBreakPulsePresenter::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (bObserving)
    {
        RebindObservedPawn(TEXT("PawnChanged"));
    }
}

void UETBreakPulsePresenter::HandleGameplayReadyChanged(AEmberTrialCharacter* Character,
                                                        bool bReady)
{
    if (!bObserving || Character != BoundCharacter.Get())
    {
        return;
    }
    // Ready广播早于BeginPlay最后的能力授予；等一次NextTick，不永久轮询未授予状态。
    if (bReady)
    {
        RequestDeferredRefresh();
    }
    else
    {
        RefreshSnapshot(TEXT("ReadyWithdrawn"));
    }
}

void UETBreakPulsePresenter::HandleTagChanged(FGameplayTag Tag, int32 Count,
                                              TWeakObjectPtr<UAbilitySystemComponent> Source,
                                              uint64 Serial)
{
    (void)Tag;
    (void)Count;
    if (!bObserving || Serial != ObservationSerial || !Source.IsValid() || Source != BoundASC)
    {
        return;
    }
    // Tag回调可能位于GE应用 / 取消的中间；合并到下一游戏Tick再查最终Spec / 时长。
    RequestDeferredRefresh();
}

void UETBreakPulsePresenter::RequestDeferredRefresh()
{
    UWorld* World = TimerWorld.Get();
    if (!bObserving || !IsValid(World) || World->bIsTearingDown || DeferredRefreshTimer.IsValid())
    {
        return;
    }
    DeferredRefreshTimer = World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &ThisClass::HandleDeferredRefresh, ObservationSerial));
}

void UETBreakPulsePresenter::HandleDeferredRefresh(uint64 Serial)
{
    if (!bObserving || Serial != ObservationSerial)
    {
        return;
    }
    DeferredRefreshTimer.Invalidate();
    RefreshSnapshot(TEXT("EventSnapshot"));
}

void UETBreakPulsePresenter::HandlePeriodicRefresh(uint64 Serial)
{
    if (bObserving && Serial == ObservationSerial)
    {
        RefreshSnapshot(TEXT("CooldownRefresh"));
    }
}

// ---- 快照可用性不等于CanActivate；冷却就绪不能承诺移动 / 忙碌时允许施放 ----
FETBreakPulseViewData UETBreakPulsePresenter::BuildSnapshot() const
{
    FETBreakPulseViewData Data;
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    if (!bObserving || !IsValid(Subsystem))
    {
        return Data;
    }
    APawn* Pawn = Subsystem->GetObservedPawn();
    if (!IsValid(Pawn))
    {
        Data.State = EETBreakPulseViewState::NoPawn;
        return Data;
    }
    Data.PawnName = Pawn->GetName();
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(Pawn);
    if (!IsValid(Body) || Body->GetCombatTeam() != EETCombatTeam::Player)
    {
        Data.State = EETBreakPulseViewState::UnsupportedPawn;
        return Data;
    }
    if (!Body->IsGameplayReady())
    {
        Data.State = EETBreakPulseViewState::WaitingForGameplayReady;
        return Data;
    }
    if (Body->IsCombatDead())
    {
        Data.State = EETBreakPulseViewState::Dead;
        return Data;
    }
    UAbilitySystemComponent* ASC = BoundASC.Get();
    if (Body != BoundCharacter.Get() || !IsValid(ASC) || Body->GetAbilitySystemComponent() != ASC ||
        !Body->GetBreakPulseCooldownSnapshot(Data.bCoolingDown, Data.Remaining, Data.Duration))
    {
        return Data;
    }
    Data.State = EETBreakPulseViewState::Ready;
    Data.bCasting = ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse);
    Data.ReadyFraction = Data.bCoolingDown ? 1.0f - Data.Remaining / Data.Duration : 1.0f;
    Data.ReadyFraction = FMath::Clamp(Data.ReadyFraction, 0.0f, 1.0f);
    return Data;
}

void UETBreakPulsePresenter::SyncPeriodicRefresh(const FETBreakPulseViewData& Data)
{
    UWorld* World = TimerWorld.Get();
    const bool bNeeded = bObserving && bPresentationEnabled &&
                         Data.State == EETBreakPulseViewState::Ready && Data.bCoolingDown &&
                         IsValid(World) && !World->bIsTearingDown;
    if (!bNeeded)
    {
        if (PeriodicRefreshTimer.IsValid())
        {
            if (IsValid(World))
            {
                World->GetTimerManager().ClearTimer(PeriodicRefreshTimer);
            }
            PeriodicRefreshTimer.Invalidate();
            UE_LOG(LogETBreakPulsePresenter, Display,
                   TEXT("[M2-05D] RefreshTimer Presenter=%s Active=0"), *GetName());
        }
        return;
    }
    if (!World->GetTimerManager().IsTimerActive(PeriodicRefreshTimer))
    {
        World->GetTimerManager().SetTimer(
            PeriodicRefreshTimer,
            FTimerDelegate::CreateUObject(this, &ThisClass::HandlePeriodicRefresh,
                                          ObservationSerial),
            .10f, true);
        UE_LOG(LogETBreakPulsePresenter, Display,
               TEXT("[M2-05D] RefreshTimer Presenter=%s Active=1 Interval=0.10"), *GetName());
    }
}

void UETBreakPulsePresenter::RefreshSnapshot(const TCHAR* Reason)
{
    const FETBreakPulseViewData NewData = BuildSnapshot();
    SyncPeriodicRefresh(NewData);
    if (Snapshot.State == NewData.State && Snapshot.PawnName == NewData.PawnName &&
        Snapshot.bCasting == NewData.bCasting && Snapshot.bCoolingDown == NewData.bCoolingDown &&
        FMath::IsNearlyEqual(Snapshot.Remaining, NewData.Remaining, .001f) &&
        FMath::IsNearlyEqual(Snapshot.Duration, NewData.Duration, .001f))
    {
        return;
    }
    const bool bStateChanged =
        Snapshot.State != NewData.State || Snapshot.PawnName != NewData.PawnName ||
        Snapshot.bCasting != NewData.bCasting || Snapshot.bCoolingDown != NewData.bCoolingDown;
    Snapshot = NewData;
    if (bStateChanged)
    {
        UE_LOG(LogETBreakPulsePresenter, Display,
               TEXT("[M2-05D] Snapshot Presenter=%s State=%s Pawn=%s Casting=%d Cooling=%d "
                    "Remaining=%.2f Reason=%s"),
               *GetName(),
               *StaticEnum<EETBreakPulseViewState>()->GetNameStringByValue(
                   static_cast<int64>(Snapshot.State)),
               *Snapshot.PawnName, Snapshot.bCasting ? 1 : 0, Snapshot.bCoolingDown ? 1 : 0,
               Snapshot.Remaining, Reason);
    }
    const FETBreakPulseViewData Published = Snapshot;
    // 广播可能关View / 重启观察；之后不再改实例字段或启动旧一轮Timer。
    OnViewDataChanged.Broadcast(Published);
}
```

## 4. 新增破势Widget基类

原生父类已有DisableNativeTick，避免之前的Native Tick警告。四个BindWidget精确对应第8节Designer名称 / 类型；不用将它们改为Optional来绕过漏配。

### 4.1 `EmberTrialUI/Public/Widgets/ETBreakPulseHUDWidget.h`

新文件全文：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETBreakPulsePresenter.h"
#include "ETBreakPulseHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UVerticalBox;

// 一份Presenter对应一份正式View；无Tick / Binding，不改变GAS冷却。
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETBreakPulseHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void StartObserving(UETBreakPulsePresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETBreakPulseViewData& Data);
    UFUNCTION()
    void HandleViewDataChanged(const FETBreakPulseViewData& Data);
    UFUNCTION()
    void HandleVisibilityChanged(ESlateVisibility InVisibility);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> BreakPulseStatusText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> BreakPulseValuesText;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> BreakPulseBar;
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> BreakPulseContent;

    TWeakObjectPtr<UETBreakPulsePresenter> BoundPresenter;
    bool bObserving = false;
};
```

### 4.2 `EmberTrialUI/Private/Widgets/ETBreakPulseHUDWidget.cpp`

新文件全文：

```cpp
#include "Widgets/ETBreakPulseHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Internationalization/Text.h"

#define LOCTEXT_NAMESPACE "ETBreakPulseHUD"

void UETBreakPulseHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (IsDesignTime())
    {
        return;
    }
    OnVisibilityChanged.AddUniqueDynamic(this, &ThisClass::HandleVisibilityChanged);
    const UETBreakPulsePresenter* Presenter = BoundPresenter.Get();
    ApplyViewData(Presenter ? Presenter->GetSnapshot() : FETBreakPulseViewData{});
}

void UETBreakPulseHUDWidget::NativeDestruct()
{
    StopObserving();
    OnVisibilityChanged.RemoveDynamic(this, &ThisClass::HandleVisibilityChanged);
    Super::NativeDestruct();
}

void UETBreakPulseHUDWidget::StartObserving(UETBreakPulsePresenter* Presenter)
{
    StopObserving();
    if (IsDesignTime())
    {
        return;
    }
    if (!IsValid(Presenter))
    {
        ApplyViewData(FETBreakPulseViewData{});
        return;
    }
    bObserving = true;
    BoundPresenter = Presenter;
    Presenter->OnViewDataChanged.AddUniqueDynamic(this, &ThisClass::HandleViewDataChanged);
    // 晚加入 / 重建时先重读真实剩余时间，再显示；不从缓存数字自行倒计时。
    Presenter->SetPresentationEnabled(IsVisible());
    if (bObserving && BoundPresenter.Get() == Presenter)
    {
        ApplyViewData(Presenter->GetSnapshot());
    }
}

void UETBreakPulseHUDWidget::StopObserving()
{
    bObserving = false;
    UETBreakPulsePresenter* Presenter = BoundPresenter.Get();
    BoundPresenter.Reset();
    if (IsValid(Presenter))
    {
        Presenter->OnViewDataChanged.RemoveDynamic(this, &ThisClass::HandleViewDataChanged);
        Presenter->SetPresentationEnabled(false);
    }
}

void UETBreakPulseHUDWidget::HandleVisibilityChanged(ESlateVisibility InVisibility)
{
    (void)InVisibility;
    if (bObserving && BoundPresenter.IsValid())
    {
        BoundPresenter->SetPresentationEnabled(IsVisible());
    }
}

void UETBreakPulseHUDWidget::HandleViewDataChanged(const FETBreakPulseViewData& Data)
{
    if (bObserving && BoundPresenter.IsValid())
    {
        ApplyViewData(Data);
    }
}

void UETBreakPulseHUDWidget::ApplyViewData(const FETBreakPulseViewData& Data)
{
    if (!BreakPulseStatusText || !BreakPulseValuesText || !BreakPulseBar || !BreakPulseContent)
    {
        return;
    }
    if (Data.State == EETBreakPulseViewState::Ready)
    {
        BreakPulseContent->SetVisibility(ESlateVisibility::HitTestInvisible);
        BreakPulseBar->SetPercent(Data.ReadyFraction);
        BreakPulseStatusText->SetText(Data.bCasting ? LOCTEXT("Casting", "破势（F）· 施放中")
                                      : Data.bCoolingDown
                                          ? LOCTEXT("Cooling", "破势（F）· 冷却中")
                                          : LOCTEXT("Ready", "破势（F）· 冷却就绪"));
        if (Data.bCoolingDown)
        {
            // Tag撤销前至少显示0.1，避免读成“0.0秒但仍不能用”。
            const float Seconds =
                FMath::Max(.1f, FMath::CeilToFloat(Data.Remaining * 10.0f) / 10.0f);
            FNumberFormattingOptions Options;
            Options.SetMinimumFractionalDigits(1);
            Options.SetMaximumFractionalDigits(1);
            BreakPulseValuesText->SetText(FText::Format(LOCTEXT("Remaining", "剩余 {0} 秒"),
                                                        FText::AsNumber(Seconds, &Options)));
        }
        else
        {
            BreakPulseValuesText->SetText(LOCTEXT("StationaryHint", "地面站定后按 F"));
        }
        return;
    }
    BreakPulseContent->SetVisibility(ESlateVisibility::Collapsed);
    BreakPulseBar->SetPercent(0.0f);
    BreakPulseValuesText->SetText(FText::GetEmpty());
    switch (Data.State)
    {
    case EETBreakPulseViewState::Dead:
        BreakPulseStatusText->SetText(LOCTEXT("Dead", "破势（F）· 角色已死亡"));
        break;
    case EETBreakPulseViewState::NoPawn:
        BreakPulseStatusText->SetText(LOCTEXT("NoPawn", "破势（F）· 暂无受控角色"));
        break;
    case EETBreakPulseViewState::UnsupportedPawn:
        BreakPulseStatusText->SetText(LOCTEXT("Unsupported", "当前角色不提供破势技能"));
        break;
    case EETBreakPulseViewState::WaitingForGameplayReady:
        BreakPulseStatusText->SetText(LOCTEXT("Waiting", "破势（F）· 角色准备中"));
        break;
    default:
        BreakPulseStatusText->SetText(LOCTEXT("Unavailable", "破势（F）· 暂不可用"));
        break;
    }
}

#undef LOCTEXT_NAMESPACE
```

## 5. 修改五个既有文件

逐文件、逐处操作。原位置都是当前C2-02基线中的唯一锚点；短锚点直接替换，完整函数插入项按文字说明保留原函数。不要覆盖角色已有F / 三段攻击 / 资源 / EndPlay，也不将Bootstrap旧HUD功能删掉。

### 5.1 `EmberTrial/Public/Characters/EmberTrialCharacter.h`

**第1处：UI专项最小friend**。

原位置：

```cpp
    friend class FETBreakPulseRangeTest;
```

替换为：

```cpp
    friend class FETBreakPulseRangeTest;
    friend class FETBreakPulseHUDTest;
```

**第2处：只读冷却公开接口**。

原位置：

```cpp
    bool RequestBreakPulse();
```

替换为：

```cpp
    bool RequestBreakPulse();

    // 冷却快照不等于允许立即施放；不播放 / 不授予 / 不修改GE。
    UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
    bool GetBreakPulseCooldownSnapshot(bool& bOutCooling, float& OutRemaining,
        float& OutDuration) const;
```

### 5.2 `EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

**第1处：只读快照实现**。

原位置：

```cpp
void AEmberTrialCharacter::HandleBreakPulseStarted()
```

在这个函数签名**之前**插入下方完整只读函数；原HandleBreakPulseStarted及其函数体保留，不再复制原签名：

```cpp
bool AEmberTrialCharacter::GetBreakPulseCooldownSnapshot(bool& bOutCooling, float& OutRemaining,
                                                         float& OutDuration) const
{
    bOutCooling = false;
    OutRemaining = 0.0f;
    OutDuration = 0.0f;
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (bAbilitySystemShuttingDown || !IsGameplayReady() || CombatTeam != EETCombatTeam::Player ||
        !IsValid(ASC) || !BreakPulseAbilityHandle.IsValid() || !ASC->AbilityActorInfo.IsValid() ||
        ASC->AbilityActorInfo->OwnerActor.Get() != this ||
        ASC->AbilityActorInfo->AvatarActor.Get() != this)
    {
        return false;
    }
    const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(BreakPulseAbilityHandle);
    const UETGA_BreakPulse* Ability = Spec ? Cast<UETGA_BreakPulse>(Spec->Ability) : nullptr;
    if (!IsValid(Ability) || Ability->GetClass() != BreakPulseAbilityClass.Get())
    {
        return false;
    }
    if (!ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse))
    {
        return true;
    }
    float Remaining = 0.0f;
    float Duration = 0.0f;
    Ability->GetCooldownTimeRemainingAndDuration(BreakPulseAbilityHandle,
                                                 ASC->AbilityActorInfo.Get(), Remaining, Duration);
    if (!FMath::IsFinite(Remaining) || !FMath::IsFinite(Duration) || Duration <= 0.0f)
    {
        return false;
    }
    // 到期Tag尚未撤销的极短窗口仍属于Cooling，不能仅按Remaining==0显示就绪。
    bOutCooling = true;
    OutDuration = Duration;
    OutRemaining = FMath::Clamp(Remaining, 0.0f, Duration);
    return true;
}
```

### 5.3 `EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h`

**第1处：Bootstrap前向声明**。

原位置：

```cpp
class UETHealingHUDWidget;
```

替换为：

```cpp
class UETHealingHUDWidget;
class UETBreakPulsePresenter;
class UETBreakPulseHUDWidget;
```

**第2处：Bootstrap公开接口**。

原位置：

```cpp
protected:
    virtual void BeginPlay()
```

替换为：

```cpp
    // ---- 破势冷却HUD：View重建与数据观察分离 ----
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETBreakPulsePresenter* GetBreakPulsePresenter() const;
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartBreakPulseObservation();
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildBreakPulseWidget();
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETBreakPulseHUDWidget* GetBreakPulseWidget() const;

protected:
    virtual void BeginPlay()
```

**第3处：Bootstrap移除声明**。

原位置：

```cpp
    void RemoveHealingWidget();
```

替换为：

```cpp
    void RemoveHealingWidget();
    void RemoveBreakPulseWidget();
```

**第4处：Bootstrap配置与对象所有权**。

原位置：

```cpp
    // EndPlay先封闭入口
```

替换为：

```cpp
    UPROPERTY(Transient)
    TObjectPtr<UETBreakPulsePresenter> BreakPulsePresenter;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETBreakPulseHUDWidget> BreakPulseWidgetClass;
    UPROPERTY(Transient)
    TObjectPtr<UETBreakPulseHUDWidget> BreakPulseWidget;

    // EndPlay先封闭入口
```

### 5.4 `EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp`

**第1处：Bootstrap includes**。

原位置：

```cpp
#include "Widgets/ETHealingHUDWidget.h"
```

替换为：

```cpp
#include "Widgets/ETHealingHUDWidget.h"
#include "Presenters/ETBreakPulsePresenter.h"
#include "Widgets/ETBreakPulseHUDWidget.h"
```

**第2处：Bootstrap HUD相对层级**。

原位置：

```cpp
    constexpr int32 HealingWidgetZOrder = 2;
```

替换为：

```cpp
    constexpr int32 HealingWidgetZOrder = 2;
    constexpr int32 BreakPulseWidgetZOrder = 1;
```

**第3处：BeginPlay启动观察**。

原位置：

```cpp
    RestartHealingObservation();
```

替换为：

```cpp
    RestartHealingObservation();
    RestartBreakPulseObservation();
```

**第4处：BeginPlay创建View**。

原位置：

```cpp
    RebuildHealingWidget();
```

替换为：

```cpp
    RebuildHealingWidget();
    RebuildBreakPulseWidget();
```

**第5处：EndPlay先停新View**。

原位置：

```cpp
    bUIShuttingDown = true;
```

替换为：

```cpp
    bUIShuttingDown = true;
    RemoveBreakPulseWidget();
```

**第6处：EndPlay撤销新Presenter**。

原位置：

```cpp
    if (HealingPresenter)
    {
        HealingPresenter->Shutdown();
```

替换为：

```cpp
    if (BreakPulsePresenter)
    {
        BreakPulsePresenter->Shutdown();
        BreakPulsePresenter = nullptr;
    }
    if (HealingPresenter)
    {
        HealingPresenter->Shutdown();
```

**第7处：文件末尾追加五个破势HUD装配函数**。

在文件最后一个函数结束的右大括号之后、文件末尾追加以下完整函数；不要放入原RemoveHealingWidget函数体：

```cpp
// ---- 破势冷却HUD；不管理施放 / 冷却GE ----
UETBreakPulsePresenter* UETUIBootstrapComponent::GetBreakPulsePresenter() const
{
    return BreakPulsePresenter.Get();
}

void UETUIBootstrapComponent::RestartBreakPulseObservation()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        if (BreakPulsePresenter)
        {
            BreakPulsePresenter->Shutdown();
        }
        return;
    }
    if (!BreakPulsePresenter)
    {
        BreakPulsePresenter = NewObject<UETBreakPulsePresenter>(this);
    }
    BreakPulsePresenter->StartObserving(
        Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>());
}

UETBreakPulseHUDWidget* UETUIBootstrapComponent::GetBreakPulseWidget() const
{
    return BreakPulseWidget.Get();
}

void UETUIBootstrapComponent::RebuildBreakPulseWidget()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    // 先停旧View，再创建新View；Presenter与GAS冷却保持原对象所有权。
    RemoveBreakPulseWidget();
    APlayerController* Controller = ResolveLocalUIController();
    if (!Controller)
    {
        return;
    }
    if (!BreakPulseWidgetClass || !IsValid(BreakPulsePresenter.Get()))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M2-05D] Missing BreakPulseWidgetClass or Presenter Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    BreakPulseWidget = CreateWidget<UETBreakPulseHUDWidget>(Controller, BreakPulseWidgetClass);
    if (!BreakPulseWidget)
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M2-05D] BreakPulse widget creation failed Owner=%s"),
               *GetNameSafe(Controller));
        return;
    }
    if (!BreakPulseWidget->AddToPlayerScreen(BreakPulseWidgetZOrder))
    {
        UE_LOG(LogETUIBootstrap, Warning,
               TEXT("[M2-05D] BreakPulse AddToPlayerScreen failed Owner=%s"),
               *GetNameSafe(Controller));
        RemoveBreakPulseWidget();
        return;
    }
    BreakPulseWidget->StartObserving(BreakPulsePresenter.Get());
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M2-05D] BreakPulse widget created Owner=%s Widget=%s"),
           *GetNameSafe(Controller), *GetNameSafe(BreakPulseWidget.Get()));
}

void UETUIBootstrapComponent::RemoveBreakPulseWidget()
{
    if (!BreakPulseWidget)
    {
        return;
    }
    BreakPulseWidget->StopObserving();
    UE_LOG(LogETUIBootstrap, Display, TEXT("[M2-05D] BreakPulse widget removed Owner=%s Widget=%s"),
           *GetNameSafe(GetOwner()), *GetNameSafe(BreakPulseWidget.Get()));
    BreakPulseWidget->RemoveFromParent();
    BreakPulseWidget = nullptr;
}
```

### 5.5 `EmberTrialUI/EmberTrialUI.Build.cs`

**第1处：UI私有直接GameplayTags依赖**。

原位置：

```cpp
            "GameplayAbilities",
```

替换为：

```csharp
            "GameplayAbilities",
            "GameplayTags",
```

### 5.6 完成后核对

- Core查询只读现有Spec / GE，输出初始化为False / 0 / 0；Tag尚未撤销的到期边缘仍Cooling，避免提前显示就绪。
- Bootstrap BeginPlay增加一次Restart / Rebuild，EndPlay先Remove新View、再Shutdown新Presenter；原四个面板 / FailureFlow不改。
- 四个新Bootstrap接口只服务观察 / 重建，不绑定F。界面ZOrder1与旧HUD5/4/3/2保持相对关系，不覆盖Pause / Failure输入。
- 新测试friend只允许夹具设出生前配置；不公开写冷却的生产接口。保留原自动化，不再加临时Controller蓝图按键。

## 6. 新增一项真实冷却 / Presenter生命周期专项

新建`EmberTrialUI/Private/Tests/ETBreakPulseHUDTests.cpp`，不是Core/Tests。它使用真实世界、保存GA / Montage、GAS、LocalPlayer身份和本项目子系统事件，检查出生Ready早于授予、显示Timer门控、Pause、晚加入 / 重启、旧代次、换身体 / 旧源隔离、死亡、到期和EndPlay。

**文件位置与C1083排查：**完整路径必须为`E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Tests\ETBreakPulseHUDTests.cpp`。若误建在`Source/EmberTrial/Tests`，UBT会把它作为Core模块编译，找不到UI的`Presenters/ETBreakPulsePresenter.h`。应移动整个测试文件到上述UI目录，旧位置不留副本；不能通过给Core添加EmberTrialUI依赖或跨目录相对include修补，否则会破坏UI→Core的单向依赖。2026-10-23已按用户授权直接移动本机误放的文件，内容保持；之后正常运行第7节刷新脚本即可，无需清缓存。

该夹具不创建真实Viewport / 屏幕Widget，也不验证引擎子系统集合的自动实例化；Designer布局和实际Bootstrap装配仍由第10节短查。LocalPlayer使用GEngine作为Within规定的Outer并由强引用持有，不把它注册到真实GameInstance；不Destroy后推进再访问旧ASC。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Presenters/ETBreakPulsePresenter.h"
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETBreakPulseHUDTest, "EmberTrial.UI.BreakPulse.Lifecycle",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETBreakPulseHUDTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* PulseClass = LoadClass<UETGA_BreakPulse>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse.GA_ET_BreakPulse_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved pulse"), PulseClass) || !TestNotNull(TEXT("Quinn"), Quinn) ||
        !TestNotNull(TEXT("Engine for LocalPlayer outer"), GEngine))
    {
        return false;
    }
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    AActor* Floor = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Floor"), Floor))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
    Floor->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(2000, 2000, 20));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEmberTrialCharacter* Body =
        World->SpawnActor<AEmberTrialCharacter>(FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    APlayerController* PC = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("PC"), PC))
    {
        return false;
    }
    auto Configure = [PulseClass, Quinn](AEmberTrialCharacter* Target)
    {
        Target->CombatTeam = EETCombatTeam::Player;
        Target->BreakPulseAbilityClass = PulseClass;
        Target->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Target->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Target->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Target->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Target->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Target->GetCharacterMovement()->bRunPhysicsWithNoController = true;
    };
    Configure(Body);
    // 只构造身份 / 观察夹具，不注册到真实GameInstance，不创建Viewport或屏幕Widget。
    TStrongObjectPtr<ULocalPlayer> LocalPlayer(NewObject<ULocalPlayer>(GEngine));
    LocalPlayer->SwitchController(PC);
    PC->Possess(Body);
    TStrongObjectPtr<UETUILocalPlayerSubsystem> Subsystem(
        NewObject<UETUILocalPlayerSubsystem>(LocalPlayer.Get()));
    Subsystem->PlayerControllerChanged(PC);
    TStrongObjectPtr<UETBreakPulsePresenter> Presenter(NewObject<UETBreakPulsePresenter>());
    Presenter->StartObserving(Subsystem.Get());
    Presenter->SetPresentationEnabled(true);
    TestTrue(TEXT("Before BeginPlay is waiting"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::WaitingForGameplayReady);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    auto Advance = [&Fixture](float Seconds)
    {
        for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
        {
            if (!Fixture.TickTestWorld(.01f))
            {
                return false;
            }
        }
        return true;
    };
    Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("Ready callback waits for final ability grant"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Ready);
    TestFalse(TEXT("Idle has no periodic timer"), Presenter->PeriodicRefreshTimer.IsValid());
    TestEqual(TEXT("Idle recharge fraction"), Presenter->GetSnapshot().ReadyFraction, 1.0f);
    if (!TestTrue(TEXT("Real F begins"), Body->RequestBreakPulse()) || !Advance(.32f))
    {
        return false;
    }
    TestTrue(TEXT("Cooling observed"), Presenter->GetSnapshot().bCoolingDown);
    TestTrue(TEXT("Casting observed"), Presenter->GetSnapshot().bCasting);
    TestTrue(TEXT("Visible cooling starts periodic refresh"),
             World->GetTimerManager().IsTimerActive(Presenter->PeriodicRefreshTimer));
    const float BeforePause = Presenter->GetSnapshot().Remaining;
    APlayerState* Pauser = PC->PlayerState;
    if (!IsValid(Pauser))
    {
        Pauser = World->SpawnActor<APlayerState>();
    }
    if (!TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("Pause freezes countdown"),
             FMath::IsNearlyEqual(Presenter->GetSnapshot().Remaining, BeforePause, .001f));
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    if (!Advance(.3f))
    {
        return false;
    }
    Presenter->SetPresentationEnabled(false);
    TestFalse(TEXT("Hidden stops periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    const float HiddenRemaining = Presenter->GetSnapshot().Remaining;
    if (!Advance(.6f))
    {
        return false;
    }
    Presenter->SetPresentationEnabled(true);
    TestTrue(TEXT("Late visible snapshot catches actual GAS time"),
             Presenter->GetSnapshot().Remaining < HiddenRemaining);
    const FTimerHandle PreviousTimer = Presenter->PeriodicRefreshTimer;
    const uint64 PreviousSerial = Presenter->ObservationSerial;
    Presenter->StartObserving(Subsystem.Get());
    TestFalse(TEXT("Restart retires previous timer"),
              World->GetTimerManager().TimerExists(PreviousTimer));
    const FTimerHandle NewTimer = Presenter->PeriodicRefreshTimer;
    Presenter->HandleDeferredRefresh(PreviousSerial);
    Presenter->HandleTagChanged(ETGameplayTags::Cooldown_BreakPulse, 0,
                                Body->GetAbilitySystemComponent(), PreviousSerial);
    TestTrue(TEXT("Old callbacks do not replace new timer"),
             Presenter->PeriodicRefreshTimer == NewTimer);
    UAbilitySystemComponent* OldASC = Body->GetAbilitySystemComponent();
    PC->UnPossess();
    TestTrue(TEXT("Unpossess publishes NoPawn"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::NoPawn);
    TestFalse(TEXT("NoPawn stops timer"), Presenter->PeriodicRefreshTimer.IsValid());
    AEmberTrialCharacter* NewBody = World->SpawnActorDeferred<AEmberTrialCharacter>(
        AEmberTrialCharacter::StaticClass(), FTransform(FVector(500, 0, 200)), nullptr, nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!TestNotNull(TEXT("New body"), NewBody))
    {
        return false;
    }
    Configure(NewBody);
    NewBody->FinishSpawning(FTransform(FVector(500, 0, 200)));
    PC->Possess(NewBody);
    if (!Advance(1.0f))
    {
        return false;
    }
    TestTrue(TEXT("New body ready"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Ready);
    TestFalse(TEXT("New body does not inherit old cooldown"),
              Presenter->GetSnapshot().bCoolingDown);
    OldASC->AddLooseGameplayTag(ETGameplayTags::State_CastingBreakPulse);
    if (!Advance(.02f))
    {
        return false;
    }
    TestTrue(TEXT("Old body event stays isolated"),
             Presenter->GetSnapshot().PawnName == NewBody->GetName() &&
                 !Presenter->GetSnapshot().bCasting);
    OldASC->RemoveLooseGameplayTag(ETGameplayTags::State_CastingBreakPulse);
    if (!TestTrue(TEXT("New body's F accepted"), NewBody->RequestBreakPulse()) || !Advance(.02f))
    {
        return false;
    }
    NewBody->GetAbilitySystemComponent()->SetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    if (!Advance(.02f))
    {
        return false;
    }
    TestTrue(TEXT("Death displayed distinctly"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Dead);
    TestFalse(TEXT("Death stops periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    PC->Possess(Body);
    if (!Advance(.02f))
    {
        return false;
    }
    TestTrue(TEXT("Same old body retains its cooldown"), Presenter->GetSnapshot().bCoolingDown);
    if (!Advance(7.0f))
    {
        return false;
    }
    TestFalse(TEXT("Natural expiry observed"), Presenter->GetSnapshot().bCoolingDown);
    TestFalse(TEXT("Expiry stops periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    if (!TestTrue(TEXT("Final F for EndPlay"), Body->RequestBreakPulse()) || !Advance(.02f))
    {
        return false;
    }
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay clears periodic refresh"), Presenter->PeriodicRefreshTimer.IsValid());
    TestFalse(TEXT("EndPlay clears deferred refresh"), Presenter->DeferredRefreshTimer.IsValid());
    Presenter->Shutdown();
    TestTrue(TEXT("Shutdown snapshot unavailable"),
             Presenter->GetSnapshot().State == EETBreakPulseViewState::Unavailable);
    Subsystem->PlayerControllerChanged(nullptr);
    PC->Player = nullptr;
    LocalPlayer->PlayerController = nullptr;
    return !HasAnyErrors();
}
#endif
```

## 7. 正常Editor / 索引 / Game Build

10份Source保存，UE仍关闭。在PowerShell逐项执行，前项失败就停止：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

CLion Reload CMake Project；预期**144 Source / 78cpp，Core55 / UI20 / Editor3**。新Presenter / Widget / 测试都在UI，不能误记为Core新增3cpp；总自动化19项（原18保留）。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标成功后再打开工程做第8–9节。反射类 / 四控件还未建WBP时，不提前Play把Missing Class误当编译失败；无需清缓存。首条Error发文字 / 文件行号 / 前后上下文，不只发退出码6。本指南仅做文本 / 本机接口静态核对，助手未实际Build或执行新专项。

## 8. 新建WBP_ETBreakPulseHUD（详细Designer步骤）

### 8.1 创建、父类与空图

1. 打开Content Browser目录`/Game/EmberTrial/ui/HUD`（沿用小写ui）。空白处右键 → User Interface / 用户界面 → Widget Blueprint。
2. 若创建窗口询问父类，展开All Classes搜索**ETBreakPulseHUDWidget**，选原生父类；命名**WBP_ETBreakPulseHUD**。若创建时只给UserWidget，先新建同名资产，打开File → Reparent Blueprint选择ETBreakPulseHUDWidget，再继续建控件。
3. Class Settings核对Parent Class=ETBreakPulseHUDWidget。此时尚无4个控件，首次Compile可能提示required binding缺失；先完成8.2–8.3，不把父类改回UserWidget或删除C++ BindWidget。
4. 不Duplicate旧Healing / Health WBP，避免混入旧名称、Binding或业务图。Graph保持空，不加Event Tick、Construct里的SetText、F输入、倒计时或Apply GE。

### 8.2 按层级建控件与命名

切Designer。从Palette拖控件到Hierarchy目标父节点，按树顺序搭建。若新WBP无根，先拖Canvas Panel成为根；若已有Canvas就复用，只保留一份根。

```text
Canvas Panel（根，可命名BreakPulseCanvas）
└─ Border：BreakPulseBorder
   └─ Vertical Box：BreakPulseBox
      ├─ Text：BreakPulseStatusText
      └─ Vertical Box：BreakPulseContent
         ├─ Text：BreakPulseValuesText
         └─ Progress Bar：BreakPulseBar
```

Border只能有一个直接子项，本树用BreakPulseBox承接；标题必须在BreakPulseContent外，状态不可用时仍能显示。每次选中Hierarchy条目，在Details顶部名称栏或F2改**控件对象名**，不是Content里的显示文字。

| 四个必需对象名（逐字） | Palette类型 | Is Variable |
| --- | --- | --- |
| BreakPulseStatusText | Text / TextBlock | 勾选 |
| BreakPulseValuesText | Text / TextBlock | 勾选 |
| BreakPulseBar | Progress Bar / ProgressBar | 勾选 |
| BreakPulseContent | Vertical Box / VerticalBox | 勾选 |

不能用Rich Text替代Text，也不能把Content建成Canvas / Horizontal Box。其他Border / Box名服务阅读，不是额外BindWidget。

### 8.3 按选中对象设置布局与预览文字

Designer预览选1920×1080。保持Render Transform默认，逐行选对应对象后设置；看不到Canvas Slot时先确认选中BreakPulseBorder，而非根Canvas / 文本。

| 选中对象 | Details配置 |
| --- | --- |
| BreakPulseBorder → Slot (Canvas Panel Slot) | Anchors Min=(0,0)、Max=(0,0)；Alignment=(0,0)；Position X=24、Y=408；Size X=360、Y=132；Auto Size=False |
| BreakPulseBorder | Padding四边12；Brush Color R/G/B=.03，A=.8（或沿用旧HUD）；不改Render Transform替代Slot位置 |
| BreakPulseBox | 默认Vertical Box，标题在上 / Content在下 |
| BreakPulseStatusText | Content → Text填“破势（F）· 冷却就绪”；Font18 / 白色；Vertical Box Slot Size=Auto、Horizontal Alignment=Fill、Padding Bottom=6 |
| BreakPulseContent | Vertical Box Slot Size=Auto、Horizontal Alignment=Fill；Designer Visibility=Visible |
| BreakPulseValuesText | Content → Text填“地面站定后按 F”；Font18 / 白色；Slot Size=Auto、Horizontal Alignment=Fill、Padding Bottom=8 |
| BreakPulseBar | Slot Size=Auto、Horizontal Alignment=Fill；Percent预览=1；Fill Color选清晰蓝色；Bar Fill Type=Left to Right |

如果Progress Bar默认高度太细，可在Style → Background / Fill Image的Image Size Y设14–18；不影响原生比例。找不到该样式项可保留默认，不为纯外观新增SizeBox或改变必需控件类型。

Y408是在现有治疗Border Y296、高100后的12px间隔；若真实旧布局另有调整，只移动**本新Border**避免遮挡，并反馈实际X/Y。字体沿用旧HUD已能显示中文的字体，不购买资源。

点击Class Defaults：**Is Focusable=False、Tick Frequency=Never、Visibility=Not Hit-Testable (Self & All Children)**，即HitTestInvisible。两个Text、Progress Bar和Visibility右侧不创建Bind函数；Designer文字只是预览，运行时C++覆盖。

### 8.4 Compile / Save检查顺序

1. Compile → Save。预期无required widget binding缺失、无Native Tick警告。
2. 有报错按Parent Class → Hierarchy对象名 → 类型 → Is Variable → Compile顺序查；肉眼文字相同不等于对象名正确。
3. Tick Never的前提是原生父类已带DisableNativeTick；不要为了消警告改回Auto Tick。截图需同时含WBP标题、Hierarchy、选中对象Details和Compiler Results。

## 9. 在唯一UIBootstrap装配与原生校验

1. 打开原**BP_ETPlayerController**，Components选中已有唯一UIBootstrap，不新增组件。
2. Details搜索`Break Pulse Widget Class`（也可搜BreakPulse），选新WBP_ETBreakPulseHUD。它是Controller组件属性，不是角色GA类字段。
3. 原Status / Health / Stamina / Healing四个Widget Class保持；FailureFlow的Pause / Failure Class、Map、Controller父类不动。EventGraph不加业务。
4. Controller Compile → Save，新WBP也Save。菜单Controller本来没有UIBootstrap，不给它添加，主菜单不显示此卡。
5. Content Browser选新WBP → 右键Asset Actions → Validate Assets；应有本项目**[M1-07C1] Valid**。通用校验器检查四个BindWidget名称 / 类型，无需新Editor规则或Probe。
6. 回正式L_ET_VerticalSlice。此时新WBP已存在但尚未验收，不提前把来源清单记为71；完成后我按实际路径 / 保存证据补记。

## 10. 自动化与四组实机短查（人工约8–12分钟）

### 10.1 三项自动化各一次

Stop PIE，Output Log逐条输入，上一项Success后再下一项：

```text
Automation RunTests EmberTrial.UI.BreakPulse.Lifecycle
Automation RunTests EmberTrial.M2.Combat.BreakPulseRange
Automation RunTests EmberTrial.M2.Combat.BreakPulseAbility
```

新专项检查数据流与有限Timer / 身份；旧Range、Ability核对只读接口接入没有破坏原范围 / 冷却。其余16项保留，不机械重跑；没有额外Controller接线 / 测试Pawn资产。新测试仍受WITH_DEV_AUTOMATION_TESTS保护，不随普通Play运行。

失败先贴首条断言 / 文件行号 / 本轮时间，停止后面勾选；闪退保留Saved/Crashes，不反复重跑同一个崩溃。新代码未由助手实际编译 / 执行，真正Success以你运行与日志为准。

### 10.2 出生、F、冷却与Pause（约2–3分钟）

1. 正式地图Play，新身体双100 / 治疗3 / Poise100；只有一张破势卡在治疗下方，中文清楚，不挡旧HUD / 鼠标。出生显示“冷却就绪”，无持续RefreshTimer Active=1日志。
2. 站定F：卡先“施放中”，显示约6.0秒；动作结束转“冷却中”，条逐渐回满。冷却中再按F拒绝，卡不重置6秒；长按不到期自动重放。
3. 冷却期间P暂停现实约2秒，屏幕数字 / 条冻结；菜单按F不补播。以一种方式继续，倒计时继续而非补扣暂停时间；到期回“冷却就绪”，一条RefreshTimer Active=0。
4. 站定成功一次后下一轮按3取消，卡仍按原冷却走；不用精确卡.4秒来重做C2规则。若在移动中看到“冷却就绪”但F拒绝正常，它只说明冷却，不承诺动作资格。

### 10.3 冷却中重建View与重启观察（约2分钟）

保持F仍在冷却，正在运行的PIE视口按~打开游戏控制台，逐条执行（不是PowerShell / 无PIE的编辑器世界）：

```text
ke ETUIBootstrapComponent RebuildBreakPulseWidget
ke ETUIBootstrapComponent RebuildBreakPulseWidget
ke ETUIBootstrapComponent RestartBreakPulseObservation
```

如果输入期间冷却已结束，下一次F后只重建一次即可，不要求卡时间。关闭控制台恢复视口。

预期只有一份新卡，立即显示真实剩余而非6秒重头；Presenter对象不因View重建更换，原GAS / 其他资源不重置。日志[M2-05D]有created / removed、状态快照及RefreshTimer启停；每次重建可出现一组停 / 开，**不是同时多份Timer**。新专项已检查旧Timer句柄失效与旧代次隔离，不搭旧Widget保留变量图。

### 10.4 补真实敌人交战样本（约3–4分钟）

1. 确认MeleeEnemy_A仍Placed in World or Spawned，实际感知 / 追击正常；本轮不临时禁用AI。训练目标旧范围证明沿用，主要补敌人画面样本。
2. 与敌人拉开约2–3米，F卡就绪后**站定F，让敌人在接近途中进入300cm范围**。不必贴脸与敌人8帧命中抢速度；正常受击可打断F，已生效Poise和冷却保留。
3. 侧移 / 闪避拉开，等卡就绪后再找一次站定F机会，观察同一敌人Poise100→40→0、旧占位破势反应 / 中断，结束后恢复追击。可用`ke BP_ETMeleeEnemy_C ET_Poise 0`只读查询，关闭控制台再操作。
4. 左键仍真实25 / 三段，2仍方向位移扣体力，受伤后Q一次40 / 减次数；P / 镜头正常。不要用开发-60替代“正式F命中”的收口样本；若多次被打断，发实际按键 / 距离 / 日志结果，我分析手感，不要求你反复撞帧或重做全套。

### 10.5 Retry / Continue、Stop与恢复（约2分钟）

1. 用原4或敌人致死到Failure，确认不叠Pause；Retry新身体破势卡冷却就绪，双100 / 治疗3；再F仍正常。
2. P返回菜单 → Continue，检查点沿既有语义，新身体冷却就绪、单卡；不重新走检查点激活矩阵。菜单本身无此HUD。
3. 冷却中Stop，再Play卡回就绪，无旧World继续更新 / Accessed None。最后E、P一种继续方式、移动 / 转向 / 跳跃短查。
4. Stop → Save All，唯一UIBootstrap / FailureFlow，5个HUD Class正确；原GA300 / 60 / 6 / 3、Stagger1.2 / 旧AM、源AM12帧通知、双100 / 3 / Poise100、slomo1 / Debug False、默认地图保持。没有临时修改就填“未改”。

本批是编辑器集成与HUD收口，不自动等同于最新包 / 性能达标；新包和真实Insights在后续阶段按负载安排，不混用Cleanup/Attempt01的历史结果。

## 11. 完成反馈与报错方式

```text
M2-05D 完成
五新 / 五旧Source；关闭UE后Editor Build / Core55-UI20-Editor3索引 / Game Build：
新WBP父类 / 四BindWidget / 无Binding / Never / Compile-Save-本项目Valid：
原Controller唯一UIBootstrap / Break Pulse Widget Class、旧四Class与Flow保留：
UI.BreakPulse.Lifecycle / BreakPulseRange / BreakPulseAbility各一次：
出生单卡 / F施放-冷却-就绪 / 拒绝不重置 / Pause冻结与不补播：
冷却中View重建 / 同Presenter重启 / 真实剩余与无重复Timer：
真实敌人正式F结果（请写100→40→0、反应后追击，或实际失败情况）：
左键-2-Q-E-P短基线 / 死亡Retry-菜单Continue新就绪 / 冷却中Stop重开：
Source资产Save All；原数值 / AI / slomo1 / Debug False恢复或未改：
新卡实际坐标（沿用24/408可填沿用）/ 最明显布局或失败问题：
完成时分 / 手动代码与配置 / 检查 / 构建等待：
```

编译 / 自动化发首条Error或断言文字、路径行号及前后约20行；控件 / 字段问题截图含当前资产名、Hierarchy、Details搜索与Compiler Results；交战时序给5–10秒视频与按键顺序。给完成时分即可，我读日志，不要求手抄成功记录。普通开发你手动实现，需直接接手时明确说明。

没有待补项后保存D / M2-05总验收，更新真实工时 / 来源，再按M2路线进入后续锁定等开发。UI状态 / 布局未验收前不将四能力最终体验写成已完成。

## 12. 本批 Commit / Push（固定放在最后）

**实施且验收通过后**Stage五新Source（前缀EmberTrial/Source/）：

```text
EmberTrialUI/Public/Presenters/ETBreakPulsePresenter.h
EmberTrialUI/Private/Presenters/ETBreakPulsePresenter.cpp
EmberTrialUI/Public/Widgets/ETBreakPulseHUDWidget.h
EmberTrialUI/Private/Widgets/ETBreakPulseHUDWidget.cpp
EmberTrialUI/Private/Tests/ETBreakPulseHUDTests.cpp
```

**五旧Source**（同前缀）：

```text
EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h
EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp
EmberTrialUI/EmberTrialUI.Build.cs
```

**一新 / 一旧UE资产**：

```text
EmberTrial/Content/EmberTrial/ui/HUD/WBP_ETBreakPulseHUD.uasset
EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset
```

上表Controller路径已按本机既有资产核对，不移动资源。原四HUD、输入 / GA / 动画 / 地图 / Config无计划差异，只有实际改动且原因明确才提交。

**文档**：本指南、完成后生成的`Docs/Reports/M2_05D_Verification.md`与M2-05总验收、来源清单新增实际WBP后71项、Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / 两份M2路线 / README / Introduction最新收尾及下一批指南；尚未提交的C2-02报告也一起包含。现在不存在的新Source / WBP / 报告不要先建空文件。

建议Commit：

```text
feat: show break pulse cooldown through lifecycle-aware HUD
```

CLion刷新Changes → Stage最新完整内容（已Stage后又修改必须再Stage）→ 检查Staged新文件含完整代码、Controller唯一组件 / 正确Class、WBP控件和LFS → Commit → Push。Raw / 日志 / 崩溃转储 / Saved / Binaries / Intermediate / CodeModel / IDE生成物 / 包体留本地，不Force Add。用户操作Git，助手未改索引、Commit或联网核验远端。

本次移动前，错误Core路径已有暂存新增记录；提交前取消该旧路径的暂存新增，再Stage正确UI路径，确认最终Staged只有UI目录的一份完整测试。移动与文档修改已由助手执行，暂存区仍由用户更新。
