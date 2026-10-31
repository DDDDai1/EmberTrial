#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/ETRelicAltar.h"
#include "Interaction/ETRelicAltarSubsystem.h"
#include "Abilities/ETGA_LightAttack.h"
#include "Abilities/ETGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Characters/EmberTrialCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Framework/ETPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicAltarTest, "EmberTrial.M3.Interaction.RelicAltar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicAltarTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Fixture;
    if (!Fixture.CreateTestWorld(EWorldType::Game))
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    UWorld* World = Fixture.GetTestWorld();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
        FVector(0, 0, 200), FRotator::ZeroRotator, Spawn);
    AETPlayerController* PC = World->SpawnActor<AETPlayerController>();
    AETRelicAltar* Near = World->SpawnActor<AETRelicAltar>(
        FVector(100, 0, 200), FRotator::ZeroRotator, Spawn);
    AETRelicAltar* Far = World->SpawnActor<AETRelicAltar>(
        FVector(150, 0, 200), FRotator::ZeroRotator, Spawn);
    UETRelicAltarSubsystem* Registry = World->GetSubsystem<UETRelicAltarSubsystem>();
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("PC"), PC) ||
        !TestNotNull(TEXT("Near altar"), Near) || !TestNotNull(TEXT("Far altar"), Far) ||
        !TestNotNull(TEXT("World registry"), Registry))
    {
        return false;
    }
    Body->CombatTeam = EETCombatTeam::Player;
    Body->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
    PC->Possess(Body);
    TestNull(TEXT("Before BeginPlay no usable altar"), Registry->FindAvailableAltar(Body));
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    // 本测试没有地面也不推进世界时间；启动后冻结移动，避免重力改变几何夹具。
    Body->GetCharacterMovement()->DisableMovement();
    TestTrue(TEXT("Initial in range works without overlap event"), Registry->FindAvailableAltar(Body) == Near);
    TestTrue(TEXT("Direct eligibility agrees"), Near->CanUseAltar(Body));
    // ---- 本专项只测Core意图，不用无Viewport夹具伪装已打开UMG页面 ----
    {
        int32 IntentCount = 0;
        TWeakObjectPtr<AETRelicAltar> Received;
        const FDelegateHandle IntentHandle = PC->OnRelicAltarRequested.AddLambda(
            [&](AETRelicAltar* Altar)
            {
                ++IntentCount;
                Received = Altar;
            });
        const bool bConsumed = PC->TryRequestRelicAltarInteraction();
        PC->OnRelicAltarRequested.Remove(IntentHandle);
        TestTrue(TEXT("One request consumed and routed nearest once"),
            bConsumed && IntentCount == 1 && Received.Get() == Near);
    }
    Body->SetActorLocation(FVector(1000, 0, 200));
    TestNull(TEXT("Leaving range clears query"), Registry->FindAvailableAltar(Body));
    TestFalse(TEXT("Outside does not consume legacy interaction"), PC->TryRequestRelicAltarInteraction());
    Body->SetActorLocation(FVector(0, 0, 200));
    Near->bEnabled = false;
    TestTrue(TEXT("Disabled nearest falls back"), Registry->FindAvailableAltar(Body) == Far);
    Near->bEnabled = true;
    Near->MaxVerticalDistance = 10.0f;
    Near->SetActorLocation(FVector(100, 0, 220));
    TestFalse(TEXT("Different floor rejected"), Near->IsInReach(Body));
    Near->MaxVerticalDistance = 180.0f;
    Near->SetActorLocation(FVector(100, 0, 200));
    Near->InteractionRadius = 0.0f;
    TestFalse(TEXT("Invalid radius rejected"), Near->CanUseAltar(Body));
    Near->InteractionRadius = 180.0f;

    // 真实Visibility阻挡，使用NoCollision撤除，避免Destroy后物理场景尚未同步。
    AActor* Wall = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Wall"), Wall)) return false;
    UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(10, 100, 100));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(50, 0, 240));
    TestNull(TEXT("Wall blocks both altars"), Registry->FindAvailableAltar(Body));
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestTrue(TEXT("Wall removal permits nearest"), Registry->FindAvailableAltar(Body) == Near);

    UAbilitySystemComponent* ASC = Body->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("ASC"), ASC)) return false;
    ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
    TestNull(TEXT("Busy rejected"), Registry->FindAvailableAltar(Body));
    TestFalse(TEXT("Busy publishes no altar intent"), PC->TryRequestRelicAltarInteraction());
    ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
    APlayerState* Pauser = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Pauser"), Pauser)) return false;
    World->GetWorldSettings()->SetPauserPlayerState(Pauser);
    TestNull(TEXT("Paused opening rejected"), Registry->FindAvailableAltar(Body));
    TestFalse(TEXT("Paused request not consumed"), PC->TryRequestRelicAltarInteraction());
    World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    PC->UnPossess();
    TestNull(TEXT("Unpossessed body rejected"), Registry->FindAvailableAltar(Body));
    PC->Possess(Body);
    TestTrue(TEXT("Restored current body"), Registry->FindAvailableAltar(Body) == Near);
    Body->CombatTeam = EETCombatTeam::Enemy;
    TestNull(TEXT("Enemy cannot open"), Registry->FindAvailableAltar(Body));
    Body->CombatTeam = EETCombatTeam::Player;
    Near->Destroy();
    TestTrue(TEXT("Destroyed nearest falls back"), Registry->FindAvailableAltar(Body) == Far);
    Far->Destroy();
    TestNull(TEXT("No old altar retained"), Registry->FindAvailableAltar(Body));
    TestFalse(TEXT("Destroyed sources cannot consume request"), PC->TryRequestRelicAltarInteraction());
    Fixture.EndPlayInTestWorld();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif