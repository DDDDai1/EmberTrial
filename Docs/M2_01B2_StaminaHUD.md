# M2-01B2：事件驱动的体力面板

生成2026-10-15，固定 **UE 5.5.4**。前置：[B1已验收](Reports/M2_01B1_Verification.md)。**以下是待你手动实施的指南；未写入Source、未创建Widget、未Build或实测。**当前实际83Source / 42cpp / 清单48项。

本批预计手动 **60–100分钟**：代码30–50、资产15–25、相关检查5–15、保存记录约5分钟；构建 / 索引等待另记，不计为实耗。继续方案B，没有新临时Controller函数或按键；复用B1的ke命令和现有死亡 / Retry / 返回菜单。

## 1. 结果、职责与覆盖范围

左上方生命面板下新增“体力值 / 100 / 100 / 绿色进度条”。开发-25后显示75并随B1真实GE恢复，生命仍100；满值无UI轮询。按2仍是原型，不消费体力；正式成本、闪避免伤 / Dodging留M2-02。

数据单向流动：`当前LocalPlayer的Pawn → Character ASC事件 → StaminaPresenter快照 → StaminaHUD`。Bootstrap持有一个新Presenter与当前体力Widget，两个都是每个游戏Controller的对象；保持既有生命Presenter / Widget独立，不在Core反向引用UI。恢复组件不接触UMG，View不写属性、不自行计时补体力。

| 情况 | 显示 / 生命周期 |
| --- | --- |
| Ready | 显示真实Stamina / MaxStamina、比例夹到0–1；0体力仍是有效Ready |
| NoPawn / UnsupportedPawn / Waiting / Unavailable | 保留原因标题，折叠数值 / 条，不显示假的100 |
| 暂停 | 世界属性冻结，面板保留快照；Widget不取得输入焦点 |
| 重新订阅 / 换Pawn | 先解绑旧属性 / Ready，再订阅并查询当前快照；旧ASC和旧观察序号不能污染新数据 |
| Widget晚加入 / 重建 | 订阅后立即GetSnapshot；只替换View，不重建Presenter或初始化资源 |
| 死亡 / Retry / 菜单 | 死亡体力保留B1停止后的真实值；新世界创建新Presenter / Widget，结束先停View再Shutdown Presenter |

先保留两份明确的小Presenter，避免在本次引入反射属性适配框架；公共Pawn观察仍复用既有Subsystem。未来资源增多并确认共性后再提取，不借增加一条体力条重写已验收生命链路。

## 2. 文件与准备

Stop PIE → Save All → 关闭UE，CLion保存。**四新Source / 两旧Source、一个新Widget资产**；不修改Core、B1恢复、UI Build.cs或Editor校验器。完成后预计87Source / 44cpp，当前尚未达到。

路径均在 `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI`：

| 操作 | 相对路径 |
| --- | --- |
| 新建 | `Public/Presenters/ETStaminaPresenter.h` |
| 新建 | `Private/Presenters/ETStaminaPresenter.cpp` |
| 新建 | `Public/Widgets/ETStaminaHUDWidget.h` |
| 新建 | `Private/Widgets/ETStaminaHUDWidget.cpp` |
| 修改 | `Public/Bootstrap/ETUIBootstrapComponent.h` |
| 修改 | `Private/Bootstrap/ETUIBootstrapComponent.cpp` |

复制下面全文，不覆盖原Health四文件。已有UMG / Core / GAS依赖足够；generated.h必须最后include。新枚举 / 结构 / 类名均为Stamina，不能与Health重复。

## 3. 新Presenter两文件：完整代码

### 3.1 ETStaminaPresenter.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ETStaminaPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETUILocalPlayerSubsystem;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EETStaminaViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Ready
};

USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETStaminaViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    EETStaminaViewState State = EETStaminaViewState::Unavailable;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    FString PawnName;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Stamina = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float MaxStamina = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float StaminaFraction = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FETStaminaViewChanged, const FETStaminaViewData&, Data);

