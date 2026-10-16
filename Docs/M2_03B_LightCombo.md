# M2-03B：三段轻攻击、单槽输入缓冲与独立命中

2026-10-16，固定 **UE5.5.4**。前置[A素材已验收](Reports/M2_03A_Verification.md)，[分批路线](M2_03_LightComboRoadmap.md)。**本批只有文档候选，尚未写入工程Source / 资产，也未Build / 运行新测试。**当前96Source / 50cpp / 来源57项；本批三新 / 六旧Source，实施后预计99 / 52。新建一份独立第1段Montage，来源预计58；既有GA玩家与第2段Montage会有配置差异。

全部C++仍由你手动录入。预计手动 **120–210分钟**：代码 / 理解80–145、资产装配20–35、相关检查10–15、保存记录5–10；构建等待另列，可分两次完成。预算不计实耗，不要求做满。

## 1. 本批结果与责任

| 输入 / 状态 | 本批结果 |
| --- | --- |
| 空闲、地面站定单击左键 | 第1段一次25，正常结束 |
| 第1 / 2段约.48秒再单击 | 输入最多存1条，有效.25世界秒；到.60秒连段窗消费，进入下一段 |
| 连段窗.60–.80秒内新输入 | 接受一次后延后至Timer调度点切段，避免在Notify调用栈里替换Montage |
| 太早按，例如开始后.05秒 | 到窗前过期，不推进 |
| 多次点击挤在同一段 | 只更新这一条输入，不储存两次未来攻击；不能靠第一段的一串点击自动排满三段 |
| 第3段再点 | 不排第4段，结束后重新按才从1开始 |
| 长按 | Started只发生一次，不自动连击 |
| 整条链 | 一个能力、Attacking持续；每段独立Trace Serial / SeenTargets，同目标每段最多一次 |
| 受击 / 死亡 / Stop | 清自己的Trace、Task、输入与待切段Timer；旧回调不能影响新动作 |
| P暂停 | 世界时间和缓冲有效期冻结；菜单输入拒绝；暂停前已合法缓存的输入可随原动作恢复后消费 |
| 攻击中按2 | **本批仍拒绝**；恢复阶段闪避取消留C，不提前删Dodge的Attacking门控 |

玩家每段仍25，三次正常命中100→75→50→25；不额外消耗体力。敌人仍原单段10，原GE / 无敌提交门控保留。数字1仍是现有攻击请求别名，不新增第二份能力或临时测试按键。

**素材隔离：**A只新增2 / 3段；B现在新建`AM_ET_Light_01`供玩家使用，让旧`AM_M0_Attack_01_Montage`继续服务敌人、Practice和Dodge真实命中测试。后续B的玩家1 / 2段添加连段NotifyState，第3段没有该窗口；不修改旧共用Montage。

**实现分工：**Character负责把活动能力的输入交给同一实例；Ability拥有当前段 / 代次、单条带到期时间的输入、Montage Task和唯一待切段Timer；NotifyState只转发带Mesh / Montage实例 / Token的事件，不在共享通知对象里保存某个角色状态。Trace既有BeginAttack / EndAttack已支持独立Serial，本批不改它。

Timer只为离开当前通知调用栈后切段，不按动画时长解锁。UE5.5默认SetTimerForNextTick可能在同一世界帧后面的Timer调度阶段执行，**不保证一定跨过一个渲染帧**；代码依赖延后调度和激活 / 段代次，而不是屏幕帧号。End先使旧身份失效、解绑旧Task的动态回调和属于该Task的原生Montage委托，再停止自己的旧Montage。Task执行EndTask完成正常解绑后，才清空这份已结束Task的Ability指针；即使动画队列保留旧原生委托的副本，也不能用它清掉新一轮相同Montage的ASC归属。停止动作可能同步回调，前后都复核激活 / 段代次；不修改引擎代码或仍活动Task的归属。

本机接口按Engine的AbilityTask_PlayMontageAndWait.cpp、TimerManager.cpp、AnimInstance.h核对，参考[Epic 5.5 Montage Task](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/Abilities/Tasks/UAbilityTask_PlayMontageAndWait?application_version=5.5)与[Notify说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine?application_version=5.5)。静态核对不代替本次实际编译 / 运行。

## 2. 准备与文件清单

Stop PIE → Save All → 关闭UE → CLion保存。先完成第3–6节全部九份Source，再第7节正常Build；不使用Live Coding处理反射结构变更。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Animation\ETANS_ComboWindow.h                 新
├─ EmberTrial\Private\Animation\ETANS_ComboWindow.cpp              新
├─ EmberTrial\Tests\ETLightComboFlowTests.cpp                       新
├─ EmberTrial\Public\Abilities\ETGA_LightAttack.h                  旧，完整替换
├─ EmberTrial\Private\Abilities\ETGA_LightAttack.cpp               旧，完整替换
├─ EmberTrial\Private\Characters\EmberTrialCharacter.cpp          旧，只替换RequestLightAttack
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h             旧，只加一行friend
├─ EmberTrial\Public\Combat\ETCombatLifeComponent.h               旧，只加一行friend
└─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp 旧，只插入一段
```

不改Build.cs / Target / uproject / Trace / Dodge / UI / GameMode或存档；新测试在既有Tests目录，不是UCLASS。`FETLightAttackStage`结构体放现有GA头文件，保留原AttackMontage / Damage等属性名称，避免已有蓝图引用失效。不创建新的GA类、Actor组件或DataAsset。

## 3. 新建无状态的连段通知（两完整文件）

### 3.1 ETANS_ComboWindow.h

路径见第2节Public/Animation，完整内容：

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ETANS_ComboWindow.generated.h"

UCLASS(meta=(DisplayName="ET Combo Window"))
class EMBERTRIAL_API UETANS_ComboWindow : public UAnimNotifyState
{
    GENERATED_BODY()
public:
    UETANS_ComboWindow();
    virtual void BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) override;
    virtual void BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) override;
private:
    void Route(FBranchingPointNotifyPayload& Payload,bool bOpen);
};
```

### 3.2 ETANS_ComboWindow.cpp

路径见Private/Animation，完整内容：

```cpp
#include "Animation/ETANS_ComboWindow.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Abilities/ETGA_LightAttack.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/SkeletalMeshComponent.h"

UETANS_ComboWindow::UETANS_ComboWindow()
{
    bIsNativeBranchingPoint=true;
#if WITH_EDITORONLY_DATA
    bShouldFireInEditor=false;NotifyColor=FColor::Cyan;
#endif
}
void UETANS_ComboWindow::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload) { Route(Payload,true); }
void UETANS_ComboWindow::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload) { Route(Payload,false); }
void UETANS_ComboWindow::Route(FBranchingPointNotifyPayload& Payload,bool bOpen)
{
    USkeletalMeshComponent* Mesh=Payload.SkelMeshComponent;
    AEmberTrialCharacter* Body=IsValid(Mesh) ? Cast<AEmberTrialCharacter>(Mesh->GetOwner()) : nullptr;
    UAbilitySystemComponent* ASC=IsValid(Body) ? Body->GetAbilitySystemComponent() : nullptr;
    UETGA_LightAttack* Attack=IsValid(ASC) ? Cast<UETGA_LightAttack>(ASC->GetAnimatingAbility()) : nullptr;
    if (IsValid(Attack)) { Attack->HandleComboWindow(Mesh,Payload.SequenceAsset,Payload.MontageInstanceID,this,bOpen); }
}
```

## 4. 完整替换 LightAttack 两文件

### 4.1 ETGA_LightAttack.h

完整覆盖该头文件。原类名 / API / 四个旧配置属性保留；两行friend是临时世界测试访问，不是蓝图装配节点。新增组合配置默认关闭，只有后面玩家GA显式启用。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "ETGA_LightAttack.generated.h"

class UAnimMontage;
class UAnimNotifyState;
class UAnimSequenceBase;
class USkeletalMeshComponent;
class UETCombatTraceComponent;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
struct FHitResult;

USTRUCT(BlueprintType)
struct EMBERTRIAL_API FETLightAttackStage
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category="Combo")
    TObjectPtr<UAnimMontage> Montage = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="Combo", meta=(ClampMin="0.01"))
    float DamageMultiplier = 1.0f;
};

UCLASS(Blueprintable)
class EMBERTRIAL_API UETGA_LightAttack : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UETGA_LightAttack();
    bool IsComboEnabled() const { return bEnableCombo; }
    bool HasValidComboConfiguration() const;
    bool TryBufferComboInput();
    void HandleComboWindow(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
        int32 MontageInstanceID, UAnimNotifyState* WindowToken, bool bOpen);

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
    friend class FETLightComboFlowTest;

    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack")
    TObjectPtr<UAnimMontage> AttackMontage;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack", meta=(ClampMin="0.0",Units="cm/s"))
    float AttackMaxHorizontalSpeed = 3.0f;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack")
    TSubclassOf<UGameplayEffect> DamageEffectClass;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Attack", meta=(ClampMin="0.0"))
    float AttackDamage = 25.0f;

    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Combo")
    bool bEnableCombo = false;
    // Exactly two follow-ups when enabled. AttackMontage remains stage one.
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Combo", meta=(EditCondition="bEnableCombo"))
    TArray<FETLightAttackStage> ComboFollowUps;
    UPROPERTY(EditDefaultsOnly, Category="EmberTrial|Combo",
        meta=(EditCondition="bEnableCombo",ClampMin="0.05",ClampMax="0.50",Units="s"))
    float ComboBufferSeconds = 0.25f;

    UPROPERTY(Transient)
    TArray<FETLightAttackStage> RuntimeStages;
    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    TWeakObjectPtr<UETCombatTraceComponent> ActiveTrace;
    TWeakObjectPtr<UAnimNotifyState> ActiveComboWindow;
    FDelegateHandle TraceHitHandle;
    FTimerHandle TransitionTimer;
    uint32 ActiveTraceSerial = 0;
    uint32 ActivationSerial = 0;
    uint32 StageSerial = 0;
    int32 ActiveStageIndex = INDEX_NONE;
    int32 ActiveMontageInstanceID = INDEX_NONE;
    float ActiveStageDamage = 0.0f;
    double BufferedInputExpiresAt = 0.0;
    bool bHasBufferedInput = false;
    bool bComboWindowOpen = false;
    bool bComboWindowPassed = false;
    bool bTransitionPending = false;
    bool bStartingStage = false;
    bool bEndingAttack = false;

    bool HasValidDamageConfiguration() const;
    bool HasNextStage() const;
    bool CanAcceptComboInput() const;
    bool StartStage(int32 Index);
    void ClearBufferedInput();
    void ClearStageResources();
    void TryConsumeBufferedInput();
    void HandleQueuedTransition(uint32 ExpectedActivation, uint32 ExpectedStage);
    void HandleTraceHit(uint32 Serial, const FHitResult& Hit);
    UFUNCTION()
    void HandleMontageCompleted();
    UFUNCTION()
    void HandleMontageAborted();
};
```

### 4.2 ETGA_LightAttack.cpp

完整覆盖该cpp，不再另外粘贴旧Activate / End或第二份HandleTraceHit。伤害函数继续使用当前Health GE / 阵营 / Ready / 无敌提交保护；只把本段请求值改为当前段缓存伤害。

```cpp
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "Animation/ETANS_AttackWindow.h"
#include "Animation/ETANS_ComboWindow.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Characters/EmberTrialCharacter.h"
#include "ETPracticeActionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/ETCombatTraceComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "GameplayEffect.h"
#include "AIController.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogETLightAttack, Log, All);

UETGA_LightAttack::UETGA_LightAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    bRetriggerInstancedAbility = false;

    FGameplayTagContainer AssetTags;
    AssetTags.AddTag(ETGameplayTags::Ability_Attack_Light);
    SetAssetTags(AssetTags);
    ActivationOwnedTags.AddTag(ETGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Attacking);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_HitReact);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dodging);
    ActivationBlockedTags.AddTag(ETGameplayTags::State_Dead);
}

bool UETGA_LightAttack::HasValidDamageConfiguration() const
{
    if (!FMath::IsFinite(AttackDamage) || AttackDamage <= 0.0f ||
        !IsValid(DamageEffectClass.Get()))
    {
        return false;
    }

    const UGameplayEffect* Effect = DamageEffectClass.GetDefaultObject();
    if (!IsValid(Effect) ||
        Effect->DurationPolicy != EGameplayEffectDurationType::Instant ||
        Effect->Modifiers.Num() != 1 || Effect->Executions.Num() != 0)
    {
        return false;
    }

    const FGameplayModifierInfo& Modifier = Effect->Modifiers[0];
    if (Modifier.Attribute != UETHealthAttributeSet::GetHealthAttribute() ||
        Modifier.ModifierOp != EGameplayModOp::AddBase ||
        Modifier.ModifierMagnitude.GetMagnitudeCalculationType() !=
            EGameplayEffectMagnitudeCalculation::SetByCaller)
    {
        return false;
    }

    return Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag ==
        ETGameplayTags::SetByCaller_HealthDelta;
}

bool UETGA_LightAttack::HasValidComboConfiguration() const
{
    if (!bEnableCombo) { return true; }
    if (!FMath::IsFinite(ComboBufferSeconds) || ComboBufferSeconds < .05f ||
        ComboBufferSeconds > .50f || ComboFollowUps.Num() != 2 || !IsValid(AttackMontage)) { return false; }
    TArray<UAnimMontage*> Montages{AttackMontage.Get()};
    for (const FETLightAttackStage& Stage : ComboFollowUps)
    {
        if (!IsValid(Stage.Montage) || !FMath::IsFinite(Stage.DamageMultiplier) ||
            Stage.DamageMultiplier <= 0.0f || !FMath::IsFinite(AttackDamage * Stage.DamageMultiplier) ||
            Montages.Contains(Stage.Montage.Get())) { return false; }
        Montages.Add(Stage.Montage.Get());
    }
    for (int32 I=0; I<Montages.Num(); ++I)
    {
        const UAnimMontage* Montage=Montages[I];
        const float Length=Montage->GetPlayLength();
        if (!IsValid(Montage->GetSkeleton()) || Montage->GetSkeleton()!=AttackMontage->GetSkeleton() ||
            !FMath::IsFinite(Length) || Length<=0.0f || Montage->HasRootMotion() ||
            !FMath::IsNearlyEqual(Montage->RateScale,1.0f) ||
            !Montage->SlotAnimTracks.ContainsByPredicate([](const FSlotAnimationTrack& T)
                { return T.SlotName==FName(TEXT("DefaultSlot")); })) { return false; }
        int32 HitCount=0,ComboCount=0;
        float HitEnd=0.0f,ComboBegin=0.0f,ComboEnd=0.0f;
        for (const FAnimNotifyEvent& Event : Montage->Notifies)
        {
            const UAnimNotifyState* State=Event.NotifyStateClass;
            if (!IsValid(State)) { continue; }
            const bool bHit=State->IsA<UETANS_AttackWindow>();
            const bool bCombo=State->IsA<UETANS_ComboWindow>();
            if (!bHit && !bCombo) { continue; }
            const float Begin=Event.GetTime(),Duration=Event.GetDuration(),End=Begin+Duration;
            if (!FMath::IsFinite(Begin) || !FMath::IsFinite(Duration) || Begin<0 ||
                Duration<=0 || End>Length+KINDA_SMALL_NUMBER) { return false; }
            if (bHit) { ++HitCount;HitEnd=End; }
            if (bCombo) { ++ComboCount;ComboBegin=Begin;ComboEnd=End; }
        }
        if (HitCount!=1 || ComboCount!=(I<2 ? 1 : 0)) { return false; }
        if (I<2 && (ComboBegin<HitEnd || ComboEnd>=Length-Montage->BlendOut.GetBlendTime())) { return false; }
    }
    return true;
}

bool UETGA_LightAttack::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !Super::CanActivateAbility(
        Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    AEmberTrialCharacter* BodyCharacter =
        Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    if (!IsValid(BodyCharacter) || BodyCharacter->IsActorBeingDestroyed() ||
        BodyCharacter->IsCombatDead() || !BodyCharacter->IsGameplayReady() ||
        ActorInfo->OwnerActor.Get() != BodyCharacter ||
        BodyCharacter->GetAbilitySystemComponent() !=
            ActorInfo->AbilitySystemComponent.Get())
    {
        return false;
    }

    UWorld* World = BodyCharacter->GetWorld();
    if (!IsValid(World) || World->GetNetMode() != NM_Standalone ||
        !BodyCharacter->HasAuthority() ||
        UGameplayStatics::IsGamePaused(BodyCharacter))
    {
        return false;
    }

    float Health = 0.0f;
    float MaxHealth = 0.0f;
    if (!BodyCharacter->GetGameplayHealthSnapshot(Health, MaxHealth) ||
        !FMath::IsFinite(Health) || !FMath::IsFinite(MaxHealth) ||
        MaxHealth < 1.0f || Health <= 0.0f || Health > MaxHealth)
    {
        return false;
    }

    AController* BodyController = BodyCharacter->GetController();
    if (!IsValid(BodyController) || BodyController->GetPawn() != BodyCharacter)
    {
        return false;
    }
    if (const APlayerController* PlayerController =
        Cast<APlayerController>(BodyController))
    {
        if (!PlayerController->IsLocalController() ||
            PlayerController->IsMoveInputIgnored() ||
            BodyCharacter->GetCombatTeam() != EETCombatTeam::Player)
        {
            return false;
        }
    }
    else if (!Cast<AAIController>(BodyController) || bEnableCombo ||
        BodyCharacter->GetCombatTeam() != EETCombatTeam::Enemy)
    {
        return false;
    }

    UCharacterMovementComponent* Movement = BodyCharacter->GetCharacterMovement();
    const FVector Velocity = BodyCharacter->GetVelocity();
    if (!IsValid(Movement) || !Movement->IsMovingOnGround() ||
        Velocity.ContainsNaN() || !FMath::IsFinite(AttackMaxHorizontalSpeed) ||
        AttackMaxHorizontalSpeed < 0.0f ||
        Velocity.SizeSquared2D() > FMath::Square(AttackMaxHorizontalSpeed))
    {
        return false;
    }

    const UETPracticeActionComponent* Practice =
        BodyCharacter->FindComponentByClass<UETPracticeActionComponent>();
    if (Practice && !Practice->IsPracticeActionIdle())
    {
        return false;
    }

    if (!HasValidDamageConfiguration())
    {
        UE_LOG(LogETLightAttack, Warning,
            TEXT("[M1-04B2] Invalid damage config Character=%s Effect=%s Damage=%.1f"),
            *GetNameSafe(BodyCharacter), *GetNameSafe(DamageEffectClass.Get()),
            AttackDamage);
        return false;
    }

    const UETCombatTraceComponent* Trace =
        BodyCharacter->FindComponentByClass<UETCombatTraceComponent>();
    UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
    return HasValidComboConfiguration() && IsValid(AttackMontage) && IsValid(AnimInstance) &&
        IsValid(Trace) && Trace->CanStartAttack() &&
        AnimInstance->GetCurrentActiveMontage() == nullptr;
}

