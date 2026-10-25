#include "Framework/ETGameMode.h"
#include "Framework/ETPlayerController.h"
#include "Characters/EmberTrialCharacter.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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

// ---- 先按实际Pawn胶囊贴近可走地面，再生成 / 初始化动画，避免首帧IK追赶出生落差 ----
APawn* AETGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer,
                                                               const FTransform& SpawnTransform)
{
    const UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
    const ACharacter* DefaultBody =
        PawnClass ? Cast<ACharacter>(PawnClass->GetDefaultObject()) : nullptr;
    const FTransform Grounded = MakeGroundedSpawnTransform(SpawnTransform, DefaultBody);
    if (!Grounded.GetLocation().Equals(SpawnTransform.GetLocation(), .01))
    {
        UE_LOG(LogETGameMode, Display, TEXT("[M2-06C] Spawn grounded StartZ=%.2f PawnZ=%.2f"),
               SpawnTransform.GetLocation().Z, Grounded.GetLocation().Z);
    }
    return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, Grounded);
}

FTransform AETGameMode::MakeGroundedSpawnTransform(const FTransform& SpawnTransform,
                                                   const ACharacter* DefaultBody) const
{
    UWorld* World = GetWorld();
    const UCapsuleComponent* Capsule =
        IsValid(DefaultBody) ? DefaultBody->GetCapsuleComponent() : nullptr;
    const UCharacterMovementComponent* Movement =
        IsValid(DefaultBody) ? DefaultBody->GetCharacterMovement() : nullptr;
    const FVector Origin = SpawnTransform.GetLocation();
    const FVector Scale = SpawnTransform.GetScale3D();
    const FRotator Rotation = SpawnTransform.Rotator().GetNormalized();
    if (!IsValid(World) || World->bIsTearingDown || !IsValid(Capsule) || !IsValid(Movement) ||
        !FMath::IsFinite(SpawnGroundProbeDistance) || SpawnGroundProbeDistance <= 0.0f ||
        SpawnGroundProbeDistance > 500.0f || Origin.ContainsNaN() || Scale.ContainsNaN() ||
        Scale.X <= 0.0 || !Scale.Equals(FVector(Scale.X), .001) || Rotation.ContainsNaN() ||
        !FMath::IsNearlyZero(Rotation.Pitch) || !FMath::IsNearlyZero(Rotation.Roll))
    {
        return SpawnTransform;
    }
    float Radius = 0.0f;
    float HalfHeight = 0.0f;
    Capsule->GetScaledCapsuleSize(Radius, HalfHeight);
    Radius *= static_cast<float>(Scale.X);
    HalfHeight *= static_cast<float>(Scale.X);
    if (!FMath::IsFinite(Radius) || !FMath::IsFinite(HalfHeight) || Radius <= 0.0f ||
        HalfHeight < Radius)
    {
        return SpawnTransform;
    }
    FHitResult Hit;
    const FCollisionQueryParams Params(SCENE_QUERY_STAT(ETSpawnGround), false);
    if (!World->SweepSingleByChannel(Hit, Origin,
                                     Origin - FVector::UpVector * SpawnGroundProbeDistance,
                                     FQuat::Identity, Capsule->GetCollisionObjectType(),
                                     FCollisionShape::MakeCapsule(Radius, HalfHeight), Params) ||
        Hit.bStartPenetrating || !Movement->IsWalkable(Hit) ||
        (Hit.GetActor() && Hit.GetActor()->IsA<APawn>()))
    {
        return SpawnTransform;
    }
    // 复用CharacterMovement原生地面间隙，不在地图或角色代码硬编码90 / 96cm。
    const float FloorGap = (UCharacterMovementComponent::MIN_FLOOR_DIST +
                            UCharacterMovementComponent::MAX_FLOOR_DIST) *
                           .5f;
    FTransform Result = SpawnTransform;
    Result.SetLocation(Hit.Location + FVector::UpVector * FloorGap);
    return Result;
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
