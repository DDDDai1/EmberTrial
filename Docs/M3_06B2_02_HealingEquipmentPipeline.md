# M3-06B2-02：治疗加成的核心装备管线

2026-11-07下发，UE **5.5.4**。[B2-01已通过](Reports/M3_06B2_01_Verification.md)，无待补。本章仍由你手动修改C++，**21份既有Core文件，无新增文件 / 资产 / Config**；正式源码尚未改动。第四件遗物与UI比较放到B2-03。

## 1. 本章做出什么、为何分批

B2-01只有“正式Q能消费聚合加成”。本章补齐遗物数据到ASC的核心链路：

```text
HealingBonus类型参数 → 两槽纯汇总 → 库存方案重验
  → 身体装备适配层预检最终Q → 自有Infinite GE同句柄更新
  → ASC.HealingBonus → 已完成的Q施放快照
```

| 责任 | 具体合同 |
| --- | --- |
| Definition / Loadout | 新枚举尾部追加；有限非零数据、双精度求和、溢出拒绝，保留旧两种效果 |
| 原装备GE | 从2个扩为3个SetByCaller，所有Spec显式赋值，包括零；不创建第二份装备效果 |
| 组件所有权 | 完整性校验 / 仅治疗变化 / 更新 / 失活清理均含第三项，外部GE贡献保留 |
| 正式预检 | 以当前聚合值减自有旧贡献再加新贡献；读取真实治疗GA基础值，同一公式检查最终1–1000 |
| 事务 | 新参数同样参与提交时重算比较，拒绝预览之后改变治疗数据的过期方案 |
| Q | 仍约0.5秒通知一次生效、只消费一次；这次只共享数值计算，不改施放快照时点 |

21份文件中，5份是既有测试，16份是生产声明 / 实现；修改较分散，因此UI与第四件不同时加入。这次不搭Designer或临时蓝图；仍三件、原比较两行是预期，不能把“核心支持”当作第四件已完成。

预计手动120–180分钟（理解与编码约105–160、专项 / 短查5–10、记录约5–10），可先完成第3–5节保存，再继续第6–8节；**第8节完成后再统一正常Build**。构建等待另记。规模仍233 Source / 133cpp（Core89 / UI41 / Editor3）/ 42测试 / Content310 / 来源93。

## 2. 开始前与操作方式

1. Stop PIE → Save All → 关闭UE，CLion保留。枚举反射变化必须正常UBT / UHT构建，不用Live Coding。
2. 下面每个文件均为完整绝对路径；只按“查找唯一片段 → 替换为”操作，其余代码保留。函数替换提供完整旧函数与新函数，不创建同名第二份。
3. 下发前已复用本机UE / MSVC参数对13份候选cpp完成**仅编译**检查，均ExitCode0。预检复用旧反射声明，不包含新UHT、链接或运行；正常Build与六专项仍需完成，不能修改任何.generated.h来绕过UHT。
4. 本章不改UI / Editor模块、不添加第四条StartingRelics、不修改旧三件默认或治疗基础40。新枚举只在末尾追加，保留旧资产序号。

## 3. 数据类型与纯规则

### 3.1 ETRelicTypes.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicTypes.h`。

**修改1：只在枚举尾部追加，保留旧序号。**

查找以下唯一片段：

```cpp
// 先实现两种确有消费者计划的加法效果；新机制在对应批次扩展。
```

仅替换为：

```cpp
// 每一种数据参数都对应实际消费者；在尾部追加，保留已保存资产的枚举值。
```

**修改2：追加HealingBonus。**

查找以下唯一片段：

```cpp
    LightAttackBonus UMETA(DisplayName = "Light Attack Bonus")
};
```

仅替换为：

```cpp
    LightAttackBonus UMETA(DisplayName = "Light Attack Bonus"),
    HealingBonus UMETA(DisplayName = "Healing Bonus")
};
```

### 3.2 ETRelicDefinition.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicDefinition.cpp`。

**修改1：原生校验接受第三种参数。**

查找以下唯一片段：

```cpp
           Kind == EETRelicModifierKind::LightAttackBonus;
```

仅替换为：

```cpp
           Kind == EETRelicModifierKind::LightAttackBonus ||
           Kind == EETRelicModifierKind::HealingBonus;
```

### 3.3 ETRelicLoadout.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicLoadout.h`。

**修改1：汇总值增加零默认治疗加成。**

查找以下唯一片段：

```cpp
// 原生计算结果；当前只处理定义支持的两种加法，不模拟任意动态GAS效果。
```

仅替换为：

```cpp
// 原生计算结果；仅汇总已支持的加法，不模拟任意动态GAS效果。
```

**修改2：在Totals尾部追加字段。**

查找以下唯一片段：

```cpp
    float LightAttackBonus = 0.0f;
};
```

仅替换为：

```cpp
    float LightAttackBonus = 0.0f;
    float HealingBonus = 0.0f;
};
```

### 3.4 ETRelicLoadout.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp`。

**修改1：用double累加治疗参数。**

查找以下唯一片段：

```cpp
    double AttackAdditive = 0.0;
```

仅替换为：

