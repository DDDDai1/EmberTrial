#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "ETCombatTraceComponent.generated.h"

class APawn;
class UAnimMontage;
class UAnimSequenceBase;
class UAnimNotifyState;
class USkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FETCombatTraceHitSignature, uint32, const FHitResult&);

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETCombatTraceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETCombatTraceComponent();

    bool CanStartAttack() const;
    uint32 BeginAttack(UAnimMontage* Montage, int32 MontageInstanceID);
    void EndAttack(uint32 Serial);

    void CancelActiveAttack();

    void SetAttackWindow(USkeletalMeshComponent* Mesh,
        UAnimSequenceBase* Animation, int32 MontageInstanceID,
        UAnimNotifyState* WindowToken, bool bOpen);

    FETCombatTraceHitSignature OnTraceHit;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace")
    FName TraceBoneName = TEXT("hand_r");

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace")
    FVector TraceLocalOffset = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace",
        meta = (ClampMin = "1.0", Units = "cm"))
    float TraceRadius = 25.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace",
        meta = (ClampMin = "1.0", Units = "cm"))
    float MaxSampleTravel = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Trace")
    bool bDrawDebug = true;

    TWeakObjectPtr<USkeletalMeshComponent> SourceMesh;
    TWeakObjectPtr<UAnimMontage> ExpectedMontage;
    TWeakObjectPtr<UAnimNotifyState> ActiveWindow;
    TSet<TWeakObjectPtr<APawn>> SeenTargets;

    uint32 SerialCounter = 0;
    uint32 ActiveSerial = 0;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    int32 WindowCount = 0;
    int32 SweepCount = 0;
    bool bWindowOpen = false;
    bool bHasPreviousPoint = false;
    bool bBlockedByWorld = false;
    FVector PreviousPoint = FVector::ZeroVector;

    bool IsExpectedMontageActive() const;
    void CloseWindow(const TCHAR* Reason);
    void BlockAttack(const FHitResult& Hit, const TCHAR* Reason);
};