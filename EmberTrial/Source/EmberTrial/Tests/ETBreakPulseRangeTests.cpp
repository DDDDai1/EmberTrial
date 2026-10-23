#if WITH_DEV_AUTOMATION_TESTS
#include "Abilities/ETGA_BreakPulse.h"
#include "Abilities/ETGameplayTags.h"
#include "Animation/ETAN_BreakPulseCommit.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETPoiseAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETBreakPulseRangeTest, "EmberTrial.M2.Combat.BreakPulseRange",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETBreakPulseRangeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* PulseClass = LoadClass<UETGA_BreakPulse>(
        nullptr, TEXT("/Game/EmberTrial/Abilities/GA_ET_BreakPulse.GA_ET_BreakPulse_C"));
    UClass* TargetClass = LoadClass<AEmberTrialCharacter>(
        nullptr, TEXT("/Game/EmberTrial/Tests/Blueprints/"
                      "BP_ETCombatTrainingTarget.BP_ETCombatTrainingTarget_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(
        nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved pulse"), PulseClass) ||
        !TestNotNull(TEXT("Saved native training body Blueprint"), TargetClass) ||
        !TestNotNull(TEXT("Quinn"), Quinn))
    {
        return false;
    }
    const UETGA_BreakPulse* Defaults = PulseClass->GetDefaultObject<UETGA_BreakPulse>();
    if (!TestTrue(TEXT("Saved one-notify config"), Defaults->HasValidConfiguration()) ||
        !TestEqual(TEXT("Saved radius"), Defaults->PulseRadius, 300.0f) ||
        !TestEqual(TEXT("Saved Poise damage"), Defaults->PoiseDamage, 60.0f))
    {
        return false;
    }
    TestTrue(TEXT("Formal asset commit point remains .4 seconds"),
             FMath::IsNearlyEqual(Defaults->CastMontage->Notifies[0].GetTime(), .4f, .001f));
    enum class EScenario : uint8
    {
        Cycle,
        Edge,
        Wall,
        Team,
        Dead,
        Waiting,
        Invulnerable,
        Stunned,
        EnterRange,
        LeaveRange,
        CancelBefore,
        Pause,
        ReentrantCancel,
        EndPlay
    };
    for (const EScenario Scenario :
         {EScenario::Cycle, EScenario::Edge, EScenario::Wall, EScenario::Team, EScenario::Dead,
          EScenario::Waiting, EScenario::Invulnerable, EScenario::Stunned, EScenario::EnterRange,
          EScenario::LeaveRange, EScenario::CancelBefore, EScenario::Pause,
          EScenario::ReentrantCancel, EScenario::EndPlay})
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
        UBoxComponent* FloorBox = NewObject<UBoxComponent>(Floor);
        Floor->SetRootComponent(FloorBox);
        FloorBox->SetBoxExtent(FVector(2000, 2000, 20));
        FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FloorBox->SetCollisionObjectType(ECC_WorldStatic);
        FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
        FloorBox->RegisterComponent();
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEmberTrialCharacter* Source = World->SpawnActor<AEmberTrialCharacter>(
            FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        AEmberTrialCharacter* A = World->SpawnActor<AEmberTrialCharacter>(
            TargetClass, FVector(180, 0, 200), FRotator::ZeroRotator, Spawn);
        AEmberTrialCharacter* B = World->SpawnActor<AEmberTrialCharacter>(
            TargetClass, FVector(0, 220, 200), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Source"), Source) || !TestNotNull(TEXT("PC"), PC) ||
            !TestNotNull(TEXT("A"), A) || !TestNotNull(TEXT("B"), B))
        {
            return false;
        }
        Source->CombatTeam = EETCombatTeam::Player;
        Source->BreakPulseAbilityClass = PulseClass;
        Source->GetMesh()->SetSkeletalMeshAsset(Quinn);
        Source->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
        Source->GetMesh()->SetRelativeLocation(FVector(0, 0, -90));
        Source->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Source->GetMesh()->VisibilityBasedAnimTickOption =
            EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        for (AEmberTrialCharacter* Body : {Source, A, B})
        {
            Body->AutoPossessAI = EAutoPossessAI::Disabled;
            Body->GetCharacterMovement()->bRunPhysicsWithNoController = true;
            Body->GetMesh()->VisibilityBasedAnimTickOption =
                EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        }
        PC->Possess(Source);
        if (!Fixture.BeginPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
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
        for (AEmberTrialCharacter* Body : {Source, A, B})
        {
            Body->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        }
        if (!Advance(1.0f) ||
            !TestTrue(TEXT("Source grounded"), Source->GetCharacterMovement()->IsMovingOnGround()))
        {
            return false;
        }
        UAbilitySystemComponent* ASC = Source->GetAbilitySystemComponent();
        UAbilitySystemComponent* ASC_A = A->GetAbilitySystemComponent();
        UAbilitySystemComponent* ASC_B = B->GetAbilitySystemComponent();
        if (!TestTrue(TEXT("Formal target A ready"), A->IsGameplayReady()) ||
            !TestTrue(TEXT("Formal target B ready"), B->IsGameplayReady()))
        {
            return false;
        }
        const FGameplayAbilitySpecHandle Handle = Source->BreakPulseAbilityHandle;
        FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
        UETGA_BreakPulse* Ability =
            Spec ? Cast<UETGA_BreakPulse>(Spec->GetPrimaryInstance()) : nullptr;
        if (!TestNotNull(TEXT("Pulse instance"), Ability))
        {
            return false;
        }
        auto Poise = [](UAbilitySystemComponent* Comp)
        { return Comp->GetNumericAttributeBase(UETPoiseAttributeSet::GetPoiseAttribute()); };
        auto Place = [Source](AEmberTrialCharacter* Target, const FVector& Offset)
        {
            Target->GetCharacterMovement()->StopMovementImmediately();
            Target->SetActorLocation(Source->GetActorLocation() + Offset, false);
        };
        // 除Cycle / 重入两组外，B移出范围，避免用例互相掩盖。
        if (Scenario != EScenario::Cycle && Scenario != EScenario::ReentrantCancel)
        {
            Place(B, FVector(700, 500, 0));
        }
        if (Scenario == EScenario::Edge)
        {
            Place(A, FVector(315, 0, 0));
        }
        if (Scenario == EScenario::EnterRange)
        {
            Place(A, FVector(400, 0, 0));
        }
        if (Scenario == EScenario::Team)
        {
            A->CombatTeam = EETCombatTeam::Player;
        }
        if (Scenario == EScenario::Dead)
        {
            ASC_A->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
        }
        if (Scenario == EScenario::Waiting)
        {
            A->SetGameplayReady(false);
        }
        if (Scenario == EScenario::Invulnerable)
        {
            ASC_A->AddLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (Scenario == EScenario::Stunned)
        {
            ASC_A->AddLooseGameplayTag(ETGameplayTags::State_Stunned);
        }
        if (Scenario == EScenario::Wall)
        {
            AActor* Wall = World->SpawnActor<AActor>();
            if (!TestNotNull(TEXT("Wall"), Wall))
            {
                return false;
            }
            UBoxComponent* WallBox = NewObject<UBoxComponent>(Wall);
            Wall->SetRootComponent(WallBox);
            WallBox->SetBoxExtent(FVector(8, 120, 180));
            WallBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            WallBox->SetCollisionObjectType(ECC_WorldStatic);
            WallBox->SetCollisionResponseToAllChannels(ECR_Block);
            WallBox->RegisterComponent();
            Wall->SetActorLocation(Source->GetActorLocation() + FVector(90, 0, 0));
        }
        if (Scenario == EScenario::Cycle)
        {
            // 第二份Pawn查询形状必须去重，不能让一名目标一次扣120。
            UBoxComponent* Duplicate = NewObject<UBoxComponent>(A);
            Duplicate->SetupAttachment(A->GetRootComponent());
            Duplicate->SetBoxExtent(FVector(25));
            Duplicate->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Duplicate->SetCollisionObjectType(ECC_Pawn);
            Duplicate->SetCollisionResponseToAllChannels(ECR_Overlap);
            Duplicate->RegisterComponent();
        }
        FDelegateHandle Reentrant;
        if (Scenario == EScenario::ReentrantCancel)
        {
            const TWeakObjectPtr<UAbilitySystemComponent> WeakSource(ASC);
            Reentrant = ASC_A
                            ->GetGameplayAttributeValueChangeDelegate(
                                UETPoiseAttributeSet::GetPoiseAttribute())
                            .AddLambda(
                                [WeakSource, Handle](const FOnAttributeChangeData& Data)
                                {
                                    if (Data.NewValue < Data.OldValue && WeakSource.IsValid())
                                    {
                                        WeakSource->CancelAbilityHandle(Handle);
                                    }
                                });
        }
        if (!TestTrue(TEXT("Formal F request accepted"), Source->RequestBreakPulse()))
        {
            return false;
        }
        UAnimMontage* OldMontage = Ability->ActiveMontage;
        UAnimNotify* OldToken = Ability->ActiveCommitToken.Get();
        const int32 OldID = Ability->ActiveMontageInstanceID;
        if (!Advance(.1f))
        {
            return false;
        }
        TestEqual(TEXT("No pre-notify damage"), Poise(ASC_A), 100.0f);
        Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
        TestFalse(TEXT("Early same-identity callback does not seal"), Ability->bCommitAttempted);
        if (Scenario == EScenario::EndPlay)
        {
            if (!Fixture.EndPlayInTestWorld())
            {
                return false;
            }
            TestFalse(TEXT("EndPlay ends cast"), Ability->IsActive());
            TestEqual(TEXT("EndPlay before notify leaves target untouched"), Poise(ASC_A), 100.0f);
            continue;
        }
        if (Scenario == EScenario::CancelBefore)
        {
            ASC->CancelAbilityHandle(Handle);
        }
        if (Scenario == EScenario::Pause)
        {
            APlayerState* Pauser = PC->PlayerState;
            if (!IsValid(Pauser))
            {
                Pauser = World->SpawnActor<APlayerState>();
            }
            if (!TestNotNull(TEXT("Pauser"), Pauser))
            {
                return false;
            }
            World->GetWorldSettings()->SetPauserPlayerState(Pauser);
            if (!Advance(1.0f))
            {
                return false;
            }
            TestEqual(TEXT("Pause does not deliver commit"), Poise(ASC_A), 100.0f);
            TestFalse(TEXT("Pause does not seal commit"), Ability->bCommitAttempted);
            World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        }
        if (Scenario == EScenario::LeaveRange)
        {
            Place(A, FVector(400, 0, 0));
        }
        if (Scenario == EScenario::EnterRange)
        {
            Place(A, FVector(180, 0, 0));
        }
        if (!Advance(.45f))
        {
            return false;
        }
        const bool bRejectedTarget =
            Scenario == EScenario::Edge || Scenario == EScenario::Wall ||
            Scenario == EScenario::Team || Scenario == EScenario::Dead ||
            Scenario == EScenario::Waiting || Scenario == EScenario::Invulnerable ||
            Scenario == EScenario::Stunned || Scenario == EScenario::LeaveRange ||
            Scenario == EScenario::CancelBefore;
        TestEqual(TEXT("Real notify applies or skips by current target policy"), Poise(ASC_A),
                  bRejectedTarget ? 100.0f : 40.0f);
        Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
        TestEqual(TEXT("Duplicate or cancelled callback cannot add damage"), Poise(ASC_A),
                  bRejectedTarget ? 100.0f : 40.0f);
        TestEqual(TEXT("Poise does not damage source health"),
                  ASC->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                  100.0f);
        if (Scenario != EScenario::Dead)
        {
            TestEqual(TEXT("Poise does not damage target health"),
                      ASC_A->GetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute()),
                      100.0f);
        }
        TestEqual(TEXT("One accepted-cast cooldown only"),
                  ASC->GetTagCount(ETGameplayTags::Cooldown_BreakPulse), 1);
        if (Scenario == EScenario::ReentrantCancel)
        {
            TestFalse(TEXT("GE callback really cancels source"), Ability->IsActive());
            TestEqual(TEXT("Cancelled source stops remaining candidates"), Poise(ASC_B), 100.0f);
            ASC_A
                ->GetGameplayAttributeValueChangeDelegate(UETPoiseAttributeSet::GetPoiseAttribute())
                .Remove(Reentrant);
        }
        if (Scenario == EScenario::Cycle || Scenario == EScenario::CancelBefore)
        {
            if (Scenario == EScenario::Cycle)
            {
                TestEqual(TEXT("Both targets once"), Poise(ASC_B), 40.0f);
            }
            if (!Advance(6.0f) || !TestTrue(TEXT("Next cast accepted after real cooldown"),
                                            Source->RequestBreakPulse()))
            {
                return false;
            }
            Ability->HandleBreakPulseCommit(Source->GetMesh(), OldMontage, OldID, OldToken);
            TestFalse(TEXT("Old montage ID cannot seal a new activation"),
                      Ability->bCommitAttempted);
            if (!Advance(.55f))
            {
                return false;
            }
            TestEqual(TEXT("New activation resolves independently"), Poise(ASC_A),
                      Scenario == EScenario::Cycle ? 0.0f : 40.0f);
            if (Scenario == EScenario::Cycle)
            {
                TestTrue(TEXT("Zero Poise triggers existing formal Stagger"),
                         ASC_A->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
                TestEqual(TEXT("Second target also reaches zero"), Poise(ASC_B), 0.0f);
                if (!Advance(1.6f))
                {
                    return false;
                }
                TestFalse(TEXT("Reaction exits"),
                          ASC_A->HasMatchingGameplayTag(ETGameplayTags::State_Stunned));
                TestEqual(TEXT("Existing reaction restores current maximum"), Poise(ASC_A), 100.0f);
            }
        }
        if (Scenario == EScenario::Invulnerable)
        {
            ASC_A->RemoveLooseGameplayTag(ETGameplayTags::State_Invulnerable);
        }
        if (Scenario == EScenario::Stunned)
        {
            ASC_A->RemoveLooseGameplayTag(ETGameplayTags::State_Stunned);
        }
        // 不Destroy后推进再读旧ASC；EndPlay返回时对象仍由Fixture世界持有。
        if (!Fixture.EndPlayInTestWorld())
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
        TestFalse(TEXT("No stale cast after EndPlay"), Ability->IsActive());
        TestFalse(TEXT("No casting tag after EndPlay"),
                  ASC->HasMatchingGameplayTag(ETGameplayTags::State_CastingBreakPulse));
    }
    return !HasAnyErrors();
}
#endif