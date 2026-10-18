#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "ETCombatLifeComponent.generated.h"

class AEmberTrialCharacter;
class UAbilitySystemComponent;
class UETGA_HitReact;
class UETCombatLifeComponent;
struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FETCombatDeathSealed, UETCombatLifeComponent*);

// 每身体一份：观察真实生命、请求受击，并在死亡时完成一次行动封口。
UCLASS(ClassGroup = (EmberTrial), meta = (BlueprintSpawnableComponent))
class EMBERTRIAL_API UETCombatLifeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UETCombatLifeComponent();

    bool RequestHitReact();
    bool IsDead() const
    {
        return bDead;
    }

    // Trace / 能力 / 移动 / 动画封锁完成后才通知失败流程，EndPlay先清空监听。
    FETCombatDeathSealed OnDeathSealed;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    friend class FETDodgeInvulnerabilityTest;
    friend class FETLightComboFlowTest;
    friend class FETLightComboCancelTest;
    friend class FETHealingAbilityTest;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Combat Life")
    TSubclassOf<UETGA_HitReact> HitReactAbilityClass;

    // 观察归属与本组件授予的受击能力；撤销Ready只释放这组资源。
    TWeakObjectPtr<AEmberTrialCharacter> CharacterOwner;
    TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
    FDelegateHandle HealthHandle;
    FGameplayAbilitySpecHandle HitReactHandle;
    bool bEndingPlay = false;

    // 死亡锁不随StopObserving清除，同一身体不会因重新Ready而复活。
    bool bDead = false;
    // 单独保存自己添加的一份Tag计数，避免BoundASC清空后失去释放对象。
    bool bOwnsDeadTag = false;
    TWeakObjectPtr<UAbilitySystemComponent> DeadTagASC;

    // 生命周期与属性事件入口；外部回调后仍需重新获取弱引用。
    UFUNCTION()
    void HandleReadyChanged(AEmberTrialCharacter* Character, bool bReady);

    void StartObserving();
    void StopObserving();
    void HandleHealthChanged(const FOnAttributeChangeData& Change);
    bool RequestHitReactInternal(const TCHAR* Reason);
    void CommitDeath(const TCHAR* Reason);
    void ClearOwnedDeadTag();
};