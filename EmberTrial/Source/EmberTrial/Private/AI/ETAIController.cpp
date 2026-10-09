#include "AI/ETAIController.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogETAIController, Log, All);

namespace
{
    const FName TargetActorKey(TEXT("TargetActor"));
    const FName CanPursueKey(TEXT("CanPursue"));
    const FName StopDistanceKey(TEXT("PursuitStopDistance"));
    constexpr float ArrivalTolerance = 1.0f;

    bool IsLiveBody(const AEmberTrialCharacter* BodyCharacter)
    {
        float Health = 0.0f;
        float MaxHealth = 0.0f;
        return IsValid(BodyCharacter) && !BodyCharacter->IsActorBeingDestroyed() &&
            !BodyCharacter->IsCombatDead() &&
            BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth) &&
            FMath::IsFinite(Health) && FMath::IsFinite(MaxHealth) &&
            MaxHealth >= 1.0f && Health > 0.0f && Health <= MaxHealth;
    }

    bool IsLiveLocalPlayer(const AEmberTrialCharacter* BodyCharacter)
    {
        if (!IsLiveBody(BodyCharacter))
        {
            return false;
        }
        const APlayerController* PlayerController =
            Cast<APlayerController>(BodyCharacter->GetController());
        return IsValid(PlayerController) && PlayerController->IsLocalController() &&
            PlayerController->GetPawn() == BodyCharacter;
    }

    void RemoveTagSubscription(UAbilitySystemComponent* OldASC,
        FDelegateHandle& Subscription, FGameplayTag Tag)
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
}

AETAIController::AETAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bStartAILogicOnPossess = false;
    bStopAILogicOnUnposses = true;

    SightPerception = CreateDefaultSubobject<UAIPerceptionComponent>(
        TEXT("EnemyPerception"));
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

void AETAIController::BeginPlay()
{
    Super::BeginPlay();
    bControllerBegunPlay = true;
    SightPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(
        this, &ThisClass::HandleSightUpdated);
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
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05A] Unsupported body Controller=%s Pawn=%s"),
            *GetNameSafe(this), *GetNameSafe(GetPawn()));
        return;
    }

    ObservedBody = BodyCharacter;
    BodyCharacter->OnGameplayReadyChanged.AddUniqueDynamic(
        this, &ThisClass::HandleBodyReadyChanged);
    bBlackboardReady = InitializeEnemyBlackboard();
    ObserveBodyState();
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] Possessed Controller=%s Body=%s Authority=%d"),
        *GetNameSafe(this), *GetNameSafe(BodyCharacter), HasAuthority() ? 1 : 0);
    LogBodySnapshot(TEXT("PossessSnapshot"));
    RefreshVisiblePlayer();
    UpdatePursuitDecision();
    TryStartEnemyBehaviorTree();
}

bool AETAIController::InitializeEnemyBlackboard()
{
    UBlackboardComponent* Board = nullptr;
    if (!IsValid(EnemyBlackboardAsset) ||
        !UseBlackboard(EnemyBlackboardAsset.Get(), Board) || !IsValid(Board) ||
        Board->GetBlackboardAsset() != EnemyBlackboardAsset.Get())
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05B1] Blackboard unavailable Controller=%s Asset=%s"),
            *GetNameSafe(this), *GetNameSafe(EnemyBlackboardAsset.Get()));
        return false;
    }

    const FBlackboard::FKey KeyID = Board->GetKeyID(TargetActorKey);
    const FBlackboardEntry* Entry = KeyID != FBlackboard::InvalidKey
        ? EnemyBlackboardAsset->GetKey(KeyID) : nullptr;
    const UBlackboardKeyType_Object* ObjectKey = Entry
        ? Cast<UBlackboardKeyType_Object>(Entry->KeyType.Get()) : nullptr;
    if (!ObjectKey || !IsValid(ObjectKey->BaseClass) ||
        !AEmberTrialCharacter::StaticClass()->IsChildOf(ObjectKey->BaseClass.Get()))
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05B1] Invalid TargetActor key Controller=%s Expected=ObjectWithCharacterCompatibleBase"),
            *GetNameSafe(this));
        return false;
    }

    const FBlackboard::FKey GateID = Board->GetKeyID(CanPursueKey);
    const FBlackboard::FKey DistanceID = Board->GetKeyID(StopDistanceKey);
    if (GateID == FBlackboard::InvalidKey ||
        DistanceID == FBlackboard::InvalidKey ||
        Board->GetKeyType(GateID) != UBlackboardKeyType_Bool::StaticClass() ||
        Board->GetKeyType(DistanceID) != UBlackboardKeyType_Float::StaticClass())
    {
        UE_LOG(LogETAIController, Warning,
            TEXT("[M1-05B2] Invalid pursuit keys Controller=%s Expected=CanPursue:Bool,PursuitStopDistance:Float"),
            *GetNameSafe(this));
        return false;
    }
    Board->SetValueAsBool(CanPursueKey, false);
    Board->ClearValue(TargetActorKey);
    Board->SetValueAsFloat(StopDistanceKey, PursuitStopDistance);
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05B1] Blackboard ready Controller=%s Asset=%s Target=None"),
        *GetNameSafe(this), *GetNameSafe(EnemyBlackboardAsset.Get()));
    return true;
}