```cpp
    double AttackAdditive = 0.0;
    double HealingAdditive = 0.0;
```

**修改2：switch增加对应消费者。**

查找以下唯一片段：

```cpp
                AttackAdditive += Modifier.AdditiveValue;
                break;
            default:
```

仅替换为：

```cpp
                AttackAdditive += Modifier.AdditiveValue;
                break;
            case EETRelicModifierKind::HealingBonus:
                HealingAdditive += Modifier.AdditiveValue;
                break;
            default:
```

**修改3：汇总溢出检查与成功输出同步。**

查找以下唯一片段：

```cpp
        !ETRelicLoadoutPrivate::FitsFloat(AttackAdditive))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
    OutTotals.MaxHealthAdditive = static_cast<float>(HealthAdditive);
    OutTotals.LightAttackBonus = static_cast<float>(AttackAdditive);
```

仅替换为：

```cpp
        !ETRelicLoadoutPrivate::FitsFloat(AttackAdditive) ||
        !ETRelicLoadoutPrivate::FitsFloat(HealingAdditive))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
    OutTotals.MaxHealthAdditive = static_cast<float>(HealthAdditive);
    OutTotals.LightAttackBonus = static_cast<float>(AttackAdditive);
    OutTotals.HealingBonus = static_cast<float>(HealingAdditive);
```

**修改4：纯预览也拒绝非法治疗汇总。**

查找以下唯一片段：

```cpp
    if (!ETRelicLoadoutPrivate::FitsFloat(Health) || !ETRelicLoadoutPrivate::FitsFloat(Damage))
```

仅替换为：

```cpp
    // 最终Q治疗量需要身体能力配置，由装备适配层继续预检；纯规则只检查有限性。
    if (!ETRelicLoadoutPrivate::FitsFloat(Health) || !ETRelicLoadoutPrivate::FitsFloat(Damage) ||
        !FMath::IsFinite(Totals.HealingBonus))
```

## 4. 原生Tag与原装备GE

### 4.1 ETGameplayTags.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h`。

**修改1：增加第三个装备SetByCaller声明。**

查找以下唯一片段：

```cpp
    // ---- Infinite装备GE的两个聚合幅度，不是Instant资源增量 ----
```

仅替换为：

```cpp
    // ---- Infinite装备GE聚合幅度，不是Instant资源增量 ----
```

**修改2：声明治疗装备Tag。**

查找以下唯一片段：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentLightAttackBonus);
```

仅替换为：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentLightAttackBonus);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentHealingBonus);
```

### 4.2 ETGameplayTags.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp`。

**修改1：更新显式幅度说明。**

查找以下唯一片段：

```cpp
    // ---- 聚合装备加法，所有Spec均须显式写入两项（可为0） ----
```

仅替换为：

```cpp
    // ---- 聚合装备加法，所有Spec均须显式写入三项（可为0） ----
```

**修改2：定义治疗装备Tag。**

查找以下唯一片段：

```cpp
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentLightAttackBonus, "SetByCaller.Equipment.LightAttackBonus");
```

仅替换为：

```cpp
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentLightAttackBonus, "SetByCaller.Equipment.LightAttackBonus");
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentHealingBonus, "SetByCaller.Equipment.HealingBonus");
```

### 4.3 ETRelicEquipmentEffect.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h`。

**修改1：更新原生GE输入契约。**

查找以下唯一片段：

```cpp
// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供两个SetByCaller，即使是0。
```

仅替换为：

```cpp
// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供三个SetByCaller，即使是0。
```

### 4.4 ETRelicEquipmentEffect.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp`。

**修改1：在原构造函数末尾增加第三Modifier。**

查找以下唯一片段：

```cpp
    AttackModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackMagnitude);
}
```

仅替换为：

```cpp
    AttackModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackMagnitude);

    FSetByCallerFloat HealingMagnitude;
    HealingMagnitude.DataTag = ETGameplayTags::SetByCaller_EquipmentHealingBonus;
    FGameplayModifierInfo& HealingModifier = Modifiers.AddDefaulted_GetRef();
    HealingModifier.Attribute = UETCombatAttributeSet::GetHealingBonusAttribute();
    HealingModifier.ModifierOp = EGameplayModOp::AddBase;
    HealingModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealingMagnitude);
}
```

## 5. 共享治疗公式与身体只读配置

### 5.1 ETGA_Heal.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Heal.h`。

**修改1：公开无副作用数值计算供装备预检复用。**

查找以下唯一片段：

```cpp
    bool HasValidTagPolicy() const;
```

仅替换为：

```cpp
    bool HasValidTagPolicy() const;
    // 仅根据本能力基础配置与传入加成计算，不要求当前可施放、不读世界或消费次数。
    bool TryCalculateHealAmount(float Bonus, float& OutAmount) const;
```

### 5.2 ETGA_Heal.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Heal.cpp`。

**修改1：用同一计算入口服务正式施放与装备预检。**

查找以下唯一片段：

