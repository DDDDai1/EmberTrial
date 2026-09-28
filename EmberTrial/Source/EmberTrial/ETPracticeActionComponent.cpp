#include "ETPracticeActionComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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
    case EETPracticeActionState::Attacking: return AttackMontage;
    case EETPracticeActionState::Dodging: return DodgeMontage;
    case EETPracticeActionState::HitReact: return HitReactMontage;
    default: return nullptr;
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
    Character->StopJumping();
    Character->ConsumeMovementInputVector();
    Movement->StopMovementImmediately();
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
        if (bStopMontage)
        {
            // Stop this instance only, not montages owned by another system.
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
    }
    else if (!AnimInstance->GetMontageInstanceForID(ActiveMontageInstanceID))
    {
        // Recovery if another system replaced the delegate or reset the instance.
        ReleaseAction(false, TEXT("MontageInstanceLost"));
    }
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