UCLASS(BlueprintType)
class EMBERTRIALUI_API UETStaminaPresenter : public UObject
{
    GENERATED_BODY()

public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETStaminaViewData GetSnapshot() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETStaminaViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void ClearAttributeBindings();
    void RebindObservedPawn(const TCHAR* Reason);
    void RefreshReadyBinding(const TCHAR* Reason);
    FETStaminaViewData BuildSnapshot() const;
    void PublishSnapshot(const TCHAR* Reason, bool bForce = false);

    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
        TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial);

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle StaminaHandle;
    FDelegateHandle MaxStaminaHandle;
    FETStaminaViewData Snapshot;
    uint64 ObservationSerial = 0;
    bool bObserving = false;
};
```

### 3.2 ETStaminaPresenter.cpp

```cpp
#include "Presenters/ETStaminaPresenter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameFramework/Pawn.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStaminaPresenter, Log, All);

void UETStaminaPresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
{
    // 保留View订阅；只清理本Presenter对数据源的订阅。
    StopObservingInternal(false);
    if (!IsValid(Subsystem))
    {
        PublishSnapshot(TEXT("MissingSubsystem"), true);
        return;
    }

    BoundSubsystem = Subsystem;
    bObserving = true;
    Subsystem->OnObservedPawnChanged.AddUniqueDynamic(
        this, &UETStaminaPresenter::HandleObservedPawnChanged);
    RebindObservedPawn(TEXT("StartSnapshot"));
}

void UETStaminaPresenter::Shutdown()
{
    StopObservingInternal(true);
}

void UETStaminaPresenter::BeginDestroy()
{
    // GC兜底不广播给View，正常清理由Bootstrap的EndPlay负责。
    StopObservingInternal(false);
    Super::BeginDestroy();
}

FETStaminaViewData UETStaminaPresenter::GetSnapshot() const
{
    return Snapshot;
}

void UETStaminaPresenter::StopObservingInternal(bool bNotify)
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(
            this, &UETStaminaPresenter::HandleObservedPawnChanged);
    }
    ClearPawnBindings();
    BoundSubsystem.Reset();

    if (bWasObserving)
    {
        UE_LOG(LogETStaminaPresenter, Display,
            TEXT("[M2-01B2] Stopped Presenter=%s"), *GetNameSafe(this));
    }
    if (bNotify)
    {
        PublishSnapshot(TEXT("Shutdown"));
    }
    else
    {
        Snapshot = FETStaminaViewData{};
    }
}

void UETStaminaPresenter::ClearAttributeBindings()
{
    ++ObservationSerial;
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(
            UETStaminaAttributeSet::GetStaminaAttribute()).Remove(StaminaHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(
            UETStaminaAttributeSet::GetMaxStaminaAttribute()).Remove(MaxStaminaHandle);
    }
    StaminaHandle.Reset();
    MaxStaminaHandle.Reset();
    BoundASC.Reset();
}

void UETStaminaPresenter::ClearPawnBindings()
{
    ClearAttributeBindings();
    if (AEmberTrialCharacter* Character = BoundCharacter.Get())
    {
        Character->OnGameplayReadyChanged.RemoveDynamic(
            this, &UETStaminaPresenter::HandleGameplayReadyChanged);
    }
    BoundCharacter.Reset();
}

void UETStaminaPresenter::RebindObservedPawn(const TCHAR* Reason)
{
    ClearPawnBindings();
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    AEmberTrialCharacter* Character = Subsystem
        ? Cast<AEmberTrialCharacter>(Subsystem->GetObservedPawn()) : nullptr;
    if (IsValid(Character))
    {
        BoundCharacter = Character;
        // 先订阅，再查询，兼容就绪通知早于Presenter创建。
        Character->OnGameplayReadyChanged.AddUniqueDynamic(
            this, &UETStaminaPresenter::HandleGameplayReadyChanged);
    }
    RefreshReadyBinding(Reason);
}

void UETStaminaPresenter::RefreshReadyBinding(const TCHAR* Reason)
{
    AEmberTrialCharacter* Character = BoundCharacter.Get();
    UAbilitySystemComponent* ASC = Character && Character->IsGameplayReady()
        ? Character->GetAbilitySystemComponent() : nullptr;

    if (!IsValid(ASC))
    {
        ClearAttributeBindings();
    }
    else if (BoundASC.Get() != ASC || !StaminaHandle.IsValid() || !MaxStaminaHandle.IsValid())
    {
        ClearAttributeBindings();
        BoundASC = ASC;
        // Payload也是弱引用，用于拒绝旧ASC回调。
        StaminaHandle = ASC->GetGameplayAttributeValueChangeDelegate(
            UETStaminaAttributeSet::GetStaminaAttribute()).AddUObject(
                this, &UETStaminaPresenter::HandleAttributeChanged, BoundASC, ObservationSerial);
        MaxStaminaHandle = ASC->GetGameplayAttributeValueChangeDelegate(
            UETStaminaAttributeSet::GetMaxStaminaAttribute()).AddUObject(
                this, &UETStaminaPresenter::HandleAttributeChanged, BoundASC, ObservationSerial);
    }
    PublishSnapshot(Reason);
}

