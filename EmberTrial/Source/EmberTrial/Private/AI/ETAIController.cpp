#include "AI/ETAIController.h"
#include "AI/ETBlackboardKeys.h"
#include "AI/ETAILiveBody.h"
#include "Characters/EmberTrialCharacter.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogETAIController, Log, All);

namespace
{
// 既有1cm到达容差；不把它与Move请求半径、恢复距离混为一项。
constexpr float ArrivalTolerance = 1.0f;

// ---- 共享存活规则之外，仍需验证本地玩家及双向控制关系 ----
bool IsLiveLocalPlayer(const AEmberTrialCharacter* BodyCharacter)
{
    if (!ETAIHealth::IsLiveBody(BodyCharacter))
    {
        return false;
    }
    const APlayerController* PlayerController =
        Cast<APlayerController>(BodyCharacter->GetController());
    return IsValid(PlayerController) && PlayerController->IsLocalController() &&
           PlayerController->GetPawn() == BodyCharacter;
}

// ---- 只移除本Controller持有的Handle；先撤销自身记录再解绑 ----
void RemoveTagSubscription(UAbilitySystemComponent* OldASC, FDelegateHandle& Subscription,
                           FGameplayTag Tag)
{
    const FDelegateHandle OldSubscription = Subscription;
    Subscription.Reset();
    if (IsValid(OldASC) && OldSubscription.IsValid())
    {
        OldASC->UnregisterGameplayTagEvent(OldSubscription, Tag,
                                           EGameplayTagEventType::NewOrRemoved);
    }
}

void RemoveDeadSubscription(TWeakObjectPtr<UAbilitySystemComponent>& WeakASC,
                            FDelegateHandle& Subscription)
{
    UAbilitySystemComponent* OldASC = WeakASC.Get();
    WeakASC.Reset();
    RemoveTagSubscription(OldASC, Subscription, ETGameplayTags::State_Dead);
}
} // namespace

// ---- 构造默认感知配置；不在Possess时自动启动AI逻辑 ----

AETAIController::AETAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bStartAILogicOnPossess = false;
    bStopAILogicOnUnposses = true;

    SightPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerception"));
    SetPerceptionComponent(*SightPerception);
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 900.0f;
    SightConfig->LoseSightRadius = 1100.0f;
    SightConfig->PeripheralVisionAngleDegrees = 70.0f;
    SightConfig->AutoSuccessRangeFromLastSeenLocation = -1.0f;
    SightConfig->SetMaxAge(2.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightPerception->ConfigureSense(*SightConfig);
    SightPerception->SetDominantSense(UAISense_Sight::StaticClass());
}

// ---- Controller / Pawn生命周期的启动握手 ----

void AETAIController::BeginPlay()
{
    Super::BeginPlay();
    bControllerBegunPlay = true;
    SightPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this,
                                                                &ThisClass::HandleSightUpdated);
    RefreshVisiblePlayer();
    UpdatePursuitDecision();
    TryStartEnemyBehaviorTree();
}

void AETAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    StopEnemyBehaviorTree(TEXT("BodyPossess"));
    ClearSensedPlayer(TEXT("BodyPossess"));
    StopObservingBody();
    bBlackboardReady = false;
    if (bEndingPlay)
    {
        return;
    }

    AEmberTrialCharacter* BodyCharacter = Cast<AEmberTrialCharacter>(GetPawn());
    if (!IsValid(BodyCharacter) || GetPawn() != InPawn)
    {
        UE_LOG(LogETAIController, Warning, TEXT("[M1-05A] Unsupported body Controller=%s Pawn=%s"),
               *GetNameSafe(this), *GetNameSafe(GetPawn()));
        return;
    }

    ObservedBody = BodyCharacter;
    BodyCharacter->OnGameplayReadyChanged.AddUniqueDynamic(this,
                                                           &ThisClass::HandleBodyReadyChanged);
    bBlackboardReady = InitializeEnemyBlackboard();
    ObserveBodyState();
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05A] Possessed Controller=%s Body=%s Authority=%d"), *GetNameSafe(this),
           *GetNameSafe(BodyCharacter), HasAuthority() ? 1 : 0);
    LogBodySnapshot(TEXT("PossessSnapshot"));
    RefreshVisiblePlayer();
    UpdatePursuitDecision();
    TryStartEnemyBehaviorTree();
}

// ---- 黑板资产 / 键类型契约；初始化后才允许决策读写 ----

