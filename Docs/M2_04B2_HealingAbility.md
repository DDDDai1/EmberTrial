# M2-04B2：正式治疗能力、单次生效Notify与Q输入

2026-10-17，固定 **UE5.5.4**。前置[A资源事务](Reports/M2_04A_Verification.md)与[B1治疗素材](Reports/M2_04B1_Verification.md)均已验收；下发基线为105Source / 56cpp / 来源61。用户现已录入本批源码，实际110Source / 59cpp，首次Build在ETGA_Heal.cpp遇到C2280；按当次明确授权，助手已直接修复两处Tag列表并同步第3.2 / 9.1节，**待用户重新Build，B2尚未验收**。资产配置完成后来源预计63，不预记完成；助手未代执行Build / 自动测试或修改UE资产。

手动预算 **120–180分钟**，含理解录入 / 配置 / 10–15分钟相关检查 / 记录，构建等待另记。原路线100–160微调，原因是本批一次补齐11旧文件门控与可复用取消 / 旧回调检查；不是增加旧验收图。可分两次：先录第3–8节源码，第二次Build / 配置 / 短查。M2-04整章8h基础预算保留，A / B1已实际75 / 等待6分钟，预计时间不当作实耗。

## 1. 做完后的行为与唯一所有者

| 条件 / 输入 | 结果 |
| --- | --- |
| 地面站定、存活、非满血、有次数、未忙碌时新按Q | 开始AM_ET_Heal_01，获得State.Healing；按键时不回血 / 不扣次数 |
| 当前Montage到唯一ET Heal Commit点，默认15帧 / .5秒 | 再核对身体 / 控制权 / 健康 / 次数，调用A固定原生GE；成功回复40并消费1次 |
| 生效前3受击 / 真实伤害受击 / 死亡 / Stop取消 | 没有治疗、没有扣次数 |
| 已成功生效后再取消 | 已生效生命保留（后续实际伤害仍可扣），次数不退款 |
| 满血 / 0次 / 空中 / 水平速度大于3cm/s / 暂停 / 配置非法 | 拒绝，无动画、资源不变，不排队 |
| 长按Q / 忙碌连续Q / 治疗中左键或2 | 不重启、不缓存；结束后需要重新按 |
| 治疗期间移动 / 跳跃 / E | 原共享Idle门控阻断，镜头照常；体力恢复暂停，结束重新按原恢复规则判定 |
| 重复Notify / 上次Montage实例 / 已结束Task迟到回调 | 不重复提交，不影响新动作 |
| Retry / Continue重建身体 | 沿既有规则双100、治疗3/3，新实例 / 单HUD |

**GA**拥有本次动作、Montage Task、当前实例ID / 通知对象、一次提交标志与清理；**Notify**只转发实例事件，不保存角色状态、不改数值；**A资源组件**拥有次数和固定Health GE成功消费。UI只观察原生命属性，次数HUD留C，不在本批加Widget或轮询。

Q采用**原生Enhanced Input Started绑定**：IA / IMC只配资源，Character调用RequestHeal → ASC.TryActivateAbility。角色 / Controller / GA Event Graph均不新增治疗业务、Delay或临时测试函数；原左键 / 2 / E / P保持原入口。按住W时Q拒绝，站定后重新按Q；治疗不是可移动施法，也不自动取消攻击或闪避。

CommitAbility在有效Notify里调用，且**Cost Gameplay Effect Class / Cooldown Gameplay Effect Class必须None**。本批的次数成本不走标准按键成本GE；框架提交成功并复核同一次激活后，才由A组件一次治疗 / 一次扣数。同步回调可能取消动作，所以Commit日志使用捕获的Serial，End只记录CommitAttempted；End先出现、随后同Serial Commit Applied1在同步取消场景下合法，不代表第二次执行。成功事实以固定GE / 次数和Commit Applied结果判断，不根据最终净血量反推。

生命周期 / Tag含义参考[Epic UE5.5 Gameplay Ability](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine?application_version=5.5)。具体接口已按本机5.5.4 GameplayAbility / AbilityTask_PlayMontageAndWait / AnimMontage / SAnimNotifyPanel源代码核对；这是静态候选，**不等于已经编译或实测**。

## 2. 准备与准确文件清单

Stop PIE → Save All → **关闭UE** → CLion保存。录完第3–8节全部**五新 / 十一旧共十六文件**再第9节正常Build；反射头 / Tag / 输入与Ability变更不用Live Coding。不要只创建空类后提前Build。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Abilities\ETGA_Heal.h                              新，第3.1节全文
├─ EmberTrial\Private\Abilities\ETGA_Heal.cpp                             新，第3.2节全文
├─ EmberTrial\Public\Animation\ETAN_HealCommit.h                         新，第3.3节全文
├─ EmberTrial\Private\Animation\ETAN_HealCommit.cpp                        新，第3.4节全文
├─ EmberTrial\Tests\ETHealingAbilityTests.cpp                             新，第8节全文
├─ EmberTrial\Public\Abilities\ETGameplayTags.h                           旧，第4.1节
├─ EmberTrial\Private\Abilities\ETGameplayTags.cpp                         旧，第4.1节
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h                     旧，第5.1节
├─ EmberTrial\Private\Characters\EmberTrialCharacter.cpp                  旧，第5.2–5.3节
├─ EmberTrial\Private\Abilities\ETGA_LightAttack.cpp                       旧，第6.1节
├─ EmberTrial\Private\Abilities\ETGA_Dodge.cpp                             旧，第6.2节
├─ EmberTrial\Public\Abilities\ETGA_HitReact.h                             旧，第6.3节
├─ EmberTrial\Private\Abilities\ETGA_HitReact.cpp                          旧，第6.3节
├─ EmberTrial\Private\Resources\ETStaminaRecoveryComponent.cpp            旧，第6.4节
├─ EmberTrial\Public\Combat\ETCombatLifeComponent.h                       旧，第6.5节一行friend
└─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp       旧，第7节
```

新通知是**ETAN**（普通点Notify），不是ETANS状态窗；所有.generated.h必须为头文件最后一条include，不手建生成文件。旧函数 / friend / 反射属性保留。A的HealingResource / 原生GE、AttributeSet、UI、PracticeAction实现、Build.cs / Target / uproject不改；旧Practice通过共享Idle查询自动获得Healing阻断。

## 3. 五份新文件中的四份Runtime类

### 3.1 ETGA_Heal.h完整内容

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ETGA_Heal.generated.h"

class AEmberTrialCharacter;
class UAnimMontage;
class UAnimNotify;
class UAnimSequenceBase;
class USkeletalMeshComponent;
class UAbilityTask_PlayMontageAndWait;
class UAbilitySystemComponent;

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_Heal : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UETGA_Heal();
    bool HasValidConfiguration() const;
    bool HasValidTagPolicy() const;
    void HandleHealCommit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
        int32 MontageInstanceID, UAnimNotify* NotifyToken);

protected:
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

private:
    friend class FETHealingAbilityTest;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Healing")
    TObjectPtr<UAnimMontage> HealMontage;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Healing", meta=(ClampMin="1.0", ClampMax="1000.0"))
    float HealAmount = 40.0f;
    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    TWeakObjectPtr<UAnimNotify> ActiveCommitToken;
    uint32 ActivationSerial = 0;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    float ActiveHealAmount = 0.0f;
    float ActiveCommitTime = 0.0f;
    bool bCommitAttempted = false;
    bool bStarting = false;
    bool bEnding = false;

    bool ReadContext(const FGameplayAbilityActorInfo* ActorInfo,
        AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const;
    void ReleaseMontage();
    UFUNCTION()
    void HandleCompleted();
    UFUNCTION()
    void HandleAborted();
};
```

### 3.2 ETGA_Heal.cpp完整内容

