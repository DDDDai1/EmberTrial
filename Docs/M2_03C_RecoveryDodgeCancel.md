# M2-03C：恢复阶段闪避取消与战斗集成

2026-10-18清理同步：本页保留历史设计、资产配置、接入差异和验收依据，整文件代码以当前Source链接为准；当前已包含后续连段、闪避与治疗集成，勿把旧步骤整批重做或覆盖现有工程。当前职责见[连段路线页](M2_03_LightComboRoadmap.md)，本次清理提交见[Cleanup](Cleanup.md)。

2026-10-16，固定 **UE5.5.4**。**本批按用户14:15反馈与源码 / 构建 / 测试日志核对验收通过，无待补项。**实际100Source / 53cpp / 来源58，一新 / 七旧、三项Success；C手动40 / 等待5，详见[C报告](Reports/M2_03C_Verification.md)。[M2-03整章](Reports/M2_03_Verification.md)同步通过；下方步骤保留供复查，无需重复执行，下一[M2-04治疗路线](M2_04_HealingRoadmap.md) / [A完整指南](M2_04A_HealingResource.md)已下发待用户操作。

原下发预算（非当前待办）：手动 **90–150分钟**：代码 / 理解60–100、资产 / 输入核对10–20、相关检查10–15、记录5–10，构建等待另列，可分两次完成。原路线C的60–100分钟上调为90–150，原因是本批补齐预检失败保留原攻击、保存标签默认值校验和可复用真实交接测试；不是新增旧功能验收矩阵。整章10h基础预算仍保持，未计本批实耗。

## 当前测试维护边界（第26批）

恢复闪避：保留三段恢复交接、预检失败保留攻击、旧Task强引用负向注入、队列清理、暂停 / 受击 / 死亡 / EndPlay。

