#include "Framework/ETGameMode.h"
#include "Framework/ETPlayerController.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETGameMode, Log, All);

// 蓝图负责具体资源类，原生构造不硬编码内容路径。
AETGameMode::AETGameMode()
{
    PlayerControllerClass = AETPlayerController::StaticClass();
    DefaultPawnClass = AEmberTrialCharacter::StaticClass();
}

UETRunSessionSubsystem* AETGameMode::GetRunSession() const
{
    UGameInstance* Instance = GetGameInstance();
    return Instance ? Instance->GetSubsystem<UETRunSessionSubsystem>() : nullptr;
}

// ---- 统一出生选择入口；不让Portal / 旧StartSpot覆盖会话意图 ----
AActor* AETGameMode::FindPlayerStart_Implementation(AController* Player,
                                                    const FString& IncomingName)
{
    // Offline single-player uses the session ID rather than a portal or old StartSpot.
    (void)IncomingName;
    return ChoosePlayerStart(Player);
}

AActor* AETGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    SelectedSpawnId = NAME_None;
    UWorld* World = GetWorld();
    if (!IsValid(World) || !IsValid(Player))
    {
        return nullptr;
    }
    UETRunSessionSubsystem* RunSession = GetRunSession();
    const FName EntryId = UETRunSessionSubsystem::GetEntryCheckpointId();
    const FName RequestedId = RunSession ? RunSession->GetSpawnCheckpointId(World) : EntryId;
    // 同一遍历同时统计入口与请求ID；保留重复ID检测，不取第一个就返回。
    APlayerStart* EntryStart = nullptr;
    APlayerStart* RequestedStart = nullptr;
    int32 EntryCount = 0;
    int32 RequestedCount = 0;
    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        APlayerStart* Candidate = *It;
        if (Candidate->bIsPIEPlayerStart)
        {
            continue;
        }
        if (Candidate->PlayerStartTag == EntryId)
        {
            EntryStart = Candidate;
            ++EntryCount;
        }
        if (Candidate->PlayerStartTag == RequestedId)
        {
            RequestedStart = Candidate;
            ++RequestedCount;
        }
    }
    // 入口必须唯一；请求ID失效才允许回退入口，不回退到世界原点。
    if (EntryCount != 1)
    {
        UE_LOG(LogETGameMode, Error,
               TEXT("[M1-06C] Entry invalid Matches=%d; require exactly one Entry"), EntryCount);
        return nullptr;
    }
    APlayerStart* Chosen = RequestedCount == 1 ? RequestedStart : EntryStart;
    if (RequestedCount != 1)
    {
        UE_LOG(LogETGameMode, Warning,
               TEXT("[M1-06C] Spawn fallback Requested=%s Matches=%d Fallback=Entry"),
               *RequestedId.ToString(), RequestedCount);
    }
    SelectedSpawnId = Chosen->PlayerStartTag;
    UE_LOG(LogETGameMode, Display,
           TEXT("[M1-06C] Spawn selected Requested=%s Actual=%s Start=%s Location=%s"),
           *RequestedId.ToString(), *SelectedSpawnId.ToString(), *GetNameSafe(Chosen),
           *Chosen->GetActorLocation().ToCompactString());
    return Chosen;
}

// ---- 复用PIE负例命令；真正写入范围由Session的WITH_EDITOR门控负责 ----
void AETGameMode::ETDebugSetCheckpointId(FName TestId)
{
    UETRunSessionSubsystem* RunSession = GetRunSession();
    const bool bApplied =
        RunSession && RunSession->SetCheckpointIdForDevelopmentTest(GetWorld(), TestId);
    UE_LOG(LogETGameMode, Display, TEXT("[M1-06C] Debug checkpoint result Id=%s Applied=%d"),
           *TestId.ToString(), bApplied ? 1 : 0);
}

// ---- Super完成后确认已有Pawn，再报告实际出生ID；不是身体GAS Ready ----
void AETGameMode::StartPlay()
{
    Super::StartPlay();
    if (!SelectedSpawnId.IsNone() && IsValid(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        if (UETRunSessionSubsystem* RunSession = GetRunSession())
        {
            RunSession->NotifyGameplayWorldReady(GetWorld(), SelectedSpawnId);
        }
    }
    UE_LOG(LogETGameMode, Display,
           TEXT("[M1-02B] Rules ready GameMode=%s ControllerClass=%s PawnClass=%s"),
           *GetNameSafe(this), *GetNameSafe(PlayerControllerClass.Get()),
           *GetNameSafe(DefaultPawnClass.Get()));
}
