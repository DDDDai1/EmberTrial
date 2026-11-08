# M3-06B3-02：体力恢复加成的装备管线

2026-11-07下发，UE **5.5.4**。[B3-01恢复消费者已验收](Reports/M3_06B3_01_Verification.md)，无待补。本章由你手动修改**19份既有Core文件**，无新文件 / 资产 / Config；当前只有Artifacts候选，正式源码尚未改变。第五件与UI比较留B3-03。

## 1. 这一章做什么

```text
StaminaRecoveryBonus定义 → 双槽纯汇总 / 方案重验
  → 真实恢复组件预检基础速率 + 聚合加成
  → 原装备GE第四SetByCaller / 同句柄更新
  → 装备写锁退出后复核恢复Timer → 已完成的实际恢复路径
```

| 范围 | 合同 |
| --- | --- |
| 类型 / 汇总 | 枚举只在尾部追加，有限有符号加法 / 溢出检查，旧三种参数保持 |
| 原装备GE | 3→4个Modifier / SetByCaller；包括测试Spec在内全部显式赋值，可为0 |
| 自有句柄 | 新幅度参与完整性 / 同值 / 更新 / 清理，外部GE贡献保留 |
| 正式预检 | 当前聚合值减自有旧贡献再加新贡献，读取实际StaminaRecovery组件配置，不硬写20 |
| 方案重验 | ID未变但恢复参数改变，也拒绝旧方案 |
| 完成发布 | 组件写锁退出后复核Timer资格，再广播原装备事件，不立即补体力 |
| 相同值通知 | 保留当前采样起点；只有实际速率改变才执行01的重采样规则 |

**接入边界：**装备写入时`bChanging`暂时使角色非Idle，恢复属性回调可能据此停表。新`PublishEquipmentChanged`在完整写入后调用恢复组件的`RefreshRecoveryState`再发原事件，重新检查资格。恢复组件仍只依赖身体 / ASC，不引用库存或祭坛；实际资源仍只在原Timer回调修改。

19份文件由13份生产声明 / 实现和6份既有测试组成。当前正式库存仍四件、UI仍只有原生命 / 轻击 / 治疗比较行；恢复遗物和新行由03接入，不提前创建第五条StartingRelics。

预计手动90–130分钟（代码与理解约75–110、检查记录10–20），构建等待另记。可先保存第3–5节，再完成第6–8节后统一Build。规模保持233 Source / 133cpp、42测试、Content311 / 来源94。

## 2. 开始前

1. Stop PIE → Save All → 关闭UE，CLion保留。新增枚举与原生GE构造变化，使用正常UBT / UHT构建，不用Live Coding。
2. 所有文件均在EmberTrial核心模块。逐个“查找唯一片段 → 替换为”，其余代码保留。没有新C++类或蓝图接线。
3. 下发前13份候选cpp仅编译预检ExitCode0；复用旧反射声明，不包含新UHT / 链接 / 运行。第9节正常Build与第10节专项仍需完成，不修改.generated.h。

## 3. 原生数据与纯规则

### 3.1 ETRelicTypes.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicTypes.h`。

**修改1：在枚举末尾追加，保留旧资产序号。**

查找以下唯一片段：

```cpp
    HealingBonus UMETA(DisplayName = "Healing Bonus")
};
```

仅替换为：

```cpp
    HealingBonus UMETA(DisplayName = "Healing Bonus"),
    StaminaRecoveryBonus UMETA(DisplayName = "Stamina Recovery Bonus")
};
```

### 3.2 ETRelicDefinition.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicDefinition.cpp`。

**修改1：原生定义校验接受恢复加成。**

查找以下唯一片段：

```cpp
           Kind == EETRelicModifierKind::HealingBonus;
```

仅替换为：

```cpp
           Kind == EETRelicModifierKind::HealingBonus ||
           Kind == EETRelicModifierKind::StaminaRecoveryBonus;
```

### 3.3 ETRelicLoadout.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicLoadout.h`。

**修改1：汇总结果尾部增加零默认字段。**

查找以下唯一片段：

```cpp
    float HealingBonus = 0.0f;
};
```

仅替换为：

```cpp
    float HealingBonus = 0.0f;
    float StaminaRecoveryBonus = 0.0f;
};
```

### 3.4 ETRelicLoadout.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp`。

**修改1：新增双精度中间和。**

查找以下唯一片段：

```cpp
    double HealingAdditive = 0.0;
```

仅替换为：

```cpp
    double HealingAdditive = 0.0;
    double RecoveryAdditive = 0.0;
```

**修改2：汇总第四种类型。**

查找以下唯一片段：

```cpp
                HealingAdditive += Modifier.AdditiveValue;
                break;
            default:
```

仅替换为：

```cpp
                HealingAdditive += Modifier.AdditiveValue;
                break;
            case EETRelicModifierKind::StaminaRecoveryBonus:
                RecoveryAdditive += Modifier.AdditiveValue;
                break;
            default:
```

**修改3：溢出与成功输出同步扩展。**

查找以下唯一片段：

```cpp
        !ETRelicLoadoutPrivate::FitsFloat(HealingAdditive))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
```

仅替换为：