```cpp
// 只读取本身体ASC，不依赖库存、遗物ID或UI。double中间值避免float相加溢出。
bool UETGA_Heal::TryGetEffectiveHealAmount(UAbilitySystemComponent* ASC, float& OutAmount) const
{
    OutAmount = 0.0f;
    if (!IsValid(ASC) || !ASC->GetSet<UETCombatAttributeSet>())
    {
        return false;
    }
    const float Bonus = ASC->GetNumericAttribute(UETCombatAttributeSet::GetHealingBonusAttribute());
    const double Total = static_cast<double>(HealAmount) + static_cast<double>(Bonus);
    if (!FMath::IsFinite(HealAmount) || !FMath::IsFinite(Bonus) ||
        !FMath::IsFinite(Total) || Total < 1.0 || Total > 1000.0)
    {
        return false;
    }
    OutAmount = static_cast<float>(Total);
    return true;
}
```

仅替换为：

```cpp
// 无副作用计算；范围与正式Q一致，装备预检不得另写一个固定40的公式。
bool UETGA_Heal::TryCalculateHealAmount(const float Bonus, float& OutAmount) const
{
    OutAmount = 0.0f;
    const double Total = static_cast<double>(HealAmount) + static_cast<double>(Bonus);
    if (!FMath::IsFinite(HealAmount) || HealAmount < 1.0f || HealAmount > 1000.0f ||
        !FMath::IsFinite(Bonus) || !FMath::IsFinite(Total) || Total < 1.0 || Total > 1000.0)
    {
        return false;
    }
    OutAmount = static_cast<float>(Total);
    return true;
}

// 施放只读本身体ASC；生效通知继续使用B2-01捕获的ActiveHealAmount。
bool UETGA_Heal::TryGetEffectiveHealAmount(UAbilitySystemComponent* ASC, float& OutAmount) const
{
    OutAmount = 0.0f;
    return IsValid(ASC) && ASC->GetSet<UETCombatAttributeSet>() &&
           TryCalculateHealAmount(
               ASC->GetNumericAttribute(UETCombatAttributeSet::GetHealingBonusAttribute()), OutAmount);
}
```

### 5.3 EmberTrialCharacter.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h`。

**修改1：增加只读治疗配置入口。**

查找以下唯一片段：

```cpp
    const UETGA_LightAttack* GetLightAttackConfiguration() const;
```

仅替换为：

```cpp
    const UETGA_LightAttack* GetLightAttackConfiguration() const;
    const UETGA_Heal* GetHealConfiguration() const;
```

### 5.4 EmberTrialCharacter.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

**修改1：沿用攻击配置的身体边界。**

查找以下唯一片段：

```cpp
bool AEmberTrialCharacter::IsRelicEquipmentReadyForInput() const
```

仅替换为：

```cpp
const UETGA_Heal* AEmberTrialCharacter::GetHealConfiguration() const
{
    return !bAbilitySystemShuttingDown && IsValid(HealAbilityClass.Get())
        ? HealAbilityClass->GetDefaultObject<UETGA_Heal>() : nullptr;
}

bool AEmberTrialCharacter::IsRelicEquipmentReadyForInput() const
```

**核对：** 原`HandleHealCommit`仍用`ActiveHealAmount`，CanActivate / Activate仍通过`TryGetEffectiveHealAmount`；不要在生效点重新读取GE。新公开计算只做纯数值检查，不要求当前Idle / 未暂停，否则祭坛暂停预览会被错误拒绝。

## 6. 自有句柄与正式预检

### 6.1 ETRelicEquipmentComponent.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h`。

**修改1：同步扩展最终能力数值预检的名称。**

查找以下唯一片段：

```cpp
    bool IsFinalDamageValid(const FETRelicModifierTotals& Modifiers) const;
```

仅替换为：

```cpp
    bool AreFinalAbilityValuesValid(const FETRelicModifierTotals& Modifiers) const;
```

### 6.2 ETRelicEquipmentComponent.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp`。

**修改1：增加治疗能力显式依赖。**

查找以下唯一片段：

```cpp
#include "Abilities/ETGA_LightAttack.h"
```

仅替换为：

```cpp
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Heal.h"
```

**修改2：真实Spec完整性校验包含第三个幅度。**

查找以下唯一片段：

```cpp
    return FMath::IsFinite(Health) && FMath::IsFinite(Attack) &&
           Health == AppliedModifiers.MaxHealthAdditive && Attack == AppliedModifiers.LightAttackBonus;
```

仅替换为：

```cpp
    const float Healing = Active->Spec.GetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_EquipmentHealingBonus, false, Missing);
    return FMath::IsFinite(Health) && FMath::IsFinite(Attack) && FMath::IsFinite(Healing) &&
           Health == AppliedModifiers.MaxHealthAdditive &&
           Attack == AppliedModifiers.LightAttackBonus && Healing == AppliedModifiers.HealingBonus;
```

**修改3：零幅度槽显式写入第三项。**

查找以下唯一片段：

```cpp
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 0.0f);
```

仅替换为：

```cpp
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 0.0f);
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 0.0f);
```

**修改4：数值合法性同时检查治疗加成。**

查找以下唯一片段：

```cpp
    if (!FMath::IsFinite(Modifiers.MaxHealthAdditive) || !FMath::IsFinite(Modifiers.LightAttackBonus))
```

