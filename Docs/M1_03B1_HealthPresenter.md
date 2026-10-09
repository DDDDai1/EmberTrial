# M1-03B1：Health Presenter 与事件快照

交接日期：2026-10-04。前置：[A2总验收](Reports/M1_03A2_Verification.md)。**本批代码 / 蓝图由你手动实现，预计90–150分钟，正常构建 / 索引等待另计。** 本页代码按本机UE5.5.4接口核对，尚未写入工程 / 构建 / 实测。

## 1. 范围与职责

B分三批：B1完成Presenter / 订阅 / 查询；B2接最小血条Widget；B3验证真实身体替换、未就绪、旧数据失效与清理。本批已有控制状态面板保持原样，不要求出现血条。

~~~text
LocalPlayerSubsystem当前Pawn事件
  → HealthPresenter：就绪事件 + ASC属性事件 → 展示快照 / OnViewDataChanged
  → B2 Widget：订阅后立即读快照，事件SetText / SetPercent
~~~

| 职责 | 本批实现 |
|---|---|
| 所有权 | Controller上的唯一UIBootstrap用UPROPERTY持有一个Presenter；每个本地Controller自己的实例 |
| 引用 | Presenter弱引用Subsystem / Character / ASC，不延长旧身体的生命 |
| 就绪 | 先订阅OnGameplayReadyChanged，再立即查IsGameplayReady；不调用Ensure、不修改生命 |
| 变化 | ASC Health / MaxHealth用native delegate与FDelegateHandle订阅；同时查询两属性，忽略不变快照 |
| DTO | FETHealthViewData是展示缓存 / 查询结果，不能回写玩法；Unavailable等状态没有有效生命数值 |
| 清理 | Pawn变化先解绑旧角色 / ASC；Shutdown显式解绑；BeginDestroy仅作无广播的兜底清理 |

正式数据仍来自ASC。HealthFraction仅用于显示比例，不是一套生命规则。`Ready`视图要求就绪且健康快照有效，Health=0也可为Ready；死亡后续实现。

官方 [ASC API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/UAbilitySystemComponent?application_version=5.5) 提供GetGameplayAttributeValueChangeDelegate；本机GameplayEffectTypes.h定义其参数FOnAttributeChangeData。这里不用旧RegisterGameplayAttributeEvent、异步蓝图监听或UMG Viewmodel插件。

## 2. 准备与五份文件

Save All、退出PIE、关闭UE。CLion保存文件。确认角色上批最终Initial Health / Max Health为100/100；若仍是临时配置，先恢复并保存后再关闭。

| 文件 | 操作 |
|---|---|
| `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Presenters/ETHealthPresenter.h` | 手动创建目录 / 文件，全文见3 |
| `E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Presenters/ETHealthPresenter.cpp` | 手动创建目录 / 文件，全文见4 |
| [EmberTrialUI.Build.cs](E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/EmberTrialUI.Build.cs) | 只增加private GameplayAbilities依赖 |
| [ETUIBootstrapComponent.h](E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Public/Bootstrap/ETUIBootstrapComponent.h) | 新字段与接口 |
| [ETUIBootstrapComponent.cpp](E:/UE_Project/Demo/EmberTrial/Source/EmberTrialUI/Private/Bootstrap/ETUIBootstrapComponent.cpp) | 创建 / 重启观察 / 清理Presenter |

Build.cs的PrivateDependencyModuleNames保持EmberTrial，添加GameplayAbilities：

~~~csharp
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "EmberTrial",
            "GameplayAbilities"
        });
~~~

Public的Core / CoreUObject / Engine / UMG不动。Public Presenter头只前向声明GAS类型，native实现才包含GAS头；核心不依赖UI。不要新建模块、改角色 / AttributeSet或.uplugin。

## 3. ETHealthPresenter.h全文

~~~cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ETHealthPresenter.generated.h"