void UETStaminaPresenter::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (bObserving)
    {
        RebindObservedPawn(TEXT("PawnChanged"));
    }
}

void UETStaminaPresenter::HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady)
{
    if (!bObserving || Character != BoundCharacter.Get())
    {
        return;
    }
    if (!bReady)
    {
        ClearAttributeBindings();
        PublishSnapshot(TEXT("ReadyFalse"));
        return;
    }
    RefreshReadyBinding(TEXT("ReadyTrue"));
}

void UETStaminaPresenter::HandleAttributeChanged(const FOnAttributeChangeData& Change,
    TWeakObjectPtr<UAbilitySystemComponent> SourceASC, uint64 Serial)
{
    if (Serial != ObservationSerial || !bObserving || !SourceASC.IsValid() || SourceASC.Get() != BoundASC.Get())
    {
        return;
    }
    PublishSnapshot(Change.Attribute == UETStaminaAttributeSet::GetStaminaAttribute()
        ? TEXT("StaminaChanged") : TEXT("MaxStaminaChanged"));
}

FETStaminaViewData UETStaminaPresenter::BuildSnapshot() const
{
    FETStaminaViewData Data;
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    if (!bObserving || !IsValid(Subsystem))
    {
        return Data;
    }
    APawn* Pawn = Subsystem->GetObservedPawn();
    if (!IsValid(Pawn))
    {
        Data.State = EETStaminaViewState::NoPawn;
        return Data;
    }
    Data.PawnName = Pawn->GetName();
    AEmberTrialCharacter* Character = Cast<AEmberTrialCharacter>(Pawn);
    if (!IsValid(Character))
    {
        Data.State = EETStaminaViewState::UnsupportedPawn;
        return Data;
    }
    if (!Character->IsGameplayReady())
    {
        Data.State = EETStaminaViewState::WaitingForGameplayReady;
        return Data;
    }

    float Stamina = 0.0f;
    float MaxStamina = 0.0f;
    if (Character != BoundCharacter.Get() || !BoundASC.IsValid() ||
        Character->GetAbilitySystemComponent() != BoundASC.Get() ||
        !Character->GetGameplayStaminaSnapshot(Stamina, MaxStamina) ||
        !FMath::IsFinite(Stamina) || !FMath::IsFinite(MaxStamina) ||
        MaxStamina <= 0.0f || Stamina < 0.0f || Stamina > MaxStamina)
    {
        return Data;
    }
    Data.State = EETStaminaViewState::Ready;
    Data.Stamina = Stamina;
    Data.MaxStamina = MaxStamina;
    Data.StaminaFraction = FMath::Clamp(Stamina / MaxStamina, 0.0f, 1.0f);
    return Data;
}

void UETStaminaPresenter::PublishSnapshot(const TCHAR* Reason, bool bForce)
{
    const FETStaminaViewData NewData = BuildSnapshot();
    const bool bSame = Snapshot.State == NewData.State && Snapshot.PawnName == NewData.PawnName &&
        Snapshot.Stamina == NewData.Stamina && Snapshot.MaxStamina == NewData.MaxStamina &&
        Snapshot.StaminaFraction == NewData.StaminaFraction;
    if (bSame && !bForce)
    {
        return;
    }
    Snapshot = NewData;
    UE_LOG(LogETStaminaPresenter, Verbose,
        TEXT("[M2-01B2] Snapshot Presenter=%s State=%s Pawn=%s Stamina=%.1f MaxStamina=%.1f Fraction=%.3f Reason=%s"),
        *GetNameSafe(this), *StaticEnum<EETStaminaViewState>()->GetNameStringByValue(
            static_cast<int64>(Snapshot.State)), *Snapshot.PawnName,
        Snapshot.Stamina, Snapshot.MaxStamina, Snapshot.StaminaFraction, Reason);
    const FETStaminaViewData Published = Snapshot;
    OnViewDataChanged.Broadcast(Published);
}
```

观察序号在清除属性订阅时递增，避免同一ASC重新订阅后接受旧一轮回调。快照按状态、Pawn身份和精确数值去重，不用显示整数反推实际资源；恢复可能每0.10秒更新，Presenter没有Tick / Timer。广播局部快照副本，避免订阅者同步回调修改成员快照时改变正在广播的数据。

## 4. 新Widget两文件：完整代码

### 4.1 ETStaminaHUDWidget.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Presenters/ETStaminaPresenter.h"
#include "ETStaminaHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UVerticalBox;

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETStaminaHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void StartObserving(UETStaminaPresenter* Presenter);
    void StopObserving();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyViewData(const FETStaminaViewData& Data, const TCHAR* Reason);

    UFUNCTION()
    void HandleViewDataChanged(const FETStaminaViewData& Data);

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StaminaStatusText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StaminaValuesText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> StaminaBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> StaminaContent;

    TWeakObjectPtr<UETStaminaPresenter> BoundPresenter;
    bool bObserving = false;
};
```

