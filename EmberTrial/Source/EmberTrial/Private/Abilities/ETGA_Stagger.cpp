#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogETStagger, Log, All);

// ---- GAS统一拥有Tag、取消与激活阻断；不手工复制一份Stunned布尔值 ----
UETGA_Stagger::UETGA_Stagger()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Stagger);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Stunned);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Stunned);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);

    const FGameplayTag Actions[] = {ETGameplayTags::Ability_Attack_Light,
                                    ETGameplayTags::Ability_Dodge, ETGameplayTags::Ability_Heal,
                                    ETGameplayTags::Ability_HitReact};
    for (const FGameplayTag& Tag : Actions)
    {
        CancelAbilitiesWithTag.AddTag(Tag);
        BlockAbilitiesWithTag.AddTag(Tag);
    }
}

bool UETGA_Stagger::HasValidConfiguration() const
{
    if (!IsValid(StaggerMontage) || !FMath::IsFinite(StaggerMontage->GetPlayLength()) ||
        StaggerMontage->GetPlayLength() <= 0.0f || !FMath::IsFinite(StaggerDuration) || StaggerDuration < .05f || StaggerDuration > 10.0f ||
        InstancingPolicy != EGameplayAbilityInstancingPolicy::InstancedPerActor ||
        NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::ServerOnly ||
        ReplicationPolicy != EGameplayAbilityReplicationPolicy::ReplicateNo ||
        bRetriggerInstancedAbility || GetCostGameplayEffect() || GetCooldownGameplayEffect() ||
        GetAssetTags().Num() != 1 || !GetAssetTags().HasTagExact(ETGameplayTags::Ability_Stagger) ||
        ActivationOwnedTags.Num() != 1 ||
        !ActivationOwnedTags.HasTagExact(ETGameplayTags::State_Stunned) ||
        ActivationBlockedTags.Num() != 2 ||
        !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Stunned) ||
        !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dead) ||
        CancelAbilitiesWithTag.Num() != 4 || BlockAbilitiesWithTag.Num() != 4)
    {
        return false;
    }
    const FGameplayTag Actions[] = {ETGameplayTags::Ability_Attack_Light,
                                    ETGameplayTags::Ability_Dodge, ETGameplayTags::Ability_Heal,
                                    ETGameplayTags::Ability_HitReact};
    for (const FGameplayTag& Tag : Actions)
    {
        if (!CancelAbilitiesWithTag.HasTagExact(Tag) || !BlockAbilitiesWithTag.HasTagExact(Tag))
        {
            return false;
        }
    }
    return true;
}

bool UETGA_Stagger::ReadContext(const FGameplayAbilityActorInfo* ActorInfo,
                                AEmberTrialCharacter*& Body, UAbilitySystemComponent*& ASC) const
{
    Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    float Health = 0.0f;
    float MaxHealth = 0.0f;
    return IsValid(Body) && Body->HasAuthority() && !Body->IsCombatDead() && IsValid(ASC) &&
           ActorInfo->OwnerActor.Get() == Body && Body->GetAbilitySystemComponent() == ASC &&
           !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead) &&
           Body->GetGameplayHealthSnapshot(Health, MaxHealth) && FMath::IsFinite(Health) &&
           FMath::IsFinite(MaxHealth) && MaxHealth >= 1.0f && Health > 0.0f && Health <= MaxHealth;
}

bool UETGA_Stagger::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayTagContainer* SourceTags,
                                       const FGameplayTagContainer* TargetTags,
                                       FGameplayTagContainer* OptionalRelevantTags) const
{
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    return HasValidConfiguration() && ReadContext(ActorInfo, Body, ASC) &&
           !UGameplayStatics::IsGamePaused(Body) &&
           Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags,
                                     OptionalRelevantTags);
}

// ---- 接管旧动作后只设一次世界时间Timer；不禁用移动模式，不接管P / 镜头 ----
void UETGA_Stagger::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    bReactionStarted = false;
    AEmberTrialCharacter* Body = nullptr;
    UAbilitySystemComponent* ASC = nullptr;
    if (!ReadContext(ActorInfo, Body, ASC) || !HasValidConfiguration() ||
        !CommitAbility(Handle, ActorInfo, ActivationInfo) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact))
    {
        // ScopeLock仍延期旧动作取消时拒绝叠加，归零事件批次再统一延后派发。
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    if (UETPracticeActionComponent* Practice =
            Body->FindComponentByClass<UETPracticeActionComponent>())
    {
        Practice->CancelPracticeAction();
    }
    if (!IsActive())
    {
        return;
    }
    if (!ReadContext(ActorInfo, Body, ASC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    if (AController* Controller = Body->GetController())
    {
        Controller->StopMovement();
    }
    if (!IsActive())
    {
        return;
    }
    if (!ReadContext(ActorInfo, Body, ASC))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    if (UCharacterMovementComponent* Movement = Body->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }
    UWorld* World = Body->GetWorld();
    if (!IsValid(World))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, StaggerMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!MontageTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    bReactionStarted = true;
    TimerWorld = World;
    const uint64 Serial = ++ActivationSerial;
    World->GetTimerManager().SetTimer(
        DurationTimer,
        FTimerDelegate::CreateUObject(this, &ThisClass::HandleDurationFinished, Serial),
        StaggerDuration, false);
    UE_LOG(LogETStagger, Display, TEXT("[M2-05B1] Stagger begin Body=%s Duration=%.2f"),
           *GetNameSafe(Body), StaggerDuration);
    // 不把Completed / Interrupted绑定到EndAbility：短动画播放完，逻辑仍持续1.2秒。
    // Task通过GAS活动任务列表托管；能力结束时只停自己的Montage。
    MontageTask->ReadyForActivation();
}

void UETGA_Stagger::HandleDurationFinished(uint64 Serial)
{
    if (!IsActive() || Serial != ActivationSerial)
    {
        return;
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UETGA_Stagger::EndAbility(const FGameplayAbilitySpecHandle Handle,
                               const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo,
                               bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle, ActorInfo))
    {
        return;
    }
    if (ScopeLockCount > 0)
    {
        Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
        return;
    }
    const FString BodyName = GetNameSafe(ActorInfo->AvatarActor.Get());
    const TWeakObjectPtr<UAbilitySystemComponent> ASC = ActorInfo->AbilitySystemComponent;
    ++ActivationSerial;
    if (UWorld* World = TimerWorld.Get())
    {
        World->GetTimerManager().ClearTimer(DurationTimer);
    }
    DurationTimer.Invalidate();
    TimerWorld.Reset();
    MontageTask = nullptr;
    // 先作废回调 / 清Timer，再由GAS撤销自己的Tag计数和Block关系。
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
    UE_LOG(LogETStagger, Display, TEXT("[M2-05B1] Stagger end Body=%s Cancelled=%d Stunned=%d"),
           *BodyName, bWasCancelled ? 1 : 0,
           ASC.IsValid() ? ASC->GetTagCount(ETGameplayTags::State_Stunned) : 0);
}