```cpp
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/ETAN_HealCommit.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Resources/ETHealingResourceComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogETHeal, Log, All);

UETGA_Heal::UETGA_Heal()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;
    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Heal);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Healing);
    const FGameplayTag BlockedTags[] = {ETGameplayTags::State_Healing, ETGameplayTags::State_Attacking,
        ETGameplayTags::State_Dodging, ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
    for (const FGameplayTag& Tag : BlockedTags)
    {
        ActivationBlockedTags.AddTag(Tag);
    }
}

bool UETGA_Heal::HasValidTagPolicy() const
{
    if (GetAssetTags().Num()!=1 || !GetAssetTags().HasTagExact(ETGameplayTags::Ability_Heal) ||
        ActivationOwnedTags.Num()!=1 || !ActivationOwnedTags.HasTagExact(ETGameplayTags::State_Healing) ||
        CancelAbilitiesWithTag.Num()!=0 || BlockAbilitiesWithTag.Num()!=0 || ActivationBlockedTags.Num()!=5) { return false; }
    const FGameplayTag BlockedTags[] = {ETGameplayTags::State_Healing, ETGameplayTags::State_Attacking,
        ETGameplayTags::State_Dodging, ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead};
    for (const FGameplayTag& Tag : BlockedTags)
    {
        if (!ActivationBlockedTags.HasTagExact(Tag)) { return false; }
    }
    return true;
}

bool UETGA_Heal::HasValidConfiguration() const
{
    if (!HasValidTagPolicy() || GetCostGameplayEffect() || GetCooldownGameplayEffect() ||
        !FMath::IsFinite(HealAmount) || HealAmount<1.0f || HealAmount>1000.0f ||
        !IsValid(HealMontage) || !IsValid(HealMontage->GetSkeleton()) || HealMontage->HasRootMotion() ||
        !FMath::IsNearlyEqual(HealMontage->RateScale,1.0f) || !HealMontage->bEnableAutoBlendOut ||
        HealMontage->SlotAnimTracks.Num()!=1 ||
        HealMontage->SlotAnimTracks[0].SlotName!=FName(TEXT("DefaultSlot")) ||
        HealMontage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num()!=1 ||
        HealMontage->CompositeSections.Num()!=1 ||
        !HealMontage->CompositeSections[0].NextSectionName.IsNone() || HealMontage->Notifies.Num()!=1)
    { return false; }
    const FAnimSegment& Segment=HealMontage->SlotAnimTracks[0].AnimTrack.AnimSegments[0];
    const UAnimSequenceBase* Clip=Segment.GetAnimReference();
    const FAnimNotifyEvent& Event=HealMontage->Notifies[0];
    const float Length=HealMontage->GetPlayLength();
    const float FadeIn=HealMontage->BlendIn.GetBlendTime();
    const float FadeOut=HealMontage->BlendOut.GetBlendTime();
    return IsValid(Clip) && Clip->GetSkeleton()==HealMontage->GetSkeleton() &&
        Clip->Notifies.Num()==0 && FMath::IsNearlyEqual(Clip->RateScale,1.0f) &&
        Segment.LoopingCount==1 && FMath::IsNearlyZero(Segment.StartPos) &&
        FMath::IsNearlyZero(Segment.AnimStartTime) && FMath::IsNearlyEqual(Segment.AnimPlayRate,1.0f) &&
        FMath::IsNearlyEqual(Segment.AnimEndTime,Clip->GetPlayLength(),.001f) &&
        FMath::IsFinite(Length) && Length>0.2f && FMath::IsFinite(FadeIn) && FadeIn>=0.0f &&
        FMath::IsFinite(FadeOut) && FadeOut>=0.0f &&
        IsValid(Event.Notify) && Event.Notify->IsA<UETAN_HealCommit>() && !Event.NotifyStateClass &&
        Event.IsBranchingPoint() && FMath::IsNearlyEqual(Event.NotifyTriggerChance,1.0f) &&
        Event.NotifyFilterType==ENotifyFilterType::NoFiltering && FMath::IsFinite(Event.GetTime()) &&
        Event.GetTime()>FMath::Max(.05f,FadeIn) && Event.GetTime()<Length-FadeOut;
}

bool UETGA_Heal::ReadContext(const FGameplayAbilityActorInfo* ActorInfo,
    AEmberTrialCharacter*& OutBody, UAbilitySystemComponent*& OutASC) const
{
    OutBody=ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    OutASC=ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    APlayerController* PC=IsValid(OutBody) ? Cast<APlayerController>(OutBody->GetController()) : nullptr;
    const UCharacterMovementComponent* Move=IsValid(OutBody) ? OutBody->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice=IsValid(OutBody)
        ? OutBody->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    return IsValid(OutBody) && !OutBody->IsActorBeingDestroyed() && OutBody->HasAuthority() &&
        OutBody->IsGameplayReady() && !OutBody->IsCombatDead() && OutBody->GetCombatTeam()==EETCombatTeam::Player &&
        ActorInfo->OwnerActor.Get()==OutBody && IsValid(OutASC) && OutBody->GetAbilitySystemComponent()==OutASC &&
        OutBody->GetWorld() && OutBody->GetWorld()->GetNetMode()==NM_Standalone && !OutBody->GetWorld()->IsPaused() &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn()==OutBody && !PC->IsMoveInputIgnored() &&
        IsValid(Move) && Move->IsMovingOnGround() && !Move->HasRootMotionSources() &&
        !OutBody->GetVelocity().ContainsNaN() && OutBody->GetVelocity().SizeSquared2D()<=9.0 &&
        (!Practice || Practice->IsPracticeActionIdle()) && IsValid(ActorInfo->GetAnimInstance());
}

bool UETGA_Heal::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    AEmberTrialCharacter* Body=nullptr;UAbilitySystemComponent* ASC=nullptr;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo,Body,ASC) || !Body->IsFormalActionIdle() ||
        !Super::CanActivateAbility(Handle,ActorInfo,SourceTags,TargetTags,OptionalRelevantTags)) { return false; }
    const UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
    UAnimInstance* Anim=ActorInfo->GetAnimInstance();
    return IsValid(Resource) && Resource->CanApplyHealing(HealAmount) &&
        !Anim->GetCurrentActiveMontage() && Anim->GetSkelMeshComponent()->GetSkeletalMeshAsset() &&
        Anim->GetSkelMeshComponent()->GetSkeletalMeshAsset()->GetSkeleton()==HealMontage->GetSkeleton();
}

void UETGA_Heal::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    if (!IsActive()) { return; }
    ++ActivationSerial;if (ActivationSerial==0) { ++ActivationSerial; }
    const uint32 Serial=ActivationSerial;
    bEnding=false;bStarting=true;bCommitAttempted=false;
    AEmberTrialCharacter* Body=nullptr;UAbilitySystemComponent* ASC=nullptr;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo,Body,ASC))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    if (ActorInfo->GetAnimInstance()->GetCurrentActiveMontage() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    const UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
    if (!IsValid(Resource) || !Resource->CanApplyHealing(HealAmount))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    ActiveMontage=HealMontage;ActiveHealAmount=HealAmount;
    ActiveCommitToken=ActiveMontage->Notifies[0].Notify;
    ActiveCommitTime=ActiveMontage->Notifies[0].GetTime();
    Body->StopJumping();Body->ConsumeMovementInputVector();
    Body->GetCharacterMovement()->StopMovementImmediately();
    UAbilityTask_PlayMontageAndWait* Task=UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this,NAME_None,ActiveMontage,1.0f,NAME_None,true,1.0f,0.0f,true);
    if (!Task) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    MontageTask=Task;
    Task->OnCompleted.AddDynamic(this,&ThisClass::HandleCompleted);
    Task->OnInterrupted.AddDynamic(this,&ThisClass::HandleAborted);
    Task->OnCancelled.AddDynamic(this,&ThisClass::HandleAborted);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC=ASC;
    const TWeakObjectPtr<UAnimInstance> WeakAnim=ActorInfo->GetAnimInstance();
    UAnimMontage* StartingMontage=ActiveMontage;
    Task->ReadyForActivation();
    ASC=WeakASC.Get();UAnimInstance* Anim=WeakAnim.Get();
    if (!IsActive() || Serial!=ActivationSerial)
    {
        // PlayMontage may assign ownership after a synchronous cancellation.
        if (!IsActive() && IsValid(ASC) && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==StartingMontage)
        {
            ASC->CurrentMontageStop(0.0f);
            if (!IsActive() && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==StartingMontage)
            { ASC->ClearAnimatingAbility(this); }
        }
        return;
    }
    FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    if (!IsValid(ASC) || !Instance || !Anim->Montage_IsPlaying(ActiveMontage) ||
        ASC->GetAnimatingAbility()!=this || MontageTask!=Task)
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    ActiveMontageInstanceID=Instance->GetInstanceID();bStarting=false;
    UE_LOG(LogETHeal,Display,TEXT("[M2-04B2] Begin Body=%s Serial=%u MontageID=%d CommitAt=%.3f Amount=%.1f"),
        *GetNameSafe(Body),Serial,ActiveMontageInstanceID,ActiveCommitTime,ActiveHealAmount);
}

void UETGA_Heal::HandleHealCommit(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
    int32 MontageInstanceID, UAnimNotify* NotifyToken)
{
    if (!IsActive() || bStarting || bEnding || bCommitAttempted || !CurrentActorInfo ||
        Animation!=ActiveMontage || MontageInstanceID!=ActiveMontageInstanceID ||
        !IsValid(NotifyToken) || ActiveCommitToken.Get()!=NotifyToken) { return; }
    AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetMontageInstanceForID(MontageInstanceID) : nullptr;
    if (!IsValid(Body) || Mesh!=Body->GetMesh() || !Instance || !Instance->IsActive() ||
        Instance->Montage!=ActiveMontage || Instance->GetPosition()+KINDA_SMALL_NUMBER<ActiveCommitTime ||
        !Body->GetWorld() || Body->GetWorld()->IsPaused()) { return; }
    // Seal the attempt before any framework / attribute callbacks can re-enter.
    bCommitAttempted=true;
    const uint32 Serial=ActivationSerial;const FString BodyName=GetNameSafe(Body);
    UAbilitySystemComponent* ASC=nullptr;
    if (!ReadContext(CurrentActorInfo,Body,ASC) || ASC->GetAnimatingAbility()!=this ||
        ASC->GetCurrentMontage()!=ActiveMontage)
    { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);return; }
    const bool bFrameworkCommitted=CommitAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo);
    if (!IsActive() || Serial!=ActivationSerial || bEnding) { return; }
    if (!bFrameworkCommitted)
    { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);return; }
    if (!ReadContext(CurrentActorInfo,Body,ASC))
    { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);return; }
    UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
    const bool bApplied=IsValid(Resource) && Resource->TryApplyHealing(ActiveHealAmount);
    // Log the captured attempt even if the successful GE synchronously ended this ability.
    UE_LOG(LogETHeal,Display,TEXT("[M2-04B2] Commit Body=%s Serial=%u Applied=%d"),*BodyName,Serial,bApplied ? 1 : 0);
    if (!IsActive() || Serial!=ActivationSerial || bEnding) { return; }
    if (!bApplied) { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true); }
}

void UETGA_Heal::ReleaseMontage()
{
    const uint32 Serial=ActivationSerial;
    UAbilityTask_PlayMontageAndWait* OldTask=MontageTask;
    UAnimMontage* OldMontage=ActiveMontage;const int32 OldID=ActiveMontageInstanceID;
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC=CurrentActorInfo
        ? CurrentActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    const TWeakObjectPtr<UAnimInstance> WeakAnim=CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
    UAnimInstance* Anim=WeakAnim.Get();
    FAnimMontageInstance* Instance=IsValid(Anim) && OldID!=INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    MontageTask=nullptr;ActiveMontage=nullptr;ActiveMontageInstanceID=INDEX_NONE;ActiveCommitToken.Reset();
    ActiveHealAmount=0.0f;ActiveCommitTime=0.0f;
    if (IsValid(OldTask))
    {
        OldTask->OnCompleted.RemoveAll(this);OldTask->OnInterrupted.RemoveAll(this);OldTask->OnCancelled.RemoveAll(this);
        if (Instance)
        {
            if (Instance->OnMontageBlendedInEnded.IsBoundToObject(OldTask)) { Instance->OnMontageBlendedInEnded.Unbind(); }
            if (Instance->OnMontageBlendingOutStarted.IsBoundToObject(OldTask)) { Instance->OnMontageBlendingOutStarted.Unbind(); }
            if (Instance->OnMontageEnded.IsBoundToObject(OldTask)) { Instance->OnMontageEnded.Unbind(); }
        }
        OldTask->EndTask();OldTask->Ability=nullptr;
    }
    UAbilitySystemComponent* ASC=WeakASC.Get();Anim=WeakAnim.Get();
    Instance=IsValid(Anim) && OldID!=INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    if (Serial==ActivationSerial && IsValid(ASC) && IsValid(OldMontage) &&
        ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==OldMontage && (Instance || OldID==INDEX_NONE))
    {
        ASC->CurrentMontageStop(.10f);
        if (Serial==ActivationSerial && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==OldMontage)
        { ASC->ClearAnimatingAbility(this); }
    }
}

void UETGA_Heal::HandleCompleted()
{ if (IsActive() && !bEnding) { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,false); } }
void UETGA_Heal::HandleAborted()
{ if (IsActive() && !bEnding) { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true); } }
void UETGA_Heal::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility, bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle,ActorInfo) || bEnding) { return; }
    if (ScopeLockCount>0)
    { Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);return; }
    const uint32 EndedSerial=ActivationSerial;const bool bAttempted=bCommitAttempted;
    const FString BodyName=GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    const TWeakObjectPtr<UAbilitySystemComponent> ASC=ActorInfo ? ActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    bEnding=true;bStarting=false;++ActivationSerial;ReleaseMontage();
    Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);
    // No instance-field resets after Super: an End callback may have begun a new activation.
    UE_LOG(LogETHeal,Display,TEXT("[M2-04B2] End Body=%s Serial=%u Cancelled=%d CommitAttempted=%d HealingTag=%d"),
        *BodyName,EndedSerial,bWasCancelled ? 1 : 0,bAttempted ? 1 : 0,
        ASC.IsValid() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing) ? 1 : 0);
}
```

ReleaseMontage沿用现有连段的所有权顺序：先撤销自身活动引用 / 解绑自身委托，正常EndTask后才置旧Task.Ability=nullptr，最后只停属于自身旧实例的Montage。自然结束时实例可能已从动画系统移除，Super.EndAbility仍会通过ASC.NotifyAbilityEnded清自己的动画能力指针；新增自动化也检查结束后无旧所有权。不能在活动Task尚未结束时清Ability，也不能用Stop All Montages / 清所有GAS Tag代替局部清理。Super.EndAbility后不再重置实例字段，保护结束回调里新开始的激活。这个顺序已核对本机引擎源码，新治疗实现仍待本批Build与测试。

### 3.3 ETAN_HealCommit.h完整内容

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ETAN_HealCommit.generated.h"

UCLASS(meta=(DisplayName="ET Heal Commit"))
class EMBERTRIAL_API UETAN_HealCommit : public UAnimNotify
{
    GENERATED_BODY()
public:
    UETAN_HealCommit();
    virtual void BranchingPointNotify(FBranchingPointNotifyPayload& Payload) override;
};
```

### 3.4 ETAN_HealCommit.cpp完整内容

```cpp
#include "Animation/ETAN_HealCommit.h"
#include "Abilities/ETGA_Heal.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETAN_HealCommit::UETAN_HealCommit()
{
    bIsNativeBranchingPoint=true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor=false;NotifyColor=FColor::Green;
#endif
}
void UETAN_HealCommit::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
    USkeletalMeshComponent* Mesh=Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body=IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC=IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_Heal* Heal=IsValid(ASC) ? Cast<UETGA_Heal>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Heal)) { Heal->HandleHealCommit(Mesh,Payload.SequenceAsset,Payload.MontageInstanceID,this); }
}
```

通知对象属于资产、可能被多个角色共享；这里不存“是否已治疗”。一次提交状态只放GA实例，Notify转发Mesh / Montage / 实例ID / 自己作为Token。编辑器预览关闭业务触发，预览AM不会扣次数。

## 4. 两个Native Tag

### 4.1 ETGameplayTags.h / .cpp各一处

头文件namespace内，保留原Invulnerable行后新增两行：

**找到原内容：**

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);
```

**替换为：**

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invulnerable);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Heal);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Healing);
```

.cpp同一namespace内：

**找到原内容：**

```cpp
    UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "State.Invulnerable");
```

**替换为：**

```cpp
    UE_DEFINE_GAMEPLAY_TAG(State_Invulnerable, "State.Invulnerable");
    UE_DEFINE_GAMEPLAY_TAG(Ability_Heal, "Ability.Heal");
    UE_DEFINE_GAMEPLAY_TAG(State_Healing, "State.Healing");
```

不在Project Settings再手动注册同名Tag；保留所有旧Native Tag。

## 5. Character：授予、请求、原生输入与结束

### 5.1 EmberTrialCharacter.h四处

第一处，前置声明区：

**找到原内容：**

```cpp
class UETGA_Dodge;
```

**替换为：**

```cpp
class UETGA_Dodge;
class UETGA_Heal;
```

第二处，既有friend区，保留旧测试friend：

**找到原内容：**

```cpp
friend class FETHealingResourceTest;
```

**替换为：**

```cpp
friend class FETHealingResourceTest;
    friend class FETHealingAbilityTest;
```

第三处，private区既有GrantDodgeAbility声明后，加Class / Action / Handle及两个私有函数：

**找到原内容：**

```cpp
void GrantDodgeAbility();
```

**替换为：**

```cpp
void GrantDodgeAbility();

    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Healing")
    TSubclassOf<UETGA_Heal> HealAbilityClass;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Healing")
    TObjectPtr<UInputAction> HealAction;
    FGameplayAbilitySpecHandle HealAbilityHandle;
    void GrantHealAbility();
    void HandleHealStarted();
```

第四处，public区RequestDodge声明后，加新的BlueprintCallable请求。原RequestDodge前的UFUNCTION保留：

**找到原内容：**

```cpp
bool RequestDodge();
```

**替换为：**

```cpp
bool RequestDodge();

    UFUNCTION(BlueprintCallable, Category="EmberTrial|Healing")
    bool RequestHeal();
```

### 5.2 EmberTrialCharacter.cpp六处

第一处，include：

**找到原内容：**

```cpp
#include "Abilities/ETGA_Dodge.h"
```

**替换为：**

```cpp
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_Heal.h"
```

第二处，BeginPlay末尾已有GrantDodgeAbility调用：

**找到原内容：**

```cpp
    GrantDodgeAbility();
```

**替换为：**

```cpp
    GrantDodgeAbility();
    GrantHealAbility();