bool UETGA_LightAttack::HasNextStage() const
{
    return bEnableCombo && ActiveStageIndex>=0 && RuntimeStages.IsValidIndex(ActiveStageIndex+1);
}

bool UETGA_LightAttack::CanAcceptComboInput() const
{
    if (!IsActive() || !CurrentActorInfo || !bEnableCombo || bEndingAttack ||
        bStartingStage || !HasNextStage()) { return false; }
    const AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    const APlayerController* PC=IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UAbilitySystemComponent* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    const UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    const FAnimMontageInstance* Instance=IsValid(Anim) && IsValid(ActiveMontage)
        ? Anim->GetActiveInstanceForMontage(ActiveMontage) : nullptr;
    return IsValid(Body) && !Body->IsActorBeingDestroyed() && Body->IsGameplayReady() &&
        !Body->IsCombatDead() && Body->HasAuthority() && Body->GetCombatTeam()==EETCombatTeam::Player &&
        CurrentActorInfo->OwnerActor.Get()==Body && Body->GetAbilitySystemComponent()==ASC &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn()==Body && !PC->IsMoveInputIgnored() &&
        !UGameplayStatics::IsGamePaused(Body) && IsValid(ASC) && ASC->GetAnimatingAbility()==this &&
        ASC->GetCurrentMontage()==ActiveMontage && Instance && Instance->GetInstanceID()==ActiveMontageInstanceID;
}

void UETGA_LightAttack::ClearBufferedInput()
{
    bHasBufferedInput=false;BufferedInputExpiresAt=0.0;
}

bool UETGA_LightAttack::TryBufferComboInput()
{
    if (!CanAcceptComboInput() || bTransitionPending || bComboWindowPassed) { return false; }
    UWorld* World=GetWorld();
    if (!IsValid(World)) { return false; }
    bHasBufferedInput=true;
    BufferedInputExpiresAt=double(World->GetTimeSeconds())+ComboBufferSeconds;
    UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Input buffered Stage=%d Expires=%.3f"),
        ActiveStageIndex+1,BufferedInputExpiresAt);
    TryConsumeBufferedInput();
    return true; // Accepted input can still expire before the window opens.
}

void UETGA_LightAttack::HandleComboWindow(USkeletalMeshComponent* Mesh,
    UAnimSequenceBase* Animation,int32 MontageInstanceID,UAnimNotifyState* WindowToken,bool bOpen)
{
    const AEmberTrialCharacter* Body=CurrentActorInfo
        ? Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
    if (!IsActive() || bStartingStage || bEndingAttack || !HasNextStage() || !IsValid(Body) ||
        Mesh!=Body->GetMesh() || Animation!=ActiveMontage || MontageInstanceID!=ActiveMontageInstanceID ||
        !IsValid(WindowToken) || !WindowToken->IsA<UETANS_ComboWindow>()) { return; }
    if (bOpen)
    {
        if (bComboWindowOpen || bComboWindowPassed) { return; }
        ActiveComboWindow=WindowToken;bComboWindowOpen=true;
        UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Combo window open Stage=%d Ticket=%u"),ActiveStageIndex+1,StageSerial);
        TryConsumeBufferedInput();
    }
    else if (bComboWindowOpen && ActiveComboWindow.Get()==WindowToken)
    {
        bComboWindowOpen=false;bComboWindowPassed=true;ActiveComboWindow.Reset();ClearBufferedInput();
    }
}

void UETGA_LightAttack::TryConsumeBufferedInput()
{
    if (!bComboWindowOpen || !bHasBufferedInput || bTransitionPending || !CanAcceptComboInput()) { return; }
    UWorld* World=GetWorld();
    if (!IsValid(World)) { return; }
    if (double(World->GetTimeSeconds())>BufferedInputExpiresAt)
    {
        ClearBufferedInput();
        UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Buffered input expired Stage=%d"),ActiveStageIndex+1);
        return;
    }
    ClearBufferedInput();bTransitionPending=true;
    TransitionTimer=World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
        this,&ThisClass::HandleQueuedTransition,ActivationSerial,StageSerial));
}

void UETGA_LightAttack::HandleQueuedTransition(uint32 ExpectedActivation,uint32 ExpectedStage)
{
    if (!IsActive() || bEndingAttack || ExpectedActivation!=ActivationSerial ||
        ExpectedStage!=StageSerial || !bTransitionPending) { return; }
    TransitionTimer.Invalidate();
    UWorld* World=GetWorld();
    if (IsValid(World) && UGameplayStatics::IsGamePaused(this))
    {
        TransitionTimer=World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
            this,&ThisClass::HandleQueuedTransition,ExpectedActivation,ExpectedStage));
        return;
    }
    if (!CanAcceptComboInput())
    {
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);return;
    }
    const int32 Next=ActiveStageIndex+1;
    const bool bStarted=StartStage(Next);
    if (ExpectedActivation!=ActivationSerial || !IsActive()) { return; }
    if (!bStarted) { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true); }
}

void UETGA_LightAttack::ClearStageResources()
{
    const uint32 ClearingActivation=ActivationSerial;
    const uint32 ClearingStage=StageSerial;
    UAbilityTask_PlayMontageAndWait* OldTask=MontageTask;
    UAnimMontage* OldMontage=ActiveMontage;
    const int32 OldID=ActiveMontageInstanceID;
    const uint32 OldTraceSerial=ActiveTraceSerial;
    MontageTask=nullptr;ActiveMontage=nullptr;ActiveMontageInstanceID=INDEX_NONE;
    ActiveTraceSerial=0;ActiveStageDamage=0.0f;
    bComboWindowOpen=false;bComboWindowPassed=false;ActiveComboWindow.Reset();
    if (UETCombatTraceComponent* Trace=ActiveTrace.Get()) { Trace->EndAttack(OldTraceSerial); }
    UAbilitySystemComponent* ASC=CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
    UAnimInstance* Anim=CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
    FAnimMontageInstance* Instance=IsValid(Anim) && OldID!=INDEX_NONE ? Anim->GetMontageInstanceForID(OldID) : nullptr;
    if (IsValid(OldTask))
    {
        OldTask->OnCompleted.RemoveAll(this);OldTask->OnInterrupted.RemoveAll(this);OldTask->OnCancelled.RemoveAll(this);
        // Native task callbacks can otherwise fire after EndTask while a new stage is active.
        if (Instance)
        {
            if (Instance->OnMontageBlendedInEnded.IsBoundToObject(OldTask)) { Instance->OnMontageBlendedInEnded.Unbind(); }
            if (Instance->OnMontageBlendingOutStarted.IsBoundToObject(OldTask)) { Instance->OnMontageBlendingOutStarted.Unbind(); }
            if (Instance->OnMontageEnded.IsBoundToObject(OldTask)) { Instance->OnMontageEnded.Unbind(); }
        }
        OldTask->EndTask();
        // Finish normal cancellation-listener / task-owner cleanup first.
        // A queued copy of a native montage delegate may still call this retired task.
        // Its old Ability must not point at a new activation using the same montage.
        OldTask->Ability=nullptr;
    }
    if (ClearingActivation==ActivationSerial && ClearingStage==StageSerial &&
        IsValid(ASC) && IsValid(OldMontage) && ASC->GetAnimatingAbility()==this &&
        ASC->GetCurrentMontage()==OldMontage && (Instance || OldID==INDEX_NONE))
    {
        ASC->CurrentMontageStop(.10f);
        // Stop can synchronously cancel or reactivate this instanced ability.
        if (ClearingActivation==ActivationSerial && ClearingStage==StageSerial && IsValid(ASC) &&
            ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==OldMontage)
        { ASC->ClearAnimatingAbility(this); }
    }
}

bool UETGA_LightAttack::StartStage(int32 Index)
{
    if (!IsActive() || bEndingAttack || !RuntimeStages.IsValidIndex(Index) || !CurrentActorInfo) { return false; }
    const uint32 Activation=ActivationSerial;
    bStartingStage=true;bTransitionPending=false;ClearBufferedInput();
    ++StageSerial;if (StageSerial==0) { ++StageSerial; }
    const uint32 Ticket=StageSerial;
    ClearStageResources();
    if (!IsActive() || Activation!=ActivationSerial || Ticket!=StageSerial || bEndingAttack) { return false; }
    UAbilitySystemComponent* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    UAnimInstance* Anim=CurrentActorInfo->GetAnimInstance();
    UETCombatTraceComponent* Trace=ActiveTrace.Get();
    UAnimMontage* Montage=RuntimeStages[Index].Montage;
    if (!IsValid(ASC) || !IsValid(Anim) || !IsValid(Trace) || !Trace->CanStartAttack() || !IsValid(Montage)) { return false; }
    ActiveStageIndex=Index;ActiveMontage=Montage;
    ActiveStageDamage=AttackDamage*RuntimeStages[Index].DamageMultiplier;
    UAbilityTask_PlayMontageAndWait* Task=UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this,NAME_None,Montage,1.0f,NAME_None,true,1.0f,0.0f,true);
    if (!Task) { return false; }
    MontageTask=Task;
    Task->OnCompleted.AddDynamic(this,&ThisClass::HandleMontageCompleted);
    Task->OnInterrupted.AddDynamic(this,&ThisClass::HandleMontageAborted);
    Task->OnCancelled.AddDynamic(this,&ThisClass::HandleMontageAborted);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakASC(ASC);
    const TWeakObjectPtr<UAnimInstance> WeakAnim(Anim);
    Task->ReadyForActivation();
    ASC=WeakASC.Get();Anim=WeakAnim.Get();
    if (!IsActive() || Activation!=ActivationSerial || Ticket!=StageSerial)
    {
        // PlayMontage may assign ASC ownership after a synchronous cancellation.
        // Do not touch a new activation of this instanced ability.
        if (!IsActive() && IsValid(ASC) && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==Montage)
        {
            ASC->CurrentMontageStop(0.0f);
            if (!IsActive() && IsValid(ASC) && ASC->GetAnimatingAbility()==this && ASC->GetCurrentMontage()==Montage)
            { ASC->ClearAnimatingAbility(this); }
        }
        return false;
    }
    FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetActiveInstanceForMontage(Montage) : nullptr;
    if (!IsValid(ASC) || !Instance || !Anim->Montage_IsPlaying(Montage) || MontageTask!=Task || ASC->GetAnimatingAbility()!=this) { return false; }
    ActiveMontageInstanceID=Instance->GetInstanceID();
    ActiveTraceSerial=Trace->BeginAttack(Montage,ActiveMontageInstanceID);
    bStartingStage=false;
    if (ActiveTraceSerial==0) { return false; }
    UE_LOG(LogETLightAttack,Display,TEXT("[M2-03B] Stage begin Body=%s Stage=%d/%d Ticket=%u TraceSerial=%u Damage=%.1f Montage=%s"),
        *GetNameSafe(GetAvatarActorFromActorInfo()),Index+1,RuntimeStages.Num(),Ticket,ActiveTraceSerial,ActiveStageDamage,*GetNameSafe(Montage));
    return true;
}

void UETGA_LightAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    ++ActivationSerial;if (ActivationSerial==0) { ++ActivationSerial; }
    const uint32 Activation=ActivationSerial;
    bEndingAttack=false;bStartingStage=true;bTransitionPending=false;ClearBufferedInput();
    ActiveStageIndex=INDEX_NONE;RuntimeStages.Reset();
    AEmberTrialCharacter* Body=ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UETCombatTraceComponent* Trace=IsValid(Body) ? Body->FindComponentByClass<UETCombatTraceComponent>() : nullptr;
    if (!IsValid(Body) || Body->IsCombatDead() || !IsValid(Trace) || !Trace->CanStartAttack() ||
        !HasValidDamageConfiguration() || !HasValidComboConfiguration() || !IsValid(AttackMontage))
    { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    FETLightAttackStage First;First.Montage=AttackMontage;First.DamageMultiplier=1.0f;
    RuntimeStages.Add(First);
    if (bEnableCombo) { RuntimeStages.Append(ComboFollowUps); }
    const bool bCommitted=CommitAbility(Handle,ActorInfo,ActivationInfo);
    if (!IsActive() || Activation!=ActivationSerial) { return; }
    if (!bCommitted) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    Body=Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    if (!IsValid(Body) || Body->IsCombatDead()) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    Body->StopJumping();Body->ConsumeMovementInputVector();
    if (UCharacterMovementComponent* Movement=Body->GetCharacterMovement()) { Movement->StopMovementImmediately(); }
    ActiveTrace=Trace;TraceHitHandle=Trace->OnTraceHit.AddUObject(this,&ThisClass::HandleTraceHit);
    const bool bStarted=StartStage(0);
    if (!IsActive() || Activation!=ActivationSerial) { return; }
    if (!bStarted) { EndAbility(Handle,ActorInfo,ActivationInfo,false,true);return; }
    UE_LOG(LogETLightAttack,Display,TEXT("[M1-04A] Begin Character=%s Ability=%s Montage=%s"),
        *GetNameSafe(Body),*GetNameSafe(this),*GetNameSafe(AttackMontage));
}

void UETGA_LightAttack::HandleMontageCompleted()
{
    EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,false);
}
void UETGA_LightAttack::HandleMontageAborted()
{
    EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,false,true);
}
void UETGA_LightAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,bool bWasCancelled)
{
    if (!IsEndAbilityValid(Handle,ActorInfo) || bEndingAttack) { return; }
    if (ScopeLockCount>0)
    { Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);return; }
    bEndingAttack=true;++ActivationSerial;++StageSerial;
    if (UWorld* World=GetWorld()) { World->GetTimerManager().ClearTimer(TransitionTimer); }
    TransitionTimer.Invalidate();bTransitionPending=false;bStartingStage=false;ClearBufferedInput();
    if (UETCombatTraceComponent* Trace=ActiveTrace.Get()) { Trace->OnTraceHit.Remove(TraceHitHandle); }
    TraceHitHandle.Reset();
    ClearStageResources();ActiveTrace.Reset();ActiveStageIndex=INDEX_NONE;RuntimeStages.Reset();
    const FString BodyName=GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
    const TWeakObjectPtr<UAbilitySystemComponent> ASC=ActorInfo ? ActorInfo->AbilitySystemComponent : TWeakObjectPtr<UAbilitySystemComponent>();
    Super::EndAbility(Handle,ActorInfo,ActivationInfo,bReplicateEndAbility,bWasCancelled);
    // No instance-field resets here: an End event may already have started a new activation.
    UE_LOG(LogETLightAttack,Display,TEXT("[M1-04A] End Character=%s Cancelled=%d AttackingTag=%d"),
        *BodyName,bWasCancelled ? 1 : 0,ASC.IsValid() && ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ? 1 : 0);
}

void UETGA_LightAttack::HandleTraceHit(uint32 Serial, const FHitResult& Hit)
{
    if (!IsActive() || Serial == 0 || Serial != ActiveTraceSerial || !CurrentActorInfo)
    {
        return;
    }

    const FString TargetName = GetNameSafe(Hit.GetActor());
    const auto Reject = [Serial, &TargetName](const TCHAR* Reason)
    {
        UE_LOG(LogETLightAttack, Display,
            TEXT("[M1-04B2] Damage skipped Serial=%u Target=%s Reason=%s"),
            Serial, *TargetName, Reason);
    };

    AEmberTrialCharacter* Source =
        Cast<AEmberTrialCharacter>(CurrentActorInfo->AvatarActor.Get());
    UAbilitySystemComponent* SourceASC = CurrentActorInfo->AbilitySystemComponent.Get();
    float SourceHealth = 0.0f;
    float SourceMaxHealth = 0.0f;
    if (!IsValid(Source) || Source->IsCombatDead() || !Source->HasAuthority() || !IsValid(SourceASC) ||
        CurrentActorInfo->OwnerActor.Get() != Source ||
        Source->GetAbilitySystemComponent() != SourceASC ||
        !Source->GetGameplayHealthSnapshot(SourceHealth, SourceMaxHealth) ||
        !FMath::IsFinite(SourceHealth) || !FMath::IsFinite(SourceMaxHealth) ||
        SourceHealth <= 0.0f || SourceMaxHealth < 1.0f ||
        SourceHealth > SourceMaxHealth || UGameplayStatics::IsGamePaused(Source))
    {
        Reject(TEXT("SourceUnavailable"));
        return;
    }
    if (!HasValidDamageConfiguration() || !FMath::IsFinite(ActiveStageDamage) || ActiveStageDamage <= 0.0f)
    {
        Reject(TEXT("InvalidDamageConfig"));
        return;
    }

    AEmberTrialCharacter* Target = Cast<AEmberTrialCharacter>(Hit.GetActor());
    if (!IsValid(Target) || Target == Source)
    {
        Reject(TEXT("UnsupportedTarget"));
        return;
    }

    if (!Source->CanDealCombatDamageTo(Target))
    {
        Reject(TEXT("CombatTeam"));
        return;
    }

    if (Target->IsCombatDead())
    {
        Reject(TEXT("TargetDead"));
        return;
    }

    float HealthBefore = 0.0f;
    float MaxHealthBefore = 0.0f;
    if (!Target->GetGameplayHealthSnapshot(HealthBefore, MaxHealthBefore) ||
        !FMath::IsFinite(HealthBefore) || !FMath::IsFinite(MaxHealthBefore) ||
        MaxHealthBefore < 1.0f || HealthBefore < 0.0f || HealthBefore > MaxHealthBefore)
    {
        Reject(TEXT("TargetNotReady"));
        return;
    }
    if (HealthBefore <= 0.0f)
    {
        Reject(TEXT("TargetZeroHealth"));
        return;
    }

    UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
    if (!IsValid(TargetASC) || !TargetASC->AbilityActorInfo.IsValid() ||
        TargetASC->AbilityActorInfo->OwnerActor.Get() != Target ||
        TargetASC->AbilityActorInfo->AvatarActor.Get() != Target ||
        !TargetASC->HasAttributeSetForAttribute(UETHealthAttributeSet::GetHealthAttribute()))
    {
        Reject(TEXT("TargetASCUnavailable"));
        return;
    }

    const FString SourceName = GetNameSafe(Source);
    const float RequestedDamage = ActiveStageDamage;
    const TWeakObjectPtr<AEmberTrialCharacter> WeakTarget(Target);
    const TWeakObjectPtr<UAbilitySystemComponent> WeakTargetASC(TargetASC);

    FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    if (!Context.IsValid())
    {
        Reject(TEXT("ContextUnavailable"));
        return;
    }
    Context.AddSourceObject(this);
    Context.AddHitResult(Hit, true);
    FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(
        DamageEffectClass, GetAbilityLevel(), Context);
    if (!Spec.IsValid())
    {
        Reject(TEXT("SpecUnavailable"));
        return;
    }
    Spec.Data->SetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_HealthDelta, -RequestedDamage);

    // This shared player/enemy combat entry checks current tags at actual submission time.
    // Diagnostic direct GE / SetNumericAttributeBase calls intentionally bypass this policy.
    if (TargetASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable))
    {
        Reject(TEXT("Invulnerable"));
        return;
    }

    const FActiveGameplayEffectHandle Result = SourceASC->ApplyGameplayEffectSpecToTarget(
        *Spec.Data.Get(), TargetASC);
    const bool bApplied = Result.WasSuccessfullyApplied();

    // Applying an effect can synchronously cancel abilities or destroy actors.
    // Do not reuse CurrentActorInfo or raw actor pointers after the call.
    float HealthAfter = 0.0f;
    float MaxHealthAfter = 0.0f;
    AEmberTrialCharacter* LiveTarget = WeakTarget.Get();
    UAbilitySystemComponent* LiveTargetASC = WeakTargetASC.Get();
    if (!IsValid(LiveTarget) || !IsValid(LiveTargetASC) ||
        LiveTarget->GetAbilitySystemComponent() != LiveTargetASC ||
        !LiveTarget->GetGameplayHealthSnapshot(HealthAfter, MaxHealthAfter))
    {
        UE_LOG(LogETLightAttack, Display,
            TEXT("[M1-04B2] Damage Serial=%u Source=%s Target=%s Requested=%.1f Applied=%d ActiveHandle=%d Snapshot=Unavailable"),
            Serial, *SourceName, *TargetName, RequestedDamage,
            bApplied ? 1 : 0, Result.IsValid() ? 1 : 0);
        return;
    }

    const float BaseAfter = LiveTargetASC->GetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute());
    UE_LOG(LogETLightAttack, Display,
        TEXT("[M1-04B2] Damage Serial=%u Source=%s Target=%s Requested=%.1f Applied=%d ActiveHandle=%d Before=%.1f After=%.1f BaseAfter=%.1f Max=%.1f Actual=%.1f"),
        Serial, *SourceName, *TargetName, RequestedDamage,
        bApplied ? 1 : 0, Result.IsValid() ? 1 : 0,
        HealthBefore, HealthAfter, BaseAfter, MaxHealthAfter,
        FMath::Max(0.0f, HealthBefore - HealthAfter));
}
```

## 5. 其他四个既有文件：精确局部修改

### 5.1 Character.cpp：完整替换 RequestLightAttack

文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp`。只替换从`bool AEmberTrialCharacter::RequestLightAttack()`到它对应结尾大括号；下一函数`GetLightAttackSpecHandle()`及其后保持。