bool AETAIController::InitializeEnemyBlackboard()
{
    UBlackboardComponent* Board = nullptr;
    if (!IsValid(EnemyBlackboardAsset) || !UseBlackboard(EnemyBlackboardAsset.Get(), Board) ||
        !IsValid(Board) || Board->GetBlackboardAsset() != EnemyBlackboardAsset.Get())
    {
        UE_LOG(LogETAIController, Warning,
               TEXT("[M1-05C2] Blackboard unavailable Controller=%s Asset=%s"), *GetNameSafe(this),
               *GetNameSafe(EnemyBlackboardAsset.Get()));
        return false;
    }

    const FBlackboard::FKey KeyID = Board->GetKeyID(ETBlackboardKeys::TargetActor);
    const FBlackboardEntry* Entry =
        KeyID != FBlackboard::InvalidKey ? EnemyBlackboardAsset->GetKey(KeyID) : nullptr;
    const UBlackboardKeyType_Object* ObjectKey =
        Entry ? Cast<UBlackboardKeyType_Object>(Entry->KeyType.Get()) : nullptr;
    if (!ObjectKey || !IsValid(ObjectKey->BaseClass) ||
        !AEmberTrialCharacter::StaticClass()->IsChildOf(ObjectKey->BaseClass.Get()))
    {
        UE_LOG(LogETAIController, Warning, TEXT("[M1-05C2] Invalid TargetActor key Controller=%s"),
               *GetNameSafe(this));
        return false;
    }

    const FBlackboard::FKey GateID = Board->GetKeyID(ETBlackboardKeys::CanPursue);
    const FBlackboard::FKey DistanceID = Board->GetKeyID(ETBlackboardKeys::PursuitStopDistance);
    const FBlackboard::FKey MeleeID = Board->GetKeyID(ETBlackboardKeys::CanMeleeAttack);
    if (GateID == FBlackboard::InvalidKey || DistanceID == FBlackboard::InvalidKey ||
        MeleeID == FBlackboard::InvalidKey ||
        Board->GetKeyType(GateID) != UBlackboardKeyType_Bool::StaticClass() ||
        Board->GetKeyType(DistanceID) != UBlackboardKeyType_Float::StaticClass() ||
        Board->GetKeyType(MeleeID) != UBlackboardKeyType_Bool::StaticClass())
    {
        UE_LOG(LogETAIController, Warning,
               TEXT("[M1-05C2] Invalid decision keys Controller=%s "
                    "Expected=TargetActor:Object,CanPursue:Bool,PursuitStopDistance:Float,"
                    "CanMeleeAttack:Bool"),
               *GetNameSafe(this));
        return false;
    }
    Board->ClearValue(ETBlackboardKeys::TargetActor);
    Board->SetValueAsBool(ETBlackboardKeys::CanPursue, false);
    Board->SetValueAsBool(ETBlackboardKeys::CanMeleeAttack, false);
    Board->SetValueAsFloat(ETBlackboardKeys::PursuitStopDistance, PursuitStopDistance);
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05C2] Blackboard ready Controller=%s Asset=%s Stop=%.1f Melee=%.1f"),
           *GetNameSafe(this), *GetNameSafe(EnemyBlackboardAsset.Get()), PursuitStopDistance,
           MeleeAttackDistance);
    return true;
}

// ---- 实时身份、配置与执行资格查询；不缓存Ready / 距离结论 ----

bool AETAIController::CanObservePlayer() const
{
    return !bEndingPlay && !bStoppingTree && bBlackboardReady && HasAuthority() &&
           ObservedBody.Get() == GetPawn() && ETAIHealth::IsLiveBody(ObservedBody.Get());
}

AEmberTrialCharacter* AETAIController::GetSensedPlayer() const
{
    AEmberTrialCharacter* PlayerBody = SensedPlayer.Get();
    return CanObservePlayer() && IsLiveLocalPlayer(PlayerBody) ? PlayerBody : nullptr;
}

bool AETAIController::HasValidPursuitConfig() const
{
    return FMath::IsFinite(PursuitStopDistance) && FMath::IsFinite(PursuitResumeDistance) &&
           FMath::IsFinite(MeleeAttackDistance) && FMath::IsFinite(MeleeRetryDelay) &&
           PursuitStopDistance >= 1.0f &&
           PursuitResumeDistance > PursuitStopDistance + ArrivalTolerance &&
           MeleeAttackDistance >= PursuitResumeDistance && MeleeRetryDelay >= 0.1f;
}