```

第三处，IsFormalActionIdle最后一条Dodging检查（分号改成继续与Healing检查）：

**找到原内容：**

```cpp
!AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Dodging);
```

**替换为：**

```cpp
!AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) &&
        !AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Healing);
```

第四处，EndPlay的RemoveAllGameplayCues前，追加清本次Heal Handle。前面的CancelAllAbilities保留：

**找到原内容：**

```cpp
        AbilitySystemComponent->RemoveAllGameplayCues();
```

**替换为：**

```cpp
        if (HealAbilityHandle.IsValid())
        {
            AbilitySystemComponent->ClearAbility(HealAbilityHandle);
        }
        AbilitySystemComponent->RemoveAllGameplayCues();
```

第五处，EndPlay末尾原Dodge Handle清空行：

**找到原内容：**

```cpp
    DodgeAbilityHandle = FGameplayAbilitySpecHandle();
```

**替换为：**

```cpp
    DodgeAbilityHandle = FGameplayAbilitySpecHandle();
    HealAbilityHandle = FGameplayAbilitySpecHandle();
```

第六处，SetupPlayerInputComponent的EnhancedInputComponent成功分支内，原Look绑定后：

**找到原内容：**

```cpp
EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEmberTrialCharacter::Look);
```

**替换为：**

```cpp
EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEmberTrialCharacter::Look);
        if (IsValid(HealAction))
        {
            EnhancedInputComponent->BindAction(HealAction, ETriggerEvent::Started,
                this, &AEmberTrialCharacter::HandleHealStarted);
        }
```

### 5.3 Character.cpp文件末尾追加三个完整函数

不要插进Look / Move / EndPlay函数内部；保持所有旧函数。

```cpp
void AEmberTrialCharacter::GrantHealAbility()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown || !HasAuthority() ||
        CombatTeam!=EETCombatTeam::Player || !HealAbilityClass || HealAbilityHandle.IsValid()) { return; }
    if (HealAbilityClass->HasAnyClassFlags(CLASS_Abstract))
    {
        UE_LOG(LogETAbilityFoundation,Warning,TEXT("[M2-04B2] Abstract Heal class Body=%s"),*GetNameSafe(this));return;
    }
    HealAbilityHandle=AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(HealAbilityClass.Get(),1,INDEX_NONE,this));
    UE_LOG(LogETAbilityFoundation,Display,TEXT("[M2-04B2] Heal granted Body=%s Valid=%d"),
        *GetNameSafe(this),HealAbilityHandle.IsValid() ? 1 : 0);
}

bool AEmberTrialCharacter::RequestHeal()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !IsValid(AbilitySystemComponent) || !HealAbilityHandle.IsValid()) { return false; }
    const FGameplayAbilitySpec* Spec=AbilitySystemComponent->FindAbilitySpecFromHandle(HealAbilityHandle);
    if (!Spec || Spec->IsActive()) { return false; }
    const bool bAccepted=AbilitySystemComponent->TryActivateAbility(HealAbilityHandle,false);
    UE_LOG(LogETAbilityFoundation,Display,TEXT("[M2-04B2] Heal request Body=%s Accepted=%d"),*GetNameSafe(this),bAccepted ? 1 : 0);
    return bAccepted;
}

void AEmberTrialCharacter::HandleHealStarted()
{
    RequestHeal();
}
```

HealAction只绑定Started，Triggered / Completed不做治疗业务；不能直接把返回bool的RequestHeal当成void输入回调，所以保留HandleHealStarted这层。Q没有C++键码硬绑定：实际键位由IMC配置，可继续扩为用户改键。

## 6. 共享门控与受击优先级

### 6.1 ETGA_LightAttack.cpp两处

构造函数Blocked加Healing：

**找到原内容：**

```cpp
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
```

**替换为：**

```cpp
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Healing);
```

CanActivateAbility末尾额外显式检查Healing，避免旧Blueprint标签默认值覆盖新原生值：

**找到原内容：**

```cpp
    return HasValidComboConfiguration() && IsValid(AttackMontage) && IsValid(AnimInstance) &&
```

**替换为：**

```cpp
    return !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(ETGameplayTags::State_Healing) &&
        HasValidComboConfiguration() && IsValid(AttackMontage) && IsValid(AnimInstance) &&
```

### 6.2 ETGA_Dodge.cpp两处

构造函数：

**找到原内容：**

```cpp
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
```

**替换为：**

```cpp
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Healing);
```

HasValidRecoveryCancelPolicy最后一条检查，新增Healing阻断；保留恢复取消攻击规则：

**找到原内容：**

```cpp
ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dead);
```

**替换为：**

```cpp
ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dead) &&
        ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Healing);
```

### 6.3 ETGA_HitReact.h / .cpp五处

头文件public构造声明后加策略查询：

**找到原内容：**

```cpp
    UETGA_HitReact();
```

**替换为：**

```cpp
    UETGA_HitReact();
    bool HasValidHealingCancelPolicy() const;
```

.cpp构造CancelAbilitiesWithTag加Heal：

**找到原内容：**

```cpp
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Dodge);
```

**替换为：**

```cpp
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Dodge);
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Heal);
```

.cpp CanActivateAbility开头：

**找到原内容：**

```cpp
    if (!ActorInfo || !Super::CanActivateAbility(
```

**替换为：**

```cpp
    if (!ActorInfo || !HasValidHealingCancelPolicy() || !Super::CanActivateAbility(
```

.cpp ActivateAbility中，原Attacking / Dodging未完全取消检查末尾加Healing：

**找到原内容：**

```cpp
ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging))
```

**替换为：**

```cpp
ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing))
```

.cpp文件末尾、现有最后函数完整结束之后追加：

```cpp
bool UETGA_HitReact::HasValidHealingCancelPolicy() const
{
    return CancelAbilitiesWithTag.HasTagExact(ETGameplayTags::Ability_Heal) &&
        !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Healing);
}
```

### 6.4 ETStaminaRecoveryComponent.cpp：同一片段必须替换两处

StopObserving和RefreshObservation各一组`const FGameplayTag Tags[]`。下面原片段共出现**两次**，两次都替换；只改订阅不改解绑会造成索引不匹配：

**找到原内容：**

```cpp
ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead, ETGameplayTags::State_Dodging};
```

**替换为：**

```cpp
ETGameplayTags::State_HitReact, ETGameplayTags::State_Dead, ETGameplayTags::State_Dodging,
            ETGameplayTags::State_Healing};
```

### 6.5 ETCombatLifeComponent.h：只加一行测试friend

**找到原内容：**

```cpp
    friend class FETLightComboCancelTest;
```

**替换为：**

```cpp
    friend class FETLightComboCancelTest;
    friend class FETHealingAbilityTest;
```

CombatLife死亡规则不改：既有死亡封锁 / CancelAllAbilities会取消治疗。3正式受击与真实负向伤害走既有HitReact；治疗成功的正向Health变化不会制造受击。敌人没有Heal Class，不增加AI施法逻辑。

## 7. Editor Validator精确扩展

只有`EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`，保留全部旧校验规则 / 名称，不替换整个文件。

### 7.1 include与Profile / 类型分发

include：

**找到原内容：**

```cpp
#include "Abilities/ETGA_Dodge.h"
```

**替换为：**

```cpp
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_Heal.h"
```

GetGameplayValidationParent的Profiles表新增正式GA：

**找到原内容：**

```cpp
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge"), UETGA_Dodge::StaticClass()},
```

**替换为：**

```cpp
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge"), UETGA_Dodge::StaticClass()},
            {TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal"), UETGA_Heal::StaticClass()},
```

ValidateBlueprint中Ability类型判断（替换整个条件片段，不改其后的调用）：

**找到原内容：**

```cpp
Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>() || Defaults->IsA<UETGA_Dodge>()
```

**替换为：**

```cpp
Defaults->IsA<UETGA_LightAttack>() || Defaults->IsA<UETGA_HitReact>() || Defaults->IsA<UETGA_Dodge>() || Defaults->IsA<UETGA_Heal>()
```

### 7.2 玩家资源引用与正式Ability规则

ValidateBody的if(bPlayer)内部，原DodgeAbilityClass那一行替换：

**找到原内容：**

```cpp
        RequireObject(Blueprint, Defaults, TEXT("DodgeAbilityClass"), TEXT("ET-DODGE-001"), Issues);
```

**替换为：**

```cpp
        RequireObject(Blueprint, Defaults, TEXT("HealAbilityClass"), TEXT("ET-HEAL-002"), Issues);
        const UInputAction* HealInput=Cast<UInputAction>(RequireObject(
            Blueprint, Defaults, TEXT("HealAction"), TEXT("ET-HEAL-004"), Issues));
        if (IsValid(HealInput) && (HealInput->GetPathName()!=TEXT("/Game/EmberTrial/Input/Actions/IA_ET_Heal.IA_ET_Heal") ||
            HealInput->ValueType!=EInputActionValueType::Boolean || HealInput->bTriggerWhenPaused || HealInput->Triggers.Num()!=0))
        {
            AddIssue(Blueprint,TEXT("ET-HEAL-004"),TEXT("HealAction must be IA_ET_Heal, Boolean, no Triggers, not triggered while paused."),Issues);
        }
        RequireObject(Blueprint, Defaults, TEXT("DodgeAbilityClass"), TEXT("ET-DODGE-001"), Issues);
```

ValidateAbility中，第一处`if (Defaults->IsA<UETGA_Dodge>())`之前插入Heal / HitReact规则。用下面整块替换该if开头一行，其后原Dodge大括号内容保留：

**找到原内容：**

```cpp
    if (Defaults->IsA<UETGA_Dodge>())
```

**替换为：**

```cpp
    if (const UETGA_Heal* Heal=Cast<UETGA_Heal>(Defaults))
    {
        if (!Heal->HasValidConfiguration())
        {
            AddIssue(Blueprint,TEXT("ET-HEAL-002"),
                TEXT("Heal requires valid tags, Amount 1..1000, no Cost/Cooldown GE, one in-place DefaultSlot segment, one native branching Heal Commit before blend-out, no clip notifies."),Issues);
        }
        return;
    }
    if (const UETGA_HitReact* Hit=Cast<UETGA_HitReact>(Defaults))
    {
        if (!Hit->HasValidHealingCancelPolicy())
        {
            AddIssue(Blueprint,TEXT("ET-HEAL-003"),TEXT("HitReact must cancel Ability.Heal and must not block State.Healing."),Issues);
        }
    }
    if (Defaults->IsA<UETGA_Dodge>())
```

同文件既有ET-DODGE-004英文提示文字，替换这段短文本：

**找到原内容：**

```cpp
keep Dodging/HitReact/Dead blocked.
```

**替换为：**

```cpp
keep Dodging/HitReact/Dead/Healing blocked.
```

### 7.3 IMC唯一Q映射

ValidateInput开头计数区：

**找到原内容：**

```cpp
    int32 CorrectCount = 0;
```

**替换为：**

```cpp
    int32 CorrectCount = 0;
    int32 HealKeyCount = 0;
    int32 HealMappingCount = 0;
```

随后GetMappings的for循环开头，插入Q / Action配对检查，原左键检查保留：

**找到原内容：**

```cpp
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
```

**替换为：**

```cpp
    for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
    {
        const bool bHealAction=IsValid(Mapping.Action) && Mapping.Action->GetPathName()==
            TEXT("/Game/EmberTrial/Input/Actions/IA_ET_Heal.IA_ET_Heal");
        if (Mapping.Key==EKeys::Q) { ++HealKeyCount; }
        if (bHealAction) { ++HealMappingCount; }
        if ((Mapping.Key==EKeys::Q && !bHealAction) || (bHealAction && Mapping.Key!=EKeys::Q))
        {
            AddIssue(Context,TEXT("ET-HEAL-004"),TEXT("Only Q may map to IA_ET_Heal; Q must not map to another action."),Issues);
        }
```

文件末尾ValidateInput的原左键错误信息至函数末尾，完整替换以下片段（不要把新if放到函数外）：

**找到原内容：**

```cpp
            TEXT("IMC_Default needs exactly one Left Mouse Button mapping to IA_ET_LightAttack; restore that row and remove duplicates."),
            Issues);
    }
}
```

**替换为：**

```cpp
            TEXT("IMC_Default needs exactly one Left Mouse Button mapping to IA_ET_LightAttack; restore that row and remove duplicates."),
            Issues);
    }
    if (HealKeyCount!=1 || HealMappingCount!=1)
    {
        AddIssue(Context,TEXT("ET-HEAL-004"),TEXT("IMC_Default needs exactly one Q mapping to IA_ET_Heal."),Issues);
    }
}
```

规则：ET-HEAL-001仍是A次数配置；002正式治疗GA / 引用 / Montage契约，003受击取消策略，004 IA / IMC。Invalid时按实际字段修，不删校验器或通过None资源绕过。

## 8. 新长期自动化：ETHealingAbilityTests.cpp全文

普通Tests目录.cpp，不是Texts，不加UCLASS / generated头。测试加载**实际保存GA与Montage / Notify**，用临时世界 / Quinn / 真实GAS与原生GE推进；正向成功必须经过动画真实通知。手动调用Notify / 旧Task只作为提前 / 重复 / 迟到事件的负向注入，不能替代成功路径。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Resources/ETHealingResourceComponent.h"
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETHealingAbilityTest,
    "EmberTrial.M2.Healing.Ability",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETHealingAbilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* HealClass=LoadClass<UETGA_Heal>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_Heal.GA_ET_Heal_C"));
    UClass* HitClass=LoadClass<UETGA_HitReact>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    UClass* AttackClass=LoadClass<UETGA_LightAttack>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass=LoadClass<UETGA_Dodge>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    USkeletalMesh* Quinn=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved heal GA"),HealClass) || !TestNotNull(TEXT("Saved hit GA"),HitClass) ||
        !TestNotNull(TEXT("Attack GA"),AttackClass) || !TestNotNull(TEXT("Dodge GA"),DodgeClass) || !TestNotNull(TEXT("Quinn"),Quinn)) { return false; }
    if (!TestTrue(TEXT("Saved heal config"),HealClass->GetDefaultObject<UETGA_Heal>()->HasValidConfiguration()) ||
        !TestTrue(TEXT("Saved hit cancels Healing"),HitClass->GetDefaultObject<UETGA_HitReact>()->HasValidHealingCancelPolicy())) { return false; }
    for (int32 Scenario=0;Scenario<13;++Scenario)
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game)) { Fixture.ForwardErrorMessages(this);return false; }
        UWorld* World=Fixture.GetTestWorld();
        AActor* Floor=World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Floor"),Floor)) { return false; }
        UBoxComponent* Box=NewObject<UBoxComponent>(Floor);Floor->AddInstanceComponent(Box);Floor->SetRootComponent(Box);
        Box->SetMobility(EComponentMobility::Static);Box->SetBoxExtent(FVector(2000,2000,50));
        Box->SetRelativeLocation(FVector(0,0,-50));Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic);Box->SetCollisionResponseToAllChannels(ECR_Block);Box->RegisterComponent();
        FActorSpawnParameters Spawn;Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Body=World->SpawnActor<AEmberTrialCharacter>(FVector(0,0,200),FRotator::ZeroRotator,Spawn);
        APlayerController* PC=World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Body"),Body) || !TestNotNull(TEXT("Controller"),PC)) { return false; }
        Body->CombatTeam=EETCombatTeam::Player;Body->InitialHealth=40.0f;
        Body->HealAbilityClass=HealClass;Body->LightAttackAbilityClass=AttackClass;Body->DodgeAbilityClass=DodgeClass;
        Body->CombatLifeComponent->HitReactAbilityClass=HitClass;
        Body->GetMesh()->SetSkeletalMeshAsset(Quinn);Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Body->GetMesh()->SetRelativeLocation(FVector(0,0,-90));Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->GetCharacterMovement()->bRunPhysicsWithNoController=true;PC->Possess(Body);
        if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        auto Advance=[&Fixture](float Seconds)
        { for (int32 I=0;I<FMath::CeilToInt(Seconds/.01f);++I) { if (!Fixture.TickTestWorld(.01f)) { return false; } }return true; };
        auto Until=[&Fixture](auto Predicate,float Limit)
        { for (int32 I=0;I<FMath::CeilToInt(Limit/.01f) && !Predicate();++I) { if (!Fixture.TickTestWorld(.01f)) { return false; } }return Predicate(); };
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) || !TestTrue(TEXT("Grounded"),Body->GetCharacterMovement()->IsMovingOnGround())) { return false; }
        UAbilitySystemComponent* ASC=Body->GetAbilitySystemComponent();
        FGameplayAbilitySpec* Spec=ASC->FindAbilitySpecFromHandle(Body->HealAbilityHandle);
        UETGA_Heal* Heal=Spec ? Cast<UETGA_Heal>(Spec->GetPrimaryInstance()) : nullptr;
        UETHealingResourceComponent* Resource=Body->FindComponentByClass<UETHealingResourceComponent>();
        UETStaminaRecoveryComponent* Recovery=Body->FindComponentByClass<UETStaminaRecoveryComponent>();
        if (!TestNotNull(TEXT("Heal instance"),Heal) || !TestNotNull(TEXT("Healing resource"),Resource) || !TestNotNull(TEXT("Recovery"),Recovery)) { return false; }
        UAnimMontage* Montage=Heal->HealMontage;
        UAnimInstance* Anim=Body->GetMesh()->GetAnimInstance();
        const FGameplayAttribute Health=UETHealthAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Stamina=UETStaminaAttributeSet::GetStaminaAttribute();
        auto Charges=[Resource]() { int32 C=0,M=0;return Resource->GetHealingSnapshot(C,M) ? C : INDEX_NONE; };
        auto Check=[this,ASC,Health,&Charges](const TCHAR* Label,float Value,int32 Count)
        {
            TestEqual(FString::Printf(TEXT("%s Health Current"),Label),ASC->GetNumericAttribute(Health),Value);
            TestEqual(FString::Printf(TEXT("%s Health Base"),Label),ASC->GetNumericAttributeBase(Health),Value);
            TestEqual(FString::Printf(TEXT("%s charges"),Label),Charges(),Count);
        };
        auto Idle=[&Until,&Advance,Body]() { return Until([Body](){return Body->IsFormalActionIdle();},3.0f) && Advance(.25f); };
        if (Scenario==0)
        {
            Heal->HealMontage=nullptr;TestFalse(TEXT("None montage rejects"),Body->RequestHeal());Heal->HealMontage=Montage;
            ASC->SetNumericAttributeBase(Health,100.0f);TestFalse(TEXT("Full health rejects"),Body->RequestHeal());
            ASC->SetNumericAttributeBase(Health,40.0f);
            Body->GetCharacterMovement()->Velocity=FVector(10,0,0);
            TestFalse(TEXT("Moving rejects"),Body->RequestHeal());Body->GetCharacterMovement()->Velocity=FVector::ZeroVector;
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);TestFalse(TEXT("Air rejects"),Body->RequestHeal());
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Check(TEXT("Prechecks"),40.0f,3);
            ASC->SetNumericAttributeBase(Stamina,75.0f);
        }
        if (Scenario==4)
        {
            for (int32 I=0;I<3;++I)
            {
                if (!TestTrue(TEXT("Arrange consumed charge with real GE"),Resource->TryApplyHealing(40.0f))) { return false; }
                ASC->SetNumericAttributeBase(Health,40.0f);
            }
            TestFalse(TEXT("Zero charges rejects activation"),Body->RequestHeal());Check(TEXT("Exhausted"),40.0f,0);
        }
        else
        {
            if (!TestTrue(TEXT("Start heal"),Body->RequestHeal()) || !TestTrue(TEXT("Healing tag owned"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing))) { return false; }
            const int32 FirstID=Heal->ActiveMontageInstanceID;
            UAnimNotify* Token=Heal->ActiveCommitToken.Get();
            const TStrongObjectPtr<UAbilityTask_PlayMontageAndWait> FirstTask(Heal->MontageTask);
            if (!TestNotNull(TEXT("Started montage task"),FirstTask.Get()) ||
                !TestTrue(TEXT("Actual montage instance"),FirstID!=INDEX_NONE)) { return false; }
            if (!TestFalse(TEXT("Busy repeat refuses"),Body->RequestHeal())) { return false; }
            TestFalse(TEXT("Attack blocked while Healing"),Body->RequestLightAttack());
            TestFalse(TEXT("Dodge blocked while Healing"),Body->RequestDodge());
            TestFalse(TEXT("Shared idle closes move / jump / E"),Body->IsFormalActionIdle());
            Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token); // Negative early-notify injection.
            Check(TEXT("Before genuine notify"),40.0f,3);
            if (Scenario==1)
            {
                if (!Advance(.20f) || !TestTrue(TEXT("Precommit hit accepted"),Body->RequestHitReact()) || !Idle()) { return false; }
                Check(TEXT("Precommit cancel"),40.0f,3);
            }
            else if (Scenario==6 || Scenario==7)
            {
                if (Scenario==7 && !Until([Heal](){return Heal->bCommitAttempted;},2.0f)) { return false; }
                const int32 ExpectedCharges=Scenario==7 ? 2 : 3;
                ASC->SetNumericAttributeBase(Health,0.0f);
                TestTrue(TEXT("Death sealed"),Body->IsCombatDead());TestFalse(TEXT("Death cancels heal"),Heal->IsActive());
                if (!Advance(1.2f)) { return false; }Check(TEXT("Death no late heal or refund"),0.0f,ExpectedCharges);
            }
            else if (Scenario==8 || Scenario==9)
            {
                if (Scenario==9 && !Until([Heal](){return Heal->bCommitAttempted;},2.0f)) { return false; }
                Check(TEXT("Before EndPlay"),Scenario==9 ? 80.0f : 40.0f,Scenario==9 ? 2 : 3);
                if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
                FirstTask->OnMontageBlendingOut(Montage,false);FirstTask->OnMontageEnded(Montage,false);
                TestFalse(TEXT("EndPlay cannot resurrect ability"),Heal->IsActive());
                TestFalse(TEXT("EndPlay clears Healing tag"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing));
                TestTrue(TEXT("Retired task detached"),FirstTask->Ability==nullptr);
                Fixture.ForwardErrorMessages(this);if (HasAnyErrors()) { return false; }continue;
            }
            else
            {
                FDelegateHandle CancelOnHealth;
                if (Scenario==3)
                {
                    if (!Advance(.20f)) { return false; }
                    APlayerState* Pauser=World->SpawnActor<APlayerState>();if (!TestNotNull(TEXT("Pauser"),Pauser)) { return false; }
                    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
                    const double FrozenTime=World->GetTimeSeconds();const float FrozenPosition=Anim->Montage_GetPosition(Montage);
                    const bool bPausedAdvance=Advance(.60f);
                    TestFalse(TEXT("Pause rejects request"),Body->RequestHeal());
                    Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token);
                    TestEqual(TEXT("Paused clock frozen"),World->GetTimeSeconds(),FrozenTime);
                    TestEqual(TEXT("Paused montage frozen"),Anim->Montage_GetPosition(Montage),FrozenPosition);
                    Check(TEXT("Paused"),40.0f,3);World->GetWorldSettings()->SetPauserPlayerState(nullptr);
                    if (!bPausedAdvance) { return false; }
                }
                if (Scenario==5)
                {
                    ASC->GameplayEffectApplicationQueries.Add(FGameplayEffectApplicationQuery::CreateLambda(
                        [](const FActiveGameplayEffectsContainer&,const FGameplayEffectSpec&){return false;}));
                }
                if (Scenario==10)
                {
                    const FGameplayAbilitySpecHandle HealHandle=Body->HealAbilityHandle;
                    CancelOnHealth=ASC->GetGameplayAttributeValueChangeDelegate(Health).AddLambda(
                        [ASC,HealHandle](const FOnAttributeChangeData& Change)
                        { if (Change.NewValue>Change.OldValue) { ASC->CancelAbilityHandle(HealHandle); } });
                }
                if (Scenario==11) { PC->UnPossess(); }
                if (Scenario==12) { ASC->SetNumericAttributeBase(Health,100.0f); }
                if (!Until([Heal](){return Heal->bCommitAttempted || !Heal->IsActive();},2.0f)) { return false; }
                if (Scenario==5 || Scenario==11 || Scenario==12)
                {
                    ASC->GameplayEffectApplicationQueries.Reset();
                    if (!Idle()) { return false; }
                    Check(TEXT("Rejected at commit"),Scenario==12 ? 100.0f : 40.0f,3);
                }
                else
                {
                    Check(TEXT("One real notify commit"),80.0f,2);
                    Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token);Check(TEXT("Duplicate ignored"),80.0f,2);
                    if (Scenario==0)
                    {
                        TestEqual(TEXT("No Stamina regeneration during Healing"),ASC->GetNumericAttribute(Stamina),75.0f);
                        TestFalse(TEXT("No recovery timer during Healing"),Recovery->IsRecoveryScheduled());
                    }
                    if (Scenario==2 && !TestTrue(TEXT("Postcommit hit accepted"),Body->RequestHitReact())) { return false; }
                    if (Scenario==10)
                    {
                        ASC->GetGameplayAttributeValueChangeDelegate(Health).Remove(CancelOnHealth);
                        TestFalse(TEXT("GE callback cancelled action"),Heal->IsActive());
                    }
                    if (!Idle()) { return false; }Check(TEXT("Completion or cancel no refund"),80.0f,2);
                    TestTrue(TEXT("Ended heal releases ASC montage ownership"),ASC->GetAnimatingAbility()!=Heal);
                    if (Scenario==0)
                    {
                        ASC->SetNumericAttributeBase(Health,40.0f);
                        if (!TestTrue(TEXT("A later independent heal can start"),Body->RequestHeal())) { return false; }
                        TestTrue(TEXT("New montage instance"),Heal->ActiveMontageInstanceID!=FirstID);
                        Heal->HandleHealCommit(Body->GetMesh(),Montage,FirstID,Token);
                        FirstTask->OnMontageBlendingOut(Montage,false);FirstTask->OnMontageEnded(Montage,false);
                        Check(TEXT("Old notify / task cannot affect new activation"),40.0f,2);
                        TestTrue(TEXT("New activation keeps montage ownership"),Heal->IsActive() && ASC->GetAnimatingAbility()==Heal);
                        if (!Idle()) { return false; }Check(TEXT("New actual notify commits independently"),80.0f,1);
                    }
                }
            }
        }
        if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        TestFalse(TEXT("Final no Healing tag"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing));
        TestFalse(TEXT("Final inactive / task released"),Heal->IsActive() || Heal->MontageTask!=nullptr);
        Fixture.ForwardErrorMessages(this);if (HasAnyErrors()) { return false; }
    }
    AddInfo(TEXT("[M2-04B2] Healing.Ability: actual saved montage / notify / GE; pre/post cancel, duplicate/old callbacks, pause, rejection, death, EndPlay, possession, synchronous cancel checked."));
    return !HasAnyErrors();
}
#endif
```

