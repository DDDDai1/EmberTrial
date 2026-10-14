# M2-01A：体力属性、一次初始化与 Ready

生成2026-10-14，固定 **UE 5.5.4**。前置：[M1总验收](Reports/M1_Verification.md)完成，当前75份Source / 37cpp / 来源清单48项，独立基线为C3 Attempt02。**本页是待你手动实施的代码与操作指南，尚未写入Source、编译或运行新测试。**所有C++继续由你手动输入，出错先发文字日志，我读取工程定位；直接修改源码需你当次明确提出。

M2-01资源基础分批推进，本页先落地Stamina / MaxStamina和统一Ready。后续小批接恢复规则 / 必要的资源观察显示；治疗次数在本页确定语义，实际能力留M2-04，Poise留M2-05。预计手动 **60–100分钟**：代码35–65、编辑器确认10–15、检查5–15、保存记录5；构建 / 索引等待另记。实际比预算短可直接交付，不要求做满预算。

## 1. 本批完成后是什么样

玩家、敌人与训练身体各有一个独立体力属性集，默认100 / 100；生命仍由原HealthAttributes管理。一次初始化把两组属性都设置完成才发布Ready，非法配置拒绝整个初始化，零体力允许Ready。重复初始化 / 再次控制同一身体保留运行值；Retry生成新身体后回默认值。

本批可通过初始化日志、三个正式身体的资产校验和一项自动测试看到结果。现有血条照常显示生命；按2仍是M0原型闪避，本批没有接体力消耗，这是预期。正式GAS闪避与成本在M2-02接入。

| 契约 | 本批决定 |
| --- | --- |
| 数值所有者 | 新 `UETStaminaAttributeSet` 由Character创建为默认子对象并由ASC管理；Character只存启动配置，运行值不另存一份 |
| 初值 | `InitialStamina=100`、`InitialMaxStamina=100`；允许0体力，上限至少1，所有值必须有限 |
| 越界 | Current与Base均夹到合法区间；降低上限压低越界体力，上升上限不自动补充 |
| 修改入口 | 初始化用ASC设置Base；运行成本 / 恢复后续用GE，本测试使用真实Instant GE验证夹限 |
| Ready | 两属性集依赖 / 注册正确，所有启动配置通过并成功设置后发布；Ready不等同于当前受控、存活或资源足够 |
| 身体身份 | 同一身体再Possess不重置；新身体重新初始化；保留原 `HealthAttributes` 子对象名与类型 |
| 当前支持的数值语义 | 体力直接加减 / 设置上限使用Instant；后续周期恢复也改变Base。临时持续Buff直接改Stamina或MaxStamina的组合尚未设计 / 验证 |
| 显示接口 | `GetGameplayStaminaSnapshot`失败时输出0 / 0；后续Presenter仍先订阅再读快照，不新增Widget Tick轮询 |

