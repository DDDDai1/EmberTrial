# M2-05A：韧性属性基础

2026-10-20，固定UE5.5.4。**本章指南已下发、待你手动实现，未执行新Build / 测试 / PIE。**前置[M2-04C与治疗整章](Reports/M2_04C_Verification.md)已验收，无待补项；破势模块分批范围见[M2-05路线](M2_05_BreakPulseRoadmap.md)。

## 1. 目标、边界与用时

给共享战斗身体新增独立`UETPoiseAttributeSet`，包含`Poise / MaxPoise`。ASC是实时数值的唯一来源，Character只保存出生配置。正式默认100 / 100；初始化 / Base与Current夹限 / 上限变化 / 非有限值 / 同身体换控不重置先有稳定契约，后续范围破势才有可靠的结算目标。

**本批不会出现新的可按技能或韧性HUD。**普通左键伤害、受击、死亡、闪避与治疗照旧；Poise=0本批是有效资源值，不等于Health=0，也不自动触发破势、死亡或恢复。破势状态、优先级、回填、范围检测、动画生效点与冷却分别在后续批次接入。不临时加蓝图技能代替正式能力，不为A制作新动画 / GE资产 / 测试Pawn。

| 契约 | 本批实现 |
| --- | --- |
| 出生上限 | 有限数且MaxPoise≥1 |
| 出生当前值 | 有限数，0≤Poise≤MaxPoise；非法时全部三组属性拒绝初始化 |
| 运行值 | Base与Current均夹在合法区间；非有限输入回退当前安全值 |
| 上限变化 | 降低时压低越界当前值；提高上限不补韧性 |
| 初始化 | Health / Stamina / Poise全部配置通过后，先各上限后当前值，完成后才Ready |
| 重复Ensure / 换控 | 保留同一身体实时值；不是恢复入口 |
| EndPlay | 撤销Ready，快照返回false并清零输出，拒绝重初始化 |
| 性能 | 新属性无Tick / Timer、无Actor扫描或UI轮询 |

预计手动约70–110分钟（代码 / 理解40–70，配置与记录15–25，相关检查5–10），构建等待另记。当前实际120 Source / 63cpp、Core43 / UI17 / Editor3、来源64项；你完成本批后预计123 Source / 65cpp、Core45 / UI17 / Editor3，资产来源条目数通常保持64。不要提前将预计数计为完成。

## 2. 准备与精确文件清单

1. Stop PIE → Save All → 关闭UE，CLion保存现有文件。新反射类型正常构建，不用Live Coding。
2. 创建三份新文件，再修改三份既有文件。**Poise属性集属于EmberTrial核心模块，不属于UI或Editor模块。**注意`Attributes`为复数；保留原Health / Stamina文件，不能改名替代它们。
3. 按第3–6节输入代码，第7节Build，之后才打开UE配置 / 校验 / 短查。

| 类型 | 相对于仓库根目录的路径 |
| --- | --- |
| 新 | `EmberTrial/Source/EmberTrial/Public/Attributes/ETPoiseAttributeSet.h` |
| 新 | `EmberTrial/Source/EmberTrial/Private/Attributes/ETPoiseAttributeSet.cpp` |
| 新 | `EmberTrial/Source/EmberTrial/Tests/ETPoiseFoundationTests.cpp` |
| 修改 | `EmberTrial/Source/EmberTrial/Public/Characters/EmberTrialCharacter.h` |
| 修改 | `EmberTrial/Source/EmberTrial/Private/Characters/EmberTrialCharacter.cpp` |
| 修改 | `EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp` |

现有GAS / Automation依赖已齐全，本章不修改Build.cs / Target / .uproject，不修改UI或输入。不要把测试文件拼成Texts或移到Public；使用既有Tests目录。

## 3. 新增Poise属性集：两份完整文件

### 3.1 ETPoiseAttributeSet.h

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETPoiseAttributeSet.generated.h"