十三场景包括激活拒绝、一次真实提交 / 重复通知、旧实例 / 旧Task回调隔离、前后受击取消、Pause、无次数、GE拒绝、生效前后死亡 / EndPlay、GE同步回调取消、控制权失效、生效前外部治疗已满，以及治疗期间恢复停止。测试使用临时前置血量写入，不证明输入映射 / 可见UI / 画面手感；这些由第11节短查补充。不需要搭临时Controller或精准按15帧。

## 9. 正常Build与CLion索引

十六份文件保存、UE关闭后，在PowerShell依次执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor / 索引成功后再Game：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion Reload CMake Project，三个CodeModel保留；预期**59cpp=Core43 + UI13 + Editor3**。新GA / Notify / 测试在Core，Validator在Editor，头文件导航正常。正常UBT / UHT才生成反射文件；不以CMake CodeModel编译、不清缓存、不用Live Coding替代本批Build。

前一项失败先处理第一条真实Error，不继续把后面的索引 / Game记为通过。新测试必须等第10节所有资源保存后才跑；这时先不开Play，当前玩家还没配Heal Class / Action。

### 9.1 首次Build的C2280补修（2026-10-17）

用户在ETGA_Heal.cpp第31行遇到`FNativeGameplayTag(const FNativeGameplayTag&)`被删除。原因是原指南两处`for (const FGameplayTag Tag : {ETGameplayTags::...})`的花括号列表会先推导为`std::initializer_list<FNativeGameplayTag>`，尝试复制原生注册对象；循环变量声明为FGameplayTag也不能改变列表推导。UE5.5.4的FNativeGameplayTag继承FNoncopyable，但提供到FGameplayTag的转换。

