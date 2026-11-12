#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ETAIController.h"
#include "AI/ETBlackboardKeys.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/ETGuardianCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Perception/AISense_Sight.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETGuardianCloseTrackingTest, "EmberTrial.M4.Guardian.CloseTracking",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETGuardianCloseTrackingTest::RunTest(const FString& Parameters)
{
    UBehaviorTree* Tree = LoadObject<UBehaviorTree>(
        nullptr, TEXT("/Game/EmberTrial/AI/BehaviorTrees/BT_ETMeleeEnemy.BT_ETMeleeEnemy"));
    if (!TestTrue(TEXT("Saved melee tree and blackboard"), Tree && Tree->BlackboardAsset))
    {
        return false;
    }

    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    if (!TestNotNull(TEXT("AI system"), World->CreateAISystem()))
    {
        return false;
    }

    AETAIController* Controller = World->SpawnActor<AETAIController>();
    APlayerController* PC = World->SpawnActor<APlayerController>();
    AETGuardianCharacter* Guardian = World->SpawnActor<AETGuardianCharacter>();
    AEmberTrialCharacter* Ordinary = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0, 700, 0), FRotator::ZeroRotator);
    AEmberTrialCharacter* Player = World->SpawnActor<AEmberTrialCharacter>(
        FVector(150, 0, 0), FRotator::ZeroRotator);
    if (!TestTrue(TEXT("Fixture actors spawned"), Controller && PC && Guardian && Ordinary && Player))
    {
        return false;
    }
    Controller->EnemyBehaviorTree = Tree;
    Controller->EnemyBlackboardAsset = Tree->BlackboardAsset;
    Controller->Possess(Guardian);
    PC->Possess(Player);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    if (!TestNotNull(TEXT("Blackboard initialized"), Controller->GetBlackboardComponent()))
    {
        return false;
    }

    const auto SendSight = [Controller, Player](FAIStimulus::FResult Result)
    {
        const FAIStimulus Stimulus(*GetDefault<UAISense_Sight>(), 1.0f, Player->GetActorLocation(),
                                   Controller->GetPawn()->GetActorLocation(), Result);
        Controller->HandleSightUpdated(Player, Stimulus);
    };
    const auto BoardTarget = [Controller]()
    {
        return Controller->GetBlackboardComponent()->GetValueAsObject(ETBlackboardKeys::TargetActor);
    };

    Player->SetActorLocation(FVector(-150, 0, 0));
    SendSight(FAIStimulus::SensingFailed);
    TestNull(TEXT("A player behind the guardian is not acquired without sight"), BoardTarget());

    Player->SetActorLocation(FVector(150, 0, 0));
    SendSight(FAIStimulus::SensingSucceeded);
    TestTrue(TEXT("Front sight acquires the player"), BoardTarget() == Player);

    Guardian->GetAbilitySystemComponent()->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
    Player->SetActorLocation(FVector(-150, 0, 0));
    SendSight(FAIStimulus::SensingFailed);
    Controller->UpdatePursuitDecision();
    TestTrue(TEXT("Blind angle during an attack retains the same player and blackboard goal"),
             BoardTarget() == Player && Controller->GetSensedPlayer() == Player &&
             Controller->bTrackingPlayerOutsideSight);
    Guardian->GetAbilitySystemComponent()->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestTrue(TEXT("The attack ending preserves the target for the next action"), BoardTarget() == Player);

    Player->SetActorLocation(FVector(-350, 0, 0));
    Controller->UpdatePursuitDecision();
    TestTrue(TEXT("Leaving the radius clears the retained target"), !BoardTarget() &&
             !Controller->bTrackingPlayerOutsideSight);

    Player->SetActorLocation(FVector(150, 0, 0));
    SendSight(FAIStimulus::SensingSucceeded);
    Player->SetActorLocation(FVector(-150, 0, 0));
    SendSight(FAIStimulus::SensingFailed);

    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Wall actor"), Wall))
    {
        return false;
    }
    UBoxComponent* Barrier = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Barrier);
    Barrier->InitBoxExtent(FVector(5, 80, 80));
    Barrier->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Barrier->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Barrier->RegisterComponent();
    Wall->SetActorLocation(FVector(-75, 0, 0));
    Controller->UpdatePursuitDecision();
    TestNull(TEXT("A wall breaks close tracking"), BoardTarget());
    Wall->Destroy();

    Player->SetActorLocation(FVector(150, 0, 0));
    SendSight(FAIStimulus::SensingSucceeded);
    SendSight(FAIStimulus::SensingFailed);
    SendSight(FAIStimulus::SensingSucceeded);
    TestTrue(TEXT("Normal sight resumes without replacing the target"), BoardTarget() == Player &&
             !Controller->bTrackingPlayerOutsideSight);

    Controller->Possess(Ordinary);
    Player->SetActorLocation(FVector(150, 700, 0));
    SendSight(FAIStimulus::SensingSucceeded);
    TestTrue(TEXT("Ordinary guard first acquires target"), BoardTarget() == Player);
    Player->SetActorLocation(FVector(-150, 700, 0));
    SendSight(FAIStimulus::SensingFailed);
    TestNull(TEXT("Ordinary guard still clears target on sight loss"), BoardTarget());

    Controller->Possess(Guardian);
    Player->SetActorLocation(FVector(150, 0, 0));
    SendSight(FAIStimulus::SensingSucceeded);
    Player->SetActorLocation(FVector(-150, 0, 0));
    SendSight(FAIStimulus::SensingFailed);
    Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 0.0f);
    TestTrue(TEXT("Player death clears retained identity immediately"), !BoardTarget() &&
             !Controller->bTrackingPlayerOutsideSight);

    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
