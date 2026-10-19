#include "Flow/ETRunSessionSubsystem.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRunSession, Log, All);

// ---- 会话生命周期；不是世界Actor，切图后仍保留 ----
void UETRunSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bShuttingDown = false;
    ResetPendingTravel();
    GameplayPackage.Reset();
    ActiveCheckpointId = NAME_None;
    PostLoadHandle =
        FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
    if (GEngine)
    {
        TravelFailureHandle =
            GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
    }
    UE_LOG(LogETRunSession, Display, TEXT("[M1-06C] Session ready Session=%s"), *GetNameSafe(this));
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
    OnCheckpointChanged.Clear();
    GameplayPackage.Reset();
    ActiveCheckpointId = NAME_None;
    UE_LOG(LogETRunSession, Display, TEXT("[M1-06C] Session end Session=%s"), *GetNameSafe(this));
    Super::Deinitialize();
}

// ---- 去除PIE前缀，使用稳定包名匹配地图 ----
FString UETRunSessionSubsystem::GetWorldPackage(UWorld* World)
{
    return IsValid(World) ? UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) : FString();
}

bool UETRunSessionSubsystem::IsGameplayIntent(EETRunTravelIntent Intent)
{
    return Intent == EETRunTravelIntent::NewRun || Intent == EETRunTravelIntent::Retry ||
           Intent == EETRunTravelIntent::Continue;
}

const TCHAR* UETRunSessionSubsystem::GetIntentName(EETRunTravelIntent Intent)
{
    switch (Intent)
    {
    case EETRunTravelIntent::NewRun:
        return TEXT("NewRun");
    case EETRunTravelIntent::Retry:
        return TEXT("Retry");
    case EETRunTravelIntent::Continue:
        return TEXT("Continue");
    case EETRunTravelIntent::MainMenu:
        return TEXT("MainMenu");
    default:
        return TEXT("None");
    }
}

// ---- 出生选择读取待定ID；菜单继续资格读取已确认快照 ----
FName UETRunSessionSubsystem::GetSpawnCheckpointId(UWorld* SpawnWorld) const
{
    if (bShuttingDown || !IsValid(SpawnWorld) || SpawnWorld->GetGameInstance() != GetGameInstance())
    {
        return GetEntryCheckpointId();
    }
    const FString Package = GetWorldPackage(SpawnWorld);
    if (bTravelPending && IsGameplayIntent(PendingIntent) && Package == PendingTargetPackage &&
        !PendingCheckpointId.IsNone())
    {
        return PendingCheckpointId;
    }
    if (Package == GameplayPackage && !ActiveCheckpointId.IsNone())
    {
        return ActiveCheckpointId;
    }
    return GetEntryCheckpointId();
}

// GameMode报告实际选择的出生ID；缺失ID回退后也必须保存实际值。
void UETRunSessionSubsystem::NotifyGameplayWorldReady(UWorld* GameplayWorld, FName SpawnId)
{
    if (bShuttingDown || !IsValid(GameplayWorld) || SpawnId.IsNone() ||
        GameplayWorld->GetGameInstance() != GetGameInstance() ||
        GameplayWorld->GetNetMode() != NM_Standalone)
    {
        return;
    }
    const FString Package = GetWorldPackage(GameplayWorld);
    if (bTravelPending)
    {
        if (IsGameplayIntent(PendingIntent) && Package == PendingTargetPackage)
        {
            PendingCheckpointId = SpawnId;
            bPendingGameplayReady = true;
        }
    }
    else if (FPackageName::IsValidLongPackageName(Package) &&
             FPackageName::DoesPackageExist(Package))
    {
        GameplayPackage = Package;
        ActiveCheckpointId = SpawnId;
    }
    UE_LOG(LogETRunSession, Display,
           TEXT("[M1-06C] Gameplay world ready Map=%s Spawn=%s Pending=%d"), *Package,
           *SpawnId.ToString(), bTravelPending ? 1 : 0);
}

bool UETRunSessionSubsystem::HasContinueSnapshot() const
{
    return !bShuttingDown && !ActiveCheckpointId.IsNone() &&
           FPackageName::IsValidLongPackageName(GameplayPackage) &&
           FPackageName::DoesPackageExist(GameplayPackage);
}

bool UETRunSessionSubsystem::IsCheckpointActive(UWorld* CheckWorld, FName CheckpointId) const
{
    return !bShuttingDown && !bTravelPending && IsValid(CheckWorld) &&
           CheckWorld->GetGameInstance() == GetGameInstance() &&
           GetWorldPackage(CheckWorld) == GameplayPackage && !CheckpointId.IsNone() &&
           ActiveCheckpointId == CheckpointId;
}

