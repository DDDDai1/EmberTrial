#if WITH_DEV_AUTOMATION_TESTS

#include "Resources/ETStaminaRecoveryComponent.h"
#include "Abilities/Effects/ETStaminaDeltaEffect.h"
#include "Abilities/ETGameplayTags.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "GameplayEffect.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>
#include "Characters/EmberTrialCharacter.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETStaminaRecoveryTest, "EmberTrial.M2.Resources.StaminaRecovery",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETStaminaRecoveryTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game))
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>();
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Temporary body"), Body) || !TestNotNull(TEXT("Pauser"), Pauser))
    {
        return false;
    }
    if (!TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    Body->GetCharacterMovement()->SetComponentTickEnabled(false);
    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    UETStaminaRecoveryComponent* Recovery =
        Body->FindComponentByClass<UETStaminaRecoveryComponent>();
    if (!TestNotNull(TEXT("ASC"), ASC) || !TestNotNull(TEXT("Recovery"), Recovery))
    {
        return false;
    }
    auto Advance = [&](int32 Steps)
    {
        for (int32 Index = 0; Index < Steps; ++Index)
        {
            if (!TestWorld.TickTestWorld(0.05f))
            {
                TestWorld.ForwardErrorMessages(this);
                return false;
            }
        }
        return true;
    };
    const FGameplayAttribute Attribute = UETStaminaAttributeSet::GetStaminaAttribute();
    auto Value = [&]() { return ASC->GetNumericAttribute(Attribute); };
    auto ApplyDelta = [&](float Delta)
    {
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UETStaminaDeltaEffect::StaticClass(),
                                                               1.0f, ASC->MakeEffectContext());
        if (!Spec.IsValid())
        {
            AddError(TEXT("Stamina delta spec unavailable"));
            return;
        }
        Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_StaminaDelta, Delta);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    };
    auto Same = [&](const TCHAR* Label, float Expected)
    { TestTrue(Label, FMath::IsNearlyEqual(Value(), Expected, 0.001f)); };
    TestFalse(TEXT("Full has no timer"), Recovery->IsRecoveryScheduled());
    TestFalse(TEXT("No component Tick capability"), Recovery->PrimaryComponentTick.bCanEverTick);
    TestFalse(TEXT("No registered component Tick"),
              Recovery->PrimaryComponentTick.IsTickFunctionRegistered());
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
    TestTrue(TEXT("Deficit schedules timer"), Recovery->IsRecoveryScheduled());
    if (!Advance(14))
    {
        return false;
    } // 0.70 seconds, before delay.
    Same(TEXT("Delay prevents early recovery"), 75.0f);
    if (!Advance(4))
    {
        return false;
    } // 0.90 seconds total, with sampling margin.
    TestTrue(TEXT("Recovery starts after delay"), Value() > 75.0f && Value() <= 79.0f);

    const float BeforePause = Value();
    const double TimeBeforePause = World->GetTimeSeconds();
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    const bool bPausedTicksOK = Advance(20);
    World->GetWorldSettings()->SetPauserPlayerState(nullptr); // Always restore before exit.
    if (!bPausedTicksOK)
    {
        return false;
    }
    Same(TEXT("Pause freezes resource"), BeforePause);
    TestTrue(TEXT("Pause freezes world clock"),
             FMath::IsNearlyEqual(World->GetTimeSeconds(), TimeBeforePause, 0.001));
    if (!Advance(4))
    {
        return false;
    }
    TestTrue(TEXT("Resume recovers without paused catch-up"),
             Value() > BeforePause && Value() <= BeforePause + 6.2f);

    ApplyDelta(-10.0f);
    if (!Advance(6))
    {
        return false;
    }
    ApplyDelta(-5.0f);
    const float AfterSecondSpend = Value();
    if (!Advance(14))
    {
        return false;
    }
    Same(TEXT("Second spend resets delay"), AfterSecondSpend);
    if (!Advance(4))
    {
        return false;
    }
    TestTrue(TEXT("Recovery resumes after reset delay"), Value() > AfterSecondSpend);
    // 独立维护测试侧的完整订阅契约；不能由被测Handle数量反推预期值。
    const FGameplayTag ExpectedActionTags[] = {ETGameplayTags::State_Attacking,
                                               ETGameplayTags::State_HitReact,
                                               ETGameplayTags::State_Dead,
                                               ETGameplayTags::State_Dodging,
                                               ETGameplayTags::State_Healing,
                                               ETGameplayTags::State_Stunned,
                                               ETGameplayTags::State_CastingBreakPulse};
    const int32 ExpectedActionTagCount = static_cast<int32>(UE_ARRAY_COUNT(ExpectedActionTags));
    // 可退出的动作都应立即停表；Dead由末尾真实Health归零覆盖。
    for (const FGameplayTag Tag : ExpectedActionTags)
    {
        if (Tag == ETGameplayTags::State_Dead)
        {
            continue;
        }
        ASC->AddLooseGameplayTag(Tag);
        TestFalse(TEXT("Busy tag stops timer immediately"), Recovery->IsRecoveryScheduled());
        const float BusyValue = Value();
        if (!Advance(20))
        {
            return false;
        }
        Same(TEXT("Busy freezes recovery"), BusyValue);
        ASC->RemoveLooseGameplayTag(Tag);
        if (!Advance(4))
        {
            return false;
        }
        TestTrue(TEXT("Busy end does not pay busy duration"),
                 Value() > BusyValue && Value() <= BusyValue + 6.2f);
    }
    ApplyDelta(1000.0f);
    Same(TEXT("Recovery / restore caps at maximum"), 100.0f);
    TestFalse(TEXT("Full stops timer"), Recovery->IsRecoveryScheduled());

    ASC->SetNumericAttributeBase(UETStaminaAttributeSet::GetMaxStaminaAttribute(), 120.0f);
    Same(TEXT("Maximum increase itself does not refill"), 100.0f);
    TestTrue(TEXT("New deficit wakes recovery"), Recovery->IsRecoveryScheduled());
    if (!Advance(4))
    {
        return false;
    }
    TestTrue(TEXT("New capacity can recover later"), Value() > 100.0f && Value() <= 106.2f);
    ApplyDelta(1000.0f);
    Recovery->RefreshObservation();
    Recovery->RefreshObservation();
    const uint64 RevisionBeforeSpend = Recovery->DecreaseRevision;
    ApplyDelta(-5.0f);
    TestEqual(TEXT("Reobserve does not duplicate spend callback"), Recovery->DecreaseRevision,
              RevisionBeforeSpend + 1);
    // 独立列出订阅契约，直接检查ASC委托，避免只凭本地Handle数组推断已解绑。
    const auto CheckActionTagBindings = [&](bool bExpected)
    {
        for (const FGameplayTag Tag : ExpectedActionTags)
        {
            TestEqual(FString::Printf(TEXT("Recovery subscription: %s"), *Tag.ToString()),
                      ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
                          .IsBoundToObject(Recovery),
                      bExpected);
        }
    };
    TestEqual(TEXT("One handle per action tag after repeated observation"),
              Recovery->ActionTagHandles.Num(), ExpectedActionTagCount);
    CheckActionTagBindings(true);
    TestTrue(TEXT("Rate subscription survives reobserve"),
             ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus).IsBoundToObject(Recovery));
    const uint64 OldSerial = Recovery->ObservationSerial;
    Recovery->Deactivate();
    TestFalse(TEXT("Deactivate clears timer"), Recovery->IsRecoveryScheduled());
    TestTrue(TEXT("Deactivate clears action handles"), Recovery->ActionTagHandles.IsEmpty());
    TestFalse(TEXT("Deactivate clears rate handle"), Recovery->RecoveryBonusHandle.IsValid());
    TestFalse(TEXT("Deactivate removes rate subscription"),
              ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus).IsBoundToObject(Recovery));
    CheckActionTagBindings(false);
    ApplyDelta(-5.0f);
    const float DisabledValue = Value();
    if (!Advance(20))
    {
        return false;
    }
    Same(TEXT("Inactive component does not recover"), DisabledValue);
    Recovery->Activate(true);
    TestEqual(TEXT("Reactivation restores exactly one handle per action tag"),
              Recovery->ActionTagHandles.Num(), ExpectedActionTagCount);
    CheckActionTagBindings(true);
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
    StaleChange.Attribute = Attribute;
    StaleChange.OldValue = 100.0f;
    StaleChange.NewValue = 50.0f;
    Recovery->HandleAttributeChanged(StaleChange, TWeakObjectPtr<UAbilitySystemComponent>(ASC),
                                     OldSerial);
    TestEqual(TEXT("Old observation cannot reset new deadline"), Recovery->RecoveryNotBefore,
              NewDeadline);

    Recovery->RecoveryRate = 0.0f;
    AddExpectedMessagePlain(TEXT("Invalid recovery settings"), ELogVerbosity::Warning,
                            EAutomationExpectedMessageFlags::Contains, 1);
    Recovery->RefreshObservation();
    TestFalse(TEXT("Invalid rate leaves no timer"), Recovery->IsRecoveryScheduled());
    Recovery->RecoveryRate = 20.0f;
    Recovery->RefreshObservation();
    if (!Advance(18))
    {
        return false;
    }
    TestTrue(TEXT("Valid settings recover again"), Value() > DisabledValue);
    TestEqual(TEXT("Stamina effect leaves Health alone"),
              ASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 100.0f);

    ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestTrue(TEXT("Actual zero Health commits death"), Body->IsCombatDead());
    TestFalse(TEXT("Death stops timer immediately"), Recovery->IsRecoveryScheduled());
    const float AtDeath = Value();
    if (!Advance(20))
    {
        return false;
    }
    Same(TEXT("Dead body does not recover"), AtDeath);
    if (!TestWorld.EndPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this);
        return false;
    }
    TestFalse(TEXT("EndPlay leaves no timer"), Recovery->IsRecoveryScheduled());
    TestFalse(TEXT("EndPlay revokes Ready"), Body->IsGameplayReady());
    TestTrue(TEXT("EndPlay clears action handles"), Recovery->ActionTagHandles.IsEmpty());
    CheckActionTagBindings(false);
    TestFalse(TEXT("EndPlay clears rate handle"), Recovery->RecoveryBonusHandle.IsValid());
    TestFalse(TEXT("EndPlay removes rate subscription"),
              ASC->GetGameplayAttributeValueChangeDelegate(RecoveryBonus).IsBoundToObject(Recovery));
    AddInfo(TEXT("Stamina recovery: additive rate / GE removal / invalid restart, "
                 "spend delay, all action blockers and balanced subscriptions"));
    TestWorld.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
