# M3-06B2-01：治疗加成属性与正式Q施放快照

2026-11-07下发，UE **5.5.4**。[B1第三件已通过](Reports/M3_06B1_Verification.md)。本章由你手动修改**六份既有Core源码**；当前只有Artifacts中的候选，正式文件尚未改动。

## 1. 这一小节做什么

在原`UETCombatAttributeSet`中新增`HealingBonus`，初始0。正式Q接受施放时读取`HealAmount + 当前聚合HealingBonus`并保存到`ActiveHealAmount`；约0.5秒真实动画通知仍负责唯一生效与次数消费。默认值保持40，原三件遗物和界面保持当前效果。

| 时点 / 条件 | 明确行为 |
| --- | --- |
| 基础40，加成+10 | 接受施放时快照50，真实通知按50治疗，仍只消费1次 |
| 动作中移除加成GE | 本次仍50，下一次重新读取40；不用中途GE变化修改已接受施放 |
| 有限负加成 | 属性可保存；合计必须1–1000，超界提前拒绝，无动画 / 消费 |
| 暂停 / 受击 / 死亡 / 旧通知 | 沿用既有门控、取消与实例 / 代次隔离 |
| 存活换装与新身体 | 这次不改原生命恢复规则；加成只在新身体一次初始化时归零 |

本章先建立真实消费者，**下一小节B2-02**再加入第四件治疗遗物、原生装备GE参数与详情比较。现在不要新增第四条StartingRelics、修改治疗基础值来冒充遗物加成，或创建额外AttributeSet。

预计手动60–90分钟（代码与理解约50–75、检查5–10、记录约5），正常构建等待另记。无新文件、资产、Designer配置或蓝图测试脚手架，规模仍233 Source / 133cpp / 42测试 / Content310，来源93不变。

## 2. 开始前

1. Stop PIE → Save All → 关闭UE。CLion可保留打开；这次增加反射属性，需要正常UBT / UHT构建，不用Live Coding完成头文件更新。
2. 下列相对文件路径均从`E:/UE_Project/Demo`起算。全部在`EmberTrial/Source/EmberTrial`核心模块中；UI / Editor模块不修改。
3. 小文件第3节完整覆盖，大文件按每项“原代码 → 替换为”操作，其他代码保留。不要整段覆盖Character或治疗能力 / 测试文件。
4. 所有代码已对六份当前文件精确匹配；助手对四个候选cpp复用本机UE / MSVC参数完成仅编译检查，四项ExitCode0。这不包含新UHT、链接或运行；第7节正常Build与第8节专项仍需你完成。

## 3. 扩展原Combat属性集

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h`。

### 3.1 完整替换这个小头文件

将这个文件全部替换为以下内容并保存：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETCombatAttributeSet.generated.h"

// 有符号加成由GAS聚合；能力保存基础数值并决定最终结算，不重复存储运行值。
UCLASS()
class EMBERTRIAL_API UETCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETCombatAttributeSet(const FObjectInitializer& ObjectInitializer);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, LightAttackBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(LightAttackBonus)

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, HealingBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(HealingBonus)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

private:
    // 基础伤害加成；段倍率仍由轻击能力配置。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData LightAttackBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(LightAttackBonus)

    // 正式Q的固定治疗加成，不改变次数、最大生命或其他治疗来源。
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData HealingBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(HealingBonus)
};
```

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp`。

### 3.2 完整替换这个小实现文件

将这个文件全部替换为以下内容并保存：

```cpp
#include "Attributes/ETCombatAttributeSet.h"

namespace ETCombatAttributeSetPrivate
{
    float FiniteOrFallback(const float Value, const float Fallback)
    {
        return FMath::IsFinite(Value) ? Value : (FMath::IsFinite(Fallback) ? Fallback : 0.0f);
    }
}

UETCombatAttributeSet::UETCombatAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitLightAttackBonus(0.0f);
    InitHealingBonus(0.0f);
}

// Base与Current分别回退，不能将包含活动GE的Current再次写入Base。
void UETCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                  float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetBaseValue());
    }
    else if (Attribute == GetHealingBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, HealingBonus.GetBaseValue());
    }
}

void UETCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetCurrentValue());
    }
    else if (Attribute == GetHealingBonusAttribute())
    {
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, HealingBonus.GetCurrentValue());
    }
    // 有限负值合法；最终伤害 / 治疗是否可用，由对应能力预检。
}
```

## 4. 在新身体Ready前初始化

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

### 4.1 在原一次初始化中增加零值，保留旧属性

查找以下唯一片段：

```cpp
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetLightAttackBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;
```

仅替换为：

```cpp
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetLightAttackBonusAttribute(), 0.0f);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetHealingBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;
```

### 4.2 扩展现有初始化日志，仍在Ready之前

查找以下唯一片段：

```cpp
           TEXT("[M3-02B] Combat initialized Character=%s LightAttackBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus());
```

仅替换为：

```cpp
           TEXT("[M3-02B] Combat initialized Character=%s LightAttackBonus=%.1f HealingBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus(),
           CombatAttributes->GetHealingBonus());
```

## 5. 正式Q查询与施放快照

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Heal.h`。

### 5.1 增加私有结算查询声明，不需要UFUNCTION

查找以下唯一片段：

```cpp
    // 生效点重新核对身体与控制权，不依赖激活时的裸指针。
    bool ReadContext
```

仅替换为：

```cpp
    // 预检与启动共用：读取当前聚合加成，失败清空输出；生效通知使用施放快照。
    bool TryGetEffectiveHealAmount(UAbilitySystemComponent* ASC, float& OutAmount) const;

    // 生效点重新核对身体与控制权，不依赖激活时的裸指针。
    bool ReadContext
```

### 5.2 说明本次快照的含义

查找以下唯一片段：

```cpp
    int32 ActiveMontageInstanceID = INDEX_NONE;
    float ActiveHealAmount = 0.0f;
```

仅替换为：

```cpp
    int32 ActiveMontageInstanceID = INDEX_NONE;
    // 本次接受施放时的基础值 + 聚合加成；中途GE变化只影响下一次施放。
    float ActiveHealAmount = 0.0f;
```

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Heal.cpp`。

### 5.3 显式包含属性头文件

查找以下唯一片段：

```cpp
#include "AbilitySystemComponent.h"
```

仅替换为：

```cpp
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
```

### 5.4 在ReadContext之前插入完整结算函数

查找以下唯一片段：

```cpp
// 生效点也调用：身体、控制器或暂停状态可能已在播放期间变化。
```

仅替换为：

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

// 生效点也调用：身体、控制器或暂停状态可能已在播放期间变化。
```

### 5.5 CanActivateAbility使用聚合后的治疗量

查找以下唯一片段：

```cpp
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    return IsValid(Resource) && Resource->CanApplyHealing(HealAmount) &&
```

仅替换为：

```cpp
    UAnimInstance* Anim = ActorInfo->GetAnimInstance();
    float EffectiveHealAmount = 0.0f;
    return TryGetEffectiveHealAmount(ASC, EffectiveHealAmount) &&
           IsValid(Resource) && Resource->CanApplyHealing(EffectiveHealAmount) &&
```

### 5.6 ActivateAbility再次核对并捕获一次快照

查找以下唯一片段：

```cpp
    if (!IsValid(Resource) || !Resource->CanApplyHealing(HealAmount))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    ActiveMontage = HealMontage;
    ActiveHealAmount = HealAmount;
```

仅替换为：

```cpp
    float EffectiveHealAmount = 0.0f;
    if (!TryGetEffectiveHealAmount(ASC, EffectiveHealAmount) ||
        !IsValid(Resource) || !Resource->CanApplyHealing(EffectiveHealAmount))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
        return;
    }
    ActiveMontage = HealMontage;
    ActiveHealAmount = EffectiveHealAmount;
```

**核对：** `HandleHealCommit`仍调用`Resource->TryApplyHealing(ActiveHealAmount)`；这一行不改成重新读加成。`ReleaseMontage`清零快照、旧Task解绑、Serial和实例ID检查都保留。你只修改上面列出的片段。

## 6. 扩展既有Healing.Ability测试

**文件：** `E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETHealAbilityTests.cpp`。

### 6.1 增加显式依赖

查找以下唯一片段：

```cpp
#include "Attributes/ETHealthAttributeSet.h"
```

仅替换为：

```cpp
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "GameplayEffect.h"
```

### 6.2 保留原场景，再增加两个命名场景

查找以下唯一片段：

```cpp
    // 原0..12场景按原顺序列出；这是测试选择，不是生产能力状态。
```

仅替换为：

```cpp
    // 原13个场景保留；追加加成快照 / 非法加成，不新增自动化入口。
```

### 6.3 枚举尾部追加两项

查找以下唯一片段：

```cpp
        FullHealthBeforeCommit,
    };
```

仅替换为：

```cpp
        FullHealthBeforeCommit,
        BonusSnapshot,
        InvalidBonus,
    };
```

### 6.4 场景数组尾部同步追加两项

查找以下唯一片段：

```cpp
        EHealingTestScenario::FullHealthBeforeCommit,
    };
```

仅替换为：

```cpp
        EHealingTestScenario::FullHealthBeforeCommit,
        EHealingTestScenario::BonusSnapshot,
        EHealingTestScenario::InvalidBonus,
    };
```

### 6.5 新增出生加成为零断言，受伤准备仍放在初始化后

查找以下唯一片段：

```cpp
        // 每个治疗场景都在出生装备恢复之后受伤，保持40→80的真实Notify / GE断言。
```

仅替换为：

```cpp
        const FGameplayAttribute HealingBonus = UETCombatAttributeSet::GetHealingBonusAttribute();
        if (!TestNotNull(TEXT("Combat attribute set"), ASC->GetSet<UETCombatAttributeSet>()) ||
            !TestEqual(TEXT("Initial healing bonus Base"),
                       ASC->GetNumericAttributeBase(HealingBonus), 0.0f) ||
            !TestEqual(TEXT("Initial healing bonus Current"),
                       ASC->GetNumericAttribute(HealingBonus), 0.0f))
        {
            return false;
        }
        // 在出生装备恢复之后受伤；40→80或90，避免满血夹限掩盖真实治疗量。
```

### 6.6 在原Normal前置检查之前插入新场景准备

查找以下唯一片段：

```cpp
        if (Scenario == EHealingTestScenario::NormalAndStaleCallbacks)
        {
            Heal->HealMontage = nullptr;
```

仅替换为：

```cpp
        FActiveGameplayEffectHandle BonusHandle;
        if (Scenario == EHealingTestScenario::BonusSnapshot)
        {
            // 真实活动GE负责聚合；不是直接改能力快照制造通过。
            UGameplayEffect* BonusEffect = NewObject<UGameplayEffect>(GetTransientPackage());
            BonusEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
            FGameplayModifierInfo& Modifier = BonusEffect->Modifiers.AddDefaulted_GetRef();
            Modifier.Attribute = HealingBonus;
            Modifier.ModifierOp = EGameplayModOp::AddBase;
            Modifier.ModifierMagnitude = FScalableFloat(10.0f);
            BonusHandle = ASC->ApplyGameplayEffectToSelf(BonusEffect, 1.0f, ASC->MakeEffectContext());
            if (!TestTrue(TEXT("Bonus GE active"), BonusHandle.IsValid()) ||
                !TestEqual(TEXT("GE leaves bonus Base zero"),
                           ASC->GetNumericAttributeBase(HealingBonus), 0.0f) ||
                !TestEqual(TEXT("GE aggregates bonus Current ten"),
                           ASC->GetNumericAttribute(HealingBonus), 10.0f))
            {
                return false;
            }
        }
        else if (Scenario == EHealingTestScenario::InvalidBonus)
        {
            ASC->SetNumericAttributeBase(HealingBonus, -40.0f);
            TestEqual(TEXT("Finite signed bonus preserved"),
                      ASC->GetNumericAttribute(HealingBonus), -40.0f);
            TestFalse(TEXT("Nonpositive total rejects before montage"), Body->RequestHeal());
            Check(TEXT("Zero total has no side effect"), 40.0f, 3);
            ASC->SetNumericAttributeBase(HealingBonus, 1000.0f);
            TestFalse(TEXT("Total above resource limit rejects"), Body->RequestHeal());
            Check(TEXT("Excess total has no side effect"), 40.0f, 3);
            TestFalse(TEXT("Invalid total owns no Healing tag"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Healing));

            float Amount = -1.0f;
            ASC->SetNumericAttributeBase(HealingBonus, -39.0f);
            TestTrue(TEXT("Total one accepted by calculation"),
                     Heal->TryGetEffectiveHealAmount(ASC, Amount));
            TestEqual(TEXT("Lower inclusive limit"), Amount, 1.0f);
            ASC->SetNumericAttributeBase(HealingBonus, 960.0f);
            TestTrue(TEXT("Total thousand accepted by calculation"),
                     Heal->TryGetEffectiveHealAmount(ASC, Amount));
            TestEqual(TEXT("Upper inclusive limit"), Amount, 1000.0f);
            TestFalse(TEXT("Missing ASC rejects"), Heal->TryGetEffectiveHealAmount(nullptr, Amount));
            TestEqual(TEXT("Rejected amount cleared"), Amount, 0.0f);
            ASC->SetNumericAttributeBase(HealingBonus, 0.0f);
            // 以下仍走正式Request / Montage / Notify，证明拒绝后可正常恢复。
        }
        if (Scenario == EHealingTestScenario::NormalAndStaleCallbacks)
        {
            Heal->HealMontage = nullptr;
```

### 6.7 在忙碌重复请求之前移除本场景GE，检验施放快照

查找以下唯一片段：

```cpp
            if (!TestFalse(TEXT("Busy repeat refuses"), Body->RequestHeal()))
```

仅替换为：

```cpp
            if (Scenario == EHealingTestScenario::BonusSnapshot)
            {
                TestEqual(TEXT("Accepted cast snapshots fifty"), Heal->ActiveHealAmount, 50.0f);
                TestTrue(TEXT("Remove only owned bonus GE"), ASC->RemoveActiveGameplayEffect(BonusHandle));
                TestEqual(TEXT("Bonus Current restored before notify"),
                          ASC->GetNumericAttribute(HealingBonus), 0.0f);
                TestEqual(TEXT("In-flight snapshot unchanged"), Heal->ActiveHealAmount, 50.0f);
            }
            if (!TestFalse(TEXT("Busy repeat refuses"), Body->RequestHeal()))
```

### 6.8 真实提交与重复通知使用本场景预期值

查找以下唯一片段：

```cpp
                    Check(TEXT("One real notify commit"), 80.0f, 2);
                    Heal->HandleHealCommit(Body->GetMesh(), Montage, FirstID, Token);
                    Check(TEXT("Duplicate ignored"), 80.0f, 2);
```

仅替换为：

```cpp
                    const float ExpectedAfterCommit =
                        Scenario == EHealingTestScenario::BonusSnapshot ? 90.0f : 80.0f;
                    Check(TEXT("One real notify commit"), ExpectedAfterCommit, 2);
                    Heal->HandleHealCommit(Body->GetMesh(), Montage, FirstID, Token);
                    Check(TEXT("Duplicate ignored"), ExpectedAfterCommit, 2);
```

### 6.9 结束检查同步本次预期，不改其他旧场景

查找以下唯一片段：

```cpp
                    Check(TEXT("Completion or cancel no refund"), 80.0f, 2);
```

仅替换为：

```cpp
                    Check(TEXT("Completion or cancel no refund"), ExpectedAfterCommit, 2);
```

### 6.10 加成场景也复用下一次施放与旧回调检查

查找以下唯一片段：

```cpp
                    if (Scenario == EHealingTestScenario::NormalAndStaleCallbacks)
                    {
                        ASC->SetNumericAttributeBase(Health, 40.0f);
```

仅替换为：

```cpp
                    if (Scenario == EHealingTestScenario::NormalAndStaleCallbacks ||
                        Scenario == EHealingTestScenario::BonusSnapshot)
                    {
                        ASC->SetNumericAttributeBase(Health, 40.0f);
```

### 6.11 下一次施放必须重新读取已移除加成后的40

查找以下唯一片段：

```cpp
                        TestTrue(TEXT("New montage instance"),
```

仅替换为：

```cpp
                        TestEqual(TEXT("Next cast reads current bonus"), Heal->ActiveHealAmount, 40.0f);
                        TestTrue(TEXT("New montage instance"),
```

### 6.12 结果说明补充新覆盖范围

查找以下唯一片段：

```cpp
                 "synchronous cancel checked."));
```

仅替换为：

```cpp
                 "synchronous cancel, additive bonus snapshot, removal and invalid totals checked."));
```

原13个场景完整保留，追加2个场景但仍为同一自动化入口。BonusSnapshot通过真实Infinite GE得到+10，接受施放后移除该句柄，再等真实Montage通知验证40→90；第二次无加成40→80，重复通知 / 旧回调仍拒绝。InvalidBonus验证合计0 / 超1000拒绝且次数不变、边界1 / 1000及拒绝后恢复。测试里的GE是隔离夹具，不加入正式库存，也不要求你手动复现精确通知时刻。

## 7. 正常Build与索引

1. 六份源码全部保存，确认UE已经关闭。
2. 在PowerShell整段复制下面两行，路径中的反斜杠必须保留：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

脚本先正常构建Editor目标，再刷新三模块索引。成功应无C++ / UHT / 链接error；不要再次单独重复同一次Editor Build。CLion按提示Reload CMake即可；没有新增文件，不需要重建项目或清缓存。

3. 同一窗口进行Game Development Build：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

4. 两次均成功再打开EmberTrial.uproject。此批不改资产默认，不强制重存三身体 / 全Content，不运行Cook。

## 8. 两个相关专项（各一次）

打开正式地图，保持**非Play**状态，在Output Log底部命令行分别输入下面两条。第一项结束后再输入第二项：

```text
Automation RunTests EmberTrial.M2.Healing.Ability
Automation RunTests EmberTrial.M3.Inventory.EquipmentEffects
```

两项均应`completed with result 'Success'`。第一项覆盖新属性零值、真实加成施放 / 移除快照、非法合计及原治疗取消 / 暂停等；第二项确认同一个Combat属性集的原轻击与生命装备效果仍正确。沿用方案B，不跑全42项或再建临时蓝图。

失败时发测试完整名称和第一条Expected / error文字；如果崩溃，直接告知发生在哪项、最近完成时分，日志由我读取。无需复制长成功日志。

## 9. 默认40的最短实机（约3–5分钟）

1. 正式地图新Play，空两槽生命100/100、治疗3/3；本章没有第四件，祭坛仍三行。Output Log应Combat initialized追加HealingBonus=0.0，位于该身体Ready之前，日志由我核对。
2. 找远离敌人的安全位置，沿用**已有开发数字4**造成两次25伤害，HUD应50/100；等受击动作结束、站定。若误遭敌人伤害，重开这条短检查即可，不改默认生命。
3. 单按Q，不在播放中重复按键：通知前仍50/100与3/3，生效后**90/100与2/3**，动作结束可移动。自动化负责加成50及精确时序，此处只确认默认路径没有回归。
4. P打开 / 继续一次，移动转向；左键或2任选一个最短观察，Stop。下次Play仍新100 / 3次，原三件与空槽配置不改。
5. Source保存；无新资产配置，无临时节点需要清理。本批不再重复所有资源、死亡Retry / Continue或镜头矩阵。

## 10. 完成反馈

```text
M3-06B2-01 完成
六既有Core源码；UE关闭后Editor Build / 三索引 / Game Build：
Healing.Ability / Inventory.EquipmentEffects各一次：
出生HealingBonus0在Ready前、原双100与3次 / 三遗物保持：
4两次到50，正式Q晚生效90与2次 / 动作结束恢复：
P与最短旧输入 / Stop新100与3次：
Source保存、资产配置保持；失败或不理解处（无填无）：
完成时分 / 手动代码与理解 / 检查记录 / 构建等待：
```

理解重点：配置基础值、GAS聚合Current与本次施放快照是三个不同层次。资源组件只执行传入的合法治疗量与次数事务，不回查某个遗物ID；UI仍订阅真实生命 / 次数事件。本章没有新Tick / Timer或查询循环。

## 11. Commit / Push（固定放在最后）

本次收尾可提交以下**12份文档 / 来源清单**。本地bcdfe6a已包含B1资产、Config与四份测试，无需重复Stage。B2-01仅下发候选，未改正式Source，不把未实施内容写进功能提交。

```text
Docs/Reports/M3_06B1_Verification.md
Docs/M3_06B1_TradeoffRelic.md
Docs/M3_06B2_01_HealingBonusFoundation.md
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

- 英文：`docs: verify tradeoff relic and plan healing bonus`
- 专业中文：`docs: 完成代价型遗物验收并规划治疗加成`

原日志、JSON、候选、包与个人ini仍留Artifacts本地。若Stage后继续编辑，重新Stage → Review → Commit / Push由你操作；助手未操作索引、提交或远端。

**你实施并验收本小节后，另外提交以下六份既有Source的实际改动：**

```text
EmberTrial/Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h
EmberTrial/Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp
EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp
EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_Heal.h
EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_Heal.cpp
EmberTrial/Source/EmberTrial/Tests/ETHealAbilityTests.cpp
```

- 英文：`feat: snapshot additive healing bonus for heal ability`
- 专业中文：`feat: 为治疗能力接入加法增益与施放快照`

届时有新的验收报告 / 状态改动，一并检查后再Stage；不要把Artifacts中的候选作为源码提交。此批没有第四件资产、Config或UI文件需要Push，Git仍由你操作。