class APawn;
class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETUILocalPlayerSubsystem;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EETHealthViewState : uint8
{
    Unavailable,
    NoPawn,
    UnsupportedPawn,
    WaitingForGameplayReady,
    Ready
};

USTRUCT(BlueprintType)
struct EMBERTRIALUI_API FETHealthViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    EETHealthViewState State = EETHealthViewState::Unavailable;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    FString PawnName;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float Health = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float MaxHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|UI")
    float HealthFraction = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FETHealthViewChanged, const FETHealthViewData&, Data);

UCLASS(BlueprintType)
class EMBERTRIALUI_API UETHealthPresenter : public UObject
{
    GENERATED_BODY()

public:
    void StartObserving(UETUILocalPlayerSubsystem* Subsystem);
    void Shutdown();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    FETHealthViewData GetSnapshot() const;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|UI")
    FETHealthViewChanged OnViewDataChanged;

protected:
    virtual void BeginDestroy() override;

private:
    void StopObservingInternal(bool bNotify);
    void ClearPawnBindings();
    void ClearAttributeBindings();
    void RebindObservedPawn(const TCHAR* Reason);
    void RefreshReadyBinding(const TCHAR* Reason);
    FETHealthViewData BuildSnapshot() const;
    void PublishSnapshot(const TCHAR* Reason, bool bForce = false);

    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    void HandleAttributeChanged(const FOnAttributeChangeData& Change,
        TWeakObjectPtr<UAbilitySystemComponent> SourceASC);

    TWeakObjectPtr<UETUILocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<AEmberTrialCharacter> BoundCharacter;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle HealthHandle;
    FDelegateHandle MaxHealthHandle;
    FETHealthViewData Snapshot;
    bool bObserving = false;
};
~~~

generated.h为最后include。两个UFUNCTION处理dynamic事件；Attribute事件是native delegate，其参数不写UFUNCTION。State是有效性判断：只有Ready时三个数字可展示成生命；其他状态的0仅为占位，B2将隐藏 / 清空血条。

## 4. ETHealthPresenter.cpp全文

~~~cpp
#include "Presenters/ETHealthPresenter.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "GameFramework/Pawn.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "UObject/Class.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHealthPresenter, Log, All);

void UETHealthPresenter::StartObserving(UETUILocalPlayerSubsystem* Subsystem)
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
        this, &UETHealthPresenter::HandleObservedPawnChanged);
    RebindObservedPawn(TEXT("StartSnapshot"));
}

void UETHealthPresenter::Shutdown()
{
    StopObservingInternal(true);
}

void UETHealthPresenter::BeginDestroy()
{
    // GC兜底不广播给View，正常清理由Bootstrap的EndPlay负责。
    StopObservingInternal(false);
    Super::BeginDestroy();
}

FETHealthViewData UETHealthPresenter::GetSnapshot() const
{
    return Snapshot;
}

void UETHealthPresenter::StopObservingInternal(bool bNotify)
{
    const bool bWasObserving = bObserving;
    bObserving = false;
    if (UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get())
    {
        Subsystem->OnObservedPawnChanged.RemoveDynamic(
            this, &UETHealthPresenter::HandleObservedPawnChanged);
    }
    ClearPawnBindings();
    BoundSubsystem.Reset();

    if (bWasObserving)
    {
        UE_LOG(LogETHealthPresenter, Display,
            TEXT("[M1-03B1] Stopped Presenter=%s"), *GetNameSafe(this));
    }
    if (bNotify)
    {
        PublishSnapshot(TEXT("Shutdown"));
    }
    else
    {
        Snapshot = FETHealthViewData{};
    }
}

