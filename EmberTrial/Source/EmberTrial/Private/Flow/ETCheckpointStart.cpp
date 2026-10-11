#include "Flow/ETCheckpointStart.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogETCheckpointStart, Log, All);

AETCheckpointStart::AETCheckpointStart(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;
    PlayerStartTag = FName(TEXT("Checkpoint_A"));
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);

    ActivationTrigger = CreateDefaultSubobject<USphereComponent>(
        TEXT("ActivationTrigger"));
    ActivationTrigger->SetupAttachment(GetRootComponent());
    ActivationTrigger->SetSphereRadius(120.0f);
    ActivationTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ActivationTrigger->SetCollisionObjectType(ECC_WorldDynamic);
    ActivationTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    ActivationTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    ActivationTrigger->SetGenerateOverlapEvents(true);
    ActivationTrigger->SetCanEverAffectNavigation(false);

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Marker->SetupAttachment(GetRootComponent());
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetGenerateOverlapEvents(false);
    Marker->SetCanEverAffectNavigation(false);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(GetRootComponent());
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(24.0f);
    Label->SetText(FText::FromString(TEXT("CHECKPOINT")));
    Label->SetTextRenderColor(FColor(0, 180, 255));
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetCanEverAffectNavigation(false);
}

void AETCheckpointStart::BeginPlay()
{
    Super::BeginPlay();
    bEndingPlay = false;
    if (UGameInstance* Instance = GetGameInstance())
    {
        Session = Instance->GetSubsystem<UETRunSessionSubsystem>();
    }
    if (Session.IsValid())
    {
        CheckpointChangedHandle = Session->OnCheckpointChanged.AddUObject(
            this, &ThisClass::HandleCheckpointChanged);
    }
    ActivationTrigger->OnComponentBeginOverlap.AddUniqueDynamic(
        this, &ThisClass::HandleTriggerBeginOverlap);
    RefreshDisplay();
    if (UWorld* World = GetWorld())
    {
        InitialProbeHandle = World->GetTimerManager().SetTimerForNextTick(
            this, &ThisClass::ProbeInitialOverlap);
    }
    UE_LOG(LogETCheckpointStart, Display,
        TEXT("[M1-06C] Checkpoint ready Actor=%s Id=%s"),
        *GetNameSafe(this), *PlayerStartTag.ToString());
}

void AETCheckpointStart::HandleTriggerBeginOverlap(
    UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    (void)OtherComp;
    (void)OtherBodyIndex;
    (void)bFromSweep;
    (void)SweepResult;
    if (!bEndingPlay && OverlappedComponent == ActivationTrigger.Get() &&
        Session.IsValid())
    {
        if (AEmberTrialCharacter* PlayerBody =
            Cast<AEmberTrialCharacter>(OtherActor))
        {
            Session->TryActivateCheckpoint(GetWorld(), this, PlayerBody);
            RefreshDisplay();
        }
    }
}

void AETCheckpointStart::ProbeInitialOverlap()
{
    InitialProbeHandle.Invalidate();
    if (bEndingPlay || !Session.IsValid() || !IsValid(ActivationTrigger.Get()))
    {
        return;
    }
    TArray<AActor*> Overlaps;
    ActivationTrigger->GetOverlappingActors(
        Overlaps, AEmberTrialCharacter::StaticClass());
    for (AActor* Overlap : Overlaps)
    {
        if (AEmberTrialCharacter* PlayerBody = Cast<AEmberTrialCharacter>(Overlap))
        {
            Session->TryActivateCheckpoint(GetWorld(), this, PlayerBody);
        }
    }
    RefreshDisplay();
}

void AETCheckpointStart::HandleCheckpointChanged(
    UWorld* ChangedWorld, FName CheckpointId)
{
    (void)CheckpointId;
    if (!bEndingPlay && ChangedWorld == GetWorld())
    {
        RefreshDisplay();
    }
}

void AETCheckpointStart::RefreshDisplay()
{
    if (!IsValid(Label.Get()))
    {
        return;
    }
    const bool bActive = Session.IsValid() &&
        Session->IsCheckpointActive(GetWorld(), PlayerStartTag);
    Label->SetText(FText::FromString(bActive ? TEXT("ACTIVE") : TEXT("CHECKPOINT")));
    Label->SetTextRenderColor(bActive ? FColor::Green : FColor(0, 180, 255));
}

void AETCheckpointStart::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InitialProbeHandle);
    }
    if (IsValid(ActivationTrigger.Get()))
    {
        ActivationTrigger->OnComponentBeginOverlap.RemoveDynamic(
            this, &ThisClass::HandleTriggerBeginOverlap);
    }
    if (Session.IsValid())
    {
        Session->OnCheckpointChanged.Remove(CheckpointChangedHandle);
    }
    CheckpointChangedHandle.Reset();
    Session.Reset();
    UE_LOG(LogETCheckpointStart, Display,
        TEXT("[M1-06C] Checkpoint end Actor=%s"), *GetNameSafe(this));
    Super::EndPlay(EndPlayReason);
}