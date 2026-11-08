# M3-06B4-02 破势加成的遗物装备管线

2026-11-08下发，UE 5.5.4。预计手动75–105分钟，构建等待另计。范围为19份既有Core源码，含14份生产源码和5份既有测试。13份候选cpp已通过编译预检，正式UHT、链接和测试按第8–10节执行。[B4-01验收报告](Reports/M3_06B4_01_Verification.md)

## 1. 本节效果

| 入口 | 行为 |
| --- | --- |
| 遗物数据 | 枚举末尾追加`BreakPulsePoiseBonus`，由双槽规则汇总 |
| 预览、提交与恢复 | 从当前聚合加成扣除本组件旧贡献，再加新贡献。读取玩家实际F配置并按1–1000点预检 |
| 原装备GE | 新增第五个SetByCaller。初始写0，装卸更新同一句柄，失活只移除自有GE |
| F施放 | 继续在接受施放时保存本次有效值。施法与换装复用同一数值规则 |
| 本轮实机 | 现有五件仍可用，F默认扣60。第六件遗物和第五比较行在B4-03配置 |

事务专项使用一件临时定义，给F提供+20。夹具保留独立的Base 2，因此装备后ASC Current为22，实际配置计算60+22=82。真实Notify的80点快照由B4-01范围专项继续回归。

## 2. 开始前

1. Stop PIE，Save All，关闭UE，CLion可保留。
2. 按第3–7节依次替换。每份文件下的修改从上往下做。一次改完再Build，保证GE和所有创建该Spec的夹具同时提交第五个幅度。
3. 资产侧沿用已保存的五件和现有蓝图。本章新增的是原生类型枚举值，通过正常UHT与UBT构建后可供下一章创建资产。

## 3. 遗物数据、双槽汇总和旧方案重验

### 3.1 ETRelicTypes.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicTypes.h`

**修改1　在末尾追加类型，保留既有枚举序号**

找到以下片段。

```cpp
    StaminaRecoveryBonus UMETA(DisplayName = "Stamina Recovery Bonus")
};
```

替换为

```cpp
    StaminaRecoveryBonus UMETA(DisplayName = "Stamina Recovery Bonus"),
    BreakPulsePoiseBonus UMETA(DisplayName = "Break Pulse Poise Bonus")
};
```

### 3.2 ETRelicDefinition.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicDefinition.cpp`

**修改1　允许新的遗物参数类型**

找到以下片段。

```cpp
           Kind == EETRelicModifierKind::StaminaRecoveryBonus;
```

替换为

```cpp
           Kind == EETRelicModifierKind::StaminaRecoveryBonus ||
           Kind == EETRelicModifierKind::BreakPulsePoiseBonus;
```

### 3.3 ETRelicLoadout.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicLoadout.h`

**修改1　新增零默认汇总值**

找到以下片段。

```cpp
    float StaminaRecoveryBonus = 0.0f;
};
```

替换为

```cpp
    float StaminaRecoveryBonus = 0.0f;
    float BreakPulsePoiseBonus = 0.0f;
};
```

### 3.4 ETRelicLoadout.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp`

**修改1　用双精度累加第五类数值**

找到以下片段。

```cpp
    double RecoveryAdditive = 0.0;
```

替换为

```cpp
    double RecoveryAdditive = 0.0;
    double BreakPulseAdditive = 0.0;
```

**修改2　双槽汇总破势加成**

找到以下片段。

```cpp
                RecoveryAdditive += Modifier.AdditiveValue;
                break;
            default:
```

替换为

```cpp
                RecoveryAdditive += Modifier.AdditiveValue;
                break;
            case EETRelicModifierKind::BreakPulsePoiseBonus:
                BreakPulseAdditive += Modifier.AdditiveValue;
                break;
            default:
```

**修改3　纳入汇总溢出检查**

找到以下片段。

```cpp
        !ETRelicLoadoutPrivate::FitsFloat(RecoveryAdditive))
```

替换为

```cpp
        !ETRelicLoadoutPrivate::FitsFloat(RecoveryAdditive) ||
        !ETRelicLoadoutPrivate::FitsFloat(BreakPulseAdditive))
```

**修改4　仅在全部合法后发布汇总**