void UETHealthPresenter::ClearAttributeBindings()
{
    if (UAbilitySystemComponent* ASC = BoundASC.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(
            UETHealthAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(
            UETHealthAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthHandle);
    }
    HealthHandle.Reset();
    MaxHealthHandle.Reset();
    BoundASC.Reset();
}

void UETHealthPresenter::ClearPawnBindings()
{
    ClearAttributeBindings();
    if (AEmberTrialCharacter* Character = BoundCharacter.Get())
    {
        Character->OnGameplayReadyChanged.RemoveDynamic(
            this, &UETHealthPresenter::HandleGameplayReadyChanged);
    }
    BoundCharacter.Reset();
}

void UETHealthPresenter::RebindObservedPawn(const TCHAR* Reason)
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
            this, &UETHealthPresenter::HandleGameplayReadyChanged);
    }
    RefreshReadyBinding(Reason);
}

void UETHealthPresenter::RefreshReadyBinding(const TCHAR* Reason)
{
    AEmberTrialCharacter* Character = BoundCharacter.Get();
    UAbilitySystemComponent* ASC = Character && Character->IsGameplayReady()
        ? Character->GetAbilitySystemComponent() : nullptr;

    if (!IsValid(ASC))
    {
        ClearAttributeBindings();
    }
    else if (BoundASC.Get() != ASC || !HealthHandle.IsValid() || !MaxHealthHandle.IsValid())
    {
        ClearAttributeBindings();
        BoundASC = ASC;
        // Payload也是弱引用，用于拒绝旧ASC回调。
        HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(
            UETHealthAttributeSet::GetHealthAttribute()).AddUObject(
                this, &UETHealthPresenter::HandleAttributeChanged, BoundASC);
        MaxHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(
            UETHealthAttributeSet::GetMaxHealthAttribute()).AddUObject(
                this, &UETHealthPresenter::HandleAttributeChanged, BoundASC);
    }
    PublishSnapshot(Reason);
}

void UETHealthPresenter::HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    (void)CurrentPawn;
    if (bObserving)
    {
        RebindObservedPawn(TEXT("PawnChanged"));
    }
}

void UETHealthPresenter::HandleGameplayReadyChanged(AEmberTrialCharacter* Character, bool bReady)
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

void UETHealthPresenter::HandleAttributeChanged(const FOnAttributeChangeData& Change,
    TWeakObjectPtr<UAbilitySystemComponent> SourceASC)
{
    if (!bObserving || !SourceASC.IsValid() || SourceASC.Get() != BoundASC.Get())
    {
        return;
    }
    PublishSnapshot(Change.Attribute == UETHealthAttributeSet::GetHealthAttribute()
        ? TEXT("HealthChanged") : TEXT("MaxHealthChanged"));
}

FETHealthViewData UETHealthPresenter::BuildSnapshot() const
{
    FETHealthViewData Data;
    UETUILocalPlayerSubsystem* Subsystem = BoundSubsystem.Get();
    if (!bObserving || !IsValid(Subsystem))
    {
        return Data;
    }
    APawn* Pawn = Subsystem->GetObservedPawn();
    if (!IsValid(Pawn))
    {
        Data.State = EETHealthViewState::NoPawn;
        return Data;
    }
    Data.PawnName = Pawn->GetName();
    AEmberTrialCharacter* Character = Cast<AEmberTrialCharacter>(Pawn);
    if (!IsValid(Character))
    {
        Data.State = EETHealthViewState::UnsupportedPawn;
        return Data;
    }
    if (!Character->IsGameplayReady())
    {
        Data.State = EETHealthViewState::WaitingForGameplayReady;
        return Data;
    }

    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (Character != BoundCharacter.Get() || !BoundASC.IsValid() ||
        Character->GetAbilitySystemComponent() != BoundASC.Get() ||
        !Character->GetGameplayHealthSnapshot(Health, MaxHealth) ||
        !FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) ||
        MaxHealth <= 0.0f || Health < 0.0f || Health > MaxHealth)
    {
        return Data;
    }
    Data.State = EETHealthViewState::Ready;
    Data.Health = Health;
    Data.MaxHealth = MaxHealth;
    Data.HealthFraction = FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
    return Data;
}