bool AETAIController::IsPursuitAllowed() const
{
    const AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    const AEmberTrialCharacter* PlayerBody = GetSensedPlayer();
    const UBlackboardComponent* Board = GetBlackboardComponent();
    const UCharacterMovementComponent* Movement =
        IsValid(BodyCharacter) ? BodyCharacter->GetCharacterMovement() : nullptr;
    return bPursuitAllowed && HasValidPursuitConfig() && IsValid(PlayerBody) && IsValid(Board) &&
           Board->GetValueAsObject(ETBlackboardKeys::TargetActor) == PlayerBody &&
           BodyCharacter->IsFormalActionIdle() && IsValid(Movement) && Movement->IsMovingOnGround();
}

bool AETAIController::IsMeleeAttackAllowed() const
{
    const AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    const AEmberTrialCharacter* PlayerBody = GetSensedPlayer();
    const UBlackboardComponent* Board = GetBlackboardComponent();
    const UWorld* World = GetWorld();
    const UCharacterMovementComponent* Movement =
        IsValid(BodyCharacter) ? BodyCharacter->GetCharacterMovement() : nullptr;
    if (!bMeleeAttackAllowed || !HasValidPursuitConfig() || !IsValid(BodyCharacter) ||
        !IsValid(PlayerBody) || !IsValid(Board) || !IsValid(World) ||
        World->GetNetMode() != NM_Standalone || World->IsPaused() ||
        World->GetTimeSeconds() < NextMeleeAttemptTime ||
        Board->GetValueAsObject(ETBlackboardKeys::TargetActor) != PlayerBody ||
        !BodyCharacter->IsFormalActionIdle() || !BodyCharacter->CanDealCombatDamageTo(PlayerBody) ||
        !BodyCharacter->GetLightAttackSpecHandle().IsValid() || !IsValid(Movement) ||
        !Movement->IsMovingOnGround())
    {
        return false;
    }
    const float Distance =
        FVector::Dist2D(BodyCharacter->GetActorLocation(), PlayerBody->GetActorLocation());
    return FMath::IsFinite(Distance) && Distance <= MeleeAttackDistance;
}

// ---- 世界时间冷却与追击 / 攻击双决策；保持各自状态边界 ----

void AETAIController::NotifyMeleeAttackAttemptFinished()
{
    UWorld* World = GetWorld();
    if (!bTreeStarted || !CanObservePlayer() || !HasValidPursuitConfig() || !IsValid(World))
    {
        return;
    }
    NextMeleeAttemptTime = static_cast<double>(World->GetTimeSeconds()) + MeleeRetryDelay;
    SetMeleeAttackAllowed(false, TEXT("RetryCooldown"), -1.0f);
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05C2] Melee retry Controller=%s Delay=%.2f NextWorldTime=%.3f"),
           *GetNameSafe(this), MeleeRetryDelay, NextMeleeAttemptTime);
}

void AETAIController::UpdatePursuitDecision()
{
    AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    AEmberTrialCharacter* PlayerBody = GetSensedPlayer();
    UBlackboardComponent* Board = GetBlackboardComponent();
    const auto CloseBoth = [this](const TCHAR* Reason, float Distance)
    {
        SetMeleeAttackAllowed(false, Reason, Distance);
        SetPursuitAllowed(false, Reason, Distance);
    };
    if (!CanObservePlayer() || !IsValid(Board))
    {
        CloseBoth(TEXT("BodyUnavailable"), -1.0f);
        return;
    }
    if (!HasValidPursuitConfig())
    {
        CloseBoth(TEXT("InvalidDistanceConfig"), -1.0f);
        return;
    }
    if (!IsValid(PlayerBody) ||
        Board->GetValueAsObject(ETBlackboardKeys::TargetActor) != PlayerBody)
    {
        bHoldingDistance = false;
        CloseBoth(TEXT("NoTarget"), -1.0f);
        return;
    }

    const float Distance =
        FVector::Dist2D(BodyCharacter->GetActorLocation(), PlayerBody->GetActorLocation());
    if (!FMath::IsFinite(Distance))
    {
        CloseBoth(TEXT("InvalidDistance"), -1.0f);
        return;
    }
    if (!BodyCharacter->CanDealCombatDamageTo(PlayerBody))
    {
        CloseBoth(TEXT("CombatTeam"), Distance);
        return;
    }
    if (Distance <= PursuitStopDistance + ArrivalTolerance)
    {
        bHoldingDistance = true;
    }
    else if (Distance >= PursuitResumeDistance)
    {
        bHoldingDistance = false;
    }

    UCharacterMovementComponent* Movement = BodyCharacter->GetCharacterMovement();
    if (!BodyCharacter->IsFormalActionIdle())
    {
        CloseBoth(TEXT("Busy"), Distance);
        return;
    }
    if (!IsValid(Movement) || !Movement->IsMovingOnGround())
    {
        CloseBoth(TEXT("OffGround"), Distance);
        return;
    }
    const UWorld* World = GetWorld();
    const bool bCanTry = IsValid(World) && World->GetNetMode() == NM_Standalone &&
                         World->GetTimeSeconds() >= NextMeleeAttemptTime &&
                         Distance <= MeleeAttackDistance &&
                         BodyCharacter->GetLightAttackSpecHandle().IsValid();
    SetMeleeAttackAllowed(bCanTry, bCanTry ? TEXT("InRange") : TEXT("RangeCooldownOrAbility"),
                          Distance);
    SetPursuitAllowed(!bHoldingDistance, bHoldingDistance ? TEXT("NearTarget") : TEXT("CanChase"),
                      Distance);
}