// ASC管理的韧性数值；Character只保存启动配置，UI只读事件快照。
UCLASS()
class EMBERTRIAL_API UETPoiseAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETPoiseAttributeSet(const FObjectInitializer& ObjectInitializer);

    // 对外只提供属性与数值查询；初始化宏留在private，修改仍经ASC / GE。
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETPoiseAttributeSet, Poise)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Poise)
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETPoiseAttributeSet, MaxPoise)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxPoise)

    virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                        float& NewValue) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                     float NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Poise",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Poise;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Poise",
              meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxPoise;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(Poise)
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxPoise)

    // Base与Current共用夹限入口；上限改变后的同步由PostAttributeChange负责。
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
```

### 3.2 ETPoiseAttributeSet.cpp

```cpp
#include "Attributes/ETPoiseAttributeSet.h"
#include "AbilitySystemComponent.h"

UETPoiseAttributeSet::UETPoiseAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 仅是构造基线；正式初值由Character验证后统一发布Ready。
    InitMaxPoise(100.0f);
    InitPoise(0.0f);
}

// ---- 数值夹限与非有限输入回退，不保存另一份实时韧性 ----

void UETPoiseAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute,
                                          float& NewValue) const
{
    const float SafeMax = FMath::IsFinite(GetMaxPoise()) ? FMath::Max(1.0f, GetMaxPoise()) : 1.0f;

    if (Attribute == GetMaxPoiseAttribute())
    {
        NewValue = FMath::IsFinite(NewValue) ? FMath::Max(1.0f, NewValue) : SafeMax;
    }
    else if (Attribute == GetPoiseAttribute())
    {
        const float SafeCurrent =
            FMath::IsFinite(GetPoise()) ? FMath::Clamp(GetPoise(), 0.0f, SafeMax) : 0.0f;
        NewValue = FMath::IsFinite(NewValue) ? FMath::Clamp(NewValue, 0.0f, SafeMax) : SafeCurrent;
    }
}

// ---- Base与Current写入均经过同一入口 ----

void UETPoiseAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute,
                                                  float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETPoiseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

// 最大值下降时核对Base和Current；提高上限不会补回之前被夹掉的韧性。
void UETPoiseAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
                                               float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxPoiseAttribute())
    {
        if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
        {
            const float Base = ASC->GetNumericAttributeBase(GetPoiseAttribute());
            if (Base > NewValue || GetPoise() > NewValue)
            {
                ASC->SetNumericAttributeBase(GetPoiseAttribute(),
                                             FMath::Clamp(Base, 0.0f, NewValue));
            }
        }
    }
}
```

构造时Poise=0只是未初始化基线；正式100/100由Character配置写入，不代表角色出生就破势。属性集只处理数值，不播放动画或添加Stunned Tag。上限回调通过ASC写Base，避免只改显示Current、Base还藏着越界值。

## 4. Character.h：六处精确修改

保持原反射名 / 默认子对象名，旧HealthAttributes、StaminaAttributes与功能不删。新增字段仍放在原public之前的私有区域，公开快照放public。

### 4.1 前置声明
找到：

```cpp
class UETStaminaAttributeSet;
```
替换为：

```cpp
class UETStaminaAttributeSet;
class UETPoiseAttributeSet;
```

### 4.2 添加原生属性集成员

在出生配置分区注释之前添加字段；原注释保留。
找到：

```cpp
    // ---- 出生配置；不是运行值副本，同身体换控不会再次写入 ----
```
替换为：

```cpp
    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Poise")
    TObjectPtr<UETPoiseAttributeSet> PoiseAttributes;

    // ---- 出生配置；不是运行值副本，同身体换控不会再次写入 ----
```

### 4.3 添加两个出生配置

在原自动化夹具注释之前添加；不要重复新增另一份InitialHealth或InitialStamina。
找到：

```cpp
    // 自动化夹具只访问既有启动配置 / 句柄，不增加生产测试开关。
```
替换为：

```cpp
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "1.0"))
    float InitialMaxPoise = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup", meta = (ClampMin = "0.0"))
    float InitialPoise = 100.0f;

    // 自动化夹具只访问既有启动配置 / 句柄，不增加生产测试开关。
```

### 4.4 添加单个测试friend
找到：

```cpp
    friend class FETStaminaFoundationTest;
```
替换为：

```cpp
    friend class FETStaminaFoundationTest;
    friend class FETPoiseFoundationTest;
```

### 4.5 更新Ready注释
找到：

```cpp
    // Ready只表示两组属性链初始化成功；不代表存活、受控或能力已授予。
```
替换为：

```cpp
    // Ready只表示三组属性链初始化成功；不代表存活、受控或能力已授予。