仅替换为：

```cpp
    if (!FMath::IsFinite(Modifiers.MaxHealthAdditive) ||
        !FMath::IsFinite(Modifiers.LightAttackBonus) || !FMath::IsFinite(Modifiers.HealingBonus))
```

**修改5：用当前值减去自有旧贡献再加新贡献。**

查找以下唯一片段：

```cpp
    const double Limit = std::numeric_limits<float>::max();
    return FMath::IsFinite(NewMax) && NewMax >= 1.0 && NewMax <= Limit &&
           FMath::IsFinite(NewBonus) && FMath::Abs(NewBonus) <= Limit;
```

仅替换为：

```cpp
    const double NewHealing = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetHealingBonusAttribute())) - AppliedModifiers.HealingBonus +
        Modifiers.HealingBonus;
    const double Limit = std::numeric_limits<float>::max();
    return FMath::IsFinite(NewMax) && NewMax >= 1.0 && NewMax <= Limit &&
           FMath::IsFinite(NewBonus) && FMath::Abs(NewBonus) <= Limit &&
           FMath::IsFinite(NewHealing) && FMath::Abs(NewHealing) <= Limit;
```

**修改6：只变治疗加成也属于真实更新。**

查找以下唯一片段：

```cpp
        Modifiers.LightAttackBonus == AppliedModifiers.LightAttackBonus)
```

仅替换为：

```cpp
        Modifiers.LightAttackBonus == AppliedModifiers.LightAttackBonus &&
        Modifiers.HealingBonus == AppliedModifiers.HealingBonus)
```

**修改7：同一个句柄批量更新三项，保留DirtyBatch。**

查找以下唯一片段：

```cpp
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, Modifiers.LightAttackBonus);
```

仅替换为：

```cpp
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, Modifiers.LightAttackBonus);
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentHealingBonus, Modifiers.HealingBonus);
```

**修改8：同步注释范围。**

查找以下唯一片段：

```cpp
        // 两个聚合器都更新后才派发脏通知；这不是任意GAS / 库存事务的原子锁。
```

仅替换为：

```cpp
        // 三个聚合器更新后才派发脏通知；这不是任意GAS / 库存事务的原子锁。
```

**修改9：恢复预检调用新名称。**

查找以下唯一片段：

```cpp
        !AreModifiersValid(ASC, Totals) || !IsFinalDamageValid(Totals))
```

仅替换为：

```cpp
        !AreModifiersValid(ASC, Totals) || !AreFinalAbilityValuesValid(Totals))
```

**修改10：完整替换最终能力数值预检函数。**

查找以下唯一片段：

```cpp
bool UETRelicEquipmentComponent::IsFinalDamageValid(const FETRelicModifierTotals& Modifiers) const
{
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    UAbilitySystemComponent* ASC = EffectASC.Get();
    const UETGA_LightAttack* Attack = IsValid(Body) ? Body->GetLightAttackConfiguration() : nullptr;
    // 未配置攻击的隔离身体只允许零攻击加成；正式玩家的配置在资产校验中保证。
    if (!IsValid(Attack))
    {
        return Modifiers.LightAttackBonus == 0.0f;
    }
    const double Bonus = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetLightAttackBonusAttribute())) - AppliedModifiers.LightAttackBonus +
        Modifiers.LightAttackBonus;
    return FMath::IsFinite(Bonus) && FMath::Abs(Bonus) <= std::numeric_limits<float>::max() &&
           Attack->IsDamageBonusValid(static_cast<float>(Bonus));
}
```

仅替换为：

```cpp
bool UETRelicEquipmentComponent::AreFinalAbilityValuesValid(
    const FETRelicModifierTotals& Modifiers) const
{
    const AEmberTrialCharacter* Body = BodyOwner.Get();
    UAbilitySystemComponent* ASC = EffectASC.Get();
    if (!IsValid(Body) || !IsValid(ASC))
    {
        return false;
    }
    const UETGA_LightAttack* Attack = Body->GetLightAttackConfiguration();
    const double AttackBonus = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetLightAttackBonusAttribute())) - AppliedModifiers.LightAttackBonus +
        Modifiers.LightAttackBonus;
    const double HealingBonus = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetHealingBonusAttribute())) - AppliedModifiers.HealingBonus +
        Modifiers.HealingBonus;
    const double Limit = std::numeric_limits<float>::max();
    if (!FMath::IsFinite(AttackBonus) || FMath::Abs(AttackBonus) > Limit ||
        !FMath::IsFinite(HealingBonus) || FMath::Abs(HealingBonus) > Limit ||
        (IsValid(Attack) ? !Attack->IsDamageBonusValid(static_cast<float>(AttackBonus))
                         : Modifiers.LightAttackBonus != 0.0f))
    {
        return false;
    }
    // 没有治疗能力的隔离身体只允许零装备治疗贡献；正式玩家读取真实GA基础值。
    const UETGA_Heal* Heal = Body->GetHealConfiguration();
    float Amount = 0.0f;
    return IsValid(Heal) ? Heal->TryCalculateHealAmount(static_cast<float>(HealingBonus), Amount)
                         : Modifiers.HealingBonus == 0.0f;
}
```

**修改11：预览与正式提交复用完整能力预检。**

查找以下唯一片段：

```cpp
    if (!AreModifiersValid(ASC, Totals) || !IsFinalDamageValid(Totals))
```

仅替换为：

```cpp
    if (!AreModifiersValid(ASC, Totals) || !AreFinalAbilityValuesValid(Totals))
```

**核对：** `AreFinalAbilityValuesValid`应出现3次：函数定义、恢复调用、预览 / 提交共用预检调用。旧`IsFinalDamageValid`在该.h / .cpp中应0处；不要更改旧的ReadPreviewBases / 提交点 / DirtyBatch清理顺序。

## 7. 提交时的方案重验

### 7.1 ETRelicInventorySubsystem.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp`。

**修改1：提交时重算比较不能遗漏治疗参数。**

查找以下唯一片段：

```cpp
        return X.MaxHealthAdditive == Y.MaxHealthAdditive && X.LightAttackBonus == Y.LightAttackBonus;
```

仅替换为：

```cpp
        return X.MaxHealthAdditive == Y.MaxHealthAdditive &&
               X.LightAttackBonus == Y.LightAttackBonus && X.HealingBonus == Y.HealingBonus;
```

## 8. 既有测试与隔离夹具

### 8.1 ETRelicLoadoutTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp`。

**修改1：追加治疗参数纯规则样本，不改保存的三遗物。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Overflow output cleared"), Totals.MaxHealthAdditive, 0.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Overflow output cleared"), Totals.MaxHealthAdditive, 0.0f);

    // 同样的两槽规则可汇总治疗参数；只有身体能力知道最终Q基础值。
    Life->Modifiers[0].Kind = EETRelicModifierKind::HealingBonus;
    Edge->Modifiers[0].Kind = EETRelicModifierKind::HealingBonus;
    Life->Modifiers[0].AdditiveValue = 7.0f;
    Edge->Modifiers[0].AdditiveValue = 3.0f;
    TestTrue(TEXT("Healing kinds accepted by definition and sum"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::Success);
    TestEqual(TEXT("Two healing contributions sum ten"), Totals.HealingBonus, 10.0f);
    TestEqual(TEXT("Healing leaves attack sum zero"), Totals.LightAttackBonus, 0.0f);
    TestTrue(TEXT("Healing replacement plan accepted"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::Success);
    TestEqual(TEXT("Plan carries healing seven"), Plan.AfterModifiers.HealingBonus, 7.0f);
    TestEqual(TEXT("Healing does not raise max"), Plan.AfterPreview.MaxHealth, 100.0f);
    TestEqual(TEXT("Healing does not raise attack"), Plan.AfterPreview.LightAttackBaseDamage, 25.0f);
    Life->Modifiers[0].AdditiveValue = -4.0f;
    TestTrue(TEXT("Signed healing input remains legal"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::Success);
    TestEqual(TEXT("Signed healing sum"), Totals.HealingBonus, -1.0f);
    Life->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    Edge->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    TestTrue(TEXT("Healing overflow rejected"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::NumericOverflow);
    TestEqual(TEXT("Healing overflow clears output"), Totals.HealingBonus, 0.0f);
```

### 8.2 ETRelicEquipmentEffectsTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp`。

**修改1：效果夹具支持第三项，默认零。**

查找以下唯一片段：

```cpp
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive)
```

仅替换为：

```cpp
    const FGameplayAttribute Healing = UETCombatAttributeSet::GetHealingBonusAttribute();
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive,
                       const float HealingAdditive = 0.0f)
```

**修改2：每个测试Spec显式给治疗幅度。**

查找以下唯一片段：

```cpp
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, AttackAdditive);
```

仅替换为：

```cpp
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, AttackAdditive);
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, HealingAdditive);
```

**修改3：首份GE附加真实加成10。**

查找以下唯一片段：

```cpp
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f);
```

仅替换为：

```cpp
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f, 10.0f);
```

**修改4：验证聚合Current与Base隔离。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Increasing max does not heal"), ASC->GetNumericAttribute(Health), 80.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Healing Current ten"), ASC->GetNumericAttribute(Healing), 10.0f);
    TestEqual(TEXT("Healing Base remains zero"), ASC->GetNumericAttributeBase(Healing), 0.0f);
    TestEqual(TEXT("Increasing max does not heal"), ASC->GetNumericAttribute(Health), 80.0f);
```

**修改5：同句柄替换携带完整三项。**

查找以下唯一片段：

```cpp
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 3.0f);
```

仅替换为：

```cpp
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 3.0f);
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 7.0f);
```

**修改6：更新后验证治疗值7。**

查找以下唯一片段：

```cpp
    TestTrue(TEXT("Owned handle still live"), ASC->GetActiveGameplayEffect(OwnedHandle) != nullptr);
```

仅替换为：

```cpp
    TestEqual(TEXT("Same handle updates healing"), ASC->GetNumericAttribute(Healing), 7.0f);
    TestTrue(TEXT("Owned handle still live"), ASC->GetActiveGameplayEffect(OwnedHandle) != nullptr);
