#include "ETPracticeActionComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/RootMotionSource.h"

DEFINE_LOG_CATEGORY_STATIC(LogETPracticeAction, Log, All);

UETPracticeActionComponent::UETPracticeActionComponent()
{
    // Lifecycle safety check, enabled only while this component owns a montage.
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    bAutoActivate = true;
}

void UETPracticeActionComponent::BeginPlay()
{
    Super::BeginPlay();
    CharacterOwner = Cast<ACharacter>(GetOwner());
    if (!CharacterOwner.IsValid())
    {
        UE_LOG(LogETPracticeAction, Error,
            TEXT("[M0-05C2] Component requires an ACharacter owner: %s"), *GetNameSafe(GetOwner()));
        return;
    }

    if (USkeletalMeshComponent* Mesh = CharacterOwner->GetMesh())
    {
        AddTickPrerequisiteComponent(Mesh);
    }

    if (UCharacterMovementComponent* Movement = CharacterOwner->GetCharacterMovement())
    {
		AddTickPrerequisiteComponent(Movement);
    }

    UE_LOG(LogETPracticeAction, Display, TEXT("[M0-05C2] Component ready Owner=%s State=Idle"),
        *GetNameSafe(CharacterOwner.Get()));
}

EETPracticeActionState UETPracticeActionComponent::GetPracticeActionState() const
{
    return CurrentState;
}

bool UETPracticeActionComponent::IsPracticeActionIdle() const
{
    return CurrentState == EETPracticeActionState::Idle;
}

UAnimMontage* UETPracticeActionComponent::ResolveMontage(EETPracticeActionState State) const
{
    switch (State)
    {
    case EETPracticeActionState::Attacking: 
        return AttackMontage;
    case EETPracticeActionState::Dodging: 
        return DodgeMontage;
    case EETPracticeActionState::HitReact: 
        return HitReactMontage;
    default: 
        return nullptr;
    }
}

bool UETPracticeActionComponent::RejectRequest(EETPracticeActionState RequestedState, const TCHAR* Reason) const
{
    UE_LOG(LogETPracticeAction, Display, TEXT("[M0-05C2] Reject %s Reason=%s"),
        *UEnum::GetValueAsString(RequestedState), Reason);
    return false;
}

bool UETPracticeActionComponent::RequestPracticeAction(EETPracticeActionState RequestedState)
{
    if (bEndingPlay || !HasBegunPlay() || !IsActive() || !CharacterOwner.IsValid())
    {
        return RejectRequest(RequestedState, TEXT("NotReady"));
    }

    if (UGameplayStatics::IsGamePaused(this))
    {
        return RejectRequest(RequestedState, TEXT("Paused"));
    }

    if (RequestedState != EETPracticeActionState::Attacking
        && RequestedState != EETPracticeActionState::Dodging
        && RequestedState != EETPracticeActionState::HitReact)
    {
        return RejectRequest(RequestedState, TEXT("InvalidAction"));
    }

    const bool bCanInterrupt = RequestedState == EETPracticeActionState::HitReact
        && (CurrentState == EETPracticeActionState::Attacking || CurrentState == EETPracticeActionState::Dodging);

    if (!IsPracticeActionIdle() && !bCanInterrupt)
    {
        return RejectRequest(RequestedState, TEXT("Busy"));
    }

    ACharacter* Character = CharacterOwner.Get();
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (!IsValid(Movement) || !Movement->IsMovingOnGround())
    {
        return RejectRequest(RequestedState, TEXT("NotGrounded"));
    }

    if (RequestedState == EETPracticeActionState::Attacking
        && Character->GetVelocity().SizeSquared2D() > FMath::Square(FMath::Max(0.0f, AttackMaxHorizontalSpeed)))
    {
        return RejectRequest(RequestedState, TEXT("Moving"));
    }

    UAnimMontage* Montage = ResolveMontage(RequestedState);
    if (!IsValid(Montage))
    {
        return RejectRequest(RequestedState, TEXT("MissingMontage"));
    }

    USkeletalMeshComponent* Mesh = Character->GetMesh();
    UAnimInstance* AnimInstance = IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr;
    if (!IsValid(AnimInstance))
    {
        return RejectRequest(RequestedState, TEXT("MissingAnimInstance"));
    }

    if (IsPracticeActionIdle() && AnimInstance->IsAnyMontagePlaying())
    {
        return RejectRequest(RequestedState, TEXT("ExternalMontage"));
    }

    // Retire the old serial before stopping: its end callback may already be queued.
    if (bCanInterrupt)
    {
        ReleaseAction(true, TEXT("HitInterrupt"));
    }

    const uint32 ThisSerial = ++ActionSerial;
    CurrentState = RequestedState;
    ActiveAnimInstance = AnimInstance;
    ActiveMontage = Montage;
    const float PlayedLength = AnimInstance->Montage_Play(Montage, 1.0f,
        EMontagePlayReturnType::MontageLength, 0.0f, false);
    if (PlayedLength <= 0.0f)
    {
        ReleaseAction(false, TEXT("PlaybackFailed"));
        return RejectRequest(RequestedState, TEXT("PlaybackFailed"));
    }

    FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(Montage);
    if (!Instance)
    {
        ReleaseAction(false, TEXT("MissingMontageInstance"));
        return RejectRequest(RequestedState, TEXT("MissingMontageInstance"));
    }

    ActiveMontageInstanceID = Instance->GetInstanceID();
    Instance->OnMontageEnded.BindUObject(this, &ThisClass::HandleMontageEnded, ThisSerial);
	Instance->OnMontageBlendingOutStarted.BindUObject(this, &ThisClass::HandleMontageBlendingOut, ThisSerial);
    Character->StopJumping();
    Character->ConsumeMovementInputVector();
    Movement->StopMovementImmediately();

    if (RequestedState == EETPracticeActionState::Dodging && !StartDodgeMovement(Character, Movement))
    {
        ReleaseAction(true, TEXT("DodgeMovementFailed"));
        return RejectRequest(RequestedState, TEXT("DodgeMovementFailed"));
    }

    SetComponentTickEnabled(true);
    UE_LOG(LogETPracticeAction, Display, TEXT("[M0-05C2] Begin %s Montage=%s Serial=%u"),
        *UEnum::GetValueAsString(CurrentState), *GetNameSafe(Montage), ThisSerial);
    return true;
}

