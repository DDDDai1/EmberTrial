#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/ETRelicAltarAccess.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Attributes/ETCombatAttributeSet.h"
#include "Engine/GameInstance.h"
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
    // ---- C1：真实暂停中的类型化上下文；原非暂停入口继续拒绝 ----
    {
        using EResult = EETRelicInventoryResult;
        UETRelicInventorySubsystem* Inventory = World->GetGameInstance()
            ->GetSubsystem<UETRelicInventorySubsystem>();
        UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
        AETRelicAltar* Source = World->SpawnActor<AETRelicAltar>(
            FVector(100, 0, 200), FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Access inventory"), Inventory) ||
            !TestNotNull(TEXT("Access equipment"), Equipment) ||
            !TestNotNull(TEXT("Access source"), Source)) return false;

        const FName LifeId(TEXT("life_ember"));
        const FName EdgeId(TEXT("keen_edge"));
        const FGameplayAttribute Health = UETHealthAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Max = UETHealthAttributeSet::GetMaxHealthAttribute();
        const FGameplayAttribute Bonus = UETCombatAttributeSet::GetLightAttackBonusAttribute();
        auto Access = FETRelicAltarAccess::TryOpen(PC, Source);
        if (!TestTrue(TEXT("Eligible nonpaused context opens"), Access.IsValid())) return false;
        TestFalse(TEXT("Context does not authorize unpaused commit"), Access->CanCommitFor(Equipment));
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        TestFalse(TEXT("No new context manufactured inside ordinary pause"),
            FETRelicAltarAccess::TryOpen(PC, Source).IsValid());
        TestTrue(TEXT("Opened context permits paused current body"), Access->CanCommitFor(Equipment));
        auto EquipAt = [&](EETRelicSlot Slot, FName Id)
        {
            return Equipment->TryEquipAtAltar(*Access, Slot, Id, Inventory->GetSnapshot().Revision).Result;
        };
        const uint64 BeforeRevision = Inventory->GetSnapshot().Revision;
        TestTrue(TEXT("Legacy paused write still rejected"), Equipment->TryEquip(
            EETRelicSlot::First, LifeId, BeforeRevision).Result == EResult::NotReady);

        ASC->SetNumericAttributeBase(Health, 75.0f);
        EResult Nested = EResult::Success;
        const FDelegateHandle ReentryHandle = ASC->GetGameplayAttributeValueChangeDelegate(Max).AddLambda(
            [&](const FOnAttributeChangeData&) { Nested = EquipAt(EETRelicSlot::Second, EdgeId); });
        TestTrue(TEXT("Paused life equip commits once"), EquipAt(EETRelicSlot::First, LifeId) == EResult::Success);
        ASC->GetGameplayAttributeValueChangeDelegate(Max).Remove(ReentryHandle);
        TestTrue(TEXT("Typed entry keeps reentrant Busy protection"), Nested == EResult::Busy);
        TestEqual(TEXT("Life equip max120"), ASC->GetNumericAttribute(Max), 120.0f);
        TestEqual(TEXT("Life equip does not heal"), ASC->GetNumericAttribute(Health), 75.0f);
        TestTrue(TEXT("Only one revision advanced"), Inventory->GetSnapshot().Revision == BeforeRevision + 1);
        TestTrue(TEXT("Old viewed version rejected"), Equipment->TryEquipAtAltar(
            *Access, EETRelicSlot::Second, EdgeId, BeforeRevision).Result == EResult::StalePlan);
        const uint64 StableRevision = Inventory->GetSnapshot().Revision;
        TestTrue(TEXT("Same equipped ID is NoChange"), EquipAt(EETRelicSlot::First, LifeId) == EResult::NoChange);
        TestTrue(TEXT("NoChange leaves version intact"), Inventory->GetSnapshot().Revision == StableRevision);
        TestTrue(TEXT("Duplicate other slot rejected"), EquipAt(EETRelicSlot::Second, LifeId) == EResult::RuleRejected);
        TestTrue(TEXT("Unowned ID rejected"), EquipAt(EETRelicSlot::Second, FName(TEXT("missing"))) == EResult::RuleRejected);
        TestTrue(TEXT("Second legal item commits"), EquipAt(EETRelicSlot::Second, EdgeId) == EResult::Success);
        TestEqual(TEXT("Real attack bonus5"), ASC->GetNumericAttribute(Bonus), 5.0f);

        const uint64 RejectedRevision = Inventory->GetSnapshot().Revision;
        auto RejectCurrent = [&]()
        {
            TestFalse(TEXT("Changed context not usable"), Access->CanCommitFor(Equipment));
            TestTrue(TEXT("Changed context write rejected"), EquipAt(EETRelicSlot::First, NAME_None) == EResult::NotReady);
            TestTrue(TEXT("Rejected context has no revision change"), Inventory->GetSnapshot().Revision == RejectedRevision);
            TestEqual(TEXT("Rejected context preserves max120"), ASC->GetNumericAttribute(Max), 120.0f);
        };
        Source->bEnabled = false;
        RejectCurrent();
        Source->bEnabled = true;
        Body->SetActorLocation(FVector(1000, 0, 200));
        RejectCurrent();
        Body->SetActorLocation(FVector(0, 0, 200));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        RejectCurrent();
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ASC->AddLooseGameplayTag(ETGameplayTags::State_Attacking);
        RejectCurrent();
        ASC->RemoveLooseGameplayTag(ETGameplayTags::State_Attacking);
        PC->UnPossess();
        TestFalse(TEXT("Lost possession invalidates current-body permission"), Access->CanCommitFor(Equipment));
        PC->Possess(Body);
        Equipment->RefreshEquipmentState();
        TestTrue(TEXT("Current identity and space restored"), Access->CanCommitFor(Equipment));

        ASC->SetNumericAttributeBase(Health, 110.0f);
        TestTrue(TEXT("Paused unequip uses same transaction"), EquipAt(EETRelicSlot::First, NAME_None) == EResult::Success);
        TestEqual(TEXT("Unequip clamps current100"), ASC->GetNumericAttribute(Health), 100.0f);
        TestEqual(TEXT("Unequip clamps base100"), ASC->GetNumericAttributeBase(Health), 100.0f);
        TestTrue(TEXT("Reequip succeeds"), EquipAt(EETRelicSlot::First, LifeId) == EResult::Success);
        TestEqual(TEXT("Reequip grants no free heal"), ASC->GetNumericAttribute(Health), 100.0f);
        Access->Close();
        TestFalse(TEXT("Closed page context remains revoked"), Access->CanCommitFor(Equipment));
        TestTrue(TEXT("Closed context cannot unequip"), EquipAt(EETRelicSlot::First, NAME_None) == EResult::NotReady);
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
        auto Fresh = FETRelicAltarAccess::TryOpen(PC, Source);
        if (!TestTrue(TEXT("New page needs a fresh context"), Fresh.IsValid())) return false;
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        TestTrue(TEXT("Fresh context usable before source destruction"), Fresh->CanCommitFor(Equipment));
        Source->Destroy();
        TestFalse(TEXT("Destroyed original source never authorizes"), Fresh->CanCommitFor(Equipment));
        Fresh->Close();
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);

        AETRelicAltar* DeathSource = World->SpawnActor<AETRelicAltar>(
            FVector(100, 0, 200), FRotator::ZeroRotator, Spawn);
        auto DeathAccess = FETRelicAltarAccess::TryOpen(PC, DeathSource);
        if (!TestTrue(TEXT("Live body opens death-case context"), DeathAccess.IsValid())) return false;
        World->GetWorldSettings()->SetPauserPlayerState(Pauser);
        const uint64 BeforeDeathRevision = Inventory->GetSnapshot().Revision;
        ASC->SetNumericAttributeBase(Health, 0.0f);
        TestFalse(TEXT("Dead body cannot retain permission"), DeathAccess->CanCommitFor(Equipment));
        TestTrue(TEXT("Dead body write rejected"), Equipment->TryEquipAtAltar(
            *DeathAccess, EETRelicSlot::First, NAME_None, BeforeDeathRevision).Result == EResult::NotReady);
        TestTrue(TEXT("Death rejection leaves inventory intact"),
            Inventory->GetSnapshot().Revision == BeforeDeathRevision);
        DeathAccess->Close();
        World->GetWorldSettings()->SetPauserPlayerState(nullptr);
    }
    Fixture.EndPlayInTestWorld();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif