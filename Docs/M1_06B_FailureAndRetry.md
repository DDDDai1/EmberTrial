# M1-06B：真实死亡、失败界面与整图重试

2026-10-18清理同步：本页保留历史实施 / 配置 / 排障经验，当前整文件实现以Source链接为准，不按旧全文覆盖现有工程。当前已接菜单继续、原生暂停与检查点；这轮清理由助手按用户授权直接修改，普通功能开发的手动实践约定保留。验证与本批Push清单见[Cleanup](Cleanup.md)。

下发 / 验收：2026-10-11；交接更新：2026-10-12；UE **5.5.4**。**本批已由用户手动实施并验收通过，无待补功能检查。**[B报告](Reports/M1_06B_Verification.md)记录验收时65Source / 32cpp / 清单45，手动80（代码40 / 资产30 / 检查10）/ 等待5。后续B完整实现进入4b83b66，B报告亦随3351b53提交，旧AM骨架 / 个人设置删除不重复处理；C现已验收，当前67 / 33 / 46。HEAD / 缓存origin一致，本轮未联网验证远端。下文保留历史实施指南，14.1.1重新Stage步骤可复用；当前文档收尾见[C末尾清单](M1_06C_SessionCheckpoints.md#13-本批需要commit--push的内容固定放在文档最后)。

原实施按C++先写文档、由你手动实现；当前清理按本轮授权由助手直接修改。使用[方案B](AcceptanceWorkflow_Options.md)，验收走正式重试 / 返回菜单按钮，只有一个很小的旧P菜单门控改动，没有临时Controller测试图。目录与文件名大小写按表填写，generated include始终是头文件最后一个include。

## 1. 本批完成后的效果与时间

玩家被敌人真实打死，或用现有4伤害入口归零后，先完成原死亡封锁，再出现一份“试炼失败”页。按钮为**重新试炼 / 返回主菜单**；失败页暂停当前世界、显示鼠标并聚焦重试。点击或键盘确认重试后重载当前战斗地图：旧角色 / 敌人 / HUD结束，新身体与敌人重新100/100，输入恢复，无旧尸体复活。

返回菜单同样先恢复世界推进再切图，菜单仍不生成Pawn / HUD；继续按钮本批保持禁用。**B重试出生在现有入口PlayerStart，检查点在C接入；没有磁盘存档或跨程序继续。**游戏默认地图仍为菜单，日常编辑器启动仍为正式地图，不新增地图或Cook条目。

原预算手动**90–150分钟**：代码40–70、Designer / 装配30–50、检查10–15，构建等待另计。本批6新 / 7旧Source、1新Widget、2旧BP；你复制代码较快时以实际反馈计时，不为凑预算重复检查。美术新增支出0元。

| 对象 | 职责 |
| --- | --- |
| Core RunSession | 当前进程的地图标识、一次切图请求、预检 / 加载后确认 / 失败释放；不持有Pawn / ASC / Widget，不依赖UMG |
| Core CombatLife | 原死亡取消 / 封锁完成后发一次原生通知；不创建界面或暂停世界 |
| UI FailureFlow组件 | Controller上的当前Pawn / Life订阅、失败View、暂停 / 焦点、按钮转成会话命令；旧源显式解绑 |
| UI FailureWidget | 必要控件 / 按钮意图 / 文案；不查询属性、不OpenLevel、不保存地图 |
| GameMode / Menu Controller | 正式世界登记当前地图；菜单开始接会话服务；保留既有HUD / 输入初始化 |
| Blueprint | Designer与组件Class / 地图引用装配；原角色OpenPauseMenu开头加死亡拒绝 |

## 2. 准备、顺序与完整文件范围

1. Stop PIE，Save All并关闭UE。CLion留在整个EmberTrial工作空间，先完成全部C++再正常Build；不靠Live Coding补UCLASS / 委托。
2. 第3–5节创建六个新文件；第6–7节局部追加，第8节全文替换主菜单Controller两文件。新文件不是用UE向导生成到默认核心模块，按完整路径在CLion对应目录创建即可。
3. 第9节正常Editor / 索引 / Game Build；成功后再进入UE，执行第10节Designer与两个BP配置。
4. 第11节仅三组相关检查；第12节反馈。最后第14节列出本批Commit / Push内容。

| 操作 | 完整路径 |
| --- | --- |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Flow\ETRunSessionSubsystem.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Flow\ETRunSessionSubsystem.cpp` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Widgets\ETFailureWidget.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Widgets\ETFailureWidget.cpp` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\Flow\ETFailureFlowComponent.h` |
| 新建 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\Flow\ETFailureFlowComponent.cpp` |
| 局部追加 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Combat\ETCombatLifeComponent.h` |
| 局部追加 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Combat\ETCombatLifeComponent.cpp` |
| 局部追加 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Framework\ETMenuGameMode.h` |
| 局部追加 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETMenuGameMode.cpp` |
| 局部追加 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Framework\ETGameMode.cpp` |
| 替换全文 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Public\FrontEnd\ETMainMenuController.h` |
| 替换全文 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialUI\Private\FrontEnd\ETMainMenuController.cpp` |

实际新增6 / 修改7，共13个Source文件。Core / UI API宏分别EMBERTRIAL_API / EMBERTRIALUI_API。现有Build.cs依赖已满足，**不改Build.cs / Target / .uproject / GameInstance Class**；GameInstanceSubsystem由引擎实例化，不挂角色 / Controller组件、不新建GI蓝图。原UIBootstrap、Presenter、HUD、战斗 / AI与ETPlayerController.cpp不改。

## 3. Core：跨地图的RunSession

### 3.1 ETRunSessionSubsystem.h全文

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "ETRunSessionSubsystem.generated.h"

class UWorld;

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FETRunTravelFailed, UWorld*, const FText&);

enum class EETRunTravelIntent : uint8
{
    None,
    NewRun,
    Retry,
    MainMenu
};

UCLASS()
class EMBERTRIAL_API UETRunSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void NotifyGameplayWorldReady(UWorld* GameplayWorld);
    bool RequestNewRun(UWorld* RequestWorld, const FSoftObjectPath& Map,
        FText& OutError);
    bool RequestRetry(UWorld* RequestWorld, FText& OutError);
    bool RequestMainMenu(UWorld* RequestWorld, const FSoftObjectPath& Map,
        FText& OutError);
    bool IsTravelPending() const { return bTravelPending; }

    FETRunTravelFailed OnTravelFailed;

private:
    bool BeginTravel(UWorld* RequestWorld, const FString& TargetPackage,
        EETRunTravelIntent Intent, FText& OutError);
    bool RejectRequest(const TCHAR* Reason, FText& OutError);
    void HandlePostLoadMap(UWorld* LoadedWorld);
    void HandleTravelFailure(UWorld* FailedWorld,
        ETravelFailure::Type FailureType, const FString& ErrorString);
    void ResetPendingTravel();
    static FString GetWorldPackage(UWorld* World);
    static const TCHAR* GetIntentName(EETRunTravelIntent Intent);

    FString GameplayPackage;
    FString PendingTargetPackage;
    EETRunTravelIntent PendingIntent = EETRunTravelIntent::None;
    TWeakObjectPtr<UWorld> OriginWorld;
    FDelegateHandle PostLoadHandle;
    FDelegateHandle TravelFailureHandle;
    bool bTravelPending = false;
    bool bShuttingDown = false;
};
```

### 3.2 ETRunSessionSubsystem.cpp全文

```cpp
#include "Flow/ETRunSessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRunSession, Log, All);

void UETRunSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bShuttingDown = false;
    ResetPendingTravel();
    GameplayPackage.Reset();
    PostLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &ThisClass::HandlePostLoadMap);
    if (GEngine)
    {
        TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
            this, &ThisClass::HandleTravelFailure);
    }
    UE_LOG(LogETRunSession, Display,
        TEXT("[M1-06B] Session ready Session=%s"), *GetNameSafe(this));
}

void UETRunSessionSubsystem::Deinitialize()
{
    bShuttingDown = true;
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadHandle);
    PostLoadHandle.Reset();
    if (GEngine && TravelFailureHandle.IsValid())
    {
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    }
    TravelFailureHandle.Reset();
    ResetPendingTravel();
    OnTravelFailed.Clear();
    GameplayPackage.Reset();
    UE_LOG(LogETRunSession, Display,
        TEXT("[M1-06B] Session end Session=%s"), *GetNameSafe(this));
    Super::Deinitialize();
}

FString UETRunSessionSubsystem::GetWorldPackage(UWorld* World)
{
    return IsValid(World)
        ? UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) : FString();
}

const TCHAR* UETRunSessionSubsystem::GetIntentName(EETRunTravelIntent Intent)
{
    switch (Intent)
    {
    case EETRunTravelIntent::NewRun: return TEXT("NewRun");
    case EETRunTravelIntent::Retry: return TEXT("Retry");
    case EETRunTravelIntent::MainMenu: return TEXT("MainMenu");
    default: return TEXT("None");
    }
}

void UETRunSessionSubsystem::NotifyGameplayWorldReady(UWorld* GameplayWorld)
{
    if (bShuttingDown || !IsValid(GameplayWorld) ||
        GameplayWorld->GetGameInstance() != GetGameInstance() ||
        GameplayWorld->GetNetMode() != NM_Standalone)
    {
        return;
    }
    const FString Package = GetWorldPackage(GameplayWorld);
    if (!bTravelPending && FPackageName::IsValidLongPackageName(Package) &&
        FPackageName::DoesPackageExist(Package))
    {
        // Direct PIE in the gameplay map is also a valid starting session.
        GameplayPackage = Package;
    }
    UE_LOG(LogETRunSession, Display,
        TEXT("[M1-06B] Gameplay world ready Map=%s Pending=%d"),
        *Package, bTravelPending ? 1 : 0);
}

bool UETRunSessionSubsystem::RequestNewRun(UWorld* RequestWorld,
    const FSoftObjectPath& Map, FText& OutError)
{
    return BeginTravel(RequestWorld, Map.GetLongPackageName(),
        EETRunTravelIntent::NewRun, OutError);
}

bool UETRunSessionSubsystem::RequestRetry(UWorld* RequestWorld, FText& OutError)
{
    if (GameplayPackage.IsEmpty() ||
        GetWorldPackage(RequestWorld) != GameplayPackage)
    {
        return RejectRequest(TEXT("当前试炼地图尚未登记，不能重试。"), OutError);
    }
    return BeginTravel(RequestWorld, GameplayPackage,
        EETRunTravelIntent::Retry, OutError);
}

bool UETRunSessionSubsystem::RequestMainMenu(UWorld* RequestWorld,
    const FSoftObjectPath& Map, FText& OutError)
{
    return BeginTravel(RequestWorld, Map.GetLongPackageName(),
        EETRunTravelIntent::MainMenu, OutError);
}

bool UETRunSessionSubsystem::RejectRequest(const TCHAR* Reason, FText& OutError)
{
    OutError = FText::FromString(Reason);
    UE_LOG(LogETRunSession, Warning,
        TEXT("[M1-06B] Travel rejected Reason=%s"), Reason);
    return false;
}

bool UETRunSessionSubsystem::BeginTravel(UWorld* RequestWorld,
    const FString& TargetPackage, EETRunTravelIntent Intent, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (bShuttingDown || bTravelPending || !IsValid(RequestWorld) ||
        RequestWorld->bIsTearingDown || !RequestWorld->HasBegunPlay() ||
        RequestWorld->GetGameInstance() != GetGameInstance() ||
        RequestWorld->GetNetMode() != NM_Standalone)
    {
        return RejectRequest(TEXT("当前无法切换地图，请稍后重试。"), OutError);
    }
    if (!FPackageName::IsValidLongPackageName(TargetPackage) ||
        !FPackageName::DoesPackageExist(TargetPackage) ||
        (Intent != EETRunTravelIntent::Retry &&
            TargetPackage == GetWorldPackage(RequestWorld)))
    {
        return RejectRequest(TEXT("目标地图不可用，请检查地图配置。"), OutError);
    }
    bTravelPending = true;
    PendingIntent = Intent;
    PendingTargetPackage = TargetPackage;
    OriginWorld = RequestWorld;
    if (UGameplayStatics::IsGamePaused(RequestWorld) &&
        !UGameplayStatics::SetGamePaused(RequestWorld, false))
    {
        ResetPendingTravel();
        return RejectRequest(TEXT("无法解除世界暂停，未执行切图。"), OutError);
    }
    UE_LOG(LogETRunSession, Display,
        TEXT("[M1-06B] Travel requested Intent=%s From=%s To=%s Paused=%d"),
        GetIntentName(Intent), *GetWorldPackage(RequestWorld), *TargetPackage,
        UGameplayStatics::IsGamePaused(RequestWorld) ? 1 : 0);
    UGameplayStatics::OpenLevel(RequestWorld, FName(*TargetPackage), true);
    return true;
}

void UETRunSessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (bShuttingDown || !bTravelPending || !IsValid(LoadedWorld) ||
        LoadedWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }
    const FString LoadedPackage = GetWorldPackage(LoadedWorld);
    const EETRunTravelIntent CompletedIntent = PendingIntent;
    const bool bExpectedMap = LoadedPackage == PendingTargetPackage;
    if (bExpectedMap && CompletedIntent == EETRunTravelIntent::NewRun)
    {
        // Commit new-run data only after this GameInstance arrived.
        GameplayPackage = PendingTargetPackage;
    }
    ResetPendingTravel();
    UE_LOG(LogETRunSession, Display,
        TEXT("[M1-06B] Travel arrived Intent=%s Map=%s Expected=%d Pending=0"),
        GetIntentName(CompletedIntent), *LoadedPackage, bExpectedMap ? 1 : 0);
    if (!bExpectedMap)
    {
        OnTravelFailed.Broadcast(LoadedWorld, FText::FromString(
            TEXT("未到达请求的地图，请检查配置后重试。")));
    }
}

void UETRunSessionSubsystem::HandleTravelFailure(UWorld* FailedWorld,
    ETravelFailure::Type FailureType, const FString& ErrorString)
{
    if (bShuttingDown || !bTravelPending || !IsValid(FailedWorld) ||
        (FailedWorld != OriginWorld.Get() &&
            FailedWorld->GetGameInstance() != GetGameInstance()))
    {
        return;
    }
    ResetPendingTravel();
    UE_LOG(LogETRunSession, Warning,
        TEXT("[M1-06B] Travel failed Type=%d Error=%s Pending=0"),
        static_cast<int32>(FailureType), *ErrorString);
    OnTravelFailed.Broadcast(FailedWorld, FText::FromString(
        TEXT("加载地图失败，请重试或返回主菜单。")));
}

void UETRunSessionSubsystem::ResetPendingTravel()
{
    bTravelPending = false;
    PendingIntent = EETRunTravelIntent::None;
    PendingTargetPackage.Reset();
    OriginWorld.Reset();
}
```

只保存包名字符串，OriginWorld仅为当前切图请求的弱引用，确认 / 失败 / 结束都释放；不加载或强持有旧地图。请求有Widget、UI拥有者与会话三层门控；拒绝发生在解除暂停之前，失败页仍可操作。进入正式地图的GI保持，下一轮PIE才重建会话。PostLoad回调过滤所属GI，避免另一PIE窗口的地图替本会话解锁。

本批只亲测正常到达与None引用预检；真正LoadMap失败可能已经拆掉原世界，不能保证原失败页还存在。保留原世界时UI恢复按钮 / 焦点；引擎进入其他缺省地图时清请求并向所属新世界报告，独立包的完整故障恢复另测。不要为负例删除 / 损坏地图文件。

## 4. UI：失败页只发按钮意图

### 4.1 ETFailureWidget.h全文

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ETFailureWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FETFailureActionRequested);

UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class EMBERTRIALUI_API UETFailureWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    bool HasRequiredLayout() const;
    void ShowReady();
    void ShowPending(const FText& Message);
    void ShowError(const FText& Message);
    void FocusRetryButton();

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Failure")
    FETFailureActionRequested OnRetryRequested;

    UPROPERTY(BlueprintAssignable, Category = "EmberTrial|Failure")
    FETFailureActionRequested OnMainMenuRequested;

protected:
    virtual void NativeOnInitialized() override;

private:
    void ApplyState(bool bPending, const FText& Message);

    UFUNCTION()
    void HandleRetryClicked();

    UFUNCTION()
    void HandleMainMenuClicked();

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> RetryButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MainMenuButton;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StatusText;

    bool bBusy = false;
};
```

### 4.2 ETFailureWidget.cpp全文

```cpp
#include "Widgets/ETFailureWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

bool UETFailureWidget::HasRequiredLayout() const
{
    return IsValid(RetryButton.Get()) && IsValid(MainMenuButton.Get()) &&
        IsValid(StatusText.Get());
}

void UETFailureWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!HasRequiredLayout())
    {
        return;
    }
    RetryButton->OnClicked.AddUniqueDynamic(
        this, &ThisClass::HandleRetryClicked);
    MainMenuButton->OnClicked.AddUniqueDynamic(
        this, &ThisClass::HandleMainMenuClicked);
}

void UETFailureWidget::ShowReady()
{
    ApplyState(false, FText::FromString(TEXT("重新试炼将从入口开始。")));
}

void UETFailureWidget::ShowPending(const FText& Message)
{
    ApplyState(true, Message);
}

void UETFailureWidget::ShowError(const FText& Message)
{
    ApplyState(false, Message);
}

void UETFailureWidget::ApplyState(bool bPending, const FText& Message)
{
    bBusy = bPending;
    if (HasRequiredLayout())
    {
        RetryButton->SetIsEnabled(!bBusy);
        MainMenuButton->SetIsEnabled(!bBusy);
        StatusText->SetText(Message);
    }
}

void UETFailureWidget::FocusRetryButton()
{
    APlayerController* Controller = GetOwningPlayer();
    if (!bBusy && IsValid(RetryButton.Get()) && IsValid(Controller))
    {
        RetryButton->SetUserFocus(Controller);
    }
}

void UETFailureWidget::HandleRetryClicked()
{
    if (!bBusy)
    {
        OnRetryRequested.Broadcast();
    }
}

void UETFailureWidget::HandleMainMenuClicked()
{
    if (!bBusy)
    {
        OnMainMenuRequested.Broadcast();
    }
}
```

## 5. UI：Controller上的失败流程组件

### 5.1 ETFailureFlowComponent.h全文

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETFailureFlowComponent.generated.h"

class APawn;
class APlayerController;
class UWorld;
class UETCombatLifeComponent;
class UETFailureWidget;
class UETRunSessionSubsystem;
class UETUILocalPlayerSubsystem;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIALUI_API UETFailureFlowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETFailureFlowComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleObservedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);

    UFUNCTION()
    void HandleRetryRequested();

    UFUNCTION()
    void HandleMainMenuRequested();

    void BindPawn(APawn* CurrentPawn);
    void StopObservingLife();
    void HandleDeathSealed(UETCombatLifeComponent* SourceLife);
    bool HasCurrentDeadBody() const;
    void QueueFailure();
    void PresentFailure();
    void RecoverFailure(const FText& Message);
    void HandleSessionFailure(UWorld* FailedWorld, const FText& Message);
    void RemoveFailureWidget();
    void RestoreOwnedInput();
    void CancelPresentation();

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSubclassOf<UETFailureWidget> FailureWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Failure")
    TSoftObjectPtr<UWorld> MainMenuMap;

    UPROPERTY(Transient)
    TObjectPtr<UETFailureWidget> FailureWidget;

    TWeakObjectPtr<APlayerController> ControllerOwner;
    TWeakObjectPtr<UETUILocalPlayerSubsystem> UIContext;
    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    TWeakObjectPtr<UETCombatLifeComponent> BoundLife;
    FDelegateHandle DeathHandle;
    FDelegateHandle SessionFailureHandle;
    FTimerHandle PresentationHandle;
    bool bPresentationQueued = false;
    bool bCommandPending = false;
    bool bOwnsPause = false;
    bool bEndingPlay = false;
};
```

### 5.2 ETFailureFlowComponent.cpp全文