void UETPracticeActionComponent::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, uint32 Serial)
{
    if (bEndingPlay || Serial != ActionSerial || Montage != ActiveMontage || IsPracticeActionIdle())
    {
        return;
    }
    ReleaseAction(false, bInterrupted ? TEXT("Interrupted") : TEXT("Completed"));
}

void UETPracticeActionComponent::ReleaseAction(bool bStopMontage, const TCHAR* Reason)
{
    const EETPracticeActionState PreviousState = CurrentState;
    UAnimInstance* AnimInstance = ActiveAnimInstance.Get();
    FAnimMontageInstance* Instance = AnimInstance
        ? AnimInstance->GetMontageInstanceForID(ActiveMontageInstanceID) : nullptr;
    ++ActionSerial;
    StopDodgeMovement(Reason);
    CurrentState = EETPracticeActionState::Idle;
    ActiveMontage = nullptr;
    ActiveAnimInstance.Reset();
    ActiveMontageInstanceID = INDEX_NONE;
    SetComponentTickEnabled(false);
    if (Instance)
    {
        if (Instance->OnMontageEnded.IsBoundToObject(this))
        {
            Instance->OnMontageEnded.Unbind();
        }
        if (Instance->OnMontageBlendingOutStarted.IsBoundToObject(this))
        {
            Instance->OnMontageBlendingOutStarted.Unbind();
        }
        if (bStopMontage)
        {
            Instance->Stop(FAlphaBlend(0.05f), true);
        }
    }
    if (PreviousState != EETPracticeActionState::Idle)
    {
        UE_LOG(LogETPracticeAction, Display, TEXT("[M0-05C2] End %s -> Idle Reason=%s"),
            *UEnum::GetValueAsString(PreviousState), Reason);
    }
}

void UETPracticeActionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    ACharacter* Character = CharacterOwner.Get();
    UAnimInstance* AnimInstance = ActiveAnimInstance.Get();
    if (!Character || !AnimInstance || !Character->GetMesh()
        || Character->GetMesh()->GetAnimInstance() != AnimInstance)
    {
        ReleaseAction(true, TEXT("AnimInstanceChanged"));
        return;
    }
    if (!AnimInstance->GetMontageInstanceForID(ActiveMontageInstanceID))
    {
        ReleaseAction(false, TEXT("MontageInstanceLost"));
        return;
    }
    UpdateDodgeMovement();
}

void UETPracticeActionComponent::CancelPracticeAction()
{
    ReleaseAction(true, TEXT("Cancelled"));
}

void UETPracticeActionComponent::Activate(bool bReset)
{
    Super::Activate(bReset);
    SetComponentTickEnabled(!bEndingPlay && !IsPracticeActionIdle());
}

void UETPracticeActionComponent::Deactivate()
{
    CancelPracticeAction();
    Super::Deactivate();
}

void UETPracticeActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    ReleaseAction(true, TEXT("EndPlay"));
    CharacterOwner.Reset();
    Super::EndPlay(EndPlayReason);
}

