#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_HitReact.h"
#include "Abilities/ETGameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "UObject/StrongObjectPtr.h"
#include "Animation/ETANS_ComboWindow.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETStaminaAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Combat/ETCombatLifeComponent.h"
#include "Combat/ETCombatTraceComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameplayAbilitySpec.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETLightComboFlowTest, "EmberTrial.M2.Combo.Flow",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETLightComboFlowTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* AttackClass = LoadClass<UETGA_LightAttack>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* EnemyClass = LoadClass<UETGA_LightAttack>(
        nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_EnemyLightAttack.GA_ET_EnemyLightAttack_C"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved player GA"), AttackClass) ||
        !TestNotNull(TEXT("Saved enemy GA"), EnemyClass) ||
        !TestNotNull(TEXT("Hit GA"), HitClass) || !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_LightAttack* Defaults = AttackClass->GetDefaultObject<UETGA_LightAttack>();
    if (!TestTrue(TEXT("Player combo configured"),
                  Defaults->IsComboEnabled() && Defaults->HasValidComboConfiguration()) ||
        !TestFalse(TEXT("Enemy stays single stage"),
                   EnemyClass->GetDefaultObject<UETGA_LightAttack>()->IsComboEnabled()))
    {
        return false;
    }

    for (const bool bEndPlayOnly : {false, true})
    {
        FTestWorldWrapper Fixture;
        if (!Fixture.CreateTestWorld(EWorldType::Game))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        UWorld* World = Fixture.GetTestWorld();
        AActor* Floor = World->SpawnActor<AActor>();
        if (!TestNotNull(TEXT("Floor"), Floor))
        {
            return false;
        }
        UBoxComponent* Box = NewObject<UBoxComponent>(Floor);
        Floor->AddInstanceComponent(Box);
        Floor->SetRootComponent(Box);
        Box->SetMobility(EComponentMobility::Static);
        Box->SetBoxExtent(FVector(2000, 2000, 50));
        Box->SetRelativeLocation(FVector(0, 0, -50));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent();
        auto SpawnBody = [World, Quinn]()
        {
            FActorSpawnParameters P;
            P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
                FVector(0, 0, 200), FRotator::ZeroRotator, P);
            if (!Body)
            {
                return Body;
            }
            Body->GetMesh()->SetSkeletalMeshAsset(Quinn);
            Body->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
            Body->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
            Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Body->GetMesh()->VisibilityBasedAnimTickOption =
                EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
            Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
            Body->GetCharacterMovement()->bRunPhysicsWithNoController = true;
            return Body;
        };
        AEmberTrialCharacter* Body = SpawnBody();
        AEmberTrialCharacter* Target = SpawnBody();
        APlayerController* PC = World->SpawnActor<APlayerController>();
        if (!TestNotNull(TEXT("Player body"), Body) || !TestNotNull(TEXT("Target"), Target) ||
            !TestNotNull(TEXT("Controller"), PC))
        {
            return false;
        }
        Body->CombatTeam = EETCombatTeam::Player;
        Body->LightAttackAbilityClass = AttackClass;
        Body->CombatLifeComponent->HitReactAbilityClass = HitClass;
        Target->CombatTeam = EETCombatTeam::Neutral;
        Target->InitialMaxHealth = 1000;
        Target->InitialHealth = 1000;
        Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
        PC->Possess(Body);
        if (!Fixture.BeginPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        auto Step = [&Fixture, Body, Target]()
        {
            // Geometry fixture: stay in reach of each different right-hand pose.
            // This still uses real montage notifies, world sweeps and the production GE callback.
            const FVector Hand = Body->GetMesh()->GetSocketLocation(FName(TEXT("hand_r")));
            Target->SetActorLocation(FVector(Hand.X, Hand.Y, Target->GetActorLocation().Z), false,
                                     nullptr, ETeleportType::TeleportPhysics);
            return Fixture.TickTestWorld(.01f);
        };
        auto Advance = [&Step](float Seconds)
        {
            for (int32 I = 0; I < FMath::CeilToInt(Seconds / .01f); ++I)
            {
                if (!Step())
                {
                    return false;
                }
            }
            return true;
        };
        auto Until = [&Step](auto Predicate, float Limit)
        {
            for (int32 I = 0; I < FMath::CeilToInt(Limit / .01f) && !Predicate(); ++I)
            {
                if (!Step())
                {
                    return false;
                }
            }
            return Predicate();
        };
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        Target->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        if (!Advance(1.0f) || !TestTrue(TEXT("Ready grounded body"),
                                        Body->IsGameplayReady() && Target->IsGameplayReady() &&
                                            Body->GetCharacterMovement()->IsMovingOnGround()))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
        UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
        FGameplayAbilitySpec* Spec =
            ASC->FindAbilitySpecFromHandle(Body->GetLightAttackSpecHandle());
        UETGA_LightAttack* Attack =
            Spec ? Cast<UETGA_LightAttack>(Spec->GetPrimaryInstance()) : nullptr;
        UETCombatTraceComponent* Trace = Body->FindComponentByClass<UETCombatTraceComponent>();
        if (!TestNotNull(TEXT("Granted attack instance"), Attack) ||
            !TestNotNull(TEXT("Trace"), Trace))
        {
            return false;
        }
        const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Stamina = UETStaminaAttributeSet::GetStaminaAttribute();
        TMap<uint32, int32> Candidates;
        const FDelegateHandle Watch = Trace->OnTraceHit.AddLambda(
            [&Candidates, Target](uint32 Serial, const FHitResult& Hit)
            {
                if (Hit.GetActor() == Target)
                {
                    ++Candidates.FindOrAdd(Serial);
                }
            });
        auto Position = [Attack, Body]()
        {
            UAnimInstance* Anim = Body->GetMesh()->GetAnimInstance();
            FAnimMontageInstance* Instance =
                IsValid(Anim) ? Anim->GetMontageInstanceForID(Attack->ActiveMontageInstanceID)
                              : nullptr;
            return Instance ? Instance->GetPosition() : -1.0f;
        };
        auto Start = [this, Body, Attack]()
        {
            return TestTrue(TEXT("Start fresh attack"), Body->RequestLightAttack()) &&
                   TestTrue(TEXT("Starts stage one"), Attack->ActiveStageIndex == 0);
        };
        auto Idle = [&Until, &Advance, Attack]()
        { return Until([Attack]() { return !Attack->IsActive(); }, 2.0f) && Advance(.25f); };
        auto Reset = [&Candidates, TargetASC, Health]()
        {
            Candidates.Reset();
            TargetASC->SetNumericAttributeBase(Health, 1000.0f);
        };
        auto CheckDamage = [this, &Candidates, TargetASC, Health](int32 Count)
        {
            TestEqual(TEXT("Unique serial count"), Candidates.Num(), Count);
            for (const auto& Pair : Candidates)
            {
                TestEqual(TEXT("One candidate per serial"), Pair.Value, 1);
            }
            TestEqual(TEXT("Real current health"), TargetASC->GetNumericAttribute(Health),
                      1000.0f - 25.0f * Count);
            TestEqual(TEXT("Real base health"), TargetASC->GetNumericAttributeBase(Health),
                      1000.0f - 25.0f * Count);
        };
        if (bEndPlayOnly)
        {
            if (!Start() || !Until([Attack]() { return Attack->bComboWindowOpen; }, 1.0f) ||
                !Body->RequestLightAttack())
            {
                return false;
            }
            const FTimerHandle Pending = Attack->TransitionTimer;
            TestTrue(TEXT("Transition pending before EndPlay"), Attack->bTransitionPending);
            if (!Fixture.EndPlayInTestWorld())
            {
                Fixture.ForwardErrorMessages(this);
                return false;
            }
            TestFalse(TEXT("EndPlay cancels attack"), Attack->IsActive());
            TestFalse(TEXT("EndPlay clears pending input"),
                      Attack->bHasBufferedInput || Attack->bTransitionPending);
            TestFalse(TEXT("EndPlay clears owned timer"),
                      World->GetTimerManager().IsTimerActive(Pending));
            TestEqual(TEXT("EndPlay disarms trace"), Attack->ActiveTraceSerial, uint32(0));
            Fixture.ForwardErrorMessages(this);
            if (HasAnyErrors())
            {
                return false;
            }
            continue;
        }

        // Player stage context must reject both requests without buffering a retry.
        Reset();
        if (!Start() || !Until([&Position]() { return Position() >= .48f; }, 1.0f))
        {
            return false;
        }
        TestTrue(TEXT("Valid stage context accepts combo query"), Attack->CanAcceptComboInput());
        TestTrue(TEXT("Valid recovery accepts dodge query"), Attack->CanCancelIntoDodge());
        auto CheckContextRejected = [this, Attack](const TCHAR* Reason)
        {
            TestFalse(FString::Printf(TEXT("%s rejects combo input"), Reason),
                      Attack->TryBufferComboInput());
            TestFalse(FString::Printf(TEXT("%s rejects recovery dodge"), Reason),
                      Attack->CanCancelIntoDodge());
            TestFalse(FString::Printf(TEXT("%s leaves no buffered retry"), Reason),
                      Attack->bHasBufferedInput || Attack->bTransitionPending);
        };
        const int32 ContextMontageID = Attack->ActiveMontageInstanceID;
        Attack->ActiveMontageInstanceID = INDEX_NONE;
        CheckContextRejected(TEXT("Mismatched montage instance"));
        Attack->ActiveMontageInstanceID = ContextMontageID;

        PC->SetIgnoreMoveInput(true);
        CheckContextRejected(TEXT("Ignored player input"));
        PC->SetIgnoreMoveInput(false);

        APlayerState* ContextPauser = World->SpawnActor<APlayerState>();
        if (!TestNotNull(TEXT("Context pauser"), ContextPauser))
        {
            return false;
        }
        World->GetWorldSettings()->SetPauserPlayerState(ContextPauser);
        CheckContextRejected(TEXT("Paused stage context"));
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        TestTrue(TEXT("Restored context accepts combo query"), Attack->CanAcceptComboInput());
        TestTrue(TEXT("Restored recovery accepts dodge query"), Attack->CanCancelIntoDodge());
        if (!Idle())
        {
            return false;
        }
        CheckDamage(1);
        AddInfo(TEXT("Player stage context: montage identity, ignored input and pause refuse both "
                     "requests without buffering"));

        Reset();
        if (!Start() || !Idle())
        {
            return false;
        }
        CheckDamage(1);
        Reset();
        if (!Start() || !Advance(.05f) || !Body->RequestLightAttack() ||
            !Until([Attack]() { return Attack->bComboWindowOpen; }, 1.0f))
        {
            return false;
        }
        TestEqual(TEXT("Expired early click stays in first stage"), Attack->ActiveStageIndex, 0);
        TestFalse(TEXT("Expired input cleared"),
                  Attack->bHasBufferedInput || Attack->bTransitionPending);
        if (!Idle())
        {
            return false;
        }
        CheckDamage(1);

        Reset();
        if (!Start() || !Until([&Position]() { return Position() >= .48f; }, 1.0f))
        {
            return false;
        }
        const uint32 FirstTrace = Attack->ActiveTraceSerial;
        UAnimMontage* OldMontage = Attack->ActiveMontage;
        const int32 OldID = Attack->ActiveMontageInstanceID;
        UAnimNotifyState* OldToken = nullptr;
        for (const FAnimNotifyEvent& Event : OldMontage->Notifies)
        {
            if (Event.NotifyStateClass && Event.NotifyStateClass->IsA<UETANS_ComboWindow>())
            {
                OldToken = Event.NotifyStateClass;
            }
        }
        if (!Body->RequestLightAttack() ||
            !Until([Attack]() { return Attack->ActiveStageIndex == 1; }, .6f))
        {
            return false;
        }
        TestTrue(TEXT("Stage two keeps Attacking"),
                 ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
        TestTrue(TEXT("New serial for stage two"), Attack->ActiveTraceSerial != FirstTrace);
        const uint32 SecondTrace = Attack->ActiveTraceSerial;
        Attack->HandleComboWindow(Body->GetMesh(), OldMontage, OldID, OldToken, false);
        Trace->EndAttack(FirstTrace); // Deliberately stale cleanup must not disarm stage two.
        TestEqual(TEXT("Stale combo end ignored"), Attack->ActiveTraceSerial, SecondTrace);
        if (!Until([&Position]() { return Position() >= .48f; }, 1.0f) ||
            !Body->RequestLightAttack() ||
            !Until([Attack]() { return Attack->ActiveStageIndex == 2; }, .6f))
        {
            return false;
        }
        TestTrue(TEXT("New serial for stage three"), Attack->ActiveTraceSerial != SecondTrace);
        TestFalse(TEXT("No fourth stage input"), Body->RequestLightAttack());
        if (!Idle())
        {
            return false;
        }
        CheckDamage(3);
        TestEqual(TEXT("Light combo does not spend stamina"), ASC->GetNumericAttribute(Stamina),
                  100.0f);
        AddInfo(TEXT("[M2-03B] Real combo 1->2->3: three unique traces, Health1000->925"));

        // Retain a finished task and deliver stale native callbacks after a fresh activation.
        // Negative injection only: positive damage still comes from real notifies and sweeps.
        Reset();
        if (!Start() || !Advance(.05f))
        {
            return false;
        }
        TStrongObjectPtr<UAbilityTask_PlayMontageAndWait> HeldOldTask(Attack->MontageTask.Get());
        UAnimMontage* RetiredMontage = Attack->ActiveMontage;
        if (!TestNotNull(TEXT("Retained old task"), HeldOldTask.Get()))
        {
            return false;
        }
        Body->CancelLightAttack();
        TestFalse(TEXT("Old activation cancelled"), Attack->IsActive());
        TestTrue(TEXT("Finished task drops old ability owner"), HeldOldTask->Ability == nullptr);
        if (!Advance(.25f) || !Start())
        {
            return false;
        }
        const uint32 FreshTrace = Attack->ActiveTraceSerial;
        HeldOldTask->OnMontageBlendingOut(RetiredMontage, true);
        HeldOldTask->OnMontageEnded(RetiredMontage, true);
        TestTrue(TEXT("Stale task keeps new activation alive"), Attack->IsActive());
        TestTrue(TEXT("Stale task cannot clear new ASC ownership"),
                 ASC->GetAnimatingAbility() == Attack);
        TestTrue(TEXT("New activation reuses first montage"),
                 ASC->GetCurrentMontage() == RetiredMontage);
        TestEqual(TEXT("Stale task leaves fresh trace intact"), Attack->ActiveTraceSerial,
                  FreshTrace);
        TestEqual(TEXT("Fresh activation stays in stage one"), Attack->ActiveStageIndex, 0);
        if (!Idle())
        {
            return false;
        }
        CheckDamage(1);

        Reset();
        if (!Start() || !Until([&Position]() { return Position() >= .48f; }, 1.0f))
        {
            return false;
        }
        for (int32 I = 0; I < 5; ++I)
        {
            TestTrue(TEXT("Burst updates one slot"), Body->RequestLightAttack());
        }
        if (!Until([Attack]() { return Attack->ActiveStageIndex == 1; }, .6f))
        {
            return false;
        }
        TestFalse(TEXT("Consumed slot empty"), Attack->bHasBufferedInput);
        if (!Idle())
        {
            return false;
        }
        CheckDamage(2);

        Reset();
        if (!Start() || !Until([Attack]() { return Attack->bComboWindowOpen; }, 1.0f) ||
            !Body->RequestLightAttack())
        {
            return false;
        }
        TestTrue(TEXT("Queued transition is deferred"), Attack->bTransitionPending);
        const FTimerHandle CancelTimer = Attack->TransitionTimer;
        if (!TestTrue(TEXT("Formal hit reaction cancels queued combo"), Body->RequestHitReact()))
        {
            return false;
        }
        TestFalse(TEXT("Cancel clears action and input"),
                  Attack->IsActive() || Attack->bHasBufferedInput || Attack->bTransitionPending);
        TestFalse(TEXT("Cancel removes transition timer"),
                  World->GetTimerManager().IsTimerActive(CancelTimer));
        if (!Advance(.15f))
        {
            return false;
        }
        TestTrue(TEXT("Old montage cannot end new hit reaction"),
                 ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact));
        if (!Until([ASC]() { return !ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact); },
                   1.5f) ||
            !Advance(.25f))
        {
            return false;
        }
        CheckDamage(1);

        Reset();
        if (!Start() || !Until([&Position]() { return Position() >= .48f; }, 1.0f) ||
            !Body->RequestLightAttack())
        {
            return false;
        }
        APlayerState* Pauser = World->SpawnActor<APlayerState>();
        if (!TestNotNull(TEXT("Pauser"), Pauser))
        {
            return false;
        }
        const float At = Position();
        const float WorldAt = World->GetTimeSeconds();
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const bool bPauseAdvance = Advance(.50f);
        TestTrue(TEXT("Pause freezes montage"), FMath::IsNearlyEqual(Position(), At, .001f));
        TestTrue(TEXT("Pause freezes buffer clock"),
                 FMath::IsNearlyEqual(World->GetTimeSeconds(), WorldAt, .001f));
        TestFalse(TEXT("Paused input rejected"), Body->RequestLightAttack());
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bPauseAdvance || !Until([Attack]() { return Attack->ActiveStageIndex == 1; }, .6f) ||
            !Idle())
        {
            return false;
        }
        CheckDamage(2); // Only the valid pre-pause input survives; menu input adds no third stage.

        UAnimMontage* Saved = Attack->ComboFollowUps[0].Montage;
        Attack->ComboFollowUps[0].Montage = nullptr;
        TestFalse(TEXT("Missing follow-up config rejected"), Attack->HasValidComboConfiguration());
        TestFalse(TEXT("Invalid combo cannot start"), Body->RequestLightAttack());
        Attack->ComboFollowUps[0].Montage = Saved;
        TestTrue(TEXT("Restored config"), Attack->HasValidComboConfiguration());

        Reset();
        if (!Start() || !Until([Attack]() { return Attack->bComboWindowOpen; }, 1.0f) ||
            !Body->RequestLightAttack())
        {
            return false;
        }
        const FTimerHandle DeathTimer = Attack->TransitionTimer;
        ASC->SetNumericAttributeBase(Health, 0.0f);
        TestTrue(TEXT("Death sealed"), Body->IsCombatDead());
        TestFalse(TEXT("Death clears attack and pending input"),
                  Attack->IsActive() || Attack->bHasBufferedInput || Attack->bTransitionPending);
        TestFalse(TEXT("Death removes transition timer"),
                  World->GetTimerManager().IsTimerActive(DeathTimer));
        if (!Advance(.5f))
        {
            return false;
        }
        TestFalse(TEXT("Dead body cannot restart"), Body->RequestLightAttack());
        Trace->OnTraceHit.Remove(Watch);
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        Fixture.ForwardErrorMessages(this);
        if (HasAnyErrors())
        {
            return false;
        }
    }
    return !HasAnyErrors();
}
#endif