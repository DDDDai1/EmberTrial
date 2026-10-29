#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicPresenter.h"
#include "Abilities/ETGA_LightAttack.h"
#include "AbilitySystemComponent.h"
#include "Attributes/ETHealthAttributeSet.h"
#include "Characters/EmberTrialCharacter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/ETRelicEquipmentComponent.h"
#include "Inventory/ETRelicInventorySubsystem.h"
#include "Lifecycle/ETUILocalPlayerSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicPresenterTest, "EmberTrial.UI.Relics.Presenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FETRelicPresenterTest::RunTest(const FString& Parameters)
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
    auto SpawnBody = [&](float X)
    {
        AEmberTrialCharacter* Body = World->SpawnActor<AEmberTrialCharacter>(
            FVector(X, 0, 200), FRotator::ZeroRotator, Spawn);
        if (Body)
        {
            Body->CombatTeam = EETCombatTeam::Player;
            Body->LightAttackAbilityClass = UETGA_LightAttack::StaticClass();
        }
        return Body;
    };
    AEmberTrialCharacter* Body = SpawnBody(0);
    AEmberTrialCharacter* OtherBody = SpawnBody(400);
    APlayerController* PC = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Body"), Body) || !TestNotNull(TEXT("Other body"), OtherBody) ||
        !TestNotNull(TEXT("PC"), PC) || !TestNotNull(TEXT("Engine"), GEngine))
    {
        return false;
    }
    // 跟随已验证的UI身份夹具，不创建Viewport或额外初始化第二份GI。
    TStrongObjectPtr<ULocalPlayer> LocalPlayer(NewObject<ULocalPlayer>(GEngine));
    LocalPlayer->SwitchController(PC);
    PC->Possess(Body);
    TStrongObjectPtr<UETUILocalPlayerSubsystem> Subsystem(
        NewObject<UETUILocalPlayerSubsystem>(LocalPlayer.Get()));
    Subsystem->PlayerControllerChanged(PC);
    TStrongObjectPtr<UETRelicPresenter> Presenter(NewObject<UETRelicPresenter>());
    Presenter->StartObserving(Subsystem.Get());
    TestTrue(TEXT("Before body BeginPlay waits"), Presenter->GetSnapshot().State == EETRelicPanelState::Waiting);
    if (!Fixture.BeginPlayInTestWorld())
    {
        Fixture.ForwardErrorMessages(this);
        return false;
    }
    TestTrue(TEXT("Zero modifier restore still publishes final readiness"),
        Presenter->GetSnapshot().State == EETRelicPanelState::Ready &&
        Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::Available);
    TestEqual(TEXT("First life comparison120"), Presenter->GetSnapshot().AfterMax, 120.0f);
    TestEqual(TEXT("Two stable items"), Presenter->GetItems().Num(), 2);
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    UETRelicListItem* LifeItem = Presenter->FindItem(LifeId);
    UETRelicListItem* EdgeItem = Presenter->FindItem(EdgeId);
    UETRelicInventorySubsystem* Inventory = World->GetGameInstance()->GetSubsystem<UETRelicInventorySubsystem>();
    UETRelicEquipmentComponent* Equipment = Body->FindComponentByClass<UETRelicEquipmentComponent>();
    if (!TestNotNull(TEXT("Life item"), LifeItem) || !TestNotNull(TEXT("Edge item"), EdgeItem) ||
        !TestNotNull(TEXT("Inventory"), Inventory) || !TestNotNull(TEXT("Equipment"), Equipment))
    {
        return false;
    }
    int32 ViewEvents = 0;
    const FDelegateHandle ViewHandle = Presenter->OnChanged.AddLambda([&](const FETRelicPanelData&) { ++ViewEvents; });
    auto Equip = [&](EETRelicSlot Slot, FName Id)
    {
        return Equipment->TryEquip(Slot, Id, Inventory->GetSnapshot().Revision).Result;
    };
    TestTrue(TEXT("Real life commit"), Equip(EETRelicSlot::First, LifeId) == EETRelicInventoryResult::Success);
    TestEqual(TEXT("One settled view no temporary Waiting"), ViewEvents, 1);
    TestTrue(TEXT("Badge changes on same item"), Presenter->FindItem(LifeId) == LifeItem &&
        LifeItem->GetSnapshot().bEquippedInSlotA);
    TestTrue(TEXT("Same slot now already equipped"),
        Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::AlreadyEquipped);
    TestTrue(TEXT("Select edge"), Presenter->SelectId(EdgeId));
    TestTrue(TEXT("Select slotB"), Presenter->SetPreviewSlotB(true));
    TestEqual(TEXT("Edge comparison30"), Presenter->GetSnapshot().AfterAttack, 30.0f);
    FETRelicListQuery Query;
    Query.Sort = EETRelicListSort::StableId;
    Presenter->SetQuery(Query);
    TestTrue(TEXT("Resort preserves selected ID and both Item objects"),
        Presenter->GetSnapshot().SelectedId == EdgeId && Presenter->FindItem(LifeId) == LifeItem &&
        Presenter->FindItem(EdgeId) == EdgeItem);
    Presenter->StartObserving(Subsystem.Get());
    TestTrue(TEXT("Restart same presenter retains item identity"), Presenter->FindItem(EdgeId) == EdgeItem);
    TestTrue(TEXT("Real edge commit"), Equip(EETRelicSlot::Second, EdgeId) == EETRelicInventoryResult::Success);
    TestEqual(TEXT("Committed preview current30"), Presenter->GetSnapshot().BeforeAttack, 30.0f);

    Equipment->Deactivate();
    TestTrue(TEXT("Deactivate unavailable comparison"),
        Presenter->GetSnapshot().PreviewState == EETRelicPreviewState::Unavailable);
    Equipment->Activate(false);
    TestTrue(TEXT("Reactivate final readiness"), Presenter->GetSnapshot().State == EETRelicPanelState::Ready);
    UAbilitySystemComponent* OldASC = Body->GetAbilitySystemComponent();
    PC->UnPossess();
    TestTrue(TEXT("NoPawn clears comparison"), Presenter->GetSnapshot().State == EETRelicPanelState::NoPawn &&
        Presenter->GetSnapshot().AfterMax == 0.0f);
    const int32 BeforeOldBody = ViewEvents;
    OldASC->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 75.0f);
    TestEqual(TEXT("Old body cannot drive current presenter"), ViewEvents, BeforeOldBody);
    PC->Possess(OtherBody);
    TestTrue(TEXT("New body restored and observed"), Presenter->GetSnapshot().State == EETRelicPanelState::Ready &&
        Presenter->BoundBody.Get() == OtherBody);
    // 只走真实Possess链，不让测试手动Refresh掩盖换控后的装备恢复遗漏。
    UAbilitySystemComponent* OtherASC = OtherBody->GetAbilitySystemComponent();
    TestEqual(TEXT("New body restores equipped maximum"),
        OtherASC->GetNumericAttribute(UETHealthAttributeSet::GetMaxHealthAttribute()), 120.0f);
    TestEqual(TEXT("New body receives initial equipped refill"),
        OtherASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 120.0f);
    TestEqual(TEXT("New body exposes equipped attack preview"), Presenter->GetSnapshot().BeforeAttack, 30.0f);
    PC->Possess(Body);
    TestTrue(TEXT("Returning body observed without manual refresh"),
        Presenter->GetSnapshot().State == EETRelicPanelState::Ready && Presenter->BoundBody.Get() == Body);
    TestEqual(TEXT("Returning body does not refill health"),
        OldASC->GetNumericAttribute(UETHealthAttributeSet::GetHealthAttribute()), 75.0f);
    PC->Possess(OtherBody);
    TestTrue(TEXT("Second switch remains ready"),
        Presenter->GetSnapshot().State == EETRelicPanelState::Ready && Presenter->BoundBody.Get() == OtherBody);
    Presenter->Shutdown();
    TestTrue(TEXT("Shutdown releases displayed items"), Presenter->GetItems().IsEmpty() &&
        Presenter->GetSnapshot().State == EETRelicPanelState::Unavailable);
    const int32 BeforeStopped = ViewEvents;
    OtherBody->GetAbilitySystemComponent()->SetNumericAttributeBase(UETHealthAttributeSet::GetHealthAttribute(), 80.0f);
    TestEqual(TEXT("Stopped presenter receives no source events"), ViewEvents, BeforeStopped);
    Presenter->OnChanged.Remove(ViewHandle);
    TestFalse(TEXT("Inventory handle removed"), Presenter->InventoryHandle.IsValid());
    TestFalse(TEXT("Equipment handle removed"), Presenter->EquipmentHandle.IsValid());
    Subsystem->Deinitialize();
    Fixture.EndPlayInTestWorld();
    Fixture.ForwardErrorMessages(this);
    return !HasAnyErrors();
}

#endif