AttributeSet保存属性、ASC负责属性修改与事件，Base / Current的区分见[Epic属性说明](https://dev.epicgames.com/documentation/unreal-engine/gameplay-attributes-and-attribute-sets-for-the-gameplay-ability-system-in-unreal-engine)。本页的函数签名、自动测试接口和AddBase枚举按本机5.5.4源码核对；官网默认版本可能更高，不据此替换本工程API。

## 2. 所有权与后续接口先定清楚

体力是连续数值资源，属于AttributeSet；治疗次数是身体拥有的离散资源，留给专门资源接口，未来生命治疗仍通过Health GE。约定：新试炼 / Retry新身体恢复默认次数，当前世界Pause / 同一身体重新控制保留次数；会话Continue目前重建世界，因此同样按新身体初值，不保存次数。

治疗请求要先检查存活、Ready、未满血和剩余次数；仅在一次治疗实际生效时消费一次，生效前取消不消费，生效后取消不退还，重复通知不得再次提交。具体消费 / Effect失败与同步回调的事务顺序在M2-04实现。本页只记录契约，不添加未使用的治疗组件、整数属性或预留空函数。

体力初始化由Character协调，数值夹限由属性集负责，静态默认配置由Editor校验器负责，运行规则由自动测试检查；血条Presenter / Widget继续走原生命链路。

## 3. 准备与精确文件清单

1. UE停止PIE → **File → Save All** → 关闭UE编辑器；CLion保存所有文件。
2. 在CLion当前 `E:\UE_Project\Demo\EmberTrial` 工程中，按下表路径新建普通 `.h` / `.cpp` 文件。可在Project树对应目录右键New → File；不用UE向导额外生成同名类，不打开只有两个文件的新工作空间。
3. 创建后若暂时提示不属于target，先完成代码并执行第7节刷新，不删除缓存。文件名 / 类名 / generated.h必须逐字一致。

| 操作 | 完整路径 |
| --- | --- |
| 新建头文件 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Attributes\ETStaminaAttributeSet.h` |
| 新建实现 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Attributes\ETStaminaAttributeSet.cpp` |
| 修改角色声明 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Public\Characters\EmberTrialCharacter.h` |
| 修改角色实现 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Private\Characters\EmberTrialCharacter.cpp` |
| 修改既有校验器 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp` |
| 新建持久测试 | `E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETStaminaFoundationTests.cpp` |

现有Build.cs已具备GameplayAbilities与AIModule依赖，测试继续使用已有FTestWorldWrapper。无需修改Build.cs / Target / uproject。AttributeSet是UObject，**不要在角色Components面板再Add一个体力组件**，也不要重命名HealthAttributes / AbilitySystem默认子对象。

## 4. 新属性集：两份文件全文

### 4.1 ETStaminaAttributeSet.h

generated.h必须是最后一条include，位于类声明前；文件没有自己的Tick或复制功能。

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ETStaminaAttributeSet.generated.h"

UCLASS()
class EMBERTRIAL_API UETStaminaAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UETStaminaAttributeSet(const FObjectInitializer& ObjectInitializer);

    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETStaminaAttributeSet, Stamina)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(Stamina)
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UETStaminaAttributeSet, MaxStamina)
    GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxStamina)

    virtual void PreAttributeBaseChange(
        const FGameplayAttribute& Attribute, float& NewValue) const override;
    virtual void PreAttributeChange(
        const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostAttributeChange(
        const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Stamina",
        meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData Stamina;

    UPROPERTY(BlueprintReadOnly, Category = "EmberTrial|Stamina",
        meta = (AllowPrivateAccess = "true"))
    FGameplayAttributeData MaxStamina;

    GAMEPLAYATTRIBUTE_VALUE_INITTER(Stamina)
    GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxStamina)

    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
```

### 4.2 ETStaminaAttributeSet.cpp

原生构造值0 / 100是初始化前的占位；Character完成初始化后才Ready。Pre两个入口分别保护Base与Current；上限降低时通过ASC修正体力Base，避免界面正常但Base藏着越界值。夹限职责见[Epic PreAttributeBaseChange说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayAbilities/UAttributeSet/PreAttributeBaseChange)。

```cpp
#include "Attributes/ETStaminaAttributeSet.h"
#include "AbilitySystemComponent.h"

UETStaminaAttributeSet::UETStaminaAttributeSet(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InitMaxStamina(100.0f);
    InitStamina(0.0f);
}

void UETStaminaAttributeSet::ClampAttribute(
    const FGameplayAttribute& Attribute, float& NewValue) const
{
    const float SafeMax = FMath::IsFinite(GetMaxStamina())
        ? FMath::Max(1.0f, GetMaxStamina()) : 1.0f;

    if (Attribute == GetMaxStaminaAttribute())
    {
        NewValue = FMath::IsFinite(NewValue)
            ? FMath::Max(1.0f, NewValue) : SafeMax;
    }
    else if (Attribute == GetStaminaAttribute())
    {
        const float SafeCurrent = FMath::IsFinite(GetStamina())
            ? FMath::Clamp(GetStamina(), 0.0f, SafeMax) : 0.0f;
        NewValue = FMath::IsFinite(NewValue)
            ? FMath::Clamp(NewValue, 0.0f, SafeMax) : SafeCurrent;
    }
}

void UETStaminaAttributeSet::PreAttributeBaseChange(
    const FGameplayAttribute& Attribute, float& NewValue) const
{
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETStaminaAttributeSet::PreAttributeChange(
    const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UETStaminaAttributeSet::PostAttributeChange(
    const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (Attribute == GetMaxStaminaAttribute())
    {
        if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
        {
            const float Base = ASC->GetNumericAttributeBase(GetStaminaAttribute());
            if (Base > NewValue || GetStamina() > NewValue)
            {
                ASC->SetNumericAttributeBase(
                    GetStaminaAttribute(), FMath::Clamp(Base, 0.0f, NewValue));
            }
        }
    }
}
```

## 5. Character接入：准确插入与完整函数替换

### 5.1 EmberTrialCharacter.h：四处修改

**① 前置声明区**，在 `class UETHealthAttributeSet;` 下一行添加：

```cpp
class UETStaminaAttributeSet;
```

**② 私有区**，找到现有 `TObjectPtr<UETHealthAttributeSet> HealthAttributes;`，在它下面添加这两行声明。不要复制HealthAttributes本身：

```cpp
    UPROPERTY(VisibleAnywhere, Category = "EmberTrial|Stamina")
    TObjectPtr<UETStaminaAttributeSet> StaminaAttributes;
```

**③ 私有区**，在现有 `float InitialHealth = 100.0f;` 后、`bool bStartupAttributesInitialized = false;` 前添加：

```cpp
    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup",
        meta = (ClampMin = "1.0"))
    float InitialMaxStamina = 100.0f;

    UPROPERTY(EditDefaultsOnly, Category = "EmberTrial|Startup",
        meta = (ClampMin = "0.0"))
    float InitialStamina = 100.0f;

    friend class FETStaminaFoundationTest;
```

friend仅让第6节自动测试配置临时身体并检查初始化标记，不把运行时字段公开。保持原bStartupAttributesInitialized / bGameplayReady各一份；本批让原成功标记同时代表生命和体力已初始化，不再加第二套初始化布尔值。

**④ public区**，找到 `bool GetGameplayHealthSnapshot(float& OutHealth, float& OutMaxHealth) const;`，在它后面添加：

```cpp
    UFUNCTION(BlueprintPure, Category = "EmberTrial|Gameplay")
    bool GetGameplayStaminaSnapshot(float& OutStamina, float& OutMaxStamina) const;
```

### 5.2 EmberTrialCharacter.cpp：include与构造

在现有 `#include "Attributes/ETHealthAttributeSet.h"` 后添加：

```cpp
#include "Attributes/ETStaminaAttributeSet.h"
```

在构造函数的现有HealthAttributes创建行后添加，保留原行：

```cpp
    StaminaAttributes = CreateDefaultSubobject<UETStaminaAttributeSet>(TEXT("StaminaAttributes"));
```

本批仍使用当前的 `LogETAbilityFoundation`，不要再定义同名日志类别或额外的ASC。BeginPlay / PossessedBy / UnPossessed / EndPlay保持现有实现，新增逻辑放在它们已调用的初始化入口内。

### 5.3 完整替换 IsGameplayReady

只替换同名函数的整个实现，不复制第二份函数：

**保留附近原有的 `IsCombatDead()` 与 `IsFormalActionIdle()`。**若链接报 `IsCombatDead() const` 的LNK2019 / LNK2001，先检查它是否只有.h声明而没有.cpp实现。补回下面原函数，放在 `RequestHitReact()` 的结束大括号后、`IsGameplayReady()` 前，三个函数各自独立，不嵌套：

```cpp
bool AEmberTrialCharacter::IsCombatDead() const
{
    return IsValid(CombatLifeComponent) && CombatLifeComponent->IsDead();
}
```

然后再确认下面的IsGameplayReady实现。无需改.h声明或清理缓存，保存 / 关闭UE后正常构建即可。

```cpp
bool AEmberTrialCharacter::IsGameplayReady() const
{
    return bGameplayReady && bStartupAttributesInitialized &&
        !bAbilitySystemShuttingDown && IsValid(AbilitySystemComponent) &&
        IsValid(HealthAttributes) && IsValid(StaminaAttributes);
}
```

### 5.4 新增 GetGameplayStaminaSnapshot

放在现有GetGameplayHealthSnapshot的结束 `}` 后、SetGameplayReady前；现有Health快照函数保留：

```cpp
bool AEmberTrialCharacter::GetGameplayStaminaSnapshot(
    float& OutStamina, float& OutMaxStamina) const
{
    OutStamina = 0.0f;
    OutMaxStamina = 0.0f;
    if (!IsGameplayReady())
    {
        return false;
    }

    OutStamina = StaminaAttributes->GetStamina();
    OutMaxStamina = StaminaAttributes->GetMaxStamina();
    return true;
}
```

### 5.5 完整替换 EnsureGameplayInitialized

这是本批关键：**先检查所有依赖与四个初值，再写入任何初值**；成功后才置标记 / Ready。没有存活门控，出生生命0仍让Life收到Ready并提交死亡，保持原M1规则。

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
        !IsValid(HealthAttributes) || !IsValid(StaminaAttributes) ||
        AbilitySystemComponent->GetSet<UETHealthAttributeSet>() != HealthAttributes.Get() ||
        AbilitySystemComponent->GetSet<UETStaminaAttributeSet>() != StaminaAttributes.Get())
    {
        SetGameplayReady(false);
        UE_LOG(LogETAbilityFoundation, Error,
            TEXT("[M2-01A] Missing gameplay dependencies Character=%s"), *GetNameSafe(this));
        return false;
    }

    if (bStartupAttributesInitialized)
    {
        UE_LOG(LogETAbilityFoundation, Display,
            TEXT("[M2-01A] Already initialized Character=%s Health=%.1f/%.1f Stamina=%.1f/%.1f"),
            *GetNameSafe(this), HealthAttributes->GetHealth(), HealthAttributes->GetMaxHealth(),
            StaminaAttributes->GetStamina(), StaminaAttributes->GetMaxStamina());
        return IsGameplayReady();
    }

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

    AbilitySystemComponent->SetNumericAttributeBase(
        UETHealthAttributeSet::GetMaxHealthAttribute(), InitialMaxHealth);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETHealthAttributeSet::GetHealthAttribute(), InitialHealth);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETStaminaAttributeSet::GetMaxStaminaAttribute(), InitialMaxStamina);
    AbilitySystemComponent->SetNumericAttributeBase(
        UETStaminaAttributeSet::GetStaminaAttribute(), InitialStamina);
    bStartupAttributesInitialized = true;

    UE_LOG(LogETAbilityFoundation, Display,
        TEXT("[M1-03A2-01] Health initialized Character=%s Health=%.1f MaxHealth=%.1f"),
        *GetNameSafe(this), HealthAttributes->GetHealth(), HealthAttributes->GetMaxHealth());
    UE_LOG(LogETAbilityFoundation, Display,
        TEXT("[M2-01A] Stamina initialized Character=%s Stamina=%.1f MaxStamina=%.1f"),
        *GetNameSafe(this), StaminaAttributes->GetStamina(), StaminaAttributes->GetMaxStamina());
    SetGameplayReady(true);
    return IsGameplayReady();
}
```

SetGameplayReady原函数不改：因此Ready changed日志仍带M1-03A2-01，这是复用原事件，不是未更新。生命日志、体力日志后才出现Ready=1；同一对象再次Ensure只出现Already initialized。

### 5.6 Editor校验器：完整替换 ValidateBody

文件是 `EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`；只替换原 `ValidateBody` 整个实现，前后的函数保留。新增StaminaAttributes必需引用与 **ET-BODY-004** 配置规则；没有新头文件 / Build依赖，继续反射读取私有配置。现有Valid日志仍带M1-07C2。

```cpp
void UETGameplayAssetValidator::ValidateBody(UBlueprint* Blueprint,
    UObject* Defaults, int32& Issues)
{
    for (const TCHAR* Name : {TEXT("AbilitySystemComponent"), TEXT("HealthAttributes"),
        TEXT("StaminaAttributes"), TEXT("CombatTraceComponent"), TEXT("CombatLifeComponent")})
    {
        RequireObject(Blueprint, Defaults, Name, TEXT("ET-BODY-001"), Issues);
    }
    const TOptional<double> Health = ReadNumber(Blueprint, Defaults, TEXT("InitialHealth"), Issues);
    const TOptional<double> Max = ReadNumber(Blueprint, Defaults, TEXT("InitialMaxHealth"), Issues);
    if (Health.IsSet() && Max.IsSet() && (!FMath::IsFinite(Health.GetValue())
        || !FMath::IsFinite(Max.GetValue()) || Max.GetValue() < 1.0
        || Health.GetValue() < 0.0 || Health.GetValue() > Max.GetValue()))
    {
        AddIssue(Blueprint, TEXT("ET-BODY-002"),
            TEXT("Require finite InitialMaxHealth>=1 and 0<=InitialHealth<=InitialMaxHealth."), Issues);
    }
    const TOptional<double> Stamina = ReadNumber(Blueprint, Defaults, TEXT("InitialStamina"), Issues);
    const TOptional<double> MaxStamina = ReadNumber(Blueprint, Defaults, TEXT("InitialMaxStamina"), Issues);
    if (Stamina.IsSet() && MaxStamina.IsSet() && (!FMath::IsFinite(Stamina.GetValue())
        || !FMath::IsFinite(MaxStamina.GetValue()) || MaxStamina.GetValue() < 1.0
        || Stamina.GetValue() < 0.0 || Stamina.GetValue() > MaxStamina.GetValue()))
    {
        AddIssue(Blueprint, TEXT("ET-BODY-004"),
            TEXT("Require finite InitialMaxStamina>=1 and 0<=InitialStamina<=InitialMaxStamina."), Issues);
    }
    const bool bTraining = Blueprint->GetOutermost()->GetName()
        == TEXT("/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget");
    if (!bTraining)
    {
        RequireObject(Blueprint, Defaults, TEXT("LightAttackAbilityClass"), TEXT("ET-BODY-003"), Issues);
    }
    const bool bPlayer = Blueprint->GetOutermost()->GetName()
        == TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter");
    if (bPlayer)
    {
        UObject* Mapping = RequireObject(Blueprint, Defaults,
            TEXT("DefaultMappingContext"), TEXT("ET-BODY-003"), Issues);
        if (IsValid(Mapping) && Mapping->GetPathName()
            != TEXT("/Game/ThirdPerson/Input/IMC_Default.IMC_Default"))
        {
            AddIssue(Blueprint, TEXT("ET-INPUT-001"),
                TEXT("Player DefaultMappingContext must reference the validated IMC_Default for this profile."),
                Issues);
        }
        for (const TCHAR* Name : {TEXT("JumpAction"), TEXT("MoveAction"), TEXT("LookAction")})
        {
            RequireObject(Blueprint, Defaults, Name, TEXT("ET-BODY-003"), Issues);
        }
    }
    const FObjectPropertyBase* LifeProperty = FindFProperty<FObjectPropertyBase>(
        Defaults->GetClass(), TEXT("CombatLifeComponent"));
    UObject* Life = LifeProperty ? LifeProperty->GetObjectPropertyValue_InContainer(Defaults) : nullptr;
    if (IsValid(Life))
    {
        RequireObject(Blueprint, Life, TEXT("HitReactAbilityClass"), TEXT("ET-BODY-003"), Issues);
    }
}
```

## 6. 可复用运行规则测试：新文件全文

创建 `Source/EmberTrial/Tests/ETStaminaFoundationTests.cpp`。这是长期保留的自动测试，不是每轮删除的临时玩法代码；原Lifecycle / DodgeMovement文件保持原样。

测试建立自己的临时世界，用真实原生Character / ASC / Instant GE；覆盖40/80、0/80、非法100/60、非有限初值、拒绝时生命不被部分初始化、修正后初始化、上下限、过量恢复后扣除、重复Ensure、两Controller换控与EndPlay。它不证明正式玩家蓝图装配、未来GA成本或体力UI；第8节用三正式身体与一条短基线补这部分。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EmberTrialCharacter.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETStaminaFoundationTest,
    "EmberTrial.M2.Resources.StaminaFoundation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETStaminaFoundationTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto MakeBody = [&]()
    {
        return World->SpawnActor<AEmberTrialCharacter>(
            FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    };
    AEmberTrialCharacter* Body = MakeBody();
    AEmberTrialCharacter* ZeroBody = MakeBody();
    AEmberTrialCharacter* InvalidBody = MakeBody();
    AEmberTrialCharacter* NonFiniteBody = MakeBody();
    if (!TestNotNull(TEXT("Main body"), Body)
        || !TestNotNull(TEXT("Zero stamina body"), ZeroBody)
        || !TestNotNull(TEXT("Invalid configuration body"), InvalidBody)
        || !TestNotNull(TEXT("Non-finite configuration body"), NonFiniteBody))
    {
        return false;
    }

    Body->InitialStamina = 40.0f;
    Body->InitialMaxStamina = 80.0f;
    ZeroBody->InitialStamina = 0.0f;
    ZeroBody->InitialMaxStamina = 80.0f;
    InvalidBody->InitialHealth = 40.0f;
    InvalidBody->InitialMaxHealth = 80.0f;
    InvalidBody->InitialStamina = 100.0f;
    InvalidBody->InitialMaxStamina = 60.0f;
    NonFiniteBody->InitialStamina = std::numeric_limits<float>::quiet_NaN();

    TestFalse(TEXT("Before BeginPlay is not Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Before BeginPlay cannot initialize"), Body->EnsureGameplayInitialized());
    AddExpectedMessagePlain(TEXT("Invalid startup stamina"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 2);
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UAbilitySystemComponent* InvalidASC = InvalidBody->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Main ASC"), ASC)
        || !TestNotNull(TEXT("Invalid body's ASC"), InvalidASC)
        || !TestNotNull(TEXT("Registered stamina set"), ASC->GetSet<UETStaminaAttributeSet>()))
    {
        return false;
    }
    TestTrue(TEXT("Registered set is the owned default subobject"),
        ASC->GetSet<UETStaminaAttributeSet>() == Body->StaminaAttributes.Get());

    auto ExpectSnapshot = [&](AEmberTrialCharacter* Subject,
        const TCHAR* Label, float ExpectedStamina, float ExpectedMax)
    {
        float Stamina = -1.0f;
        float MaxStamina = -1.0f;
        TestTrue(FString::Printf(TEXT("%s available"), Label),
            Subject->GetGameplayStaminaSnapshot(Stamina, MaxStamina));
        TestTrue(FString::Printf(TEXT("%s current"), Label),
            FMath::IsNearlyEqual(Stamina, ExpectedStamina, 0.001f));
        TestTrue(FString::Printf(TEXT("%s maximum"), Label),
            FMath::IsNearlyEqual(MaxStamina, ExpectedMax, 0.001f));
        const UAbilitySystemComponent* SubjectASC = Subject->GetAbilitySystemComponent();
        TestTrue(FString::Printf(TEXT("%s base"), Label), FMath::IsNearlyEqual(
            SubjectASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetStaminaAttribute()),
            ExpectedStamina, 0.001f));
        TestTrue(FString::Printf(TEXT("%s maximum base"), Label), FMath::IsNearlyEqual(
            SubjectASC->GetNumericAttributeBase(UETStaminaAttributeSet::GetMaxStaminaAttribute()),
            ExpectedMax, 0.001f));
    };
    ExpectSnapshot(Body, TEXT("Initial"), 40.0f, 80.0f);
    ExpectSnapshot(ZeroBody, TEXT("Zero is valid"), 0.0f, 80.0f);
    TestTrue(TEXT("Zero stamina does not mean dead"), !ZeroBody->IsCombatDead());
    TestFalse(TEXT("Invalid stamina remains Waiting"), InvalidBody->IsGameplayReady());
    TestFalse(TEXT("NaN startup remains Waiting"), NonFiniteBody->IsGameplayReady());
    TestFalse(TEXT("Rejected initialization is not committed"),
        InvalidBody->bStartupAttributesInitialized);
    TestEqual(TEXT("Rejected stamina did not partially set Health"),
        InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()), 0.0f);
    TestEqual(TEXT("Rejected stamina did not partially set MaxHealth"),
        InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetMaxHealthAttribute()), 100.0f);
    float UnavailableStamina = -1.0f;
    float UnavailableMax = -1.0f;
    TestFalse(TEXT("Waiting snapshot unavailable"),
        InvalidBody->GetGameplayStaminaSnapshot(UnavailableStamina, UnavailableMax));
    TestEqual(TEXT("Unavailable stamina cleared"), UnavailableStamina, 0.0f);
    TestEqual(TEXT("Unavailable maximum cleared"), UnavailableMax, 0.0f);

    InvalidBody->InitialStamina = 30.0f;
    TestTrue(TEXT("Correcting before first successful initialization works"),
        InvalidBody->EnsureGameplayInitialized());
    ExpectSnapshot(InvalidBody, TEXT("Recovered configuration"), 30.0f, 60.0f);
    TestEqual(TEXT("Recovered Health initialized once"),
        InvalidASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()), 40.0f);

    // Transient real Instant effects; these do not create or save project assets.
    auto ApplyInstant = [&](const FGameplayAttribute& Attribute,
        EGameplayModOp::Type Operation, float Magnitude)
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
    const FGameplayAttribute StaminaAttribute = UETStaminaAttributeSet::GetStaminaAttribute();
    const FGameplayAttribute MaxAttribute = UETStaminaAttributeSet::GetMaxStaminaAttribute();
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, -25.0f);
    ExpectSnapshot(Body, TEXT("Consumed"), 15.0f, 80.0f);
    TestTrue(TEXT("Repeat initialization succeeds"), Body->EnsureGameplayInitialized());
    TestTrue(TEXT("Second repeat initialization succeeds"), Body->EnsureGameplayInitialized());
    ExpectSnapshot(Body, TEXT("Repeat preserves consumed value"), 15.0f, 80.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, 200.0f);
    ExpectSnapshot(Body, TEXT("Over recovery"), 80.0f, 80.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, -25.0f);
    ExpectSnapshot(Body, TEXT("No hidden overflow"), 55.0f, 80.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 40.0f);
    ExpectSnapshot(Body, TEXT("Maximum reduced"), 40.0f, 40.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 120.0f);
    ExpectSnapshot(Body, TEXT("Maximum increase does not refill"), 40.0f, 120.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, -200.0f);
    ExpectSnapshot(Body, TEXT("Over consumption"), 0.0f, 120.0f);
    TestTrue(TEXT("Exhausted body stays Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("Exhaustion is not death"), Body->IsCombatDead());
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, 30.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 0.0f);
    ExpectSnapshot(Body, TEXT("Zero maximum clamped"), 1.0f, 1.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, -20.0f);
    ExpectSnapshot(Body, TEXT("Negative maximum clamped"), 1.0f, 1.0f);
    ApplyInstant(MaxAttribute, EGameplayModOp::Override, 80.0f);
    ApplyInstant(StaminaAttribute, EGameplayModOp::AddBase, 39.0f);
    ExpectSnapshot(Body, TEXT("Restored test values"), 40.0f, 80.0f);

    ASC->SetNumericAttributeBase(StaminaAttribute, std::numeric_limits<float>::quiet_NaN());
    ASC->SetNumericAttributeBase(MaxAttribute, std::numeric_limits<float>::infinity());
    ExpectSnapshot(Body, TEXT("Non-finite writes preserve safe values"), 40.0f, 80.0f);
    TestEqual(TEXT("Stamina effects leave Health isolated"),
        ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);

    AAIController* FirstController = World->SpawnActor<AAIController>();
    AAIController* SecondController = World->SpawnActor<AAIController>();
    if (!TestNotNull(TEXT("First controller"), FirstController)
        || !TestNotNull(TEXT("Second controller"), SecondController))
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

    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay revokes Ready"), Body->IsGameplayReady());
    TestFalse(TEXT("EndPlay cannot reinitialize"), Body->EnsureGameplayInitialized());
    TestFalse(TEXT("EndPlay snapshot unavailable"),
        Body->GetGameplayStaminaSnapshot(UnavailableStamina, UnavailableMax));
    TestEqual(TEXT("EndPlay snapshot clears stamina"), UnavailableStamina, 0.0f);
    TestEqual(TEXT("EndPlay snapshot clears maximum"), UnavailableMax, 0.0f);
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

Instant GE可能不返回有效的持续Effect Handle，因为它即时执行后不会常驻；测试以属性变化与断言为据。两个非法启动Warning用AddExpectedMessagePlain精确登记次数，不把意外Error或其他Warning屏蔽。friend名字必须是 `FETStaminaFoundationTest`，与IMPLEMENT里的测试类名一致。

## 7. 正常 Build与CLion索引

所有文件保存，确认UE仍关闭。在PowerShell执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

这是Development Editor构建 + 三模块索引刷新；CLion收到CMake变更后Reload CMake Project，等待索引完成。新两个属性文件与测试文件应属于EmberTrialCodeModel；校验器属于EmberTrialEditorCodeModel。Ctrl+B可定位新属性头与引擎AttributeSet.h；generated.h由UHT生成，不手动创建。

Editor Build成功后，仍保持UE关闭，执行一次Game构建：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

Game Target不应新增Editor依赖；这是正常UBT构建，不从CMake CodeModel目标编译游戏。脚本末尾 `UBT failed with exit code 6` 只是汇总，报错时发它上面的**第一条UHT / C++实际错误**和文件路径 / 行号，不只发exit code。

## 8. 分层检查（合计约5–15分钟）

### 8.1 三个正式身体装配（约2–4分钟）

打开UE，**不Play**。依次打开以下既有蓝图，不改变父类：

| 资产 | 保持的职责 |
| --- | --- |
| `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter` | 正式玩家与当前输入 / UI桥接 |
| `/Game/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy` | 正式近战敌人 |
| `/Game/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget` | 正式训练目标 |

每份执行：

1. 顶部点击 **Class Defaults**，Details搜索 `Initial Stamina`，确认 **100**；搜索 `Initial Max Stamina`，确认 **100**。若搜索无结果，清空搜索后展开 `EmberTrial → Startup`。保留Health / MaxHealth=100、阵营、能力Class / Montage等原配置。
2. Compile → Save，回Content Browser该资产缩略图右键 → **Asset Actions → Validate Assets**。预期日志 `[M1-07C2] Valid Asset=该资产 Issues=0`；ET-BODY-001现在同时保护体力子对象，ET-BODY-004检查体力启动范围。
3. 新子对象来自原生构造，不需要在Components添加。若StaminaAttributes引用None导致ET-BODY-001，停止处理该资产，发规则全文与资产路径，我核对原生默认对象 / 序列化；不要贸然Reparent、重复Add ASC或重建正式玩家蓝图。

不需要新建属性集蓝图、GE资产或体力Widget。本批三个Class Defaults都是100 / 100，无正式非法设置要恢复；负例在自动测试临时对象中进行。

本次Editor规则先检查三个正常资产；ET-BODY-004的静态错误分支未另做隔离资产负例。8.2的非法初值测试验证运行时初始化保护，不能冒充Editor负例已实测。后续有实际配置错误或重复成本时再增加对应Probe。

### 8.2 运行一项新自动测试（约1–3分钟）

打开 **Tools → Debug → Output Log**（或底部Output Log标签），确认不在PIE；在底部**命令输入框**输入：

```text
Automation RunTests EmberTrial.M2.Resources.StaminaFoundation
```

等待出现该路径的 `Test Completed. Result={Success}`。它是一项测试，多组断言不是多项测试。预期临时非法身体有两条 `Invalid startup stamina`，被测试框架登记为预期Warning；正式玩家不应报Missing gameplay dependencies。测试建立 / 结束自己的临时世界，不修改当前地图 / 正式资产；结束后编辑器继续可操作。

如果Failure：复制**从Test Started到Test Completed的对应段**或给完整Saved日志路径；我会读取断言和前后日志定位。不要为让测试变绿删除断言、增加Warning吞掉范围或改正式默认值。成功后无需连续重跑两次；旧M0自动化此次未修改，后续迁移闪避时再运行对应专项。

### 8.3 一条正式地图短基线（约2–5分钟）

1. 打开 `/Game/EmberTrial/Maps/L_ET_VerticalSlice`，正常Play，仍从Default Player Start出生。
2. Output Log搜索 `[M2-01A] Stamina initialized`：玩家、敌人、训练目标在各自BeginPlay应各一条 **Stamina=100.0 MaxStamina=100.0**，随后各有Ready=1。**100/100体力日志在Ready之前**，玩家HUD仍为生命100/100。现有日志的Character名字按实际实例为准。
3. 站定左键打训练目标一次，确认仍真实扣25；按2一次，确认原闪避正常；靠近练习目标按E看中文提示；按P打开再用一种已会的方式继续，确认恢复移动 / 转向 / 跳跃。不要让这一轮为了检查基础入口拖到敌人真实致死。
4. Stop后再Play一次，只看出生体力100 / 100与HUD100；停止，Save All。两个世界各自新实例，Stop有Ready=0清理，没有EndPlay后重新初始化或Accessed None。

这里不重新搭4/5/8/9图、不重复十次HUD重建或完整死亡矩阵。换控 / 重复初始化 / 数值夹限由8.2覆盖；未来正式体力成本、恢复节奏、UI显示再按新增功能检查。本轮无新Cook / Trace，Attempt02仍是M1历史版本，不能运行旧包来验证新体力代码。

## 9. 最简反馈与失败材料

```text
M2-01A 完成 / 卡在第__节第__步
两属性文件 / Character四处声明与实现 / Validator / 新测试已保存：
关闭UE后 Editor Build / 三模块索引 / Game Build：
三个正式身体初始体力100/100、Compile / Save / 本项目Valid：
StaminaFoundation一次：Success / Failure
首次体力日志在Ready前；玩家HUD生命100/100：
左键25 / 原2 / E / P与移动转向跳跃短基线：
第二次Play新身体100/100、Stop清理 / 无错误：
源码 / 资产已保存；失败或特别说明（无则填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