### 4.2 ETStaminaHUDWidget.cpp

```cpp
#include "Widgets/ETStaminaHUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Internationalization/Text.h"
#include "Logging/LogMacros.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStaminaHUD, Log, All);
#define LOCTEXT_NAMESPACE "ETStaminaHUD"

void UETStaminaHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    if (IsDesignTime())
    {
        return;
    }

    // Construct may run more than once; subscription belongs to StartObserving.
    const UETStaminaPresenter* Presenter = BoundPresenter.Get();
    ApplyViewData(Presenter ? Presenter->GetSnapshot() : FETStaminaViewData{},
        TEXT("ConstructSnapshot"));
}

void UETStaminaHUDWidget::NativeDestruct()
{
    StopObserving();
    Super::NativeDestruct();
}

void UETStaminaHUDWidget::StartObserving(UETStaminaPresenter* Presenter)
{
    StopObserving();
    if (IsDesignTime())
    {
        return;
    }

    if (!IsValid(Presenter))
    {
        ApplyViewData(FETStaminaViewData{}, TEXT("MissingPresenter"));
        return;
    }

    BoundPresenter = Presenter;
    bObserving = true;
    Presenter->OnViewDataChanged.AddUniqueDynamic(
        this, &UETStaminaHUDWidget::HandleViewDataChanged);

    UE_LOG(LogETStaminaHUD, Display,
        TEXT("[M2-01B2] View subscribed Widget=%s Presenter=%s"),
        *GetNameSafe(this), *GetNameSafe(Presenter));

    // Bind first, then get current data: a late-joining View needs no new effect.
    ApplyViewData(Presenter->GetSnapshot(), TEXT("SubscribeSnapshot"));
}

void UETStaminaHUDWidget::StopObserving()
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    UETStaminaPresenter* Presenter = BoundPresenter.Get();
    if (Presenter)
    {
        Presenter->OnViewDataChanged.RemoveDynamic(
            this, &UETStaminaHUDWidget::HandleViewDataChanged);
    }
    BoundPresenter.Reset();

    if (bWasObserving)
    {
        UE_LOG(LogETStaminaHUD, Display,
            TEXT("[M2-01B2] View unsubscribed Widget=%s"), *GetNameSafe(this));
    }
}

void UETStaminaHUDWidget::HandleViewDataChanged(const FETStaminaViewData& Data)
{
    if (!bObserving || !BoundPresenter.IsValid())
    {
        return;
    }
    ApplyViewData(Data, TEXT("ViewDataChanged"));
}

void UETStaminaHUDWidget::ApplyViewData(
    const FETStaminaViewData& Data, const TCHAR* Reason)
{
    if (!StaminaStatusText || !StaminaValuesText || !StaminaBar || !StaminaContent)
    {
        UE_LOG(LogETStaminaHUD, Warning,
            TEXT("[M2-01B2] Missing required BindWidget Widget=%s"),
            *GetNameSafe(this));
        return;
    }

    if (Data.State == EETStaminaViewState::Ready)
    {
        StaminaStatusText->SetText(LOCTEXT("StaminaTitle", "体力值"));
        StaminaContent->SetVisibility(ESlateVisibility::HitTestInvisible);
        FNumberFormattingOptions NumberOptions;
        NumberOptions.SetMinimumFractionalDigits(0);
        NumberOptions.SetMaximumFractionalDigits(0);
        StaminaValuesText->SetText(FText::Format(
            LOCTEXT("StaminaValues", "{0} / {1}"),
            FText::AsNumber(Data.Stamina, &NumberOptions),
            FText::AsNumber(Data.MaxStamina, &NumberOptions)));
        StaminaBar->SetPercent(Data.StaminaFraction);
    }
    else
    {
        StaminaContent->SetVisibility(ESlateVisibility::Collapsed);
        StaminaValuesText->SetText(FText::GetEmpty());
        StaminaBar->SetPercent(0.0f);
        switch (Data.State)
        {
        case EETStaminaViewState::NoPawn:
            StaminaStatusText->SetText(LOCTEXT("NoPawn", "暂无受控角色"));
            break;
        case EETStaminaViewState::UnsupportedPawn:
            StaminaStatusText->SetText(LOCTEXT("UnsupportedPawn", "当前角色不提供体力信息"));
            break;
        case EETStaminaViewState::WaitingForGameplayReady:
            StaminaStatusText->SetText(LOCTEXT("Waiting", "角色准备中"));
            break;
        default:
            StaminaStatusText->SetText(LOCTEXT("Unavailable", "体力信息暂不可用"));
            break;
        }
    }

    const FString StateName = StaticEnum<EETStaminaViewState>()->GetNameStringByValue(
        static_cast<int64>(Data.State));
    UE_LOG(LogETStaminaHUD, Verbose,
        TEXT("[M2-01B2] View Widget=%s State=%s Stamina=%.1f MaxStamina=%.1f Fraction=%.3f Reason=%s"),
        *GetNameSafe(this), *StateName, Data.Stamina, Data.MaxStamina,
        Data.StaminaFraction, Reason);
}

#undef LOCTEXT_NAMESPACE
```

