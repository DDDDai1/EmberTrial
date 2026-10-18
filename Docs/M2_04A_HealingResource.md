# M2-04A：有限治疗次数与成功生效事务

2026-10-18清理同步：本页保留历史接入 / 配置 / 验收经验，相关整文件或完整方法以当前Source链接为准，不覆盖已完成工程。本次格式、入口审计与验证见[Cleanup](Cleanup.md)。

2026-10-17，固定 **UE5.5.4**。**本批已按用户19:17反馈与源码 / 构建 / 自动化 / 实机日志核对验收通过，无待补项。**实际105Source / 56cpp / 来源58，五新 / 五旧与指南一致，三Valid与Healing.Resource Success。手动60 / 等待5，累计2451 / 明确等待240分钟；见[A验收报告](Reports/M2_04A_Verification.md)。以下步骤保留供复查，无需重复执行；下一[B1治疗动画指南](M2_04B1_HealingAnimation.md)已下发，从第2节开始；原A下发时将动作 / Q / 次数HUD留给后续；当前正式动作 / Q已完成，次数HUD仍未完成。

原下发预算手动 **100–150分钟**（非当前待办），含理解与录入、组件配置、5–10分钟相关检查和记录；构建等待另记。可分两次做：先录第3–7节，第二次录测试并Build / 检查。治疗整章8h基础预算与批次边界见[治疗路线](M2_04_HealingRoadmap.md)。

## 当前PIE命令与正式Q边界

ET_Healing长期保留为PIE检查入口，调用同一资源服务；0只读，非Editor实现为空。当前[Controller代码](../EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp)将0–1000幅度作为内部诊断保护常量，保留全部World / 身体 / 数值和Idle门控，不改变正式GA默认40或次数3/3。正式Q仍在唯一Notify处提交，开发命令不代替动画验证。

## 1. 本批结果、职责与规则

本批先得到“真实生命GE成功一次 → 治疗次数减一”，生命HUD直接响应原属性事件。通过永久保留、仅PIE有效的原生开发命令检查，不搭临时蓝图图表。原A阶段尚无动作 / Q；当前正式Q已通过GA / Notify接入同一服务，次数HUD仍未完成，不另写扣次数逻辑。

| 条件 | 结果 |
| --- | --- |
| 玩家新身体 | Initial Charges=3、Max Charges=3，一次初始化 |
| 非满血、存活、未暂停、有次数、Amount合法 | 原生Instant GE将Health Add(Base)正向增加，成功返回后次数减1 |
| 默认开发治疗40；25血治疗一次 | 25→65，3→2；生命Current / Base一致，原生命HUD更新 |
| 80血治疗40 | 夹到100，仍消费1次；不把溢出存为隐藏Base |
| 满血 / 无次数 / 死亡 / 暂停 / 未Ready / 敌人 / 非法值 | 拒绝，生命与次数不变，无次数变化通知 |
| GAS拒绝应用GE | 不扣次数；Instant成功判据是WasSuccessfullyApplied，不是Handle.IsValid |
| 同一身体重复Ensure / Possess / 组件Deactivate后Activate | 次数保留；失活期间快照不可用，重新启用不补满 |
| Retry / 新试炼 / 当前会话Continue | 当前实现重建世界 / 身体，新身体恢复3；不是保存旧身体的次数 |
| 成功治疗引起同步回调再次请求治疗 | 回调期间拒绝重入，避免重复治疗 / 扣次数；成功后回调失活不退款 |

资源属于角色的`UETHealingResourceComponent`，是离散整数，不放进连续AttributeSet。生命变化仍走GAS；上限夹限与死亡封锁仍由原Health AttributeSet负责。组件无Tick、无Timer，不依赖UMG / UI模块。只对Player阵营启用次数，敌人 / 训练目标继承同一原生子对象但保持休眠，不输出错误。

**GameplayReady继续表示原Health / Stamina初始化可用。**治疗另有资源快照可用性，不将治疗配置错误扩大成整个身体无法攻击 / 闪避。当前正式GA同时检查这两项可用性。组件只负责资源与生效；动作互斥 / 地面 / 动画窗口留给GA，不能在资源服务里硬性要求Idle（生效时GA本来就处于Healing状态）。本批开发入口单独要求Idle。