bool AETAIController::CanObservePlayer() const
{
    return !bEndingPlay && bBlackboardReady && HasAuthority() &&
        ObservedBody.Get() == GetPawn() && IsLiveBody(ObservedBody.Get());
}

AEmberTrialCharacter* AETAIController::GetSensedPlayer() const
{
    AEmberTrialCharacter* PlayerBody = SensedPlayer.Get();
    return CanObservePlayer() && IsLiveLocalPlayer(PlayerBody) ? PlayerBody : nullptr;
}

bool AETAIController::HasValidPursuitConfig() const
{
    return FMath::IsFinite(PursuitStopDistance) &&
        FMath::IsFinite(PursuitResumeDistance) && PursuitStopDistance >= 1.0f &&
        PursuitResumeDistance > PursuitStopDistance + ArrivalTolerance;
}

bool AETAIController::IsPursuitAllowed() const
{
    const AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    const AEmberTrialCharacter* PlayerBody = GetSensedPlayer();
    const UBlackboardComponent* Board = GetBlackboardComponent();
    const UCharacterMovementComponent* Movement = IsValid(BodyCharacter)
        ? BodyCharacter->GetCharacterMovement() : nullptr;
    return bPursuitAllowed && HasValidPursuitConfig() &&
        IsValid(PlayerBody) && IsValid(Board) &&
        Board->GetValueAsObject(TargetActorKey) == PlayerBody &&
        BodyCharacter->IsFormalActionIdle() && IsValid(Movement) &&
        Movement->IsMovingOnGround();
}

void AETAIController::UpdatePursuitDecision()
{
    AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    AEmberTrialCharacter* PlayerBody = GetSensedPlayer();
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!CanObservePlayer() || !IsValid(Board))
    {
        SetPursuitAllowed(false, TEXT("BodyUnavailable"), -1.0f);
        return;
    }
    if (!HasValidPursuitConfig())
    {
        SetPursuitAllowed(false, TEXT("InvalidDistanceConfig"), -1.0f);
        return;
    }
    if (!IsValid(PlayerBody) ||
        Board->GetValueAsObject(TargetActorKey) != PlayerBody)
    {
        bHoldingDistance = false;
        SetPursuitAllowed(false, TEXT("NoTarget"), -1.0f);
        return;
    }

    const float Distance = FVector::Dist2D(BodyCharacter->GetActorLocation(),
        PlayerBody->GetActorLocation());
    if (!FMath::IsFinite(Distance))
    {
        SetPursuitAllowed(false, TEXT("InvalidDistance"), -1.0f);
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
        SetPursuitAllowed(false, TEXT("Busy"), Distance);
    }
    else if (!IsValid(Movement) || !Movement->IsMovingOnGround())
    {
        SetPursuitAllowed(false, TEXT("OffGround"), Distance);
    }
    else
    {
        SetPursuitAllowed(!bHoldingDistance,
            bHoldingDistance ? TEXT("NearTarget") : TEXT("CanChase"), Distance);
    }
}