```cpp
bool AEmberTrialCharacter::RequestLightAttack()
{
    if (!IsGameplayReady() || IsCombatDead() || bAbilitySystemShuttingDown ||
        !LightAttackAbilityHandle.IsValid())
    {
        UE_LOG(LogETAbilityFoundation, Display,
            TEXT("[M1-04A] Request rejected Character=%s Reason=NotReadyOrNotGranted"),
            *GetNameSafe(this));
        return false;
    }
    const FGameplayAbilitySpec* Spec =
        AbilitySystemComponent->FindAbilitySpecFromHandle(LightAttackAbilityHandle);
    if (!Spec)
    {
        UE_LOG(LogETAbilityFoundation, Display,
            TEXT("[M1-04A] Request rejected Character=%s Reason=MissingSpecOrBusy"),
            *GetNameSafe(this));
        return false;
    }

    if (Spec->IsActive())
    {
        UETGA_LightAttack* Attack=Cast<UETGA_LightAttack>(Spec->GetPrimaryInstance());
        const bool bBuffered=IsValid(Attack) && Attack->TryBufferComboInput();
        UE_LOG(LogETAbilityFoundation,Display,TEXT("[M2-03B] Active attack input Character=%s Accepted=%d"),
            *GetNameSafe(this),bBuffered ? 1 : 0);
        return bBuffered;
    }

    const bool bAccepted = AbilitySystemComponent->TryActivateAbility(
        LightAttackAbilityHandle, false);
    UE_LOG(LogETAbilityFoundation, Display,
        TEXT("[M1-04A] Request Character=%s Accepted=%d"),
        *GetNameSafe(this), bAccepted ? 1 : 0);
    return bAccepted;
}
```

活动输入的Accepted=1表示已接受一条缓冲，不等于立即进入下一段或造成伤害；没有对活动Spec重复TryActivate。旧空闲入口 / Ready / 死亡 / 结束保护保留。

### 5.2 Character.h：只加一行friend

在该文件已有以下行后面：

```cpp
friend class FETDodgeInvulnerabilityTest;
```

紧接着新增：

```cpp
friend class FETLightComboFlowTest;
```

保留Stamina / Dodge等全部旧friend、属性和函数。

### 5.3 CombatLife.h：同样只加一行friend

在`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Combat\ETCombatLifeComponent.h`的private区，已有`friend class FETDodgeInvulnerabilityTest;`下一行新增同一个`friend class FETLightComboFlowTest;`。不改受击类属性、死亡逻辑或cpp；测试用它给临时身体选择正式受击类。

### 5.4 Editor Validator.cpp：插入连段规则

在`ValidateAbility`中找到**唯一**一行：

```cpp
    if (!bAttack) { return; }
```

用下面完整段落替换这一行；它后面的`const TOptional<double> Damage = ...`等旧数值 / GE校验继续保留，前面Dodge分支完全保留。

```cpp
if (!bAttack) { return; }
    const UETGA_LightAttack* Attack=CastChecked<UETGA_LightAttack>(Defaults);
    if (!Attack->HasValidComboConfiguration())
    {
        AddIssue(Blueprint,TEXT("ET-COMBO-001"),
            TEXT("Combo requires two distinct follow-ups, finite buffer .05...50, same skeleton / DefaultSlot / no root motion, one hit state each, one later combo state only on stages 1/2 before blend-out."),Issues);
    }
    const FString Package=Blueprint->GetOutermost()->GetName();
    if ((Package==TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack") && !Attack->IsComboEnabled()) ||
        (Package==TEXT("/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack") && Attack->IsComboEnabled()))
    {
        AddIssue(Blueprint,TEXT("ET-COMBO-002"),TEXT("Formal player requires Combo enabled; formal enemy must keep Combo disabled."),Issues);
    }
```

该cpp原本已include ETGA_LightAttack，不重复添加。ET-COMBO-001共用Runtime谓词，ET-COMBO-002明确玩家启用 / 敌人禁用；原Validation Probe路径不强制启用玩家连段，旧负例规则保留。

## 6. 新增长期真实连段自动化

新文件：`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETLightComboFlowTests.cpp`。完整内容如下。

测试加载保存后的玩家 / 敌人GA与真实Montage，临时目标生命1000，不改变地图 / 正式默认值。每次Tick把目标放在当前右手附近以隔离几何距离变化，仍由真实Notify / Sweep / GE提交命中；这验证时序 / 去重 / 伤害链，不冒充敌人AI或完整手感。没有伪造FHitResult、直接调用伤害回调或手动广播成功通知；仅故意发送旧窗口结束 / 旧Serial清理作为负例。

精确时机由有上限的Until观察实际Montage位置 / 段索引安排，不要求你赶.25秒，也不依赖.16秒这类紧贴Timer边界的断言。暂停、待切段受击 / 死亡 / EndPlay、旧段清理和五次输入只存一条都在测试中驱动。另用强引用保留已结束Task，在新一轮使用同名Montage时投递旧回调，检查新ASC归属和Trace未被清空；这与旧窗口 / Serial清理同属负向注入，不伪造正向命中。代码不先写成Success，需完成第8节资产装配后实际运行。