本机5.5.4已核对：`GameplayAbilities/Public/ActiveGameplayEffectHandle.h`的WasSuccessfullyApplied、`Private/AbilitySystemComponent.cpp`的ApplyGameplayEffectSpecToSelf与应用查询、原Health夹限及Ready顺序。固定原生GE只有一个正向Health Modifier，没有Exec / 概率 / 可替换蓝图效果。本服务针对这份固定契约；不声称可以回滚任意外部回调对世界的修改。连续两次独立成功调用会各消费一次；当前每个治疗动作只允许一个有效Notify提交，由已完成的B2能力负责。

## 2. 准备与文件路径

1. Stop PIE → Save All → 关闭UE → CLion保存。先完成第3–8节所有文件，再第9节正常Build；不要用Live Coding处理本批UCLASS / 子对象 / Exec变化。
2. 在CLion当前完整工程中创建普通C++文件即可，不需要从UE菜单创建空类。注意扩展名和下方Public / Private位置。已有Resources为复数，Effects在Abilities下，避免重现目录拼写错误。
3. 四个新类文件和一个非UCLASS测试共**五新文件**；另有**五旧文件**精确插入。旧函数 / friend / 属性保留；不替换整份Character或Controller。

```text
E:\UE_Project\Demo\EmberTrial\Source\
├─ EmberTrial\Public\Resources\ETHealingResourceComponent.h             新，第3.1节全文
├─ EmberTrial\Private\Resources\ETHealingResourceComponent.cpp            新，第3.2节全文
├─ EmberTrial\Public\Abilities\Effects\ETHealingEffect.h                 新，第4.1节全文
├─ EmberTrial\Private\Abilities\Effects\ETHealingEffect.cpp               新，第4.2节全文
├─ EmberTrial\Public\Characters\EmberTrialCharacter.h                     旧，第5.1节三处
├─ EmberTrial\Private\Characters\EmberTrialCharacter.cpp                  旧，第5.2节两处
├─ EmberTrial\Public\Framework\ETPlayerController.h                       旧，第6.1节一处
├─ EmberTrial\Private\Framework\ETPlayerController.cpp                    旧，第6.2节include与末尾函数
├─ EmberTrialEditor\Private\Validation\ETGameplayAssetValidator.cpp       旧，第7节三处
└─ EmberTrial\Tests\ETHealingResourceTests.cpp                             新，第8节全文
```

不改Build.cs / Target / uproject、原Health / Stamina集、攻击 / 闪避 / 输入资产、UI模块、地图。四个新头 / 源文件的文件名 / 类名必须一一对应；两个头的`.generated.h`均是最后一条include，不能手建generated文件。测试放**Tests**，不是Texts。

## 3. 治疗次数资源组件

### 3.1 ETHealingResourceComponent.h：完整内容

当前完整实现见[ETHealingResourceComponent.h](../EmberTrial/Source/EmberTrial/Public/Resources/ETHealingResourceComponent.h)。已完成首批排版与注释清理，当前Source为唯一维护版本；以下保留装配步骤和设计说明。

### 3.2 ETHealingResourceComponent.cpp：完整内容

当前完整实现见[ETHealingResourceComponent.cpp](../EmberTrial/Source/EmberTrial/Private/Resources/ETHealingResourceComponent.cpp)。已完成首批排版与注释清理，当前Source为唯一维护版本；以下保留装配步骤和设计说明。

`bInitialized`与当前次数只属于这一身体；Activate(true)也不能重新初始化。`bApplying`从应用GE前持续到次数广播之后，保护Health和次数两个同步回调入口。失活不清当前次数，也不撤销已经成功的治疗；EndPlay使快照不可用、解绑Ready和次数监听，不让退出后重新Ready。后续Presenter需要“订阅事件后立即读快照”，不能靠等待下一次扣次数获取出生值。

## 4. 固定原生治疗GE

### 4.1 ETHealingEffect.h：完整内容

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ETHealingEffect.generated.h"

UCLASS()
class EMBERTRIAL_API UETHealingEffect : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UETHealingEffect(const FObjectInitializer& ObjectInitializer);
};
```

### 4.2 ETHealingEffect.cpp：完整内容

```cpp
#include "Abilities/Effects/ETHealingEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETHealthAttributeSet.h"

UETHealingEffect::UETHealingEffect(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FSetByCallerFloat Magnitude;
    Magnitude.DataTag = ETGameplayTags::SetByCaller_HealthDelta;
    FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = UETHealthAttributeSet::GetHealthAttribute();
    Modifier.ModifierOp = EGameplayModOp::AddBase;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
}
```

这里是原生类，不在Content Browser再建GE蓝图，不新增SetByCaller Tag。复用已有HealthDelta的正数表达治疗；原攻击继续传负数，伤害阵营 / 无敌判定不改。正式治疗不能通过SetNumericAttributeBase直接改血或由UI先改数字。

## 5. Character：只装配单个资源组件

### 5.1 EmberTrialCharacter.h：三处精确修改

第一处，前置声明区：

**找到原内容：**

```cpp
class UETStaminaRecoveryComponent;
```

**替换为：**

```cpp
class UETStaminaRecoveryComponent;
class UETHealingResourceComponent;
```

第二处，原StaminaRecoveryComponent属性的末行（原属性的UPROPERTY保持）：

**找到原内容：**

```cpp
TObjectPtr<UETStaminaRecoveryComponent> StaminaRecoveryComponent;
```

**替换为：**

```cpp
TObjectPtr<UETStaminaRecoveryComponent> StaminaRecoveryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EmberTrial|Healing",
        meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UETHealingResourceComponent> HealingResourceComponent;
```

第三处，既有测试friend末尾（所有旧friend保留）：

**找到原内容：**

```cpp
friend class FETLightComboCancelTest;
```

**替换为：**

```cpp
friend class FETLightComboCancelTest;
    friend class FETHealingResourceTest;
```

### 5.2 EmberTrialCharacter.cpp：两处精确修改

第一处，include区：

**找到原内容：**

```cpp
#include "Resources/ETStaminaRecoveryComponent.h"
```

**替换为：**

```cpp
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Resources/ETHealingResourceComponent.h"
```

第二处，构造函数已有StaminaRecovery创建行：

**找到原内容：**

```cpp
StaminaRecoveryComponent = CreateDefaultSubobject<UETStaminaRecoveryComponent>(TEXT("StaminaRecovery"));
```

**替换为：**

```cpp
StaminaRecoveryComponent = CreateDefaultSubobject<UETStaminaRecoveryComponent>(TEXT("StaminaRecovery"));
    HealingResourceComponent = CreateDefaultSubobject<UETHealingResourceComponent>(TEXT("HealingResource"));
```

不要在BeginPlay / Possess再次NewObject或Add Component，不改EnsureGameplayInitialized / SetGameplayReady，不给敌人另写次数初始化。Native默认子对象沿继承链只创建一个，名称`HealingResource`固定；以后在蓝图不要手动Add同类组件。

## 6. 复用原生PIE开发入口

### 6.1 ETPlayerController.h

在public区既有ET_Stamina声明后插入新Exec：

**找到原内容：**

```cpp
void ET_Stamina(float Delta);
```

**替换为：**

```cpp
void ET_Stamina(float Delta);

    UFUNCTION(Exec, meta = (DevelopmentOnly))
    void ET_Healing(float Amount);
```

### 6.2 ETPlayerController.cpp

第一处，include区：

**找到原内容：**

```cpp
#include "Resources/ETStaminaRecoveryComponent.h"
```

**替换为：**

```cpp
#include "Resources/ETStaminaRecoveryComponent.h"
#include "Resources/ETHealingResourceComponent.h"
```

ET_Healing与ET_Stamina是两个独立成员，各自保留Editor条件编译；当前实现见源码入口，不重复追加历史版本：

当前源码：[ETPlayerController.cpp](../EmberTrial/Source/EmberTrial/Private/Framework/ETPlayerController.cpp)。

`Amount=0`只查询，暂停时也可查询。大于0表示尝试这次固定量的治疗；未空闲 / 暂停 / 满血等失败只输出Applied=0，不缓存命令。函数在非Editor构建中无修改行为，正式Q输入走GA，诊断入口不作为成品输入。

## 7. Editor校验器：保留已有全部规则

文件路径是第2节的**EmberTrialEditor/Private/Validation/ETGameplayAssetValidator.cpp**，不是Runtime。

第一处，include区：

**找到原内容：**

```cpp
#include "Characters/EmberTrialCharacter.h"
```

**替换为：**

```cpp
#include "Characters/EmberTrialCharacter.h"
#include "Resources/ETHealingResourceComponent.h"
```

第二处，ValidateBody开头的组件名称列表中，替换这一行，前后名称保留：

**找到原内容：**

```cpp
TEXT("StaminaAttributes"), TEXT("StaminaRecoveryComponent"),
```

**替换为：**

```cpp
TEXT("StaminaAttributes"), TEXT("StaminaRecoveryComponent"), TEXT("HealingResourceComponent"),
```

第三处，ValidateBody的`if (bPlayer)`内部，找到唯一DodgeAbilityClass检查行，替换成：

**找到原内容：**

```cpp
        RequireObject(Blueprint, Defaults, TEXT("DodgeAbilityClass"), TEXT("ET-DODGE-001"), Issues);
```

**替换为：**

```cpp
        const FObjectPropertyBase* HealingProperty = FindFProperty<FObjectPropertyBase>(
            Defaults->GetClass(), TEXT("HealingResourceComponent"));
        const UETHealingResourceComponent* Healing = HealingProperty
            ? Cast<UETHealingResourceComponent>(HealingProperty->GetObjectPropertyValue_InContainer(Defaults))
            : nullptr;
        if (IsValid(Healing) && !Healing->HasValidConfiguration())
        {
            AddIssue(Blueprint, TEXT("ET-HEAL-001"),
                TEXT("Healing requires AutoActivate, MaxCharges 1..9 and InitialCharges 0..MaxCharges."), Issues);
        }

        RequireObject(Blueprint, Defaults, TEXT("DodgeAbilityClass"), TEXT("ET-DODGE-001"), Issues);
```

ET-BODY-001覆盖缺少原生组件；ET-HEAL-001只检查正式玩家的次数配置。敌人 / 训练目标不要求具有可用治疗能力。暂不创建故意配置错误的Probe资产；非法次数由下面自动化覆盖，节省临时资产和回滚操作。

## 8. 长期自动化：ETHealingResourceTests.cpp全文

这是普通.cpp，没有UCLASS / generated头文件。用临时世界与原生角色创建真实ASC / AttributeSet、调用固定原生治疗GE；SetNumericAttributeBase只用于安排测试前置血量，正向治疗必须调用正式服务。

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Resources/ETHealingResourceComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETHealingResourceTest,
    "EmberTrial.M2.Healing.Resource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETHealingResourceTest::RunTest(const FString& Parameters)
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
    auto MakeBody = [&](EETCombatTeam Team, float Health)
    {
        AEmberTrialCharacter* NewBody = World->SpawnActor<AEmberTrialCharacter>(
            FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        if (NewBody)
        {
            NewBody->CombatTeam = Team;
            NewBody->InitialHealth = Health;
        }
        return NewBody;
    };
    AEmberTrialCharacter* Body = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Empty = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Invalid = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Enemy = MakeBody(EETCombatTeam::Enemy, 20.0f);
    AEmberTrialCharacter* Dead = MakeBody(EETCombatTeam::Player, 0.0f);
    AEmberTrialCharacter* Reentrant = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Deactivated = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Fresh = MakeBody(EETCombatTeam::Player, 20.0f);
    AEmberTrialCharacter* Bodies[] = {Body, Empty, Invalid, Enemy, Dead, Reentrant, Deactivated, Fresh};
    for (AEmberTrialCharacter* Subject : Bodies)
    {
        if (!TestNotNull(TEXT("Spawned body"), Subject) ||
            !TestNotNull(TEXT("Native healing component"), Subject->HealingResourceComponent.Get()))
        {
            return false;
        }
    }
    UETHealingResourceComponent* Healing = Body->HealingResourceComponent.Get();
    Empty->HealingResourceComponent->InitialCharges = 0;
    Invalid->HealingResourceComponent->InitialCharges = 4;
    int32 Charges = -1;
    int32 MaxCharges = -1;
    TestFalse(TEXT("Before BeginPlay unavailable"), Healing->GetHealingSnapshot(Charges, MaxCharges));
    AddExpectedMessagePlain(TEXT("Invalid healing settings"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 1);
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Main ASC"), ASC) ||
        !TestTrue(TEXT("Main body Ready"), Body->IsGameplayReady()))
    {
        return false;
    }
    const FGameplayAttribute HealthAttribute = UETHealthAttributeSet::GetHealthAttribute();
    auto ExpectCharges = [&](UETHealingResourceComponent* Resource, const TCHAR* Label,
        bool bExpectedAvailable, int32 ExpectedCharges, int32 ExpectedMax = 3)
    {
        int32 Current = -1;
        int32 Maximum = -1;
        TestEqual(FString::Printf(TEXT("%s available"), Label),
            Resource->GetHealingSnapshot(Current, Maximum), bExpectedAvailable);
        TestEqual(FString::Printf(TEXT("%s charges"), Label), Current, bExpectedAvailable ? ExpectedCharges : 0);
        TestEqual(FString::Printf(TEXT("%s maximum"), Label), Maximum, bExpectedAvailable ? ExpectedMax : 0);
    };
    auto ExpectHealth = [&](const TCHAR* Label, float Expected)
    {
        TestEqual(FString::Printf(TEXT("%s Current"), Label), ASC->GetNumericAttribute(HealthAttribute), Expected);
        TestEqual(FString::Printf(TEXT("%s Base"), Label), ASC->GetNumericAttributeBase(HealthAttribute), Expected);
    };
    ExpectCharges(Healing, TEXT("Initial"), true, 3);
    ExpectCharges(Empty->HealingResourceComponent.Get(), TEXT("Zero initial valid"), true, 0);
    ExpectCharges(Invalid->HealingResourceComponent.Get(), TEXT("Invalid settings"), false, 0);
    TestTrue(TEXT("Healing config does not revoke shared Health/Stamina Ready"), Invalid->IsGameplayReady());
    ExpectCharges(Enemy->HealingResourceComponent.Get(), TEXT("Enemy dormant"), false, 0);
    TestFalse(TEXT("Empty rejects"), Empty->HealingResourceComponent->TryApplyHealing(40.0f));
    TestFalse(TEXT("Dead rejects"), Dead->HealingResourceComponent->TryApplyHealing(40.0f));
    TestFalse(TEXT("Enemy rejects"), Enemy->HealingResourceComponent->TryApplyHealing(40.0f));

    int32 ChargeEvents = 0;
    const FDelegateHandle CountHandle = Healing->OnSnapshotChanged.AddLambda([&]() { ++ChargeEvents; });
    for (const float Amount : {0.0f, -1.0f, 1001.0f, std::numeric_limits<float>::quiet_NaN()})
    {
        TestFalse(TEXT("Invalid amount rejects"), Healing->TryApplyHealing(Amount));
    }
    TestEqual(TEXT("Rejects do not publish charge changes"), ChargeEvents, 0);
    ASC->GameplayEffectApplicationQueries.Add(FGameplayEffectApplicationQuery::CreateLambda(
        [](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&) { return false; }));
    TestFalse(TEXT("ASC rejected GE does not spend"), Healing->TryApplyHealing(40.0f));
    ASC->GameplayEffectApplicationQueries.Reset();
    ExpectCharges(Healing, TEXT("Rejected GE preserves charges"), true, 3);
    ExpectHealth(TEXT("Rejected GE preserves Health"), 20.0f);
    TestEqual(TEXT("Rejected GE no notification"), ChargeEvents, 0);

    TestTrue(TEXT("Native Instant heal succeeds"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("20 plus 40"), 60.0f);
    ExpectCharges(Healing, TEXT("One success one charge"), true, 2);
    TestEqual(TEXT("One change notification"), ChargeEvents, 1);
    TestTrue(TEXT("Repeat Ensure succeeds"), Body->EnsureGameplayInitialized());
    Healing->Deactivate();
    ExpectCharges(Healing, TEXT("Deactivated unavailable"), false, 0);
    Healing->Activate(true);
    ExpectCharges(Healing, TEXT("Reactivation preserves count"), true, 2);

    AAIController* Controller = World->SpawnActor<AAIController>();
    if (!TestNotNull(TEXT("Temporary controller"), Controller)) { return false; }
    Controller->Possess(Body);
    Controller->UnPossess();
    Controller->Possess(Body);
    ExpectCharges(Healing, TEXT("Same body Possess preserves count"), true, 2);
    Healing->OnSnapshotChanged.Remove(CountHandle);

    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pauser"), Pauser)) { return false; }
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestFalse(TEXT("Paused rejects mutation"), Healing->TryApplyHealing(40.0f));
    ExpectCharges(Healing, TEXT("Paused query works"), true, 2);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    TestTrue(TEXT("Second success reaches full"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("Full"), 100.0f);
    TestFalse(TEXT("Full Health rejects without spending"), Healing->TryApplyHealing(40.0f));
    ExpectCharges(Healing, TEXT("Full rejection"), true, 1);
    ASC->SetNumericAttributeBase(HealthAttribute, 80.0f); // Test setup, not the healing implementation.
    TestTrue(TEXT("Partial effective heal still consumes one"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("Overheal clamps Base and Current"), 100.0f);
    ExpectCharges(Healing, TEXT("Exhausted"), true, 0);
    ASC->SetNumericAttributeBase(HealthAttribute, 20.0f);
    TestFalse(TEXT("Exhausted missing Health rejects"), Healing->TryApplyHealing(40.0f));
    ExpectHealth(TEXT("No charge no heal"), 20.0f);
    ExpectCharges(Fresh->HealingResourceComponent.Get(), TEXT("Another body independent"), true, 3);

    UETHealingResourceComponent* NestedHealing = Reentrant->HealingResourceComponent.Get();
    UAbilitySystemComponent* NestedASC = Reentrant->GetAbilitySystemComponent();
    int32 RecursiveAttempts = 0;
    bool bRecursiveAccepted = false;
    const FDelegateHandle HealthHandle = NestedASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute)
        .AddLambda([&](const FOnAttributeChangeData& Change)
        {
            if (Change.NewValue > Change.OldValue)
            {
                ++RecursiveAttempts;
                bRecursiveAccepted |= NestedHealing->TryApplyHealing(40.0f);
            }
        });
    const FDelegateHandle SnapshotHandle = NestedHealing->OnSnapshotChanged.AddLambda([&]()
    {
        ++RecursiveAttempts;
        bRecursiveAccepted |= NestedHealing->TryApplyHealing(40.0f);
    });
    TestTrue(TEXT("Outer healing succeeds"), NestedHealing->TryApplyHealing(40.0f));
    TestEqual(TEXT("Health and charge callbacks each attempted once"), RecursiveAttempts, 2);
    TestFalse(TEXT("Recursive heal rejected in both callbacks"), bRecursiveAccepted);
    TestEqual(TEXT("Recursive callbacks do not double heal"), NestedASC->GetNumericAttribute(HealthAttribute), 60.0f);
    ExpectCharges(NestedHealing, TEXT("No recursive double spend"), true, 2);
    NestedASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).Remove(HealthHandle);
    NestedHealing->OnSnapshotChanged.Remove(SnapshotHandle);

    UETHealingResourceComponent* DisabledHealing = Deactivated->HealingResourceComponent.Get();
    UAbilitySystemComponent* DisabledASC = Deactivated->GetAbilitySystemComponent();
    const FDelegateHandle DisableHandle = DisabledASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute)
        .AddLambda([&](const FOnAttributeChangeData& Change)
        {
            if (Change.NewValue > Change.OldValue) { DisabledHealing->Deactivate(); }
        });
    TestTrue(TEXT("Heal remains successful if callback deactivates resource"), DisabledHealing->TryApplyHealing(40.0f));
    DisabledASC->GetGameplayAttributeValueChangeDelegate(HealthAttribute).Remove(DisableHandle);
    ExpectCharges(DisabledHealing, TEXT("Disabled query unavailable"), false, 0);
    DisabledHealing->Activate();
    ExpectCharges(DisabledHealing, TEXT("Successful heal was not refunded"), true, 2);

    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    ExpectCharges(Healing, TEXT("EndPlay unavailable"), false, 0);
    TestFalse(TEXT("EndPlay cannot heal"), Healing->TryApplyHealing(40.0f));
    TestFalse(TEXT("EndPlay ready binding removed"),
        Body->OnGameplayReadyChanged.IsAlreadyBound(Healing,
            &UETHealingResourceComponent::HandleGameplayReadyChanged));
    TestFalse(TEXT("EndPlay count listeners cleared"), Healing->OnSnapshotChanged.IsBound());
    AddInfo(TEXT("[M2-04A] Healing.Resource: native GE / one charge / rejection / snapshot / possession / reentrancy / EndPlay checked."));
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
```