DisableNativeTick与蓝图Never搭配，沿用生命HUD的已验证设置。View只渲染快照；显示整数只是格式化，进度条仍用真实浮点比例。日常订阅 / 清理日志为Display；连续数值Snapshot / View日志为Verbose，避免恢复期间反复刷Display。常规检查使用可见面板和B1开发读数，不要求收集几十行恢复日志。

## 5. Bootstrap准确接入（保留原文件）

### 5.1 .h：四个位置

1. 前置声明区，在已有 `class UETHealthHUDWidget;` 后加：

```cpp
class UETStaminaPresenter;
class UETStaminaHUDWidget;
```

2. public区，在 `UETHealthHUDWidget* GetHealthWidget() const;` 后、protected前加：

```cpp
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETStaminaPresenter* GetStaminaPresenter() const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartStaminaObservation();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RebuildStaminaWidget();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETStaminaHUDWidget* GetStaminaWidget() const;
```

3. private函数区，在 `void RemoveHealthWidget();` 后加：

```cpp
    void RemoveStaminaWidget();
```

4. private字段区，在 `TObjectPtr<UETHealthHUDWidget> HealthWidget;` 后、`bool bUIShuttingDown` 前加：

```cpp
    UPROPERTY(Transient)
    TObjectPtr<UETStaminaPresenter> StaminaPresenter;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|UI")
    TSubclassOf<UETStaminaHUDWidget> StaminaWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UETStaminaHUDWidget> StaminaWidget;
```

原Status / Health字段、函数及bUIShuttingDown保留，不添加第二个UIBootstrap。

### 5.2 .cpp：include与两个完整生命周期函数

include区末尾追加：

```cpp
#include "Presenters/ETStaminaPresenter.h"
#include "Widgets/ETStaminaHUDWidget.h"
```

只用以下两个函数替换原BeginPlay / EndPlay，其他原函数不覆盖：

```cpp
void UETUIBootstrapComponent::BeginPlay()
{
    bUIShuttingDown = false;
    Super::BeginPlay();
    RestartHealthObservation();
    RestartStaminaObservation();
    RebuildStatusWidget();
    RebuildHealthWidget();
    RebuildStaminaWidget();
}

void UETUIBootstrapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bUIShuttingDown = true;
    RemoveStaminaWidget();
    RemoveHealthWidget();
    RemoveStatusWidget();
    if (StaminaPresenter)
    {
        StaminaPresenter->Shutdown();
        StaminaPresenter = nullptr;
    }
    if (HealthPresenter)
    {
        HealthPresenter->Shutdown();
        HealthPresenter = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
```

### 5.3 .cpp：文件末尾追加五个新函数

确认在所有原函数的结束大括号之后，添加以下全文。层级3低于原生命4 / 状态5，体力Widget本身不接收鼠标。

