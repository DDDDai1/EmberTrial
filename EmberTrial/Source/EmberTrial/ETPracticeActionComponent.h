#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ETPracticeActionComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UAnimMontage;

UENUM(BlueprintType)
enum class EETPracticeActionState : uint8
{
    Idle,
    Attacking,
    Dodging,
    HitReact
};

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETPracticeActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETPracticeActionComponent();

    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

    /** One request per input Started event. Rejected requests are not queued. */
    UFUNCTION(BlueprintCallable, Category = "M0|Practice Action")
    bool RequestPracticeAction(EETPracticeActionState RequestedState);

    UFUNCTION(BlueprintCallable, Category = "M0|Practice Action")
    void CancelPracticeAction();

    UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
    EETPracticeActionState GetPracticeActionState() const;

    UFUNCTION(BlueprintPure, Category = "M0|Practice Action")
    bool IsPracticeActionIdle() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "M0|Practice Action|Animation",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> AttackMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "M0|Practice Action|Animation",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> DodgeMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "M0|Practice Action|Animation",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> HitReactMontage;

    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action", meta = (ClampMin = "0.0"))
    float AttackMaxHorizontalSpeed = 3.0f;

    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action|Dodge", meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "cm"))
    float DodgeDistance = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "M0|Practice Action|Dodge", meta = (ClampMin = "0.05", ClampMax = "0.50", Units = "s"))
    float DodgeDuration = 0.30f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient,
        Category = "M0|Practice Action", meta = (AllowPrivateAccess = "true"))
    EETPracticeActionState CurrentState = EETPracticeActionState::Idle;

    UPROPERTY(Transient)
    TWeakObjectPtr<ACharacter> CharacterOwner;

    UPROPERTY(Transient)
    TWeakObjectPtr<UAnimInstance> ActiveAnimInstance;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    int32 ActiveMontageInstanceID = INDEX_NONE;
    uint32 ActionSerial = 0;
    bool bEndingPlay = false;

    UAnimMontage* ResolveMontage(EETPracticeActionState State) const;
    void ReleaseAction(bool bStopMontage, const TCHAR* Reason);
    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, uint32 Serial);
    bool RejectRequest(EETPracticeActionState RequestedState, const TCHAR* Reason) const;

#if WITH_DEV_AUTOMATION_TESTS
    friend class FETPracticeActionLifecycleTest;
#endif
};