```cpp
        !ETRelicLoadoutPrivate::FitsFloat(HealingAdditive) ||
        !ETRelicLoadoutPrivate::FitsFloat(RecoveryAdditive))
    {
        return EETRelicLoadoutResult::NumericOverflow;
    }
```

**修改4：输出恢复加成。**

查找以下唯一片段：

```cpp
    OutTotals.HealingBonus = static_cast<float>(HealingAdditive);
```

仅替换为：

```cpp
    OutTotals.HealingBonus = static_cast<float>(HealingAdditive);
    OutTotals.StaminaRecoveryBonus = static_cast<float>(RecoveryAdditive);
```

**修改5：纯预览检查有限性，身体层负责最终速率。**

查找以下唯一片段：

```cpp
    // 最终Q治疗量需要身体能力配置，由装备适配层继续预检；纯规则只检查有限性。
```

仅替换为：

```cpp
    // 最终治疗量 / 恢复速率由身体配置继续预检；纯规则检查参数有限性。
```

**修改6：预览不接受非有限恢复参数。**

查找以下唯一片段：

```cpp
        !FMath::IsFinite(Totals.HealingBonus))
```

仅替换为：

```cpp
        !FMath::IsFinite(Totals.HealingBonus) ||
        !FMath::IsFinite(Totals.StaminaRecoveryBonus))
```

## 4. 原生Tag与原装备GE第四幅度

### 4.1 ETGameplayTags.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h`。

**修改1：声明第四个装备幅度。**

查找以下唯一片段：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentHealingBonus);
```

仅替换为：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentHealingBonus);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentStaminaRecoveryBonus);
```

### 4.2 ETGameplayTags.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp`。

**修改1：明确四项必填。**

查找以下唯一片段：

```cpp
    // ---- 聚合装备加法，所有Spec均须显式写入三项（可为0） ----
```

仅替换为：

```cpp
    // ---- 聚合装备加法，所有Spec均须显式写入四项（可为0） ----
```

**修改2：定义恢复速率的装备Tag。**

查找以下唯一片段：

```cpp
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentHealingBonus, "SetByCaller.Equipment.HealingBonus");
```

仅替换为：

```cpp
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentHealingBonus, "SetByCaller.Equipment.HealingBonus");
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentStaminaRecoveryBonus,
                           "SetByCaller.Equipment.StaminaRecoveryBonus");
```

### 4.3 ETRelicEquipmentEffect.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h`。

**修改1：更新原GE的输入契约。**

查找以下唯一片段：

```cpp
// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供三个SetByCaller，即使是0。
```

仅替换为：

```cpp
// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供四个SetByCaller，即使是0。
```

### 4.4 ETRelicEquipmentEffect.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp`。

**修改1：原GE增加第四Modifier。**

查找以下唯一片段：

```cpp
    HealingModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealingMagnitude);
}
```

仅替换为：

```cpp
    HealingModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealingMagnitude);

    FSetByCallerFloat RecoveryMagnitude;
    RecoveryMagnitude.DataTag = ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus;
    FGameplayModifierInfo& RecoveryModifier = Modifiers.AddDefaulted_GetRef();
    RecoveryModifier.Attribute = UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute();
    RecoveryModifier.ModifierOp = EGameplayModOp::AddBase;
    RecoveryModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RecoveryMagnitude);
}
```

## 5. 恢复消费者的完成复核与同值通知

### 5.1 ETStaminaRecoveryComponent.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Resources/ETStaminaRecoveryComponent.h`。

**修改1：提供事务写入结束后的资格复核入口。**

查找以下唯一片段：

```cpp
    bool TryCalculateRecoveryRate(float Bonus, float& OutRate) const;
```

仅替换为：

```cpp
    bool TryCalculateRecoveryRate(float Bonus, float& OutRate) const;

    // 仅复核当前计时资格；保留观察订阅、消费延迟与仍有效的Timer。
    void RefreshRecoveryState();
```

### 5.2 ETStaminaRecoveryComponent.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp`。

**修改1：实现无资源副作用的资格复核。**

查找以下唯一片段：

```cpp
void UETStaminaRecoveryComponent::ClearRecoveryTimer(const TCHAR* Reason)
```

仅替换为：

```cpp
void UETStaminaRecoveryComponent::RefreshRecoveryState()
{
    SyncRecoveryTimer();
}

void UETStaminaRecoveryComponent::ClearRecoveryTimer(const TCHAR* Reason)
```

**修改2：相同值通知保留当前积分起点。**

查找以下唯一片段：

```cpp
    if (Change.Attribute == UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute())
    {
        // 以变化时刻作为新积分起点
```

仅替换为：

```cpp
    if (Change.Attribute == UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute())
    {
        if (Change.NewValue == Change.OldValue)
        {
            return;
        }
        // 以变化时刻作为新积分起点
```

**核对：** `RefreshRecoveryState`只调用原`SyncRecoveryTimer`，不重新订阅、不改RecoveryNotBefore、不Apply GE。同值分支使用精确相等，实际数值改变仍走01原规则。

## 6. 装备组件更新与统一发布

### 6.1 ETRelicEquipmentComponent.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h`。

**修改1：集中发布完成事件前复核资源消费者。**