```cpp
#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "UObject/StrongObjectPtr.h"
#include "Animation/ETANS_ComboWindow.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Combat/ETCombatTraceComponent.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETLightComboFlowTest,
    "EmberTrial.M2.Combo.Flow",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETLightComboFlowTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* AttackClass=LoadClass<UETGA_LightAttack>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* EnemyClass=LoadClass<UETGA_LightAttack>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack.GA_ET_EnemyLightAttack_C"));
    UClass* HitClass=LoadClass<UETGA_HitReact>(nullptr,TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    USkeletalMesh* Quinn=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved player GA"),AttackClass) || !TestNotNull(TEXT("Saved enemy GA"),EnemyClass) ||
        !TestNotNull(TEXT("Hit GA"),HitClass) || !TestNotNull(TEXT("Quinn"),Quinn)) { return false; }
    const UETGA_LightAttack* Defaults=AttackClass->GetDefaultObject<UETGA_LightAttack>();
    if (!TestTrue(TEXT("Player combo configured"),Defaults->IsComboEnabled() && Defaults->HasValidComboConfiguration()) ||
        !TestFalse(TEXT("Enemy stays single stage"),EnemyClass->GetDefaultObject<UETGA_LightAttack>()->IsComboEnabled())) { return false; }

    for (const bool bEndPlayOnly : {false,true})
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
        auto SpawnBody=[World,Quinn]()
        {
            FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AEmberTrialCharacter* Body=World->SpawnActor<AEmberTrialCharacter>(FVector(0,0,200),FRotator::ZeroRotator,P);
            if (!Body) { return Body; }
            Body->GetMesh()->SetSkeletalMeshAsset(Quinn);Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
            Body->GetMesh()->SetRelativeLocation(FVector(0,0,-90));Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Body->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
            Body->GetCharacterMovement()->bRunPhysicsWithNoController=true;
            return Body;
        };
        AEmberTrialCharacter* Body=SpawnBody();AEmberTrialCharacter* Target=SpawnBody();
        APlayerController* PC=World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Player body"),Body) || !TestNotNull(TEXT("Target"),Target) || !TestNotNull(TEXT("Controller"),PC)) { return false; }
        Body->CombatTeam=EETCombatTeam::Player;Body->LightAttackAbilityClass=AttackClass;
        Body->CombatLifeComponent->HitReactAbilityClass=HitClass;
        Target->CombatTeam=EETCombatTeam::Neutral;Target->InitialMaxHealth=1000;Target->InitialHealth=1000;
        Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Overlap);
        PC->Possess(Body);
        if (!Fixture.BeginPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        auto Step=[&Fixture,Body,Target]()
        {
            // Geometry fixture: stay in reach of each different right-hand pose.
            // This still uses real montage notifies, world sweeps and the production GE callback.
            const FVector Hand=Body->GetMesh()->GetSocketLocation(FName(TEXT("hand_r")));
            Target->SetActorLocation(FVector(Hand.X,Hand.Y,Target->GetActorLocation().Z),false,nullptr,ETeleportType::TeleportPhysics);
            return Fixture.TickTestWorld(.01f);
        };
        auto Advance=[&Step](float Seconds)
        {
            for (int32 I=0;I<FMath::CeilToInt(Seconds/.01f);++I) { if (!Step()) { return false; } }
            return true;
        };
        auto Until=[&Step](auto Predicate,float Limit)
        {
            for (int32 I=0;I<FMath::CeilToInt(Limit/.01f) && !Predicate();++I) { if (!Step()) { return false; } }
            return Predicate();
        };
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);Target->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) || !TestTrue(TEXT("Ready grounded body"),Body->IsGameplayReady() && Target->IsGameplayReady() &&
            Body->GetCharacterMovement()->IsMovingOnGround())) { return false; }
        UAbilitySystemComponent* ASC=Body->GetAbilitySystemComponent();
        UAbilitySystemComponent* TargetASC=Target->GetAbilitySystemComponent();
        FGameplayAbilitySpec* Spec=ASC->FindAbilitySpecFromHandle(Body->GetLightAttackSpecHandle());
        UETGA_LightAttack* Attack=Spec ? Cast<UETGA_LightAttack>(Spec->GetPrimaryInstance()) : nullptr;
        UETCombatTraceComponent* Trace=Body->FindComponentByClass<UETCombatTraceComponent>();
        if (!TestNotNull(TEXT("Granted attack instance"),Attack) || !TestNotNull(TEXT("Trace"),Trace)) { return false; }
        const FGameplayAttribute Health=UETHealthAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Stamina=UETStaminaAttributeSet::GetStaminaAttribute();
        TMap<uint32,int32> Candidates;
        const FDelegateHandle Watch=Trace->OnTraceHit.AddLambda([&Candidates,Target](uint32 Serial,const FHitResult& Hit)
            { if (Hit.GetActor()==Target) { ++Candidates.FindOrAdd(Serial); } });
        auto Position=[Attack,Body]()
        {
            UAnimInstance* Anim=Body->GetMesh()->GetAnimInstance();
            FAnimMontageInstance* Instance=IsValid(Anim) ? Anim->GetMontageInstanceForID(Attack->ActiveMontageInstanceID) : nullptr;
            return Instance ? Instance->GetPosition() : -1.0f;
        };
        auto Start=[this,Body,Attack]()
        { return TestTrue(TEXT("Start fresh attack"),Body->RequestLightAttack()) && TestTrue(TEXT("Starts stage one"),Attack->ActiveStageIndex==0); };
        auto Idle=[&Until,&Advance,Attack]()
        { return Until([Attack](){return !Attack->IsActive();},2.0f) && Advance(.25f); };
        auto Reset=[&Candidates,TargetASC,Health]() { Candidates.Reset();TargetASC->SetNumericAttributeBase(Health,1000.0f); };
        auto CheckDamage=[this,&Candidates,TargetASC,Health](int32 Count)
        {
            TestEqual(TEXT("Unique serial count"),Candidates.Num(),Count);
            for (const auto& Pair : Candidates) { TestEqual(TEXT("One candidate per serial"),Pair.Value,1); }
            TestEqual(TEXT("Real current health"),TargetASC->GetNumericAttribute(Health),1000.0f-25.0f*Count);
            TestEqual(TEXT("Real base health"),TargetASC->GetNumericAttributeBase(Health),1000.0f-25.0f*Count);
        };
        if (bEndPlayOnly)
        {
            if (!Start() || !Until([Attack](){return Attack->bComboWindowOpen;},1.0f) || !Body->RequestLightAttack()) { return false; }
            const FTimerHandle Pending=Attack->TransitionTimer;
            TestTrue(TEXT("Transition pending before EndPlay"),Attack->bTransitionPending);
            if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
            TestFalse(TEXT("EndPlay cancels attack"),Attack->IsActive());
            TestFalse(TEXT("EndPlay clears pending input"),Attack->bHasBufferedInput || Attack->bTransitionPending);
            TestFalse(TEXT("EndPlay clears owned timer"),World->GetTimerManager().IsTimerActive(Pending));
            TestEqual(TEXT("EndPlay disarms trace"),Attack->ActiveTraceSerial,uint32(0));
            Fixture.ForwardErrorMessages(this);if (HasAnyErrors()) { return false; }
            continue;
        }

        Reset();if (!Start() || !Idle()) { return false; }CheckDamage(1);
        Reset();if (!Start() || !Advance(.05f) || !Body->RequestLightAttack() ||
            !Until([Attack](){return Attack->bComboWindowOpen;},1.0f)) { return false; }
        TestEqual(TEXT("Expired early click stays in first stage"),Attack->ActiveStageIndex,0);
        TestFalse(TEXT("Expired input cleared"),Attack->bHasBufferedInput || Attack->bTransitionPending);
        if (!Idle()) { return false; }CheckDamage(1);

        Reset();if (!Start() || !Until([&Position](){return Position()>=.48f;},1.0f)) { return false; }
        const uint32 FirstTrace=Attack->ActiveTraceSerial;
        UAnimMontage* OldMontage=Attack->ActiveMontage;const int32 OldID=Attack->ActiveMontageInstanceID;
        UAnimNotifyState* OldToken=nullptr;
        for (const FAnimNotifyEvent& Event : OldMontage->Notifies)
        { if (Event.NotifyStateClass && Event.NotifyStateClass->IsA<UETANS_ComboWindow>()) { OldToken=Event.NotifyStateClass; } }
        if (!Body->RequestLightAttack() || !Until([Attack](){return Attack->ActiveStageIndex==1;},.6f)) { return false; }
        TestTrue(TEXT("Stage two keeps Attacking"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
        TestTrue(TEXT("New serial for stage two"),Attack->ActiveTraceSerial!=FirstTrace);
        const uint32 SecondTrace=Attack->ActiveTraceSerial;
        Attack->HandleComboWindow(Body->GetMesh(),OldMontage,OldID,OldToken,false);
        Trace->EndAttack(FirstTrace); // Deliberately stale cleanup must not disarm stage two.
        TestEqual(TEXT("Stale combo end ignored"),Attack->ActiveTraceSerial,SecondTrace);
        if (!Until([&Position](){return Position()>=.48f;},1.0f) || !Body->RequestLightAttack() ||
            !Until([Attack](){return Attack->ActiveStageIndex==2;},.6f)) { return false; }
        TestTrue(TEXT("New serial for stage three"),Attack->ActiveTraceSerial!=SecondTrace);
        TestFalse(TEXT("No fourth stage input"),Body->RequestLightAttack());
        if (!Idle()) { return false; }CheckDamage(3);
        TestEqual(TEXT("Light combo does not spend stamina"),ASC->GetNumericAttribute(Stamina),100.0f);
        AddInfo(TEXT("[M2-03B] Real combo 1->2->3: three unique traces, Health1000->925"));

        // Retain a finished task and deliver stale native callbacks after a fresh activation.
        // Negative injection only: positive damage still comes from real notifies and sweeps.
        Reset();if (!Start() || !Advance(.05f)) { return false; }
        TStrongObjectPtr<UAbilityTask_PlayMontageAndWait> HeldOldTask(Attack->MontageTask.Get());
        UAnimMontage* RetiredMontage=Attack->ActiveMontage;
        if (!TestNotNull(TEXT("Retained old task"),HeldOldTask.Get())) { return false; }
        Body->CancelLightAttack();
        TestFalse(TEXT("Old activation cancelled"),Attack->IsActive());
        TestTrue(TEXT("Finished task drops old ability owner"),HeldOldTask->Ability==nullptr);
        if (!Advance(.25f) || !Start()) { return false; }
        const uint32 FreshTrace=Attack->ActiveTraceSerial;
        HeldOldTask->OnMontageBlendingOut(RetiredMontage,true);
        HeldOldTask->OnMontageEnded(RetiredMontage,true);
        TestTrue(TEXT("Stale task keeps new activation alive"),Attack->IsActive());
        TestTrue(TEXT("Stale task cannot clear new ASC ownership"),ASC->GetAnimatingAbility()==Attack);
        TestTrue(TEXT("New activation reuses first montage"),ASC->GetCurrentMontage()==RetiredMontage);
        TestEqual(TEXT("Stale task leaves fresh trace intact"),Attack->ActiveTraceSerial,FreshTrace);
        TestEqual(TEXT("Fresh activation stays in stage one"),Attack->ActiveStageIndex,0);
        if (!Idle()) { return false; }CheckDamage(1);

        Reset();if (!Start() || !Until([&Position](){return Position()>=.48f;},1.0f)) { return false; }
        for (int32 I=0;I<5;++I) { TestTrue(TEXT("Burst updates one slot"),Body->RequestLightAttack()); }
        if (!Until([Attack](){return Attack->ActiveStageIndex==1;},.6f)) { return false; }
        TestFalse(TEXT("Consumed slot empty"),Attack->bHasBufferedInput);
        if (!Idle()) { return false; }CheckDamage(2);

        Reset();if (!Start() || !Until([Attack](){return Attack->bComboWindowOpen;},1.0f) || !Body->RequestLightAttack()) { return false; }
        TestTrue(TEXT("Queued transition is deferred"),Attack->bTransitionPending);
        const FTimerHandle CancelTimer=Attack->TransitionTimer;
        if (!TestTrue(TEXT("Formal hit reaction cancels queued combo"),Body->RequestHitReact())) { return false; }
        TestFalse(TEXT("Cancel clears action and input"),Attack->IsActive() || Attack->bHasBufferedInput || Attack->bTransitionPending);
        TestFalse(TEXT("Cancel removes transition timer"),World->GetTimerManager().IsTimerActive(CancelTimer));
        if (!Advance(.15f)) { return false; }
        TestTrue(TEXT("Old montage cannot end new hit reaction"),ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact));
        if (!Until([ASC](){return !ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact);},1.5f) || !Advance(.25f)) { return false; }
        CheckDamage(1);

        Reset();if (!Start() || !Until([&Position](){return Position()>=.48f;},1.0f) || !Body->RequestLightAttack()) { return false; }
        APlayerState* Pauser=World->SpawnActor<APlayerState>();if (!TestNotNull(TEXT("Pauser"),Pauser)) { return false; }
        const float At=Position();const float WorldAt=World->GetTimeSeconds();
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const bool bPauseAdvance=Advance(.50f);
        TestTrue(TEXT("Pause freezes montage"),FMath::IsNearlyEqual(Position(),At,.001f));
        TestTrue(TEXT("Pause freezes buffer clock"),FMath::IsNearlyEqual(World->GetTimeSeconds(),WorldAt,.001f));
        TestFalse(TEXT("Paused input rejected"),Body->RequestLightAttack());
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bPauseAdvance || !Until([Attack](){return Attack->ActiveStageIndex==1;},.6f) || !Idle()) { return false; }
        CheckDamage(2); // Only the valid pre-pause input survives; menu input adds no third stage.

        UAnimMontage* Saved=Attack->ComboFollowUps[0].Montage;
        Attack->ComboFollowUps[0].Montage=nullptr;
        TestFalse(TEXT("Missing follow-up config rejected"),Attack->HasValidComboConfiguration());
        TestFalse(TEXT("Invalid combo cannot start"),Body->RequestLightAttack());
        Attack->ComboFollowUps[0].Montage=Saved;
        TestTrue(TEXT("Restored config"),Attack->HasValidComboConfiguration());

        Reset();if (!Start() || !Until([Attack](){return Attack->bComboWindowOpen;},1.0f) || !Body->RequestLightAttack()) { return false; }
        const FTimerHandle DeathTimer=Attack->TransitionTimer;
        ASC->SetNumericAttributeBase(Health,0.0f);
        TestTrue(TEXT("Death sealed"),Body->IsCombatDead());
        TestFalse(TEXT("Death clears attack and pending input"),Attack->IsActive() || Attack->bHasBufferedInput || Attack->bTransitionPending);
        TestFalse(TEXT("Death removes transition timer"),World->GetTimerManager().IsTimerActive(DeathTimer));
        if (!Advance(.5f)) { return false; }
        TestFalse(TEXT("Dead body cannot restart"),Body->RequestLightAttack());
        Trace->OnTraceHit.Remove(Watch);
        if (!Fixture.EndPlayInTestWorld()) { Fixture.ForwardErrorMessages(this);return false; }
        Fixture.ForwardErrorMessages(this);if (HasAnyErrors()) { return false; }
    }
    return !HasAnyErrors();
}
#endif
```