本次按用户明确授权，助手已直接修复**构造函数和HasValidTagPolicy两处**：先声明`const FGameplayTag BlockedTags[]`，再按`const FGameplayTag&`遍历。这样数组元素在初始化时转换成可复制的普通Tag值，不复制注册对象，五项Blocked规则不变。第3.2节完整代码也已同步。这属于指南代码问题，不是用户录入错误；其他源码与资产未改，不需手动重复替换。

**接下来仍由用户构建：**保持UE关闭，CLion如提示外部修改则Reload from Disk，重新执行本节Refresh-CLion脚本；成功后执行Game Development Build，再继续第10节配置。无需清缓存或删除Intermediate。本次只做修改范围 / 文档一致性与本机引擎接口核对，未代执行Build / 自动测试，B2尚未验收。此前已Stage的ETGA_Heal.cpp需要在最终Commit前重新Stage最新修改。

## 10. UE资产配置（不新增蓝图业务节点）

### 10.1 AM_ET_Heal_01：唯一原生点Notify

1. 正常打开工程，关闭Sequencer，打开`/Game/EmberTrial/Animations/Healing/Montages/AM_ET_Heal_01`，AS引用仍B1的AS_ET_Heal_01 / DefaultSlot / Loop1 / Blend .10与.10 / AutoBlendOutTrue。
2. 找到时间轴**Notifies**组下第一条子轨道（默认常叫1）。如无子轨道，用Notifies旁 **+ / Add Track → Add Notify Track**建一条即可，不必重命名轨道。
3. 在这条子轨道空白处右键 → **Add Notify → ET Heal Commit**（原生类ETAN_HealCommit）。选择普通点Notify；不是Add Notify State，不是New Notify，不在Skeleton上新建通知名，也不放到AS或LS。
4. 添加后应是一个通知点 / 标签，没有可拉长的状态条。单选该点，**右键通知点本体** → **Notify Begin Time**填 **0.500000**，按Enter提交。再右键看**Notify Frame=15**（30fps）。不是在普通Details找Start / Duration；点通知没有需要填写的Duration。
5. 左键选中通知点，在Details搜索Montage Tick Type，可见时设**Branching Point**。原生类已经强制分支点；字段未显示时确认类名，不建替代通知。Trigger Chance保持1 / 100%，Notify Filter Type保持No Filtering；其他触发设置默认。
6. 只保留这一条点Notify，位置在8–18姿势停留内、自动淡出之前；AS自身Notifies仍空。保存AM，关闭重开确认15帧；在编辑器预览没有回血日志正常。

普通Notify / 分支点类型参照[Epic UE5.5 Animation Notifies](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine?application_version=5.5)。本机SAnimNotifyPanel的Notify Begin Time / Notify Frame在通知本体右键菜单，Enter才提交，沿用已修正的旧章节做法。

### 10.2 新GA_ET_Heal：只配Class Defaults

1. 在`/Game/EmberTrial/Abilities`空白处右键 → Blueprint Class → All Classes，搜索**ETGA_Heal**，选原生父类，命名**GA_ET_Heal**。不要以GameplayAbility空基类或Dodge为父类。
2. 打开 → Class Defaults，搜索Heal：**Heal Montage=AM_ET_Heal_01，Heal Amount=40**。
3. 清空搜索，在Gameplay Ability / Advanced等分类找到**Cost Gameplay Effect Class=None、Cooldown Gameplay Effect Class=None**。不把A的原生治疗GE放到Cost，不建新的成本GE；次数由Notify提交A服务。
4. Tags核对下表；Native默认已正确继承时不制造额外覆盖。字段有旧覆盖且不符时，只重置 / 修改该容器，不重置整份Class Defaults导致Montage丢失。

| GA_ET_Heal容器 | 内容 |
| --- | --- |
| Asset Tags / 默认Ability Tags | 只Ability.Heal |
| Activation Owned Tags | 只State.Healing |
| Activation Blocked Tags | State.Healing / State.Attacking / State.Dodging / State.HitReact / State.Dead，共5项 |
| Cancel Abilities With Tag | 空 |
| Block Abilities With Tag | 空 |
| Activation Required / 其他Target或Source限制 | 默认空 |

5. Instancing Policy=Instanced Per Actor、Net Execution Policy=Server Only，沿用Standalone策略；Retrigger关闭，Event Graph保持无业务，不加Play Montage / Apply GE / End Ability蓝图链。
6. Compile → Save。右键该GA → Asset Actions → Validate Assets，需本项目Valid / Issues=0；ET-HEAL-002先查Montage唯一通知 / AS无通知、Cost / Cooldown None、Tags与Amount。默认生效点.5秒，不用改源码常量。

### 10.3 新IA_ET_Heal与IMC_Default唯一Q行

1. 在`/Game/EmberTrial/Input/Actions`右键 → Input → Input Action，创建**IA_ET_Heal**。若菜单分类折叠，用资产搜索Input Action；不是Input Mapping Context。
2. 打开IA，Value Type=**Digital / Boolean**，Triggers为空、Modifiers为空，Trigger When Paused=False，Consume Input=True（默认值可保留），保存。
3. 打开原`/Game/ThirdPerson/Input/IMC_Default`，保留所有既有行，在Mappings点+新增**一行**：Action=IA_ET_Heal，Key=**Q**，该行Triggers / Modifiers均空。
4. 展开核对没有第二行Q、没有其他键指向IA_ET_Heal；如果之前自己增加过Q映射，先处理冲突，不能让Q同时触发两件事。不要删左键 / 原2 / E等已通过输入。
5. 保存IMC；此时先不跑Validate，因为玩家HealAction下一节才接好。**不在角色 / Controller的Event Graph再建Q或IA_ET_Heal事件**，原生SetupPlayerInputComponent已绑定一次Started。

### 10.4 正式玩家配置与旧GA标签

1. 打开`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` → Class Defaults，搜索Heal：**Heal Ability Class=GA_ET_Heal，Heal Action=IA_ET_Heal**。不是在PracticeAction或PlayerController填。
2. 组件仍单ASC / HealingResource / StaminaRecovery / Trace / Life；HealingResource3/3 / AutoActivate、初始双100、原Attack / Dodge类与既有输入保持。Compile → Save，不Add Component。
3. 打开**GA_ET_LightAttack与GA_ET_EnemyLightAttack**的Class Defaults → Tags，在原Activation Blocked Tags中确认多一项**State.Healing**，原Attacking / HitReact / Dodging / Dead保留；不加取消治疗，不改三段 / 25与敌人单段 / 10。
4. 打开**GA_ET_Dodge**，Activation Blocked Tags应为Dodging / HitReact / Dead / **Healing**，仍**没有Attacking**；Cancel Abilities With Tag仍只Ability.Attack.Light。150 / .30 / .15 / Cost25保持，不把治疗也设成恢复可取消对象。
5. 打开**GA_ET_HitReact**，Cancel Abilities With Tag保留Ability.Attack.Light / Ability.Dodge，并增加**Ability.Heal**；Activation Blocked Tags保留HitReact / Dead，**没有Healing**。不把Healing误加成受击激活阻断。
6. 每个实际调整资产Compile → Save。Native默认已经符合时仅核对，不强制制造差异。训练目标 / 敌人身体不配置Heal Class / Action，不新增AI治疗。
7. Content Browser分别Validate：**GA_ET_Heal、玩家BP、IMC_Default、玩家攻击GA、敌人攻击GA、Dodge GA、HitReact GA**七项，需本项目Valid / Issues=0。GA问题按对应字段修；旧状态HUD / UI资产不重做全套校验。
8. 回正式地图L_ET_VerticalSlice，Save All。前面原生Grant使用Class引用，真正Play时只有玩家一条Heal granted；敌人无Heal Class正常，不把它当缺依赖。

## 11. 分组检查（实机10–15分钟）

### 11.1 长期自动化，约2–4分钟

不Play，在Output Log顺序运行；前组结束后再下组：

```text
Automation RunTests EmberTrial.M2.Healing
```

应有**Resource / Ability两项Success**（先后顺序不限）。然后：

```text
Automation RunTests EmberTrial.M2.Resources.StaminaRecovery
```

等Success，再：

```text
Automation RunTests EmberTrial.M2.Combo.RecoveryDodge
```

等Success，合计四项。Recovery确认新增Healing Tag监听 / 解绑，RecoveryDodge确认旧三段交接与受击仍成立；不重复Flow / Invulnerability / 墙坡 / 帧率 / Lifecycle全部矩阵。新Ability自动化不得在AM通知 / GA / 旧Tag未保存前跑；未运行不填写Success。

失败发测试名、第一条Expected与大致时分，我读取日志；不删断言或扩大生效窗口。测试的旧Notify / Task调用是负向注入，正式成功仍由实际保存Montage触发，不在生产代码手动调用Commit补成功。

### 11.2 Q正式治疗与次数，约3–4分钟

**准备：**正式地图Entry空地Play，远离敌人，双100 / 3次。可通过原命令查询：

```text
ke BP_ETPlayerController_C ET_Healing 0
```

1. 满血按Q，预期拒绝、无Begin / 没有治疗动作、次数3；Request Accepted0允许。随后按原4一次到75，等受击结束、站定，再新按Q。
2. 预期开始后暂时75 / 3，到停留段约.5秒才75→100 / 2；生命HUD真实更新，一次Begin / Commit Applied1 / End，结束HealingTag0。没有次数HUD是本批预期，用上述0查询次数，不手动再执行ET_Healing40来代替Q。
3. 同一次长按Q / 播放中连按不重启、不补下一次；结束后再4到75、松开后新按Q可再成功。再做一次到0次数，4后Q拒绝；其他动作仍可用。不要为长按反复扣资源而加Triggered链。

### 11.3 动作互斥、受击与P，约4–6分钟

**准备：**次数耗尽可用既有4致死 → Retry，新身体100 / 3；再4一次，等空闲后开始Q。

- **操作A：**治疗期间按W / Space / E / 左键 / 2，镜头转动；Q中按3一次。再试攻击中按Q、移动中按Q，站定后重新按。
- **预期A：**治疗中移动 / 跳跃 / E / 攻击 / 闪避不执行、无排队，镜头可转；3真实受击取消治疗。若取消在生效前，不回血不扣；若生效后保留已成功治疗 / 消费，受击本身可能伴随后续伤害。**精确前 / 后时点已由自动化覆盖，不要求人工掐15帧**。普通移动 / 攻击期间Q拒绝，结束需新按Q。
- **操作B：**让敌人普通命中一次确认真实伤害 / 受击 / HUD仍正常，再到安全空地治疗中P暂停约2秒。可查ET_Healing0；在菜单按下并松开Q / 左键 / 2，再用一种已支持方式继续。
- **预期B：**治疗Montage与世界时钟冻结、生命 / 次数不继续变化；恢复从原处继续，不补菜单按键、不多提交、不漂移不卡键。暂停可能发生在生效点前或后，两者均合法，无需手工赶时点。治疗状态没有无敌，普通伤害应能受击取消。

### 11.4 世界重建与短基线，约2–3分钟

治疗后用4完成死亡 → Retry，查询新双100 / 3次、单生命 / 体力HUD；用P返回菜单 → Continue仍新身体 / 按现有检查点出生。再开始一次Q并Stop（血量不足时先4），重开确认没有旧Healing / 多余动作。

最后左键真实25、2一次成本25、E一次、P一种继续方式、移动转向跳跃正常即可。恢复正式双100、治疗3/3 / Amount40 / 原AM .5秒唯一通知、原25 / 10、Dodge150 / .30 / .15 / Cost25、slomo1 / Debug False，所有Source / 资产Save All。当前不Cook / 采Insights，不重新建UI换身体验收图。

## 12. 完成反馈与报错材料

```text
M2-04B2 完成 / 卡在第__节第__步
五新 / 十一旧Source；关闭UE后Editor Build / 索引59cpp / Game Build：
AM唯一ET Heal Commit点在15帧 / .5秒，AS无Notify；GA父类 / Amount40 / 无Cost-Cooldown GE / Tags：
IA Boolean / 无Triggers / 非暂停触发；IMC唯一Q；玩家两引用 / 原生单Started绑定：
旧Attack / Dodge / HitReact标签已核对，七正式资产Compile / Save / Valid：
Healing.Resource / Healing.Ability / StaminaRecovery / RecoveryDodge四项：
正式Q晚生效 / 一次40与一次消费 / 长按忙碌无补执行 / 满血和耗尽拒绝：
动作互斥、3与真实伤害取消、P冻结继续 / 无补播：
死亡Retry / Continue新100与3、动作Stop重开 / 短基线：
默认值 / slomo1 / Debug False恢复，Source资产Save All：
失败或手感问题（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

代码错误文字贴第一条真实C++ / UHT / 链接错误及前后几行，不只给UBT exit6；自动化给测试名 / 首条Expected。通知问题截图完整AM轨道 + 右键时间菜单 / 所选通知Details；Tag问题截图该GA资产名与容器，输入问题截图IA与IMC唯一Q行 / 玩家两个引用。行为说明输入顺序、当时血量 / 次数与实际 / 期望，必要时短视频。日志只需路径 / 大致时间，不必摘几十行；普通报错不自动授权改源码。

我保存B2验收报告且无待补项后，会同一轮直接生成C次数Presenter / HUD指南；有问题先给窄补修，不重跑未变矩阵。

## 13. 本批 Commit / Push（固定放在最后）

**当前实现阶段：**B1素材与A源码已由用户提交fd94aad；B2源码已录入，尚待正常Build与后续配置 / 检查。提交本指南、B1报告 / 来源与状态文档的实际差异；第9.1节直接补修的ETGA_Heal.cpp也要重新Stage最新内容，不能只提交原先暂存的版本。Raw候选 / 证据保持本地，本次没有自动Commit / Push。

**B2实物完成并验收后：**CLion刷新Changes，按第2节完整路径Stage五新 / 十一旧共十六份Source；新测试需完整实现，不只创建骨架，已Stage后又修改需重新Stage最新差异。

两新LFS资产：

```text
EmberTrial/Content/EmberTrial/Abilities/GA_ET_Heal.uasset
EmberTrial/Content/EmberTrial/Input/Actions/IA_ET_Heal.uasset
```

七旧资产按实际合法变化Stage，不人为制造重存：

```text
EmberTrial/Content/EmberTrial/Animations/Healing/Montages/AM_ET_Heal_01.uasset
EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset
EmberTrial/Content/ThirdPerson/Input/IMC_Default.uasset
EmberTrial/Content/EmberTrial/Abilities/GA_ET_LightAttack.uasset
EmberTrial/Content/EmberTrial/Abilities/GA_ET_EnemyLightAttack.uasset
EmberTrial/Content/EmberTrial/Abilities/GA_ET_Dodge.uasset
EmberTrial/Content/EmberTrial/Abilities/GA_ET_HitReact.uasset
```

来源清单待实物核对后预计61→63，并更新AM / 玩家 / IA / IMC / 旧GA实际SHA；新原生Notify不是新的美术资产，不另造来源行。制作LS / AS、Rig / Skeleton / AnimBP / 地图本批无计划变化，出现额外修改先给原因，不全部勾选掩盖。

届时一起Stage B2报告 / 本指南 / 状态设计文档与真实来源更新，检查Staged Diff / LFS指针，再由用户Commit / Push。建议`feat: add notify-committed healing ability and Q input`；仅当前指南可用`docs: prepare formal healing ability`。Raw证据 / 候选 / 本地脚本、日志 / 视频、Binaries / Intermediate / Saved、生成CMake / IDE缓存和历史包留本地，不Force Add；助手不操作Git索引或上传。