```

**修改7：另一独立GE携带加成4。**

查找以下唯一片段：

```cpp
    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f);
```

仅替换为：

```cpp
    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f, 4.0f);
```

**修改8：删除自己效果保留其他治疗贡献。**

查找以下唯一片段：

```cpp
    TestTrue(TEXT("Remove second handle"), ASC->RemoveActiveGameplayEffect(OtherHandle));
```

仅替换为：

```cpp
    TestEqual(TEXT("Other healing survives exact removal"), ASC->GetNumericAttribute(Healing), 4.0f);
    TestTrue(TEXT("Remove second handle"), ASC->RemoveActiveGameplayEffect(OtherHandle));
    TestEqual(TEXT("Removed healing returns zero"), ASC->GetNumericAttribute(Healing), 0.0f);
    TestEqual(TEXT("Other body healing isolated"), OtherASC->GetNumericAttribute(Healing), 0.0f);
```

### 8.3 ETRelicEquipmentOwnershipTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentOwnershipTests.cpp`。

**修改1：增加治疗属性句柄。**

查找以下唯一片段：

```cpp
    const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
```

仅替换为：

```cpp
    const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    const FGameplayAttribute Healing = UETCombatAttributeSet::GetHealingBonusAttribute();
```

**修改2：验证零默认与仅治疗更新，同句柄身份保持。**

查找以下唯一片段：

```cpp
    ASC->GetGameplayAttributeValueChangeDelegate(MaxHealth).Remove(MaxObserver);
```

仅替换为：

```cpp
    ASC->GetGameplayAttributeValueChangeDelegate(MaxHealth).Remove(MaxObserver);

    TestEqual(TEXT("Empty healing contribution"), ASC->GetNumericAttribute(Healing), 0.0f);
    TestTrue(TEXT("Healing-only change is applied"),
             Equipment->TrySetModifiers({40.0f, 7.0f, 10.0f}) == EWrite::Applied);
    TestEqual(TEXT("Owned healing ten"), ASC->GetNumericAttribute(Healing), 10.0f);
    TestTrue(TEXT("Healing update keeps exact handle"), Equipment->OwnedEffect == InitialHandle);
    TestTrue(TEXT("Same healing totals are unchanged"),
             Equipment->TrySetModifiers({40.0f, 7.0f, 10.0f}) == EWrite::Unchanged);
    TMap<FGameplayTag, float> Tamper;
    Tamper.Add(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 99.0f);
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(InitialHandle, Tamper);
    TestFalse(TEXT("Third magnitude tampering detected"), Equipment->GetAppliedModifiers(Snapshot));
    Tamper[ETGameplayTags::SetByCaller_EquipmentHealingBonus] = 10.0f;
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(InitialHandle, Tamper);
    TestTrue(TEXT("Exact spec restored"), Equipment->GetAppliedModifiers(Snapshot));
```

**修改3：独立GE治疗幅度3。**

查找以下唯一片段：

```cpp
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 2.0f);
```

仅替换为：

```cpp
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 2.0f);
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 3.0f);
```

**修改4：失活精确移除不能清除其他治疗GE。**

查找以下唯一片段：

```cpp
    Equipment->Activate(false);
    TestTrue(TEXT("Explicit reactivation starts zero slot"), Equipment->GetAppliedModifiers(Snapshot));
```

仅替换为：

```cpp
    TestEqual(TEXT("Foreign healing survives deactivation"), ASC->GetNumericAttribute(Healing), 3.0f);
    Equipment->Activate(false);
    TestTrue(TEXT("Explicit reactivation starts zero slot"), Equipment->GetAppliedModifiers(Snapshot));
```

### 8.4 ETLightComboFlowTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp`。

**修改1：原连段Spec补零，保留旧装备 / 伤害断言。**

查找以下唯一片段：

```cpp
            ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 5.0f);
```

仅替换为：

```cpp
            ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 5.0f);
        EquipmentSpec.Data->SetSetByCallerMagnitude(
            ETGameplayTags::SetByCaller_EquipmentHealingBonus, 0.0f);
```

### 8.5 ETRelicDefinition.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicDefinition.h`。

**修改1：仅向既有事务测试开放隔离数据夹具。**

查找以下唯一片段：

```cpp
    friend class FETRelicLoadoutTest;
```

仅替换为：

```cpp
    friend class FETRelicLoadoutTest;
    friend class FETRelicEquipmentTransactionTest;
```

### 8.6 ETRelicEquipmentTransactionTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp`。

**修改1：增加纯配置与定义依赖。**

查找以下唯一片段：

```cpp
#include "Abilities/ETGA_LightAttack.h"
```

仅替换为：

```cpp
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Heal.h"
#include "Inventory/ETRelicDefinition.h"
```

**修改2：隔离身体提供正式基础40配置，不播放动画。**

查找以下唯一片段：

```cpp
            Result->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
```

仅替换为：

```cpp
            Result->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
            Result->HealAbilityClass = UETGA_Heal::StaticClass();
```

**修改3：在旧NewRun断言之后、外部移除之前插入治疗事务场景。**

