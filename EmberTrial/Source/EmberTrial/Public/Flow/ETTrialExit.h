#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETTrialExit.generated.h"

class AETEncounter;
class UBoxComponent;

UCLASS()
class EMBERTRIAL_API AETTrialExit : public AActor
{
    GENERATED_BODY()

public:
    AETTrialExit();

protected:
    virtual void BeginPlay() override;

private:
    friend class FETTrialVictoryTest;

    UPROPERTY(VisibleAnywhere, Category = "Trial Exit")
    TObjectPtr<UBoxComponent> ExitTrigger;

    UPROPERTY(EditInstanceOnly, Category = "Trial Exit")
    TArray<TObjectPtr<AETEncounter>> RequiredEncounters;

    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent,
                       int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);
};
