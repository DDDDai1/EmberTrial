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