void AETAIController::SetPursuitAllowed(bool bNewAllowed, const TCHAR* Reason,
    float Distance, bool bForceStop)
{
    const bool bChanged = bPursuitAllowed != bNewAllowed;
    const bool bWasAllowed = bPursuitAllowed;
    // Commit before Blackboard observers / path completion callbacks run.
    bPursuitAllowed = bNewAllowed;
    if (bBlackboardReady)
    {
        if (UBlackboardComponent* Board = GetBlackboardComponent())
        {
            if (Board->GetValueAsBool(CanPursueKey) != bNewAllowed)
            {
                Board->SetValueAsBool(CanPursueKey, bNewAllowed);
            }
        }
    }
    if (!bNewAllowed && (bWasAllowed || bForceStop))
    {
        StopPursuitMovement();
    }
    if (bChanged || bForceStop)
    {
        UE_LOG(LogETAIController, Display,
            TEXT("[M1-05B2] Pursuit gate Controller=%s Allowed=%d Reason=%s Distance2D=%.1f Holding=%d"),
            *GetNameSafe(this), bNewAllowed ? 1 : 0, Reason, Distance,
            bHoldingDistance ? 1 : 0);
    }
}

void AETAIController::StopPursuitMovement()
{
    StopMovement();
    if (AEmberTrialCharacter* BodyCharacter = ObservedBody.Get())
    {
        if (UCharacterMovementComponent* Movement =
            BodyCharacter->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
}

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
            TEXT("[M1-05B2] Tree config rejected Controller=%s Tree=%s Stop=%.1f Resume=%.1f Expected=ValidRootAndSameBlackboard"),
            *GetNameSafe(this), *GetNameSafe(EnemyBehaviorTree.Get()),
            PursuitStopDistance, PursuitResumeDistance);
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
        TEXT("[M1-05B2] Tree start requested Controller=%s Tree=%s Accepted=%d Stop=%.1f Resume=%.1f"),
        *GetNameSafe(this), *GetNameSafe(EnemyBehaviorTree.Get()),
        bAccepted && bTreeStarted ? 1 : 0,
        PursuitStopDistance, PursuitResumeDistance);
}

void AETAIController::StopEnemyBehaviorTree(const TCHAR* Reason)
{
    const bool bWasStarted = bTreeStarted;
    bTreeStarted = false;
    bTreeStartAttempted = false;
    bHoldingDistance = false;
    SetPursuitAllowed(false, Reason, -1.0f);
    if (UBehaviorTreeComponent* TreeComponent =
        Cast<UBehaviorTreeComponent>(GetBrainComponent()))
    {
        TreeComponent->StopTree(EBTStopMode::Forced);
    }
    StopPursuitMovement();
    if (bWasStarted)
    {
        UE_LOG(LogETAIController, Display,
            TEXT("[M1-05B2] Tree stopped Controller=%s Reason=%s"),
            *GetNameSafe(this), Reason);
    }
}

void AETAIController::HandleBodyReadyChanged(
    AEmberTrialCharacter* BodyCharacter, bool bReady)
{
    if (bEndingPlay || !IsValid(BodyCharacter) ||
        ObservedBody.Get() != BodyCharacter || GetPawn() != BodyCharacter)
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
    if (!IsValid(BodyASC) || (ObservedBodyASC.Get() == BodyASC &&
        BodyDeadHandle.IsValid() && BodyAttackingHandle.IsValid() &&
        BodyHitReactHandle.IsValid()))
    {
        return;
    }
    RemoveBodyStateSubscriptions();
    ObservedBodyASC = BodyASC;
    BodyDeadHandle = BodyASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleDeadTagChanged);
    BodyAttackingHandle = BodyASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_Attacking, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleBodyActionTagChanged);
    BodyHitReactHandle = BodyASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleBodyActionTagChanged);
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

void AETAIController::HandleBodyActionTagChanged(FGameplayTag ChangedTag,
    int32 NewCount)
{
    if (bEndingPlay || (ChangedTag != ETGameplayTags::State_Attacking &&
        ChangedTag != ETGameplayTags::State_HitReact))
    {
        return;
    }
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05B2] Body action tag Controller=%s Tag=%s Count=%d"),
        *GetNameSafe(this), *ChangedTag.ToString(), NewCount);
    UpdatePursuitDecision();
}

void AETAIController::RefreshVisiblePlayer()
{
    if (!CanObservePlayer())
    {
        return;
    }
    TArray<AActor*> VisibleActors;
    SightPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(),
        VisibleActors);
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