```cpp
#include "Flow/ETFailureFlowComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Logging/LogMacros.h"
#include "Widgets/ETFailureWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogETFailureFlow, Log, All);

UETFailureFlowComponent::UETFailureFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bAutoActivate = true;
    MainMenuMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(
        TEXT("/Game/EmberTrial/Maps/L_ET_MainMenu.L_ET_MainMenu")));
}

void UETFailureFlowComponent::BeginPlay()
{
    bEndingPlay = false;
    Super::BeginPlay();
    APlayerController* Controller = Cast<APlayerController>(GetOwner());
    if (!IsValid(Controller) || !Controller->IsLocalController() ||
        !Controller->GetLocalPlayer() || !GetWorld() ||
        GetWorld()->GetNetMode() != NM_Standalone)
    {
        return;
    }
    ControllerOwner = Controller;
    UIContext = Controller->GetLocalPlayer()->GetSubsystem<UETUILocalPlayerSubsystem>();
    UGameInstance* Instance = Controller->GetGameInstance();
    Session = Instance ? Instance->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
    if (!UIContext.IsValid() || !Session.IsValid())
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06B] Missing UI context or session Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }
    UIContext->OnObservedPawnChanged.AddUniqueDynamic(
        this, &ThisClass::HandleObservedPawnChanged);
    SessionFailureHandle = Session->OnTravelFailed.AddUObject(
        this, &ThisClass::HandleSessionFailure);
    BindPawn(UIContext->GetObservedController() == Controller
        ? UIContext->GetObservedPawn() : nullptr);
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure flow ready Owner=%s Session=%s"),
        *GetNameSafe(Controller), *GetNameSafe(Session.Get()));
}

void UETFailureFlowComponent::HandleObservedPawnChanged(
    APawn* PreviousPawn, APawn* CurrentPawn)
{
    (void)PreviousPawn;
    if (!bEndingPlay && UIContext.IsValid())
    {
        BindPawn(UIContext->GetObservedController() == ControllerOwner.Get()
            ? CurrentPawn : nullptr);
    }
}

void UETFailureFlowComponent::BindPawn(APawn* CurrentPawn)
{
    AEmberTrialCharacter* Body = Cast<AEmberTrialCharacter>(CurrentPawn);
    UETCombatLifeComponent* NextLife = IsValid(Body)
        ? Body->FindComponentByClass<UETCombatLifeComponent>() : nullptr;
    if (IsValid(NextLife) && BoundLife.Get() == NextLife && DeathHandle.IsValid())
    {
        if (HasCurrentDeadBody())
        {
            QueueFailure();
        }
        return;
    }
    CancelPresentation();
    StopObservingLife();
    RemoveFailureWidget();
    RestoreOwnedInput();
    bCommandPending = false;
    if (bEndingPlay || !IsValid(NextLife))
    {
        return;
    }
    BoundLife = NextLife;
    DeathHandle = NextLife->OnDeathSealed.AddUObject(
        this, &ThisClass::HandleDeathSealed);
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Life bound Owner=%s Body=%s Dead=%d"),
        *GetNameSafe(GetOwner()), *GetNameSafe(CurrentPawn),
        NextLife->IsDead() ? 1 : 0);
    if (HasCurrentDeadBody())
    {
        QueueFailure();
    }
}

void UETFailureFlowComponent::StopObservingLife()
{
    if (UETCombatLifeComponent* Life = BoundLife.Get())
    {
        Life->OnDeathSealed.Remove(DeathHandle);
        UE_LOG(LogETFailureFlow, Display,
            TEXT("[M1-06B] Life unbound Owner=%s Body=%s"),
            *GetNameSafe(GetOwner()), *GetNameSafe(Life->GetOwner()));
    }
    DeathHandle.Reset();
    BoundLife.Reset();
}

bool UETFailureFlowComponent::HasCurrentDeadBody() const
{
    APlayerController* Controller = ControllerOwner.Get();
    UETCombatLifeComponent* Life = BoundLife.Get();
    UWorld* World = GetWorld();
    AEmberTrialCharacter* Body = IsValid(Controller)
        ? Cast<AEmberTrialCharacter>(Controller->GetPawn()) : nullptr;
    return !bEndingPlay && IsValid(World) && !World->bIsTearingDown &&
        IsValid(Controller) && Controller->IsLocalController() &&
        UIContext.IsValid() && UIContext->GetObservedController() == Controller &&
        UIContext->GetObservedPawn() == Body && IsValid(Body) &&
        Body->IsGameplayReady() && IsValid(Life) && Life->GetOwner() == Body &&
        Life->IsDead();
}

void UETFailureFlowComponent::HandleDeathSealed(UETCombatLifeComponent* SourceLife)
{
    if (SourceLife == BoundLife.Get() && HasCurrentDeadBody())
    {
        QueueFailure();
    }
}

void UETFailureFlowComponent::QueueFailure()
{
    if (!HasCurrentDeadBody() || bPresentationQueued ||
        IsValid(FailureWidget.Get()) || bCommandPending)
    {
        return;
    }
    bPresentationQueued = true;
    // Run after this frame's death callbacks and Controller BeginPlay input setup.
    PresentationHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(
        this, &ThisClass::PresentFailure);
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure queued Owner=%s Body=%s"),
        *GetNameSafe(GetOwner()), *GetNameSafe(BoundLife->GetOwner()));
}

void UETFailureFlowComponent::CancelPresentation()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PresentationHandle);
    }
    PresentationHandle.Invalidate();
    bPresentationQueued = false;
}

void UETFailureFlowComponent::PresentFailure()
{
    bPresentationQueued = false;
    PresentationHandle.Invalidate();
    if (!HasCurrentDeadBody() || IsValid(FailureWidget.Get()))
    {
        return;
    }
    APlayerController* Controller = ControllerOwner.Get();
    if (!FailureWidgetClass || FailureWidgetClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06B] Missing concrete FailureWidgetClass Owner=%s"),
            *GetNameSafe(Controller));
        return;
    }
    FailureWidget = CreateWidget<UETFailureWidget>(Controller, FailureWidgetClass);
    if (!IsValid(FailureWidget.Get()) || !FailureWidget->HasRequiredLayout())
    {
        UE_LOG(LogETFailureFlow, Warning,
            TEXT("[M1-06B] Failure layout invalid Owner=%s"), *GetNameSafe(Controller));
        RemoveFailureWidget();
        return;
    }
    FailureWidget->OnRetryRequested.AddUniqueDynamic(
        this, &ThisClass::HandleRetryRequested);
    FailureWidget->OnMainMenuRequested.AddUniqueDynamic(
        this, &ThisClass::HandleMainMenuRequested);
    FailureWidget->ShowReady();
    if (!FailureWidget->AddToPlayerScreen(200))
    {
        RemoveFailureWidget();
        return;
    }
    const bool bWasPaused = UGameplayStatics::IsGamePaused(this);
    if (!bWasPaused && !UGameplayStatics::SetGamePaused(this, true))
    {
        UE_LOG(LogETFailureFlow, Warning, TEXT("[M1-06B] Failure pause rejected"));
        RemoveFailureWidget();
        return;
    }
    bOwnsPause = !bWasPaused;
    bCommandPending = false;
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        Controller, FailureWidget.Get(), EMouseLockMode::DoNotLock, true);
    FailureWidget->FocusRetryButton();
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure shown Owner=%s Body=%s Widget=%s Paused=%d"),
        *GetNameSafe(Controller), *GetNameSafe(BoundLife->GetOwner()),
        *GetNameSafe(FailureWidget.Get()), UGameplayStatics::IsGamePaused(this) ? 1 : 0);
}

void UETFailureFlowComponent::HandleRetryRequested()
{
    if (bCommandPending || !HasCurrentDeadBody() || !IsValid(FailureWidget.Get()))
    {
        return;
    }
    bCommandPending = true;
    FailureWidget->ShowPending(FText::FromString(TEXT("正在重新进入试炼…")));
    FText Error;
    if (!Session.IsValid() || !Session->RequestRetry(GetWorld(), Error))
    {
        RecoverFailure(Error.IsEmpty()
            ? FText::FromString(TEXT("试炼会话不可用，请重新开始。")) : Error);
    }
}

void UETFailureFlowComponent::HandleMainMenuRequested()
{
    if (bCommandPending || !HasCurrentDeadBody() || !IsValid(FailureWidget.Get()))
    {
        return;
    }
    bCommandPending = true;
    FailureWidget->ShowPending(FText::FromString(TEXT("正在返回主菜单…")));
    FText Error;
    if (!Session.IsValid() ||
        !Session->RequestMainMenu(GetWorld(), MainMenuMap.ToSoftObjectPath(), Error))
    {
        RecoverFailure(Error.IsEmpty()
            ? FText::FromString(TEXT("试炼会话不可用，请重试。")) : Error);
    }
}

void UETFailureFlowComponent::HandleSessionFailure(
    UWorld* FailedWorld, const FText& Message)
{
    if (!bEndingPlay && FailedWorld == GetWorld() &&
        bCommandPending && IsValid(FailureWidget.Get()))
    {
        RecoverFailure(Message);
    }
}

void UETFailureFlowComponent::RecoverFailure(const FText& Message)
{
    bCommandPending = false;
    if (!HasCurrentDeadBody() || !IsValid(FailureWidget.Get()))
    {
        return;
    }
    if (!UGameplayStatics::IsGamePaused(this))
    {
        bOwnsPause = UGameplayStatics::SetGamePaused(this, true);
    }
    FailureWidget->ShowError(Message);
    APlayerController* Controller = ControllerOwner.Get();
    Controller->bShowMouseCursor = true;
    UWidgetBlueprintLibrary::SetInputMode_UIOnlyEx(
        Controller, FailureWidget.Get(), EMouseLockMode::DoNotLock, true);
    FailureWidget->FocusRetryButton();
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure recovered Owner=%s Message=%s"),
        *GetNameSafe(Controller), *Message.ToString());
}

void UETFailureFlowComponent::RemoveFailureWidget()
{
    if (IsValid(FailureWidget.Get()))
    {
        const FString WidgetName = GetNameSafe(FailureWidget.Get());
        FailureWidget->OnRetryRequested.RemoveDynamic(
            this, &ThisClass::HandleRetryRequested);
        FailureWidget->OnMainMenuRequested.RemoveDynamic(
            this, &ThisClass::HandleMainMenuRequested);
        FailureWidget->RemoveFromParent();
        UE_LOG(LogETFailureFlow, Display,
            TEXT("[M1-06B] Failure removed Owner=%s Widget=%s"),
            *GetNameSafe(GetOwner()), *WidgetName);
    }
    FailureWidget = nullptr;
}

void UETFailureFlowComponent::RestoreOwnedInput()
{
    const bool bReleasePause = bOwnsPause;
    bOwnsPause = false;
    APlayerController* Controller = ControllerOwner.Get();
    UWorld* World = GetWorld();
    if (!bReleasePause || bEndingPlay || !IsValid(World) || World->bIsTearingDown ||
        !IsValid(Controller) || !UIContext.IsValid() ||
        UIContext->GetObservedController() != Controller ||
        (Session.IsValid() && Session->IsTravelPending()))
    {
        return;
    }
    UGameplayStatics::SetGamePaused(this, false);
    FInputModeGameOnly GameInput;
    GameInput.SetConsumeCaptureMouseDown(false);
    Controller->SetInputMode(GameInput);
    Controller->bShowMouseCursor = false;
    Controller->FlushPressedKeys();
}

void UETFailureFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    CancelPresentation();
    if (UIContext.IsValid())
    {
        UIContext->OnObservedPawnChanged.RemoveDynamic(
            this, &ThisClass::HandleObservedPawnChanged);
    }
    if (Session.IsValid())
    {
        Session->OnTravelFailed.Remove(SessionFailureHandle);
    }
    SessionFailureHandle.Reset();
    StopObservingLife();
    RemoveFailureWidget();
    bOwnsPause = false;
    bCommandPending = false;
    UIContext.Reset();
    Session.Reset();
    ControllerOwner.Reset();
    UE_LOG(LogETFailureFlow, Display,
        TEXT("[M1-06B] Failure flow end Owner=%s"), *GetNameSafe(GetOwner()));
    // Do not apply input or pause changes to an ending world.
    Super::EndPlay(EndPlayReason);
}
```

