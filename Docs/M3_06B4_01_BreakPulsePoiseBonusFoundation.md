# M3-06B4-01 F技能破势加成基础

2026-11-08下发，UE 5.5.4。预计手动45–65分钟，构建等待另计。四份候选cpp已通过编译预检，正式UHT、链接和实机按第5–7节执行。[B3-03验收报告](Reports/M3_06B3_03_Verification.md)

## 1. 效果与职责

本节更新五份既有Core源码。`UETCombatAttributeSet`提供默认0的`BreakPulsePoiseBonus`，由角色在发布Ready前初始化。按F接受施放时，`UETGA_BreakPulse`读取施法者ASC的Current，将其加到资产的基础破势值，再锁定本次结果。Montage的生效通知使用同一份快照，经现有Poise Delta GE扣减目标架势。

- 空装备基础值为60点，首个命中的满架势训练目标从100到40。
- 自动化夹具用+20点验证总值80，目标从100到20。接受施放后将夹具的加成恢复到0，本次仍结算80。
- 最终值须为有限数且在1–1000之间。超界时拒绝施放，冷却保持可用。
- 本节范围是F的破势量。后续B4-02接入装备事务，B4-03再接入第六件资产和比较行。

## 2. Combat属性与角色初始化

### 2.1 ETCombatAttributeSet.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h`

**修改1　新增公开属性访问器**

找到以下片段。

```cpp
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, StaminaRecoveryBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(StaminaRecoveryBonus)
```

替换为

```cpp
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, StaminaRecoveryBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(StaminaRecoveryBonus)

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, BreakPulsePoiseBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(BreakPulsePoiseBonus)
```

**修改2　追加原生GAS属性**

找到以下片段。

```cpp
    GAMEPLAYATTRIBUTE_VALUE_INITTER(StaminaRecoveryBonus)
};
```

替换为

```cpp
    GAMEPLAYATTRIBUTE_VALUE_INITTER(StaminaRecoveryBonus)

    // 正式F施放时快照的固定破势加成，单位为架势点。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData BreakPulsePoiseBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(BreakPulsePoiseBonus)
};
```

### 2.2 ETCombatAttributeSet.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp`

**修改1　构造时初始化零加成**

找到以下片段。

```cpp
    InitHealingBonus(0.0f);
    InitStaminaRecoveryBonus(0.0f);
```

替换为

```cpp
    InitHealingBonus(0.0f);
    InitStaminaRecoveryBonus(0.0f);
    InitBreakPulsePoiseBonus(0.0f);
```

**修改2　Base使用同层有限值回退**

找到以下片段。

```cpp
    else if (Attribute == GetStaminaRecoveryBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, StaminaRecoveryBonus.GetBaseValue());
    }
```

替换为

```cpp
    else if (Attribute == GetStaminaRecoveryBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, StaminaRecoveryBonus.GetBaseValue());
    }
    else if (Attribute == GetBreakPulsePoiseBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, BreakPulsePoiseBonus.GetBaseValue());
    }
```

**修改3　Current使用同层有限值回退**

找到以下片段。

```cpp
    else if (Attribute == GetStaminaRecoveryBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, StaminaRecoveryBonus.GetCurrentValue());
    }
```

替换为

```cpp
    else if (Attribute == GetStaminaRecoveryBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, StaminaRecoveryBonus.GetCurrentValue());
    }
    else if (Attribute == GetBreakPulsePoiseBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, BreakPulsePoiseBonus.GetCurrentValue());
    }
```

### 2.3 EmberTrialCharacter.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

**修改1　在发布Ready前设置新属性**

找到以下片段。

```cpp
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;
```

替换为

```cpp
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute(), 0.0f);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;
```

**修改2　沿用Combat初始化日志**

找到以下片段。

```cpp
                "StaminaRecoveryBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus(),
           CombatAttributes->GetHealingBonus(), CombatAttributes->GetStaminaRecoveryBonus());
```

替换为

```cpp
                "StaminaRecoveryBonus=%.1f BreakPulsePoiseBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus(),
           CombatAttributes->GetHealingBonus(), CombatAttributes->GetStaminaRecoveryBonus(),
           CombatAttributes->GetBreakPulsePoiseBonus());
```

## 3. 正式F结算

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp`

**修改1　引用Combat属性集**

找到以下片段。

```cpp
#include "Attributes/ETPoiseAttributeSet.h"
```

替换为

```cpp
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
```

**修改2　共用有效范围并读取施法者的当前聚合值**

找到以下片段。

```cpp
DEFINE_LOG_CATEGORY_STATIC(LogETBreakPulse, Log, All);
```

替换为

```cpp
DEFINE_LOG_CATEGORY_STATIC(LogETBreakPulse, Log, All);

namespace ETBreakPulsePrivate
{
    constexpr float MinPoiseDamage = 1.0f;
    constexpr float MaxPoiseDamage = 1000.0f;

    bool TryResolvePoiseDamage(const UAbilitySystemComponent& ASC, float BaseDamage,
                               float& OutDamage)
    {
        if (!ASC.GetSet<UETCombatAttributeSet>())
        {
            return false;
        }
        const float Bonus =
            ASC.GetNumericAttribute(UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute());
        const double Total = static_cast<double>(BaseDamage) + Bonus;
        if (!FMath::IsFinite(Total) || Total < MinPoiseDamage || Total > MaxPoiseDamage)
        {
            return false;
        }
        OutDamage = static_cast<float>(Total);
        return true;
    }
}
```

**修改3　资源校验复用已有的1至1000范围**

找到以下片段。

```cpp
        PulseRadius > 3000.0f || !FMath::IsFinite(PoiseDamage) || PoiseDamage < 1.0f ||
        PoiseDamage > 1000.0f)
```

替换为

```cpp
        PulseRadius > 3000.0f || !FMath::IsFinite(PoiseDamage) ||
        PoiseDamage < ETBreakPulsePrivate::MinPoiseDamage ||
        PoiseDamage > ETBreakPulsePrivate::MaxPoiseDamage)
```

**修改4　资格检查拒绝非法有效数值**

找到以下片段。

```cpp
    if (!HasValidConfiguration() || !ReadContext(ActorInfo, Body, ASC) ||
        !Body->IsFormalActionIdle() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
```

替换为

```cpp
    float EffectivePoiseDamage = 0.0f;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo, Body, ASC) ||
        !Body->IsFormalActionIdle() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse) ||
        !ETBreakPulsePrivate::TryResolvePoiseDamage(*ASC, PoiseDamage, EffectivePoiseDamage) ||
        !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
```

**修改5　在生成任务和提交冷却前捕获有效值**

找到以下片段。

```cpp
    if (!HasValidConfiguration() || !ReadContext(ActorInfo, Body, ASC) ||
        ActorInfo->GetAnimInstance()->GetCurrentActiveMontage() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse))
```

替换为

```cpp
    float EffectivePoiseDamage = 0.0f;
    if (!HasValidConfiguration() || !ReadContext(ActorInfo, Body, ASC) ||
        ActorInfo->GetAnimInstance()->GetCurrentActiveMontage() ||
        ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse) ||
        !ETBreakPulsePrivate::TryResolvePoiseDamage(*ASC, PoiseDamage, EffectivePoiseDamage))
```

**修改6　生效通知沿用本次快照**

找到以下片段。

```cpp
    ActivePoiseDamage = PoiseDamage;
```

替换为

```cpp
    // 同次施放与所有目标共用此值，期间的ASC属性变化留给下一次施放。
    ActivePoiseDamage = EffectivePoiseDamage;
```

## 4. 扩展既有范围专项

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETBreakPulseRangeTests.cpp`

**修改1　引用Combat属性集**

找到以下片段。

```cpp
#include "Attributes/ETHealthAttributeSet.h"
```

替换为

```cpp
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
```

**修改2　为破势加成保留独立场景**

找到以下片段。

```cpp
        Cycle,
        Edge,
```

替换为

```cpp
        Cycle,
        BonusSnapshot,
        Edge,
```

**修改3　将独立场景加入既有专项**

找到以下片段。

```cpp
         {EScenario::Cycle, EScenario::Edge, EScenario::Wall, EScenario::Team, EScenario::Dead,
          EScenario::Waiting, EScenario::Invulnerable, EScenario::Stunned, EScenario::EnterRange,
```

替换为

```cpp
         {EScenario::Cycle, EScenario::BonusSnapshot, EScenario::Edge, EScenario::Wall,
          EScenario::Team, EScenario::Dead, EScenario::Waiting, EScenario::Invulnerable,
          EScenario::Stunned, EScenario::EnterRange,
```

**修改4　验证非法值无冷却，合法值在施放后保持快照**

找到以下片段。

```cpp
        if (!TestTrue(TEXT("Formal F request accepted"), Source->RequestBreakPulse()))
        {
            return false;
        }
```

替换为

```cpp
        const FGameplayAttribute Bonus = UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute();
        if (Scenario == EScenario::BonusSnapshot)
        {
            TestEqual(TEXT("Fresh Break Pulse bonus is zero"), ASC->GetNumericAttribute(Bonus),
                      0.0f);
            for (const float RejectedBonus : {-60.0f, 941.0f})
            {
                ASC->SetNumericAttributeBase(Bonus, RejectedBonus);
                if (!TestFalse(TEXT("Out-of-range effective Poise damage rejects F"),
                               Source->RequestBreakPulse()))
                {
                    return false;
                }
                TestFalse(TEXT("Rejected F has no cooldown"),
                          ASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_BreakPulse));
            }
            ASC->SetNumericAttributeBase(Bonus, 20.0f);
        }
        if (!TestTrue(TEXT("Formal F request accepted"), Source->RequestBreakPulse()))
        {
            return false;
        }
        if (Scenario == EScenario::BonusSnapshot)
        {
            ASC->SetNumericAttributeBase(Bonus, 0.0f);
        }
```

**修改5　通过真实Notify检查目标架势扣80并去重**

找到以下片段。

```cpp
        TestEqual(TEXT("Real notify applies or skips by current target policy"), Poise(ASC_A),
                  bRejectedTarget ? 100.0f : 40.0f);
        Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
        TestEqual(TEXT("Duplicate or cancelled callback cannot add damage"), Poise(ASC_A),
                  bRejectedTarget ? 100.0f : 40.0f);
```

替换为

```cpp
        float ExpectedPoise = 40.0f;
        if (bRejectedTarget)
        {
            ExpectedPoise = 100.0f;
        }
        else if (Scenario == EScenario::BonusSnapshot)
        {
            ExpectedPoise = 20.0f;
        }
        TestEqual(TEXT("Real notify applies captured damage or skips by current target policy"),
                  Poise(ASC_A), ExpectedPoise);
        Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
        TestEqual(TEXT("Duplicate or cancelled callback cannot add damage"), Poise(ASC_A),
                  ExpectedPoise);
```

## 5. 正常构建

保存五份源码，关闭UE。在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本完成Editor Build和三模块索引。CLion有提示时Reload CMake，再执行Game Development Build。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 6. 两项专项

打开UE，在非Play状态下于Output Log底部逐条运行。每条等到`Result={Success}`再运行下一条。

```text
Automation RunTests EmberTrial.M2.Combat.BreakPulseRange
Automation RunTests EmberTrial.M2.Combat.BreakPulseAbility
```

范围专项保留基础60、命中去重和原目标规则，新增真实Notify的80点快照与超界拒绝。能力专项覆盖冷却、动作门控与清理。

## 7. 实机短查

1. 打开正式地图`/Game/EmberTrial/Maps/L_ET_VerticalSlice`并Play。Output Log中筛`Combat initialized`，玩家行应包含`BreakPulsePoiseBonus=0.0`。该行应在同一身体的Ready前。
2. 走到训练目标约300 cm内，松开移动键站定，按F一次。等到生效通知，在Output Log筛`LogETBreakPulse`。`Pulse`行应有`Damage=60.0`，对应训练目标的`Target`行应有`Applied=1`与`Poise=100.0->40.0`。观察F卡进入冷却。
3. 按P并继续，短查移动。Stop后再Play一次，确认玩家新的`BreakPulsePoiseBonus=0.0`和F就绪，再Stop。

反馈时给出双Build／三索引、两项专项、F实际扣60、新身体加成0、手动工时与构建等待。若有失败，贴首条错误与对应的用例或运行步骤。

## 8. Commit / Push

B3-03功能提交为`2a8d1dd`。本轮先在CLion Review并Stage以下12份文档，随后Commit和Push。

```text
Docs/Reports/M3_06B3_03_Verification.md
Docs/AssetManifest.csv
Docs/M3_06B3_03_StaminaRecoveryRelicAndComparison.md
Docs/M3_06B4_01_BreakPulsePoiseBonusFoundation.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文 `docs: verify stamina recovery relic and plan break pulse bonus`
- 中文 `docs: 完成体力恢复遗物验收并规划破势加成`

B4-01手动实施并验收后，功能提交包含以下五份Core源码。

```text
EmberTrial/Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h
EmberTrial/Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp
EmberTrial/Source/EmberTrial/Tests/ETBreakPulseRangeTests.cpp
```

- 英文 `feat: support additive break pulse poise damage`
- 中文 `feat: 支持破势技的架势伤害加成`