全部原断言、日志 / 预期Warning、测试注册与场景顺序保留；期望值独立于生产默认值。当前源码排版 / 注释与命名以Source为准，历史完整实现不重复覆盖。本批改动和验证结果见[第26批文档](Cleanup.md#cleanup-batch-26)；三批提交清单统一见Cleanup末尾。

## 1. 交付规则与实现分工

| 情况 | 结果 |
| --- | --- |
| 任一段前摇 / 命中期间按2 | 拒绝，不扣体力、不缓存闪避，原攻击继续；需到恢复段重新按 |
| 任一段命中结束后、自动淡出开始前按2 | 通过Dodge完整激活预检后取消当前轻攻击，再启动原正式闪避 |
| 第1 / 2段命中8–14帧 | 恢复从14/30约.466667秒开始；到各自Montage自动淡出前结束 |
| 第3段命中6–12帧 | 恢复从12/30=.400000秒开始；同样到自动淡出前结束 |
| 体力不足 / Montage None / 位移参数非法 / 非地面 / 暂停 / 受击 / 死亡 | 激活预检拒绝，不先取消原攻击、不扣成本 |
| 2与已缓存的下一段同处一个恢复时段 | 2若在待切段Timer调度前被接受，清旧输入 / Timer；若下一段已实际开始，则按新段的恢复状态重新判定 |
| 交接成功 | 旧Attacking / Trace / 输入 / Task清理，Dodge只扣一次25，沿用150cm / .30秒与.15秒无敌 |
| Dodge再被3 / 死亡 / Stop取消 | 沿用原清理，不退款、不补播旧连击；下一次左键从1开始 |

恢复时段直接读取**当前Montage唯一命中NotifyState的结束时间**，不另建Notify类、Delay或配置数组。若未来修改命中时间，恢复起点随真实通知移动；自动淡出阶段只等原能力完成，不缓存2。窗口依据Montage位置，不能把渲染帧率或墙钟秒数当动画帧号。

LightAttack只提供无副作用的`CanCancelIntoDodge()`：当前玩家 / 控制权 / Montage实例必须匹配、能力可取消、位置处于恢复时段。Dodge的CanActivate保留成本 / 冷却 / 地面 / 资源 / 输入与配置检查，只在该查询为真时允许原Attacking共存。通过检查后，GAS自身`CancelAbilitiesWithTag=Ability.Attack.Light`在PreActivate里取消原攻击；不在Character或蓝图里先Cancel再“试试看”。

因此Dodge构造中移除Attacking这一条硬阻断，**不等于任意攻击阶段都允许闪避**：原生CanActivate仍有恢复门控；Dodging / HitReact / Dead继续阻断。Activate开头再核对原攻击确实结束、没有其他Montage所有者，之后复用原Montage / RMS准备、一次付款与保护逻辑。普通空闲闪避也继续使用同一入口。

明确边界：预检拒绝不取消原攻击；若预检已通过、交接已开始后发生底层播放失败或同步取消，清理本次准备资源，不自动复活旧攻击。尚未提交则不扣费，成功付款后取消仍不退款；不能将这段流程说成支持所有异常回滚的事务。

接口与顺序已按本机5.5.4的GameplayAbility.cpp `PreActivate` / `CallActivateAbility`、ASC `InternalTryActivateAbility`及AnimInstance.cpp核对；[Epic 5.5能力生命周期与标签说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-gameplay-abilities-in-unreal-engine?application_version=5.5)提供公开契约。静态检查不代替本批实际编译 / 测试。

## 2. 准备与文件清单

Stop PIE → Save All → 关闭UE → CLion保存。完成第3–6节全部八文件，再第7节正常Build；不使用Live Coding处理反射类头文件变更。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Abilities\ETGA_LightAttack.h                   旧，加查询 / friend
├─ EmberTrial\Private\Abilities\ETGA_LightAttack.cpp                旧，加查询函数
├─ EmberTrial\Public\Abilities\ETGA_Dodge.h                         旧，加策略查询 / friend
├─ EmberTrial\Private\Abilities\ETGA_Dodge.cpp                      旧，五处修改
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h               旧，一行friend
├─ EmberTrial\Public\Combat\ETCombatLifeComponent.h                 旧，一行friend
├─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp 旧，加策略规则
└─ EmberTrial\Tests\ETLightComboCancelTests.cpp                       新，完整非UCLASS测试
```

不创建新C++类 / 组件 / GE / Notify / 输入资产；Character.cpp、Trace、位移Task、UI、Build.cs / Target / uproject不改。旧三段动画 / Notify时值保持，唯一需重点核对的是既有GA_ET_Dodge的标签默认值。所有旧friend、函数与属性保留。

## 3. LightAttack：公开恢复查询，不改原连段与伤害

### 3.1 ETGA_LightAttack.h 两处

路径见第2节Public/Abilities。第一处在public区：

**找到原内容：**

```cpp
bool TryBufferComboInput();
```

**替换为：**

```cpp
bool TryBufferComboInput();
    bool CanCancelIntoDodge() const;
```

第二处在private区，保留旧Flow friend：

**找到原内容：**

```cpp
friend class FETLightComboFlowTest;
```

**替换为：**

```cpp
friend class FETLightComboFlowTest;
    friend class FETLightComboCancelTest;
```

### 3.2 ETGA_LightAttack.cpp 插入完整查询

在该文件找到`void UETGA_LightAttack::ClearBufferedInput()`，将下面完整函数插在它**前面**。原ClearBufferedInput及其后函数保持；不要把新函数写进旧函数花括号里。本文件已有所需include，不再重复加。

当前源码：[ETGA_LightAttack.cpp](../EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp)。

CanCancelIntoDodge先复用私有玩家段上下文查询，再检查实际播放状态和命中结束至淡出前的恢复区间；第三段仍允许取消，不复用HasNextStage。

它不复用`HasNextStage()`，所以第3段恢复也允许闪避；不清输入、不停Montage、不修改Tag。B的连段窗口.60–.80不改变，本查询的起点是命中结束，而非连段窗开启。

## 4. Dodge：预检后由GAS取消原攻击

### 4.1 ETGA_Dodge.h 两处

在public区：

**找到原内容：**

```cpp
bool HasValidInvulnerabilityConfiguration() const;
```

**替换为：**

```cpp
bool HasValidInvulnerabilityConfiguration() const;
    bool HasValidRecoveryCancelPolicy() const;
```

在private区：

**找到原内容：**

```cpp
friend class FETDodgeInvulnerabilityTest;
```

**替换为：**

```cpp
friend class FETDodgeInvulnerabilityTest;
    friend class FETLightComboCancelTest;
```

### 4.2 ETGA_Dodge.cpp include与构造策略

第一处，文件开头：

**找到原内容：**

```cpp
#include "Abilities/ETGA_Dodge.h"
```

**替换为：**

```cpp
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/ETGA_LightAttack.h"
```

第二处，**仅替换构造函数中的这一行**，Dodging / HitReact / Dead三行保留，不把它们清空：

**找到原内容：**

```cpp
ActivationBlockedTags.AddTag(ETGameplayTags::State_Attacking);
```

**替换为：**

```cpp
// The native CanActivateAbility gate permits only our player's recovery window.
    // GAS executes this cancellation only after all activation prechecks pass.
    CancelAbilitiesWithTag.AddTag(ETGameplayTags::Ability_Attack_Light);
```

`Ability.Attack.Light`是能力身份标签，`State.Attacking`是状态标签，两者用途不同。Cancel容器只填前者，不填State或Ability.Dodge。

### 4.3 插入策略查询

在`bool UETGA_Dodge::CanActivateAbility(`前面插入下面完整函数。ReadCostAmount / CheckCost / ApplyCost / 位移与无敌函数保持。

```cpp
bool UETGA_Dodge::HasValidRecoveryCancelPolicy() const
{
    return CancelAbilitiesWithTag.Num()==1 &&
        CancelAbilitiesWithTag.HasTagExact(ETGameplayTags::Ability_Attack_Light) &&
        !ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Attacking) &&
        ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dodging) &&
        ActivationBlockedTags.HasTagExact(ETGameplayTags::State_HitReact) &&
        ActivationBlockedTags.HasTagExact(ETGameplayTags::State_Dead);
}
```

### 4.4 整体替换 CanActivateAbility

从该函数签名开始到对应结束`}`，用下面**完整函数**替换。不是替换ActivateAbility，也不重复保留两个同名函数。

```cpp
bool UETGA_Dodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!ActorInfo || !HasValidRecoveryCancelPolicy() ||
        !CheckCost(Handle, ActorInfo, OptionalRelevantTags) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    { return false; }
    AEmberTrialCharacter* Body=Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get());
    APlayerController* PC=IsValid(Body) ? Cast<APlayerController>(Body->GetController()) : nullptr;
    UCharacterMovementComponent* Movement=IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    const UETPracticeActionComponent* Practice=IsValid(Body)
        ? Body->FindComponentByClass<UETPracticeActionComponent>() : nullptr;
    UAbilitySystemComponent* ASC=ActorInfo->AbilitySystemComponent.Get();
    const FGameplayAbilitySpec* AttackSpec=IsValid(Body) && IsValid(ASC)
        ? ASC->FindAbilitySpecFromHandle(Body->GetLightAttackSpecHandle()) : nullptr;
    const UETGA_LightAttack* Attack=AttackSpec && AttackSpec->IsActive()
        ? Cast<UETGA_LightAttack>(AttackSpec->GetPrimaryInstance()) : nullptr;
    const bool bRecovery=IsValid(Attack) && Attack->CanCancelIntoDodge();
    UAnimInstance* Anim=ActorInfo->GetAnimInstance();
    return IsValid(Body) && Body->GetCombatTeam()==EETCombatTeam::Player &&
        IsValid(PC) && PC->IsLocalController() && PC->GetPawn()==Body && !PC->IsMoveInputIgnored() &&
        (Body->IsFormalActionIdle() || bRecovery) && (!Practice || Practice->IsPracticeActionIdle()) &&
        IsValid(Movement) && Movement->IsMovingOnGround() && !Movement->HasRootMotionSources() &&
        !Body->GetVelocity().ContainsNaN() && HasValidMovementConfiguration() &&
        HasValidInvulnerabilityConfiguration() && !Body->GetDodgeWorldDirection().IsNearlyZero() &&
        IsValid(Anim) && (!Anim->GetCurrentActiveMontage() || bRecovery);
}
```

### 4.5 整体替换 ActivateAbility

同样只替换下面这个完整函数。旧EndAbility、四个完成 / 取消回调、成本与无敌实现均保留；下面已经包含原Montage / 位移 / 扣费逻辑，不再额外粘一遍。

```cpp
void UETGA_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    (void)TriggerEventData;
    if (!IsActive()) { return; } // PreActivate cancellation may synchronously end us.
    const uint32 Serial = ++ActivationSerial;
    bCostSubmitted = false;
    AEmberTrialCharacter* Body = ActorInfo ? Cast<AEmberTrialCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
    UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    float Before = 0.0f;
    float MaxStamina = 0.0f;
    float Cost = 0.0f;
    const FVector Direction = IsValid(Body) ? Body->GetDodgeWorldDirection() : FVector::ZeroVector;
    UCharacterMovementComponent* Movement = IsValid(Body) ? Body->GetCharacterMovement() : nullptr;
    // GAS PreActivate already attempted cancellation. Do not overwrite an uncancelled action.
    if (!IsValid(Body) || !IsValid(ASC) || !HasValidMovementConfiguration() ||
        !HasValidInvulnerabilityConfiguration() || !HasValidRecoveryCancelPolicy() ||
        Body->IsCombatDead() || UGameplayStatics::IsGamePaused(Body) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact) ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dead) || ASC->GetAnimatingAbility()!=nullptr ||
        Direction.IsNearlyZero() || !IsValid(Movement) || !Movement->IsMovingOnGround() ||
        Movement->HasRootMotionSources() ||
        !Body->GetGameplayStaminaSnapshot(Before, MaxStamina) ||
        !ReadCostAmount(GetCostGameplayEffect(), GetAbilityLevel(Handle, ActorInfo), Cost))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    Body->StopJumping();
    Body->ConsumeMovementInputVector();
    Movement->StopMovementImmediately();
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, DodgeMontage, 1.0f, NAME_None, true, 0.0f, 0.0f, true);
    if (!MontageTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageAborted);
    MontageTask->ReadyForActivation();
    if (Serial != ActivationSerial || !IsActive()) { return; }
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    if (!IsValid(Anim) || !Anim->GetActiveInstanceForMontage(DodgeMontage) ||
        ASC->GetAnimatingAbility() != this || !Anim->Montage_IsPlaying(DodgeMontage))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    // Prepare our source before charging. No movement tick runs inside this synchronous call.
    MoveTask = UETAbilityTask_DodgeMove::CreateDodgeMove(this, Direction, DodgeDistance, DodgeDuration);
    if (!MoveTask)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    MoveTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMoveCompleted);
    MoveTask->OnAborted.AddDynamic(this, &ThisClass::HandleMoveAborted);
    MoveTask->ReadyForActivation();
    if (Serial != ActivationSerial || !IsActive()) { return; }
    if (!IsValid(MoveTask) || !MoveTask->IsMovementReady())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    // Montage and source exist. Commit once; cancellation cleans the prepared source.
    const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
    if (Serial != ActivationSerial || !IsActive()) { return; }
    float After = 0.0f;
    if (!bCommitted || !bCostSubmitted || Body->IsCombatDead() ||
        !Body->GetGameplayStaminaSnapshot(After, MaxStamina) ||
        !FMath::IsNearlyEqual(Before - After, Cost, 0.001f))
    {
        UE_LOG(LogETDodge, Warning, TEXT("[M2-02A] Commit failed Body=%s Submitted=%d"),
            *GetNameSafe(Body), bCostSubmitted ? 1 : 0);
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    const bool bWindowStarted = StartInvulnerabilityWindow(ASC, Serial);
    if (Serial != ActivationSerial || !IsActive()) { return; }
    if (!bWindowStarted)
    {
        UE_LOG(LogETDodge, Warning, TEXT("[M2-02C] Window application failed after payment Body=%s"),
            *GetNameSafe(Body));
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    UE_LOG(LogETDodge, Display,
        TEXT("[M2-02A] Begin Body=%s Serial=%u Before=%.1f After=%.1f Cost=%.1f Dodging=%d"),
        *GetNameSafe(Body), Serial, Before, After, Cost, ASC->GetTagCount(ETGameplayTags::State_Dodging));
}
```

## 5. 两行测试friend与Editor资产规则

### 5.1 Character.h

路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h`。找到已有`friend class FETLightComboFlowTest;`，下一行新增：

```cpp
friend class FETLightComboCancelTest;
```

保留所有旧friend；Character.cpp不改。测试只用它给临时身体选择正式Attack / Dodge类、体力初值，不新增蓝图测试变量。

### 5.2 CombatLife.h

路径`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Combat\ETCombatLifeComponent.h`。在同一个旧Flow friend下一行新增**同一行**`friend class FETLightComboCancelTest;`，不改cpp；供临时身体选择已有正式受击类。

### 5.3 Validator.cpp：新增ET-DODGE-004

路径见第2节Editor/Private/Validation。在`ValidateAbility`的Dodge分支，找到这一行：

```cpp
if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidInvulnerabilityConfiguration())
```

将下面整段插在该行**前面**；原无敌if、ET-DODGE-001 / 002 / 003和后面的Combo规则全部保留。本文件已经有Dodge头文件，不新增依赖。

```cpp
if (!CastChecked<UETGA_Dodge>(Defaults)->HasValidRecoveryCancelPolicy())
        {
            AddIssue(Blueprint, TEXT("ET-DODGE-004"),
                TEXT("Dodge must cancel only Ability.Attack.Light; do not block State.Attacking; keep Dodging/HitReact/Dead blocked."), Issues);
        }
```

## 6. 新建可复用的真实交接自动化

新文件`E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETLightComboCancelTests.cpp`，完整内容如下。它不是UCLASS，不加generated.h或Build.cs条目，旧Flow / Dodge测试不删除。

测试加载已保存的GA / Montage，用真实通知、Trace / GE与RMS。目标跟随手部只隔离几何，不伪造HitResult或广播正向命中；旧Task回调是负向注入。自动推进到实际恢复时段，检查三段均可交接、预检失败保留原攻击、待切段被2清除、Pause / 受击 / 死亡 / EndPlay；EndPlay按UE真实协议检查本次Source已标记移除 / Task已释放，不要求世界结束后再Tick才能让容器变空。无需你手赶.15 / .25秒。

当前源码：[ETLightComboCancelTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETLightComboCancelTests.cpp)。

原真实三段恢复闪避交接、预检失败保留攻击、一次成本和取消清理断言全部保留。

## 7. 正常 Build与索引

1. 确认上述一新 / 七旧Source保存，UE关闭。PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

2. Editor Build成功后CLion Reload CMake Project。三个CodeModel预计**37 + 13 + 3 = 53cpp**；只有Core新加一个测试cpp，生成文件不计入手写数。
3. 再执行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

4. 两目标成功再开工程。若失败，贴第一条真实C++ / UHT错误与文件行号；exit6不是根因。不删除全工程缓存，不用旧DLL继续验收。

### 7.1 测试时间类型补修（已关闭，2026-10-16）

第6节完整测试已将`FrozenTime`更正为`double`。UE5.5.4的`UWorld::GetTimeSeconds()`返回double；旧指南用float保存后再传入TestEqual，使两实参变成double / float，触发C2666重载歧义。这是指南的类型错误，不是手动录入问题。

若已录入旧版，只在ETLightComboCancelTests.cpp约第240行，把`const float FrozenTime=World->GetTimeSeconds();`改为`const double FrozenTime=World->GetTimeSeconds();`。第243行TestEqual保持原样，不降低断言或修改玩法。保存、保持UE关闭后继续第7节Editor / 索引 / Game Build，再按第9节运行相关测试；该补修已随正常双构建与三项Success关闭，无需重做本节或未受影响资产配置。

### 7.2 暂停变量声明遗漏补修（已关闭，2026-10-16）

若第246行报`bDodgePaused`未声明，说明局部替换时漏掉了`const bool bDodgePaused = Advance(.50f);`。上次两行示例的替换边界不够明确，现给出完整区块：从**紧接闪避推进.05秒之后**的`SetPauserPlayerState(Pauser)`开始，到后面的`CheckDamage(1);`结束，整体对齐为下列内容；不要改前面第一次暂停攻击的bPaused。

```cpp
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const FVector FrozenLocation = Body->GetActorLocation();
        const double FrozenTime = World->GetTimeSeconds();
        const bool bDodgePaused = Advance(.50f);
        TestTrue(TEXT("Paused handoff displacement frozen"), Body->GetActorLocation().Equals(FrozenLocation, .01f));
        TestEqual(TEXT("Paused handoff clock frozen"), World->GetTimeSeconds(), FrozenTime);
        TestTrue(TEXT("Paused own protection retained"), ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bDodgePaused || !Settle()) { return false; }
        CheckDamage(1);
```

这行不只是变量声明：它实际推进暂停世界.50秒并保存结果，后面的if会检查推进是否成功。不能仅改if变量名、设为true或删除检查。该声明与实际暂停推进已由用户补回，最终源码匹配更正候选、双构建与三项测试通过，本项已关闭。

## 8. 既有资产与输入核对（不新建蓝图验收图）

### 8.1 GA_ET_Dodge标签默认值

1. 打开`/Game/EmberTrial/Abilities/GA_ET_Dodge` → **Class Defaults**。Parent仍ETGA_Dodge，Event Graph空；不要改成新建的GA或替换父类。
2. 清空Details搜索，展开 **Tags**；如果看不到，搜索`Cancel Abilities`（也可搜索`Cancel`），找到 **Cancel Abilities With Tag**。点击容器编辑按钮 / 下拉，勾选**Ability → Attack → Light**，只这一项。不要选State → Attacking。
3. 搜索`Activation Blocked`，找到 **Activation Blocked Tags**：有**State.Dodging / State.HitReact / State.Dead**，没有**State.Attacking**。如果构造默认值已正确继承，不额外改；若仍显示旧Attacking，取消勾选它。只核对这两个容器，不重置整份GA。
4. 对两个容器，若字段旁存在黄色回退箭头且当前与上述不符，可只对该字段Reset to Default；再逐项确认。不要清空整个Tags区域，不改Asset Tags / Activation Owned Tags。
5. Attack / Dodge已有能力标签、Owned Dodging、ServerOnly / InstancedPerActor保留；Dodge Montage仍原闪避AM，Cost仍GE_ET_DodgeCost、150 / .30 / .15保持。
6. Compile → Save。Content Browser右键GA → Asset Actions → Validate Assets，应有本项目`Valid / Issues0`。**ET-DODGE-004**只指这两个标签容器不符合交接契约；按2–4修正。找不到字段时截图完整Class Defaults的Tags与搜索内容，不自行删规则或搭蓝图Cancel节点。

### 8.2 玩家与旧输入

1. BP_ThirdPersonCharacter能力Class仍正式玩家GA / GA_ET_Dodge；单ASC / Trace / Life / Recovery保持，Compile / Save。
2. Event Graph找到既有 **IA_PracticeDodge**（显示名仍沿用M0，不重命名）：**Started**白线经过已验收的暂停 / UI允许门控，最后到 **Request Dodge，Target=Self**；Triggered / Completed不连业务，Return Value留空即可。
3. 若Request前额外串了`Is Formal Action Idle`的Branch，它会吞掉攻击中的2：只把该层前的允许白线直接接Request执行输入，保留更前面的暂停 / UI门控。**E / 移动 / 跳跃自己的Idle检查保留**。不将2连到PracticeAction，不加Cancel Light Attack / Delay / Combo索引变量。
4. 左键 / 数字1仍Started到Request Light Attack(Self)。三份AM与命中 / 连段时间不动，敌人仍原单段10。新版Dodge的精确合法性在原生函数里，不再重复在蓝图拼一套恢复时段。

## 9. 分组检查（10–15分钟）

### 9.1 新交接与相关旧项自动化，约2–4分钟

不Play，Output Log先运行：

```text
Automation RunTests EmberTrial.M2.Combo
```

等 **Flow / RecoveryDodge 两项Success**，再运行：

```text
Automation RunTests EmberTrial.M2.Dodge.Invulnerability
```

等Invulnerability Success。本批新RecoveryDodge覆盖正式位移与一次成本交接；旧Flow确认三段 / 缓冲保持，旧Invulnerability确认伤害保护路径。没有修改位移Task / 成本实现 / Resources / Practice，不重跑它们的全部矩阵，不另加墙坡 / 帧率或UI换Pawn图。

新测试应有`[M2-03C] Recovery dodge: stages1/2/3...`汇总。失败给测试名、第一条Expected与大致时分 / 日志路径；不要删断言、扩大正式恢复范围或把成本改0来凑成功。没有运行就不写Success。

### 9.2 三段恢复取消与预检拒绝，约3–4分钟

- **准备：**正式地图Entry空地Play，生命 / 体力100；可临时控制台`slomo .25`。这是方便看动作，精确窗口已由自动化负责。
- **操作：**先开第一段后马上按2，观察原攻击继续；下一轮等拳头完成命中动作后的收势再按2。再按正常节奏进入第2、3段，各在收势时按2。每轮等体力恢复后再试，不要求手动赶边界。
- **预期：**过早2拒绝且不补执行；恢复2取消当前链、立即原方向闪避、只扣25，旧链不续播，后续左键从1开始。三段都能取消；日志原Attack `Cancelled=1 / AttackingTag=0`后有Dodge Begin / 成本一次、Move与Window清理。
- **拒绝检查：**不足体力 / None Montage / 非法位移保持原攻击由自动化覆盖，不临时改正式GA或搭扣体力函数；若实机错过恢复窗口，观察日志 / 画面，不要求补十轮精确按键。

### 9.3 交战 / 受击 / P，约3–4分钟

- **操作A：**真实目标完成一次三段25或使用一段命中后2取消；让敌人接近挥击，确认普通10与HUD，随后在空地恢复闪避中按3。
- **预期A：**真实命中与敌人单段保持；已命中伤害不撤回，2取消后没有旧Trace额外扣血；3取消闪避，不退款，受击结束恢复输入。
- **操作B：**恢复闪避开始后P暂停约2秒，在菜单按左键 / 2，再以一种支持方式继续。
- **预期B：**位移与保护按世界时间冻结 / 恢复，没有菜单补播、补跳、漂移或卡键；无旧连段重启。暂停期间拒绝2与精确保护清理已由自动化覆盖，不再人工掐.15秒。

### 9.4 世界重建与短基线，约2–3分钟

- **操作：**一次恢复闪避后用现有4完成真实死亡 → Failure Retry；新身体确认双100 / 单HUD，再开始一次连击并Stop，重开。
- **预期：**死亡和EndPlay不遗留旧输入 / RMS / 保护或续播；新左键从1、空闲2、E一次、P一种继续方式与移动转向跳跃正常即可。死亡中P仍由既有Failure控制；不重跑菜单所有None Map / Pawn替换矩阵。
- **恢复：**slomo1，玩家25 / 敌人10 / 双100，Dodge150 / .30 / .15 / Cost25，三段配置 / Notify保持，Debug False，GA标签符合8.1，Source / 资产Save All，回正式地图。当前不Cook / 采Insights；新增包在后续阶段有实际需求时统一。

## 10. 完成反馈与报错材料

```text
M2-03C 完成 / 卡在第__节
一新 / 七旧Source；关闭UE后Editor Build / 索引53cpp / Game Build：
GA_Dodge取消Ability.Attack.Light、Blocked无Attacking / 保留其余三项；Compile / Save / Valid：
既有2的Started到角色Request，无额外Idle拦截；原三段资源保持：
Combo.Flow / Combo.RecoveryDodge / Dodge.Invulnerability：
过早2拒绝不补执行；三段恢复2可取消 / 一次25 / 旧链不续播：
真实命中25、敌人10 / HUD；3取消不退款：
恢复闪避中P冻结恢复 / 菜单无补播不卡键：
死亡Retry新双100 / 动作Stop重开 / E-P与基础输入短基线：
slomo1 / 默认数值 / Debug False、Source资产Save All：
失败或手感问题（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

编译问题文字贴第一条真实错误；标签 / 输入问题截图Class Defaults两个容器或Started到Request的完整局部白线，包含资产名。动画 / 取消问题给输入顺序、当时段落与短视频；日志只给路径 / 大致时间，我自行提取。自动化失败不要求你先判断根因。

## 11. 本批 Commit / Push（固定放在最后）

本页历史功能已完成；第26批对应源码 / 文档的改动与验收见[Cleanup对应章节](Cleanup.md#cleanup-batch-26)。用户重新Stage最新差异、检查Staged Diff后Commit / Push，Raw / 日志 / 生成文件留本地。第24–26批一起Stage / Commit / Push，完整18项清单与建议Commit信息见[Cleanup末尾](Cleanup.md#本批-commit--push固定放在最后)；助手不操作Git索引。