// ---- 先提交Controller状态，再通知黑板观察者 / 停止移动 ----

void AETAIController::SetPursuitAllowed(bool bNewAllowed, const TCHAR* Reason, float Distance,
                                        bool bForceStop)
{
    const bool bChanged = bPursuitAllowed != bNewAllowed;
    const bool bWasAllowed = bPursuitAllowed;
    // Commit before Blackboard observers / path completion callbacks run.
    bPursuitAllowed = bNewAllowed;
    UpdateBlackboardDecision(ETBlackboardKeys::CanPursue, bNewAllowed);
    if (!bNewAllowed && (bWasAllowed || bForceStop))
    {
        StopPursuitMovement();
    }
    if (bChanged || bForceStop)
    {
        UE_LOG(LogETAIController, Display,
               TEXT("[M1-05B2] Pursuit gate Controller=%s Allowed=%d Reason=%s Distance2D=%.1f "
                    "Holding=%d"),
               *GetNameSafe(this), bNewAllowed ? 1 : 0, Reason, Distance, bHoldingDistance ? 1 : 0);
    }
}

void AETAIController::SetMeleeAttackAllowed(bool bNewAllowed, const TCHAR* Reason, float Distance)
{
    const bool bChanged = bMeleeAttackAllowed != bNewAllowed;
    bMeleeAttackAllowed = bNewAllowed;
    UpdateBlackboardDecision(ETBlackboardKeys::CanMeleeAttack, bNewAllowed);
    if (bChanged)
    {
        UE_LOG(LogETAIController, Display,
               TEXT("[M1-05C2] Melee gate Controller=%s Allowed=%d Reason=%s Distance2D=%.1f"),
               *GetNameSafe(this), bNewAllowed ? 1 : 0, Reason, Distance);
    }
}

void AETAIController::UpdateBlackboardDecision(const FName& KeyName, bool bNewAllowed)
{
    // 保留原相同值去重；调用方负责先提交自己的决策标记。
    if (bBlackboardReady)
    {
        if (UBlackboardComponent* Board = GetBlackboardComponent())
        {
            if (Board->GetValueAsBool(KeyName) != bNewAllowed)
            {
                Board->SetValueAsBool(KeyName, bNewAllowed);
            }
        }
    }
}

