#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Inventory/ETRelicLoadout.h"
#include "ETRelicEquipmentComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;

enum class EETRelicEffectWriteResult : uint8
{
    Applied,
    Unchanged,
    Unavailable,
    Busy,
    InvalidModifiers
};

// 身体侧自有GE适配层；C2-02再接库存事务 / 世界恢复，不允许Widget直接写效果。
UCLASS(ClassGroup = (EmberTrial))
class EMBERTRIAL_API UETRelicEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETRelicEquipmentComponent();
    virtual void Activate(bool bReset = false) override;
    virtual void Deactivate() override;

    // 只表示本身体的GE槽完整，不代表库存已恢复或当前允许祭坛换装。
    bool GetAppliedModifiers(FETRelicModifierTotals& OutModifiers) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETRelicEquipmentOwnershipTest;

    bool ReadBody(AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const;
    bool HasIntactOwnedEffect(UAbilitySystemComponent* ASC) const;
    bool EnsureOwnedEffect();
    void ReleaseOwnedEffect();

    // 仅效果写入原语，不提交库存ID；C2-02必须在领域事务内调用。
    EETRelicEffectWriteResult TrySetModifiers(const FETRelicModifierTotals& Modifiers);
    bool AreModifiersValid(UAbilitySystemComponent* ASC,
                            const FETRelicModifierTotals& Modifiers) const;

    UFUNCTION()
    void HandleGameplayReadyChanged(AEmberTrialCharacter* BodyCharacter, bool bReady);

    // 弱身体 / ASC与精确句柄；不保存GI库存、旧World或Widget。
    TWeakObjectPtr<AEmberTrialCharacter> BodyOwner;
    TWeakObjectPtr<UAbilitySystemComponent> EffectASC;
    FActiveGameplayEffectHandle OwnedEffect;
    FETRelicModifierTotals AppliedModifiers;
    bool bChanging = false;
    bool bEndingPlay = false;
};