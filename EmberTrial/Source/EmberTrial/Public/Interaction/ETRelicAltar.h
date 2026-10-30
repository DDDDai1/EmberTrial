#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETRelicAltar.generated.h"

class AEmberTrialCharacter;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// 祭坛提供空间条件；库存、页面和暂停各由原有服务负责。
UCLASS()
class EMBERTRIAL_API AETRelicAltar : public AActor
{
    GENERATED_BODY()

public:
    AETRelicAltar();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Relic Altar")
    bool CanUseAltar(AEmberTrialCharacter* PlayerBody) const;

    // 空间只读查询不检查暂停；后续页面必须另核验身份 / 权限，不能凭它直接写装备。
    bool IsInReach(const AEmberTrialCharacter* PlayerBody) const;

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETRelicAltarTest;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> AltarRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USphereComponent> RangePreview;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Marker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Altar",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar",
              meta = (ClampMin = "1.0", Units = "cm"))
    float InteractionRadius = 180.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar",
              meta = (ClampMin = "0.0", Units = "cm"))
    float MaxVerticalDistance = 180.0f;

    UPROPERTY(EditAnywhere, Category = "EmberTrial|Altar")
    FVector SightPointOffset = FVector(0.0f, 0.0f, 80.0f);

    bool bEndingPlay = false;
};