覆盖一次初始化、0次数合法 / 非法配置隔离、满血 / 死亡 / 敌人 / 金额拒绝、真实GE被ASC拒绝、Current与Base夹限、一次成功一次扣数、Pause、同身体控制切换 / 失活恢复、两个同步事件重入保护与EndPlay解绑。不需要手动制造这些故障，不把原生测试当作动画Notify / Q键或次数UI已经完成。新身体初值 / 身份隔离由独立身体覆盖，实际世界Retry再做一次短查。

## 9. 正常Build与CLion索引

保存第3–8节**十份文件**，UE保持关闭。在PowerShell逐项执行，前一项失败先处理第一条真实错误：

```powershell
Set-Location 'E:\UE_Project\Demo'
.\Tools\Refresh-CLion.ps1
```

Editor与索引成功后再执行Game：

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrial Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

CLion → Reload CMake Project。三个CodeModel保留，新资源.cpp / 新GE.cpp / 新测试.cpp属于EmberTrialCodeModel，Validator仍属Editor。预计全项目56cpp（Core40 / UI13 / Editor3）。Ctrl+B可跳到新头；不以CodeModel CMake Build代替UBT，不删缓存或清空Intermediate。

本指南仅做候选与本机API静态核对，没有替你编译。若UHT报generated顺序 / 找不到类型，先核对路径、文件名与头末尾；若链接报声明无实现，核对新cpp已保存并加入实际Source，不只创建空骨架。保留第一条完整编译错误，不只发UBT exit6尾部。

