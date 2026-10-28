# M3-02B：真实战斗属性、装备GE与轻击快照

2026-10-28，固定UE **5.5.4**。本批已由你手动实现并验收；[B报告](Reports/M3_02B_Verification.md)记录四相关Success、双Build与短基线，无待补。实际代码50 / 检查2 / 构建等待5分钟；当前进入[C1会话库存](M3_02C1_SessionInventory.md)。以下代码保留供复查，本轮助手未重写运行工程或重复构建 / 测试。

## 1. 本批效果与范围

把A中的静态加法接到真实GAS：MaxHealth已有属性，新增有符号`LightAttackBonus`；原生Infinite装备GE修改两者，轻击以**(基础伤害 + 加成) × 段倍率**结算。

- 五新Source：CombatAttributeSet.h / cpp、RelicEquipmentEffect.h / cpp、一新专项。
- 七旧Source：Character.h / cpp、LightAttack.h / cpp、GameplayTags.h / cpp、原Combo.Flow测试。每处精确锚点见第5–8节，旧功能保留。
- 新属性基础固定0，不再加一份可配InitialBonus；三正式身体继承一份，重复Ensure / 换控不重置活动效果。
- 整条攻击在激活时读取一次基伤 / 加成，段倍率仍按各段配置；途中GE变化从下一条攻击生效。位移 / 输入 / 镜头规则保持。
- 原生GE无新蓝图资产、周期、Cue或授予Tag。B只提供效果原语，**没有库存 / 正式装备入口**；正常Play仍原100 / 25 / 敌人10。自动化才临时施加20 / 5验证真实120上限 / 三段各30。
- 已验收实际169 Source / 93cpp（Core67 / UI23 / Editor3）、26测试，Content296 / 来源77保持；库存 / 身体事务仍后续。

## 2. 先理解四件事

1. **属性归ASC，不归UI。**CombatAttributeSet只存加成与非有限输入保护；基础25 / 敌人10仍在原GA配置，不复制到Character或Widget。
2. **Infinite与Instant不同。**装备GE有活动句柄，修改Current；Base仍最大生命100 / 加成0。更新本句柄的SetByCaller可改变当前幅度，精确移除后恢复；不能拿Instant“加20”再“减20”冒充可追踪装备。
3. **基础0不是禁止负加成。**负值可用于取舍，最终伤害必须有限 / 为正；Base非法值回退旧Base，Current非法值回退旧Current，避免把含GE的值写回Base。
4. **B还不是装备事务。**GE本身不会防重复Apply，Stacking=None也不是“全身体只能一份”。C组件才维护自己的单句柄、预检 / 库存版本 / 提交与清理；最大值变化导致资源夹取、失败恢复 / 受击归因也在C完整处理。本批专项用80生命在新上限内，证明提高上限不治疗与精确移除，不冒充已覆盖全部换装回滚。

## 3. 创建五个新文件

保存资产、关闭UE，CLion保持整个工程。新建以下文件，不覆盖原Health / Stamina / Poise属性集：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Attributes\ETCombatAttributeSet.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Attributes\ETCombatAttributeSet.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Abilities\Effects\ETRelicEquipmentEffect.h
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Abilities\Effects\ETRelicEquipmentEffect.cpp
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETRelicEquipmentEffectsTests.cpp
```

两个新反射头的generated.h必须最后include；不自己创建生成文件。Tests仍在Core，不改Build.cs / Target / uproject或Config。

## 4. 五份完整新文件代码

### 4.1 ETCombatAttributeSet.h

路径：`Source/EmberTrial/Public/Attributes/ETCombatAttributeSet.h`。完整写入：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETCombatAttributeSet.generated.h"

// 有符号轻击加成由GAS聚合；基础伤害 / 段倍率仍由GA配置，不保存重复副本。
UCLASS()
class EMBERTRIAL_API UETCombatAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETCombatAttributeSet(const FObjectInitializer& ObjectInitializer);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETCombatAttributeSet, LightAttackBonus)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(LightAttackBonus)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Combat",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData LightAttackBonus;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(LightAttackBonus)
};
```

### 4.2 ETCombatAttributeSet.cpp

路径：`Source/EmberTrial/Private/Attributes/ETCombatAttributeSet.cpp`。完整写入：

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
}

void UETCombatAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                  float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        // Base回退必须读Base，不能把含活动GE的Current写回Base再多叠一层。
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetBaseValue());
    }
}

void UETCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetLightAttackBonusAttribute())
    {
        // 有限负值合法，最终攻击是否仍为正数由能力 / 装备预检决定。
        NewValue = ETCombatAttributeSetPrivate::FiniteOrFallback(
            NewValue, LightAttackBonus.GetCurrentValue());
    }
}
```

### 4.3 ETRelicEquipmentEffect.h

路径：`Source/EmberTrial/Public/Abilities/Effects/ETRelicEquipmentEffect.h`。完整写入：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETRelicEquipmentEffect.generated.h"

// 无周期 / 无授予Tag的装备聚合GE；每份Spec必须提供两个SetByCaller，即使是0。
UCLASS()
class EMBERTRIAL_API UETRelicEquipmentEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETRelicEquipmentEffect(const FObjectInitializer& ObjectInitializer);
};
```