void UETHealthPresenter::PublishSnapshot(const TCHAR* Reason, bool bForce)
{
    const FETHealthViewData NewData = BuildSnapshot();
    const bool bSame = Snapshot.State == NewData.State && Snapshot.PawnName == NewData.PawnName &&
        Snapshot.Health == NewData.Health && Snapshot.MaxHealth == NewData.MaxHealth &&
        Snapshot.HealthFraction == NewData.HealthFraction;
    if (bSame && !bForce)
    {
        return;
    }
    Snapshot = NewData;
    UE_LOG(LogETHealthPresenter, Display,
        TEXT("[M1-03B1] Snapshot Presenter=%s State=%s Pawn=%s Health=%.1f MaxHealth=%.1f Fraction=%.3f Reason=%s"),
        *GetNameSafe(this), *StaticEnum<EETHealthViewState>()->GetNameStringByValue(
            static_cast<int64>(Snapshot.State)), *Snapshot.PawnName,
        Snapshot.Health, Snapshot.MaxHealth, Snapshot.HealthFraction, Reason);
    OnViewDataChanged.Broadcast(Snapshot);
}
~~~

Health / MaxHealth的事件各自订阅，但事件到来后读取同一时刻的完整快照；最大值下降可能产生两次native回调，不变快照不会重复发给View。只有查询与比例计算，没有业务写入。StartObserving先清旧数据源订阅，View订阅保留；重新观察相同Ready角色会发布一份新快照，不重置GAS。

## 5. 接入UIBootstrap

### 5.1 头文件插入

在原 `class UETLifecycleStatusWidget;` 下新增 `class UETHealthPresenter;`。

public区域保留RebuildStatusWidget，添加：

~~~cpp
    UFUNCTION(BlueprintPure, Category = "EmberTrial|UI")
    UETHealthPresenter* GetHealthPresenter() const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|UI")
    void RestartHealthObservation();
~~~

private区域追加：

~~~cpp
    UPROPERTY(Transient)
    TObjectPtr<UETHealthPresenter> HealthPresenter;

    bool bUIShuttingDown = false;
~~~

### 5.2 cpp接入

新增三个include，保留原include：

~~~cpp
#include "Engine/LocalPlayer.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Presenters/ETHealthPresenter.h"
~~~

只替换BeginPlay / EndPlay全文；原构造函数、不Tick、CreateWidget / 移除逻辑保留：

~~~cpp
void UETUIBootstrapComponent::BeginPlay()
{
    bUIShuttingDown = false;
    Super::BeginPlay();
    RestartHealthObservation();
    RebuildStatusWidget();
}

void UETUIBootstrapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bUIShuttingDown = true;
    RemoveStatusWidget();
    if (HealthPresenter)
    {
        HealthPresenter->Shutdown();
        HealthPresenter = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}
~~~

在RebuildStatusWidget开头，把现有 `if (!HasBegunPlay())` 改为 `if (!HasBegunPlay() || bUIShuttingDown)`；原return与后续函数体保持，阻止结束时重建。

在cpp追加两份函数全文：

~~~cpp
UETHealthPresenter* UETUIBootstrapComponent::GetHealthPresenter() const
{
    return HealthPresenter.Get();
}

void UETUIBootstrapComponent::RestartHealthObservation()
{
    if (!HasBegunPlay() || bUIShuttingDown)
    {
        return;
    }
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() || !Controller->GetLocalPlayer())
    {
        if (HealthPresenter)
        {
            HealthPresenter->Shutdown();
        }
        return;
    }
    if (!HealthPresenter)
    {
        HealthPresenter = NewObject<UETHealthPresenter>(this);
    }
    HealthPresenter->StartObserving(
        Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>());
}
~~~

Restart重复使用同一Presenter对象，解绑 / 重绑其数据源；不会重建身体、属性集、Widget，也不清除View自己的订阅。正常EndPlay主动Shutdown，不能用等待GC替代清理。

## 6. 正常构建与首次观察