没有常驻Tick、属性Binding或反复扫描角色。一次NextTick呈现让死亡委托 / HUD属性事件和Controller初始Game Only先完成，避免初始死亡快照刚显示UI又被BeginPlay夺回输入；它不是0.2秒之类的猜测Delay，旧Pawn / EndPlay会取消该一次任务。菜单 / 失败按钮只在松开时执行，下一世界仍使用A已验收的Flush初始化。

## 6. CombatLife：在原死亡封锁之后发通知

**只做下列三处追加；现有死亡代码、Tag所有权与Health观察全部保留。**不在Health刚归零时直接Pause / OpenLevel，不把失败UI写入核心组件。

### 6.1 .h声明原生委托与实例事件

在`ETCombatLifeComponent.h`的前向声明区增加下面两行；class与委托放在`UCLASS`之前，generated include仍是最后一个include：

```cpp
class UETCombatLifeComponent;
DECLARE_MULTICAST_DELEGATE_OneParam(
    FETCombatDeathSealed, UETCombatLifeComponent*);
```

在类的`public:`中、既有`IsDead()`之后追加：

```cpp
    FETCombatDeathSealed OnDeathSealed;
```

这是原生事件，不加UPROPERTY / BlueprintAssignable，不新建死亡Blueprint事件。UI持有FDelegateHandle订阅；DeathSealed已完成才通知。

### 6.2 .cpp CommitDeath末尾追加一次广播

找到`CommitDeath`函数末尾现有`UE_LOG`：文本包含`[M1-04C2] Death sealed`，最后一项是`IsValid(Mesh) && Mesh->bPauseAnims ? 1 : 0);`。在该日志完整语句的**分号之后、CommitDeath最后一个右花括号之前**只加一行：

```cpp
    OnDeathSealed.Broadcast(this);
```

不要加到`Death committed`日志后、动画冻结前、HealthChanged或EndPlay；原bDead保护保证同一身体仅一次。通知的参数是**当前Life组件this**，UI只接受自己已绑定的同一实例，不从敌人死亡推断玩家失败。

### 6.3 .cpp EndPlay开头清空事件

在现有`EndPlay`内，紧接`bEndingPlay = true;`之后追加：

```cpp
    OnDeathSealed.Clear();
```

其余解绑Ready / Health、取消受击、释放自己Dead Tag、清弱引用和Super顺序原样保留。通知清理不是重置bDead / 复活；新生命来自下一地图的新身体。

## 7. 两处GameMode接入

### 7.1 菜单GameMode：关闭A已定位的空HUD生成路径

在`ETMenuGameMode.h`的`protected:`中，紧邻既有HandleStartingNewPlayer声明追加：

```cpp
    virtual void InitializeHUDForPlayer_Implementation(
        APlayerController* NewPlayer) override;
```

在`ETMenuGameMode.cpp`构造函数之后、已有HandleStartingNewPlayer函数之前加入：

```cpp
void AETMenuGameMode::InitializeHUDForPlayer_Implementation(
    APlayerController* NewPlayer)
{
    // The menu world intentionally has no AHUD actor.
    (void)NewPlayer;
}
```

若你已按A第11.1节手动加过，核对一致即可，**不要重复定义**。没有Super调用，保持原DefaultPawnClass / SpectatorClass / HUDClass=null和菜单HandleStartingNewPlayer不RestartPlayer。正式游戏GameMode的HUD路径不改。

### 7.2 正式GameMode登记当前地图

在`ETGameMode.cpp`顶部现有include区追加：

```cpp
#include "Engine/GameInstance.h"
#include "Flow/ETRunSessionSubsystem.h"
```

在既有`AETGameMode::StartPlay()`中，紧接`Super::StartPlay();`之后、原Rules ready日志之前追加：

```cpp
    if (UGameInstance* Instance = GetGameInstance())
    {
        if (UETRunSessionSubsystem* RunSession =
            Instance->GetSubsystem<UETRunSessionSubsystem>())
        {
            RunSession->NotifyGameplayWorldReady(GetWorld());
        }
    }
```

构造函数、原Rules日志、GameMode头文件与Blueprint父类不变。直接从正式地图PIE也会登记可重试地图；菜单GameMode**不调用NotifyGameplayWorldReady**，不能把菜单误登记为试炼地图。新游戏请求的地图在PostLoad确认后才提交到会话。

## 8. 主菜单Controller改走同一会话服务

当前菜单Controller实现见本节Source入口；统一请求门控 / UIOnly焦点已整理，会话失败筛选与主动解绑顺序保持。原B阶段未接Continue，当前已接C的会话继续；不要重放旧文件覆盖现状。

为避免两个对象分别持有引擎TravelFailure回调，原B将请求统一交给Session；完整类以当前Source为准。WBP_ETMainMenu、ETMainMenuWidget、菜单Controller BP父类 / MainMenuWidgetClass / GameplayMap配置保持原样。本批不启用Continue。

### 8.1 ETMainMenuController.h全文

当前源码：[ETMainMenuController.h](../EmberTrial/Source/EmberTrialUI/Public/FrontEnd/ETMainMenuController.h)。

### 8.2 ETMainMenuController.cpp全文

当前源码：[ETMainMenuController.cpp](../EmberTrial/Source/EmberTrialUI/Private/FrontEnd/ETMainMenuController.cpp)。

不保留旧`HandleTravelFailure`声明 / 定义 / 引擎TravelFailureHandle，全部由Session管理。BeginPlay绑定Session原生事件，EndPlay主动Remove；Controller / Widget仍属于当前菜单世界，不跨地图缓存它们。

## 9. 正常Build与CLion索引（你执行）

1. 六新 / 七旧共13个文件全部Save，UE保持关闭。在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

2. Editor Build成功后CLion Reload CMake Project。当前29个cpp加Session / FailureWidget / FailureFlow三个，预计**32工程cpp / 65份Source**；实际以编译数据库核对。两个CodeModel都保留，Core头`Flow/ETRunSessionSubsystem.h`与UI头`Flow/ETFailureFlowComponent.h`各属于自己的模块。
3. 再正常Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

4. 两目标成功才打开UE。CLion从Session cpp导航`Subsystems/GameInstanceSubsystem.h`、`UObject/UObjectGlobals.h`；从FailureFlow cpp导航Core `Combat/ETCombatLifeComponent.h`、TimerManager.h与Widget头。不要为索引另设IDE原生编译器替代UBT。
5. UHT / C++报错请给**第一条真实错误 + 文件 / 行号 + 前后10–20行**。不要只给exit6、不要删除缓存试错。所有新文件名字 / include按第2节，API宏 / generated include正确；DeathSealed是native，不误加UFUNCTION / UPROPERTY。

当前助手仅核对引擎API / 源码与静态路径，没有执行本批构建、自动测试或PIE。新的编译 / 运行结果待你反馈，Cook / 独立包仍留M1-07。

## 10. UE资产装配与最小蓝图改动（你执行）

### 10.1 新建WBP_ETFailure并完成Designer

1. 双目标Build成功后打开UE，在Content Drawer进入`/Game/EmberTrial/ui`；没有Flow文件夹就新建**Flow**。
2. 右键空白 → User Interface → Widget Blueprint，新建`WBP_ETFailure`。打开 → File / Class Settings，Parent Class选原生**ETFailureWidget**（C++名UETFailureWidget），不是ETHealthHUDWidget / ETMainMenuWidget。若创建对话框允许直接选父类，可直接选ETFailureWidget。先搭齐下面控件再做最终Compile。
3. Designer中保留一个Canvas Panel根，搭建此层级。粗体三项名称 / 类型用于BindWidget，必须精确，Button内部文字不设变量：

