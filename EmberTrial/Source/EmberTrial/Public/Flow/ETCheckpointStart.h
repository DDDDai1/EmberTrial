#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "ETCheckpointStart.generated.h"

class AEmberTrialCharacter;
class UETRunSessionSubsystem;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// 可激活的PlayerStart；只把意图交给Session，不拥有复活 / 存档规则。
UCLASS()
class EMBERTRIAL_API AETCheckpointStart : public APlayerStart
{
    GENERATED_BODY()

public:
    AETCheckpointStart(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // ---- Overlap与一次NextTick补查；没有常驻Tick ----
    UFUNCTION()
    void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                   bool bFromSweep, const FHitResult& SweepResult);

    void ProbeInitialOverlap();
    void HandleCheckpointChanged(UWorld* ChangedWorld, FName CheckpointId);
    void RefreshDisplay();

    // ---- 既有组件名属于蓝图身份，碰撞 / 导航规则保持 ----
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Checkpoint",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USphereComponent> ActivationTrigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Checkpoint",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> Marker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Checkpoint",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTextRenderComponent> Label;

    // ---- 弱会话引用 / 自有委托与一次补查句柄，结束时主动清理 ----
    TWeakObjectPtr<UETRunSessionSubsystem> Session;
    FDelegateHandle CheckpointChangedHandle;
    FTimerHandle InitialProbeHandle;
    bool bEndingPlay = false;
};