五份文件保存、UE关闭，PowerShell：

~~~powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
~~~

不加-SkipBuild；成功后CLion Reload CMake Project。新Presenter文件归属EmberTrialUICodeModel，GAS与核心公开头导航正常。再Game Build：

~~~powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
~~~

重开UE / 新地图PIE。Controller仍只有一个UIBootstrap，StatusWidgetClass原配置保持，不手动添加Presenter组件。正常出生应出现 `[M1-03B1] Snapshot ... State=Ready ... Health=100.0 MaxHealth=100.0 Fraction=1.000`。

初始化顺序可能先NoPawn / Waiting再Ready，也可能首次就是Ready；两者均可。关键是最终Ready且数据正确，不用Delay补初始化。Stop有Stopped及清理，没有结束后该Presenter再发布Ready。已有控制状态面板仍正常。

## 7. 临时蓝图验收入口

仅在BP_ETPlayerController测试，Compile / Save。使用顶部数字键Released，Execute When Paused=False；原1/2/3、E/P不动。

### 7.1 数字8：查询Presenter快照

从Components把 **UIBootstrap拖到Graph，选择Get**。其输出接Get Health Presenter的Target；返回值接带执行线的Is Valid.Input Object。

8 Released → Is Valid：无效Print `[M1-03B1] No Presenter`；有效Print完整快照。Get Snapshot的Target接Presenter返回值；Return Value接Break ETHealthViewData，Format Text格式：

~~~text
[M1-03B1] BP snapshot State={State} Pawn={Pawn} Health={H}/{M} Fraction={F}
~~~

State从Break.State拖出搜索Enum to String / To String再连接；Pawn接PawnName；H/M/F接Health / MaxHealth / HealthFraction。Text→String自动转换。Print to Log=True。不直接读取ASC作为替代，否则无法证明Presenter查询成功。

### 7.2 数字9：重启观察

9 Released → Restart Health Observation（Target=UIBootstrap Get）→ 同样的Print快照。可以将7.1有效打印部分折叠成临时函数ETTempPrintPresenterSnapshot，再让8 / 9调用它；若建立此函数，验收后一起删除。

按9三次，Presenter名字不变，每次重新订阅后都有一份快照；没有新的Health initialized / Ready=1，原角色当前属性不变。随后一次扣血只有一条实际变化Snapshot，验证没有叠加订阅。

### 7.3 数字6 / 7：真实GE修改