查找以下唯一片段：

```cpp
    void WriteModifiersUnchecked(const FETRelicModifierTotals& Modifiers);
```

仅替换为：

```cpp
    void WriteModifiersUnchecked(const FETRelicModifierTotals& Modifiers);
    void PublishEquipmentChanged();
```

### 6.2 ETRelicEquipmentComponent.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp`。

**修改1：引用已完成的速率消费者。**

查找以下唯一片段：

```cpp
#include "Attributes/ETHealthAttributeSet.h"
```

仅替换为：

```cpp
#include "Attributes/ETHealthAttributeSet.h"
#include "Resources/ETStaminaRecoveryComponent.h"
```

**修改2：真实Spec完整性包含第四项。**

查找以下唯一片段：

```cpp
    return FMath::IsFinite(Health) && FMath::IsFinite(Attack) && FMath::IsFinite(Healing) &&
           Health == AppliedModifiers.MaxHealthAdditive &&
           Attack == AppliedModifiers.LightAttackBonus && Healing == AppliedModifiers.HealingBonus;
```

仅替换为：

```cpp
    const float Recovery = Active->Spec.GetSetByCallerMagnitude(
        ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, false, Missing);
    return FMath::IsFinite(Health) && FMath::IsFinite(Attack) &&
           FMath::IsFinite(Healing) && FMath::IsFinite(Recovery) &&
           Health == AppliedModifiers.MaxHealthAdditive &&
           Attack == AppliedModifiers.LightAttackBonus && Healing == AppliedModifiers.HealingBonus &&
           Recovery == AppliedModifiers.StaminaRecoveryBonus;
```

**修改3：零幅度Spec补第四项。**

查找以下唯一片段：

```cpp
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 0.0f);
```

仅替换为：

```cpp
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 0.0f);
    Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 0.0f);
```

**修改4：输入数值有限性检查完整。**

查找以下唯一片段：

```cpp
        !FMath::IsFinite(Modifiers.LightAttackBonus) || !FMath::IsFinite(Modifiers.HealingBonus))
```

仅替换为：

```cpp
        !FMath::IsFinite(Modifiers.LightAttackBonus) || !FMath::IsFinite(Modifiers.HealingBonus) ||
        !FMath::IsFinite(Modifiers.StaminaRecoveryBonus))
```

**修改5：排除自有旧贡献后检查恢复加成范围。**

查找以下唯一片段：

```cpp
    const double Limit = std::numeric_limits<float>::max();
    return FMath::IsFinite(NewMax) && NewMax >= 1.0 && NewMax <= Limit &&
           FMath::IsFinite(NewBonus) && FMath::Abs(NewBonus) <= Limit &&
           FMath::IsFinite(NewHealing) && FMath::Abs(NewHealing) <= Limit;
```

仅替换为：

```cpp
    const double NewRecovery = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute())) -
        AppliedModifiers.StaminaRecoveryBonus + Modifiers.StaminaRecoveryBonus;
    const double Limit = std::numeric_limits<float>::max();
    return FMath::IsFinite(NewMax) && NewMax >= 1.0 && NewMax <= Limit &&
           FMath::IsFinite(NewBonus) && FMath::Abs(NewBonus) <= Limit &&
           FMath::IsFinite(NewHealing) && FMath::Abs(NewHealing) <= Limit &&
           FMath::IsFinite(NewRecovery) && FMath::Abs(NewRecovery) <= Limit;
```

**修改6：仅恢复加成变化也属于更新。**

查找以下唯一片段：

```cpp
        Modifiers.HealingBonus == AppliedModifiers.HealingBonus)
```

仅替换为：

```cpp
        Modifiers.HealingBonus == AppliedModifiers.HealingBonus &&
        Modifiers.StaminaRecoveryBonus == AppliedModifiers.StaminaRecoveryBonus)
```

**修改7：写入第四个幅度。**

查找以下唯一片段：

```cpp
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentHealingBonus, Modifiers.HealingBonus);
```

仅替换为：

```cpp
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentHealingBonus, Modifiers.HealingBonus);
    Magnitudes.Add(ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus,
                   Modifiers.StaminaRecoveryBonus);
```

**修改8：更新批处理说明。**

查找以下唯一片段：

```cpp
        // 三个聚合器更新后才派发脏通知；这不是任意GAS / 库存事务的原子锁。
```

仅替换为：

```cpp
        // 四个聚合器更新后才派发脏通知；这不是任意GAS / 库存事务的原子锁。
```

**修改9：最终治疗预检通过后继续检查实际恢复组件。**

查找以下唯一片段：

```cpp
    return IsValid(Heal) ? Heal->TryCalculateHealAmount(static_cast<float>(HealingBonus), Amount)
                         : Modifiers.HealingBonus == 0.0f;
```

仅替换为：