```

### 4.6 添加公开快照声明

原Ensure声明保留；新函数与原Health / Stamina快照一样是只读查询。
找到：

```cpp
    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Gameplay")
    bool EnsureGameplayInitialized();
```
替换为：

```cpp
    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool GetGameplayPoiseSnapshot(float& OutPoise, float& OutMaxPoise) const;

    UFUNCTION(BlueprintCallable, Category = "EmberTrial|Gameplay")
    bool EnsureGameplayInitialized();
```

## 5. Character.cpp：五处修改

### 5.1 include
找到：

```cpp
#include "Attributes/ETStaminaAttributeSet.h"
```
替换为：

```cpp
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
```

### 5.2 构造一个默认属性集

只在构造函数中创建一次；不在BeginPlay / PossessedBy / Ensure里再NewObject。
找到：

```cpp
    StaminaAttributes = CreateDefaultSubobject<UETStaminaAttributeSet>(TEXT("StaminaAttributes"));
```
替换为：

```cpp
    StaminaAttributes = CreateDefaultSubobject<UETStaminaAttributeSet>(TEXT("StaminaAttributes"));
    PoiseAttributes = CreateDefaultSubobject<UETPoiseAttributeSet>(TEXT("PoiseAttributes"));
```

### 5.3 完整替换IsGameplayReady函数

找到原定义`bool AEmberTrialCharacter::IsGameplayReady() const`，从函数签名到它自己的结束`}`整体替换为下文。不要覆盖后面的`IsFormalActionIdle`或`IsCombatDead`，以免重现曾经漏定义的链接错误。原函数上方注释改为“Ready表示三组初始化完成；零资源或失去控制不会单独撤销它。”

```cpp
bool AEmberTrialCharacter::IsGameplayReady() const
{
    return bGameplayReady && bStartupAttributesInitialized && !bAbilitySystemShuttingDown &&
           IsValid(AbilitySystemComponent) && IsValid(HealthAttributes) &&
           IsValid(StaminaAttributes) && IsValid(PoiseAttributes);
}
```

### 5.4 添加快照实现

在既有`GetGameplayStaminaSnapshot`结束后、`// ---- 一次初始化与Ready通知`分区注释前，插入完整新函数。它不修改初始化标志、不缓存另一份Poise。

```cpp
bool AEmberTrialCharacter::GetGameplayPoiseSnapshot(float& OutPoise, float& OutMaxPoise) const
{
    OutPoise = 0.0f;
    OutMaxPoise = 0.0f;
    if (!IsGameplayReady())
    {
        return false;
    }

    OutPoise = PoiseAttributes->GetPoise();
    OutMaxPoise = PoiseAttributes->GetMaxPoise();
    return true;
}
```

### 5.5 完整替换EnsureGameplayInitialized函数