// ---- 激活要求本GI / 同世界 / 本地受控存活身体，且ID唯一 ----
bool UETRunSessionSubsystem::TryActivateCheckpoint(UWorld* RequestWorld,
                                                   APlayerStart* CheckpointStart,
                                                   AEmberTrialCharacter* PlayerBody)
{
    if (bShuttingDown || bTravelPending || !IsValid(RequestWorld) || RequestWorld->bIsTearingDown ||
        !RequestWorld->HasBegunPlay() || RequestWorld->GetGameInstance() != GetGameInstance() ||
        RequestWorld->GetNetMode() != NM_Standalone ||
        UGameplayStatics::IsGamePaused(RequestWorld) ||
        GetWorldPackage(RequestWorld) != GameplayPackage || !IsValid(CheckpointStart) ||
        CheckpointStart->GetWorld() != RequestWorld || !IsValid(PlayerBody) ||
        PlayerBody->GetWorld() != RequestWorld || !PlayerBody->IsGameplayReady() ||
        PlayerBody->IsCombatDead())
    {
        return false;
    }
    APlayerController* PlayerController = Cast<APlayerController>(PlayerBody->GetController());
    if (!IsValid(PlayerController) || !PlayerController->IsLocalController() ||
        PlayerController->GetPawn() != PlayerBody)
    {
        return false;
    }
    const FName CheckpointId = CheckpointStart->PlayerStartTag;
    if (CheckpointId.IsNone() || CheckpointId == GetEntryCheckpointId())
    {
        return false;
    }
    int32 Matches = 0;
    for (TActorIterator<APlayerStart> It(RequestWorld); It; ++It)
    {
        if (!It->bIsPIEPlayerStart && It->PlayerStartTag == CheckpointId)
        {
            ++Matches;
        }
    }
    if (Matches != 1 || ActiveCheckpointId == CheckpointId)
    {
        return false;
    }
    // 先提交再广播，观察者同步读取时能看到新ID。
    ActiveCheckpointId = CheckpointId;
    UE_LOG(LogETRunSession, Display, TEXT("[M1-06C] Checkpoint activated Map=%s Id=%s Body=%s"),
           *GameplayPackage, *CheckpointId.ToString(), *GetNameSafe(PlayerBody));
    OnCheckpointChanged.Broadcast(RequestWorld, CheckpointId);
    return true;
}

// ---- 命令入口保留各自权限：Retry需同地图，Continue可从菜单发起 ----
bool UETRunSessionSubsystem::RequestNewRun(UWorld* RequestWorld, const FSoftObjectPath& Map,
                                           FText& OutError)
{
    return BeginTravel(RequestWorld, Map.GetLongPackageName(), GetEntryCheckpointId(),
                       EETRunTravelIntent::NewRun, OutError);
}

// 仅编辑器PIE的负例入口；Game目标保留签名但不修改会话。
bool UETRunSessionSubsystem::SetCheckpointIdForDevelopmentTest(UWorld* TestWorld, FName TestId)
{
#if WITH_EDITOR
    if (bShuttingDown || bTravelPending || !IsValid(TestWorld) ||
        TestWorld->WorldType != EWorldType::PIE || TestWorld->bIsTearingDown ||
        TestWorld->GetGameInstance() != GetGameInstance() ||
        GetWorldPackage(TestWorld) != GameplayPackage || !HasContinueSnapshot() || TestId.IsNone())
    {
        return false;
    }
    ActiveCheckpointId = TestId;
    UE_LOG(LogETRunSession, Display, TEXT("[M1-06C] Debug checkpoint override Map=%s Id=%s PIE=1"),
           *GameplayPackage, *TestId.ToString());
    OnCheckpointChanged.Broadcast(TestWorld, TestId);
    return true;
#else
    (void)TestWorld;
    (void)TestId;
    return false;
#endif
}

bool UETRunSessionSubsystem::RequestRetry(UWorld* RequestWorld, FText& OutError)
{
    if (!HasContinueSnapshot() || GetWorldPackage(RequestWorld) != GameplayPackage)
    {
        return RejectRequest(TEXT("当前试炼会话不可用，不能重试。"), OutError);
    }
    return BeginTravel(RequestWorld, GameplayPackage, ActiveCheckpointId, EETRunTravelIntent::Retry,
                       OutError);
}

bool UETRunSessionSubsystem::RequestContinue(UWorld* RequestWorld, FText& OutError)
{
    if (!HasContinueSnapshot())
    {
        return RejectRequest(TEXT("尚无可继续的本次试炼。"), OutError);
    }
    return BeginTravel(RequestWorld, GameplayPackage, ActiveCheckpointId,
                       EETRunTravelIntent::Continue, OutError);
}

bool UETRunSessionSubsystem::RequestMainMenu(UWorld* RequestWorld, const FSoftObjectPath& Map,
                                             FText& OutError)
{
    return BeginTravel(RequestWorld, Map.GetLongPackageName(), NAME_None,
                       EETRunTravelIntent::MainMenu, OutError);
}

bool UETRunSessionSubsystem::RejectRequest(const TCHAR* Reason, FText& OutError)
{
    OutError = FText::FromString(Reason);
    UE_LOG(LogETRunSession, Warning, TEXT("[M1-06C] Travel rejected Reason=%s"), Reason);
    return false;
}