按 [A2-02第5节](M1_03A2_02_GameplayEffects.md#5-临时函数二ettempapplyhealtheffect) 重建临时ETTempApplyHealthEffect（EffectClass / CaseLabel），保留暂停 / Pawn / Ready / ASC门控、Make Effect Context与Apply Gameplay Effect to Self。

**本批把Apply后打印改为7.1的Presenter查询**，不必重建A2的Current / Base打印函数。6 Released调用Damage25 / D25；7 Released调用Heal40 / H40。按6出生100→75，应出现Presenter原生日志Reason=HealthChanged，8读75/100、Fraction=0.75；7回100/100、Fraction=1。函数与这两按键都临时使用。

### 7.4 数字4 / 5：同一身体失去 / 恢复控制

创建临时变量 **ETTempHealthPawn（Pawn Object Reference，默认None）**。

- 4 Released → Is Valid（Input Object=Get Controlled Pawn，Target=Self）：有效先Set ETTempHealthPawn=当前Pawn，再UnPossess（Target=Self）；无效只Print，不清空已保存变量。
- 5 Released → Is Valid（Input Object=Get ETTempHealthPawn）：无效只Print；有效再Branch（Condition=Get Controlled Pawn == ETTempHealthPawn）。True只Print `Already possessed`；False → Possess（Target=Self、In Pawn=ETTempHealthPawn）。

注意：4的IsValid检查**当前Pawn**；5检查**保存的ETTempHealthPawn**，不能全部留空或接Self。临时强引用仅用于这个控制测试，正式Presenter仍只用弱引用。无Spawn / Destroy / 额外ASC。

## 8. 分组验收

1. **首次 / 查询**：100/100出生，Presenter最终Ready；8快照100/100 / 1，旧状态面板正常。Presenter不Tick、不初始化角色。
2. **属性事件**：6→75/100 / 0.75；7→100/100 / 1；再7无数值变化，不产生一条新的变化Snapshot（BP主动Print仍可出现）。再次6→75。无Tick即可更新native快照。
3. **重启幂等**：75/100时9三次，对象名字不变、快照仍75/100；没有新Health initialized / Ready=1。随后6一次→50/100，只产生一条HealthChanged变化Snapshot；7一次→90/100。
4. **失去 / 恢复**：90/100时4→8：State=NoPawn、Pawn空、数字0为不可用占位；5→8：同一身体Ready、90/100 / 0.9，无重置。三轮4→5，重复5不Possess / 不重复变化通知；原移动输入恢复。
5. **未就绪**：Stop，角色Class Defaults临时Initial Health=100 / Initial Max Health=60，Compile / Save。新PIE出现A2预期Warning，Presenter为WaitingForGameplayReady；8为该状态，不能显示成Ready的0血量；6/7被测试入口拒绝。9三次仍Waiting，不执行Ensure或初始化。
6. **恢复 / 结束**：Stop，恢复100/100，Compile / Save；连续三次PIE，最终Ready100/100。Stop解绑 / 清理；没有旧Presenter在结束后再Ready，没有Accessed None / 脚本错误。暂停菜单完整按下 / 松开6/7/8/9后继续，数值不变，无补执行 / 卡键。

本批验证原生事件→Presenter快照与重订阅；**View对OnViewDataChanged的真实订阅 / 解绑在B2测试**。UnsupportedPawn、真正的新角色实例、旧身体属性变化不能影响新View等场景留B3；不声称它们已通过。本批也不刻意改变角色初始化时序来制造晚Ready，时序专项在B3安排。

## 9. 清理、回归与反馈

Stop，删除Controller临时4–9事件、ETTempHealthPawn变量、ETTempApplyHealthEffect及可选ETTempPrintPresenterSnapshot函数。UIBootstrap / Presenter原生实现保留，角色最终100/100。Compile / Save；原1/2/3、2→3、E-A/B、P鼠标 / 键盘 / 背景恢复、移动 / 转向 / 跳跃回归；Stop / Save All。

~~~text
M1-03B1 完成 / 卡在第 __ 节
Presenter两文件 / UI依赖 / Bootstrap接入已保存：是
关闭UE后Editor Build / 索引刷新 / Game Build：成功
UI CodeModel / GAS与核心头导航：通过
单个UIBootstrap，旧状态Widget配置保留：是
出生Presenter Ready100/100 / Fraction=1；8查询：通过
6/7属性事件与比例、满血不变快照去重：通过
75/100时9三次，对象不换 / 属性不重置：通过
重启观察后一次扣血只一条变化Snapshot：通过
三轮4→5 / NoPawn→Ready / 同一身体属性保留：通过
重复5无重复Possess，恢复基础输入：通过
非法100/60时Waiting，6/7拒绝、9不初始化：通过
恢复100/100，三次PIE清理 / 无结束后旧Ready：通过
暂停菜单6/7/8/9无执行 / 无补播不卡键：通过
临时输入 / 变量 / 函数已删除，原生实现保留：是
角色100/100，原1/2/3、2→3、E/P与基础输入：正常
源码 / 资产已保存：是
大致完成时间：今天 __:__
实际手动用时：约 __ 分钟；构建 / 索引等待：约 __ 分钟
~~~

报错先发首个具体error、文件 / 行号 / 上下文；时序问题给发生前后的Snapshot / Ready日志；蓝图连线截图应包含执行线和数据引脚。普通报错不授权助手直接改C++，需要接管时请明确提出。
