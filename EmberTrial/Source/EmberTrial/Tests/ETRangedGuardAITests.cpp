#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ETAIController.h"
#include "AI/ETBlackboardKeys.h"
#include "AI/ETBTService_Pursuit.h"
#include "AI/ETBTTask_RangedAttack.h"
#include "Abilities/ETGA_RangedShot.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRangedGuardAITest, "EmberTrial.M4.AI.RangedGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRangedGuardAITest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UClass* ShotClass = LoadClass<UETGA_RangedShot>(nullptr,
        TEXT("/Game/EmberTrial/Abilities/GA_ET_RangedShot.GA_ET_RangedShot_C"));
    USkeletalMesh* Quinn = LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (!TestNotNull(TEXT("Saved ranged GA"), ShotClass) || !TestNotNull(TEXT("Quinn mesh"), Quinn))
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
    // FTestWorldWrapper creates a world without the AI system used by Blackboard initialization.
    if (!TestTrue(TEXT("Test world AI system"), World->CreateAISystem() != nullptr))
    {
        return false;
    }
    AETAIController* Controller = World->SpawnActor<AETAIController>();
    APlayerController* PlayerController = World->SpawnActor<APlayerController>();
    AEmberTrialCharacter* Guard = World->SpawnActor<AEmberTrialCharacter>();
    AEmberTrialCharacter* Player = World->SpawnActor<AEmberTrialCharacter>(
        FVector(850.0, 0.0, 0.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Guard controller"), Controller) || !TestNotNull(TEXT("Player controller"), PlayerController) ||
        !TestNotNull(TEXT("Guard body"), Guard) || !TestNotNull(TEXT("Player body"), Player))
    {
        return false;
    }

    // Run the real controller and attack task. Positions are controlled to test the gates without navigation baking.
    UBehaviorTree* Tree = NewObject<UBehaviorTree>(Controller);
    UBlackboardData* Blackboard = NewObject<UBlackboardData>(Tree);
    Blackboard->UpdatePersistentKey<UBlackboardKeyType_Object>(ETBlackboardKeys::TargetActor)->BaseClass =
        AActor::StaticClass();
    Blackboard->UpdatePersistentKey<UBlackboardKeyType_Bool>(ETBlackboardKeys::CanPursue);
    Blackboard->UpdatePersistentKey<UBlackboardKeyType_Float>(ETBlackboardKeys::PursuitStopDistance);
    Blackboard->UpdatePersistentKey<UBlackboardKeyType_Bool>(ETBlackboardKeys::CanMeleeAttack);
    Blackboard->UpdatePersistentKey<UBlackboardKeyType_Bool>(ETBlackboardKeys::CanRangedAttack);
    Blackboard->UpdatePersistentKey<UBlackboardKeyType_Vector>(ETBlackboardKeys::RangedRetreatGoal);

    UBTComposite_Selector* Selector = NewObject<UBTComposite_Selector>(Tree);
    Selector->Services.Add(NewObject<UETBTService_Pursuit>(Tree));
    Selector->Children.AddDefaulted_GetRef().ChildTask = NewObject<UETBTTask_RangedAttack>(Tree);
    Selector->Children.AddDefaulted_GetRef().ChildTask = NewObject<UBTTask_Wait>(Tree);
    Tree->RootNode = Selector;
    Tree->BlackboardAsset = Blackboard;
    Controller->EnemyBehaviorTree = Tree;
    Controller->EnemyBlackboardAsset = Blackboard;
    Controller->bUseRangedAttack = true;
    Controller->PursuitStopDistance = 650.0f;
    Controller->PursuitResumeDistance = 780.0f;

    Guard->CombatTeam = EETCombatTeam::Enemy;
    Guard->RangedShotAbilityClass = ShotClass;
    Guard->GetMesh()->SetSkeletalMeshAsset(Quinn);
    Guard->GetMesh()->SetAnimInstanceClass(UAnimInstance::StaticClass());
    Guard->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Player->CombatTeam = EETCombatTeam::Player;
    PlayerController->Possess(Player);
    Controller->Possess(Guard);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    for (AEmberTrialCharacter* Body : {Guard, Player})
    {
        Body->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Body->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
        Body->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Body->GetCharacterMovement()->SetComponentTickEnabled(false);
    }

    UBlackboardComponent* Board = Controller->GetBlackboardComponent();
    if (!TestNotNull(TEXT("Configured blackboard"), Board) ||
        !TestTrue(TEXT("Behavior tree started"), Controller->bTreeStarted))
    {
        return false;
    }
    Controller->SetSensedPlayer(Player, TEXT("Automation"));
    const auto PlacePlayer = [Player, Controller](double X)
    {
        Player->SetActorLocation(FVector(X, 0.0, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
        Controller->UpdatePursuitDecision();
    };
    const auto CanFire = [Board]() { return Board->GetValueAsBool(ETBlackboardKeys::CanRangedAttack); };
    const auto CanMove = [Board]() { return Board->GetValueAsBool(ETBlackboardKeys::CanPursue); };
    const auto HasRetreat = [Board]() { return Board->IsVectorValueSet(ETBlackboardKeys::RangedRetreatGoal); };

    PlacePlayer(850.0);
    TestTrue(TEXT("Approach at distance"), CanMove() && !CanFire() && !HasRetreat());
    PlacePlayer(600.0);
    TestTrue(TEXT("Hold and aim inside stop distance"), !CanMove() && CanFire());
    PlacePlayer(720.0);
    TestTrue(TEXT("Hold until resume threshold"), !CanMove() && CanFire());
    PlacePlayer(790.0);
    TestTrue(TEXT("Pursuit resumes beyond threshold"), CanMove() && !CanFire());
    PlacePlayer(200.0);
    TestTrue(TEXT("Retreat gate excludes pursuit and fire"), HasRetreat() && !CanMove() && !CanFire());
    TestTrue(TEXT("Retreat goal points away"), Board->GetValueAsVector(ETBlackboardKeys::RangedRetreatGoal).X < 0.0);
    PlacePlayer(400.0);
    TestTrue(TEXT("Retreat persists across lower bound"), HasRetreat());
    PlacePlayer(600.0);
    TestTrue(TEXT("Clear retreat after separation"), !HasRetreat() && CanFire());

    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Sight blocker"), Wall))
    {
        return false;
    }
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->InitBoxExtent(FVector(3.0, 80.0, 80.0));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(300.0, 0.0, 0.0));
    Controller->UpdatePursuitDecision();
    TestFalse(TEXT("Wall closes fire gate"), CanFire());
    Wall->Destroy();
    Controller->UpdatePursuitDecision();
    if (!TestTrue(TEXT("Clear aim restored"), CanFire()))
    {
        return false;
    }

    for (int32 Index = 0; Index < 110; ++Index)
    {
        if (!Fixture.TickTestWorld(.01f))
        {
            Fixture.ForwardErrorMessages(this);
            return false;
        }
    }
    const UAbilitySystemComponent* GuardASC = Guard->GetAbilitySystemComponent();
    UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent();
    const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
    TestEqual(TEXT("Real behavior tree task hit the player once"), PlayerASC->GetNumericAttribute(Health), 88.0f);
    TestTrue(TEXT("GAS cooldown keeps the fire gate closed"),
             GuardASC->HasMatchingGameplayTag(ETGameplayTags::Cooldown_RangedShot) && !CanFire());

    Controller->ClearSensedPlayer(TEXT("Automation"));
    TestTrue(TEXT("Target cleared with all ranged gates closed"),
             !Controller->GetSensedPlayer() && !CanFire() && !CanMove() && !HasRetreat());
    if (!Fixture.EndPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
