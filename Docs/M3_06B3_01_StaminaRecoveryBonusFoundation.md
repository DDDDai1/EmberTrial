# M3-06B3-01：体力恢复速率加成基础

2026-11-07下发，UE **5.5.4**。[B2-03回生遗物已验收](Reports/M3_06B2_03_Verification.md)，治疗三小节已通过。本章由你手动修改**六份既有Core源码**；只有Artifacts候选，正式文件尚未修改。

## 1. 做什么与结算规则

在原CombatAttributeSet加入`StaminaRecoveryBonus`，新身体初始0，单位为**体力点 / 游戏秒**。现有StaminaRecovery组件读取聚合Current，计算`基础RecoveryRate + 加成`，沿用已有按需Timer和Instant GE恢复体力。默认基础20、消费后延迟.75秒、采样间隔.10秒保持。

| 条件 | 行为 |
| --- | --- |
| 基础20 / 加成0 | 每秒20，当前四件遗物的默认路径保持 |
| 测试GE加成10 | 每秒30，属性Base仍0，移除GE后回20 |
| 有限负加成 | 可以保存；最终速率必须大于0且≤1000 |
| 最终非法 | 停止恢复Timer；后续合法属性事件重新唤醒，不补非法期间时间 |
| 满值 / 忙碌 / 死亡 | 沿用原停表规则，不因加成新增常驻Timer |
| 暂停 | 使用世界时间，冻结期间不累计恢复 |
| 结束 / 重启观察 | 新速率委托与原资源 / 动作Tag配对解绑，旧代次不能覆盖新采样 |

**速率切换的明确规则：**以属性变化时的世界时间作为新积分起点；变化前尚未结算的小段按规则舍弃，恢复只在原Timer回调生效。最近一次实际消费的RecoveryNotBefore保持，变化本身不立即加体力。这样在装备属性回调中只调整计时资格，实际资源修改仍集中在原恢复路径。

本章只做真实消费者，不加入第五件、第四个装备SetByCaller或新UI行；后续B3批次再接装备 / 数据 / 比较。测试+10用于清楚区分20与30，不代表已经发放一件新遗物。

预计手动60–90分钟（代码与理解约50–75、检查记录约10–15），正常构建等待另记。无新文件 / 资产 / Config，规模保持233 Source / 133cpp / 42测试、Content311 / 来源94；无蓝图接线或Designer工作。

## 2. 开始前

1. Stop PIE → Save All → 关闭UE，CLion可保留。新增反射属性需要正常UBT / UHT构建，不用Live Coding完成头文件更新。
2. 下列六个文件全部在EmberTrial核心模块。按每项“查找唯一片段 → 替换为”操作，其余代码保留；不要重建属性集或恢复组件。
3. 下发前四份候选cpp仅编译预检均ExitCode0，复用既有反射声明；这不包含新UHT / 链接 / 运行。第7节双Build和第8节专项仍需你完成。

## 3. 原CombatAttributeSet增加恢复加成

### 3.1 ETCombatAttributeSet.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h`。

**修改1：增加每秒恢复加成查询，保留原两属性。**

查找以下唯一片段：

```cpp
    GAMEPLAYATTRIBUTE_VALUE_GETTER(HealingBonus)

    virtual void
```

仅替换为：

```cpp
    GAMEPLAYATTRIBUTE_VALUE_GETTER(HealingBonus)

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, StaminaRecoveryBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(StaminaRecoveryBonus)

    virtual void
```

**修改2：在原属性集尾部增加零默认数据。**

查找以下唯一片段：

```cpp
    GAMEPLAYATTRIBUTE_VALUE_INITTER(HealingBonus)
};
```

仅替换为：

```cpp
    GAMEPLAYATTRIBUTE_VALUE_INITTER(HealingBonus)

    // 体力每秒恢复的固定加成，不改变消费、上限或消费后延迟。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData StaminaRecoveryBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(StaminaRecoveryBonus)
};
```

### 3.2 ETCombatAttributeSet.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp`。

**修改1：构造默认零值。**

查找以下唯一片段：

```cpp
    InitHealingBonus(0.0f);
```

仅替换为：

```cpp
    InitHealingBonus(0.0f);
    InitStaminaRecoveryBonus(0.0f);
```

**修改2：非有限Base回退保持Base语义。**

查找以下唯一片段：

```cpp
            NewValue, HealingBonus.GetBaseValue());
    }
}
```

仅替换为：

```cpp
            NewValue, HealingBonus.GetBaseValue());
    }
    else if (Attribute == GetStaminaRecoveryBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, StaminaRecoveryBonus.GetBaseValue());
    }
}
```

**修改3：非有限Current回退保持Current语义。**

查找以下唯一片段：

```cpp
    // 有限负值合法；最终伤害 / 治疗是否可用，由对应能力预检。
```

仅替换为：

```cpp
    else if (Attribute == GetStaminaRecoveryBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, StaminaRecoveryBonus.GetCurrentValue());
    }
    // 有限负值合法；最终伤害 / 治疗 / 恢复速率由各自消费者预检。
```

## 4. 新身体Ready前一次初始化

### 4.1 EmberTrialCharacter.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

**修改1：新身体仅一次初始化，在Ready前归零。**

查找以下唯一片段：

```cpp
        UETCombatAttributeSet::GetHealingBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;
```

仅替换为：

```cpp
        UETCombatAttributeSet::GetHealingBonusAttribute(), 0.0f);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;
```

**修改2：扩展同一Combat初始化日志。**

查找以下唯一片段：

```cpp
           TEXT("[M3-02B] Combat initialized Character=%s LightAttackBonus=%.1f HealingBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus(),
           CombatAttributes->GetHealingBonus());
```

仅替换为：

```cpp
           TEXT("[M3-02B] Combat initialized Character=%s LightAttackBonus=%.1f HealingBonus=%.1f "
                "StaminaRecoveryBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus(),
           CombatAttributes->GetHealingBonus(), CombatAttributes->GetStaminaRecoveryBonus());
```

## 5. 原恢复组件消费聚合速率

### 5.1 ETStaminaRecoveryComponent.h

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Resources/ETStaminaRecoveryComponent.h`。

**修改1：提供纯数值入口，供正式恢复与后续装备预检共用。**

查找以下唯一片段：

```cpp
    bool IsRecoveryScheduled() const;
```

仅替换为：

```cpp
    bool IsRecoveryScheduled() const;

    // 只计算基础速率 + 加成；不要求当前能恢复、不读世界或修改资源。
    bool TryCalculateRecoveryRate(float Bonus, float& OutRate) const;
```

**修改2：增加当前聚合速率读取声明。**

查找以下唯一片段：

```cpp
    void HandleRecoveryTimer(uint64 Serial);
```

仅替换为：

```cpp
    void HandleRecoveryTimer(uint64 Serial);
    bool ReadRecoveryRate(UAbilitySystemComponent* ASC, float& OutRate) const;
```

**修改3：独立持有速率属性委托，不改动作Tag数组。**

查找以下唯一片段：

```cpp
    FDelegateHandle MaxStaminaHandle;
```

仅替换为：

```cpp
    FDelegateHandle MaxStaminaHandle;
    FDelegateHandle RecoveryBonusHandle;
```

### 5.2 ETStaminaRecoveryComponent.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp`。

**修改1：显式引用Combat属性。**

查找以下唯一片段：

```cpp
#include "Attributes/ETStaminaAttributeSet.h"
```

仅替换为：

```cpp
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETCombatAttributeSet.h"
```

**修改2：增加统一纯计算和ASC读取实现。**

查找以下唯一片段：

```cpp
void UETStaminaRecoveryComponent::ClearRecoveryTimer(const TCHAR* Reason)
```

仅替换为：

```cpp
// double中间值避免float相加溢出；有效速率必须大于0且不超过现有1000上限。
bool UETStaminaRecoveryComponent::TryCalculateRecoveryRate(const float Bonus, float& OutRate) const
{
    OutRate = 0.0f;
    const double Rate = static_cast<double>(RecoveryRate) + static_cast<double>(Bonus);
    if (!FMath::IsFinite(RecoveryRate) || RecoveryRate <= 0.0f || RecoveryRate > 1000.0f ||
        !FMath::IsFinite(Bonus) || !FMath::IsFinite(Rate) || Rate <= 0.0 || Rate > 1000.0)
    {
        return false;
    }
    OutRate = static_cast<float>(Rate);
    return OutRate > 0.0f;
}

bool UETStaminaRecoveryComponent::ReadRecoveryRate(
    UAbilitySystemComponent* ASC, float& OutRate) const
{
    OutRate = 0.0f;
    return IsValid(ASC) && ASC->GetSet<UETCombatAttributeSet>() &&
           TryCalculateRecoveryRate(
               ASC->GetNumericAttribute(UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute()),
               OutRate);
}

void UETStaminaRecoveryComponent::ClearRecoveryTimer(const TCHAR* Reason)
```

**修改3：解绑第三个属性订阅，保留原Tag配对清理。**

查找以下唯一片段：

```cpp
            .Remove(MaxStaminaHandle);
        const TConstArrayView<FGameplayTag> Tags
```

仅替换为：

```cpp
            .Remove(MaxStaminaHandle);
        ASC->GetGameplayAttributeValueChangeDelegate(
               UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute())
            .Remove(RecoveryBonusHandle);
        const TConstArrayView<FGameplayTag> Tags
```

**修改4：订阅身份清空时同步Reset新Handle。**

查找以下唯一片段：

```cpp
    MaxStaminaHandle.Reset();
    ActionTagHandles.Reset();
```

仅替换为：

```cpp
    MaxStaminaHandle.Reset();
    RecoveryBonusHandle.Reset();
    ActionTagHandles.Reset();
```

**修改5：订阅同一个属性回调，沿用弱ASC与观察代次。**

查找以下唯一片段：

```cpp
            .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC, ObservationSerial);
    const TConstArrayView<FGameplayTag> Tags
```

仅替换为：

```cpp
            .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC, ObservationSerial);
    RecoveryBonusHandle =
        ASC->GetGameplayAttributeValueChangeDelegate(
               UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute())
            .AddUObject(this, &ThisClass::HandleAttributeChanged, BoundASC, ObservationSerial);
    const TConstArrayView<FGameplayTag> Tags
```

**修改6：速率变化不立即补资源、不重置消费延迟。**

查找以下唯一片段：

```cpp
    if (Change.Attribute == UETStaminaAttributeSet::GetStaminaAttribute() &&
        Change.NewValue < Change.OldValue)
```

仅替换为：

```cpp
    if (Change.Attribute == UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute())
    {
        // 以变化时刻作为新积分起点；未结算的旧小段不补发，也不把旧时间乘新速率。
        // 不改RecoveryNotBefore，保留最近一次实际消费的截止时间。
        LastSampleTime = GetWorld()->GetTimeSeconds();
        float Rate = 0.0f;
        const bool bValidRate = ReadRecoveryRate(SourceASC.Get(), Rate);
        UE_LOG(LogETStaminaRecovery, Display,
               TEXT("[M3-06B3-01] Rate changed Owner=%s Valid=%d Effective=%.1f"),
               *GetNameSafe(GetOwner()), bValidRate ? 1 : 0, Rate);
        SyncRecoveryTimer();
        return;
    }
    if (Change.Attribute == UETStaminaAttributeSet::GetStaminaAttribute() &&
        Change.NewValue < Change.OldValue)
```

**修改7：只有有效速率才保留按需Timer。**

查找以下唯一片段：

```cpp
    if (!IsRecoveryScheduled())
    {
        LastSampleTime = GetWorld()->GetTimeSeconds();
```

仅替换为：

```cpp
    float EffectiveRate = 0.0f;
    if (!ReadRecoveryRate(ASC, EffectiveRate))
    {
        ClearRecoveryTimer(TEXT("InvalidRate"));
        return;
    }
    if (!IsRecoveryScheduled())
    {
        LastSampleTime = GetWorld()->GetTimeSeconds();
```

**修改8：真实恢复回调读取当前有效速率。**

查找以下唯一片段：

```cpp
    const double Now = GetWorld()->GetTimeSeconds();
    // 同时裁掉已结算时间
```

仅替换为：

```cpp
    float EffectiveRate = 0.0f;
    if (!ReadRecoveryRate(ASC, EffectiveRate))
    {
        SyncRecoveryTimer();
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    // 同时裁掉已结算时间
```

**修改9：以double裁到剩余容量后再转float。**

查找以下唯一片段：

```cpp
    const float Amount =
        FMath::Min(MaxStamina - Before, static_cast<float>(EligibleTime * RecoveryRate));
```

仅替换为：

```cpp
    const float Amount = static_cast<float>(FMath::Min(
        static_cast<double>(MaxStamina - Before), EligibleTime * EffectiveRate));
```

**核对：** 新RecoveryBonusHandle单独保存，原ActionTagHandles仍只对应原7个Tag，不把新属性Handle塞进该数组。恢复GE只在HandleRecoveryTimer中Apply；速率事件仅重设采样 / Sync，不修改消费截止时间。

## 6. 扩展既有StaminaRecovery专项

### 6.1 ETStaminaRecoveryTests.cpp

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETStaminaRecoveryTests.cpp`。

**修改1：增加加成 / 真实GE与非法数值依赖。**

查找以下唯一片段：

```cpp
#include "Attributes/ETHealthAttributeSet.h"
```

仅替换为：

```cpp
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "GameplayEffect.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>
```

**修改2：在原默认恢复场景之前扩展有效速率和真实计时。**

查找以下唯一片段：

```cpp
    ApplyDelta(-25.0f);
    Same(TEXT("Real GE consumes 25"), 75.0f);