```cpp
    if (IsValid(Heal) ? !Heal->TryCalculateHealAmount(static_cast<float>(HealingBonus), Amount)
                      : Modifiers.HealingBonus != 0.0f)
    {
        return false;
    }
    const double RecoveryBonus = static_cast<double>(ASC->GetNumericAttribute(
        UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute())) -
        AppliedModifiers.StaminaRecoveryBonus + Modifiers.StaminaRecoveryBonus;
    if (!FMath::IsFinite(RecoveryBonus) || FMath::Abs(RecoveryBonus) > Limit)
    {
        return false;
    }
    const UETStaminaRecoveryComponent* Recovery =
        Body->FindComponentByClass<UETStaminaRecoveryComponent>();
    float Rate = 0.0f;
    return IsValid(Recovery)
        ? Recovery->TryCalculateRecoveryRate(static_cast<float>(RecoveryBonus), Rate)
        : Modifiers.StaminaRecoveryBonus == 0.0f;
```

**修改10：低层幅度更新完成后复核恢复。**

查找以下唯一片段：

```cpp
    OnEquipmentChanged.Broadcast(this);
    // Applied表示
```

仅替换为：

```cpp
    PublishEquipmentChanged();
    // Applied表示
```

**修改11：释放身份后通过同一发布入口。**

查找以下唯一片段：

```cpp
    if (bHadIdentity)
    {
        OnEquipmentChanged.Broadcast(this);
    }
```

仅替换为：

```cpp
    if (bHadIdentity)
    {
        PublishEquipmentChanged();
    }
```

**修改12：身体恢复完成后发布。**

查找以下唯一片段：

```cpp
    OnEquipmentChanged.Broadcast(this);
}

bool UETRelicEquipmentComponent::AreFinalAbilityValuesValid
```

仅替换为：

```cpp
    PublishEquipmentChanged();
}

bool UETRelicEquipmentComponent::AreFinalAbilityValuesValid
```

**修改13：正式事务的组件写锁退出后发布。**

查找以下唯一片段：

```cpp
            OnEquipmentChanged.Broadcast(this);
        });
```

仅替换为：

```cpp
            PublishEquipmentChanged();
        });
```

**修改14：集中复核放在广播前，不在恢复组件反向引用库存。**

查找以下唯一片段：

```cpp
bool UETRelicEquipmentComponent::GetAppliedModifiers(FETRelicModifierTotals& OutModifiers) const
```

仅替换为：

```cpp
void UETRelicEquipmentComponent::PublishEquipmentChanged()
{
    // 属性回调可能在bChanging期间暂时停表；完整写入后重查资格，不立即补资源。
    if (!bEndingPlay)
    {
        if (AEmberTrialCharacter* Body = BodyOwner.Get())
        {
            if (UETStaminaRecoveryComponent* Recovery =
                    Body->FindComponentByClass<UETStaminaRecoveryComponent>())
            {
                Recovery->RefreshRecoveryState();
            }
        }
    }
    OnEquipmentChanged.Broadcast(this);
}

bool UETRelicEquipmentComponent::GetAppliedModifiers(FETRelicModifierTotals& OutModifiers) const
```

**核对：** .cpp中`PublishEquipmentChanged`是**1个定义 + 4处调用**；直接`OnEquipmentChanged.Broadcast(this)`只保留在新函数内1处。四个原发布点分别是低层更新、身份释放、身体恢复、正式事务。按上面顺序完成，避免把新函数中的广播又替换成递归调用。

## 7. 库存提交重验

### 7.1 ETRelicInventorySubsystem.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp`。

**修改1：过期方案重验含第四字段。**

查找以下唯一片段：

```cpp
               X.LightAttackBonus == Y.LightAttackBonus && X.HealingBonus == Y.HealingBonus;
```

仅替换为：

```cpp
               X.LightAttackBonus == Y.LightAttackBonus && X.HealingBonus == Y.HealingBonus &&
               X.StaminaRecoveryBonus == Y.StaminaRecoveryBonus;
```

## 8. 扩展六份既有专项

### 8.1 ETRelicLoadoutTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp`。

