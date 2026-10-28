#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/ETRelicInventorySubsystem.h"
#include "Engine/GameInstance.h"
#include "Flow/ETRunSessionSubsystem.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FETRelicInventorySessionTest, "EmberTrial.M3.Inventory.Session",
                                 EAutomationTestFlags::EditorContext |
                                     EAutomationTestFlags::EngineFilter)

bool FETRelicInventorySessionTest::RunTest(const FString& Parameters)
{
    using EResult = EETRelicInventoryResult;
    const FName LifeId(TEXT("life_ember"));
    const FName EdgeId(TEXT("keen_edge"));
    // 只构造内存GI / 模型，不启动World，不向正式身体或磁盘资产写值。
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGameInstance> OtherGI(NewObject<UGameInstance>());
    TStrongObjectPtr<UETRelicInventorySubsystem> Inventory(
        NewObject<UETRelicInventorySubsystem>(GI.Get()));
    TStrongObjectPtr<UETRelicInventorySubsystem> Other(
        NewObject<UETRelicInventorySubsystem>(OtherGI.Get()));
    if (!TestTrue(TEXT("Real configured catalog loads"), Inventory->InitializeDataFromConfig()) ||
        !TestTrue(TEXT("Independent model loads"), Other->InitializeDataFromConfig()))
    {
        return false;
    }
    auto Snapshot = Inventory->GetSnapshot();
    TestTrue(TEXT("Ready version1 empty slots"), Snapshot.bReady && Snapshot.Revision == 1 &&
             Snapshot.Equipped.SlotA.IsNone() && Snapshot.Equipped.SlotB.IsNone());
    TestEqual(TEXT("Two starting owned IDs"), Snapshot.OwnedIds.Num(), 2);
    TestTrue(TEXT("Both configured IDs present"), Snapshot.OwnedIds.Contains(LifeId) &&
             Snapshot.OwnedIds.Contains(EdgeId));
    TestNotNull(TEXT("Definition cached"), Inventory->FindDefinition(LifeId));

    int32 Events = 0;
    const FDelegateHandle Observer = Inventory->OnChanged.AddLambda(
        [&](const FETRelicInventorySnapshot&) { ++Events; });
    auto Build = [&](EETRelicSlot Slot, FName Id, FETRelicInventoryPlan& Out)
    {
        return Inventory->TryBuildPlan(Slot, Id, 100.0f, 25.0f, Out).Result;
    };
    FETRelicInventoryPlan LifePlan;
    FETRelicInventoryPlan CompetingPlan;
    TestTrue(TEXT("Life preview accepted"), Build(EETRelicSlot::First, LifeId, LifePlan) == EResult::Success);
    TestTrue(TEXT("Second plan at same version"),
             Build(EETRelicSlot::Second, EdgeId, CompetingPlan) == EResult::Success);
    TestEqual(TEXT("Preview health120"), LifePlan.GetLoadoutPlan().AfterPreview.MaxHealth, 120.0f);
    TestTrue(TEXT("Preview leaves model unchanged"), Inventory->GetSnapshot().Equipped.SlotA.IsNone());
    TestEqual(TEXT("Preview emits no event"), Events, 0);

    // ---- 来源 / 版本 / NoChange；只提交模型，不宣称已给身体装备GE ----
    TestTrue(TEXT("Foreign model cannot commit"), Other->TryCommitPlan(LifePlan).Result == EResult::InvalidPlan);
    TestTrue(TEXT("Default plan rejected"),
             Inventory->TryCommitPlan(FETRelicInventoryPlan()).Result == EResult::InvalidPlan);
    TestTrue(TEXT("Logical commit accepted"), Inventory->TryCommitPlan(LifePlan).Result == EResult::Success);
    TestEqual(TEXT("One commit one event"), Events, 1);
    TestTrue(TEXT("Other model isolated"), Other->GetSnapshot().Equipped.SlotA.IsNone());
    TestTrue(TEXT("Competing old revision rejected"),
             Inventory->TryCommitPlan(CompetingPlan).Result == EResult::StalePlan);
    FETRelicInventoryPlan Scratch;
    TestTrue(TEXT("Same item is NoChange"), Build(EETRelicSlot::First, LifeId, Scratch) == EResult::NoChange);
    TestTrue(TEXT("NoChange has no reusable plan"),
             Inventory->TryCommitPlan(Scratch).Result == EResult::InvalidPlan);
    const auto Rejected = Inventory->TryBuildPlan(
        EETRelicSlot::Second, FName(TEXT("missing")), 100.0f, 25.0f, Scratch);
    TestTrue(TEXT("Unowned returns domain and rule"), Rejected.Result == EResult::RuleRejected &&
             Rejected.Rule == EETRelicLoadoutResult::NotOwned);
    TestEqual(TEXT("Refusals do not emit changes"), Events, 1);

    // ---- 同步观察者读取提交后快照，重入写得到Busy；不是递归二次提交 ----
    EResult NestedResult = EResult::Success;
    bool bReadCommitted = false;
    const FDelegateHandle Reentrant = Inventory->OnChanged.AddLambda(
        [&](const FETRelicInventorySnapshot& Changed)
        {
            bReadCommitted = Changed.Equipped.SlotB == EdgeId &&
                             Inventory->GetSnapshot().Revision == Changed.Revision;
            FETRelicInventoryPlan Nested;
            if (Build(EETRelicSlot::Second, NAME_None, Nested) == EResult::Success)
            {
                NestedResult = Inventory->TryCommitPlan(Nested).Result;
            }
        });
    TestTrue(TEXT("Fresh edge plan"), Build(EETRelicSlot::Second, EdgeId, Scratch) == EResult::Success);
    TestTrue(TEXT("Second logical commit"), Inventory->TryCommitPlan(Scratch).Result == EResult::Success);
    TestTrue(TEXT("Observer sees committed state"), bReadCommitted);
    TestTrue(TEXT("Nested commit blocked"), NestedResult == EResult::Busy);
    TestEqual(TEXT("No nested extra event"), Events, 2);
    Inventory->OnChanged.Remove(Reentrant);

    // ---- 模拟确认意图测试模型语义；真实PostLoad委托由短实机另查 ----
    for (const EETRunTravelIntent Intent : {EETRunTravelIntent::MainMenu,
                                           EETRunTravelIntent::Continue,
                                           EETRunTravelIntent::Retry})
    {
        TestTrue(TEXT("Plan before world transition"),
                 Build(EETRelicSlot::Second, NAME_None, Scratch) == EResult::Success);
        const uint64 BeforeRevision = Inventory->GetSnapshot().Revision;
        Inventory->ApplyConfirmedIntent(Intent);
        Snapshot = Inventory->GetSnapshot();
        TestTrue(TEXT("Preserved IDs; advanced revision"), Snapshot.Revision > BeforeRevision &&
                 Snapshot.Equipped.SlotA == LifeId && Snapshot.Equipped.SlotB == EdgeId);
        TestTrue(TEXT("Old world plan invalidated"),
                 Inventory->TryCommitPlan(Scratch).Result == EResult::StalePlan);
    }
    const uint64 BeforeNone = Inventory->GetSnapshot().Revision;
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::None);
    TestTrue(TEXT("No confirmed intent changes nothing"), Inventory->GetSnapshot().Revision == BeforeNone);
    Inventory->ApplyConfirmedIntent(EETRunTravelIntent::NewRun);
    Snapshot = Inventory->GetSnapshot();
    TestTrue(TEXT("NewRun keeps starting ownership, clears slots"), Snapshot.OwnedIds.Num() == 2 &&
             Snapshot.Equipped.SlotA.IsNone() && Snapshot.Equipped.SlotB.IsNone());

    Inventory->OnChanged.Remove(Observer);
    TestTrue(TEXT("Plan before end"), Build(EETRelicSlot::First, LifeId, Scratch) == EResult::Success);
    Inventory->Deinitialize();
    TestFalse(TEXT("End revokes readiness"), Inventory->GetSnapshot().bReady);
    TestTrue(TEXT("End clears ID snapshot"), Inventory->GetSnapshot().OwnedIds.IsEmpty());
    TestTrue(TEXT("End refuses old plan"), Inventory->TryCommitPlan(Scratch).Result == EResult::NotReady);
    Other->Deinitialize();
    return !HasAnyErrors();
}

#endif