```

仅替换为：

```cpp
    const FGameplayAttribute RecoveryBonus =
        UETCombatAttributeSet::GetStaminaRecoveryBonusAttribute();
    TestEqual(TEXT("Recovery bonus initial Base zero"), ASC->GetNumericAttributeBase(RecoveryBonus), 0.0f);
    TestEqual(TEXT("Recovery bonus initial Current zero"), ASC->GetNumericAttribute(RecoveryBonus), 0.0f);
    TestTrue(TEXT("Rate observer bound"), ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus)
        .IsBoundToObject(Recovery));
    float Rate = -1.0f;
    TestTrue(TEXT("Base20 plus10 accepted"), Recovery->TryCalculateRecoveryRate(10.0f, Rate));
    TestEqual(TEXT("Additive rate thirty"), Rate, 30.0f);
    TestTrue(TEXT("Signed rate remains valid"), Recovery->TryCalculateRecoveryRate(-5.0f, Rate));
    TestEqual(TEXT("Signed effective fifteen"), Rate, 15.0f);
    TestFalse(TEXT("Nonpositive rate rejected"), Recovery->TryCalculateRecoveryRate(-20.0f, Rate));
    TestEqual(TEXT("Rejected rate cleared"), Rate, 0.0f);
    TestFalse(TEXT("Rate above thousand rejected"), Recovery->TryCalculateRecoveryRate(981.0f, Rate));
    TestFalse(TEXT("NaN rate bonus rejected"), Recovery->TryCalculateRecoveryRate(
        std::numeric_limits<float>::quiet_NaN(), Rate));

    // 暂用零延迟隔离速率积分；通过真实GE / World Timer，不手动调用恢复成功路径。
    Recovery->RecoveryDelay = 0.0f;
    Recovery->RefreshObservation();
    ApplyDelta(-80.0f);
    if (!Advance(4))
    {
        return false;
    }
    // 移除后还要推进世界并复用这份GE，强持有隔离夹具，避免依赖GC时机。
    const TStrongObjectPtr<UGameplayEffect> BonusEffect(NewObject<UGameplayEffect>(GetTransientPackage()));
    BonusEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
    FGameplayModifierInfo& Modifier = BonusEffect->Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = RecoveryBonus;
    Modifier.ModifierOp = EGameplayModOp::AddBase;
    Modifier.ModifierMagnitude = FScalableFloat(10.0f);
    const float BeforeRateChange = Value();
    const double ExistingDeadline = Recovery->RecoveryNotBefore;
    const FTimerHandle ExistingTimer = Recovery->RecoveryTimer;
    const FActiveGameplayEffectHandle BonusHandle =
        ASC->ApplyGameplayEffectToSelf(BonusEffect.Get(), 1.0f, ASC->MakeEffectContext());
    if (!TestTrue(TEXT("Real recovery bonus GE active"), BonusHandle.IsValid()))
    {
        return false;
    }
    Same(TEXT("Rate change does not immediately recover"), BeforeRateChange);
    TestEqual(TEXT("Rate change preserves spend deadline"), Recovery->RecoveryNotBefore, ExistingDeadline);
    TestTrue(TEXT("Valid rate change keeps one timer"), Recovery->RecoveryTimer == ExistingTimer);
    TestEqual(TEXT("GE keeps recovery Base zero"), ASC->GetNumericAttributeBase(RecoveryBonus), 0.0f);
    TestEqual(TEXT("GE Current ten"), ASC->GetNumericAttribute(RecoveryBonus), 10.0f);
    // 采样时刻以真实Timer最后结算时间为准，避免猜测TimerManager在某帧前后派发。
    auto CheckRateWindow = [&](const TCHAR* Label, const float ExpectedRate)
    {
        const float Before = Value();
        const double Start = Recovery->LastSampleTime;
        if (!Advance(6))
        {
            return false;
        }
        const double PaidSeconds = Recovery->LastSampleTime - Start;
        return TestTrue(Label, PaidSeconds > 0.1 &&
            FMath::IsNearlyEqual(static_cast<double>(Value() - Before),
                                PaidSeconds * ExpectedRate, 0.003));
    };
    if (!CheckRateWindow(TEXT("Real timer uses effective thirty"), 30.0f))
    {
        return false;
    }
    ASC->SetNumericAttributeBase(RecoveryBonus, std::numeric_limits<float>::quiet_NaN());
    TestEqual(TEXT("NaN Base falls back to zero"), ASC->GetNumericAttributeBase(RecoveryBonus), 0.0f);
    TestEqual(TEXT("NaN does not duplicate current bonus"), ASC->GetNumericAttribute(RecoveryBonus), 10.0f);
    TestTrue(TEXT("Owned rate GE removed"), ASC->RemoveActiveGameplayEffect(BonusHandle));
    TestEqual(TEXT("Removed bonus returns zero"), ASC->GetNumericAttribute(RecoveryBonus), 0.0f);
    if (!CheckRateWindow(TEXT("Removal restores twenty for later interval"), 20.0f))
    {
        return false;
    }
    ASC->SetNumericAttributeBase(RecoveryBonus, -20.0f);
    const float InvalidRateValue = Value();
    TestFalse(TEXT("Invalid effective rate stops timer"), Recovery->IsRecoveryScheduled());
    if (!Advance(20))
    {
        return false;
    }
    Same(TEXT("Invalid rate cannot recover"), InvalidRateValue);
    ASC->SetNumericAttributeBase(RecoveryBonus, 0.0f);
    TestTrue(TEXT("Valid rate event wakes deficit"), Recovery->IsRecoveryScheduled());
    if (!CheckRateWindow(TEXT("Resume does not repay invalid duration"), 20.0f))
    {
        return false;
    }

    // 加成改变不能绕过正在等待的消费延迟。
    ApplyDelta(1000.0f);
    Recovery->RecoveryDelay = 0.75f;
    Recovery->RefreshObservation();
    ApplyDelta(-25.0f);
    const double DelayedDeadline = Recovery->RecoveryNotBefore;
    if (!Advance(4))
    {
        return false;
    }
    const FActiveGameplayEffectHandle DelayedBonus =
        ASC->ApplyGameplayEffectToSelf(BonusEffect.Get(), 1.0f, ASC->MakeEffectContext());
    if (!TestTrue(TEXT("Bonus during delay active"), DelayedBonus.IsValid()))
    {
        return false;
    }
    TestEqual(TEXT("Bonus during delay preserves original deadline"),
              Recovery->RecoveryNotBefore, DelayedDeadline);
    if (!Advance(8))
    {
        return false;
    }
    Same(TEXT("Bonus does not bypass pending delay"), 75.0f);
    ASC->RemoveActiveGameplayEffect(DelayedBonus);
    ApplyDelta(1000.0f);
    // 恢复原默认20 / .75 / .10与满值前提，下面所有原场景保持。
    ApplyDelta(-25.0f);
    Same(TEXT("Real GE consumes 25"), 75.0f);
```

**修改3：重复观察同样核对速率订阅。**

查找以下唯一片段：

```cpp
    CheckActionTagBindings(true);
    const uint64 OldSerial
```

仅替换为：

```cpp
    CheckActionTagBindings(true);
    TestTrue(TEXT("Rate subscription survives reobserve"),
             ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus).IsBoundToObject(Recovery));
    const uint64 OldSerial
```

**修改4：失活清除新属性的真实委托。**

查找以下唯一片段：

```cpp
    TestTrue(TEXT("Deactivate clears action handles"), Recovery->ActionTagHandles.IsEmpty());
```

仅替换为：

```cpp
    TestTrue(TEXT("Deactivate clears action handles"), Recovery->ActionTagHandles.IsEmpty());
    TestFalse(TEXT("Deactivate clears rate handle"), Recovery->RecoveryBonusHandle.IsValid());
    TestFalse(TEXT("Deactivate removes rate subscription"),
              ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus).IsBoundToObject(Recovery));
```

**修改5：旧代次速率回调不能覆盖新采样起点。**

查找以下唯一片段：

```cpp
    const double NewDeadline = Recovery->RecoveryNotBefore;
    FOnAttributeChangeData StaleChange;
```

仅替换为：

```cpp
    const double NewDeadline = Recovery->RecoveryNotBefore;
    const double NewSample = Recovery->LastSampleTime;
    FOnAttributeChangeData StaleRate;
    StaleRate.Attribute = RecoveryBonus;
    StaleRate.OldValue = 0.0f;
    StaleRate.NewValue = 10.0f;
    Recovery->HandleAttributeChanged(StaleRate, TWeakObjectPtr<UAbilitySystemComponent>(ASC), OldSerial);
    TestEqual(TEXT("Old rate callback cannot reset new sample"), Recovery->LastSampleTime, NewSample);
    TestTrue(TEXT("Reactivated rate observer bound"),
             ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus).IsBoundToObject(Recovery));
    FOnAttributeChangeData StaleChange;
```

**修改6：结束后速率委托同样清理。**

查找以下唯一片段：

```cpp
    AddInfo(TEXT("Stamina recovery: all action blockers and balanced tag subscription cleanup"));
```

仅替换为：

```cpp
    TestFalse(TEXT("EndPlay clears rate handle"), Recovery->RecoveryBonusHandle.IsValid());
    TestFalse(TEXT("EndPlay removes rate subscription"),
              ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus).IsBoundToObject(Recovery));
    AddInfo(TEXT("Stamina recovery: additive rate / GE removal / invalid restart, "
                 "spend delay, all action blockers and balanced subscriptions"));