void AETAIController::StopPursuitMovement()
{
    StopMovement();
    if (AEmberTrialCharacter* BodyCharacter = ObservedBody.Get())
    {
        if (UCharacterMovementComponent* Movement = BodyCharacter->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
}

// ---- 行为树一次启动与强制停止；保留同步回调防重入 ----

void AETAIController::TryStartEnemyBehaviorTree()
{
    if (!bControllerBegunPlay || bTreeStartAttempted || !CanObservePlayer())
    {
        return;
    }
    bTreeStartAttempted = true;
    if (!HasValidPursuitConfig() || !IsValid(EnemyBehaviorTree) ||
        EnemyBehaviorTree->BlackboardAsset.Get() != EnemyBlackboardAsset.Get() ||
        !IsValid(EnemyBehaviorTree->RootNode))
    {
        UE_LOG(LogETAIController, Warning,
               TEXT("[M1-05B2] Tree config rejected Controller=%s Tree=%s Stop=%.1f Resume=%.1f "
                    "Expected=ValidRootAndSameBlackboard"),
               *GetNameSafe(this), *GetNameSafe(EnemyBehaviorTree.Get()), PursuitStopDistance,
               PursuitResumeDistance);
        return;
    }
    UpdatePursuitDecision();
    bTreeStarted = true;
    const bool bAccepted = RunBehaviorTree(EnemyBehaviorTree.Get());
    if (!bAccepted || !CanObservePlayer())
    {
        StopEnemyBehaviorTree(TEXT("StartRejectedOrBodyUnavailable"));
        // Avoid retrying invalid configuration every service tick.
        bTreeStartAttempted = true;
    }
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05B2] Tree start requested Controller=%s Tree=%s Accepted=%d Stop=%.1f "
                "Resume=%.1f"),
           *GetNameSafe(this), *GetNameSafe(EnemyBehaviorTree.Get()),
           bAccepted && bTreeStarted ? 1 : 0, PursuitStopDistance, PursuitResumeDistance);
}

void AETAIController::StopEnemyBehaviorTree(const TCHAR* Reason)
{
    TGuardValue<bool> StopGuard(bStoppingTree, true);
    const bool bWasStarted = bTreeStarted;
    bTreeStarted = false;
    bTreeStartAttempted = false;
    bHoldingDistance = false;
    NextMeleeAttemptTime = 0.0;
    SetMeleeAttackAllowed(false, Reason, -1.0f);
    SetPursuitAllowed(false, Reason, -1.0f);
    if (UBehaviorTreeComponent* TreeComponent = Cast<UBehaviorTreeComponent>(GetBrainComponent()))
    {
        TreeComponent->StopTree(EBTStopMode::Forced);
    }
    StopPursuitMovement();
    if (bWasStarted)
    {
        UE_LOG(LogETAIController, Display, TEXT("[M1-05B2] Tree stopped Controller=%s Reason=%s"),
               *GetNameSafe(this), Reason);
    }
}

// ---- 身体Ready / ASC订阅；旧身体事件不能覆盖当前身体 ----

void AETAIController::HandleBodyReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady)
{
    if (bEndingPlay || !IsValid(BodyCharacter) || ObservedBody.Get() != BodyCharacter ||
        GetPawn() != BodyCharacter)
    {
        return;
    }
    if (!bReady)
    {
        StopEnemyBehaviorTree(TEXT("BodyNotReady"));
        ClearSensedPlayer(TEXT("BodyNotReady"));
        RemoveBodyStateSubscriptions();
    }
    else
    {
        ObserveBodyState();
        RefreshVisiblePlayer();
        UpdatePursuitDecision();
        TryStartEnemyBehaviorTree();
    }
    LogBodySnapshot(TEXT("ReadyChanged"));
}

void AETAIController::ObserveBodyState()
{
    AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    if (bEndingPlay || !IsValid(BodyCharacter) || !BodyCharacter->IsGameplayReady())
    {
        return;
    }
    UAbilitySystemComponent* BodyASC = BodyCharacter->GetAbilitySystemComponent();
    if (!IsValid(BodyASC) || (ObservedBodyASC.Get() == BodyASC && BodyDeadHandle.IsValid() &&
                              BodyAttackingHandle.IsValid() && BodyHitReactHandle.IsValid()))
    {
        return;
    }
    RemoveBodyStateSubscriptions();
    ObservedBodyASC = BodyASC;
    BodyDeadHandle = BodyASC
                         ->RegisterGameplayTagEvent(ETGameplayTags::State_Dead,
                                                    EGameplayTagEventType::NewOrRemoved)
                         .AddUObject(this, &ThisClass::HandleDeadTagChanged);
    BodyAttackingHandle = BodyASC
                              ->RegisterGameplayTagEvent(ETGameplayTags::State_Attacking,
                                                         EGameplayTagEventType::NewOrRemoved)
                              .AddUObject(this, &ThisClass::HandleBodyActionTagChanged);
    BodyHitReactHandle = BodyASC
                             ->RegisterGameplayTagEvent(ETGameplayTags::State_HitReact,
                                                        EGameplayTagEventType::NewOrRemoved)
                             .AddUObject(this, &ThisClass::HandleBodyActionTagChanged);
    if (BodyASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead))
    {
        ClearSensedPlayer(TEXT("BodyAlreadyDead"));
        StopEnemyBehaviorTree(TEXT("BodyAlreadyDead"));
    }
}

