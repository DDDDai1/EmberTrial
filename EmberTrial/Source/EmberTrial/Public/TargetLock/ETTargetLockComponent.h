#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ETTargetLockComponent.generated.h"

class APawn;
class APlayerController;
class AEmberTrialCharacter;
class UETCombatLifeComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FETLockTargetChanged, AEmberTrialCharacter*,
                                     AEmberTrialCharacter*);

// Controller拥有一份目标身份；不转镜头、不改朝向、不替能力结算伤害。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETTargetLockComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETTargetLockComponent();

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Target Lock")
    bool ToggleTargetLock();

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Target Lock")
    AEmberTrialCharacter* GetLockedTarget() const;

    FETLockTargetChanged OnTargetChanged;
    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETTargetLockSelectionTest;

    // 搜索与脱锁半径形成迟滞；视角评分只影响首次选择，不在维持时抢换目标。
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "1.0", Units = "cm"))
    float AcquireRadius = 1200.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "1.0", Units = "cm"))
    float BreakRadius = 1500.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "1.0", ClampMax = "89.0", Units = "deg"))
    float AcquireHalfAngle = 60.0f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ViewWeight = .75f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.02", ClampMax = "1.0", Units = "s"))
    float MonitorInterval = .10f;
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Target Lock",
              meta = (ClampMin = "0.0", ClampMax = "3.0", Units = "s"))
    float OcclusionGrace = .35f;

    TWeakObjectPtr<APlayerController> BoundController;
    TWeakObjectPtr<AEmberTrialCharacter> BoundSource;
    TWeakObjectPtr<AEmberTrialCharacter> LockedTarget;
    TWeakObjectPtr<UETCombatLifeComponent> SourceLife;
    TWeakObjectPtr<UETCombatLifeComponent> TargetLife;
    FDelegateHandle SourceDeathHandle;
    FDelegateHandle TargetDeathHandle;
    FTimerHandle MonitorTimer;
    double OccludedSince = -1.0;
    uint64 LockSerial = 0;
    bool bEndingPlay = false;

    bool HasValidConfiguration() const;
    AEmberTrialCharacter* ResolveSource() const;
    bool IsTargetEligible(const AEmberTrialCharacter* Source, const AEmberTrialCharacter* Target,
                          float Radius) const;
    bool HasVisibility(const AEmberTrialCharacter* Source, const AEmberTrialCharacter* Target,
                       const FVector& ViewLocation) const;
    AEmberTrialCharacter* FindBestTarget(const AEmberTrialCharacter* Source,
                                         const FVector& ViewLocation,
                                         const FVector& ViewForward) const;

    void RebindSource();
    void ClearSourceBindings();
    void SetLockedTarget(AEmberTrialCharacter* Target);
    void ClearTarget(const TCHAR* Reason);
    void MaintainLock(uint64 Serial);
    void HandleDeathSealed(UETCombatLifeComponent* Life);
    UFUNCTION()
    void HandlePossessedPawnChanged(APawn* PreviousPawn, APawn* CurrentPawn);
    UFUNCTION()
    void HandleBodyReadyChanged(AEmberTrialCharacter* Body, bool bReady);
    UFUNCTION()
    void HandleBodyEndPlay(AActor* Actor, EEndPlayReason::Type Reason);
};