找到以下片段。

```cpp
    OutTotals.StaminaRecoveryBonus = static_cast<float>(RecoveryAdditive);
```

替换为

```cpp
    OutTotals.StaminaRecoveryBonus = static_cast<float>(RecoveryAdditive);
    OutTotals.BreakPulsePoiseBonus = static_cast<float>(BreakPulseAdditive);
```

**修改5　纯预览只检查参数有限性**

找到以下片段。

```cpp
    // 最终治疗量 / 恢复速率由身体配置继续预检；纯规则检查参数有限性。
    if (!ETRelicLoadoutPrivate::FitsFloat(Health) || !ETRelicLoadoutPrivate::FitsFloat(Damage) ||
        !FMath::IsFinite(Totals.HealingBonus) ||
        !FMath::IsFinite(Totals.StaminaRecoveryBonus))
```

替换为

```cpp
    // 治疗量、恢复速率与破势值按身体配置继续预检。
    if (!ETRelicLoadoutPrivate::FitsFloat(Health) || !ETRelicLoadoutPrivate::FitsFloat(Damage) ||
        !FMath::IsFinite(Totals.HealingBonus) ||
        !FMath::IsFinite(Totals.StaminaRecoveryBonus) ||
        !FMath::IsFinite(Totals.BreakPulsePoiseBonus))
```

### 3.5 ETRelicInventorySubsystem.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp`

**修改1　同一ID改破势数值时识别过期方案**

找到以下片段。

```cpp
               X.StaminaRecoveryBonus == Y.StaminaRecoveryBonus;
```

替换为

```cpp
               X.StaminaRecoveryBonus == Y.StaminaRecoveryBonus &&
               X.BreakPulsePoiseBonus == Y.BreakPulsePoiseBonus;
```

## 4. 复用正式F的数值规则

### 4.1 ETGA_BreakPulse.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_BreakPulse.h`

**修改1　提供可用于装备预检的纯数值计算**

找到以下片段。

```cpp
    bool HasValidConfiguration() const;
```

替换为

```cpp
    bool HasValidConfiguration() const;

    // 装备预检与施放使用同一个范围，结果单位为架势点。
    bool TryCalculatePoiseDamage(double Bonus, float& OutDamage) const;
```

**修改2　声明读取当前ASC数值的内部入口**

找到以下片段。

```cpp
    bool IsCurrentCast(uint32 Serial) const;
```

替换为

```cpp
    bool TryGetEffectivePoiseDamage(const UAbilitySystemComponent& ASC, float& OutDamage) const;
    bool IsCurrentCast(uint32 Serial) const;
```

### 4.2 ETGA_BreakPulse.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp`

**修改1　把B4-01的计算移到能力，供换装与施放复用**

找到以下片段。

```cpp
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

替换为

```cpp
    constexpr float MaxPoiseDamage = 1000.0f;

    bool IsPoiseDamageInRange(const double Damage)
    {
        return FMath::IsFinite(Damage) && Damage >= MinPoiseDamage && Damage <= MaxPoiseDamage;
    }
}

bool UETGA_BreakPulse::TryCalculatePoiseDamage(const double Bonus, float& OutDamage) const
{
    OutDamage = 0.0f;
    const double Total = static_cast<double>(PoiseDamage) + Bonus;
    if (!ETBreakPulsePrivate::IsPoiseDamageInRange(PoiseDamage) ||
        !ETBreakPulsePrivate::IsPoiseDamageInRange(Total))
    {
        return false;
    }
    OutDamage = static_cast<float>(Total);
    return true;
}

bool UETGA_BreakPulse::TryGetEffectivePoiseDamage(
    const UAbilitySystemComponent& ASC, float& OutDamage) const
{
    OutDamage = 0.0f;
    return ASC.GetSet<UETCombatAttributeSet>() && TryCalculatePoiseDamage(
        ASC.GetNumericAttribute(UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute()),
        OutDamage);
}
```

**修改2　基础值验证复用同一范围**

找到以下片段。

```cpp
        PulseRadius > 3000.0f || !FMath::IsFinite(PoiseDamage) ||
        PoiseDamage < ETBreakPulsePrivate::MinPoiseDamage ||
        PoiseDamage > ETBreakPulsePrivate::MaxPoiseDamage)
```

替换为

```cpp
        PulseRadius > 3000.0f || !ETBreakPulsePrivate::IsPoiseDamageInRange(PoiseDamage))
```

**修改3　资格检查调用新的ASC读取入口**

找到以下片段。

```cpp
        !ETBreakPulsePrivate::TryResolvePoiseDamage(*ASC, PoiseDamage, EffectivePoiseDamage) ||
```

替换为

```cpp
        !TryGetEffectivePoiseDamage(*ASC, EffectivePoiseDamage) ||
```

**修改4　实际激活用相同规则捕获快照**

找到以下片段。

```cpp
        !ETBreakPulsePrivate::TryResolvePoiseDamage(*ASC, PoiseDamage, EffectivePoiseDamage))
```

替换为

```cpp
        !TryGetEffectivePoiseDamage(*ASC, EffectivePoiseDamage))
```

### 4.3 EmberTrialCharacter.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h`

**修改1　公开已有角色配置的只读入口**

找到以下片段。

```cpp
    const UETGA_Heal* GetHealConfiguration() const;
```

替换为

```cpp
    const UETGA_Heal* GetHealConfiguration() const;
    const UETGA_BreakPulse* GetBreakPulseConfiguration() const;
```

### 4.4 EmberTrialCharacter.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp`

**修改1　从实际配置的能力Class取CDO**

找到以下片段。

```cpp
bool AEmberTrialCharacter::IsRelicEquipmentReadyForInput() const
```

替换为

```cpp
const UETGA_BreakPulse* AEmberTrialCharacter::GetBreakPulseConfiguration() const
{
    return !bAbilitySystemShuttingDown && IsValid(BreakPulseAbilityClass.Get())
        ? BreakPulseAbilityClass->GetDefaultObject<UETGA_BreakPulse>() : nullptr;
}

bool AEmberTrialCharacter::IsRelicEquipmentReadyForInput() const
```

## 5. 装备GE的第五个SetByCaller

### 5.1 ETGameplayTags.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h`

**修改1　声明第五个SetByCaller标签**

找到以下片段。

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentStaminaRecoveryBonus);
```

替换为

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentStaminaRecoveryBonus);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentBreakPulsePoiseBonus);
```

### 5.2 ETGameplayTags.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp`

**修改1　更新五项必填的契约**

找到以下片段。

```cpp
    // ---- 聚合装备加法，所有Spec均须显式写入四项（可为0） ----
```

替换为

```cpp
    // ---- 聚合装备加法，所有Spec均须显式写入五项（可为0） ----
```

**修改2　注册第五个标签**

找到以下片段。

```cpp
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentStaminaRecoveryBonus,
                           "SetByCaller.Equipment.StaminaRecoveryBonus");
```

替换为

```cpp
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentStaminaRecoveryBonus,
                           "SetByCaller.Equipment.StaminaRecoveryBonus");
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentBreakPulsePoiseBonus,
                           "SetByCaller.Equipment.BreakPulsePoiseBonus");
```

### 5.3 ETRelicEquipmentEffect.h

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h`

**修改1　同步原生GE的输入契约**

找到以下片段。

```cpp
// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供四个SetByCaller，即使是0。
```

替换为

```cpp
// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供五个SetByCaller，即使是0。
```

### 5.4 ETRelicEquipmentEffect.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp`

**修改1　在现有无限GE上增加第五个加法Modifier**

找到以下片段。

```cpp
    RecoveryModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RecoveryMagnitude);
}
```

替换为

```cpp
    RecoveryModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RecoveryMagnitude);

    FSetByCallerFloat BreakPulseMagnitude;
    BreakPulseMagnitude.DataTag = ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus;
    FGameplayModifierInfo& BreakPulseModifier = Modifiers.AddDefaulted_GetRef();
    BreakPulseModifier.Attribute = UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute();
    BreakPulseModifier.ModifierOp = EGameplayModOp::AddBase;
    BreakPulseModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(BreakPulseMagnitude);
}
```

## 6. 现有装备组件的写入和预检

### 6.1 ETRelicEquipmentComponent.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp`

**修改1　引用F能力的数值规则**

找到以下片段。

```cpp
#include "Abilities/ETGA_Heal.h"
```

替换为

```cpp
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_BreakPulse.h"
```

**修改2　缺失或篡改第五幅度会使自有Spec失效**

找到以下片段。

```cpp
    return FMath::IsFinite(Health) && FMath::IsFinite(Attack) &&
           FMath::IsFinite(Healing) && FMath::IsFinite(Recovery) &&
           Health == AppliedModifiers.MaxHealthAdditive &&
           Attack == AppliedModifiers.LightAttackBonus && Healing == AppliedModifiers.HealingBonus &&
           Recovery == AppliedModifiers.StaminaRecoveryBonus;
```

替换为

```cpp
    const float BreakPulse = Active->Spec.GetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus, false, Missing);
    return FMath::IsFinite(Health) && FMath::IsFinite(Attack) &&
           FMath::IsFinite(Healing) && FMath::IsFinite(Recovery) && FMath::IsFinite(BreakPulse) &&
           Health == AppliedModifiers.MaxHealthAdditive &&
           Attack == AppliedModifiers.LightAttackBonus && Healing == AppliedModifiers.HealingBonus &&
           Recovery == AppliedModifiers.StaminaRecoveryBonus &&
           BreakPulse == AppliedModifiers.BreakPulsePoiseBonus;
```

**修改3　空槽Spec显式写入第五个零**

找到以下片段。

```cpp
    const FActiveGameplayEffectHandle Created = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
```

替换为

```cpp
    Spec.Data->SetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus, 0.0f);
    const FActiveGameplayEffectHandle Created = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
```

**修改4　低层写入要求五项有限**

找到以下片段。

```cpp
        !FMath::IsFinite(Modifiers.StaminaRecoveryBonus))
```

替换为

```cpp
        !FMath::IsFinite(Modifiers.StaminaRecoveryBonus) ||
        !FMath::IsFinite(Modifiers.BreakPulsePoiseBonus))
```

**修改5　低层确认替换后的聚合值可存为float**

找到以下片段。

```cpp
        AppliedModifiers.StaminaRecoveryBonus + Modifiers.StaminaRecoveryBonus;
    const double Limit
```

替换为

```cpp
        AppliedModifiers.StaminaRecoveryBonus + Modifiers.StaminaRecoveryBonus;
    const double NewBreakPulse = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute())) -
        AppliedModifiers.BreakPulsePoiseBonus + Modifiers.BreakPulsePoiseBonus;
    const double Limit
```

**修改6　拒绝破势聚合溢出**

找到以下片段。

```cpp
           FMath::IsFinite(NewRecovery) && FMath::Abs(NewRecovery) <= Limit;
```

替换为

```cpp
           FMath::IsFinite(NewRecovery) && FMath::Abs(NewRecovery) <= Limit &&
           FMath::IsFinite(NewBreakPulse) && FMath::Abs(NewBreakPulse) <= Limit;
```

**修改7　仅第五项变化也触发真实写入**

找到以下片段。

```cpp
        Modifiers.StaminaRecoveryBonus == AppliedModifiers.StaminaRecoveryBonus)
```

替换为

```cpp
        Modifiers.StaminaRecoveryBonus == AppliedModifiers.StaminaRecoveryBonus &&
        Modifiers.BreakPulsePoiseBonus == AppliedModifiers.BreakPulsePoiseBonus)
```

**修改8　随同一句柄批量更新第五幅度**

找到以下片段。

```cpp
                   Modifiers.StaminaRecoveryBonus);
    {
        // 四个聚合器更新后才派发脏通知；这不是任意GAS / 库存事务的原子锁。
```

替换为

```cpp
                   Modifiers.StaminaRecoveryBonus);
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus,
                   Modifiers.BreakPulsePoiseBonus);
    {
        // 批量更新后才派发聚合器通知。库存提交仍由原事务负责。
```

**修改9　预览、正式提交与恢复使用实际F配置预检**

找到以下片段。

```cpp
    const double RecoveryBonus = static_cast<double>(ASC->GetNumericAttribute(
```

替换为

```cpp
    const UETGA_BreakPulse* BreakPulse = Body->GetBreakPulseConfiguration();
    const double BreakPulseBonus = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute())) -
        AppliedModifiers.BreakPulsePoiseBonus + Modifiers.BreakPulsePoiseBonus;
    float PoiseDamage = 0.0f;
    if (IsValid(BreakPulse) ? !BreakPulse->TryCalculatePoiseDamage(BreakPulseBonus, PoiseDamage)
                           : Modifiers.BreakPulsePoiseBonus != 0.0f)
    {
        return false;
    }

    const double RecoveryBonus = static_cast<double>(ASC->GetNumericAttribute(
```

## 7. 扩展既有测试与Spec夹具

### 7.1 ETRelicLoadoutTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp`

**修改1　扩展原规则专项，检查第五类有符号加法和溢出**

找到以下片段。

```cpp
    TestEqual(TEXT("Recovery overflow clears output"), Totals.StaminaRecoveryBonus, 0.0f);
```

替换为

```cpp
    TestEqual(TEXT("Recovery overflow clears output"), Totals.StaminaRecoveryBonus, 0.0f);

    Life->Modifiers[0] = {EETRelicModifierKind::BreakPulsePoiseBonus, 20.0f};
    Edge->Modifiers[0] = {EETRelicModifierKind::BreakPulsePoiseBonus, -5.0f};
    TestTrue(TEXT("Signed Break Pulse modifiers sum"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::Success);
    TestEqual(TEXT("Break Pulse sum fifteen"), Totals.BreakPulsePoiseBonus, 15.0f);
    TestEqual(TEXT("Break Pulse leaves recovery zero"), Totals.StaminaRecoveryBonus, 0.0f);
    Life->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    Edge->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    TestTrue(TEXT("Break Pulse sum overflow rejected"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::NumericOverflow);
    TestEqual(TEXT("Break Pulse overflow clears output"), Totals.BreakPulsePoiseBonus, 0.0f);
```

### 7.2 ETRelicEquipmentEffectsTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp`

**修改1　测试Spec显式提供第五项**

找到以下片段。

```cpp
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive,
                       const float HealingAdditive = 0.0f, const float RecoveryAdditive = 0.0f)
```

替换为

```cpp
    const FGameplayAttribute BreakPulseBonus =
        UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute();
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive,
                       const float HealingAdditive, const float RecoveryAdditive,
                       const float BreakPulseAdditive)
```

**修改2　给原生GE第五幅度赋值**

找到以下片段。

```cpp
            ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, RecoveryAdditive);
        return ASC
```

替换为

```cpp
            ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, RecoveryAdditive);
        Spec.Data->SetSetByCallerMagnitude(
            ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus, BreakPulseAdditive);
        return ASC
```

**修改3　验证GAS的Base和Current分离**

找到以下片段。

```cpp
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f, 10.0f, 5.0f);
```

替换为

```cpp
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f, 10.0f, 5.0f, 20.0f);
```

**修改4　观察第五属性生效**

找到以下片段。

```cpp
    TestEqual(TEXT("Recovery Base remains zero"), ASC->GetNumericAttributeBase(RecoveryBonus), 0.0f);
```

替换为

```cpp
    TestEqual(TEXT("Recovery Base remains zero"), ASC->GetNumericAttributeBase(RecoveryBonus), 0.0f);
    TestEqual(TEXT("Break Pulse Current twenty"), ASC->GetNumericAttribute(BreakPulseBonus), 20.0f);
    TestEqual(TEXT("Break Pulse Base remains zero"), ASC->GetNumericAttributeBase(BreakPulseBonus),
              0.0f);
```

**修改5　真实更新Spec保持第五幅度**

找到以下片段。

```cpp
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 8.0f);
```

替换为

```cpp
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 8.0f);
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus, 12.0f);
```

**修改6　验证同句柄更新与外部效果独立**

找到以下片段。

```cpp
    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f, 4.0f, 2.0f);
```

替换为

```cpp
    TestEqual(TEXT("Same handle updates Break Pulse"), ASC->GetNumericAttribute(BreakPulseBonus),
              12.0f);
    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f, 4.0f, 2.0f, 3.0f);
```

**修改7　精确移除后外部贡献仍在**

找到以下片段。

```cpp
    TestTrue(TEXT("Remove second handle"), ASC->RemoveActiveGameplayEffect(OtherHandle));
```

替换为

```cpp
    TestEqual(TEXT("Other Break Pulse survives exact removal"),
              ASC->GetNumericAttribute(BreakPulseBonus), 3.0f);
    TestTrue(TEXT("Remove second handle"), ASC->RemoveActiveGameplayEffect(OtherHandle));
    TestEqual(TEXT("Removing all effects clears Break Pulse"), ASC->GetNumericAttribute(BreakPulseBonus),
              0.0f);
    TestEqual(TEXT("Other body Break Pulse isolated"), OtherASC->GetNumericAttribute(BreakPulseBonus),
              0.0f);
```

### 7.3 ETRelicEquipmentOwnershipTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentOwnershipTests.cpp`

**修改1　覆盖仅第五幅度变化与篡改**

找到以下片段。

```cpp
    TestTrue(TEXT("Fourth magnitude restored"), Equipment->GetAppliedModifiers(Snapshot));
```

替换为

```cpp
    TestTrue(TEXT("Fourth magnitude restored"), Equipment->GetAppliedModifiers(Snapshot));

    const FETRelicModifierTotals WithBreakPulse{40.0f, 7.0f, 10.0f, 5.0f, 20.0f};
    TestTrue(TEXT("Break Pulse-only change applies"),
             Equipment->TrySetModifiers(WithBreakPulse) == EWrite::Applied);
    TestTrue(TEXT("Break Pulse update keeps handle"), Equipment->OwnedEffect == InitialHandle);
    TestTrue(TEXT("Same five totals unchanged"),
             Equipment->TrySetModifiers(WithBreakPulse) == EWrite::Unchanged);
    Tamper.Reset();
    Tamper.Add(ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus, 99.0f);
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(InitialHandle, Tamper);
    TestFalse(TEXT("Fifth magnitude tampering detected"), Equipment->GetAppliedModifiers(Snapshot));
    Tamper[ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus] = 20.0f;
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(InitialHandle, Tamper);
    TestTrue(TEXT("Fifth magnitude restored"), Equipment->GetAppliedModifiers(Snapshot));
```

**修改2　外部同类Spec显式初始化第五项**

找到以下片段。

```cpp
    const FActiveGameplayEffectHandle Foreign = ASC->ApplyGameplayEffectSpecToSelf(*ForeignSpec.Data.Get());
```

替换为

```cpp
    ForeignSpec.Data->SetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus, 3.0f);
    const FActiveGameplayEffectHandle Foreign = ASC->ApplyGameplayEffectSpecToSelf(*ForeignSpec.Data.Get());
```

### 7.4 ETLightComboFlowTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp`

**修改1　现有连段夹具写入第五个零**

找到以下片段。

```cpp
            ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 0.0f);
```

替换为

```cpp
            ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 0.0f);
        EquipmentSpec.Data->SetSetByCallerMagnitude(
            ETGameplayTags::SetByCaller_EquipmentBreakPulsePoiseBonus, 0.0f);
```

### 7.5 ETRelicEquipmentTransactionTests.cpp

**文件** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp`

**修改1　引用实际破势配置类型**

找到以下片段。

```cpp
#include "Abilities/ETGA_Heal.h"
```

替换为

```cpp
#include "Abilities/ETGA_Heal.h"
#include "Abilities/ETGA_BreakPulse.h"
```

**修改2　事务夹具使用现有正式F资产**

找到以下片段。

```cpp
    APlayerController* PC = World->SpawnActor<APlayerController>();
```

替换为

```cpp
    UClass* BreakPulseClass = LoadClass<UETGA_BreakPulse>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse.GA_ET_BreakPulse_C"));
    if (!TestNotNull(TEXT("Saved Break Pulse class"), BreakPulseClass))
    {
        return false;
    }
    APlayerController* PC = World->SpawnActor<APlayerController>();
```

**修改3　给测试身体配置同一F**

找到以下片段。

```cpp
            Result->HealAbilityClass = UETGA_Heal::StaticClass();
```

替换为

```cpp
            Result->HealAbilityClass = UETGA_Heal::StaticClass();
            Result->BreakPulseAbilityClass = BreakPulseClass;
```

**修改4　扩展现有事务专项，覆盖拒绝、过期、装卸及外部贡献**

找到以下片段。

```cpp
    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
```

替换为

```cpp
    // ---- 破势：换装预检、同句柄提交与旧方案失效 ----
    const FGameplayAttribute BreakPulseBonus = UETCombatAttributeSet::GetBreakPulsePoiseBonusAttribute();
    NewASC->SetNumericAttributeBase(BreakPulseBonus, 2.0f);
    UETRelicDefinition* PulseFixture = NewObject<UETRelicDefinition>(Inventory);
    const FName PulseId(TEXT("fixture_break_pulse"));
    PulseFixture->RelicId = PulseId;
    PulseFixture->DisplayName = FText::FromString(TEXT("Break Pulse fixture"));
    PulseFixture->Description = FText::FromString(TEXT("Transient Break Pulse transaction fixture"));
    PulseFixture->Modifiers = {{EETRelicModifierKind::BreakPulsePoiseBonus, 20.0f}};
    Inventory->Definitions.Add(PulseId, PulseFixture);
    Inventory->OwnedIds.Add(PulseId);
    Inventory->OwnedSet.Add(PulseId);
    Inventory->AdvanceRevision();
    Inventory->PublishSnapshot();

    const uint64 PulseRevision = Inventory->GetSnapshot().Revision;
    for (const float InvalidBonus : {-62.0f, 939.0f})
    {
        PulseFixture->Modifiers[0].AdditiveValue = InvalidBonus;
        const FETRelicInventoryReply PreviewReply = NewEquipment->TryBuildPreview(
            EETRelicSlot::First, PulseId, PulseRevision, Preview);
        const FETRelicInventoryReply EquipReply = NewEquipment->TryEquip(
            EETRelicSlot::First, PulseId, PulseRevision);
        TestTrue(TEXT("Out-of-range Break Pulse preview rejected by final value"),
                 PreviewReply.Result == EResult::RuleRejected &&
                 PreviewReply.Rule == EETRelicLoadoutResult::InvalidFinalValues);
        TestTrue(TEXT("Out-of-range Break Pulse equip rejected by final value"),
                 EquipReply.Result == EResult::RuleRejected &&
                 EquipReply.Rule == EETRelicLoadoutResult::InvalidFinalValues);
    }
    TestTrue(TEXT("Rejected Break Pulse leaves revision"), Inventory->GetSnapshot().Revision == PulseRevision);
    TestEqual(TEXT("Rejected Break Pulse preserves existing contribution"),
              NewASC->GetNumericAttribute(BreakPulseBonus), 2.0f);
    PulseFixture->Modifiers[0].AdditiveValue = 20.0f;
    FETRelicInventoryPlan PulsePlan;
    TestTrue(TEXT("Break Pulse plan created"), Inventory->TryBuildPlan(
        EETRelicSlot::First, PulseId, 100.0f, 25.0f, PulsePlan).Result == EResult::Success);
    PulseFixture->Modifiers[0].AdditiveValue = 21.0f;
    TestTrue(TEXT("Break Pulse-only edit invalidates plan"),
             Inventory->TryCommitPlan(PulsePlan).Result == EResult::StalePlan);
    PulseFixture->Modifiers[0].AdditiveValue = 20.0f;
    TestTrue(TEXT("Break Pulse preview succeeds"), NewEquipment->TryBuildPreview(
        EETRelicSlot::First, PulseId, PulseRevision, Preview).Result == EResult::Success);
    TestEqual(TEXT("Preview carries twenty"), Preview.AfterModifiers.BreakPulsePoiseBonus, 20.0f);
    TestEqual(TEXT("Preview leaves real bonus two"), NewASC->GetNumericAttribute(BreakPulseBonus), 2.0f);
    const FActiveGameplayEffectHandle PulseHandle = NewEquipment->OwnedEffect;
    TestTrue(TEXT("Equip Break Pulse contribution"), NewEquipment->TryEquip(
        EETRelicSlot::First, PulseId, PulseRevision).Result == EResult::Success);
    TestTrue(TEXT("Break Pulse equip keeps handle"), NewEquipment->OwnedEffect == PulseHandle);
    TestEqual(TEXT("Break Pulse equipment leaves Base two"),
              NewASC->GetNumericAttributeBase(BreakPulseBonus), 2.0f);
    TestEqual(TEXT("Break Pulse equipment aggregates to twenty-two"),
              NewASC->GetNumericAttribute(BreakPulseBonus), 22.0f);
    float EffectivePoiseDamage = 0.0f;
    const UETGA_BreakPulse* Pulse = NewRunBody->GetBreakPulseConfiguration();
    if (!TestNotNull(TEXT("Configured Break Pulse"), Pulse))
    {
        return false;
    }
    TestTrue(TEXT("Actual F configuration accepts equipped value"),
             Pulse->TryCalculatePoiseDamage(NewASC->GetNumericAttribute(BreakPulseBonus), EffectivePoiseDamage));
    TestEqual(TEXT("F configured damage eighty-two"), EffectivePoiseDamage, 82.0f);
    NewEquipment->Deactivate();
    TestEqual(TEXT("Deactivate leaves external two"), NewASC->GetNumericAttribute(BreakPulseBonus), 2.0f);
    NewEquipment->Activate(false);
    TestEqual(TEXT("Reactivation restores equipped contribution"),
              NewASC->GetNumericAttribute(BreakPulseBonus), 22.0f);
    TestTrue(TEXT("Unequip Break Pulse"), NewEquipment->TryEquip(
        EETRelicSlot::First, NAME_None, Inventory->GetSnapshot().Revision).Result == EResult::Success);
    TestEqual(TEXT("Unequip leaves external two"), NewASC->GetNumericAttribute(BreakPulseBonus), 2.0f);
    NewASC->SetNumericAttributeBase(BreakPulseBonus, 0.0f);

    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
