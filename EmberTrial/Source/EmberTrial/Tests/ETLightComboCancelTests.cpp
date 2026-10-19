#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGA_Dodge.h"
#include "Abilities/Tasks/ETAbilityTask_DodgeMove.h"
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
#include "GameFramework/RootMotionSource.h"
#include "GameplayAbilitySpec.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETLightComboCancelTest, "EmberTrial.M2.Combo.RecoveryDodge",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETLightComboCancelTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    // ---- 载入保存的正式GA；每个分支在独立世界中使用真实能力实例 ----
    UClass* AttackClass = LoadClass<UETGA_LightAttack>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_LightAttack.GA_ET_LightAttack_C"));
    UClass* DodgeClass = LoadClass<UETGA_Dodge>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_Dodge.GA_ET_Dodge_C"));
    UClass* HitClass = LoadClass<UETGA_HitReact>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_HitReact.GA_ET_HitReact_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved player GA"), AttackClass) ||
        !TestNotNull(TEXT("Saved dodge GA"), DodgeClass) ||
        !TestNotNull(TEXT("Hit GA"), HitClass) || !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_LightAttack* Defaults = AttackClass->GetDefaultObject<UETGA_LightAttack>();
    if (!TestTrue(TEXT("Player combo configured"),
                  Defaults->IsComboEnabled() && Defaults->HasValidComboConfiguration()) ||
        !TestTrue(TEXT("Saved recovery cancellation policy"),
                  DodgeClass->GetDefaultObject<UETGA_Dodge>()->HasValidRecoveryCancelPolicy()))
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
        Body->DodgeAbilityClass = DodgeClass;
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
        // 手部跟随只隔离三段姿势的几何差异，实际命中仍经Notify / Sweep / GE。
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
        FGameplayAbilitySpec* DodgeSpec = ASC->FindAbilitySpecFromHandle(Body->DodgeAbilityHandle);
        UETGA_Dodge* Dodge =
            DodgeSpec ? Cast<UETGA_Dodge>(DodgeSpec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Granted dodge instance"), Dodge))
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
        auto Reset = [&Candidates, TargetASC, Health, ASC, Stamina]()
        {
            Candidates.Reset();
            TargetASC->SetNumericAttributeBase(Health, 1000.0f);
            ASC->SetNumericAttributeBase(Stamina, 100.0f);
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
        auto Settle = [&Until, &Advance, Attack, ASC]()
        {
            return Until(
                       [Attack, ASC]()
                       {
                           return !Attack->IsActive() &&
                                  !ASC->HasMatchingGameplayTag(ETGameplayTags::State_Dodging) &&
                                  !ASC->HasMatchingGameplayTag(ETGameplayTags::State_HitReact);
                       },
                       3.0f) &&
                   Advance(.25f);
        };
        // ---- 等待真实段 / 恢复窗口；不把能力私有状态写成测试期望值 ----
        auto EnterRecovery = [&Start, &Until, &Position, Body, Attack](int32 Stage)
        {
            if (!Start())
            {
                return false;
            }
            for (int32 I = 0; I < Stage; ++I)
            {
                if (!Until([&Position]() { return Position() >= .48f; }, 1.0f) ||
                    !Body->RequestLightAttack() ||
                    !Until([Attack, I]() { return Attack->ActiveStageIndex == I + 1; }, .6f))
                {
                    return false;
                }
            }
            return Until([Attack]() { return Attack->CanCancelIntoDodge(); }, 1.0f);
        };
        // 交接须一次付费、结束旧链 / Trace，且新Dodge独占Montage与无敌窗口。
        auto DodgeNow = [this, Body, Attack, Dodge, ASC]()
        {
            const bool bRequested = Body->RequestDodge();
            TestTrue(TEXT("Recovery dodge accepted"), bRequested);
            TestFalse(TEXT("Attack really cancelled"), Attack->IsActive());
            TestFalse(TEXT("No old Attacking tag"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Attacking));
            TestFalse(TEXT("Old pending input cleared"),
                      Attack->bHasBufferedInput || Attack->bTransitionPending);
            TestEqual(TEXT("Old trace disarmed"), Attack->ActiveTraceSerial, uint32(0));
            TestTrue(TEXT("Dodge is sole montage owner"),
                     ASC->GetAnimatingAbility() == Dodge && Dodge->IsActive());
            TestEqual(TEXT("Exactly one cost"),
                      ASC->GetNumericAttribute(UETStaminaAttributeSet::GetStaminaAttribute()),
                      75.0f);
            TestTrue(TEXT("Own invulnerability begins"),
                     ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
            return bRequested && Dodge->IsActive();
        };
        // ---- EndPlay保留引擎RMS标记移除协议，不要求无后续Tick立即清空容器 ----
        if (bEndPlayOnly)
        {
            Reset();
            if (!EnterRecovery(0) || !DodgeNow() || !Advance(.05f))
            {
                return false;
            }
            const TSharedPtr<FRootMotionSource> OwnedSource =
                Body->GetCharacterMovement()->GetRootMotionSource(FName(TEXT("ETFormalDodge")));
            if (!TestTrue(TEXT("Actual handoff source before EndPlay"), OwnedSource.IsValid()))
            {
                return false;
            }
            if (!Fixture.EndPlayInTestWorld())
            {
                Fixture.ForwardErrorMessages(this);
                return false;
            }
            TestFalse(TEXT("EndPlay ends handoff dodge"), Dodge->IsActive() || Attack->IsActive());
            TestFalse(TEXT("EndPlay removes own protection"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
            // UE marks the owned source for removal; no movement tick is required after EndPlay.
            TestTrue(TEXT("EndPlay marks the owned source for removal"),
                     OwnedSource->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval));
            TestTrue(TEXT("EndPlay drops the movement task"), Dodge->MoveTask == nullptr);
            Trace->OnTraceHit.Remove(Watch);
            Fixture.ForwardErrorMessages(this);
            if (HasAnyErrors())
            {
                return false;
            }
            continue;
        }

        Reset();
        if (!Start() || !Advance(.10f))
        {
            return false;
        }
        // ---- 过早拒绝不缓存；原攻击身份、伤害与资源保持 ----
        const uint32 EarlySerial = Attack->ActiveTraceSerial;
        TestFalse(TEXT("Windup is not recovery"), Attack->CanCancelIntoDodge());
        TestFalse(TEXT("Early dodge rejected without buffering"), Body->RequestDodge());
        TestTrue(TEXT("Rejected request preserves attack"), Attack->IsActive());
        TestEqual(TEXT("Rejected request preserves serial"), Attack->ActiveTraceSerial,
                  EarlySerial);
        TestEqual(TEXT("Early refusal costs nothing"), ASC->GetNumericAttribute(Stamina), 100.0f);
        if (!Idle())
        {
            return false;
        }
        CheckDamage(1);
        TestFalse(TEXT("No delayed dodge after early refusal"), Dodge->IsActive());

        // 三段逐段交接；强持有旧Task后注入迟到结束，不能结束新能力。
        for (int32 Stage = 0; Stage < 3; ++Stage)
        {
            Reset();
            if (!EnterRecovery(Stage))
            {
                return false;
            }
            TStrongObjectPtr<UAbilityTask_PlayMontageAndWait> RetiredTask(
                Attack->MontageTask.Get());
            UAnimMontage* RetiredMontage = Attack->ActiveMontage;
            if (!TestNotNull(TEXT("Old task retained"), RetiredTask.Get()) || !DodgeNow())
            {
                return false;
            }
            RetiredTask->OnMontageBlendingOut(RetiredMontage, true);
            RetiredTask->OnMontageEnded(RetiredMontage, true);
            TestTrue(TEXT("Stale task cannot clear dodge owner"),
                     ASC->GetAnimatingAbility() == Dodge && Dodge->IsActive());
            if (!Settle())
            {
                return false;
            }
            CheckDamage(Stage + 1);
            TestFalse(TEXT("No protection after normal completion"),
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
            TestFalse(TEXT("No remaining movement source"),
                      Body->GetCharacterMovement()->HasRootMotionSources());
        }

        Reset();
        if (!EnterRecovery(0))
        {
            return false;
        }
        // ---- 预检失败必须保留原攻击，而不是先取消攻击再发现不可闪避 ----
        const uint32 PreservedSerial = Attack->ActiveTraceSerial;
        ASC->SetNumericAttributeBase(Stamina, 0.0f);
        TestFalse(TEXT("Unaffordable dodge refused"), Body->RequestDodge());
        TestTrue(TEXT("Insufficient stamina preserves attack"), Attack->IsActive());
        TestEqual(TEXT("Insufficient stamina preserves trace identity"), Attack->ActiveTraceSerial,
                  PreservedSerial);
        ASC->SetNumericAttributeBase(Stamina, 100.0f);
        UAnimMontage* SavedMontage = Dodge->DodgeMontage;
        Dodge->DodgeMontage = nullptr;
        TestFalse(TEXT("Missing dodge montage refused before cancellation"), Body->RequestDodge());
        Dodge->DodgeMontage = SavedMontage;
        const float SavedDuration = Dodge->DodgeDuration;
        Dodge->DodgeDuration = 0.0f;
        TestFalse(TEXT("Invalid movement refused before cancellation"), Body->RequestDodge());
        Dodge->DodgeDuration = SavedDuration;
        TestTrue(TEXT("Configuration refusal preserves original attack"), Attack->IsActive());
        TestEqual(TEXT("Configuration refusal costs nothing"), ASC->GetNumericAttribute(Stamina),
                  100.0f);
        if (!Idle())
        {
            return false;
        }
        CheckDamage(1);

        Reset();
        if (!EnterRecovery(0) || !Until([Attack]() { return Attack->bComboWindowOpen; }, 1.0f) ||
            !Body->RequestLightAttack())
        {
            return false;
        }
        // 待切段Timer也属于旧攻击；交接后不能继续推进连段。
        const FTimerHandle Queued = Attack->TransitionTimer;
        TestTrue(TEXT("Follow-up pending before dodge"), Attack->bTransitionPending);
        if (!DodgeNow())
        {
            return false;
        }
        TestFalse(TEXT("Handoff removes queued transition timer"),
                  World->GetTimerManager().IsTimerActive(Queued));
        if (!Settle())
        {
            return false;
        }
        CheckDamage(1);
        Reset();
        if (!Start() || !Idle())
        {
            return false;
        }
        CheckDamage(1);

        Reset();
        if (!EnterRecovery(0))
        {
            return false;
        }
        APlayerState* Pauser = World->SpawnActor<APlayerState>();
        if (!TestNotNull(TEXT("Pauser"), Pauser))
        {
            return false;
        }
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const float At = Position();
        const bool bPaused = Advance(.50f);
        TestFalse(TEXT("Paused recovery query refuses"), Attack->CanCancelIntoDodge());
        TestFalse(TEXT("Paused dodge request refused"), Body->RequestDodge());
        TestTrue(TEXT("Paused attack position frozen"),
                 FMath::IsNearlyEqual(At, Position(), .001f));
        TestEqual(TEXT("Paused request costs nothing"), ASC->GetNumericAttribute(Stamina), 100.0f);
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bPaused || !DodgeNow() || !Advance(.05f))
        {
            return false;
        }
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const FVector FrozenLocation = Body->GetActorLocation();
        const double FrozenTime = World->GetTimeSeconds();
        // 源时间 / 位移与世界时钟分开观测，原double时钟断言与暂停结果保留。
        const bool bDodgePaused = Advance(.50f);
        TestTrue(TEXT("Paused handoff displacement frozen"),
                 Body->GetActorLocation().Equals(FrozenLocation, .01f));
        TestEqual(TEXT("Paused handoff clock frozen"), World->GetTimeSeconds(), FrozenTime);
        TestTrue(TEXT("Paused own protection retained"),
                 ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        if (!bDodgePaused || !Settle())
        {
            return false;
        }
        CheckDamage(1);

        Reset();
        if (!EnterRecovery(0) || !DodgeNow() || !Advance(.05f))
        {
            return false;
        }
        if (!TestTrue(TEXT("Formal hit react cancels handoff dodge"), Body->RequestHitReact()))
        {
            return false;
        }
        TestFalse(TEXT("Hit ends dodge and own protection"),
                  Dodge->IsActive() ||
                      ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        TestFalse(TEXT("No dodge during hit reaction"), Body->RequestDodge());
        TestEqual(TEXT("Cancellation does not refund"), ASC->GetNumericAttribute(Stamina), 75.0f);
        if (!Advance(.05f))
        {
            return false;
        }
        TestFalse(TEXT("Hit removes handoff root source"),
                  Body->GetCharacterMovement()->HasRootMotionSources());
        if (!Settle())
        {
            return false;
        }
        CheckDamage(1);

        Reset();
        if (!EnterRecovery(0) || !DodgeNow() || !Advance(.05f))
        {
            return false;
        }
        ASC->SetNumericAttributeBase(Health, 0.0f);
        TestTrue(TEXT("Real death sealed"), Body->IsCombatDead());
        TestFalse(TEXT("Death ends both owners"), Dodge->IsActive() || Attack->IsActive());
        TestFalse(TEXT("Death removes own protection"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_Invulnerable));
        if (!Advance(.10f))
        {
            return false;
        }
        TestFalse(TEXT("Death releases movement source"),
                  Body->GetCharacterMovement()->HasRootMotionSources());
        TestFalse(TEXT("Dead body refuses dodge"), Body->RequestDodge());
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
    AddInfo(TEXT("[M2-03C] Recovery dodge: stages1/2/3, refusal preserves attack, queued follow-up "
                 "cleared, Pause/Hit/Death/EndPlay"));
    return !HasAnyErrors();
}
#endif