```text
Canvas_Root（Canvas Panel）
└─ Border_Background（Border）
   └─ VBox_Failure（Vertical Box）
      ├─ Text_Title（Text Block）：试炼失败
      ├─ Text_Subtitle（Text Block）：重新开始本次试炼
      ├─ RetryButton（Button）
      │  └─ Text_Retry（Text Block）：重新试炼
      ├─ MainMenuButton（Button）
      │  └─ Text_MainMenu（Text Block）：返回主菜单
      └─ StatusText（Text Block）：重新试炼将从入口开始。
```

| 必须名 | 控件类型 | Is Variable |
| --- | --- | --- |
| **RetryButton** | Button | True |
| **MainMenuButton** | Button | True |
| **StatusText** | Text Block | True |

4. 选Border_Background → Details → **Slot(Canvas Panel Slot)**：Anchors Min=(0,0)、Max=(1,1)，先设置拉满锚点；再将Offsets的Left / Top / Right / Bottom**全部0**。不要只改锚点保留原右 / 下大偏移。Visibility=Visible；Brush Color可设RGBA=(0.02,0.02,0.02,0.90)，形成覆盖整个屏幕的可见背景。
5. 选VBox_Failure → **Slot(Border Slot)**：Horizontal Alignment=Center、Vertical Alignment=Center、Padding=48 / 48 / 48 / 48。名称不是C++绑定项，但整列要居中。
6. 选Text_Title：Content → Text=`试炼失败`，Font Size=40、Justification=Center；Slot(Vertical Box Slot)Size=Auto、HAlignment=Fill。Text_Subtitle文字=`重新开始本次试炼`、18号、Center；同一Slot Size=Auto / H=Fill，Padding上下各10。
7. **两个Button分别设置**：Is Variable=True、Is Focusable=True、Is Enabled=True、Visibility=Visible；Interaction中Click Method=`Down And Up`、Press Method=`Down And Up`。选中Button本身的**Slot(Vertical Box Slot)**：Size=Auto、Horizontal Alignment=Fill、Padding Left=0 / Top=8 / Right=0 / Bottom=8。不要设Fill高度。
8. **Button内两份Text分别设置**：24号、Justification=Center，Content → Text填写对应中文；它们自己的**Slot(Button Slot)**为HCenter / VCenter，Padding Left=60 / Top=14 / Right=60 / Bottom=14。内容内边距在内层Text的Slot，不能在外层Button搜索“Content Padding”。
9. 选StatusText：Is Variable=True，Content文字`重新试炼将从入口开始。`，16号、Center、Auto Wrap Text=True、Wrap Text At=480；Slot(Vertical Box Slot)Size=Auto / H=Fill，Padding=0 / 20 / 0 / 0。Text右侧**不创建Binding**。
10. Class Defaults：**Is Focusable=True，Tick Frequency=Never**。Graph不接Event Tick / Construct / Button OnClicked，保留自动生成的未连事件即可；原生已绑定按钮。不要新建蓝图Dispatcher、不要Cast角色 / 写OpenLevel / SetGamePaused。
11. Compile → 看最后Compiler Results应无BindWidget缺失或Native Tick警告 → Save，确认页签无星号。若警告，按上面三控件的名称、类型、变量及父类查，不把BindWidget改Optional隐藏问题。

### 10.2 正式Controller只挂一份FailureFlow

1. 打开正式`/Game/EmberTrial/Blueprints/Framework/BP_ETPlayerController`（父类仍ETPlayerController），左侧Components → Add，搜索原生**ETFailureFlowComponent** / ET Failure Flow，添加后命名**FailureFlow**。
2. Components列表应是**一个原UIBootstrap + 一个FailureFlow**；不删UIBootstrap，不挂到Pawn / 敌人 / 菜单Controller，若出现两个FailureFlow删除多余那份。
3. 选FailureFlow → Details → EmberTrial / Failure：
   - Failure Widget Class = **WBP_ETFailure**。
   - Main Menu Map = **L_ET_MainMenu**，完整资产路径`/Game/EmberTrial/Maps/L_ET_MainMenu`；不是正式战斗地图，最终不能None。
   - Auto Activate=True，Tick不新增 / 不启用。
4. 原UIBootstrap两份Widget Class保持WBP_ETLifecycleStatus / WBP_ETHealthHUD。Controller Graph不增加任何输入、函数或切图节点；Compile → Save。
5. 打开既有BP_ETMainMenuController，核对原MainMenuWidgetClass=WBP_ETMainMenu、GameplayMap=L_ET_VerticalSlice；**不添加FailureFlow / UIBootstrap**，Compile → Save。没有实际改动时不强行保存制造差异，父类新反射读取正确即可。

### 10.3 旧P菜单新增死亡拒绝（完整连线）

位置是**BP_ThirdPersonCharacter → My Blueprint → Functions → OpenPauseMenu**，不是EventGraph的P输入事件，也不是WBP_PauseMenu。只在函数入口前端加一项，原关闭 / 继续 / EndPlay链路不改。

1. 找到OpenPauseMenu紫色入口节点；它的白色执行输出原来连第一个前置检查Branch（通常Condition=bPauseMenuOpen）。记住这个原节点，断开**入口→该Branch这一条白线**。
2. 新建一个Branch。入口白色执行输出 → **新Branch白色输入**。
3. 右键搜索**Is Combat Dead**（原生EmberTrialCharacter只读查询）。该纯节点的**Target=Self**；如果没有Target引脚，表示它就是当前Self版本。它的Return Value → **新Branch.Condition**。
4. **新Branch.True（已死亡）** → Return Node；若无输出的普通函数未显示Return Node，可把True保持无白线，表示结束。不要把True接旧逻辑。
5. **新Branch.False（仍存活）** → 第1步原先的第一个Branch白色输入，后面PauseWidget有效性、PC0、SetGamePaused、Focus与日志全部保持原样。
6. 纯Is Combat Dead没有白线，**不添加Is Valid**：Self是本函数所属角色，原生查询内部处理Life组件有效性。不能把Target设Controller / FailureFlow，也不能用IsFormalActionIdle替代（活着攻击中仍允许原P暂停）。
7. Compile → Save。键盘P.Completed原打开 / 关闭路线、WBP_PauseMenu的继续按钮 / OnKeyUp.P、角色bPauseMenuOpen、PauseWidget和原E / 1–4都保留。

```text
OpenPauseMenu入口
  → 新Branch（Condition = IsCombatDead，Target=Self）
      True  → Return / 无白线
      False → 原第一个bPauseMenuOpen Branch → 原有效性 / 暂停 / UI逻辑
```

死亡页采用UI Only；死亡时P不会打开旧Pause页。原ClosePauseMenu仍可正确结束已经存在的旧暂停，统一迁移到Controller的业务留D做。本批不更改活着时P的动作门控，不搭临时按键 / 测试函数。

### 10.4 最终配置快速核对

- 正式地图Override仍BP_ETGameMode、Default Pawn仍BP_ThirdPersonCharacter / 正式Controller仍BP_ETPlayerController，敌人 / 导航保持原配置。
- 玩家 / 敌人默认100/100，GA玩家25 / 敌人10，4仍Apply GE_ET_Test_Damage25；slomo1、Draw Debug=False。
- Game Default Map=L_ET_MainMenu，Editor Startup Map=L_ET_VerticalSlice，两张地图已在Cook列表；本批没有新地图，不再添加重复Cook行。
- Save All，进入第11节。不要改生命默认值为0、临时改敌人伤害或新摆测试Pawn。

## 11. 分层验收：三组，合计约10–15分钟

你只需要观察结果、记录大致完成时分。我读取日志核对生命、请求次数、旧源清理与新世界，不要求你抄长日志或统计对象名称。所有检查均用正式按钮 / 既有伤害入口，没有秒级抢时序操作。

### 11.1 正常开始、非玩家死亡隔离、真实死亡与鼠标重试（约4–5分钟）

**准备：**最终100/100、4伤害25、FailureFlow / Map配置正常。打开L_ET_MainMenu，Play。

**按顺序操作：**

1. 菜单仍无玩家身体 / 战斗HUD，Continue禁用，开始 / 退出正常；控制台不应再有菜单空HUD的`SpawnActor failed because no class was specified`。
2. 鼠标点击开始一次，进入正式地图；站定后左键可用，正常100/100、单HUD，移动 / 转向 / 跳跃正常。
3. 靠近训练目标，用真实左键四次击败它，玩家保持存活；**训练目标死亡不显示玩家失败页**。没有必要再打第二个目标或重跑阵营矩阵。
4. 按现有4入口扣自己的生命，间隔约半秒，最多四次即可归零；若敌人期间已扣血，可能少于四次，按实际为准，不要求伪造100→75→50→25。
5. 死亡应冻结原身体并只出现一份失败页，鼠标可见 / 重试默认焦点，背景世界暂停。按P、左键 / 2 / E、移动键各一次，不应打开旧P菜单或执行游戏动作。HUD0的数据由我读日志，页覆盖时不用你额外隐藏View查看。
6. 鼠标点击**重新试炼**一次，可快速再点一次观察没有第二次切图；新房间应回现有入口，玩家 / 敌人 / 训练目标重新100/100，死亡姿势消失、旧失败页消失、鼠标隐藏。无需额外点击视口，W / 转向 / 跳跃 / 首次左键可用。
7. 活着时P打开旧暂停，再用鼠标继续一次，确认未被新死亡门控误拦。

**画面 / 日志预期：**

```text
[M1-06B] Session ready ...
[M1-06B] Travel requested Intent=NewRun ... Paused=0
[M1-06B] Gameplay world ready ...
[M1-06B] Travel arrived Intent=NewRun ... Expected=1 Pending=0
...当前玩家 Death committed / Death sealed（原M1-04C2日志）...
[M1-06B] Failure queued ...当前玩家...
[M1-06B] Failure shown ... Paused=1
[M1-06B] Travel requested Intent=Retry ... Paused=0
...旧Life unbound / Failure removed / Failure flow end，旧HUD与AI结束...
...新身体Health initialized100/100、HUD Ready100...
[M1-06B] Travel arrived Intent=Retry ... Expected=1 Pending=0
```

上面是关键节点，不规定旧UI / AI EndPlay之间唯一总顺序；要求均结束且新View只观察新源。训练目标的Death sealed没有对应Failure shown。

**清理：**本组不改任何资产配置；保持本轮新身体存活，可直接继续11.2。不要为了“恢复训练目标”手动改地图，重试已恢复它。

### 11.2 敌人真实致死、键盘重试、返回菜单与下一轮（约3–5分钟）

**准备：**上一组重试后新身体100/100、世界未暂停，正式敌人仍有原自动交战能力。

**按顺序操作：**

1. 靠近敌人，站在它能实际命中的正前方，不反击，等其真实10伤害归零；保持敌人原参数，不调伤害 / slomo。正常十次命中约十几至几十秒，若空挥则稍靠近，它不是测试失败。
2. 真实死亡显示失败页后，默认重试获得焦点，按**Enter并松开**重试；新身体 / HUD恢复100，未自动补攻击 / 跳跃，Space一次仍正常跳跃。若先用Tab切走焦点，用Shift+Tab / Tab回重试再确认。
3. 用4再次归零，这次失败页按**Space并松开**执行重试；确认不自动跳跃。仅看新身体与第一步输入，不重复训练目标死亡或战斗矩阵。
4. 用4再次归零，Tab切到**返回主菜单**，按Enter并松开。应进入一份主菜单，无战斗Pawn / HUD，Continue仍禁用。
5. 在菜单开始新试炼一次，确认新100/100与基础输入；Stop PIE。
6. 从菜单重新Play一次，Tab到退出、Enter并松开，当前PIE结束。两轮Session应分别ready / end，不把同一GI跨地图持有Session误当泄漏。

**画面 / 日志预期：**真实Enemy→Player伤害造成Death sealed后出现Failure shown；两次键盘Retry各只有一个Travel requested，均Expected1 / Pending0。返回菜单有Intent=MainMenu与旧失败 / HUD清理，Menu ready Pawn=None；再次开始有新NewRun。具体键盘动作 / 无补播由你观察，日志不能单独区分按了Enter还是Space。

**清理：**Stop后回正式地图。默认值 / 4 / AI / Debug未修改，没有临时节点；原P暂停继续功能保留，不要求再做十次开关。

### 11.3 返回菜单None预检、按钮恢复与最终清理（约2–3分钟）

**准备：**Stop PIE，打开BP_ETPlayerController → Components选FailureFlow，把**Main Menu Map临时设None**，Compile → Save。不要改BP_ETMainMenuController的Gameplay Map，不删地图文件。

**按顺序操作：**

1. 直接在L_ET_VerticalSlice Play；这验证GameMode登记当前地图，不要求你再经过菜单。用4归零，看到失败页。
2. 只点击返回主菜单一次。应保留失败页、显示“目标地图不可用，请检查地图配置。”，按钮恢复可用；世界仍暂停，不能无界反复请求。
3. 点击重新试炼一次，应正常重载并恢复100/100。确认一次移动 / 镜头；Stop。
4. **恢复FailureFlow.Main Menu Map=L_ET_MainMenu，Compile → Save**。直接正式地图Play、4归零、点击返回主菜单一次，确认正常菜单且可开始；开始后只做移动 / P键盘继续 / E提示短基线，Stop。
5. 最后直接正式地图Play，用4归零，**失败页显示时用UE工具栏Stop停止PIE**；再Play只确认新身体100/100、没有旧失败页，然后Stop。检查暂停状态结束清理，无需再重试或重复战斗输入。
6. Save All，编辑器回正式地图；确认最终Main Menu Map正确、100/100、4伤害25、slomo1 / Debug False。组件和Widget保留，没有临时Graph要删除。

**画面 / 日志预期：**None返回有一条`Travel rejected` Warning和`Failure recovered`；没有该次MainMenu Travel requested，重试仍能成功。最终恢复后MainMenu到达Expected1，不把负例Warning当正式失败，也**不把None预检当真正TravelFailure实机测试**。

**清理：**临时None已恢复并正常保存。成功后不重跑前两组，完成第12节反馈。

## 12. 完成反馈（复制填写；我负责读日志）

```text
M1-06B 完成
六新 / 七旧Source；Editor Build / 索引32cpp / Game Build：
DeathSealed广播位置 / 原死亡清理保留 / 菜单空HUD Warning消失：
WBP父类 / 三BindWidget / 无Binding / Never / Compile：
单UIBootstrap + 单FailureFlow / 两Class与Map / 旧P死亡Branch：
11.1 非玩家死亡隔离 / 玩家失败一次 / 鼠标重试 / 新100与输入：
11.2 敌人真实致死 / Enter与Space重试 / 无补播：
11.2 返回菜单隔离 / 再开始 / 两轮PIE与退出清理：
11.3 直接地图登记 / None拒绝可重试 / 恢复后正常返回：
11.3 失败页中Stop / 重开100与无旧页：
最后E / P键盘继续 / 移动转向跳跃短基线：
Main Menu Map恢复、100/100、4=Damage25、slomo1、Debug False / Save All：
失败或特别说明：
大致完成时间（完整时分，例如21:18）：
实际用时：代码__ / 资产__ / 检查__；构建等待__分钟
```

不用手抄日志 / 哈希或统计实例；我核对源码、receipt / 实际32cpp索引、菜单开始 / 两种死亡来源、请求 / 到达、旧Life / View / HUD / AI结束与新快照。未保留的具体操作按你的明确反馈登记，只有证据矛盾才窄补。**2026-10-11 15:05本批反馈已验收**，见[B报告](Reports/M1_06B_Verification.md)：三次Retry / 四次MainMenu到达，七页显示 / 移除与十一组Life / Flow / HUD清理闭合；None预检后配置恢复，菜单空HUD Warning关闭。新Widget与两旧BP实际哈希已登记，[来源清单](AssetManifest.csv)44→45，无需重复第11节。