### 4.4 ETRelicEquipmentEffect.cpp

路径：`Source/EmberTrial/Private/Abilities/Effects/ETRelicEquipmentEffect.cpp`。完整写入：

```cpp
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"

UETRelicEquipmentEffect::UETRelicEquipmentEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    Period = FScalableFloat(0.0f);
    StackingType = EGameplayEffectStackingType::None;

    FSetByCallerFloat HealthMagnitude;
    HealthMagnitude.DataTag = ETGameplayTags::SetByCaller_EquipmentMaxHealth;
    FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
    HealthModifier.Attribute = UETHealthAttributeSet::GetMaxHealthAttribute();
    HealthModifier.ModifierOp = EGameplayModOp::AddBase;
    HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthMagnitude);

    FSetByCallerFloat AttackMagnitude;
    AttackMagnitude.DataTag = ETGameplayTags::SetByCaller_EquipmentLightAttackBonus;
    FGameplayModifierInfo& AttackModifier = Modifiers.AddDefaulted_GetRef();
    AttackModifier.Attribute = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    AttackModifier.ModifierOp = EGameplayModOp::AddBase;
    AttackModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackMagnitude);
}
```

### 4.5 ETRelicEquipmentEffectsTests.cpp

路径：`Source/EmberTrial/Tests/ETRelicEquipmentEffectsTests.cpp`。完整写入：

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicEquipmentEffectsTest,
    "EmberTrial.M3.Inventory.EquipmentEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicEquipmentEffectsTest::RunTest(const FString& Parameters)
{
    // ---- 数值顺序 / 非法输入：加成先于段倍率，失败输出不残留 ----
    float Damage = -1.0f;
    TestTrue(TEXT("Bonus precedes multiplier"),
             UETGA_LightAttack::TryCalculateStageDamage(25.0f, 5.0f, 2.0f, Damage));
    TestEqual(TEXT("Expected stage damage"), Damage, 60.0f);
    TestTrue(TEXT("Signed tradeoff supported"),
             UETGA_LightAttack::TryCalculateStageDamage(25.0f, -10.0f, 1.0f, Damage));
    TestEqual(TEXT("Tradeoff damage"), Damage, 15.0f);
    TestFalse(TEXT("Nonpositive total rejected"),
              UETGA_LightAttack::TryCalculateStageDamage(25.0f, -25.0f, 1.0f, Damage));
    TestEqual(TEXT("Rejected damage cleared"), Damage, 0.0f);
    TestFalse(TEXT("NaN bonus rejected"), UETGA_LightAttack::TryCalculateStageDamage(
        25.0f, std::numeric_limits<float>::quiet_NaN(), 1.0f, Damage));
    TestFalse(TEXT("Overflow rejected"), UETGA_LightAttack::TryCalculateStageDamage(
        std::numeric_limits<float>::max(), 0.0f, 2.0f, Damage));

    // ---- 真实身体 / ASC / Infinite GE；没有Actor控制或动画夹具 ----
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    AEmberTrialCharacter* Other = World->SpawnActor<AEmberTrialCharacter>(
        FVector(400, 0, 200), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Independent body"), Other))
    {
        return false;
    }
    float Bonus = 99.0f;
    TestFalse(TEXT("Before BeginPlay snapshot unavailable"),
              Body->GetGameplayLightAttackBonusSnapshot(Bonus));
    TestEqual(TEXT("Unavailable output cleared"), Bonus, 0.0f);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* OtherASC = Other->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("ASC"), ASC) || !TestNotNull(TEXT("Other ASC"), OtherASC) ||
        !TestNotNull(TEXT("Owned Combat set"), ASC->GetSet<UETCombatAttributeSet>()))
    {
        return false;
    }
    const FGameplayAttribute BonusAttribute = UETCombatAttributeSet::GetLightAttackBonusAttribute();
    const FGameplayAttribute MaxHealth = UETHealthAttributeSet::GetMaxHealthAttribute();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    TestTrue(TEXT("Ready includes combat snapshot"), Body->GetGameplayLightAttackBonusSnapshot(Bonus));
    TestEqual(TEXT("Default bonus zero"), Bonus, 0.0f);
    int32 CombatSetCount = 0;
    for (const UAttributeSet* Set : ASC->GetSpawnedAttributes())
    {
        CombatSetCount += IsValid(Set) && Set->IsA<UETCombatAttributeSet>() ? 1 : 0;
    }
    TestEqual(TEXT("Exactly one combat set"), CombatSetCount, 1);
    auto Apply = [ASC](const float HealthAdditive, const float AttackAdditive)
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
            UETRelicEquipmentEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
        if (!Spec.IsValid())
        {
            return FActiveGameplayEffectHandle();
        }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentMaxHealth, HealthAdditive);
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, AttackAdditive);
        return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    };
    ASC->SetNumericAttributeBase(Health, 80.0f);
    const FActiveGameplayEffectHandle OwnedHandle = Apply(20.0f, 5.0f);
    if (!TestTrue(TEXT("Infinite effect has a live handle"), OwnedHandle.IsValid()))
    {
        return false;
    }
    TestEqual(TEXT("Effect raises current max"), ASC->GetNumericAttribute(MaxHealth), 120.0f);
    TestEqual(TEXT("Effect leaves max Base"), ASC->GetNumericAttributeBase(MaxHealth), 100.0f);
    TestEqual(TEXT("Effect adds current bonus"), ASC->GetNumericAttribute(BonusAttribute), 5.0f);
    TestEqual(TEXT("Effect leaves bonus Base"), ASC->GetNumericAttributeBase(BonusAttribute), 0.0f);
    TestEqual(TEXT("Increasing max does not heal"), ASC->GetNumericAttribute(Health), 80.0f);
    TestTrue(TEXT("Repeated Ensure stays initialized"), Body->EnsureGameplayInitialized());
    TestEqual(TEXT("Repeated Ensure keeps equipment"), ASC->GetNumericAttribute(BonusAttribute), 5.0f);

    // 非有限Base回退到旧Base0，而不是含活动GE的Current5。
    ASC->SetNumericAttributeBase(BonusAttribute, std::numeric_limits<float>::quiet_NaN());
    TestEqual(TEXT("NaN Base retains zero"), ASC->GetNumericAttributeBase(BonusAttribute), 0.0f);
    TestEqual(TEXT("NaN does not double current bonus"), ASC->GetNumericAttribute(BonusAttribute), 5.0f);
    TMap<FGameplayTag, float> Replacement;
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentMaxHealth, 10.0f);
    Replacement.Add(ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 3.0f);
    ASC->UpdateActiveGameplayEffectSetByCallerMagnitudes(OwnedHandle, Replacement);
    TestEqual(TEXT("Same handle updates max"), ASC->GetNumericAttribute(MaxHealth), 110.0f);
    TestEqual(TEXT("Same handle updates bonus"), ASC->GetNumericAttribute(BonusAttribute), 3.0f);
    TestTrue(TEXT("Owned handle still live"), ASC->GetActiveGameplayEffect(OwnedHandle) != nullptr);

    const FActiveGameplayEffectHandle OtherHandle = Apply(0.0f, 2.0f);
    if (!TestTrue(TEXT("Independent effect active"), OtherHandle.IsValid()))
    {
        return false;
    }
    TestTrue(TEXT("Remove owned handle"), ASC->RemoveActiveGameplayEffect(OwnedHandle));
    TestEqual(TEXT("Other effect survives exact removal"), ASC->GetNumericAttribute(BonusAttribute), 2.0f);
    TestTrue(TEXT("Remove second handle"), ASC->RemoveActiveGameplayEffect(OtherHandle));
    TestEqual(TEXT("Removed max returns to Base"), ASC->GetNumericAttribute(MaxHealth), 100.0f);
    TestEqual(TEXT("Removed bonus returns to zero"), ASC->GetNumericAttribute(BonusAttribute), 0.0f);
    TestEqual(TEXT("Removal preserves health within new limit"), ASC->GetNumericAttribute(Health), 80.0f);
    TestEqual(TEXT("Other body max isolated"), OtherASC->GetNumericAttribute(MaxHealth), 100.0f);
    TestEqual(TEXT("Other body bonus isolated"), OtherASC->GetNumericAttribute(BonusAttribute), 0.0f);
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Bonus = 99.0f;
    TestFalse(TEXT("EndPlay snapshot revoked"), Body->GetGameplayLightAttackBonusSnapshot(Bonus));
    TestEqual(TEXT("EndPlay output cleared"), Bonus, 0.0f);
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

## 5. Character：第四组属性、快照与一次初始化

按以下顺序改同名文件；每个原锚点只匹配一次。片段只改指定位置，其余原代码保留。

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h`。

### 5.1 补前置声明

原片段：

```cpp
class UETPoiseAttributeSet;
```

仅替换该片段为：

```cpp
class UETPoiseAttributeSet;
class UETCombatAttributeSet;
```

### 5.2 补身体拥有的属性集

原片段：

```cpp
    TObjectPtr<UETPoiseAttributeSet> PoiseAttributes;
```

仅替换该片段为：

```cpp
    TObjectPtr<UETPoiseAttributeSet> PoiseAttributes;

    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Combat")
    TObjectPtr<UETCombatAttributeSet> CombatAttributes;
```

### 5.3 同步Ready注释

原片段：

```cpp
// Ready只表示三组属性链初始化成功；不代表存活、受控或能力已授予。
```

仅替换该片段为：

```cpp
// Ready只表示四组属性链初始化成功；不代表存活、受控、装备就绪或能力已授予。
```

### 5.4 补只读快照声明

原片段：

```cpp
    bool GetGameplayPoiseSnapshot(float& OutPoise, float& OutMaxPoise) const;
```

仅替换该片段为：

```cpp
    bool GetGameplayPoiseSnapshot(float& OutPoise, float& OutMaxPoise) const;

    UFUNCTION(BlueprintPure, Category = "EmberTrial|Combat")
    bool GetGameplayLightAttackBonusSnapshot(float& OutBonus) const;
