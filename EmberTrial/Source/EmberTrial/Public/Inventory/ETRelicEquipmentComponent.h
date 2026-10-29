#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "ETRelicEquipmentComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETRunSessionSubsystem;

enum class EETRelicEffectWriteResult : uint8
{
    Applied,
    Unchanged,
    Unavailable,
    Busy,
    InvalidModifiers
};

// 身体装备适配层：独占自身GE句柄；逻辑库存由GI持有。UI不能直接写ASC。
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

    // 基础GameplayReady不等于装备已恢复；正式就绪还要求当前控制与会话一致。
    bool IsEquipmentReady() const;
    void RefreshEquipmentState();

    // 调用方必须带看到的库存版本；正式祭坛权限由后续入口层接入。
    // 本章唯一交互调用者是受限PIE开发命令，不暴露BlueprintCallable。
    FETRelicInventoryReply TryEquip(EETRelicSlot Slot, FName RelicId, uint64 ExpectedRevision);


protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETRelicEquipmentOwnershipTest;
    friend class FETRelicEquipmentTransactionTest;

    bool IsCurrentPlayerBody() const;
    bool ResolveSessionSources();
    bool ObserveSession(UETRelicInventorySubsystem* Inventory, UETRunSessionSubsystem* Session);
    void StopObservingSession();
    void HandleInventoryChanged(const FETRelicInventorySnapshot& Snapshot);
    void HandleTravelFailed(UWorld* FailedWorld, const FText& Error);
    bool ReadPreviewBases(float& OutMaxHealth, float& OutAttackDamage) const;
    bool IsFinalDamageValid(const FETRelicModifierTotals& Modifiers) const;
    void WriteModifiersUnchecked(const FETRelicModifierTotals& Modifiers);


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

    // 只缓存本身体已同步的身份 / 版本；不拥有另一个库存或旧世界。
    TWeakObjectPtr<UETRelicInventorySubsystem> InventorySource;
    TWeakObjectPtr<UETRunSessionSubsystem> SessionSource;
    FDelegateHandle InventoryChangedHandle;
    FDelegateHandle TravelFailedHandle;
    FETRelicLoadout AppliedLoadout;
    uint64 AppliedRevision = 0;
    bool bInitialRestorePending = true;

    bool bChanging = false;
    bool bEndingPlay = false;
};