## 13. 静态核对与常见问题

本机5.5.4已核对GameInstanceSubsystem所在头为`Runtime/Engine/Public/Subsystems/GameInstanceSubsystem.h`、Initialize / Deinitialize、UWorld.RemovePIEPrefix、HasBegunPlay / bIsTearingDown、FCoreUObjectDelegates.PostLoadMapWithWorld、Engine.OnTravelFailure、TimerManager.SetTimerForNextTick / ClearTimer与既有UI Only / Button / Flush API。引擎LoadMap在World.BeginPlay后广播PostLoadMapWithWorld，失败可广播nullptr，本实现不会把无所属World的全局广播归到其他PIE会话。

上述API核对属于下发时的静态依据，独立于后续实测。**本批双目标构建 / 运行现已验收通过**，依据和边界见[B报告](Reports/M1_06B_Verification.md)，未把None预检称为真正TravelFailure。没有给Core引入UMG依赖，没有增加常驻Tick、通用大型流程框架 / 测试台或复制角色死亡规则。失败页是当前世界View，RunSession是当前进程的地图状态，C再加稳定检查点ID，D再迁原暂停业务。

| 现象 | 优先检查 / 给我的材料 |
| --- | --- |
| Build exit6 | 第一条真实UHT / C++错误与上下文；新文件目录 / include / API宏，DeathSealed是否按第6节声明 |
| 玩家死亡没失败页 | FailureFlow是否只在正式Controller、FailureWidgetClass是否WBP_ETFailure；Death sealed→Failure queued / shown的文字日志。当前World若原P已暂停，先用原继续按钮恢复，不改死亡规则 |
| 敌人 / 目标死也弹页 | 完整操作与Life bound.Body / Failure shown.Body日志；不应在NPC上挂FailureFlow或绑定全局伤害事件 |
| 重试提示未登记地图 | 正式地图Override / GameMode父类、ETGameMode.StartPlay追加与Gameplay world ready日志；不临时伪造会话变量 |
| 页面能显示但按钮没效果 | Widget父类、三名 / 类型 / Is Variable、Button按下松开设置；Travel rejected / requested原文，Main Menu Map实际路径 |
| 切图后鼠标 / 卡键 / 旧页 | 请求时Paused0、游戏Game input ready、旧Life unbound / Failure removed / HUD清理日志；给操作顺序与短视频，不重搭旧换Pawn测试图 |
| HUD显示0被认为未就绪 | 死亡仍Ready0；不要让Presenter Waiting / None替代死亡结果，也不要移除Dead Tag治疗旧身体 |

报错用**文字日志优先**；Designer / 蓝图连线用截图，输入时序 / 动画用短视频。源文件可由我直接读取，你只要说明在哪节、预期 / 实际、是否已恢复配置。继续默认手动改C++；明确说“你直接操作”后才接管。

## 14. 本批需要Commit / Push的内容（固定放在文档最后）

### 14.1 源码：六新 / 七旧

选择第2节全部13份实际变化Source：Session / FailureWidget / FailureFlow各头与cpp；CombatLife头 / cpp、MenuGameMode头 / cpp、ETGameMode.cpp、MainMenuController头 / cpp。Build.cs / Target / .uproject / ETPlayerController.cpp / 旧Bootstrap本批无必需改动，不提交生成的UHT / Unity cpp或索引文件。

推荐Commit标题：`feat: add death screen and session-based level retry`。**验收时曾发现六个新文件为AM：暂存区只有类骨架，完整代码在工作区；后续已全部提交4b83b66，不再重复暂存。**以下14.1.1保留为日后“Stage后又修改”的简单操作；提交前看Staged Diff确认完整实现，避免只提交早期暂存内容。

#### 14.1.1 CLion重新Stage最新修改：简单步骤

重新Stage就是把已保存的最新代码更新到暂存区；直接添加最新修改即可，不需要先取消旧暂存。

1. 在CLion按 **Ctrl+S** 保存代码，再按 **Ctrl+K** 打开Commit提交窗口。
2. 如果窗口没有 **Staged（已暂存）/ Unstaged（未暂存）** 两组：按 **Ctrl+Alt+S** → **Version Control → Git** → 勾选 **Enable staging area（启用暂存区）** → Apply / OK，再回到Commit窗口。
3. 展开 **Unstaged**，按住Ctrl选中下面六个文件的**最新修改**，按 **Ctrl+Alt+A** 添加到暂存区；也可以逐个点击文件右侧的 **＋ / Add** 按钮。文件同时出现在Staged和Unstaged时，本步选择Unstaged中的那一份。

   | 文件组 | 本次需要重新Stage的文件 |
   | --- | --- |
   | Session | `ETRunSessionSubsystem.h`、`ETRunSessionSubsystem.cpp` |
   | 失败View | `ETFailureWidget.h`、`ETFailureWidget.cpp` |
   | 失败流程 | `ETFailureFlowComponent.h`、`ETFailureFlowComponent.cpp` |

4. 确认这六个文件已在 **Staged** 中，并且 **Unstaged不再列出它们的修改**。这表示暂存区已经跟上最新代码；之后若又改动这些文件，需要再Stage一次。
5. 在 **Staged** 选中`ETRunSessionSubsystem.cpp`，按 **Ctrl+D** 查看差异，确认新增内容包含完整函数实现，例如`Initialize`、`BeginTravel`，而不是只有最初的include骨架。
6. 按第14.1–14.3节清单，把本批其他源码、资产和文档也Stage进去，然后填写上面的Commit标题，**Commit → Push**。

界面入口与快捷键依据[JetBrains CLion官方说明](https://www.jetbrains.com/help/clion/commit-and-push-changes.html#use-the-git-staging-area-to-commit-changes)核对；中文界面可对应查看括号中的名称。本步骤由你操作，我没有替你暂存或提交。

### 14.2 可编辑资产

| 内容 | 仓库相对路径 |
| --- | --- |
| 新失败Widget | `EmberTrial/Content/EmberTrial/ui/Flow/WBP_ETFailure.uasset` |
| 组件装配 | `EmberTrial/Content/EmberTrial/Blueprints/Framework/BP_ETPlayerController.uasset` |
| 旧P死亡门控 | `EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset` |

BP_ETMainMenuController只有确有父类刷新 / 必要配置保存变化才加入；本批不修改菜单Widget、地图、启动 / Cook设置或旧HUD。三个必需资产继续走LFS；若实际出现其他资产差异，保留并说明，不盲目丢弃或强行全选。

### 14.3 文档与已有待提交项

- `Docs/M1_06B_FailureAndRetry.md`、`Docs/M1_06_FlowRoadmap.md`。
- 本批更新的`Docs/Progress.md`、`Docs/Backlog.md`、`Docs/Architecture.md`、`Docs/VersionControl.md`、`Docs/AcceptanceWorkflow_Options.md`、`Introduction.md`。
- 已保存的`Docs/Reports/M1_06B_Verification.md`和`Docs/AssetManifest.csv`（一新Widget / 两旧BP实际哈希，清单45项）。
- 原个人设置`EmberTrial/Config/DefaultEditorPerProjectUserSettings.ini`取消跟踪删除已进入4b83b66，磁盘个人设置保留，**不再执行删除磁盘文件或重复git rm**。

### 14.4 留本地与最后操作

Raw核查 / 日志、Artifacts包 / 视频、Binaries / Intermediate / Saved / DDC、`.idea` / `.run`、生成CMake / compile_commands与索引留本地，不Force Add绕过忽略规则。

B功能 / 实现已进入4b83b66，B报告随3351b53已跟踪，不按旧AM提示重复提交Source / 资产。本轮C已验收，只追加C验收 / 来源清单 / 状态文档，按[C第13节](M1_06C_SessionCheckpoints.md#13-本批需要commit--push的内容固定放在文档最后)选择实际变化，Stage → Commit → Push已有origin/main。助手不代Stage / Commit / Push；上传失败给原始错误文本，我再定位。