**修改1：追加恢复加成汇总与溢出样本，保留原测试。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Healing overflow clears output"), Totals.HealingBonus, 0.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Healing overflow clears output"), Totals.HealingBonus, 0.0f);

    Life->Modifiers[0].Kind = EETRelicModifierKind::StaminaRecoveryBonus;
    Edge->Modifiers[0].Kind = EETRelicModifierKind::StaminaRecoveryBonus;
    Life->Modifiers[0].AdditiveValue = 3.0f;
    Edge->Modifiers[0].AdditiveValue = 2.0f;
    TestTrue(TEXT("Recovery kind validated and summed"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::Success);
    TestEqual(TEXT("Recovery contributions sum five"), Totals.StaminaRecoveryBonus, 5.0f);
    TestTrue(TEXT("Recovery replacement plan accepted"),
             Build(Empty, EETRelicSlot::First, LifeId) == EResult::Success);
    TestEqual(TEXT("Plan carries recovery three"), Plan.AfterModifiers.StaminaRecoveryBonus, 3.0f);
    TestEqual(TEXT("Recovery leaves healing zero"), Plan.AfterModifiers.HealingBonus, 0.0f);
    Life->Modifiers[0].AdditiveValue = -4.0f;
    TestTrue(TEXT("Signed recovery contribution supported"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::Success);
    TestEqual(TEXT("Signed recovery sum"), Totals.StaminaRecoveryBonus, -2.0f);
    Life->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    Edge->Modifiers[0].AdditiveValue = std::numeric_limits<float>::max();
    TestTrue(TEXT("Recovery sum overflow rejected"),
             ETRelicLoadoutRules::Evaluate(Both, Owned, Catalog, Totals) == EResult::NumericOverflow);
    TestEqual(TEXT("Recovery overflow clears output"), Totals.StaminaRecoveryBonus, 0.0f);
```

### 8.2 ETRelicEquipmentEffectsTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp`。

**修改1：效果夹具显式支持第四参数，默认零。**

查找以下唯一片段：

```cpp
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive,
                       const float HealingAdditive = 0.0f)
```

仅替换为：

```cpp
    const FGameplayAttribute RecoveryBonus = UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute();
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive,
                       const float HealingAdditive = 0.0f, const float RecoveryAdditive = 0.0f)
```

**修改2：每个Spec补第四项。**

查找以下唯一片段：

```cpp
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, HealingAdditive);
```

仅替换为：

```cpp
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, HealingAdditive);
        Spec.Data->SetSetByCallerMagnitude(
            ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, RecoveryAdditive);
```

**修改3：首份效果带恢复加成5。**

查找以下唯一片段：

```cpp
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f, 10.0f);
```

仅替换为：

```cpp
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f, 10.0f, 5.0f);
```

**修改4：校验Current与Base隔离。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Healing Base remains zero"), ASC->GetNumericAttributeBase(Healing), 0.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Healing Base remains zero"), ASC->GetNumericAttributeBase(Healing), 0.0f);
    TestEqual(TEXT("Recovery Current five"), ASC->GetNumericAttribute(RecoveryBonus), 5.0f);
    TestEqual(TEXT("Recovery Base remains zero"), ASC->GetNumericAttributeBase(RecoveryBonus), 0.0f);
```

**修改5：同句柄替换第四幅度。**

查找以下唯一片段：

```cpp
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 7.0f);
```

仅替换为：

```cpp
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 7.0f);
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 8.0f);
```

**修改6：验证第四幅度更新。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Same handle updates healing"), ASC->GetNumericAttribute(Healing), 7.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Same handle updates healing"), ASC->GetNumericAttribute(Healing), 7.0f);
    TestEqual(TEXT("Same handle updates recovery"), ASC->GetNumericAttribute(RecoveryBonus), 8.0f);
```

**修改7：外部GE带独立恢复贡献2。**

查找以下唯一片段：

```cpp
    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f, 4.0f);
```

仅替换为：

```cpp
    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f, 4.0f, 2.0f);
```

**修改8：精确删除不清外部恢复贡献。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Other healing survives exact removal"), ASC->GetNumericAttribute(Healing), 4.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Other healing survives exact removal"), ASC->GetNumericAttribute(Healing), 4.0f);
    TestEqual(TEXT("Other recovery survives exact removal"), ASC->GetNumericAttribute(RecoveryBonus), 2.0f);
```

**修改9：全部移除后归零且其他身体隔离。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Removed healing returns zero"), ASC->GetNumericAttribute(Healing), 0.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Removed healing returns zero"), ASC->GetNumericAttribute(Healing), 0.0f);
    TestEqual(TEXT("Removed recovery returns zero"), ASC->GetNumericAttribute(RecoveryBonus), 0.0f);
    TestEqual(TEXT("Other body recovery isolated"), OtherASC->GetNumericAttribute(RecoveryBonus), 0.0f);
```

### 8.3 ETRelicEquipmentOwnershipTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentOwnershipTests.cpp`。

**修改1：持有恢复属性身份。**

查找以下唯一片段：

```cpp
    const FGameplayAttribute Healing = UETCombatAttributeSet::GetHealingBonusAttribute();
```

仅替换为：

```cpp
    const FGameplayAttribute Healing = UETCombatAttributeSet::GetHealingBonusAttribute();
    const FGameplayAttribute RecoveryBonus = UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute();
```

**修改2：仅第四字段变化及篡改检测。**

查找以下唯一片段：

```cpp
    TestTrue(TEXT("Exact spec restored"), Equipment->GetAppliedModifiers(Snapshot));
```

仅替换为：

```cpp
    TestTrue(TEXT("Exact spec restored"), Equipment->GetAppliedModifiers(Snapshot));
    TestTrue(TEXT("Recovery-only change applies"),
             Equipment->TrySetModifiers({40.0f, 7.0f, 10.0f, 5.0f}) == EWrite::Applied);
    TestEqual(TEXT("Owned recovery five"), ASC->GetNumericAttribute(RecoveryBonus), 5.0f);
    TestTrue(TEXT("Recovery update keeps handle"), Equipment->OwnedEffect == InitialHandle);
    TestTrue(TEXT("Same four totals unchanged"),
             Equipment->TrySetModifiers({40.0f, 7.0f, 10.0f, 5.0f}) == EWrite::Unchanged);
    Tamper.Reset();
    Tamper.Add(ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 99.0f);
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(InitialHandle, Tamper);
    TestFalse(TEXT("Fourth magnitude tampering detected"), Equipment->GetAppliedModifiers(Snapshot));
    Tamper[ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus] = 5.0f;
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(InitialHandle, Tamper);
    TestTrue(TEXT("Fourth magnitude restored"), Equipment->GetAppliedModifiers(Snapshot));
```