void AETAIController::RemoveBodyStateSubscriptions()
{
    UAbilitySystemComponent* OldASC = ObservedBodyASC.Get();
    ObservedBodyASC.Reset();
    RemoveTagSubscription(OldASC, BodyDeadHandle, ETGameplayTags::State_Dead);
    RemoveTagSubscription(OldASC, BodyAttackingHandle, ETGameplayTags::State_Attacking);
    RemoveTagSubscription(OldASC, BodyHitReactHandle, ETGameplayTags::State_HitReact);
}

void AETAIController::HandleBodyActionTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
    if (bEndingPlay || (ChangedTag != ETGameplayTags::State_Attacking &&
                        ChangedTag != ETGameplayTags::State_HitReact))
    {
        return;
    }
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05B2] Body action tag Controller=%s Tag=%s Count=%d"), *GetNameSafe(this),
           *ChangedTag.ToString(), NewCount);
    UpdatePursuitDecision();
}

// ---- 感知快照与目标替换；停止旧移动后再清除旧目标键 ----

void AETAIController::RefreshVisiblePlayer()
{
    if (!CanObservePlayer())
    {
        return;
    }
    TArray<AActor*> VisibleActors;
    SightPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), VisibleActors);
    for (AActor* VisibleActor : VisibleActors)
    {
        AEmberTrialCharacter* PlayerBody = Cast<AEmberTrialCharacter>(VisibleActor);
        if (IsLiveLocalPlayer(PlayerBody))
        {
            SetSensedPlayer(PlayerBody, TEXT("PerceptionSnapshot"));
            return;
        }
    }
}

void AETAIController::HandleSightUpdated(AActor* SensedActor, FAIStimulus Stimulus)
{
    if (bEndingPlay || Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
    {
        return;
    }
    if (!Stimulus.WasSuccessfullySensed())
    {
        if (SensedPlayer.Get(true) == SensedActor)
        {
            ClearSensedPlayer(TEXT("SightLost"));
        }
        return;
    }
    AEmberTrialCharacter* PlayerBody = Cast<AEmberTrialCharacter>(SensedActor);
    if (CanObservePlayer() && IsLiveLocalPlayer(PlayerBody))
    {
        SetSensedPlayer(PlayerBody, TEXT("SightAcquired"));
    }
}

void AETAIController::SetSensedPlayer(AEmberTrialCharacter* NewPlayer, const TCHAR* Reason)
{
    if (!CanObservePlayer() || !IsLiveLocalPlayer(NewPlayer) || SensedPlayer.Get() == NewPlayer)
    {
        return;
    }
    UAbilitySystemComponent* TargetASC = NewPlayer->GetAbilitySystemComponent();
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!IsValid(TargetASC) || !IsValid(Board))
    {
        return;
    }
    ClearSensedPlayer(TEXT("TargetChanged"));
    const FString NewPlayerName = GetNameSafe(NewPlayer);
    SensedPlayer = NewPlayer;
    ObservedTargetASC = TargetASC;
    NewPlayer->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleTargetEndPlay);
    TargetDeadHandle = TargetASC
                           ->RegisterGameplayTagEvent(ETGameplayTags::State_Dead,
                                                      EGameplayTagEventType::NewOrRemoved)
                           .AddUObject(this, &ThisClass::HandleDeadTagChanged);
    Board->SetValueAsObject(ETBlackboardKeys::TargetActor, NewPlayer);
    if (SensedPlayer.Get() != NewPlayer)
    {
        return;
    }
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05B1] Target acquired Controller=%s Player=%s Reason=%s BlackboardTarget=%s"),
           *GetNameSafe(this), *NewPlayerName, Reason,
           *GetNameSafe(Board->GetValueAsObject(ETBlackboardKeys::TargetActor)));
    UpdatePursuitDecision();
}