```cpp
UETStaminaPresenter* UETUIBootstrapComponent::GetStaminaPresenter() const
{
    return StaminaPresenter.Get();
}

void UETUIBootstrapComponent::RestartStaminaObservation()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() || !Controller->GetLocalPlayer())
    {
        if (StaminaPresenter)
        {
            StaminaPresenter->Shutdown();
        }
        return;
    }
    if (!StaminaPresenter)
    {
        StaminaPresenter = NewObject<UETStaminaPresenter>(this);
    }
    StaminaPresenter->StartObserving(
        Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>());
}

UETStaminaHUDWidget* UETUIBootstrapComponent::GetStaminaWidget() const
{
    return StaminaWidget.Get();
}

void UETUIBootstrapComponent::RebuildStaminaWidget()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    RemoveStaminaWidget();
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController()
        || !Controller->GetLocalPlayer())
    {
        return;
    }
    if (!StaminaWidgetClass || !IsValid(StaminaPresenter.Get()))
    {
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M2-01B2] Missing StaminaWidgetClass or Presenter Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }
    StaminaWidget = CreateWidget<UETStaminaHUDWidget>(Controller, StaminaWidgetClass);
    if (!StaminaWidget)
    {
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M2-01B2] Stamina widget creation failed Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }
    if (!StaminaWidget->AddToPlayerScreen(3))
    {
        UE_LOG(LogETUIBootstrap, Warning,
            TEXT("[M2-01B2] Stamina AddToPlayerScreen failed Owner=%s"),
            *GetNameSafe(Controller));
        RemoveStaminaWidget();
        return;
    }
    StaminaWidget->StartObserving(StaminaPresenter.Get());
    UE_LOG(LogETUIBootstrap, Display,
        TEXT("[M2-01B2] Stamina widget created Owner=%s Widget=%s"),
        *GetNameSafe(Controller), *GetNameSafe(StaminaWidget.Get()));
}

void UETUIBootstrapComponent::RemoveStaminaWidget()
{
    if (!StaminaWidget)
    {
        return;
    }
    StaminaWidget->StopObserving();
    UE_LOG(LogETUIBootstrap, Display,
        TEXT("[M2-01B2] Stamina widget removed Owner=%s Widget=%s"),
        *GetNameSafe(GetOwner()), *GetNameSafe(StaminaWidget.Get()));
    StaminaWidget->RemoveFromParent();
    StaminaWidget = nullptr;
}
```

已有 `RebuildHealthWidget / RestartHealthObservation` 只处理生命，新函数只处理体力；任何重建入口都不能调用EnsureGameplayInitialized或写资源。

## 6. 正常Build与索引

保存全部Source，UE关闭，PowerShell先运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor / 索引成功后，再执行Game：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

前一步失败先处理，不继续记后一步成功。CLion Reload CMake Project，新Presenter / Widget两cpp属于EmberTrialUICodeModel；原全项目42cpp增加到44cpp。正常构建不用CLion的CMake CodeModel目标，不删除缓存。不要在此时Play；先创建Widget并配置Class，避免缺少配置Warning。

## 7. Widget资产：按顺序逐项设置

### 7.1 复制一份现有面板并更换父类

1. 打开Content Browser，定位已有 `/Game/EmberTrial/ui/HUD/WBP_ETHealthHUD`。先确认原资产能Compile / Save。
2. 原缩略图右键 → Duplicate（或Ctrl+W），复制名设为 **WBP_ETStaminaHUD**，同目录保存。后续全部操作新副本，原生命HUD不改。
3. 打开新副本，Widget编辑器顶栏File → Reparent Blueprint，搜索 **ETStaminaHUDWidget** 并选择。Class Settings的Parent Class应为ETStaminaHUDWidget。
4. 新父类要求Stamina名称；此时出现四项required widget binding缺失是尚未完成下一节的中间状态。先改完四个必需控件再Compile；不要为消除提示把BindWidget改Optional、删除原生字段或另建Text Binding。

### 7.2 控件改名：Hierarchy名称，不是Text内容

在左侧 **Hierarchy** 点击一个控件，按F2（或Details最上方Name）改控件名称。控件类型保持。四个必需控件逐个核对Is Variable=True。

| 原副本中的名称 | 改成 | 类型 / 要求 |
| --- | --- | --- |
| HUDCanvas | StaminaHUDCanvas | Canvas Panel，根 |
| HealthBorder | StaminaBorder | Border |
| HealthBox | StaminaBox | Vertical Box |
| HealthStatusText | **StaminaStatusText** | Text Block，Is Variable=True |
| HealthContent | **StaminaContent** | Vertical Box，Is Variable=True |
| HealthValuesText | **StaminaValuesText** | Text Block，Is Variable=True |
| BarSize | StaminaBarSize | Size Box |
| HealthBar | **StaminaBar** | Progress Bar，Is Variable=True |