// ---- 全部预检通过后登记Pending，再解除暂停 / 发OpenLevel ----
bool UETRunSessionSubsystem::BeginTravel(UWorld* RequestWorld, const FString& TargetPackage,
                                         FName SpawnId, EETRunTravelIntent Intent, FText& OutError)
{
    OutError = FText::GetEmpty();
    if (bShuttingDown || bTravelPending || !IsValid(RequestWorld) || RequestWorld->bIsTearingDown ||
        !RequestWorld->HasBegunPlay() || RequestWorld->GetGameInstance() != GetGameInstance() ||
        RequestWorld->GetNetMode() != NM_Standalone)
    {
        return RejectRequest(TEXT("当前无法切换地图，请稍后重试。"), OutError);
    }
    if (!FPackageName::IsValidLongPackageName(TargetPackage) ||
        !FPackageName::DoesPackageExist(TargetPackage) ||
        (Intent != EETRunTravelIntent::Retry && TargetPackage == GetWorldPackage(RequestWorld)) ||
        (IsGameplayIntent(Intent) && SpawnId.IsNone()))
    {
        return RejectRequest(TEXT("目标地图或出生标识不可用，请检查配置。"), OutError);
    }
    // 先提交请求身份，避免解除暂停 / 外部回调时重复接受请求。
    bTravelPending = true;
    PendingIntent = Intent;
    PendingTargetPackage = TargetPackage;
    PendingCheckpointId = SpawnId;
    bPendingGameplayReady = false;
    OriginWorld = RequestWorld;
    if (UGameplayStatics::IsGamePaused(RequestWorld) &&
        !UGameplayStatics::SetGamePaused(RequestWorld, false))
    {
        ResetPendingTravel();
        return RejectRequest(TEXT("无法解除世界暂停，未执行切图。"), OutError);
    }
    UE_LOG(LogETRunSession, Display,
           TEXT("[M1-06C] Travel requested Intent=%s From=%s To=%s Checkpoint=%s Paused=%d"),
           GetIntentName(Intent), *GetWorldPackage(RequestWorld), *TargetPackage,
           *SpawnId.ToString(), UGameplayStatics::IsGamePaused(RequestWorld) ? 1 : 0);
    UGameplayStatics::OpenLevel(RequestWorld, FName(*TargetPackage), true);
    return true;
}

// ---- 同GI的到达确认；先提交 / 释放Pending，再允许失败观察者重试 ----
void UETRunSessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (bShuttingDown || !bTravelPending || !IsValid(LoadedWorld) ||
        LoadedWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }
    const FString LoadedPackage = GetWorldPackage(LoadedWorld);
    const EETRunTravelIntent CompletedIntent = PendingIntent;
    const FName CompletedCheckpoint = PendingCheckpointId;
    const bool bExpectedMap = LoadedPackage == PendingTargetPackage;
    // gameplay还需GameMode出生报告；菜单无需该报告，但仍核对目标地图。
    const bool bConfirmed =
        bExpectedMap && (!IsGameplayIntent(CompletedIntent) ||
                         (bPendingGameplayReady && !CompletedCheckpoint.IsNone()));
    if (bConfirmed && IsGameplayIntent(CompletedIntent))
    {
        GameplayPackage = PendingTargetPackage;
        ActiveCheckpointId = CompletedCheckpoint;
        UE_LOG(LogETRunSession, Display,
               TEXT("[M1-06C] Snapshot committed Map=%s Checkpoint=%s Intent=%s"), *GameplayPackage,
               *ActiveCheckpointId.ToString(), GetIntentName(CompletedIntent));
    }
    ResetPendingTravel();
    UE_LOG(LogETRunSession, Display,
           TEXT("[M1-06C] Travel arrived Intent=%s Map=%s Expected=%d Confirmed=%d Pending=0"),
           GetIntentName(CompletedIntent), *LoadedPackage, bExpectedMap ? 1 : 0,
           bConfirmed ? 1 : 0);
    if (!bConfirmed)
    {
        OnTravelFailed.Broadcast(
            LoadedWorld, FText::FromString(TEXT("未确认试炼地图与出生配置，请检查后重试。")));
    }
}

// 失败可能在来源世界，也可能已到本GI的新世界；不放宽为任意全局事件。
void UETRunSessionSubsystem::HandleTravelFailure(UWorld* FailedWorld,
                                                 ETravelFailure::Type FailureType,
                                                 const FString& ErrorString)
{
    if (bShuttingDown || !bTravelPending || !IsValid(FailedWorld) ||
        (FailedWorld != OriginWorld.Get() && FailedWorld->GetGameInstance() != GetGameInstance()))
    {
        return;
    }
    ResetPendingTravel();
    UE_LOG(LogETRunSession, Warning, TEXT("[M1-06C] Travel failed Type=%d Error=%s Pending=0"),
           static_cast<int32>(FailureType), *ErrorString);
    OnTravelFailed.Broadcast(FailedWorld,
                             FText::FromString(TEXT("加载地图失败，请重试或返回主菜单。")));
}

// 清请求不清已确认快照，失败后仍可重试 / 继续。
void UETRunSessionSubsystem::ResetPendingTravel()
{
    bTravelPending = false;
    bPendingGameplayReady = false;
    PendingIntent = EETRunTravelIntent::None;
    PendingTargetPackage.Reset();
    PendingCheckpointId = NAME_None;
    OriginWorld.Reset();
}
