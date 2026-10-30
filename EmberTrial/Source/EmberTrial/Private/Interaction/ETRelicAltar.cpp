#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarSubsystem.h"
#include "Characters/EmberTrialCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogETRelicAltar, Log, All);

AETRelicAltar::AETRelicAltar()
{
    PrimaryActorTick.bCanEverTick = false;
    AltarRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AltarRoot"));
    SetRootComponent(AltarRoot);

    RangePreview = CreateDefaultSubobject<USphereComponent>(TEXT("RangePreview"));
    RangePreview->SetupAttachment(AltarRoot);
    RangePreview->InitSphereRadius(InteractionRadius);
    RangePreview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RangePreview->SetGenerateOverlapEvents(false);
    RangePreview->SetCanEverAffectNavigation(false);
    RangePreview->SetHiddenInGame(true);

    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Marker->SetupAttachment(AltarRoot);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetGenerateOverlapEvents(false);
    Marker->SetCanEverAffectNavigation(false);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(AltarRoot);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    Label->SetText(FText::FromString(TEXT("RELIC ALTAR")));
    Label->SetTextRenderColor(FColor::Cyan);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(20.0f);
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label->SetCanEverAffectNavigation(false);
}

void AETRelicAltar::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    // 预览圆只辅助搭图；非法配置仍由真实查询拒绝，不靠编辑器Clamp保证正确性。
    RangePreview->SetSphereRadius(FMath::IsFinite(InteractionRadius)
        ? FMath::Max(0.0f, InteractionRadius) : 0.0f, false);
}

void AETRelicAltar::BeginPlay()
{
    Super::BeginPlay();
    if (UETRelicAltarSubsystem* Registry = GetWorld()->GetSubsystem<UETRelicAltarSubsystem>())
    {
        Registry->RegisterAltar(this);
    }
    UE_LOG(LogETRelicAltar, Display, TEXT("[M3-04A] Altar ready Actor=%s"), *GetName());
}

bool AETRelicAltar::CanUseAltar(AEmberTrialCharacter* PlayerBody) const
{
    return UETRelicAltarSubsystem::IsEligiblePlayer(PlayerBody) && IsInReach(PlayerBody);
}

bool AETRelicAltar::IsInReach(const AEmberTrialCharacter* PlayerBody) const
{
    if (bEndingPlay || !bEnabled || !HasActorBegunPlay() || IsActorBeingDestroyed() ||
        !IsValid(PlayerBody) || PlayerBody->IsActorBeingDestroyed() ||
        PlayerBody->GetWorld() != GetWorld() || !GetWorld() ||
        !FMath::IsFinite(InteractionRadius) || InteractionRadius <= 0.0f ||
        !FMath::IsFinite(MaxVerticalDistance) || MaxVerticalDistance < 0.0f ||
        SightPointOffset.ContainsNaN())
    {
        return false;
    }
    const FVector From = PlayerBody->GetActorLocation();
    const FVector Origin = GetActorLocation();
    const FVector To = GetActorTransform().TransformPosition(SightPointOffset);
    if (From.ContainsNaN() || Origin.ContainsNaN() || To.ContainsNaN() ||
        FVector::DistSquared2D(From, Origin) > FMath::Square(static_cast<double>(InteractionRadius)) ||
        FMath::Abs(From.Z - Origin.Z) > MaxVerticalDistance)
    {
        return false;
    }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETRelicAltarReach), false, PlayerBody);
    Query.AddIgnoredActor(this);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Query);
}

void AETRelicAltar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    if (UWorld* World = GetWorld())
    {
        if (UETRelicAltarSubsystem* Registry = World->GetSubsystem<UETRelicAltarSubsystem>())
        {
            Registry->UnregisterAltar(this);
        }
    }
    UE_LOG(LogETRelicAltar, Display, TEXT("[M3-04A] Altar end Actor=%s"), *GetName());
    Super::EndPlay(EndPlayReason);
}