**修改3：外部Spec第四幅度为2。**

查找以下唯一片段：

```cpp
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 3.0f);
```

仅替换为：

```cpp
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentHealingBonus, 3.0f);
    ForeignSpec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 2.0f);
```

**修改4：失活只移除自有恢复贡献。**

查找以下唯一片段：

```cpp
    TestEqual(TEXT("Foreign healing survives deactivation"), ASC->GetNumericAttribute(Healing), 3.0f);
```

仅替换为：

```cpp
    TestEqual(TEXT("Foreign healing survives deactivation"), ASC->GetNumericAttribute(Healing), 3.0f);
    TestEqual(TEXT("Foreign recovery survives deactivation"), ASC->GetNumericAttribute(RecoveryBonus), 2.0f);
```

### 8.4 ETLightComboFlowTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp`。

**修改1：旧连段装备Spec补第四零幅度。**

查找以下唯一片段：

```cpp
            ETGameplayTags::SetByCaller_EquipmentHealingBonus, 0.0f);
```

仅替换为：

```cpp
            ETGameplayTags::SetByCaller_EquipmentHealingBonus, 0.0f);
        EquipmentSpec.Data->SetSetByCallerMagnitude(
            ETGameplayTags::SetByCaller_EquipmentStaminaRecoveryBonus, 0.0f);
```

### 8.5 ETRelicEquipmentTransactionTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp`。

**修改1：引用实际速率消费者和体力属性。**

查找以下唯一片段：

```cpp
#include "Abilities/ETGA_Heal.h"
```

仅替换为：

```cpp
#include "Abilities/ETGA_Heal.h"
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Attributes/ETStaminaAttributeSet.h"
```

**修改2：追加恢复事务与写入后醒表回归。**

查找以下唯一片段：

```cpp
    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
```

仅替换为：

```cpp
    // ---- 恢复参数：正式预检、过期拒绝与组件写锁退出后的Timer复核 ----
    UETStaminaRecoveryComponent* Recovery =
        NewRunBody->FindComponentByClass<UETStaminaRecoveryComponent>();
    if (!TestNotNull(TEXT("Recovery consumer"), Recovery))
    {
        return false;
    }
    const FGameplayAttribute Stamina = UETStaminaAttributeSet::GetStaminaAttribute();
    const FGameplayAttribute RecoveryBonus = UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute();
    NewASC->SetNumericAttributeBase(Stamina, 50.0f);
    TestTrue(TEXT("Deficit starts baseline recovery timer"), Recovery->IsRecoveryScheduled());
    UETRelicDefinition* RecoveryFixture = NewObject<UETRelicDefinition>(Inventory);
    const FName RecoveryId(TEXT("fixture_recovery"));
    RecoveryFixture->RelicId = RecoveryId;
    RecoveryFixture->DisplayName = FText::FromString(TEXT("Recovery fixture"));
    RecoveryFixture->Description = FText::FromString(TEXT("Transient recovery transaction fixture"));
    FETRelicModifier RecoveryModifier;
    RecoveryModifier.Kind = EETRelicModifierKind::StaminaRecoveryBonus;
    RecoveryModifier.AdditiveValue = 1000.0f;
    RecoveryFixture->Modifiers = {RecoveryModifier};
    Inventory->Definitions.Add(RecoveryId, RecoveryFixture);
    Inventory->OwnedIds.Add(RecoveryId);
    Inventory->OwnedSet.Add(RecoveryId);
    Inventory->AdvanceRevision();
    Inventory->PublishSnapshot();
    const uint64 RecoveryRevision = Inventory->GetSnapshot().Revision;
    auto EquipRecovery = [&]()
    {
        return NewEquipment->TryEquip(
            EETRelicSlot::First, RecoveryId, Inventory->GetSnapshot().Revision).Result;
    };
    TestTrue(TEXT("Excess recovery preview rejected"), NewEquipment->TryBuildPreview(
        EETRelicSlot::First, RecoveryId, RecoveryRevision, Preview).Result == EResult::RuleRejected);
    TestTrue(TEXT("Excess recovery commit rejected"), EquipRecovery() == EResult::RuleRejected);
    RecoveryFixture->Modifiers[0].AdditiveValue = -20.0f;
    TestTrue(TEXT("Zero effective recovery rejected"), EquipRecovery() == EResult::RuleRejected);
    TestTrue(TEXT("Invalid recovery keeps revision"), Inventory->GetSnapshot().Revision == RecoveryRevision);
    TestEqual(TEXT("Invalid recovery keeps stamina"), NewASC->GetNumericAttribute(Stamina), 50.0f);

    RecoveryFixture->Modifiers[0].AdditiveValue = 5.0f;
    FETRelicInventoryPlan RecoveryPlan;
    TestTrue(TEXT("Recovery plan created"), Inventory->TryBuildPlan(
        EETRelicSlot::First, RecoveryId, 100.0f, 25.0f, RecoveryPlan).Result == EResult::Success);
    RecoveryFixture->Modifiers[0].AdditiveValue = 6.0f;
    TestTrue(TEXT("Recovery parameter invalidates old plan"),
             Inventory->TryCommitPlan(RecoveryPlan).Result == EResult::StalePlan);
    RecoveryFixture->Modifiers[0].AdditiveValue = 5.0f;
    TestTrue(TEXT("Recovery preview has no side effect"), NewEquipment->TryBuildPreview(
        EETRelicSlot::First, RecoveryId, RecoveryRevision, Preview).Result == EResult::Success);
    TestEqual(TEXT("Preview carries recovery five"), Preview.AfterModifiers.StaminaRecoveryBonus, 5.0f);
    TestEqual(TEXT("Preview leaves actual recovery zero"), NewASC->GetNumericAttribute(RecoveryBonus), 0.0f);
    const FActiveGameplayEffectHandle RecoveryHandle = NewEquipment->OwnedEffect;
    TestTrue(TEXT("Recovery equips"), EquipRecovery() == EResult::Success);
    TestTrue(TEXT("Recovery update keeps owned handle"), NewEquipment->OwnedEffect == RecoveryHandle);
    TestEqual(TEXT("Equipment Current recovery five"), NewASC->GetNumericAttribute(RecoveryBonus), 5.0f);
    TestEqual(TEXT("Equipment Base recovery zero"), NewASC->GetNumericAttributeBase(RecoveryBonus), 0.0f);
    float EffectiveRate = 0.0f;
    TestTrue(TEXT("Actual consumer accepts equipment bonus"),
             Recovery->TryCalculateRecoveryRate(NewASC->GetNumericAttribute(RecoveryBonus), EffectiveRate));
    TestEqual(TEXT("Actual configured effective rate25"), EffectiveRate, 25.0f);
    TestEqual(TEXT("Equip does not immediately restore stamina"), NewASC->GetNumericAttribute(Stamina), 50.0f);
    TestTrue(TEXT("Commit completion wakes timer after transient Busy"), Recovery->IsRecoveryScheduled());
    NewEquipment->Deactivate();
    TestFalse(TEXT("Inactive equipment stops controlled-body recovery"), Recovery->IsRecoveryScheduled());
    NewEquipment->Activate(false);
    TestEqual(TEXT("Reactivation restores recovery bonus"), NewASC->GetNumericAttribute(RecoveryBonus), 5.0f);
    TestTrue(TEXT("Restored ready equipment wakes recovery"), Recovery->IsRecoveryScheduled());
    TestEqual(TEXT("Reactivation does not refill stamina"), NewASC->GetNumericAttribute(Stamina), 50.0f);
    TestTrue(TEXT("Recovery unequip"), NewEquipment->TryEquip(
        EETRelicSlot::First, NAME_None, Inventory->GetSnapshot().Revision).Result == EResult::Success);
    TestEqual(TEXT("Unequip clears recovery contribution"), NewASC->GetNumericAttribute(RecoveryBonus), 0.0f);
    TestTrue(TEXT("Unequip completion preserves recovery eligibility"), Recovery->IsRecoveryScheduled());
    TestEqual(TEXT("Unequip does not refill stamina"), NewASC->GetNumericAttribute(Stamina), 50.0f);

    // ---- 外部移除不能伪造成功；结束后解绑与清句柄仍精确 ----
```