## 10. Blueprint配置（无需新增节点）

1. 从原EmberTrial.uproject正常启动编辑器，打开正式玩家`/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`，左上Components找到**继承的HealingResource**。
2. 只选这一个组件，Details清空搜索，展开**EmberTrial → Healing**，或分别搜索`Initial Charges` / `Max Charges`；设Initial=3、Max=3。在Activation区域确认Auto Activate勾选。这里3是次数，默认每次回复40由当前命令参数 / 后续GA决定，不在组件填40。
3. Components中同类只一份，带继承标记；不要Add Component。如果看不到继承组件，先确认正常Editor Build成功并关闭重开了编辑器，不尝试用蓝图补一份。
4. 玩家原Health100/100、Stamina100/100、单ASC / Recovery / Trace / Life、Attack / Dodge类、原IA与Practice保持。Compile → Save。
5. 打开正式敌人`BP_ETMeleeEnemy`与训练目标`BP_ETCombatTrainingTarget`，确认各继承一份HealingResource，保留默认3/3 / AutoActivate，不接治疗业务，Compile → Save。三身体原父类、阵营Player / Enemy / Neutral保持。
6. Content Browser逐个右键这三个正式身体 → Asset Actions → Validate Assets；需看到本项目`Valid / Issues=0`。ET-HEAL-001先核对玩家组件3/3与AutoActivate，不删除校验规则。已有测试身体 / Waiting样例不用逐个打开或重存。
7. 回`/Game/EmberTrial/Maps/L_ET_VerticalSlice`，Save All。没有新GE / GA / IA / WBP / Notify资产，也没有需要删除的临时图表。

## 11. 分组检查（相关手动约5–10分钟）

### 11.1 新资源自动化，约1–2分钟

不Play，在Output Log执行：

```text
Automation RunTests EmberTrial.M2.Healing.Resource
```

等待Healing.Resource的**Success**。测试内一条`Invalid healing settings` Warning是登记过的负向用例，其余首次失败保留Expected与时分 / 日志路径。我读取日志分析，不要求你修改测试配置凑通过。

只运行这一项新测试；本批没有改旧Ready、体力恢复、攻击 / 闪避实现，不重复Combo / Dodge / Lifecycle或旧HUD换身体矩阵。若新测试或短基线揭示公共组件影响，再按实际问题扩相关回归。

### 11.2 正式玩家真实治疗与满血拒绝，约2–3分钟

**准备：**正式地图Play，在远离敌人的Entry空地站定。原开发4保持Damage25，玩家出生100/100；使用原Output Log命令栏，不创建额外按键。

1. 先查询：

```text
ke BP_ETPlayerController_C ET_Healing 0
```

预期一条`[M2-04A] Debug ... Available=1 Amount=0.0 Applied=0 Health=100.0->100.0/100.0 Charges=3/3`。出生应仅当前玩家有`Healing initialized ... 3/3`；敌人 / 训练目标不出现初始化或Invalid配置日志。HUD此时仍只显示已有生命 / 体力。