void AETAIController::SetSensedPlayer(AEmberTrialCharacter* NewPlayer,
    const TCHAR* Reason)
{
    if (!CanObservePlayer() || !IsLiveLocalPlayer(NewPlayer) ||
        SensedPlayer.Get() == NewPlayer)
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
    TargetDeadHandle = TargetASC->RegisterGameplayTagEvent(
        ETGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this, &ThisClass::HandleDeadTagChanged);
    Board->SetValueAsObject(TargetActorKey, NewPlayer);
    if (SensedPlayer.Get() != NewPlayer)
    {
        return;
    }
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05B1] Target acquired Controller=%s Player=%s Reason=%s BlackboardTarget=%s"),
        *GetNameSafe(this), *NewPlayerName, Reason,
        *GetNameSafe(Board->GetValueAsObject(TargetActorKey)));
    UpdatePursuitDecision();
}

void AETAIController::ClearSensedPlayer(const TCHAR* Reason)
{
    AEmberTrialCharacter* OldPlayer = SensedPlayer.Get();
    const bool bHadTarget = !SensedPlayer.IsExplicitlyNull();
    const FString OldPlayerName = GetNameSafe(OldPlayer);
    SensedPlayer.Reset();
    bHoldingDistance = false;
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
            Board->ClearValue(TargetActorKey);
        }
    }
    if (bHadTarget)
    {
        UE_LOG(LogETAIController, Display,
            TEXT("[M1-05B1] Target cleared Controller=%s OldPlayer=%s Reason=%s BlackboardTarget=None"),
            *GetNameSafe(this), *OldPlayerName, Reason);
    }
}

void AETAIController::HandleDeadTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
    if (bEndingPlay || ChangedTag != ETGameplayTags::State_Dead || NewCount <= 0)
    {
        return;
    }
    if (!IsLiveBody(ObservedBody.Get()))
    {
        ClearSensedPlayer(TEXT("BodyDead"));
        StopEnemyBehaviorTree(TEXT("BodyDead"));
    }
    else if (!SensedPlayer.IsExplicitlyNull() &&
        !IsLiveLocalPlayer(SensedPlayer.Get()))
    {
        ClearSensedPlayer(TEXT("TargetDead"));
    }
}

void AETAIController::HandleTargetEndPlay(AActor* EndedTarget,
    EEndPlayReason::Type PlayEndReason)
{
    (void)PlayEndReason;
    // Get(true) is used only for identity comparison during target destruction.
    if (!bEndingPlay && SensedPlayer.Get(true) == EndedTarget)
    {
        ClearSensedPlayer(TEXT("TargetEndPlay"));
    }
}

void AETAIController::StopObservingBody()
{
    AEmberTrialCharacter* OldBody = ObservedBody.Get();
    ObservedBody.Reset();
    RemoveBodyStateSubscriptions();
    if (IsValid(OldBody))
    {
        OldBody->OnGameplayReadyChanged.RemoveDynamic(
            this, &ThisClass::HandleBodyReadyChanged);
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
        TEXT("[M1-05A] Unpossessed Controller=%s OldBody=%s Ending=%d"),
        *GetNameSafe(this), *OldBodyName, bEndingPlay ? 1 : 0);
}

void AETAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    const FString OldBodyName = GetNameSafe(ObservedBody.Get());
    SightPerception->OnTargetPerceptionUpdated.RemoveDynamic(
        this, &ThisClass::HandleSightUpdated);
    StopEnemyBehaviorTree(TEXT("ControllerEndPlay"));
    ClearSensedPlayer(TEXT("ControllerEndPlay"));
    StopObservingBody();
    bBlackboardReady = false;
    Super::EndPlay(EndPlayReason);
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] End Controller=%s OldBody=%s Reason=%d"),
        *GetNameSafe(this), *OldBodyName, static_cast<int32>(EndPlayReason));
}

void AETAIController::LogBodySnapshot(const TCHAR* Reason) const
{
    const AEmberTrialCharacter* BodyCharacter = ObservedBody.Get();
    if (bEndingPlay || !IsValid(BodyCharacter) || GetPawn() != BodyCharacter)
    {
        return;
    }
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    const bool bSnapshotValid = BodyCharacter->GetGameplayHealthSnapshot(
        Health, MaxHealth);
    UE_LOG(LogETAIController, Display,
        TEXT("[M1-05A] Body snapshot Controller=%s Body=%s Reason=%s Ready=%d Snapshot=%d Health=%.1f MaxHealth=%.1f Dead=%d"),
        *GetNameSafe(this), *GetNameSafe(BodyCharacter), Reason,
        BodyCharacter->IsGameplayReady() ? 1 : 0,
        bSnapshotValid ? 1 : 0, Health, MaxHealth,
        BodyCharacter->IsCombatDead() ? 1 : 0);
}