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