最终Hierarchy：

```text
StaminaHUDCanvas
└─ StaminaBorder
   └─ StaminaBox
      ├─ StaminaStatusText
      └─ StaminaContent
         ├─ StaminaValuesText
         └─ StaminaBarSize
            └─ StaminaBar
```

如果你原生命HUD的非必需容器名略有不同，按类型与位置找到对应控件；四个粗体名称和类型必须完全一致。标题必须在StaminaContent外，否则Waiting时标题也会被隐藏。

### 7.3 Designer属性：逐项核对

Designer在Widget编辑器右上角，选中Hierarchy控件后在右侧Details编辑。下表的Text指 **Details → Content → Text**，不是名叫Designer Text的独立面板。

| 对象 | 属性 |
| --- | --- |
| StaminaBorder → Slot(Canvas Panel Slot) | Anchors Min=(0,0),Max=(0,0)；Alignment=(0,0)；Position X=24,Y=160；Size X=360,Y=124；Auto Size=False |
| StaminaBorder | Padding四边12；Brush Color=(0.03,0.03,0.04,0.80)；Horizontal / Vertical Alignment=Fill |
| StaminaStatusText | Content → Text填“体力值”；Font Size18，白色；Slot Size=Auto、Horizontal=Fill、Padding Bottom6 |
| StaminaContent | Slot Size=Auto、Horizontal=Fill；默认Visibility=Visible |
| StaminaValuesText | Content → Text预览填“100 / 100”；Font Size24，白色；Slot Size=Auto、Horizontal=Fill |
| StaminaBarSize | Height Override勾选20；Slot Size=Auto、Horizontal=Fill、Padding Top6 |
| StaminaBar | Percent=1；Bar Fill Type=Left to Right；Bar Fill Style=Scale；Is Marquee=False；Fill Color=(0.18,0.72,0.32,1) |
| Widget Class Defaults | Is Focusable=False；Tick Frequency=Never；Visibility=Not Hit-Testable(Self & All Children) |

位置Y160位于旧生命面板Y24、高124的下方，间隔12。字体沿用旧面板已能显示中文的字体，所有Render Transform保持原值。标题 / 数值Text和Percent右侧 **没有Bind / 绑定函数**；Event Graph不新增Construct、Tick或任何Set Text / Set Percent / 输入业务。

Compile → 确认没有required widget binding或Native Tick警告 → Save。若缺Binding，按第7.2节检查Hierarchy名称、类型、Is Variable和新父类；只改显示文字不能修复控件绑定。不要先反复重编C++。

### 7.4 Controller装配与静态检查

1. 打开 `/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`，Components选中已有唯一 **UIBootstrap**。
2. Details搜索 `Stamina Widget Class`，选择 **WBP_ETStaminaHUD**。清空搜索，原Status Widget Class / Health Widget Class保持原资源。FailureFlow与Pause / Failure Class不改。
3. Controller Compile → Save。不在BeginPlay另写Create Widget，不新增组件，不把新Widget挂到Character。
4. Content Browser选中新WBP → 右键Asset Actions → Validate Assets。预期项目 `[M1-07C1] Valid Asset=...WBP_ETStaminaHUD`。原生通用BindWidget校验自动遍历新父类，不需要修改Editor源码或制作负例Probe。

新Widget为自主布局 / 代码，无外部下载；来源清单在本批实物验收时追加，当前不提前宣称49项。

## 8. 分层检查：总计5–15分钟

### 8.1 出生 / 真实资源与生命隔离（约2–4分钟）

1. 打开正式纵向地图，在Entry附近正常Play，暂不靠近敌人。左上生命100/100，下方单份绿色体力100/100，右上状态面板保留；鼠标仍控制视角，没有新增焦点。
2. Output Log命令框输入已有命令：

   ```text
   ke BP_ETPlayerController_C ET_Stamina -25
   ```

3. 观察体力立即下降后逐步回满，生命仍100。命令日志应Before100 / After75 / Max100；显示可因切窗口耗时已开始恢复，不要求你截到恰好75。约0.75世界秒延迟及速率不重做掐秒测试，以B1已有时序证据为准。
4. 再用原命令 `ke BP_ETPlayerController_C ET_Stamina 0` 查询，满值After100 / Timer0。按2仍不消费体力是当前预期。