2. 满血尝试：

```text
ke BP_ETPlayerController_C ET_Healing 40
```

预期Applied=0、100→100、3/3，无Healing applied。**这不是失效**，满血应拒绝。

3. 关闭控制台 / 点回游戏，按既有4一次让玩家100→75；等受击结束、站定。再执行同一条`ET_Healing 40`。预期Applied=1、75→100、Charges=2/3，同时一条Healing applied，原生命HUD同步100；过量部分被夹掉。此阶段只有开发命令生效，无治疗动画正常。
4. 再执行一次`ET_Healing 0`，预期仍2/3。若命令栏没显示相关日志，清空Output Log搜索 / 类别过滤；确认正在正式地图的PIE，不是菜单或未Play。`ke`后的名称是**Controller类**，不是玩家Character或Widget。

### 11.3 Pause、耗尽与新身体，约2–4分钟

**操作A：**再按4一次到75，等受击结束，P暂停。通过Output Log依次执行`ET_Healing 0`及`ET_Healing 40`（完整前缀仍`ke BP_ETPlayerController_C`）。

**预期A：**查询Available=1 / 2/3 / Paused=1；修改Applied=0，75不变、2/3不变。以一种已有方式继续，不自动治疗；再次主动执行40才75→100 / 1/3。

**操作B：**再4一次，等空闲，执行40到100 / 0/3；再4到75，执行40。

**预期B：**次数0但身体仍Ready，后一次Applied=0 / Health75→75 / Charges0/3，原攻击、闪避和移动不因没治疗次数全锁。非法量 / 重入 / 同身体Possess由11.1负责，不再搭临时测试图。

**操作C：**用既有4完成真实死亡，Failure → Retry。新身体一次查询0，预期Health100、Charges3/3、单份原HUD。随后左键一次（目标25）、2一次（成本25 / 方向闪避）、E一次、P一种继续方式、移动转向跳跃短基线，Stop再开一次确认新100 / 3与结束清理即可。

**恢复：**slomo1、正式身体初始双100、Healing3/3 / AutoActivate、4仍Damage25、Debug False，Source保存 / 资产Save All，回正式地图。没有临时场景 / 输入 / GE配置需要清理；当前不Cook或采Insights。命令日志只需保留原Saved/Logs，报大致时分即可，不必逐条复制。

## 12. 完成反馈与报错材料

```text
M2-04A 完成 / 卡在第__节第__步
五新 / 五旧Source；关闭UE后Editor Build / 索引56cpp / Game Build：
三正式身体单继承HealingResource、3/3 / AutoActivate；Compile / Save / Valid：
Healing.Resource一次：
出生3/3；满血拒绝；4伤害后40真实治疗、夹限 / 次数2/3与原生命HUD：
P内查询 / 拒绝；继续不补治疗；耗尽0拒绝、其他动作仍正常：
死亡Retry新100 / 3/3、单HUD；Stop重开与短基线：
源码 / 资产Save All；最终默认值恢复：
失败或特别说明（无填无）：
完成时分（如21:18）/ 手动用时 / 构建等待：
```

Build错误用**文字**贴第一条真实编译 / UHT / 链接错误及前后数行；自动化给测试名 / 第一条Expected。Details / 组件 / Valid问题截图包含资产名、Components列表和所选属性；行为问题说明“当前血量 / 次数 → 做什么 → 实际 / 期望”，必要时短视频。日志给路径 / 大致时分，我自行提取。不需要复制整段思考、重建旧验收图或清缓存；明确让我直接修复源码时我再执行，否则继续给手动补修方法。

## 13. 本批 Commit / Push（固定放在最后）

当前Stage [B1指南](M2_04B1_HealingAnimation.md)、[数值JSON](Data/M2_04B1_HealPoseValues.json)、治疗路线 / 进度 / 任务板 / 架构 / 验收 / 版本管理 / Introduction交接与自动进入下一批约定；A验收报告 / 来源三个身体说明等未提交收尾可合并。Source主体已在685a0f3，本轮没有新B1源码或资产。B1实物完成后按[指南末尾](M2_04B1_HealingAnimation.md#9-本批-commit--push固定放在最后)提交三新LFS资产、核实后的来源61项与报告 / 状态。Raw / 日志 / 生成文件 / 历史包本地；Stage / Commit / Push由用户完成，助手未联网核验远端。
