#include "Interaction/ETRelicAltarSubsystem.h"
#include "Interaction/ETRelicAltar.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Kismet/GameplayStatics.h"

bool UETRelicAltarSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UETRelicAltarSubsystem::IsEligiblePlayer(const AEmberTrialCharacter* PlayerBody)
{
    if (!IsValid(PlayerBody) || PlayerBody->IsActorBeingDestroyed() ||
        !PlayerBody->HasActorBegunPlay() || !PlayerBody->HasAuthority() ||
        PlayerBody->GetCombatTeam() != EETCombatTeam::Player ||
        !PlayerBody->IsGameplayReady() || PlayerBody->IsCombatDead() ||
        !PlayerBody->IsFormalActionIdle())
    {
        return false;
    }
    const UWorld* World = PlayerBody->GetWorld();
    const APlayerController* PC = Cast<APlayerController>(PlayerBody->GetController());
    const UETRelicEquipmentComponent* Equipment =
        PlayerBody->FindComponentByClass<UETRelicEquipmentComponent>();
    return World && World->GetNetMode() == NM_Standalone &&
        !UGameplayStatics::IsGamePaused(PlayerBody) && IsValid(PC) &&
        PC->IsLocalController() && PC->GetPawn() == PlayerBody &&
        Equipment && Equipment->IsEquipmentReady();
}

void UETRelicAltarSubsystem::RegisterAltar(AETRelicAltar* Altar)
{
    if (!bClosing && IsValid(Altar) && Altar->GetWorld() == GetWorld() && Altar->HasActorBegunPlay())
    {
        Altars.Add(TWeakObjectPtr<AETRelicAltar>(Altar));
    }
}

void UETRelicAltarSubsystem::UnregisterAltar(AETRelicAltar* Altar)
{
    Altars.Remove(TWeakObjectPtr<AETRelicAltar>(Altar));
}

AETRelicAltar* UETRelicAltarSubsystem::FindAvailableAltar(AEmberTrialCharacter* PlayerBody) const
{
    if (bClosing || !IsEligiblePlayer(PlayerBody) || PlayerBody->GetWorld() != GetWorld())
    {
        return nullptr;
    }
    AETRelicAltar* Best = nullptr;
    double BestDistanceSquared = TNumericLimits<double>::Max();
    for (const TWeakObjectPtr<AETRelicAltar>& Candidate : Altars)
    {
        AETRelicAltar* Altar = Candidate.Get();
        if (!IsValid(Altar) || !Altar->IsInReach(PlayerBody))
        {
            continue;
        }
        const double DistanceSquared = FVector::DistSquared2D(
            PlayerBody->GetActorLocation(), Altar->GetActorLocation());
        // 完全同距时按对象路径破同分，不依赖TSet遍历顺序。
        if (!Best || DistanceSquared < BestDistanceSquared ||
            (DistanceSquared == BestDistanceSquared && Altar->GetPathName() < Best->GetPathName()))
        {
            Best = Altar;
            BestDistanceSquared = DistanceSquared;
        }
    }
    return Best;
}

void UETRelicAltarSubsystem::Deinitialize()
{
    bClosing = true;
    Altars.Reset();
    Super::Deinitialize();
}