```

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp`。

### 5.5 补属性集include

原片段：

```cpp
#include "Attributes/ETPoiseAttributeSet.h"
```

仅替换该片段为：

```cpp
#include "Attributes/ETPoiseAttributeSet.h"
#include "Attributes/ETCombatAttributeSet.h"
```

### 5.6 构造函数只创建一份CombatAttributes

原片段：

```cpp
    PoiseAttributes = CreateDefaultSubobject<UETPoiseAttributeSet>(TEXT("PoiseAttributes"));
```

仅替换该片段为：

```cpp
    PoiseAttributes = CreateDefaultSubobject<UETPoiseAttributeSet>(TEXT("PoiseAttributes"));
    CombatAttributes = CreateDefaultSubobject<UETCombatAttributeSet>(TEXT("CombatAttributes"));
```

### 5.7 完整替换IsGameplayReady

定位`bool AEmberTrialCharacter::IsGameplayReady() const`，从函数开始到配对结束大括号，完整替换为：

```cpp
// Ready表示四组属性初始化完成；零资源、失去控制或Stunned不单独撤销它。
bool AEmberTrialCharacter::IsGameplayReady() const
{
    return bGameplayReady && bStartupAttributesInitialized && !bAbilitySystemShuttingDown &&
           IsValid(AbilitySystemComponent) && IsValid(HealthAttributes) &&
           IsValid(StaminaAttributes) && IsValid(PoiseAttributes) && IsValid(CombatAttributes);
}
```

### 5.8 在SetGameplayReady前插入新快照函数

定位原锚点（保留）：

```cpp
// ---- 一次初始化与Ready通知；先提交状态，再允许同步观察者查询 ----
```

在锚点**前面**插入下面整段，不复制第二份锚点：

```cpp
bool AEmberTrialCharacter::GetGameplayLightAttackBonusSnapshot(float& OutBonus) const
{
    OutBonus = 0.0f;
    if (!IsGameplayReady() || !IsValid(CombatAttributes) ||
        AbilitySystemComponent->GetSet<UETCombatAttributeSet>() != CombatAttributes.Get())
    {
        return false;
    }
    const float Bonus = CombatAttributes->GetLightAttackBonus();
    if (!FMath::IsFinite(Bonus))
    {
        return false;
    }
    OutBonus = Bonus;
    return true;
}
```

### 5.9 完整替换EnsureGameplayInitialized

定位`bool AEmberTrialCharacter::EnsureGameplayInitialized()`，从函数开始到配对结束大括号，完整替换为：

