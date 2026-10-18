#include "Abilities/Tasks/ETAbilityTask_DodgeMove.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETDodgeMove, Log, All);

UETAbilityTask_DodgeMove::UETAbilityTask_DodgeMove(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bTickingTask = true;
}

// ---- 工厂与配置查询 ----

UETAbilityTask_DodgeMove* UETAbilityTask_DodgeMove::CreateDodgeMove(UGameplayAbility* OwningAbility,
                                                                    const FVector& WorldDirection,
                                                                    float Distance, float Duration)
{
    UETAbilityTask_DodgeMove* Task = NewAbilityTask<UETAbilityTask_DodgeMove>(OwningAbility);
    Task->Direction = WorldDirection;
    Task->MoveDistance = Distance;
    Task->MoveDuration = Duration;
    return Task;
}

bool UETAbilityTask_DodgeMove::IsConfigurationValid(float Distance, float Duration)
{
    return FMath::IsFinite(Distance) && Distance >= 1.0f && Distance <= 300.0f &&
           FMath::IsFinite(Duration) && Duration >= 0.05f && Duration <= 0.50f;
}

bool UETAbilityTask_DodgeMove::IsMovementReady() const
{
    UCharacterMovementComponent* Move = Movement.Get();
    return !bFinished && IsValid(Move) && Source.IsValid() &&
           Move->GetRootMotionSourceByID(Source->LocalID) == Source;
}

// ---- 激活与移动监测 ----

void UETAbilityTask_DodgeMove::Activate()
{
    Super::Activate();
    ACharacter* Body = Cast<ACharacter>(GetAvatarActor());
    UCharacterMovementComponent* Move = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    if (!IsValid(Body) || !Body->HasAuthority() || Body->IsActorBeingDestroyed() ||
        !Body->GetWorld() || Body->GetWorld()->GetNetMode() != NM_Standalone ||
        UGameplayStatics::IsGamePaused(Body) || !IsValid(Move) || !Move->IsMovingOnGround() ||
        Move->HasRootMotionSources() || !IsConfigurationValid(MoveDistance, MoveDuration) ||
        Direction.ContainsNaN())
    {
        FinishMove(false, TEXT("InvalidStart"));
        return;
    }
    Direction = Direction.GetSafeNormal2D();
    if (Direction.IsNearlyZero())
    {
        FinishMove(false, TEXT("InvalidDirection"));
        return;
    }
    TSharedPtr<FRootMotionSource_ConstantForce> Candidate =
        MakeShared<FRootMotionSource_ConstantForce>();
    // 名称与优先级是正式闪避RMS协议，不是每敌人调参项。
    // 只覆盖水平速度；碰撞 / 坡面继续由CharacterMovement处理。
    Candidate->InstanceName = FName(TEXT("ETFormalDodge"));
    Candidate->Priority = 500;
    Candidate->AccumulateMode = ERootMotionAccumulateMode::Override;
    Candidate->bInLocalSpace = false;
    Candidate->Force = Direction * (MoveDistance / MoveDuration);
    Candidate->Duration = MoveDuration;
    // 保留末帧的部分时间积分，避免不同帧率下把最后一段位移截断。
    Candidate->Settings.UnSetFlag(ERootMotionSourceSettingsFlags::DisablePartialEndTick);
    Candidate->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
    Candidate->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
    Candidate->FinishVelocityParams.ClampVelocity = 0.0f;
    const uint16 ID = Move->ApplyRootMotionSource(Candidate);
    if (ID == static_cast<uint16>(ERootMotionSourceID::Invalid))
    {
        FinishMove(false, TEXT("SourceRejected"));
        return;
    }
    Movement = Move;
    Source = Candidate;
    StartLocation = Body->GetActorLocation();
    StartWorldTime = GetWorld()->GetTimeSeconds();
    UE_LOG(LogETDodgeMove, Display,
           TEXT("[M2-02B] Move begin Body=%s ID=%u Distance=%.1f Duration=%.3f Direction=%s"),
           *GetNameSafe(Body), ID, MoveDistance, MoveDuration, *Direction.ToCompactString());
}

void UETAbilityTask_DodgeMove::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);
    if (bFinished || UGameplayStatics::IsGamePaused(GetAvatarActor()))
    {
        return;
    }
    UCharacterMovementComponent* Move = Movement.Get();
    if (!IsValid(Move) || !Source.IsValid())
    {
        FinishMove(false, TEXT("MovementLost"));
        return;
    }
    // The movement component may already have removed a naturally finished source.
    if (Source->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
    {
        FinishMove(true, TEXT("WindowEnded"));
    }
    else if (!Move->IsMovingOnGround())
    {
        FinishMove(false, TEXT("LeftGround"));
    }
    else if (Source->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval) ||
             Move->GetRootMotionSourceByID(Source->LocalID) != Source)
    {
        FinishMove(false, TEXT("SourceRemoved"));
    }
}

// ---- 所有权释放与幂等结束 ----

void UETAbilityTask_DodgeMove::ReleaseSource(const TCHAR* Reason)
{
    if (!Source.IsValid())
    {
        return;
    }
    UCharacterMovementComponent* Move = Movement.Get();
    const ACharacter* Body = IsValid(Move) ? Move->GetCharacterOwner() : nullptr;
    const uint16 ID = Source->LocalID;
    const float SourceTime = Source->GetTime();
    const float Travel =
        IsValid(Body) ? FVector::Dist2D(StartLocation, Body->GetActorLocation()) : 0.0f;
    const float Elapsed = GetWorld() ? GetWorld()->GetTimeSeconds() - StartWorldTime : 0.0f;
    if (IsValid(Move))
    {
        const bool bOwned = Move->GetRootMotionSourceByID(ID) == Source;
        if (bOwned)
        {
            Move->RemoveRootMotionSourceByID(ID);
        }
        // 外部RMS仍在工作时不清零它的水平速度；只消除本次结束后的残留。
        if (bOwned || !Move->HasRootMotionSources())
        {
            Move->Velocity.X = 0.0f;
            Move->Velocity.Y = 0.0f;
        }
    }
    Source.Reset();
    Movement.Reset();
    UE_LOG(
        LogETDodgeMove, Display,
        TEXT(
            "[M2-02B] Move end Body=%s ID=%u Reason=%s Travel2D=%.2f SourceTime=%.4f Elapsed=%.3f"),
        *GetNameSafe(Body), ID, Reason, Travel, SourceTime, Elapsed);
}

void UETAbilityTask_DodgeMove::FinishMove(bool bSucceeded, const TCHAR* Reason)
{
    if (bFinished)
    {
        return;
    }
    // 清理或广播可能触发外部取消；先封口，保证只有一条完成 / 中断信号。
    bFinished = true;
    ReleaseSource(Reason);
    // EndTask marks this UObject as garbage. Copy the callback before ending it.
    const bool bNotify = ShouldBroadcastAbilityTaskDelegates();
    FETDodgeMoveSignal Callback = bSucceeded ? OnCompleted : OnAborted;
    EndTask();
    if (bNotify)
    {
        Callback.Broadcast();
    }
}

void UETAbilityTask_DodgeMove::OnDestroy(bool bInOwnerFinished)
{
    bFinished = true;
    ReleaseSource(bInOwnerFinished ? TEXT("AbilityEnd") : TEXT("TaskEnd"));
    Super::OnDestroy(bInOwnerFinished);
}