本批Core / 恢复源码未改，**不重复跑两项资源自动测试或旧M0测试**；若你实际修改过它们或此步失败，再按差异运行相关测试。

### 8.2 暂停与重建世界（约2–5分钟）

1. 注入-100，切回视口按P；暂停时绿色条不继续恢复。用一种已有方式继续，恢复更新、满值停止。若切窗口过慢，重做这一小步即可，不改速率。
2. 正常恢复后按原4键四次真实致死；出现唯一Failure页。鼠标Retry，新世界单生命 / 体力HUD均100。原4仍Damage25，不改Health默认值。
3. P → 返回菜单：旧体力View unsubscribed / removed与Presenter Stopped；主菜单不出现游戏HUD。选择Continue，再次进入当前会话出生点，两份资源面板均100，只有一份绿色体力条。

这组复用正式菜单 / Retry作为UI对象重建检查，不搭Controller临时函数，不重演所有Enter / Space / 背景焦点矩阵；不同世界的Widget尾号递增是新实例，正常，不要求同一对象复用。

### 8.3 Stop清理与短基线（约1–3分钟）

注入-100后Stop，观察旧体力View取消订阅、Bootstrap移除、Presenter停止；再Play新100。做一次左键真实25、E提示与移动转向跳跃，P的一种继续方式已在8.2完成；保存Widget / Controller及实际更改资产。最终100/100、Damage25/10、slomo1、Debug False保持。

本次手动覆盖Ready显示、数值 / 比例变化、暂停和世界切换 / Stop；NoPawn / Unsupported / Waiting、同世界单独重建View和旧ASC回调的防护按代码静态核对，**不能写成这次已逐项实测**。后续真正改变换Pawn或重建入口时再触发专项，不重复M1-03B3的整套临时图。

日常数值事件使用Verbose。仅排障需要时可输入 `log LogETStaminaPresenter Verbose` / `log LogETStaminaHUD Verbose`，复现后用对应`Display`恢复；不要仅因Display没有每0.1秒一行日志就认为没有刷新。无需新Cook / Insights，旧M1包无体力UI。

## 9. 最简反馈

```text
M2-01B2 完成 / 卡在第__节第__步
四新 / Bootstrap两旧Source；关闭UE后Editor Build / 索引44cpp / Game Build：
新WBP父类 / 四BindWidget / 无Bind / Never / Compile / Save / Valid：
单UIBootstrap / Stamina Widget Class，生命与原状态配置保留：
出生单生命 / 体力100，开发-25真实下降 / 逐步回满 / 生命隔离：
Pause冻结 / 恢复；真实死亡Retry新100 / 返回菜单Continue单面板：
未满Stop / 新一轮100 / View与Presenter结束清理：
左键25 / 原2不耗体力 / E / P与移动转向跳跃短基线：
Source / 资产Save All；失败或特别说明（无则填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

错误优先给首条真实编译错误 / 日志；控件名称、类型、父类或布局不符时发截图与资产名。反馈不用逐行摘录，我读取本机日志；无法截到精确体力值不要求重做已覆盖时序。

## 10. 本批 Commit / Push（固定放在最后）

- 四新UI Source：Presenter / Widget各两文件；旧Bootstrap.h / .cpp的完整修改。Core、恢复 / 测试、Editor校验器、Build.cs / Target / uproject本批无计划变化。
- 新 `/Game/EmberTrial/ui/HUD/WBP_ETStaminaHUD` 和配置Class后的BP_ETPlayerController实际.uasset差异，沿用Git LFS；原HealthHUD不应因为复制而被改父类或改名。来源清单待实物验收后由我补记真实资产。
- 本指南、B1验收 / 当前状态与最终B2报告、路线 / 架构实际更新；B1未提交实现先按其清单重新Stage，**旧Effects / Resource路径删除与新正确目录文件不能遗漏**。
- Raw / 日志、Binaries / Intermediate / Saved、CMake / IDE状态、旧包 / Trace留本地，不Force Add。

CLion刷新Changes → 重新Stage最新完整Source / 合法资产 / 文档 → 检查Staged Diff → Commit → Push。通过后可用 `feat: show stamina through event-driven HUD`；仅下发文档可用 `docs: prepare stamina HUD`。由用户操作Git；本指南下发不等于B2或正式成本已完成。
