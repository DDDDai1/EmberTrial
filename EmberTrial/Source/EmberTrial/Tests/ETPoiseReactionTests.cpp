#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_Stagger.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/Effects/ETPoiseDeltaEffect.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatPoiseComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "Misc/AutomationTest.h"
#include "Templates/SharedPointer.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETPoiseReactionTest, "EmberTrial.M2.Combat.PoiseReaction",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETPoiseReactionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* StaggerClass = LoadClass<UETGA_Stagger>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Stagger.GA_ET_Stagger_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved reaction GA"), StaggerClass) ||
        !TestNotNull(TEXT("Quinn"), Quinn) ||
        !TestTrue(TEXT("Configured GA"),
                  StaggerClass->GetDefaultObject<UETGA_Stagger>()->HasValidConfiguration()))
    {
        return false;
    }
    enum class EScenario : uint8
    {
        Cycle,
        ZeroBirth,
        LateObserve,
        Pause,
        AliveCancel,
        DeathQueued,
        DeathActive,
        EndQueued,
        EndActive,
        ExternalGate,
        OldGeneration
    };
    for (const EScenario Scenario :
         {EScenario::Cycle, EScenario::ZeroBirth, EScenario::LateObserve, EScenario::Pause,
          EScenario::AliveCancel, EScenario::DeathQueued, EScenario::DeathActive,
          EScenario::EndQueued, EScenario::EndActive, EScenario::ExternalGate,
          EScenario::OldGeneration})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = Fixture.GetTestWorld();
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>();
        if (!TestNotNull(TEXT("Body"), Body))
        {
            return false;
        }
        UETCombatPoiseComponent* Component = Body->FindComponentByClass<UETCombatPoiseComponent>();
        if (!TestNotNull(TEXT("CombatPoise"), Component))
        {
            return false;
        }
        Component->StaggerAbilityClass = StaggerClass;
        Body->InitialPoise = Scenario == EScenario::ZeroBirth ? 0.0f : 100.0f;
        Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Body->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Body->GetCharacterMovement()->GravityScale = 0.0f;
        if (!Fixture.BeginPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        int32 Starts = 0;
        const FDelegateHandle StartedHandle = ASC->AbilityActivatedCallbacks.AddLambda(
            [&Starts](UGameplayAbility* Ability)
            {
                if (Ability && Ability->IsA<UETGA_Stagger>())
                {
                    ++Starts;
                }
            });
        auto Advance = [&Fixture](float Seconds)
        {
            for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
            {
                if (!Fixture.TickTestWorld(.01f))
                {
                    return false;
                }
            }
            return true;
        };
        auto Apply = [ASC](float Delta)
        {
            FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
                UETPoiseDeltaEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
            if (!Spec.IsValid())
            {
                return false;
            }
            Spec.Data->SetSetByCallerMagnitude(ETGameplayTags::SetByCaller_PoiseDelta, Delta);
            return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).WasSuccessfullyApplied();
        };
        auto ExpectPoise = [this, ASC](const TCHAR* Label, float Expected)
        {
            TestTrue(FString::Printf(TEXT("%s Current"), Label),
                     FMath::IsNearlyEqual(
                         ASC->GetNumericAttribute(UETPoiseAttributeSet::GetPoiseAttribute()),
                         Expected, .001f));
            TestTrue(FString::Printf(TEXT("%s Base"), Label),
                     FMath::IsNearlyEqual(
                         ASC->GetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute()),
                         Expected, .001f));
        };
        if (Scenario == EScenario::LateObserve)
        {
            Component->Deactivate();
            TestTrue(TEXT("Real GE while not observed"), Apply(-100.0f));
            if (!Advance(.05f))
            {
                return false;
            }
            TestEqual(TEXT("No observer no reaction"), Starts, 0);
            Component->Activate(false);
        }
        else if (Scenario != EScenario::ZeroBirth)
        {
            TestTrue(TEXT("Real -60 GE"), Apply(-60.0f));
            ExpectPoise(TEXT("Partial reduction"), 40.0f);
            TestEqual(TEXT("Positive poise does not react"), Starts, 0);
            if (Scenario == EScenario::ExternalGate)
            {
                ASC->AddLooseGameplayTag(ETGameplayTags::State_Stunned);
            }
            TestTrue(TEXT("Over-depletion clamps through GE"), Apply(-60.0f));
        }
        ExpectPoise(TEXT("Zero before dispatch"), 0.0f);
        TestEqual(TEXT("No reaction inside GE dispatch"), Starts, 0);
        const FTimerHandle QueuedTimer = Component->EvaluationTimer;
        if (Scenario == EScenario::DeathQueued)
        {
            ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
        }
        if (Scenario == EScenario::EndQueued || Scenario == EScenario::EndActive)
        {
            if (Scenario == EScenario::EndActive && !Advance(.03f))
            {
                return false;
            }
            const int32 ExpectedReactions = Scenario == EScenario::EndActive ? 1 : 0;
            TestEqual(TEXT("Reaction count before Destroy"), Starts, ExpectedReactions);
            ExpectPoise(TEXT("EndPlay snapshot before Destroy"), 0.0f);

            // 销毁后ASC及其属性容器不再是可查询的数据源。
            // 只保留原生观察记录；委托按值持有它，不捕获局部变量引用或旧UObject。
            struct FEndPlayObservation
            {
                int32 PoiseChanges = 0;
                int32 NewReactions = 0;
                int32 OwnedReactionEnds = 0;
                int32 StunnedCount = 0;
            };
            const TSharedRef<FEndPlayObservation> Observation = MakeShared<FEndPlayObservation>();
            Observation->StunnedCount = ASC->GetTagCount(ETGameplayTags::State_Stunned);
            const FGameplayAbilitySpecHandle OwnedHandle = Component->StaggerHandle;
            const FTimerHandle PendingTimer = Component->EvaluationTimer;

            // 原计数器按引用捕获Starts，必须在销毁前解绑；后续记录拥有独立生命周期。
            ASC->AbilityActivatedCallbacks.Remove(StartedHandle);
            ASC->AbilityActivatedCallbacks.AddLambda(
                [Observation](UGameplayAbility* Ability)
                {
                    if (Ability && Ability->IsA<UETGA_Stagger>())
                    {
                        ++Observation->NewReactions;
                    }
                });
            ASC->GetGameplayAttributeValueChangeDelegate(UETPoiseAttributeSet::GetPoiseAttribute())
                .AddLambda(
                    [Observation](const FOnAttributeChangeData& Data)
                    {
                        (void)Data;
                        ++Observation->PoiseChanges;
                    });
            ASC->RegisterGameplayTagEvent(ETGameplayTags::State_Stunned,
                                          EGameplayTagEventType::NewOrRemoved)
                .AddLambda(
                    [Observation](FGameplayTag Tag, int32 Count)
                    {
                        (void)Tag;
                        Observation->StunnedCount = Count;
                    });
            ASC->OnAbilityEnded.AddLambda(
                [Observation, OwnedHandle](const FAbilityEndedData& Data)
                {
                    if (Data.AbilitySpecHandle == OwnedHandle)
                    {
                        ++Observation->OwnedReactionEnds;
                    }
                });

            TestTrue(TEXT("Body Destroy succeeds"), Body->Destroy());
            TestFalse(TEXT("EndPlay clears pending evaluation"),
                      World->GetTimerManager().TimerExists(PendingTimer));
            if (!Advance(1.5f))
            {
                return false;
            }
            // 此处不再调用ExpectPoise / ASC / Body / Component；它们可能已失效或被GC。
            TestEqual(TEXT("EndPlay never writes poise"), Observation->PoiseChanges, 0);
            TestEqual(TEXT("EndPlay does not add reactions"), Observation->NewReactions, 0);
            TestEqual(TEXT("Owned state removed on EndPlay"), Observation->StunnedCount, 0);
            TestEqual(TEXT("Only active owned reaction ends"), Observation->OwnedReactionEnds,
                      ExpectedReactions);
            TestFalse(TEXT("No delayed evaluation after Destroy"),
                      World->GetTimerManager().TimerExists(PendingTimer));
            continue;
        }
        if (Scenario == EScenario::OldGeneration)
        {
            const uint64 OldSerial = Component->ObservationSerial;
            Component->Deactivate();
            TestFalse(TEXT("Deactivate clears queued timer"),
                      World->GetTimerManager().TimerExists(QueuedTimer));
            Component->Activate(false);
            const FTimerHandle NewTimer = Component->EvaluationTimer;
            Component->Evaluate(OldSerial);
            TestTrue(TEXT("Stale callback preserves new evaluation"),
                     World->GetTimerManager().TimerExists(NewTimer));
        }
        if (!Advance(.03f))
        {
            return false;
        }
        if (Scenario == EScenario::DeathQueued)
        {
            TestEqual(TEXT("Same-frame death prevents reaction"), Starts, 0);
            ExpectPoise(TEXT("Death does not refill"), 0.0f);
            ASC->AbilityActivatedCallbacks.Remove(StartedHandle);
            continue;
        }
        if (Scenario == EScenario::ExternalGate)
        {
            TestEqual(TEXT("External state prevents reaction"), Starts, 0);
            if (!Advance(.1f))
            {
                return false;
            }
            TestFalse(TEXT("No repeating failure timer"), Component->EvaluationTimer.IsValid());
            ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Stunned);
            if (!Advance(.03f))
            {
                return false;
            }
        }
        TestEqual(TEXT("One reaction starts"), Starts, 1);
        TestTrue(TEXT("Owned Stunned is active"),
                 ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
        TestFalse(TEXT("Busy request cannot restart"), Body->RequestStagger());
        TestTrue(TEXT("Zero GE cannot produce a second reaction"), Apply(-1.0f));
        ExpectPoise(TEXT("During reaction remains zero"), 0.0f);
        if (Scenario == EScenario::DeathActive)
        {
            ASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
            if (!Advance(1.5f))
            {
                return false;
            }
            ExpectPoise(TEXT("Active death never restores"), 0.0f);
            TestEqual(TEXT("Death does not restart"), Starts, 1);
            TestTrue(TEXT("Death still sealed"), Body->IsCombatDead());
            TestFalse(TEXT("Dead state loses Stunned"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            ASC->AbilityActivatedCallbacks.Remove(StartedHandle);
            continue;
        }
        if (Scenario == EScenario::Pause)
        {
            APlayerState* Pauser = World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Pauser"), Pauser))
            {
                return false;
            }
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            if (!Advance(1.5f))
            {
                return false;
            }
            TestTrue(TEXT("Pause keeps reaction active"),
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            ExpectPoise(TEXT("Pause does not restore"), 0.0f);
            World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        }
        if (Scenario == EScenario::AliveCancel)
        {
            ASC->CancelAbilityHandle(Component->StaggerHandle);
        }
        else
        {
            if (!Advance(.8f))
            {
                return false;
            }
            TestTrue(TEXT("Short pose does not unlock state early"),
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
            // 回填读取结束当时的Max，不把出生100硬编码进运行逻辑。
            if (Scenario == EScenario::Cycle)
            {
                ASC->SetNumericAttributeBase(UETPoiseAttributeSet::GetMaxPoiseAttribute(), 80.0f);
            }
        }
        if (!Advance(1.0f))
        {
            return false;
        }
        ExpectPoise(TEXT("Restored to current maximum"),
                    Scenario == EScenario::Cycle ? 80.0f : 100.0f);
        TestFalse(TEXT("Reaction ended"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
        TestEqual(TEXT("Restore does not recurse"), Starts, 1);
        TestFalse(TEXT("No idle evaluation timer"), Component->EvaluationTimer.IsValid());
        TestTrue(
            TEXT("Old health unchanged"),
            FMath::IsNearlyEqual(
                ASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 100.0f));
        if (Scenario == EScenario::Cycle)
        {
            TestTrue(TEXT("Next independent cycle GE"), Apply(-80.0f));
            if (!Advance(.03f))
            {
                return false;
            }
            TestEqual(TEXT("Next independent reaction"), Starts, 2);
        }
        ASC->AbilityActivatedCallbacks.Remove(StartedHandle);
    }
    return true;
}
#endif