```cpp
bool AEmberTrialCharacter::EnsureGameplayInitialized()
{
    if (!HasActorBegunPlay() || bAbilitySystemShuttingDown || IsActorBeingDestroyed())
    {
        return false;
    }

    if (!IsValid(AbilitySystemComponent) || !AbilitySystemComponent->AbilityActorInfo.IsValid() ||
        AbilitySystemComponent->AbilityActorInfo->OwnerActor.Get() != this ||
        AbilitySystemComponent->AbilityActorInfo->AvatarActor.Get() != this ||
        !IsValid(HealthAttributes) || !IsValid(StaminaAttributes) || !IsValid(PoiseAttributes) ||
        !IsValid(CombatAttributes) ||
        AbilitySystemComponent->GetSet<UETHealthAttributeSet>() != HealthAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETStaminaAttributeSet>() != StaminaAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETPoiseAttributeSet>() != PoiseAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETCombatAttributeSet>() != CombatAttributes.Get())
    {
        SetGameplayReady(false);
        UE_LOG(LogETAbilityFoundation, Error,
               TEXT("[M2-01A] Missing gameplay dependencies Character=%s"), *GetNameSafe(this));
        return false;
    }

    // 已成功初始化就只读；重复Ensure或换控都不补血 / 体力 / 韧性。
    if (bStartupAttributesInitialized)
    {
        UE_LOG(LogETAbilityFoundation, Display,
               TEXT("[M2-01A] Already initialized Character=%s Health=%.1f/%.1f Stamina=%.1f/%.1f "
                    "Poise=%.1f/%.1f"),
               *GetNameSafe(this), HealthAttributes->GetHealth(), HealthAttributes->GetMaxHealth(),
               StaminaAttributes->GetStamina(), StaminaAttributes->GetMaxStamina(),
               PoiseAttributes->GetPoise(), PoiseAttributes->GetMaxPoise());
        return IsGameplayReady();
    }

    // 三组可配初值全通过后才写入；Combat加成基础固定0，不开放重复启动配置。
    if (!FMath::IsFinite(InitialMaxHealth) || !FMath::IsFinite(InitialHealth) ||
        InitialMaxHealth < 1.0f || InitialHealth < 0.0f || InitialHealth > InitialMaxHealth)
    {
        UE_LOG(LogETAbilityFoundation, Warning,
               TEXT("[M1-03A2-01] Invalid startup health Character=%s Health=%.1f MaxHealth=%.1f"),
               *GetNameSafe(this), InitialHealth, InitialMaxHealth);
        return false;
    }

    if (!FMath::IsFinite(InitialMaxStamina) || !FMath::IsFinite(InitialStamina) ||
        InitialMaxStamina < 1.0f || InitialStamina < 0.0f || InitialStamina > InitialMaxStamina)
    {
        UE_LOG(LogETAbilityFoundation, Warning,
               TEXT("[M2-01A] Invalid startup stamina Character=%s Stamina=%.1f MaxStamina=%.1f"),
               *GetNameSafe(this), InitialStamina, InitialMaxStamina);
        return false;
    }

    if (!FMath::IsFinite(InitialMaxPoise) || !FMath::IsFinite(InitialPoise) ||
        InitialMaxPoise < 1.0f || InitialPoise < 0.0f || InitialPoise > InitialMaxPoise)
    {
        UE_LOG(LogETAbilityFoundation, Warning,
               TEXT("[M2-05A] Invalid startup poise Character=%s Poise=%.1f MaxPoise=%.1f"),
               *GetNameSafe(this), InitialPoise, InitialMaxPoise);
        return false;
    }

    // 每组先上限后当前值；基础资源与Combat写入后才发布Ready。
    AbilitySystemComponent->SetNumericAttributeBase(UETHealthAttributeSet::GetMaxHealthAttribute(),
                                                    InitialMaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(),
                                                    InitialHealth);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETStaminaAttributeSet::GetMaxStaminaAttribute(), InitialMaxStamina);
    AbilitySystemComponent->SetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute(),
                                                    InitialStamina);
    AbilitySystemComponent->SetNumericAttributeBase(UETPoiseAttributeSet::GetMaxPoiseAttribute(),
                                                    InitialMaxPoise);
    AbilitySystemComponent->SetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute(),
                                                    InitialPoise);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETCombatAttributeSet::GetLightAttackBonusAttribute(), 0.0f);
    bStartupAttributesInitialized = true;

    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M1-03A2-01] Health initialized Character=%s Health=%.1f MaxHealth=%.1f"),
           *GetNameSafe(this), HealthAttributes->GetHealth(), HealthAttributes->GetMaxHealth());
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M2-01A] Stamina initialized Character=%s Stamina=%.1f MaxStamina=%.1f"),
           *GetNameSafe(this), StaminaAttributes->GetStamina(), StaminaAttributes->GetMaxStamina());
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M2-05A] Poise initialized Character=%s Poise=%.1f MaxPoise=%.1f"),
           *GetNameSafe(this), PoiseAttributes->GetPoise(), PoiseAttributes->GetMaxPoise());
    UE_LOG(LogETAbilityFoundation, Display,
           TEXT("[M3-02B] Combat initialized Character=%s LightAttackBonus=%.1f"),
           *GetNameSafe(this), CombatAttributes->GetLightAttackBonus());
    SetGameplayReady(true);
    return IsGameplayReady();
}
```

## 6. LightAttack：运行预检、整链快照与段计算

按以下顺序改同名文件；每个原锚点只匹配一次。片段只改指定位置，其余原代码保留。

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGA_LightAttack.h`。

### 6.1 补身体前置声明

原片段：

```cpp
class UAnimMontage;
```

仅替换该片段为：

```cpp
class AEmberTrialCharacter;
class UAnimMontage;
```

### 6.2 补公共伤害计算声明

原片段：

```cpp
    bool HasValidComboConfiguration() const;
```

仅替换该片段为：

```cpp
    bool HasValidComboConfiguration() const;
    // 同时供真实段启动和数值专项使用，结果必须有限且为正。
    static bool TryCalculateStageDamage(float BaseDamage, float Bonus, float Multiplier,
                                        float& OutDamage);
```

### 6.3 补整链快照字段

原片段：

```cpp
    float ActiveStageDamage = 0.0f;