```

## 8. 正常构建

保存19份源码。关闭UE后，在PowerShell执行。

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本完成Editor Build与三模块索引。CLion有提示时Reload CMake，再执行Game Development Build。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

## 9. 六项专项各一次

打开UE，在非Play状态下于Output Log底部逐条执行。每条等到`Result={Success}`再继续。

```text
Automation RunTests EmberTrial.M3.Inventory.LoadoutRules
Automation RunTests EmberTrial.M3.Inventory.EquipmentEffects
Automation RunTests EmberTrial.M3.Inventory.EquipmentOwnership
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
Automation RunTests EmberTrial.M2.Combat.BreakPulseRange
Automation RunTests EmberTrial.M2.Combo.Flow
```

前四项分别核对双槽、实际GE、自有句柄与装备事务。最后两项回归F的真实Notify与连段测试中直接创建装备Spec的入口。

## 10. 实机短查

1. Play正式地图`/Game/EmberTrial/Maps/L_ET_VerticalSlice`。到祭坛旁按E，确认库存仍为五件。选凝息护符装备到槽A，确认界面返回已装备状态。
2. 点击继续，走到满架势训练目标约300 cm内，站定按一次F。日志`LogETBreakPulse`应见`Damage=60.0`，训练目标`Poise=100.0->40.0`。F卡进入冷却。
3. Stop再Play，确认新身体日志`BreakPulsePoiseBonus=0.0`，移动和P继续正常。随后Stop。

反馈时记录双Build／三索引、六项专项、祭坛五件与装配、F实际扣60、Stop新身体0、手动工时与构建等待。遇到失败，贴首条错误与测试名称或实机步骤。

## 11. Commit / Push

B4-01实现已在本地提交`764f98e`。在CLion Review并Stage本轮11份文档。

```text
Docs/Reports/M3_06B4_01_Verification.md
Docs/M3_06B4_01_BreakPulsePoiseBonusFoundation.md
Docs/M3_06B4_02_BreakPulseRelicEquipmentPipeline.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文 `docs: verify break pulse bonus and plan relic equipment integration`
- 中文 `docs: 完成破势加成验收并规划遗物装备接入`

B4-02操作与验收通过后，功能提交包含以下19份既有Core源码。

```text
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicTypes.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicDefinition.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicLoadout.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_BreakPulse.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_BreakPulse.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h
EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentOwnershipTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp
```

- 英文 `feat: integrate break pulse poise bonus into relic equipment`
- 中文 `feat: 将破势技架势伤害加成接入遗物装备`