```

新增速率段使用真实Infinite GE / World Timer并观察实际已结算时间，不猜TimerManager在哪帧派发。夹具暂用零延迟隔离数值后恢复.75与满值，所有原默认延迟 / 暂停 / 七Tag订阅 / 死亡 / 清理断言保持。GE移除后还要跨Tick复用，测试强持有其生命周期；没有新增测试入口或文件。

## 7. 正常Build与索引

六份源码保存、UE关闭，在PowerShell整段复制：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本正常构建Editor并刷新三模块索引；CLion提示时Reload CMake。仍Core89 / UI41 / Editor3共133cpp，不清缓存 / 重建工程，不重复单独Editor Build。

同一窗口进行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两项成功再打开工程。不改资产默认，不强制全Content重存，不运行Cook。

## 8. 两个相关专项（各一次）

非Play状态，在Output Log底部依次输入，上一项Success再执行下一项：

```text
Automation RunTests EmberTrial.M2.Resources.StaminaRecovery
Automation RunTests EmberTrial.M2.Dodge.CostContract
```

第一项覆盖真实加成30 / 移除20、非法停表与恢复、延迟不重置 / 不绕过、原暂停 / 忙碌与配对清理。第二项保护正式闪避体力消费和原动作成本约束。保持42个入口，不全组复跑、不搭临时蓝图。

专项中原“RecoveryRate=0”仍有一条预期Invalid recovery settings Warning；有效速率非法样本由新逻辑停表处理。失败发完整测试名和首条Expected / error文字，成功长日志由我读取。

## 9. 默认路径短查（约3分钟）

1. 正式地图新Play，生命 / 体力100、治疗3次、仍四件。日志Combat initialized追加StaminaRecoveryBonus=0.0，在Ready之前，日志由我核对。
2. 原StaminaRecovery组件仍单个、Auto Activate保持，默认20 / .75 / .10不用改。按2一次，体力消耗25后按原延迟恢复到100，满值后稳定；无需计秒或截图精确采样点。
3. P打开 / 继续一次，移动转向，左键或Q任选一项最短观察。暂停冻结与加成变化精确时序由专项负责，不要求手动抢时点。
4. Stop → 新Play，体力新100 / 加成0、原资源与四件保持，再Stop。保存源码；无临时蓝图、第五件 / 新配置或帧率设置要清理。

## 10. 完成反馈

```text
M3-06B3-01 完成
六既有Core源码；UE关闭后Editor Build / 三索引 / Game Build：
StaminaRecovery / Dodge.CostContract各一次：
出生StaminaRecoveryBonus0在Ready前，原四件 / 资源保持：
2一次扣25 / 延迟恢复到100、满值稳定：
P与最短旧输入 / Stop新100与加成0：
Source保存，资产配置保持；失败或不理解处（无填无）：
完成时分 / 手动代码与理解 / 检查记录 / 构建等待：
```

学习重点：恢复速率来自属性聚合，基础配置与实际体力仍各有唯一来源；速率事件只调整采样，Timer是实际修改资源的入口。观察Serial保护旧订阅，动作Tag数组保持原合同。

## 11. Commit / Push（固定放在最后）

本次B2-03收尾可Stage以下**12份文档 / 来源清单**。本地4f8da23已经包含五份Source、Config和回生实物，原Panel指纹保持，不需要重复Stage实现。B3-01目前只有Artifacts候选。

```text
Docs/Reports/M3_06B2_03_Verification.md
Docs/M3_06B2_03_HealingRelicAndComparison.md
Docs/M3_06B3_01_StaminaRecoveryBonusFoundation.md
Docs/AssetManifest.csv
README.md
Introduction.md
Docs/Progress.md
Docs/Backlog.md
Docs/M3_Roadmap.md
Docs/Architecture.md
Docs/AcceptanceWorkflow_Options.md
Docs/VersionControl.md
```

- 英文：`docs: verify healing relic and plan stamina recovery bonus`
- 专业中文：`docs: 完成治疗遗物验收并规划体力恢复加成`

Artifacts候选 / 预检 / JSON / 原日志、包及个人ini保持本地。Stage后继续编辑需重新Stage → Review → Commit / Push，由你操作；助手未操作索引、提交或远端。

**你实施并验收本小节后，再提交以下六份既有Core源码的真实改动：**

```text
EmberTrial/Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h
EmberTrial/Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Public/Resources/ETStaminaRecoveryComponent.h
EmberTrial/Source/EmberTrial/Private/Resources/ETStaminaRecoveryComponent.cpp
EmberTrial/Source/EmberTrial/Tests/ETStaminaRecoveryTests.cpp
```

- 英文：`feat: support additive stamina recovery rates`
- 专业中文：`feat: 支持体力恢复速率加成`

届时有新的报告 / 状态变化再一并Stage；本批没有资产、Config、UI或第五件需要Push，不提交Artifacts候选。
