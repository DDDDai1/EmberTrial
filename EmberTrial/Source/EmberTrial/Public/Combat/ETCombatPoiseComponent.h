#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "ETCombatPoiseComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETGA_Stagger;

// 每身体一份反应入口；本批只拥有Spec，下一批再观察Poise归零。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETCombatPoiseComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETCombatPoiseComponent();
    bool RequestStagger();
    bool HasValidConfiguration() const;
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETStaggerAbilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Poise")
    TSubclassOf<UETGA_Stagger> StaggerAbilityClass;

    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FGameplayAbilitySpecHandle StaggerHandle;
    bool bEndingPlay = false;

    bool ReadBody(AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC) const;
    void StartObserving();
    void StopObserving();

    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* Body, bool bReady);
};