#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "ETCombatLifeComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETGA_HitReact;
struct FOnAttributeChangeData;

UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETCombatLifeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETCombatLifeComponent();

    bool RequestHitReact();
    bool IsDead() const { return bDead; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Combat Life")
    TSubclassOf<UETGA_HitReact> HitReactAbilityClass;

    TWeakObjectPtr<AEmberTrialCharacter> CharacterOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle HealthHandle;
    FGameplayAbilitySpecHandle HitReactHandle;
    bool bEndingPlay = false;

    bool bDead = false;
    bool bOwnsDeadTag = false;
    TWeakObjectPtr<UAbilitySystemComponent> DeadTagASC;

    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    void StartObserving();
    void StopObserving();
    void HandleHealthChanged(const FOnAttributeChangeData& Change);
    bool RequestHitReactInternal(const TCHAR* Reason);
    void CommitDeath(const TCHAR* Reason);
    void ClearOwnedDeadTag();
};