### 8.6 ETStaminaRecoveryTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETStaminaRecoveryTests.cpp`。

**修改1：相同值通知不得重置未结算时间。**

查找以下唯一片段：

```cpp
    ASC->SetNumericAttributeBase(RecoveryBonus, std::numeric_limits<float>::quiet_NaN());
```

仅替换为：

```cpp
    // 等到真实采样间隔中的未结算片段，验证相同值通知不能丢弃它。
    for (int32 Step = 0; Step < 3 &&
         World->GetTimeSeconds() <= Recovery->LastSampleTime + 0.001; ++Step)
    {
        if (!Advance(1))
        {
            return false;
        }
    }
    if (!TestTrue(TEXT("Unpaid sampling interval exists"),
                  World->GetTimeSeconds() > Recovery->LastSampleTime + 0.001))
    {
        return false;
    }
    const double BeforeNoOpSample = Recovery->LastSampleTime;
    FOnAttributeChangeData SameRate;
    SameRate.Attribute = RecoveryBonus;
    SameRate.OldValue = 10.0f;
    SameRate.NewValue = 10.0f;
    Recovery->HandleAttributeChanged(SameRate, TWeakObjectPtr<UAbilitySystemComponent>(ASC),
                                     Recovery->ObservationSerial);
    TestEqual(TEXT("Same rate notification preserves sample"), Recovery->LastSampleTime, BeforeNoOpSample);
    ASC->SetNumericAttributeBase(RecoveryBonus, std::numeric_limits<float>::quiet_NaN());
```

事务夹具只向测试世界的目录加入fixture_recovery，不创建正式资产 / 配置。它在受本地Controller控制、体力50的真实身体上确认“写入后仍有恢复Timer”，能发现属性回调在临时Busy停表后的遗漏；恢复 / 卸装也不立即补体力。相同值用例等待真实未结算区间再负向注入通知，原数值与生命周期断言保留。

## 9. 正常Build与索引