```

仅替换该片段为：

```cpp
    float ActiveStageDamage = 0.0f;
    // 仅本次整链的快照；ClearStageResources不清，EndAbility在Super前清。
    float ActiveBaseAttackDamage = 0.0f;
    float ActiveLightAttackBonus = 0.0f;
```

### 6.4 补内部快照预检声明

原片段：

```cpp
    bool HasValidDamageConfiguration() const;
```

仅替换该片段为：

```cpp
    bool HasValidDamageConfiguration() const;
    bool TryCaptureDamageBonus(const AEmberTrialCharacter* Body, float& OutBonus) const;
```

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGA_LightAttack.cpp`。

### 6.5 补数值上限include

原片段：

```cpp
#include "Engine/World.h"
```

仅替换该片段为：

```cpp
#include "Engine/World.h"
#include "Math/NumericLimits.h"
```

### 6.6 在CanActivateAbility前插入两函数

定位原锚点（保留）：

```cpp
// ---- 初次激活与当前玩家段身份查询 ----
```

在锚点**前面**插入下面整段，不复制第二份锚点：

```cpp
bool UETGA_LightAttack::TryCalculateStageDamage(
    const float BaseDamage, const float Bonus, const float Multiplier, float& OutDamage)
{
    OutDamage = 0.0f;
    if (!FMath::IsFinite(BaseDamage) || BaseDamage <= 0.0f || !FMath::IsFinite(Bonus) ||
        !FMath::IsFinite(Multiplier) || Multiplier <= 0.0f)
    {
        return false;
    }
    const double Resolved = (static_cast<double>(BaseDamage) + Bonus) * Multiplier;
    if (!FMath::IsFinite(Resolved) || Resolved <= 0.0 || Resolved > TNumericLimits<float>::Max())
    {
        return false;
    }
    const float Damage = static_cast<float>(Resolved);
    if (!FMath::IsFinite(Damage) || Damage <= 0.0f)
    {
        return false;
    }
    OutDamage = Damage;
    return true;
}

bool UETGA_LightAttack::TryCaptureDamageBonus(
    const AEmberTrialCharacter* Body, float& OutBonus) const
{
    OutBonus = 0.0f;
    float Bonus = 0.0f;
    float Damage = 0.0f;
    if (!IsValid(Body) || !Body->GetGameplayLightAttackBonusSnapshot(Bonus) ||
        !TryCalculateStageDamage(AttackDamage, Bonus, 1.0f, Damage))
    {
        return false;
    }
    if (bEnableCombo)
    {
        for (const FETLightAttackStage& Stage : ComboFollowUps)
        {
            if (!TryCalculateStageDamage(AttackDamage, Bonus, Stage.DamageMultiplier, Damage))
            {
                return false;
            }
        }
    }
    OutBonus = Bonus;
    return true;
}
```

### 6.7 CanActivateAbility补运行加成预检

定位原锚点（保留）：

```cpp
    const UETCombatTraceComponent* Trace =
        BodyCharacter->FindComponentByClass<UETCombatTraceComponent>();
```

在锚点**前面**插入下面整段，不复制第二份锚点：

```cpp
    float CapturedBonus = 0.0f;
    if (!TryCaptureDamageBonus(BodyCharacter, CapturedBonus))
    {
        return false;
    }
```

### 6.8 StartStage使用整链快照

原片段：

```cpp
    ActiveStageDamage = AttackDamage * RuntimeStages[Index].DamageMultiplier;
```

仅替换该片段为：

```cpp
    if (!TryCalculateStageDamage(ActiveBaseAttackDamage, ActiveLightAttackBonus,
                                  RuntimeStages[Index].DamageMultiplier, ActiveStageDamage))
    {
        return false;
    }
```

### 6.9 ActivateAbility开始清整链字段

原片段：

```cpp
    ActiveStageIndex = INDEX_NONE;
    RuntimeStages.Reset();
    AEmberTrialCharacter* Body =
```

仅替换该片段为：

```cpp
    ActiveStageIndex = INDEX_NONE;
    RuntimeStages.Reset();
    ActiveBaseAttackDamage = 0.0f;
    ActiveLightAttackBonus = 0.0f;
    AEmberTrialCharacter* Body =
```

### 6.10 ActivateAbility再次捕获本次加成

原片段：

```cpp
    if (!IsValid(Body) || Body->IsCombatDead() || !IsValid(Trace) || !Trace->CanStartAttack() ||
        !HasValidDamageConfiguration() || !HasValidComboConfiguration() || !IsValid(AttackMontage))
```

仅替换该片段为：

```cpp
    float CapturedBonus = 0.0f;
    if (!IsValid(Body) || Body->IsCombatDead() || !IsValid(Trace) || !Trace->CanStartAttack() ||
        !HasValidDamageConfiguration() || !HasValidComboConfiguration() || !IsValid(AttackMontage) ||
        !TryCaptureDamageBonus(Body, CapturedBonus))
```

### 6.11 Commit前保存整链快照

定位原锚点（保留）：

```cpp
    const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);
```

在锚点**前面**插入下面整段，不复制第二份锚点：

```cpp
    ActiveBaseAttackDamage = AttackDamage;
    ActiveLightAttackBonus = CapturedBonus;
```

### 6.12 EndAbility在Super前清快照

原片段：

```cpp
    ActiveStageIndex = INDEX_NONE;
    RuntimeStages.Reset();
    const FString BodyName =
```

仅替换该片段为：

```cpp
    ActiveStageIndex = INDEX_NONE;
    RuntimeStages.Reset();
    ActiveBaseAttackDamage = 0.0f;
    ActiveLightAttackBonus = 0.0f;
    const FString BodyName =
```

## 7. GameplayTags：两个装备幅度通道

按以下顺序改同名文件；每个原锚点只匹配一次。片段只改指定位置，其余原代码保留。

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Public/Abilities/ETGameplayTags.h`。

### 7.1 声明两个原生装备通道

原片段：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PoiseDelta);
```

仅替换该片段为：

```cpp
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PoiseDelta);
    // ---- Infinite装备GE的两个聚合幅度，不是Instant资源增量 ----
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentMaxHealth);
    EMBERTRIAL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_EquipmentLightAttackBonus);
```

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Private/Abilities/ETGameplayTags.cpp`。

### 7.2 定义两个原生Tag

定位原锚点（保留）：

```cpp
    // ---- 受击与死亡状态 ----
```

在锚点**前面**插入下面整段，不复制第二份锚点：

```cpp
    // ---- 聚合装备加法，所有Spec均须显式写入两项（可为0） ----
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentMaxHealth, "SetByCaller.Equipment.MaxHealth");
    UE_DEFINE_GAMEPLAY_TAG(SetByCaller_EquipmentLightAttackBonus, "SetByCaller.Equipment.LightAttackBonus");
```

## 8. 保留Combo.Flow原用例，追加真实装备伤害

按以下顺序改同名文件；每个原锚点只匹配一次。片段只改指定位置，其余原代码保留。

文件：`E:/UE_Project/Demo/EmberTrial/Source/EmberTrial/Tests/ETLightComboFlowTests.cpp`。

### 8.1 Flow测试增加原生GE include

原片段：

```cpp
#include "Abilities/ETGameplayTags.h"
```

仅替换该片段为：

```cpp
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Effects/ETRelicEquipmentEffect.h"
```

### 8.2 原负例配置段前插入真实装备连段正例

定位原锚点（保留）：

```cpp
        UAnimMontage* Saved = Attack->ComboFollowUps[0].Montage;
```

在锚点**前面**插入下面整段，不复制第二份锚点：

```cpp
        // 装备正例复用真实Montage / Notify / Sweep / GE，不伪造命中回调。
        Reset();
        FGameplayEffectSpecHandle EquipmentSpec = ASC->MakeOutgoingSpec(
            UETRelicEquipmentEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
        if (!TestTrue(TEXT("Equipment spec created"), EquipmentSpec.IsValid()))
        {
            return false;
        }
        EquipmentSpec.Data->SetSetByCallerMagnitude(
            ETGameplayTags::SetByCaller_EquipmentMaxHealth, 20.0f);
        EquipmentSpec.Data->SetSetByCallerMagnitude(
            ETGameplayTags::SetByCaller_EquipmentLightAttackBonus, 5.0f);
        const FActiveGameplayEffectHandle EquipmentHandle =
            ASC->ApplyGameplayEffectSpecToSelf(*EquipmentSpec.Data.Get());
        if (!TestTrue(TEXT("Equipment effect active"), EquipmentHandle.IsValid()) ||
            !Start() || !Until([&Position]() { return Position() >= .48f; }, 1.0f))
        {
            return false;
        }
        TestEqual(TEXT("First stage includes equipment"), Attack->ActiveStageDamage, 30.0f);
        // 中途撤效果后，这一整链仍使用开始时快照；下一链才读取新的0加成。
        if (!TestTrue(TEXT("Remove only equipment handle"),
                      ASC->RemoveActiveGameplayEffect(EquipmentHandle)) ||
            !Body->RequestLightAttack() ||
            !Until([Attack]() { return Attack->ActiveStageIndex == 1; }, .6f) ||
            !Until([&Position]() { return Position() >= .48f; }, 1.0f) ||
            !Body->RequestLightAttack() ||
            !Until([Attack]() { return Attack->ActiveStageIndex == 2; }, .6f) || !Idle())
        {
            return false;
        }
        TestEqual(TEXT("Equipment combo has three unique serials"), Candidates.Num(), 3);
        TestEqual(TEXT("Equipment combo real current health"),
                  TargetASC->GetNumericAttribute(Health), 910.0f);
        TestEqual(TEXT("Equipment combo real base health"),
                  TargetASC->GetNumericAttributeBase(Health), 910.0f);
        Reset();
        if (!Start() || !Idle())
        {
            return false;
        }
        CheckDamage(1);
        AddInfo(TEXT("[M3-02B] Real GE combo: three times30; next activation returns25"));
```

## 9. Build、原生装配与最小检查

### 9.1 正常双Build / 索引

确认第4节五新文件与第5–8节七旧文件全部保存，UE保持关闭：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后CLion Reload CMake / Refresh，核对三个模块目标；预计Core67 / UI23 / Editor3仅作辅助。仍保持UE关闭，Game Development：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

成功后打开原工程。没有新GE蓝图、GA Class / IA引用或资产数值要填；不用挂新的ActorComponent。`CombatAttributes`是原生属性子对象，**不是Add Component里的组件**；单份由专项GetSpawnedAttributes核对，不要求在Components树找到它。

### 9.2 四项相关自动化，各一次

保持非PIE，在Output Log命令栏**每次只执行一行，等该项明确Success后再下一行**：

```text
Automation RunTests EmberTrial.M3.Inventory.EquipmentEffects
Automation RunTests EmberTrial.M2.Combo.Flow
Automation RunTests EmberTrial.M2.Resources.StaminaFoundation
Automation RunTests EmberTrial.M2.Combat.PoiseFoundation
```

第一项查真实ASC属性 / 原生GE / Base与Current、重复Ensure、非有限Base、同句柄更新 / 精确移除 / 身体隔离与结束快照；Flow在原真实通知 / Sweep链补三段各30、下一链25，原用例保留。两基础项核对公共Ready依赖变化。**不重跑全部25项，也不重做A两项库存规则。**

若私有成员错误，先核对是否改到正确Header / 是否完整保留旧friend；新EquipmentEffects只用公开接口，不需要额外friend。若Missing gameplay dependencies，核对Character构造创建CombatAttributes及Ensure完整依赖，不能删依赖检查绕过。新Tag必须声明 / 定义各一次，不在ini重复注册。

### 9.3 正式Play一次短检查（约2–3分钟）

1. 进入原正式地图Play。三身体日志在Ready前各一条`[M3-02B] Combat initialized…LightAttackBonus=0.0`；由我读取，不用你逐条抄。原玩家HUD仍100生命 / 体力、治疗3、F就绪。
2. 对训练目标左键单点确认仍25，再做一次三段；2 / E / P一种继续方式、移动 / 镜头 / 跳跃正常。没有手动施加装备，所以不期待120或30。
3. 有机会观察原敌人一次真实10伤害即可，不要求精准碰帧或重复死亡矩阵；是否观察到按实际反馈。一次4→Q可复用确认原治疗 / HUD仍正常，不建临时按键。
4. Stop一次再Play，新身体加成仍0、原资源正常，然后Stop / Save All。无新资产计划；如果编辑器确实产生了合法资产差异，反馈并保存，后续只提交真实变化，不强制重存所有Blueprint。

无需Cook、十次HUD重建、多Pawn矩阵、所有坡面 / 帧率或再次性能采集。真实装备 / 版本 / 失败回滚与跨World恢复尚未接，不先堆这些验收入口。

## 10. 简短反馈

```text
M3-02B 完成 / 卡在第__节第__步
五新 / 七旧Source已保存；Editor Build / 三索引 / Game Build：
EquipmentEffects / Combo.Flow / StaminaFoundation / PoiseFoundation各一次：
三身体Combat0在Ready前、原100与25 / 三段，2-E-P-移动跳跃短查：
实际敌人伤害（观察到10 / 未观察）；4→Q与Stop新身体0：
源码 / 原资产Save All；失败或不理解处（无填无）：
完成时分 / 手动代码 / 检查 / 构建等待：
```

成功长日志不用贴，报错给第一条错误文字 / 文件行号，找不到原锚点时给所在函数与附近10–20行。GAS效果已作用但画面预期不符时先说当前是否真的有装备入口；本批正向装备只在自动化中。

## 11. 本批 Commit / Push（固定放在最后）

完整B与C1实现已在本地8dbd243提交，之前空文件问题已解决；本轮运行源码 / Config / 资产保持，只同步文档。当前C1已验收、C2-01待手动，最新文档 / 实现清单见[C2-01末尾](M3_02C2_01_EquipmentEffectOwnership.md#10-本批-commit--push固定放在最后)。

当前建议英文`docs: verify session inventory and prepare equipment ownership`，中文译文`docs: 记录会话库存验收并准备装备效果句柄管理`。Stage最新内容 / Review Diff / Commit / Push由你操作，Raw / 日志 / 包保持本地；助手未改索引或核验远端。