## 7. 正常 Build、索引与 Game Build

1. 确认九份文件保存，UE保持关闭。PowerShell依次执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

2. 该脚本先Editor Build，再生成CLion索引。成功后在CLion Reload CMake Project，核对Core / UI / Editor三个CodeModel；预计 **36 + 13 + 3 = 52cpp**。新Notify cpp和测试cpp属于Core；UHT / 自动生成.cpp不计入这个手写源码数。
3. 若出现CMake版本 / 目标问题，按既有[CLion创建类指南](CLion_UEClassCreation.md)处理，不删全项目缓存。新头文件最后一个include必须是本文件.generated.h。
4. 再执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

5. 两目标成功再开原.uproject。此时玩家GA还未配置启用连段，不能据此判断新功能失败；继续第8节。失败时停在第一条真正C++ / UHT错误，文字贴文件 / 行号及上下5–10行，`UBT exit code6`本身不是根因。

## 8. 资产装配：先Montage，再GA

### 8.1 新建独立第1段 Montage

1. Content Browser进入`/Game/EmberTrial/Animations/Combo/Montages`，空白处右键 → **Animation → Animation Montage**；选择Skeleton **SK_Mannequin**，命名 **AM_ET_Light_01**。不要选Create Child Montage，也不覆盖旧AM_M0_Attack_01_Montage。
2. 打开新AM，在编辑器右侧Asset Browser搜索 **AS_M0_Attack_01**，核对来源`/Game/EmberTrial/Animations/Practice/Clips`；拖到新Montage已有的DefaultSlot轨道，放在时间0。只一个Segment，不把AS_ET_Light_02拖作第1段。
3. 单击Segment，在Details核对Animation Reference=AS_M0_Attack_01，Start Time=0，End Time=该AS全长约1.033333，Play Rate=1，Loop Count=1；Slot Name=DefaultGroup.DefaultSlot。保留一个Default Section，不接回自身形成循环。
4. Montage Asset Details：Rate Scale=1、Enable Auto Blend Out=True，Blend In → Blend Time=.10，Blend Out → Blend Time=.10。Preview Mesh选Quinn_Simple，预览原前挥正常；不更改旧AS的Root Motion或玩家AnimBP。
5. 新建ET_HitWindow通知轨道，在轨道空白右键 **Add Notify State → ET Attack Window**，只一条。右键状态条本体，`Notify Begin Time=.266667` Enter，再右键`Anim Notify State Duration=.200000` Enter；开始帧8 / 持续帧6 / 结束帧14。Montage Tick Type=Branching Point，Save。

### 8.2 玩家第1 / 2段各加一个连段窗口

对下表两份AM分别做同样步骤：

| Montage | 原命中状态 | 新连段状态 |
| --- | --- | --- |
| Combo/Montages/AM_ET_Light_01 | ET Attack Window：8–14帧，保留 | ET Combo Window：18–24帧 |
| Combo/Montages/AM_ET_Light_02 | A已配的8–14帧，保留 | ET Combo Window：18–24帧 |
| Combo/Montages/AM_ET_Light_03 | A已配的6–12帧，保留 | **没有连段窗口** |

1. 在AM的Notifies下增加另一条Notify Track，命名 **ET_ComboWindow**，避免与命中状态重叠看错。
2. 轨道空白约.60秒处右键 → **Add Notify State → ET Combo Window**。这是第3节新原生类，不选ET Attack Window或普通Notify；找不到类先查本次Build，不临时新建蓝图代替。
3. 单选新状态条，右键**中间本体**：`Notify Begin Time=.600000`，Enter；再右键`Anim Notify State Duration=.200000`，Enter。复核`Notify Frame=18`、`Anim Notify State Frames=6`；Duration不是End，不填.80。
4. 在Details把Montage Tick Type设为Branching Point（原生类也会强制）；时间在右键菜单，不在Details。菜单只有Add Notify表示点到空白；改值后必须Enter。
5. 每份AM应有**一条命中 + 一条连段**。Save后关闭重开，核对.60 / .20。第3段只核对已有命中，不加ET Combo Window，也不修改其.700秒长度。
6. 旧Practice的AM_M0_Attack_01_Montage不动，敌人继续用它。新Notify不在AnimBP接事件，不放到AS或LS，也不在通知对象里加角色变量。

### 8.3 玩家 GA Class Defaults：按字段装配

1. 打开`/Game/EmberTrial/Abilities/GA_ET_LightAttack`，**Class Defaults**，Parent仍ETGA_LightAttack。Event Graph保持空，不加事件业务。
2. EmberTrial | Attack：**Attack Montage换成刚建的AM_ET_Light_01**，必须确认是Combo/Montages路径；Attack Damage保持25，Attack Max Horizontal Speed=3，Damage Effect Class仍GE_ET_LightAttackDamage。不要换成GE_ET_DodgeCost。
3. EmberTrial | Combo：勾选 **Enable Combo**，**Combo Buffer Seconds=.25**。
4. 展开 **Combo Follow Ups**，点击数组右侧`+`两次，使数组只有 **2个元素**。展开每个元素，按下表填：

| 数组元素 | 代表段落 | Montage | Damage Multiplier |
| --- | --- | --- | ---: |
| Element 0 / Index 0 | 第2段 | AM_ET_Light_02（Combo/Montages） | 1.0 |
| Element 1 / Index 1 | 第3段 | AM_ET_Light_03（Combo/Montages） | 1.0 |

第1段使用上面的Attack Montage，不在数组再放一次。变量类型是Montage Object Reference，不是Class / AS；不把三份AM串到同一Montage里。两个数组元素不能重复。

5. Cost Gameplay Effect Class / Cooldown Class保持原None，本批不增加轻攻击成本。Compile → Save，Content Browser右键该GA → Asset Actions → Validate Assets，应有`[M1-07C2] Valid / Issues0`。
6. ET-COMBO-001说明段数组、时长 / Skeleton / Slot / Root Motion或通知契约有缺项；先按8.1–8.3逐项核对，不删除规则。ET-COMBO-002说明玩家Enable Combo未开或敌人误开。