查找以下唯一片段：

```cpp
    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
    UAbilitySystemComponent* NewASC = NewRunBody->GetAbilitySystemComponent();
```

仅替换为：

```cpp
    // ---- 治疗参数预检 / 过期方案 / 同句柄更新；瞬态定义不写正式资产或配置 ----
    UAbilitySystemComponent* NewASC = NewRunBody->GetAbilitySystemComponent();
    const FGameplayAttribute Healing = UETCombatAttributeSet::GetHealingBonusAttribute();
    UETRelicDefinition* HealFixture = NewObject<UETRelicDefinition>(Inventory);
    const FName HealId(TEXT("fixture_healing"));
    HealFixture->RelicId = HealId;
    HealFixture->DisplayName = FText::FromString(TEXT("Healing fixture"));
    HealFixture->Description = FText::FromString(TEXT("Transient healing transaction fixture"));
    FETRelicModifier HealModifier;
    HealModifier.Kind = EETRelicModifierKind::HealingBonus;
    HealModifier.AdditiveValue = 1000.0f;
    HealFixture->Modifiers = {HealModifier};
    Inventory->Definitions.Add(HealId, HealFixture);
    Inventory->OwnedIds.Add(HealId);
    Inventory->OwnedSet.Add(HealId);
    Inventory->AdvanceRevision();
    Inventory->PublishSnapshot();
    const uint64 HealingRevision = Inventory->GetSnapshot().Revision;
    auto EquipHealing = [&]()
    {
        return NewEquipment->TryEquip(
            EETRelicSlot::First, HealId, Inventory->GetSnapshot().Revision).Result;
    };
    TestTrue(TEXT("Excess healing preview rejected"), NewEquipment->TryBuildPreview(
        EETRelicSlot::First, HealId, HealingRevision, Preview).Result == EResult::RuleRejected);
    TestTrue(TEXT("Excess healing commit rejected"), EquipHealing() == EResult::RuleRejected);
    TestTrue(TEXT("Rejected healing keeps revision"), Inventory->GetSnapshot().Revision == HealingRevision);
    TestEqual(TEXT("Rejected healing keeps zero"), NewASC->GetNumericAttribute(Healing), 0.0f);

    HealFixture->Modifiers[0].AdditiveValue = 10.0f;
    FETRelicInventoryPlan HealingPlan;
    TestTrue(TEXT("Healing domain plan created"), Inventory->TryBuildPlan(
        EETRelicSlot::First, HealId, 100.0f, 25.0f, HealingPlan).Result == EResult::Success);
    HealFixture->Modifiers[0].AdditiveValue = 11.0f;
    TestTrue(TEXT("Healing definition change invalidates old plan"),
             Inventory->TryCommitPlan(HealingPlan).Result == EResult::StalePlan);
    TestTrue(TEXT("Stale healing plan keeps revision"), Inventory->GetSnapshot().Revision == HealingRevision);
    HealFixture->Modifiers[0].AdditiveValue = 10.0f;
    NewRunBody->HealAbilityClass = nullptr;
    TestTrue(TEXT("Missing heal consumer rejects nonzero contribution"), EquipHealing() == EResult::RuleRejected);
    NewRunBody->HealAbilityClass = UETGA_Heal::StaticClass();
    TestTrue(TEXT("Healing preview has no side effect"), NewEquipment->TryBuildPreview(
        EETRelicSlot::First, HealId, HealingRevision, Preview).Result == EResult::Success);
    TestEqual(TEXT("Healing preview carries ten"), Preview.AfterModifiers.HealingBonus, 10.0f);
    TestEqual(TEXT("Preview leaves actual bonus zero"), NewASC->GetNumericAttribute(Healing), 0.0f);
    const FActiveGameplayEffectHandle HealingHandle = NewEquipment->OwnedEffect;
    TestTrue(TEXT("Equip healing contribution"), EquipHealing() == EResult::Success);
    TestEqual(TEXT("Equipment aggregates healing ten"), NewASC->GetNumericAttribute(Healing), 10.0f);
    TestEqual(TEXT("Equipment keeps healing Base zero"), NewASC->GetNumericAttributeBase(Healing), 0.0f);
    TestTrue(TEXT("Healing updates same owned handle"), NewEquipment->OwnedEffect == HealingHandle);
    NewASC->SetNumericAttributeBase(Health, 60.0f);
    NewEquipment->Deactivate();
    NewEquipment->Activate(false);
    TestEqual(TEXT("Reactivation restores healing contribution"), NewASC->GetNumericAttribute(Healing), 10.0f);
    TestEqual(TEXT("Healing restore does not heal living body"), NewASC->GetNumericAttribute(Health), 60.0f);
    TestTrue(TEXT("Unequip healing"), NewEquipment->TryEquip(
        EETRelicSlot::First, NAME_None, Inventory->GetSnapshot().Revision).Result == EResult::Success);
    TestEqual(TEXT("Unequip clears own healing"), NewASC->GetNumericAttribute(Healing), 0.0f);
    TestEqual(TEXT("Unequip does not heal"), NewASC->GetNumericAttribute(Health), 60.0f);

    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
```