bool UETPracticeActionComponent::StartDodgeMovement(ACharacter* Character, UCharacterMovementComponent* Movement)
{
    const FVector Direction = Character->GetActorRightVector().GetSafeNormal2D();
    if (Direction.IsNearlyZero())
    {
        return false;
    }

    TSharedPtr<FRootMotionSource_ConstantForce> Source =
        MakeShared<FRootMotionSource_ConstantForce>();
    Source->InstanceName = FName(TEXT("ETPracticeDodge"));
    Source->Priority = 500;
    Source->AccumulateMode = ERootMotionAccumulateMode::Override;
    Source->bInLocalSpace = false;
    Source->Force = Direction * (DodgeDistance / DodgeDuration);
    Source->Duration = DodgeDuration;
    // Scale the last partial frame instead of overshooting the requested duration.
    Source->Settings.UnSetFlag(ERootMotionSourceSettingsFlags::DisablePartialEndTick);
    Source->Settings.SetFlag(ERootMotionSourceSettingsFlags::IgnoreZAccumulate);
    Source->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
    Source->FinishVelocityParams.ClampVelocity = 0.0f;

    const uint16 SourceID = Movement->ApplyRootMotionSource(Source);
    if (SourceID == static_cast<uint16>(ERootMotionSourceID::Invalid))
    {
        return false;
    }
    DodgeMovement = Movement;
    DodgeSource = Source;
    DodgeStartLocation = Character->GetActorLocation();
    DodgeStartWorldTime = GetWorld()->GetTimeSeconds();
    UE_LOG(LogETPracticeAction, Display,
        TEXT("[M0-05C2] DodgeMove Begin ID=%u Distance=%.1f Duration=%.3f Direction=%s"),
        SourceID, DodgeDistance, DodgeDuration, *Direction.ToCompactString());
    return true;
}

void UETPracticeActionComponent::StopDodgeMovement(const TCHAR* Reason)
{
    if (!DodgeSource.IsValid())
    {
        return;
    }
    UCharacterMovementComponent* Movement = DodgeMovement.Get();
    ACharacter* Character = Movement ? Movement->GetCharacterOwner() : CharacterOwner.Get();
    const float Travel2D = IsValid(Character)
        ? FVector::Dist2D(DodgeStartLocation, Character->GetActorLocation()) : 0.0f;
    const float Elapsed = GetWorld()
        ? GetWorld()->GetTimeSeconds() - DodgeStartWorldTime : 0.0f;
    if (Movement)
    {
        const bool bStillOwned =
            Movement->GetRootMotionSourceByID(DodgeSource->LocalID) == DodgeSource;
        if (bStillOwned)
        {
            Movement->RemoveRootMotionSourceByID(DodgeSource->LocalID);
        }
        // Clear our horizontal momentum; never zero the character's falling speed.
        if (bStillOwned || !Movement->HasRootMotionSources())
        {
            Movement->Velocity.X = 0.0f;
            Movement->Velocity.Y = 0.0f;
        }
    }
    DodgeSource.Reset();
    DodgeMovement.Reset();
    UE_LOG(LogETPracticeAction, Display,
        TEXT("[M0-05C2] DodgeMove End Reason=%s Travel2D=%.1f Elapsed=%.3f"),
        Reason, Travel2D, Elapsed);
}

void UETPracticeActionComponent::UpdateDodgeMovement()
{
    if (!DodgeSource.IsValid())
    {
        return;
    }
    UCharacterMovementComponent* Movement = DodgeMovement.Get();
    if (!Movement)
    {
        StopDodgeMovement(TEXT("MovementLost"));
        return;
    }
    if (!Movement->IsMovingOnGround())
    {
        StopDodgeMovement(TEXT("LeftGround"));
        return;
    }
    if (Movement->GetRootMotionSourceByID(DodgeSource->LocalID) != DodgeSource)
    {
        StopDodgeMovement(TEXT("SourceRemoved"));
        return;
    }
    if (DodgeSource->Status.HasFlag(ERootMotionSourceStatusFlags::Finished))
    {
        StopDodgeMovement(TEXT("DistanceWindowEnded"));
    }
    else if (DodgeSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval))
    {
        StopDodgeMovement(TEXT("SourceRemoved"));
    }
}

void UETPracticeActionComponent::HandleMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, uint32 Serial)
{
    if (bEndingPlay || Serial != ActionSerial || Montage != ActiveMontage
        || IsPracticeActionIdle())
    {
        return;
    }
    if (bInterrupted)
    {
        StopDodgeMovement(TEXT("MontageInterrupted"));
    }
}