只替换`bool AEmberTrialCharacter::EnsureGameplayInitialized()`自身，从签名到它自己的结束`}`；后面的`RefreshEmberAbilityActorInfo`及其他函数全部保留。下文保留既有ActorInfo检查、Health / Stamina日志和一次初始化标志，只把Poise纳入同一边界。

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
        AbilitySystemComponent->GetSet<UETHealthAttributeSet>() != HealthAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETStaminaAttributeSet>() != StaminaAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETPoiseAttributeSet>() != PoiseAttributes.Get())
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

    // 三组配置全部通过后才开始写入，非法韧性也不能部分初始化生命 / 体力。
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

    // 每组先上限后当前值；六次写入完成才提交成功标记 / Ready。
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
    SetGameplayReady(true);
    return IsGameplayReady();
}
```

关键顺序：三套依赖与全部出生配置先通过 → 六次写入 → 标志成功 → 三组初始化日志 → Ready广播。重复Ensure先命中已初始化分支，不重新按出生配置补满；为修正非法配置而重试只对尚未成功初始化的身体生效。EndPlay的原封口与撤销逻辑保留即可，无需新清理子对象代码。

## 6. Validator与稳定自动化

### 6.1 既有ETGameplayAssetValidator.cpp：两处增补

找到`ValidateBody`中的必需对象列表，添加PoiseAttributes，原列表其余名称保留：
找到：

```cpp
{TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"), TEXT("StaminaAttributes"),
```
替换为：

```cpp
{TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"), TEXT("StaminaAttributes"),
          TEXT("PoiseAttributes"),
```

再在同函数原训练目标分区注释前添加出生Poise校验。现有规则ET-BODY-004仍检查Stamina；新的ET-BODY-005只检查Poise。
找到：

```cpp
    // 训练目标无需攻击；玩家才要求正式输入、闪避和治疗引用。
```
替换为：

```cpp
    const TOptional<double> Poise = ReadNumber(Blueprint, Defaults, TEXT("InitialPoise"), Issues);
    const TOptional<double> MaxPoise =
        ReadNumber(Blueprint, Defaults, TEXT("InitialMaxPoise"), Issues);
    if (Poise.IsSet() && MaxPoise.IsSet() &&
        (!FMath::IsFinite(Poise.GetValue()) || !FMath::IsFinite(MaxPoise.GetValue()) ||
         MaxPoise.GetValue() < 1.0 || Poise.GetValue() < 0.0 ||
         Poise.GetValue() > MaxPoise.GetValue()))
    {
        AddIssue(Blueprint, TEXT("ET-BODY-005"),
                 TEXT("Require finite InitialMaxPoise>=1 and 0<=InitialPoise<=InitialMaxPoise."),
                 Issues);
    }

    // 训练目标无需攻击；玩家才要求正式输入、闪避和治疗引用。
```

校验只读CDO / 字段，不Spawn身体或应用GE。沿用正式三身体白名单，不把故意非法的历史Waiting测试资产纳入正式Profile。不需要新增Editor源文件、校验Probe或工程插件。

### 6.2 新增ETPoiseFoundationTests.cpp：完整内容

这是一项有实际风险边界的长期测试：真实ASC / Instant GE、Base与Current、非法配置不能部分初始化前两组、零值与非有限值、同身体换控和结束清理。它复用已运行过的临时世界模式，不创建 / 保存项目测试资产，不修改旧断言。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EmberTrialCharacter.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETPoiseFoundationTest, "EmberTrial.M2.Combat.PoiseFoundation",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETPoiseFoundationTest::RunTest(const FString& Parameters)
{
    // 快照的Current / Base共用原有绝对容差；保留float精度，不放宽任何断言。
    constexpr float SnapshotTolerance = 0.001f;

    // ---- 临时世界与四种启动配置；在BeginPlay前完成赋值 ----
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    // 固定AlwaysSpawn，避免同原点的测试身体因碰撞生成失败而干扰资源规则。
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto MakeBody = [&]()
    {
        return World->SpawnActor<AEmberTrialCharacter>(FVector::ZeroVector, FRotator::ZeroRotator,
                                                       Spawn);
    };
    AEmberTrialCharacter* Body = MakeBody();
    AEmberTrialCharacter* ZeroBody = MakeBody();
    AEmberTrialCharacter* InvalidBody = MakeBody();
    AEmberTrialCharacter* NonFiniteBody = MakeBody();
    if (!TestNotNull(TEXT("Main body"), Body) || !TestNotNull(TEXT("Zero poise body"), ZeroBody) ||
        !TestNotNull(TEXT("Invalid configuration body"), InvalidBody) ||
        !TestNotNull(TEXT("Non-finite configuration body"), NonFiniteBody))
    {
        return false;
    }

    Body->InitialPoise = 40.0f;
    Body->InitialMaxPoise = 80.0f;
    ZeroBody->InitialPoise = 0.0f;
    ZeroBody->InitialMaxPoise = 80.0f;
    InvalidBody->InitialHealth = 40.0f;
    InvalidBody->InitialMaxHealth = 80.0f;
    InvalidBody->InitialStamina = 20.0f;
    InvalidBody->InitialMaxStamina = 50.0f;
    InvalidBody->InitialPoise = 100.0f;
    InvalidBody->InitialMaxPoise = 60.0f;
    NonFiniteBody->InitialPoise = std::numeric_limits<float>::quiet_NaN();

    // ---- 初始化边界：未开局拒绝、非法配置原子拒绝，不部分写入生命 ----
    TestFalse(TEXT("Before BeginPlay is not Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Before BeginPlay cannot initialize"), Body->EnsureGameplayInitialized());
    AddExpectedMessagePlain(TEXT("Invalid startup poise"), ELogVerbosity::Warning,
                            EAutomationExpectedMessageFlags::Contains, 2);
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* InvalidASC = InvalidBody->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Main ASC"), ASC) ||
        !TestNotNull(TEXT("Invalid body's ASC"), InvalidASC) ||
        !TestNotNull(TEXT("Registered poise set"), ASC->GetSet<UETPoiseAttributeSet>()))
    {
        return false;
    }
    TestTrue(TEXT("Registered set is the owned default subobject"),
             ASC->GetSet<UETPoiseAttributeSet>() == Body->PoiseAttributes.Get());

    // 同时核对公开快照与ASC Base，防止显示值正确但Base藏着越界值。
    auto ExpectSnapshot = [&](AEmberTrialCharacter* Subject, const TCHAR* Label,
                              float ExpectedPoise, float ExpectedMax)
    {
        float Poise = -1.0f;
        float MaxPoise = -1.0f;
        TestTrue(FString::Printf(TEXT("%s available"), Label),
                 Subject->GetGameplayPoiseSnapshot(Poise, MaxPoise));
        TestTrue(FString::Printf(TEXT("%s current"), Label),
                 FMath::IsNearlyEqual(Poise, ExpectedPoise, SnapshotTolerance));
        TestTrue(FString::Printf(TEXT("%s maximum"), Label),
                 FMath::IsNearlyEqual(MaxPoise, ExpectedMax, SnapshotTolerance));
        const UAbilitySystemComponent* SubjectASC = Subject->GetAbilitySystemComponent();
        TestTrue(FString::Printf(TEXT("%s base"), Label),
                 FMath::IsNearlyEqual(
                     SubjectASC->GetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute()),
                     ExpectedPoise, SnapshotTolerance));
        TestTrue(FString::Printf(TEXT("%s maximum base"), Label),
                 FMath::IsNearlyEqual(SubjectASC->GetNumericAttributeBase(
                                          UETPoiseAttributeSet::GetMaxPoiseAttribute()),
                                      ExpectedMax, SnapshotTolerance));
    };
    // 期望值独立写明，不从生产默认配置反推，避免实现与测试一同漂移。
    ExpectSnapshot(Body, TEXT("Initial"), 40.0f, 80.0f);
    ExpectSnapshot(ZeroBody, TEXT("Zero is valid"), 0.0f, 80.0f);
    TestTrue(TEXT("Zero poise does not mean dead"), !ZeroBody->IsCombatDead());
    TestFalse(TEXT("Invalid poise remains Waiting"), InvalidBody->IsGameplayReady());
    TestFalse(TEXT("NaN startup remains Waiting"), NonFiniteBody->IsGameplayReady());
    TestFalse(TEXT("Rejected initialization is not committed"),
              InvalidBody->bStartupAttributesInitialized);
    TestEqual(TEXT("Rejected poise did not partially set Health"),
              InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
              0.0f);
    TestEqual(TEXT("Rejected poise did not partially set MaxHealth"),
              InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetMaxHealthAttribute()),
              100.0f);
    TestEqual(TEXT("Rejected poise did not partially set Stamina"),
              InvalidASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
              0.0f);
    TestEqual(TEXT("Rejected poise did not partially set MaxStamina"),
              InvalidASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetMaxStaminaAttribute()),
              100.0f);
    float UnavailablePoise = -1.0f;
    float UnavailableMax = -1.0f;
    TestFalse(TEXT("Waiting snapshot unavailable"),
              InvalidBody->GetGameplayPoiseSnapshot(UnavailablePoise, UnavailableMax));
    TestEqual(TEXT("Unavailable poise cleared"), UnavailablePoise, 0.0f);
    TestEqual(TEXT("Unavailable maximum cleared"), UnavailableMax, 0.0f);

    // ---- 拒绝后修正配置，仍允许首次成功初始化 ----
    InvalidBody->InitialPoise = 30.0f;
    TestTrue(TEXT("Correcting before first successful initialization works"),
             InvalidBody->EnsureGameplayInitialized());
    ExpectSnapshot(InvalidBody, TEXT("Recovered configuration"), 30.0f, 60.0f);
    TestEqual(TEXT("Recovered Health initialized once"),
              InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
              40.0f);

    TestEqual(TEXT("Corrected initialization also writes Stamina once"),
              InvalidASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
              20.0f);

    // ---- 真实瞬时GE与夹限：只创建Transient对象，不保存项目资产 ----
    // Instant效果不常驻；断言读取真实属性变化，不以持续Effect Handle判断成败。
    auto ApplyInstant =
        [&](const FGameplayAttribute& Attribute, EGameplayModOp::Type Operation, float Magnitude)
    {
        UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
        Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
        FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
        Modifier.Attribute = Attribute;
        Modifier.ModifierOp = Operation;
        Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));
        const FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1.0f);
        ASC->ApplyGameplayEffectSpecToSelf(Spec);
    };
    const FGameplayAttribute PoiseAttribute = UETPoiseAttributeSet::GetPoiseAttribute();
    const FGameplayAttribute MaxAttribute = UETPoiseAttributeSet::GetMaxPoiseAttribute();
    ApplyInstant(PoiseAttribute, EGameplayModOp::AddBase, -25.0f);
    ExpectSnapshot(Body, TEXT("Consumed"), 15.0f, 80.0f);
    TestTrue(TEXT("Repeat initialization succeeds"), Body->EnsureGameplayInitialized());
    TestTrue(TEXT("Second repeat initialization succeeds"), Body->EnsureGameplayInitialized());
    ExpectSnapshot(Body, TEXT("Repeat preserves consumed value"), 15.0f, 80.0f);
    ApplyInstant(PoiseAttribute, EGameplayModOp::AddBase, 200.0f);
    ExpectSnapshot(Body, TEXT("Positive instant adjustment clamped"), 80.0f, 80.0f);
    ApplyInstant(PoiseAttribute, EGameplayModOp::AddBase, -25.0f);
    ExpectSnapshot(Body, TEXT("No hidden overflow"), 55.0f, 80.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 40.0f);
    ExpectSnapshot(Body, TEXT("Maximum reduced"), 40.0f, 40.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 120.0f);
    ExpectSnapshot(Body, TEXT("Maximum increase does not refill"), 40.0f, 120.0f);
    ApplyInstant(PoiseAttribute, EGameplayModOp::AddBase, -200.0f);
    ExpectSnapshot(Body, TEXT("Over reduction"), 0.0f, 120.0f);
    TestTrue(TEXT("Zero poise body stays Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Zero poise is not death"), Body->IsCombatDead());
    ApplyInstant(PoiseAttribute, EGameplayModOp::AddBase, 30.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 0.0f);
    ExpectSnapshot(Body, TEXT("Zero maximum clamped"), 1.0f, 1.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, -20.0f);
    ExpectSnapshot(Body, TEXT("Negative maximum clamped"), 1.0f, 1.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 80.0f);
    ApplyInstant(PoiseAttribute, EGameplayModOp::AddBase, 39.0f);
    ExpectSnapshot(Body, TEXT("Restored test values"), 40.0f, 80.0f);

    // ---- 非有限写入保留安全值；韧性Effect不改变生命 / 体力 ----
    ASC->SetNumericAttributeBase(PoiseAttribute, std::numeric_limits<float>::quiet_NaN());
    ASC->SetNumericAttributeBase(MaxAttribute, std::numeric_limits<float>::infinity());
    ExpectSnapshot(Body, TEXT("Non-finite writes preserve safe values"), 40.0f, 80.0f);
    TestEqual(TEXT("Poise effects leave Health isolated"),
              ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);

    TestEqual(TEXT("Poise effects leave Stamina isolated"),
              ASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()), 100.0f);

    // ---- 同一身体换控不重置资源；两个Controller仅验证Possess生命周期 ----
    AAIController* FirstController = World->SpawnActor<AAIController>();
    AAIController* SecondController = World->SpawnActor<AAIController>();
    if (!TestNotNull(TEXT("First controller"), FirstController) ||
        !TestNotNull(TEXT("Second controller"), SecondController))
    {
        return false;
    }
    FirstController->Possess(Body);
    TestTrue(TEXT("First possession established"), FirstController->GetPawn() == Body);
    FirstController->UnPossess();
    ExpectSnapshot(Body, TEXT("UnPossess preserves body resource"), 40.0f, 80.0f);
    SecondController->Possess(Body);
    TestTrue(TEXT("Second possession established"), SecondController->GetPawn() == Body);
    TestTrue(TEXT("Ensure after possession succeeds"), Body->EnsureGameplayInitialized());
    ExpectSnapshot(Body, TEXT("New controller preserves resource"), 40.0f, 80.0f);

    // ---- 正常结束必须撤销Ready、拒绝重初始化，并清空不可用快照输出 ----
    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay revokes Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("EndPlay cannot reinitialize"), Body->EnsureGameplayInitialized());
    TestFalse(TEXT("EndPlay snapshot unavailable"),
              Body->GetGameplayPoiseSnapshot(UnavailablePoise, UnavailableMax));
    TestEqual(TEXT("EndPlay snapshot clears poise"), UnavailablePoise, 0.0f);
    TestEqual(TEXT("EndPlay snapshot clears maximum"), UnavailableMax, 0.0f);
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

测试正向恢复值由Transient Instant GE写入，只为验证夹限；本批没有自动恢复系统。fixture里两条Invalid startup poise是预期Warning（100/60和NaN），其余错误不能随便加Expected去消除。请勿删除原StaminaFoundation测试或把新测试改为只检查常量。

## 7. 正常双目标Build与CLion索引

### 7.1 Editor与索引

确认六份文件都已保存、UE已关闭，在PowerShell运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

成功后在原CLion工程Reload CMake；Core CodeModel应由43变45，UI17 / Editor3保持，总65cpp。新属性两文件与新测试属于EmberTrialCodeModel，Ctrl+B能到AttributeSet与新头文件。CMake只服务索引，不使用CodeModel目标构建游戏。

### 7.2 Game Development

保持UE关闭，执行：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

两目标通过才打开UE。报错发第一条真实C++ / UHT / 链接错误及文件行号 / 上下文，不只贴UBT exit code 6。generated.h必须是各新头最后一个include；h声明与cpp实现一致，尤其别删旧IsCombatDead。不要把清缓存作为第一步。

## 8. 三正式身体配置与项目校验

### 8.1 逐个检查，均不新增组件

依次打开以下三个既有Blueprint，点击Class Defaults：

| 身体 | 资产路径 |
| --- | --- |
| 玩家 | `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` |
| 训练目标 | `/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget` |
| 正式敌人 | `/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy` |

每一个都按相同步骤：

1. Details搜索`Initial Max Poise`，确认为100；再搜索`Initial Poise`，确认为100。它们是出生配置，不是运行时扣除后的数值。
2. 两项默认从原生继承就是100时，无需人为改成别的再改回；旧生命 / 体力100/100、HealingResource 3/3等配置保持。
3. Details搜索`Poise Attributes`应有一项原生继承引用。AttributeSet不是ActorComponent，不要求出现在Components可添加列表；**不要Add Component、不在蓝图NewObject / Add Attribute Set**。
4. Compile → Save。若实际资产无字节差异，后续Push不强求重存；若出现旧绑定 / 类引用错误，先停下发具体错误，别删除继承ASC或旧属性集。

### 8.2 验证三身体

Content Browser逐个选上述身体 → 右键Asset Actions → Validate Assets。预期各有项目`[M1-07C2] Valid`、Issues=0，不只看UE总体“已验证”。新规则正常配置不会报ET-BODY-005。

非法配置与NaN场景由自动化覆盖，本批不另建负例Blueprint或临时Controller节点。旧历史Waiting身体保留原意，不放入正式地图、不将它设成Default Pawn。

## 9. 分组短查：约5–10分钟

### 9.1 三项相关自动化，各一次

非PIE状态，在Output Log逐条执行，等上一项最终完成再输入下一项：

```text
Automation RunTests EmberTrial.M2.Combat.PoiseFoundation
Automation RunTests EmberTrial.M2.Resources.StaminaFoundation
Automation RunTests EmberTrial.M2.Healing.Ability
```

新Poise测试应Success；两个旧项分别检查扩展公共Ready后原体力基础和正式治疗仍正常。现有其他未变战斗测试沿用已验收结果，不重跑完整13项旧矩阵。自动化覆盖不到视觉与键鼠，下面只补必要短基线。

### 9.2 一次出生快照与旧HUD

**准备：**正式地图`L_ET_VerticalSlice`，不改场景 / GameMode，Play。

**操作与预期：**
1. Output Log应有三正式身体各一条`[M2-05A] Poise initialized`、Poise=100 / MaxPoise=100，位于各身体Ready=1之前；不要求你精确抓时刻，我读日志核对。
2. 玩家只显示原生命100、体力100、治疗3/3；本批没有新韧性面板。无Missing gameplay dependencies / Accessed None。
3. 左键真实训练目标25、2一次25体力、4后Q一次治疗和次数3→2；E提示与P一种继续方式正常。移动 / 转向 / 跳跃正常。
4. 这时普通攻击**不会扣Poise**，也没有新范围技能键；这是本批预期，不去复制一套临时技能来检查。

### 9.3 Stop与第二次出生

Stop PIE后再Play，三身体新的Poise100/100，玩家生命 / 体力100与治疗3/3，Stop无结束后重新Ready或旧观察错误。随后Stop，回正式地图，Save All。三身体默认值保持100/100；slomo1、Debug False、原伤害25/10与动画 / 输入 / UI配置保持。

本批不改Poise为0来手工测试破势，因为该状态功能尚未接入。合法0、部分初始化拒绝、换控保留、上限改变与非有限值由本批真实ASC自动化检查，不再人工搭相同矩阵。

## 10. 反馈与解释训练

```text
M2-05A 完成
三新 / 三旧Source；关闭UE后Editor Build / Core45-UI17-Editor3索引 / Game Build：
三正式身体Poise原生引用、Initial100/100；Compile / Save / Valid：
PoiseFoundation / StaminaFoundation / Healing.Ability各一次：
三身体Poise初始化在Ready前；原生命-体力-次数HUD正常：
左键25 / 2 / 4→Q / E-P与移动转向跳跃短基线：
Stop再开新Poise100/100与原资源默认值、无清理错误：
Source / 资产保存、配置已恢复；失败或特别说明（无填无）：
完成时分（如21:18） / 手动代码与配置 / 检查 / 构建等待：
```

能口头解释：为什么Poise=0仍Ready；为什么非法Poise要在Health / Stamina写入前拒绝；为什么提高MaxPoise不能偷偷恢复当前值；为什么普通受击、破势和死亡应分开决定。解释不额外搭验收代码，可在后续复盘继续展开。

反馈后我会核对实现、日志与保存，保存A报告；无待补项即生成B。普通开发仍由你手动输入C++与配置；需要我直接处理时由你明确提出。

## 11. 本批 Commit / Push（固定放在最后）

### 先提交已验收的C与下一章文档

本轮尚未修改实际Source / UE资产或运行A构建；C源码已由你完成。当前四份新C Source暂存区仍是0字节骨架，必须在CLion**重新Stage完整最新内容**，其余C实现 / 资产和收尾文档一起提交。精确清单见[C第11节](M2_04C_HealingHUD.md#11-本批-commit--push固定放在最后)及[C报告](Reports/M2_04C_Verification.md)。把新增本指南、[破势路线](M2_05_BreakPulseRoadmap.md)与状态同步文档一起纳入即可；没有尚未实施的A Source可提交。

建议C实现与文档Commit：

```text
feat: add event-driven healing charges HUD
```

### 你完成A并收到验收收尾后

- 第2节三份新Source与三份既有Source的完整最终修改；新增文件创建时自动Stage的空骨架不算实现，需要重新Stage最新版本。
- 三正式身体Blueprint仅在实际保存后有差异时提交：玩家`EmberTrial/Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`，训练`EmberTrial/Content/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget.uasset`，敌人`EmberTrial/Content/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy.uasset`。无差异不强制重存，不新增测试资产。
- 届时生成的`Docs/Reports/M2_05A_Verification.md`、需要更新的`Docs/AssetManifest.csv`及本指南 / 路线 / Progress / Backlog / Architecture / AcceptanceWorkflow_Options / VersionControl / README / Introduction最新差异；A报告本轮尚未生成。
- Build.cs / Target / .uproject、输入 / Montage / UI / 地图本批无计划变化，有额外差异先看原因。

建议A功能Commit：

```text
feat: establish poise attributes and initialization contract
```

CLion刷新Changes → 重新Stage最新修改 → 检查Staged Diff有完整代码、资产由LFS管理 → Commit → Push，全部由你操作。Raw / 日志 / 缓存 / 生成CodeModel / Saved / Binaries / Intermediate与包体留本地；不Force Add。