void AETAIController::ClearSensedPlayer(const TCHAR* Reason)
{
    AEmberTrialCharacter* OldPlayer = SensedPlayer.Get();
    const bool bHadTarget = !SensedPlayer.IsExplicitlyNull();
    const FString OldPlayerName = GetNameSafe(OldPlayer);
    SensedPlayer.Reset();
    bHoldingDistance = false;
    SetMeleeAttackAllowed(false, Reason, -1.0f);
    // Abort the old move before clearing its goal key.
    SetPursuitAllowed(false, Reason, -1.0f, bHadTarget);
    RemoveDeadSubscription(ObservedTargetASC, TargetDeadHandle);
    if (IsValid(OldPlayer))
    {
        OldPlayer->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleTargetEndPlay);
    }
    if (UBlackboardComponent* Board = GetBlackboardComponent())
    {
        if (bBlackboardReady)
        {
            Board->ClearValue(ETBlackboardKeys::TargetActor);
        }
    }
    if (bHadTarget)
    {
        UE_LOG(LogETAIController, Display,
               TEXT("[M1-05B1] Target cleared Controller=%s OldPlayer=%s Reason=%s "
                    "BlackboardTarget=None"),
               *GetNameSafe(this), *OldPlayerName, Reason);
    }
}

void AETAIController::HandleDeadTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
    if (bEndingPlay || ChangedTag != ETGameplayTags::State_Dead || NewCount <= 0)
    {
        return;
    }
    if (!ETAIHealth::IsLiveBody(ObservedBody.Get()))
    {
        ClearSensedPlayer(TEXT("BodyDead"));
        StopEnemyBehaviorTree(TEXT("BodyDead"));
    }
    else if (!SensedPlayer.IsExplicitlyNull() && !IsLiveLocalPlayer(SensedPlayer.Get()))
    {
        ClearSensedPlayer(TEXT("TargetDead"));
    }
}

void AETAIController::HandleTargetEndPlay(AActor* EndedTarget, EEndPlayReason::Type PlayEndReason)
{
    (void)PlayEndReason;
    // Get(true) is used only for identity comparison during target destruction.
    if (!bEndingPlay && SensedPlayer.Get(true) == EndedTarget)
    {
        ClearSensedPlayer(TEXT("TargetEndPlay"));
    }
}

// ---- 控制权 / 世界结束清理；先封住新活动，再撤销自己的订阅 ----

void AETAIController::StopObservingBody()
{
    AEmberTrialCharacter* OldBody = ObservedBody.Get();
    ObservedBody.Reset();
    RemoveBodyStateSubscriptions();
    if (IsValid(OldBody))
    {
        OldBody->OnGameplayReadyChanged.RemoveDynamic(this, &ThisClass::HandleBodyReadyChanged);
    }
}

void AETAIController::OnUnPossess()
{
    const FString OldBodyName = GetNameSafe(ObservedBody.Get());
    StopEnemyBehaviorTree(TEXT("BodyUnpossess"));
    ClearSensedPlayer(TEXT("BodyUnpossess"));
    StopObservingBody();
    bBlackboardReady = false;
    Super::OnUnPossess();
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05A] Unpossessed Controller=%s OldBody=%s Ending=%d"), *GetNameSafe(this),
           *OldBodyName, bEndingPlay ? 1 : 0);
}

void AETAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    const FString OldBodyName = GetNameSafe(ObservedBody.Get());
    SightPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleSightUpdated);
    StopEnemyBehaviorTree(TEXT("ControllerEndPlay"));
    ClearSensedPlayer(TEXT("ControllerEndPlay"));
    StopObservingBody();
    bBlackboardReady = false;
    Super::EndPlay(EndPlayReason);
    UE_LOG(LogETAIController, Display, TEXT("[M1-05A] End Controller=%s OldBody=%s Reason=%d"),
           *GetNameSafe(this), *OldBodyName, static_cast<int32>(EndPlayReason));
}

// ---- 诊断只读取当前身体，不重新初始化属性 ----

void AETAIController::LogBodySnapshot(const TCHAR* Reason) const
{
    const AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    if (bEndingPlay || !IsValid(BodyCharacter) || GetPawn() != BodyCharacter)
    {
        return;
    }
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    const bool bSnapshotValid = BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth);
    UE_LOG(LogETAIController, Display,
           TEXT("[M1-05A] Body snapshot Controller=%s Body=%s Reason=%s Ready=%d Snapshot=%d "
                "Health=%.1f MaxHealth=%.1f Dead=%d"),
           *GetNameSafe(this), *GetNameSafe(BodyCharacter), Reason,
           BodyCharacter->IsGameplayReady() ? 1 : 0, bSnapshotValid ? 1 : 0, Health, MaxHealth,
           BodyCharacter->IsCombatDead() ? 1 : 0);
}