事务测试的fixture_healing只是瞬态目录样本，结束随测试世界释放；不写.uasset、不增加配置或自动化入口。新friend只给既有测试构造私有静态数据，正式Widget不能借此写库存。连段Spec补零是第三幅度成为必填后的兼容修改，所有旧三段伤害断言保留。

## 9. 正常Build与索引

1. 确认21份文件保存，UE已关闭。PowerShell从仓库根目录正常构建Editor并刷新三索引：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

这一次Editor Build会重新运行UHT。CLion按提示Reload CMake，不需要清缓存 / 删除工程，也不用再重复单独Editor Build。

2. 同一窗口进行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

3. 两者成功再打开工程。此批不改任何蓝图默认值，不要求全Content重存，不运行Cook。

## 10. 六个相关专项（各一次）

保持**非Play**，在Output Log底部命令行依次运行。上一项显示Success再执行下一项：

```text
Automation RunTests EmberTrial.M3.Inventory.LoadoutRules
Automation RunTests EmberTrial.M3.Inventory.EquipmentEffects
Automation RunTests EmberTrial.M3.Inventory.EquipmentOwnership
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
Automation RunTests EmberTrial.M2.Healing.Ability
Automation RunTests EmberTrial.M2.Combo.Flow
```

| 专项 | 本次必要覆盖 |
| --- | --- |
| LoadoutRules | 新Kind原生校验、两槽求和 / 负值 / 溢出与清输出 |
| EquipmentEffects | 正式三Modifier GE真实聚合、Current / Base与精确删除 |
| EquipmentOwnership | 仅治疗变化、同句柄、第三项篡改识别、外部GE保留 |
| EquipmentTransactions | 超1000提前拒绝 / 无消费或版本改变、过期参数拒绝、缺消费者拒绝、恢复不治疗 |
| Healing.Ability | 共享公式之后，真实通知50 / 下次40与全部原取消边界保持 |
| Combo.Flow | 旧连段的装备Spec补第三项零后仍按真实三段结算 |

这些是此次实际改到或直接调用的路径，不跑全42项、UI整套 / AI矩阵。若出现缺SetByCaller提示，不忽略Warning；发首条文字错误和测试全名，我会检查是否漏了某个Spec补零。成功长日志由我读取。

## 11. 默认路径短查（约3分钟）

1. 正式地图新Play，不装备，生命100/100、治疗3次、库存仍三件；第四件与治疗比较文字尚未提供。
2. 安全站定，已有数字4造成两次25伤害到50；等受击结束按Q，通知生效后90/100、次数2/3，动作结束可移动。
3. P打开 / 继续一次，移动转向，左键或2任选一项；Stop再Play确认新100 / 3次，随后Stop。三件遗物和所有默认配置保持。
4. 保存源码即可，无临时蓝图、资产、导航或帧率设置需要清理。不重复01完整检查，也不要求你手动注入+10或抓精确通知时点：真实GE / 组件更新由专项负责，第四件的实际装备与UI由03验收。

## 12. 完成反馈

```text
M3-06B2-02 完成
21份既有Core源码；UE关闭后Editor Build / 三索引 / Game Build：
LoadoutRules / EquipmentEffects / EquipmentOwnership / EquipmentTransactions：
Healing.Ability / Combo.Flow各一次：
正式三件保持；50→Q90与次数2 / 动作恢复：
P与最短旧输入 / Stop新100与3次：
Source已保存、资产配置保持；失败或不理解处（无填无）：
完成时分 / 手动代码与理解 / 检查记录 / 构建等待：
```

学习时重点解释：聚合Current包含外部贡献，替换自己的装备必须先减旧贡献；仅治疗变化不能被旧两字段比较当成NoChange；旧方案即使ID未变，也必须重验新增参数。清理只移除自有句柄，不清整张ASC。

## 13. Commit / Push（固定放在最后）

本轮B2-01验收收尾只提交下面**11份文档**。本地c354f4f已包含B2-01六份Core实现，不需要重复Stage；B2-02候选未写正式Source，当前不提交候选或未实现功能。

```text
Docs/Reports/M3_06B2_01_Verification.md
Docs/M3_06B2_01_HealingBonusFoundation.md
Docs/M3_06B2_02_HealingEquipmentPipeline.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文：`docs: verify healing snapshot and plan equipment pipeline`
- 专业中文：`docs: 完成治疗施放快照验收并规划装备增益管线`

Artifacts候选 / 编译预检 / 日志 / JSON、包和个人ini保持本地。Stage后继续编辑需重新Stage → Review → Commit / Push，由你操作，助手未核验远端。

**你实施并验收本小节后，再提交以下21份既有Core的真实改动：**

```text
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicTypes.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicDefinition.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicLoadout.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h
EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Heal.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Heal.cpp
EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentOwnershipTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicDefinition.h
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp
```

- 英文：`feat: extend relic equipment pipeline with healing bonus`
- 专业中文：`feat: 扩展遗物装备管线以支持治疗加成`

届时新验收报告 / 状态有实际修改再一并Stage；不要提交Artifacts候选、第四件空资产、个人配置或构建产物。