19份文件全部保存，UE已关闭。在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本正常Editor Build并刷新三模块索引；CLion提示时Reload CMake。仍Core89 / UI41 / Editor3共133cpp，无新文件，不清缓存 / 重建工程，也不重复单独Editor Build。

同一窗口进行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

成功再打开工程。不改现有Data Asset / UI / 组件配置，不全Content重存，不运行Cook。

## 10. 六个相关专项（各一次）

非Play，在Output Log底部逐项执行；上一项Success再输入下一项：

```text
Automation RunTests EmberTrial.M3.Inventory.LoadoutRules
Automation RunTests EmberTrial.M3.Inventory.EquipmentEffects
Automation RunTests EmberTrial.M3.Inventory.EquipmentOwnership
Automation RunTests EmberTrial.M3.Inventory.EquipmentTransactions
Automation RunTests EmberTrial.M2.Resources.StaminaRecovery
Automation RunTests EmberTrial.M2.Combo.Flow
```

| 专项 | 本批必要覆盖 |
| --- | --- |
| LoadoutRules | 第四Kind验证 / 双槽加法 / 有符号输入 / 溢出清输出 |
| EquipmentEffects | 四Modifier真实GE、Base / Current、同句柄更新与精确移除 |
| EquipmentOwnership | 仅恢复字段改变 / 第四项篡改 / 外部贡献保留 |
| EquipmentTransactions | 非法最终速率 / 过期方案、真实组件配置25、写入 / 重激活 / 卸装后醒表、不立即恢复 |
| StaminaRecovery | 同值事件保留采样、原速率 / 延迟 / 暂停 / 阻断 / 订阅清理 |
| Combo.Flow | 旧连段装备Spec补第四零幅度后，真实三段和快照保持 |

所有测试仍为既有入口，共42项。当前不跑全组、UI整套 / AI矩阵，也不要求你在实机抢某个恢复时点。若出现缺SetByCaller Warning，发第一条文字和测试名，不忽略；成功长日志由我读取。

## 11. 默认路径短查（约3分钟）

1. 正式地图新Play，四件仍保留、资源默认生命 / 体力100与治疗3，StaminaRecoveryBonus0。没有第五件或新比较行符合预期。
2. 单按2，实际扣25后按原规则恢复到满；P打开 / 继续、移动转向，左键或Q任选一项最短观察。
3. Stop → 新Play确认新100 / 加成0与原四件，再Stop。源码保存，资产 / Config保持，无临时蓝图、帧率或导航设置需要恢复。

实际非零装备加成与精确醒表由本批专项负责；第五件资产 / 界面真实演示在03检查，不搭重复临时按键。

## 12. 完成反馈

```text
M3-06B3-02 完成
19份既有Core源码；UE关闭后Editor Build / 三索引 / Game Build：
LoadoutRules / EquipmentEffects / EquipmentOwnership / EquipmentTransactions：
StaminaRecovery / Combo.Flow各一次：
正式四件保持、2扣25后恢复满值 / P与最短旧输入：
Stop新100与加成0；源码保存，资产配置保持：
失败或不理解处（无填无）：
完成时分 / 手动代码与理解 / 检查记录 / 构建等待：
```

学习重点：属性变化发生时，业务事务可能尚在写入；完成发布需要显式复核计时资格。资源组件负责恢复规则，装备适配层负责调用完成复核，保持依赖单向；没有新增Tick / 常驻轮询或在属性回调中补资源。

## 13. Commit / Push（固定放在最后）

B3-01收尾当前只提交以下**11份文档**。本地13dd9da已包含六份Core实现，来源94 / 资产 / Config保持；B3-02的19份Core仅候选，未写入正式源码。

```text
Docs/Reports/M3_06B3_01_Verification.md
Docs/M3_06B3_01_StaminaRecoveryBonusFoundation.md
Docs/M3_06B3_02_StaminaRecoveryEquipmentPipeline.md
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文：`docs: verify stamina recovery bonus and plan equipment integration`
- 专业中文：`docs: 完成体力恢复加成验收并规划装备接入`

Artifacts候选 / 编译预检 / 原日志 / JSON、包与个人ini保持本地。Stage后继续编辑需重新Stage → Review → Commit / Push，由你操作；助手未操作索引、提交或远端。

**你实施并验收本小节后，再提交以下19份既有Core源码的真实改动：**

```text
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicTypes.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicDefinition.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicLoadout.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicLoadout.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h
EmberTrial/Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp
EmberTrial/Source/EmberTrial/Public/Resources/ETStaminaRecoveryComponent.h
EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp
EmberTrial/Source/EmberTrial/Public/Inventory/ETRelicEquipmentComponent.h
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicEquipmentComponent.cpp
EmberTrial/Source/EmberTrial/Private/Inventory/ETRelicInventorySubsystem.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicLoadoutTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentOwnershipTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETRelicEquipmentTransactionTests.cpp
EmberTrial/Source/EmberTrial/Tests/ETStaminaRecoveryTests.cpp
```

- 英文：`feat: integrate stamina recovery bonus into relic equipment`
- 专业中文：`feat: 将体力恢复加成接入遗物装备管线`

届时新的报告 / 状态改动一并Review；本批没有Config、资产或第五件需要Push，不提交Artifacts候选。