### 8.4 敌人 GA 与角色

1. 打开`GA_ET_EnemyLightAttack`的Class Defaults：**Enable Combo=False**，Combo Follow Ups保持空；Attack Montage仍旧AM_M0_Attack_01_Montage，Attack Damage=10，原GE / 速度不变。Compile → Save → 项目Valid / Issues0。
2. BP_ThirdPersonCharacter的Light Attack Ability Class仍GA_ET_LightAttack；不重新挂组件或改父类。Compile / Save，单个ASC / Trace / Life / Recovery保持。
3. 玩家Mesh / Anim Class / Always Tick Pose等沿用既有配置，不重新编辑ABP_Manny / Quinn；GA_ET_Dodge仍.15 / 150 / .30 / Cost25。

## 9. 蓝图输入：沿用请求入口，不新接连段图

**正常情况下不需要新增蓝图节点。**左键既有IA_ET_LightAttack、数字1既有别名，都应使用Started，经过已有暂停 / UI允许门控，最终只到`Request Light Attack(Target=Self)`。Triggered / Completed / Canceled仍不连业务，Return Value可以留空。

```text
IA_ET_LightAttack.Started
    → 原来已验收的暂停 / UI门控的“允许游戏输入”白线
        → Request Light Attack（Target = Self）
```

本批不重建旧暂停变量或门控。**攻击请求前不要再用Is Formal Action Idle作额外Branch**：它在攻击期间会False，导致第二次输入到不了C++缓冲。若你现有图有这样一层，只把该层前面的白线直接连到原Request执行输入，保留更前面的暂停 / UI门控；原Branch断开即可。E / 移动 / 跳跃各自的Idle检查继续保留，不跟着删。

Request的Target是角色Self，不是PracticeAction / ASC / GA类。C++函数会区分“空闲激活”和“当前活动实例收输入”，不在蓝图中自行TryActivateAbility、Get Primary Instance、Delay或记连段索引。若自动化可连但实际左键只有单段，截图Started到Request的完整局部白线与Target即可，不先搭临时按键。

## 10. 分组检查（约10–15分钟）

### 10.1 两项自动化，约2–4分钟

不Play，Output Log先运行：

```text
Automation RunTests EmberTrial.M2.Combo
```

等 **Flow Success** 后再运行：

```text
Automation RunTests EmberTrial.M2.Dodge.Invulnerability
```

只复用这一项旧真实命中测试，因为本批改共享LightAttack；不重跑未变的CostContract / Movement / Resources / Practice全套。Flow检查新连段 / 缓冲，旧Invulnerability同时验证敌人单段10与无敌拒绝仍成立。

Flow应有`[M2-03B] Real combo 1->2->3: three unique traces, Health1000->925`。这组临时目标1000不代表正式地图配置变了。它还覆盖单次 / 过期 / 一槽五次输入只多打一段 / 无第4段、旧窗口 / Serial清理、已结束Task的迟到回调隔离、受击取消待调度、暂停前缓冲保留 / 菜单输入拒绝、非法配置 / 死亡 / EndPlay；不是性能基准。

失败时提供测试名、**第一项Expected失败**和大致时间 / 日志路径，我读取真实时序。不要删断言或人为加长缓冲来“凑成功”；也不要求你在手动测试中掐精确.25秒。

### 10.2 玩家请求 / 连击，约2–3分钟

- **操作：**正式地图Entry空地Play。只点一次让第1段结束；再长按一次；再按正常节奏逐次点击打1→2→3。数字1仍能作为同一请求入口。
- **预期：**单点 / 长按只一段；有节奏追加能三段，三段姿态 / 时长有区别；日志每段`Stage begin`，整链末尾只一次`End / AttackingTag0`，段间无移动 / E / 跳跃解锁。轻攻击不扣体力，镜头仍可转。
- **不易把握节奏时：**仅临时`slomo .25`放慢，观察日志Stage begin后在动作中后部追加；具体早过期 / 单槽契约已由自动化验证，不做十轮手动时序矩阵。本组末尾恢复`slomo 1`。

### 10.3 一次真实三段命中，约1–2分钟

- **操作：**靠近仍100血的训练目标，站定保持在右手范围内打完整三段，结束后不继续点。
- **预期：**三段各一次25，目标100→75→50→25；每段Trace Serial不同，同段多次Sweep没有重复扣。新段允许重新命中同目标；不要求本批再摆双目标 / 墙坡 / 帧率矩阵。
- **若没打满：**先看是否已有Stage2 / 3日志，再看各段Candidate与距离；占位动画轨迹不同，空挥不是强行改去重 / 提高范围的理由。给目标位置 / 当时状态和短视频，我定位；不要改正式25cm半径掩盖问题。

### 10.4 取消与P，约2–3分钟

- **操作A：**在任意段按3，等正式受击结束再点左键。再开一条链，在第1或2段按2。
- **预期A：**3取消当前链，Trace / 输入 / 待切段清空，受击结束新请求从1开始；2本批拒绝是预期，C后续才开放恢复取消。
- **操作B：**本次只单击一次左键、不要预缓存下一段，在动作中P暂停；菜单停留约2秒，点背景 / 按1，再用一种已支持方式继续。
- **预期B：**原动作恢复并结束，不把菜单输入补成下一段，不补跳 / 卡键。若暂停前已合法缓存，恢复后消费那一条是既定规则，自动化已单测；不要把它与菜单输入混淆。

### 10.5 敌人与结束短基线，约2分钟

- **操作：**让敌人正常接近挥击一次，观察普通10点扣血 / HUD与受击；再于空地开始攻击并Stop，重开一次。
- **预期：**敌人仍单段，原动作 / 无敌规则正常；新身体生命 / 体力100、单HUD，新攻击从1开始，无旧链续播。最后E一次、P一种继续方式、移动转向跳跃正常即可。
- **恢复：**slomo1、玩家25 / 敌人10、100/100、GA数组2元素 / 倍率1、Buffer.25、Dodge原配置、Debug False；无新增临时箱 / 测试Actor，Source / GA / Montage / 蓝图Save All。当前不Cook，不重做Retry / Continue或全部UI旧矩阵。

## 11. 完成反馈与报错

```text
M2-03B 完成 / 卡在第__节
三新 / 六旧Source；关闭UE后Editor Build / 索引52cpp / Game Build：
独立AM_ET_Light_01 / 原AS；旧共用AM未改：
玩家1/2命中与连段双状态、第三段只命中；.60/.20与8/6帧命中：
玩家Enable Combo / .25 / 两元素02-03与倍率1，25；敌人关闭 / 原AM / 10；Valid：
左键 / 数字1仍Started到角色Request，无额外攻击Idle拦截；Compile / Save：
Combo.Flow / Dodge.Invulnerability各一次Success：
单点 / 长按一段、正常1→2→3、每段独立Serial与真实各25：
3取消 / 结束再从1、2仍拒绝；P菜单输入无补播不卡键：
敌人单段10 / HUD；动作中Stop / 新身体与E-P-基础输入：
slomo1 / 原数值 / Debug False、Source资产Save All：
失败或占位动作最明显问题（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

编译报错用文字贴第一条真实错误；字段 / 通知菜单问题截图GA Class Defaults或状态条右键菜单，包含资产路径。输入问题截图Started到Request的白线，运行问题描述输入 / 当时段落与实际现象；日志给路径或大致时间，我自行提取，不要求手摘几十行。自动化可复现的窄窗口交由测试驱动，不让你重复手动赶时点。

## 12. 本批 Commit / Push（固定放在最后）

- **三新Source**：ETANS_ComboWindow.h / .cpp、ETLightComboFlowTests.cpp；**六旧**：ETGA_LightAttack.h / .cpp、Character.h / .cpp、ETCombatLifeComponent.h、ETGameplayAssetValidator.cpp。保留全部旧文件；Trace / Dodge / Build依赖 / UI无计划修改。
- **一新资产**：`EmberTrial/Content/EmberTrial/Animations/Combo/Montages/AM_ET_Light_01.uasset`；旧GA_ET_LightAttack与AM_ET_Light_02的真实配置差异。Enemy GA / BP_ThirdPersonCharacter只在确有合法保存差异时提交；没有新Input Action / IMC或地图。原Practice共用AM不修改。来源清单验收时新增实物 / 更新真实哈希，当前仍57，预计58不预填。
- 本指南、Progress / Backlog / M2与连击路线 / Architecture / VersionControl / AcceptanceWorkflow / Introduction，以及A报告 / 清单等未提交收尾。2026-10-16只读确认HEAD / 缓存origin/main均2202e74，旧C与A六素材已纳入提交，远端本轮未联网核验。当前仅本指南及同步文档；本批实现完成后重新Stage实际新代码 / 资产，避免已Stage早期骨架遗漏最新内容。
- .25 / 25 / 10 / 100、Dodge.15 / 150 / .30 / Cost25、slomo1 / Debug False恢复后Save All。Raw / 日志 / Binaries / Intermediate / Saved、生成CMake / IDE缓存与历史包仍本地，不Force Add，不修改全局Git设置。

CLion刷新Changes → 重新Stage本批完整Source / 新Montage / 真实资产差异 / 最新文档 → 检查Staged Diff → Commit → Push。可用`feat: add three-stage light combo with bounded input buffer`；仅指南用`docs: prepare light combo runtime guide`。仍由用户操作Git；B通过不代表C恢复闪避取消或整个M2-03已完成，当前仅指南下发。
