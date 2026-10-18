#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHitReact, Log, All);

// ---- 默认取消优先级与激活门控 ----

UETGA_HitReact::UETGA_HitReact()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;

    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_HitReact);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Attack_Light);
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Dodge);
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Heal);
}

bool UETGA_HitReact::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayTagContainer* SourceTags,
                                        const FGameplayTagContainer* TargetTags,
                                        FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !HasValidHealingCancelPolicy() ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    AEmberTrialCharacter* Character = Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (!IsValid(Character) || Character->IsCombatDead() || !Character->HasAuthority() ||
        ActorInfo->OwnerActor.Get() != Character ||
        Character->GetAbilitySystemComponent() != ActorInfo->AbilitySystemComponent.Get() ||
        !Character->GetGameplayHealthSnapshot(Health, MaxHealth) || !FMath::IsFinite(Health) ||
        !FMath::IsFinite(MaxHealth) || MaxHealth < 1.0f || Health <= 0.0f || Health > MaxHealth ||
        UGameplayStatics::IsGamePaused(Character))
    {
        return false;
    }
    return IsValid(HitReactMontage) && IsValid(ActorInfo->GetAnimInstance());
}

// ---- 启动：确认GAS已取消互斥动作，再清理旧Practice与输入 ----

void UETGA_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo,
                                     const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    AEmberTrialCharacter* Character =
        ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    if (!IsValid(Character) || Character->IsCombatDead() || !IsValid(ASC) ||
        !IsValid(HitReactMontage) || !CommitAbility(Handle, ActorInfo, ActivationInfo) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing))
    {
        // PreActivate取消可能仍被ScopeLock延期；不能和未结束的攻击 / 闪避 / 治疗叠播。
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }

    if (UETPracticeActionComponent* Practice =
            Character->FindComponentByClass<UETPracticeActionComponent>())
    {
        Practice->CancelPracticeAction();
    }
    Character->StopJumping();
    Character->ConsumeMovementInputVector();
    if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }

    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, HitReactMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!MontageTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageAborted);
    UE_LOG(LogETHitReact, Display, TEXT("[M1-04C1] Hit begin Character=%s Montage=%s"),
           *GetNameSafe(Character), *GetNameSafe(HitReactMontage.Get()));
    MontageTask->ReadyForActivation();
}

// ---- 动画回调与结束；不在父类结束后重置可能已属新激活的成员 ----

void UETGA_HitReact::HandleMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_HitReact::HandleMontageAborted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
}

void UETGA_HitReact::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                const FGameplayAbilityActorInfo* ActorInfo,
                                const FGameplayAbilityActivationInfo ActivationInfo,
                                bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo))
    {
        return;
    }
    // 延期结束交由GAS排队，真正执行时再清理Task与Tag。
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }
    const FString CharacterName = GetNameSafe(ActorInfo->AvatarActor.Get());
    const TWeakObjectPtr<UAbilitySystemComponent> ASC = ActorInfo->AbilitySystemComponent;
    MontageTask = nullptr;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogETHitReact, Display, TEXT("[M1-04C1] Hit end Character=%s Cancelled=%d HitTag=%d"),
           *CharacterName, bWasCancelled ? 1 : 0,
           ASC.IsValid() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact) ? 1 : 0);
}

bool UETGA_HitReact::HasValidHealingCancelPolicy() const
{
    return CancelAbilitiesWithTag.HasTagExact(ETGameplayTags::Ability_Heal) &&
           !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Healing);
}