文字错误日志优先，Details名称 / 界面布局用截图；本批没有新增蓝图业务连线，无需给所有旧图截图。只报告结果和大致时分即可，我自行读取本机构建 / 运行日志；不要求精确记录按键时间。

## 10. 本批需要 Commit / Push（固定放在最后）

- 新增三份Source：`EmberTrial/Source/EmberTrial/Public/Attributes/ETStaminaAttributeSet.h`、`Private/Attributes/ETStaminaAttributeSet.cpp`、`Tests/ETStaminaFoundationTests.cpp`。
- 修改两份Character Source：`Public/Characters/EmberTrialCharacter.h`、`Private/Characters/EmberTrialCharacter.cpp`；修改既有 `EmberTrial/Source/EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp`。
- 三正式身体蓝图Compile / Save后有真实差异才纳入：`Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset`、`Content/EmberTrial/Characters/Enemies/BP_ETMeleeEnemy.uasset`、`Content/EmberTrial/Tests/Blueprints/BP_ETCombatTrainingTarget.uasset`，走既有Git LFS。它们已经登记来源，不新增不存在的体力UE资产。
- 本指南、M2路线与Progress / Backlog / Architecture / VersionControl / AcceptanceWorkflow_Options / Introduction实际更新；完成后我生成的验收报告 / 清单真实变化再追加。M1 / C3收尾若尚未提交，先按原清单纳入独立收尾提交。
- 本批没有计划改Build.cs / Target / uproject / 共享地图配置；看到意外Diff先保留并发给我核对。Binaries / Intermediate / Saved、生成CMake / IDE配置、测试报告Raw / Runtime、旧包 / Trace仍留本地，不Force Add。

CLion **Save All → 刷新Changes → 将最新完整Source / 合法资产与文档重新Stage → 检查Staged Diff → Commit → Push**。功能检查通过后可用 `feat: add stamina attributes and unified gameplay readiness`；当前只下发指南时可单独提交 `docs: prepare M2 stamina foundation`，不要把未实施代码写成已完成。Git操作由你执行。
